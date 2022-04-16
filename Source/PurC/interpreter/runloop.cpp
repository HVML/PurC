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


uintptr_t purc_runloop_add_fd_monitor(purc_runloop_t runloop, int fd,
        purc_runloop_io_event event, purc_runloop_io_callback callback,
        void *ctxt)
{
    if (!runloop) {
        runloop = purc_runloop_get_current();
    }

    void *stack = pcintr_get_stack();
    return ((RunLoop*)runloop)->addFdMonitor(fd, to_gio_condition(event),
            [callback, ctxt, stack] (gint fd, GIOCondition condition) -> gboolean {
            return callback(fd, to_runloop_io_event(condition), ctxt, stack);
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

