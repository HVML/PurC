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

#include <wtf/Threading.h>
#include <wtf/RunLoop.h>
#include <wtf/threads/BinarySemaphore.h>


#include <stdlib.h>
#include <string.h>


#define MAIN_RUNLOOP_THREAD_NAME    "__purc_main_runloop_thread"

void purc_runloop_init_main(void)
{
    if (purc_runloop_is_main_initialized()) {
        return;
    }
    BinarySemaphore semaphore;
    Thread::create(MAIN_RUNLOOP_THREAD_NAME, [&] {
        RunLoop::initializeMain();
        RunLoop& runloop = RunLoop::main();
        semaphore.signal();
        runloop.run();
    })->detach();
    semaphore.wait();
}

void purc_runloop_stop_main(void)
{
    if (purc_runloop_is_main_initialized()) {
        BinarySemaphore semaphore;
        RunLoop& runloop = RunLoop::main();
        runloop.dispatch([&] {
            RunLoop::stopMain();
            semaphore.signal();
        });
        semaphore.wait();
    }
}

bool purc_runloop_is_main_initialized(void)
{
    return RunLoop::isMainInitizlized();
}

purc_runloop_t purc_runloop_get_current(void)
{
    return (purc_runloop_t)&RunLoop::current();
}

bool purc_runloop_is_on_main(void)
{
    return RunLoop::isMain();
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
            pcintr_heap_t heap = pcintr_get_heap();
            PC_ASSERT(heap->running_coroutine == NULL);
            heap->running_coroutine = target;
            func(ctxt);
            heap->running_coroutine = NULL;
        });
}

