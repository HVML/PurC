/*
 * @file scheduler.c
 * @author XueShuming
 * @date 2022/07/11
 * @brief The scheduler for coroutine.
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

#include "internal.h"

#include "private/var-mgr.h"
#include "private/errors.h"
#include "private/instance.h"
#include "private/utils.h"
#include "private/variant.h"
#include "private/ports.h"

#include <stdlib.h>
#include <string.h>

#include <sys/time.h>

#define SCHEDULE_TIMEOUT        10000           // usec
#define IDLE_EVENT_TIMEOUT      100             // ms

static inline
double current_time()
{
    struct timeval now;
    gettimeofday(&now, 0);
    return now.tv_sec * 1000 + now.tv_usec / 1000;
}

void
pcintr_schedule(void *ctxt)
{
    struct pcinst *inst = (struct pcinst *)ctxt;
    if (!inst) {
        return;
    }
    struct pcintr_heap *heap = inst->intr_heap;
    if (!heap) {
        return;
    }


    // check if all coroutine STACK_STAGE_EVENT_LOOP
    struct rb_root *coroutines = &heap->coroutines;
    struct rb_node *p, *n;
    struct rb_node *first = pcutils_rbtree_first(coroutines);
    pcutils_rbtree_for_each_safe(first, p, n) {
        pcintr_coroutine_t co = container_of(p, struct pcintr_coroutine,
                node);
        pcintr_stack_t stack = &co->stack;
        if (stack->stage != STACK_STAGE_EVENT_LOOP) {
            goto out;
        }
    }

    double now = current_time();
    // first update timeout
    if (heap->timeout == 0) {
        heap->timeout = now;
    }

    if (now - IDLE_EVENT_TIMEOUT > heap->timeout) {
        // TODO: broadcast idle
        heap->timeout =now;
    }

out:
    pcutils_usleep(SCHEDULE_TIMEOUT);
}

