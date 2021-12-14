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

#include <wtf/RunLoop.h>

#include <stdlib.h>
#include <string.h>

void pcrunloop_init_main(void)
{
    RunLoop::initializeMain();
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

void pcrunloop_warkup(pcrunloop_t runloop)
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

