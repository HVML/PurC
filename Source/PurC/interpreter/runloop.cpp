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

#include "private/runloop.h"
#include "private/errors.h"

#include <wtf/Threading.h>
#include <wtf/RunLoop.h>
#include <wtf/threads/BinarySemaphore.h>

#include <stdlib.h>
#include <string.h>


#define MAIN_RUNLOOP_THREAD_NAME    "__purc_main_runloop_thread"

void pcrunloop_init_main(void)
{
    if (pcrunloop_is_main_initialized()) {
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

void pcrunloop_stop_main(void)
{
    BinarySemaphore semaphore;
    if (pcrunloop_is_main_initialized()) {
        RunLoop& runloop = RunLoop::main();
        runloop.dispatch([&] {
            RunLoop::stopMain();
            semaphore.signal();
        });
    }
    semaphore.wait();
}

bool pcrunloop_is_main_initialized(void)
{
    return RunLoop::isMainInitizlized();
}

pcrunloop_t pcrunloop_get_main(void)
{
    return (pcrunloop_t)&RunLoop::main();
}

pcrunloop_t pcrunloop_get_current(void)
{
    return (pcrunloop_t)&RunLoop::current();
}

bool pcrunloop_is_on_main(void)
{
    return RunLoop::isMain();
}

void pcrunloop_run(void)
{
    RunLoop::run();
}

void pcrunloop_stop(pcrunloop_t runloop)
{
    if (runloop) {
        ((RunLoop*)runloop)->stop();
    }
}

void pcrunloop_wakeup(pcrunloop_t runloop)
{
    if (runloop) {
        ((RunLoop*)runloop)->wakeUp();
    }
}

void pcrunloop_dispatch(pcrunloop_t runloop, pcrunloop_func func, void* ctxt)
{
    if (runloop) {
        ((RunLoop*)runloop)->dispatch([func, ctxt]() {
            func(ctxt);
        });
    }
}

void pcrunloop_set_idle_func(pcrunloop_t runloop, pcrunloop_func func, void* ctxt)
{
    if (runloop) {
        ((RunLoop*)runloop)->setIdleCallback([func, ctxt]() {
            func(ctxt);
        });
    }
}
