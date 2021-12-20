/*
 * @file timer.cpp
 * @author XueShuming
 * @date 2021/12/20
 * @brief The C api for timer.
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

#include "private/timer.h"
#include "private/errors.h"
#include "private/interpreter.h"
#include "private/runloop.h"

#include <stdlib.h>
#include <string.h>


pcintr_timer_t
pcintr_timer_create(const char* id, void* ctxt, pcintr_timer_fire_func func);
{
    UNUSED_PARAM(id);
}

void
pcintr_timer_set_interval(pcintr_timer_t timer, uint32_t interval);
{
    UNUSED_PARAM(timer);
    UNUSED_PARAM(interval);
}

uint32_t
pcintr_timer_get_interval(pcintr_timer_t timer);
{
    UNUSED_PARAM(timer);
}

void
pcintr_timer_start(pcintr_timer_t timer);
{
    UNUSED_PARAM(timer);
}

void
pcintr_timer_start_oneshot(pcintr_timer_t timer)
{
    UNUSED_PARAM(timer);
}

void
pcintr_timer_stop(pcintr_timer_t timer)
{
    UNUSED_PARAM(timer);
}

void
pcintr_timer_destroy(pcintr_timer_t timer)
{
    UNUSED_PARAM(timer);
}
