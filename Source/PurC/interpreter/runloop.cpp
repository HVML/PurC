/*
 * @file runloop.cpp
 * @author XueShuming
 * @date 2021/12/14
 * @brief The C api for RunLoop.
 *
 * Copyright (C) 2021, 2022 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of PurC (short for Purring Cat), an HVML interpreter.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "purc.h"

#include "config.h"

#include "purc-runloop.h"
#include "private/errors.h"
#include "private/interpreter.h"
#include "private/instance.h"
#include "private/runners.h"
#include "private/sorted-array.h"
#include "internal.h"

#include <wtf/Threading.h>
#include <wtf/RunLoop.h>
#include <wtf/threads/BinarySemaphore.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>


purc_runloop_t purc_runloop_get_current(void)
{
    return (purc_runloop_t)&RunLoop::current();
}

void purc_runloop_run(void)
{
    RunLoop::run();
}

void purc_runloop_stop(purc_runloop_t runloop)
{
    if (runloop) {
        ((RunLoop*)runloop)->stop();
    }
}

void purc_runloop_wakeup(purc_runloop_t runloop)
{
    if (runloop) {
        ((RunLoop*)runloop)->wakeUp();
    }
}

void purc_runloop_dispatch(purc_runloop_t runloop, purc_runloop_func func,
        void* ctxt)
{
    if (runloop) {
        ((RunLoop*)runloop)->dispatch([func, ctxt]() {
            func(ctxt);
        });
    }
}

void purc_runloop_dispatch_after(purc_runloop_t runloop, long time_ms,
        purc_runloop_func func, void *ctxt)
{
    if (runloop) {
        ((RunLoop*)runloop)->dispatchAfter(
            PurCWTF::Seconds::fromMilliseconds(time_ms),
            [func, ctxt]() {
                func(ctxt);
            }
        );
    }
}

void purc_runloop_set_idle_func(purc_runloop_t runloop, purc_runloop_func func,
        void* ctxt)
{
    if (runloop) {
        ((RunLoop*)runloop)->setIdleCallback([func, ctxt]() {
            func(ctxt);
        });
    }
}

static int
to_runloop_io_event(GIOCondition condition)
{
    int event = 0;;
    if (condition & G_IO_IN) {
        event |= PCRUNLOOP_IO_IN;
    }
    if (condition & G_IO_PRI) {
        event |= PCRUNLOOP_IO_PRI;
    }
    if (condition & G_IO_OUT) {
        event |= PCRUNLOOP_IO_OUT;
    }
    if (condition & G_IO_ERR) {
        event |= PCRUNLOOP_IO_ERR;
    }
    if (condition & G_IO_HUP) {
        event |= PCRUNLOOP_IO_HUP;
    }
    if (condition & G_IO_NVAL) {
        event |= PCRUNLOOP_IO_NVAL;
    }
    return event;
}

static GIOCondition
to_gio_condition(int event)
{
    gushort condition = 0;
    if (event & PCRUNLOOP_IO_IN) {
        condition |= G_IO_IN;
    }
    if (event & PCRUNLOOP_IO_PRI) {
        condition |= G_IO_PRI;
    }
    if (event & PCRUNLOOP_IO_OUT) {
        condition |= G_IO_OUT;
    }
    if (event & PCRUNLOOP_IO_ERR) {
        condition |= G_IO_ERR;
    }
    if (event & PCRUNLOOP_IO_HUP) {
        condition |= G_IO_HUP;
    }
    if (event & PCRUNLOOP_IO_NVAL) {
        condition |= G_IO_NVAL;
    }
    return (GIOCondition) condition;
}

uintptr_t purc_runloop_add_fd_monitor(purc_runloop_t runloop, int fd,
        int event, purc_runloop_io_callback callback,
        void *ctxt)
{
    RunLoop *runLoop = (RunLoop*)runloop;

    return runLoop->addFdMonitor(fd, to_gio_condition(event),
            [callback, ctxt] (gint fd, GIOCondition condition) -> gboolean {
            PC_ASSERT(pcintr_get_runloop()==nullptr);
            int io_event = to_runloop_io_event(condition);
            return callback(fd, io_event, ctxt);
        });
}

void purc_runloop_remove_fd_monitor(purc_runloop_t runloop, uintptr_t handle)
{
    if (!runloop) {
        runloop = purc_runloop_get_current();
    }
    ((RunLoop*)runloop)->removeFdMonitor(handle);
}

void purc_runloop_set_timeout(purc_runloop_t runloop,
        purc_runloop_timeout_callback callback, void *ctxt, uint32_t interval)
{
    if (!runloop) {
        runloop = purc_runloop_get_current();
    }
    ((RunLoop*)runloop)->setTimeout(G_SOURCE_FUNC(callback), ctxt, interval);
}

extern "C" purc_atom_t
pcrun_create_inst_thread(const char *app_name, const char *runner_name,
        purc_cond_handler cond_handler,
        struct purc_instance_extra_info *extra_info, void **th)
{
    purc_atom_t atom = 0;
    BinarySemaphore semaphore;

    RefPtr<Thread> inst_th =
        Thread::create("hvml-instance", [&] {
                int ret = purc_init_ex(PURC_MODULE_HVML,
                        app_name, runner_name, extra_info);

                if (ret != PURC_ERROR_OK) {
                    semaphore.signal();
                }
                else {
                    struct pcinst *inst = pcinst_current();
                    assert(inst && inst->intr_heap);
                    purc_atom_t my_atom;
                    atom = my_atom = inst->intr_heap->move_buff;

                    purc_cond_handler my_handler = cond_handler;

#if USE(PTHREADS)
                    pthread_t *my_th = (pthread_t *)malloc(sizeof(pthread_t));
                    *my_th = pthread_self();
                    *th = (void *)my_th;
#else
#error "Need code when not using PThreads"
#endif
                    if (cond_handler) {
                        cond_handler(PURC_COND_STARTED,
                                (void *)(uintptr_t)atom, extra_info);
                    }
                    semaphore.signal();

                    purc_run(my_handler);

                    pcrun_notify_instmgr(PCRUN_EVENT_inst_stopped, my_atom);
                    if ((my_handler = inst->intr_heap->cond_handler)) {
                        my_handler(PURC_COND_STOPPED,
                                (void *)(uintptr_t)my_atom, NULL);
                    }

                    purc_cleanup();
                }
            });

    inst_th->detach();
    semaphore.wait();

    return atom;
}

static void my_sa_free(void *sortv, void *data)
{
    (void)sortv;
    if (data)
        free(data);
}

#define MAIN_RUNLOOP_THREAD_NAME    "__purc_main_runloop_thread"

static RefPtr<Thread> _main_thread;
static pthread_once_t _main_once_control = PTHREAD_ONCE_INIT;
static purc_runloop_t _main_runloop;

static void _runloop_init_main(void)
{
    BinarySemaphore semaphore;
    purc_atom_t rid_main = pcinst_current()->endpoint_atom;
    PC_ASSERT(rid_main);

    _main_thread = Thread::create(MAIN_RUNLOOP_THREAD_NAME, [&] {
            RunLoop& runloop = RunLoop::current();
            _main_runloop = &runloop;

            struct instmgr_info info = { rid_main, 0, NULL };

            int ret;
            purc_atom_t atom;

            ret = purc_init_ex(PURC_MODULE_EJSON,
                    PCRUN_INSTMGR_APP_NAME, PCRUN_INSTMGR_RUN_NAME, NULL);
            if (ret != PURC_ERROR_OK) {
                purc_log_error("Failed to init InstMgr\n");
                return;
            }

            atom = purc_inst_create_move_buffer(PCINST_MOVE_BUFFER_FLAG_NONE,
                    PCINTR_MOVE_BUFFER_SIZE >> 1);
            if (atom == 0) {
                purc_log_error("Failed to create move buffer for InstMgr.\n");
                return;
            }
            semaphore.signal();

            pcinst_current()->is_instmgr = 1;
            info.sa_insts = pcutils_sorted_array_create(SAFLAG_DEFAULT, 0,
                    my_sa_free, NULL);

            purc_runloop_func func = pcrun_instmgr_handle_message;
            runloop.setIdleCallback([func, &info]() {
                    func(&info);
                    });

            runloop.run();

            pcutils_sorted_array_destroy(info.sa_insts);

            size_t n = purc_inst_destroy_move_buffer();
            PC_DEBUG("InstMgr is quiting, %u messages discarded\n",
                    (unsigned)n);

            purc_cleanup();
            });
    semaphore.wait();
}

static void _runloop_stop_main(void)
{
    if (_main_thread) {
        ((RunLoop*)_main_runloop)->stop();
        _main_thread->waitForCompletion();
    }
}

static int _init_once(void)
{
    RunLoop::initializeMain();
    atexit(_runloop_stop_main);
    return 0;
}

static int _init_instance(struct pcinst* curr_inst,
        const purc_instance_extra_info* extra_info)
{
    UNUSED_PARAM(curr_inst);
    UNUSED_PARAM(extra_info);

    int r;
    r = pthread_once(&_main_once_control, _runloop_init_main);
    PC_ASSERT(r == 0);

    return 0;
}

static void _cleanup_instance(struct pcinst* curr_inst)
{
    UNUSED_PARAM(curr_inst);
}

struct pcmodule _module_runloop = {
    .id              = PURC_HAVE_HVML,
    .module_inited   = 0,

    .init_once              = _init_once,
    .init_instance          = _init_instance,
    .cleanup_instance       = _cleanup_instance,
};

