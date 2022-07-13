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

static purc_runloop_io_event
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
    return (purc_runloop_io_event)event;
}

static GIOCondition
to_gio_condition(purc_runloop_io_event event)
{
    int condition = 0;
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
    return (GIOCondition)condition;
}

uintptr_t purc_runloop_add_fd_monitor(purc_runloop_t runloop, int fd,
        purc_runloop_io_event event, purc_runloop_io_callback callback,
        void *ctxt)
{
    pcintr_coroutine_t co = pcintr_get_coroutine();
    PC_ASSERT(co);
    PC_ASSERT(pcintr_get_runloop() == runloop);

    RunLoop *runLoop = (RunLoop*)runloop;

    return runLoop->addFdMonitor(fd, to_gio_condition(event),
            [callback, ctxt, runLoop, co] (gint fd, GIOCondition condition) -> gboolean {
            PC_ASSERT(pcintr_get_runloop()==nullptr);
            purc_runloop_io_event io_event;
            io_event = to_runloop_io_event(condition);
            callback(fd, io_event, ctxt);
            return true;
        });
}

void purc_runloop_remove_fd_monitor(purc_runloop_t runloop, uintptr_t handle)
{
    if (!runloop) {
        runloop = purc_runloop_get_current();
    }
    ((RunLoop*)runloop)->removeFdMonitor(handle);
}

void
pcintr_wakeup_target_with(pcintr_coroutine_t target, void *ctxt,
        void (*func)(void *ctxt))
{
    purc_runloop_t target_runloop;
    target_runloop = pcintr_co_get_runloop(target);
    PC_ASSERT(target_runloop);
    // FIXME: try catch ?
    ((RunLoop*)target_runloop)->dispatch([target, ctxt, func]() {
            PC_ASSERT(pcintr_get_heap());
            pcintr_set_current_co(target);
            func(ctxt);
            pcintr_set_current_co(nullptr);
        });
}

void
pcintr_post_msg_to_target(pcintr_coroutine_t target, void *ctxt,
        pcintr_msg_callback_f cb)
{
    pcintr_heap_t heap = pcintr_get_heap();
    pcintr_coroutine_t co = pcintr_get_coroutine();
    if (heap)
        PC_ASSERT(co);

    if (target == nullptr) {
        target = pcintr_get_coroutine();
        PC_ASSERT(target);
    }

    if (co == target) {
        pcintr_msg_t msg;
        msg = (pcintr_msg_t)calloc(1, sizeof(*msg));
        PC_ASSERT(msg);

        msg->ctxt        = ctxt;
        msg->on_msg      = cb;

        list_add_tail(&msg->node, &target->msgs);

        return;
    }

    pcintr_heap_t heap_target = target->owner;
    PC_ASSERT(heap_target == heap);

    purc_runloop_t target_runloop;
    target_runloop = pcintr_co_get_runloop(target);
    PC_ASSERT(target_runloop);

    // FIXME: try catch ?
    ((RunLoop*)target_runloop)->dispatch([target, ctxt, cb]() {
            PC_ASSERT(pcintr_get_heap());
            PC_ASSERT(pcintr_get_coroutine() == nullptr);

            pcintr_msg_t msg;
            msg = (pcintr_msg_t)calloc(1, sizeof(*msg));
            PC_ASSERT(msg);

            msg->ctxt        = ctxt;
            msg->on_msg      = cb;

            list_add_tail(&msg->node, &target->msgs);

            pcintr_set_current_co(target);
            pcintr_check_after_execution();
            pcintr_set_current_co(nullptr);
        });
}

void
pcintr_fire_event_to_target(pcintr_coroutine_t target,
        purc_atom_t msg_type,
        purc_variant_t msg_sub_type,
        purc_variant_t src,
        purc_variant_t payload)
{
    pcintr_heap_t heap = pcintr_get_heap();
    pcintr_coroutine_t co = pcintr_get_coroutine();
    if (heap)
        PC_ASSERT(co);

    if (target == nullptr) {
        target = pcintr_get_coroutine();
        PC_ASSERT(target);
    }

    pcintr_heap_t heap_target = target->owner;
    PC_ASSERT(heap_target == heap);

    PC_ASSERT(co != target);

    pcintr_event_t event;
    event = (pcintr_event_t)calloc(1, sizeof(*event));
    PC_ASSERT(event);

    event->msg_type             = msg_type;
    event->msg_sub_type         = purc_variant_ref(msg_sub_type);
    event->src                  = purc_variant_ref(src);
    event->payload              = purc_variant_ref(payload);

    purc_runloop_t target_runloop;
    target_runloop = pcintr_co_get_runloop(target);
    PC_ASSERT(target_runloop);

    // FIXME: try catch ?
    ((RunLoop*)target_runloop)->dispatch([target, event]() {
            pcintr_set_current_co(target);
            if (target->continuation) {
                pcintr_resume(target, event);
            }
            else {
                PC_ASSERT(0);
            }
            pcintr_check_after_execution();
            pcintr_set_current_co(nullptr);
        });
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

static void _runloop_init_main(void)
{
    BinarySemaphore semaphore;
    _main_thread = Thread::create(MAIN_RUNLOOP_THREAD_NAME, [&] {
            PC_ASSERT(RunLoop::isMainInitizlized() == false);
            RunLoop::initializeMain();
            RunLoop& runloop = RunLoop::main();
            PC_ASSERT(&runloop == &RunLoop::current());
            semaphore.signal();

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

            pcinst_current()->is_instmgr = 1;
            struct instmgr_info info = { 0, NULL };
            info.sa_insts = pcutils_sorted_array_create(SAFLAG_DEFAULT, 0,
                    my_sa_free, NULL);

            purc_runloop_func func = pcrun_instmgr_handle_message;
            runloop.setIdleCallback([func, &info]() {
                    func(&info);
                    });

            runloop.run();

            pcutils_sorted_array_destroy(info.sa_insts);

            size_t n = purc_inst_destroy_move_buffer();
            purc_log_debug("InstMgr is quiting, %u messages discarded\n",
                    (unsigned)n);

            purc_cleanup();
            });
    semaphore.wait();
}

static void _runloop_stop_main(void)
{
    if (_main_thread) {
        RunLoop& runloop = RunLoop::main();
        runloop.dispatch([&] {
                RunLoop::stopMain();
                });
        _main_thread->waitForCompletion();
    }
}

static RefPtr<Thread> _sync_thread;
static RunLoop *_sync_runloop = nullptr;

void
pcintr_synchronize(void *ctxt, void (*routine)(void *ctxt))
{
    PC_ASSERT(_sync_runloop);
    BinarySemaphore semaphore;
    _sync_runloop->dispatch([&] () {
        if (routine == NULL)
            return;

        routine(ctxt);
        semaphore.signal();
    });
    semaphore.wait();
}

static void _runloop_init_sync(void)
{
    BinarySemaphore semaphore;
    _sync_thread = Thread::create("_sync_runloop", [&] {
            RunLoop& runloop = RunLoop::current();
            _sync_runloop = &runloop;
            semaphore.signal();
            runloop.run();
            });

    semaphore.wait();
}

static void _runloop_stop_sync(void)
{
    if (_sync_runloop) {
        _sync_runloop->dispatch([&] {
                if (_sync_runloop) {
                    _sync_runloop->stop();
                }
                });
        _sync_thread->waitForCompletion();
        _sync_runloop = nullptr;
    }
}

void pcintr_add_heap(struct list_head *all_heaps)
{
    pcintr_heap_t heap = pcintr_get_heap();
    PC_ASSERT(heap);
    PC_ASSERT(heap->owning_heaps == nullptr);
    PC_ASSERT(_sync_runloop);
    BinarySemaphore sema;
    _sync_runloop->dispatch([heap, all_heaps, &sema]() {
            PC_ASSERT(heap->owning_heaps == nullptr);
            list_add_tail(&heap->sibling, all_heaps);
            heap->owning_heaps = all_heaps;
            sema.signal();
    });
    sema.wait();
}

void pcintr_remove_heap(struct list_head *all_heaps)
{
    pcintr_heap_t heap = pcintr_get_heap();
    PC_ASSERT(heap);
    PC_ASSERT(heap->owning_heaps == all_heaps);
    PC_ASSERT(_sync_runloop);
    BinarySemaphore sema;
    _sync_runloop->dispatch([heap, all_heaps, &sema]() {
            PC_ASSERT(heap->owning_heaps == all_heaps);
            list_del(&heap->sibling);
            heap->owning_heaps = nullptr;
            sema.signal();
    });
    sema.wait();
}

static int _init_once(void)
{
    _runloop_init_sync();
    atexit(_runloop_stop_sync);

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

