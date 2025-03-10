/*
 * @file timer.h
 * @author XueShuming
 * @date 2021/12/27
 * @brief The api for timer.
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

#ifndef PURC_PRIVATE_TIMER_H
#define PURC_PRIVATE_TIMER_H

#include "config.h"

#include "purc.h"

#include "private/variant.h"
#include "private/map.h"
#include "purc-runloop.h"

typedef void* pcintr_timer_t;
typedef void (*pcintr_timer_fire_func)(pcintr_timer_t timer, const char* id,
        void *data);

PCA_EXTERN_C_BEGIN

pcintr_timer_t
pcintr_timer_create(purc_runloop_t runloop, const char* id,
        pcintr_timer_fire_func func, void *data) WTF_INTERNAL;

void
pcintr_timer_set_interval(pcintr_timer_t timer, uint32_t interval)
    WTF_INTERNAL;

uint32_t
pcintr_timer_get_interval(pcintr_timer_t timer) WTF_INTERNAL;

void
pcintr_timer_start(pcintr_timer_t timer) WTF_INTERNAL;

void
pcintr_timer_start_oneshot(pcintr_timer_t timer) WTF_INTERNAL;

void
pcintr_timer_stop(pcintr_timer_t timer) WTF_INTERNAL;

void
pcintr_timer_destroy(pcintr_timer_t timer) WTF_INTERNAL;

PCA_EXTERN_C_END

#endif /* not defined PURC_PRIVATE_TIMER_H */

