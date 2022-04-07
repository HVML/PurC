/*
 * @file runloop.cpp
 * @author XueShuming
 * @date 2021/12/14
 * @brief The C api for RunLoop.
 *
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
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

static purc_runloop_io_state
to_runloop_io_state(GIOCondition condition)
{
    switch (condition) {
    case G_IO_IN:
        return PCRUNLOOP_IO_IN;
    case G_IO_OUT:
        return PCRUNLOOP_IO_OUT;
    case G_IO_PRI:
        return PCRUNLOOP_IO_PRI;
    case G_IO_ERR:
        return PCRUNLOOP_IO_ERR;
    case G_IO_HUP:
        return PCRUNLOOP_IO_HUP;
    case G_IO_NVAL:
        return PCRUNLOOP_IO_NVAL;
    }
}

static GIOCondition
to_gio_condition(purc_runloop_io_state state)
{
    switch (state) {
    case PCRUNLOOP_IO_IN:
        return G_IO_IN;
    case PCRUNLOOP_IO_OUT:
        return G_IO_OUT;
    case PCRUNLOOP_IO_PRI:
        return G_IO_PRI;
    case PCRUNLOOP_IO_ERR:
        return G_IO_ERR;
    case PCRUNLOOP_IO_HUP:
        return G_IO_HUP;
    case PCRUNLOOP_IO_NVAL:
        return G_IO_NVAL;
    }
}


uintptr_t purc_runloop_add_fd_monitor(purc_runloop_t runloop, int fd,
        purc_runloop_io_state state, purc_runloop_io_callback callback,
        void *ctxt)
{
    if (!runloop) {
        runloop = purc_runloop_get_current();
    }
    return ((RunLoop*)runloop)->addFdMonitor(fd, to_gio_condition(state),
            [callback, ctxt] (gint fd, GIOCondition condition) -> gboolean {
            return callback(fd, to_runloop_io_state(condition), ctxt);
        });
}

void purc_runloop_remove_fd_monitor(purc_runloop_t runloop, uintptr_t handle)
{
    if (!runloop) {
        runloop = purc_runloop_get_current();
    }
    ((RunLoop*)runloop)->removeFdMonitor(handle);
}

