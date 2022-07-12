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

#define SCHEDULE_SLEEP        10000           // usec
#define IDLE_EVENT_TIMEOUT      100             // ms

#define MSG_TYPE_IDLE           "idle"
#define BUILDIN_VAR_HVML        "HVML"

static inline
double current_time()
{
    struct timeval now;
    gettimeofday(&now, 0);
    return now.tv_sec * 1000 + now.tv_usec / 1000;
}

static void
broadcast_idle_event(struct pcinst *inst)
{
    struct pcintr_heap *heap = inst->intr_heap;
    struct rb_root *coroutines = &heap->coroutines;
    struct rb_node *p, *n;
    struct rb_node *first = pcutils_rbtree_first(coroutines);
    pcutils_rbtree_for_each_safe(first, p, n) {
        pcintr_coroutine_t co = container_of(p, struct pcintr_coroutine,
                node);
        pcintr_stack_t stack = &co->stack;
        if (stack->observe_idle) {
            purc_variant_t hvml = pcintr_get_coroutine_variable(stack->co,
                    BUILDIN_VAR_HVML);
            pcintr_coroutine_post_event(stack->co->cid,
                    PCRDR_MSG_EVENT_REDUCE_OPT_OVERLAY,
                    hvml, MSG_TYPE_IDLE, NULL, PURC_VARIANT_INVALID);
        }
    }
}

// execute one step for all ready coroutines of the inst
// return the number of ready coroutines
static size_t
execute_one_step(struct pcinst *inst)
{
    struct pcintr_heap *heap = inst->intr_heap;
    size_t nr_ready = 0;
    struct rb_root *coroutines = &heap->coroutines;
    struct rb_node *p, *n;
    struct rb_node *first = pcutils_rbtree_first(coroutines);
    pcutils_rbtree_for_each_safe(first, p, n) {
        pcintr_coroutine_t co = container_of(p, struct pcintr_coroutine,
                node);
        if (co->state != CO_STATE_READY) {
            continue;
        }
        // TODO:execute ont step

        // calc state
#if 0
        if (co->state == CO_STATE_READY) {
            nr_ready++;
        }
#endif
    }
    return nr_ready;
}

static size_t
dispatch_event(struct pcinst *inst, size_t *nr_stopped, size_t *nr_observing)
{
    UNUSED_PARAM(inst);
    UNUSED_PARAM(nr_stopped);
    UNUSED_PARAM(nr_observing);
    return 0;
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


    // 1. exec one step for all ready coroutines
    size_t nr_ready = execute_one_step(inst);

    // 2. dispatch event for observing / stopped coroutines
    size_t nr_stopped = 0;
    size_t nr_observing = 0;
    size_t nr_event = dispatch_event(inst, &nr_stopped, &nr_observing);

    // 3. its busy, goto next scheduler without sleep
    if (nr_ready || nr_event) {
        heap->timeout = current_time();
        goto out;
    }

    // 4. wating for something, sleep SCHEDULE_SLEEP before next scheduler
    if (nr_stopped) {
        heap->timeout = current_time();
        goto out_sleep;
    }

    // 5. broadcast idle event
    double now = current_time();
    if (now - IDLE_EVENT_TIMEOUT > heap->timeout) {
        broadcast_idle_event(inst);
        heap->timeout = now;
    }

out_sleep:
    pcutils_usleep(SCHEDULE_SLEEP);

out:
    return;
}

