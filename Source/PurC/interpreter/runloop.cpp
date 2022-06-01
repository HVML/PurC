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

struct fd_monitor_data {
    pcintr_coroutine_t                  co;
    int                                 fd;
    purc_runloop_io_event               io_event;
    purc_runloop_io_callback            callback;
    void                               *ctxt;
};

static void on_io_event(void *ud)
{
    struct fd_monitor_data *data;
    data = (struct fd_monitor_data*)ud;
    data->callback(data->fd, data->io_event, data->ctxt);
    free(data);
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
            PC_ASSERT(pcintr_get_runloop()==NULL);
            purc_runloop_io_event io_event;
            io_event = to_runloop_io_event(condition);
            runLoop->dispatch([fd, io_event, ctxt, co, callback] {
                    PC_ASSERT(pcintr_get_heap());
                    PC_ASSERT(co);
                    PC_ASSERT(co->state == CO_STATE_READY);

                    struct fd_monitor_data *data;
                    data = (struct fd_monitor_data*)calloc(1, sizeof(*data));
                    PC_ASSERT(data);
                    data->co = co;
                    data->fd = fd;
                    data->callback = callback;
                    data->ctxt = ctxt;

                    pcintr_set_current_co(co);
                    co->state = CO_STATE_RUN;
                    pcintr_post_msg(data, on_io_event);
                    pcintr_check_after_execution();
                    pcintr_set_current_co(NULL);
            });
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

int purc_runloop_dispatch_message(purc_runloop_t runloop, purc_variant_t source,
        purc_variant_t type, purc_variant_t sub_type, purc_variant_t extra,
        void *stack)
{
    if (!runloop) {
        runloop = purc_runloop_get_current();
    }
    if (!stack) {
        stack = pcintr_get_stack();
        return -1;
    }

    if (stack) {
        return pcintr_dispatch_message_ex((struct pcintr_stack *)stack, source,
                type, sub_type, extra);
    }
    return -1;
}

static void apply_none(void *ctxt)
{
    co_routine_f routine = (co_routine_f)ctxt;
    routine();
}

void
pcintr_wakeup_target(pcintr_coroutine_t target, co_routine_f routine)
{
    pcintr_wakeup_target_with(target, (void*)routine, apply_none);
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
            pcintr_set_current_co(NULL);
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

    if (target == NULL) {
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
            PC_ASSERT(pcintr_get_coroutine() == NULL);

            pcintr_msg_t msg;
            msg = (pcintr_msg_t)calloc(1, sizeof(*msg));
            PC_ASSERT(msg);

            msg->ctxt        = ctxt;
            msg->on_msg      = cb;

            list_add_tail(&msg->node, &target->msgs);

            pcintr_set_current_co(target);
            pcintr_check_after_execution();
            pcintr_set_current_co(NULL);
        });
}

static void on_event(void *ud)
{
    pcintr_event_t event;
    event = (pcintr_event_t)ud;
    PC_ASSERT(event);

    pcintr_on_event(event);
}

void
pcintr_fire_event_to_target(pcintr_coroutine_t target,
        purc_atom_t msg_type,
        purc_variant_t msg_sub_type,
        purc_variant_t src,
        purc_variant_t payload)
{
    pcintr_event_t event;
    event = (pcintr_event_t)calloc(1, sizeof(*event));
    PC_ASSERT(event);

    pcintr_heap_t heap = pcintr_get_heap();
    pcintr_coroutine_t co = pcintr_get_coroutine();
    if (heap)
        PC_ASSERT(co);

    if (target == NULL) {
        target = pcintr_get_coroutine();
        PC_ASSERT(target);
    }

    pcintr_heap_t heap_target = target->owner;
    PC_ASSERT(heap_target == heap);

    event->msg_type             = msg_type;
    event->msg_sub_type         = purc_variant_ref(msg_sub_type);
    event->src                  = purc_variant_ref(src);
    event->payload              = purc_variant_ref(payload);

    pcintr_msg_t msg;
    msg = (pcintr_msg_t)calloc(1, sizeof(*msg));
    PC_ASSERT(msg);

    msg->ctxt        = event;
    msg->on_msg      = on_event;

    if (co == target) {
        list_add_tail(&msg->node, &target->msgs);
        return;
    }

    purc_runloop_t target_runloop;
    target_runloop = pcintr_co_get_runloop(target);
    PC_ASSERT(target_runloop);

    // FIXME: try catch ?
    ((RunLoop*)target_runloop)->dispatch([target, msg]() {
            PC_ASSERT(pcintr_get_heap());
            PC_ASSERT(pcintr_get_coroutine() == NULL);

            list_add_tail(&msg->node, &target->msgs);

            pcintr_set_current_co(target);
            pcintr_check_after_execution();
            pcintr_set_current_co(NULL);
        });
}


#define MAIN_RUNLOOP_THREAD_NAME    "__purc_main_runloop_thread"

static RefPtr<Thread> _main_thread;
static pthread_once_t _once_control = PTHREAD_ONCE_INIT;

static void _runloop_init_main(void)
{
    BinarySemaphore semaphore;
    _main_thread = Thread::create(MAIN_RUNLOOP_THREAD_NAME, [&] {
            PC_ASSERT(RunLoop::isMainInitizlized() == false);
            RunLoop::initializeMain();
            RunLoop& runloop = RunLoop::main();
            PC_ASSERT(&runloop == &RunLoop::current());
            semaphore.signal();
            runloop.run();
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

static int _init_once(void)
{
    atexit(_runloop_stop_main);
    return 0;
}

static int _init_instance(struct pcinst* curr_inst,
        const purc_instance_extra_info* extra_info)
{
    UNUSED_PARAM(curr_inst);
    UNUSED_PARAM(extra_info);

    int r;
    r = pthread_once(&_once_control, _runloop_init_main);
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

