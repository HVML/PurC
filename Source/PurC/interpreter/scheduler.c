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

void
pcintr_check_after_execution_full(struct pcinst *inst, pcintr_coroutine_t co)
{
    pcintr_stack_t stack = &co->stack;
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    switch (co->state) {
        case CO_STATE_READY:
            break;
        case CO_STATE_RUNNING:
            co->state = CO_STATE_READY;
            break;
        case CO_STATE_STOPPED:
            PC_ASSERT(frame && frame->type == STACK_FRAME_TYPE_NORMAL);
            PC_ASSERT(inst->errcode == 0);
            PC_ASSERT(co->yielded_ctxt);
            PC_ASSERT(co->continuation);
            return;
        default:
            PC_ASSERT(0);
    }

    if (inst->errcode) {
        PC_ASSERT(stack->except == 0);
        pcintr_exception_copy(&stack->exception);
        stack->except = 1;
        pcinst_clear_error(inst);
        PC_ASSERT(inst->errcode == 0);
#ifndef NDEBUG                     /* { */
        pcintr_dump_stack(stack);
#endif                             /* } */
        PC_ASSERT(inst->errcode == 0);
    }

    if (frame) {
        if (co->execution_pending == 0) {
            co->execution_pending = 1;
            //pcintr_wakeup_target(co, run_ready_co);
        }
        return;
    }

    PC_ASSERT(co->yielded_ctxt == NULL);
    PC_ASSERT(co->continuation == NULL);

    /* send doc to rdr */
    if (stack->stage == STACK_STAGE_FIRST_ROUND &&
            !pcintr_rdr_page_control_load(stack))
    {
        PC_ASSERT(0); // TODO:
        // stack->exited = 1;
        return;
    }

    pcintr_dump_document(stack);
    stack->stage = STACK_STAGE_EVENT_LOOP;


    if (co->stack.except) {
        const char *error_except = NULL;
        purc_atom_t atom;
        atom = co->stack.exception.error_except;
        PC_ASSERT(atom);
        error_except = purc_atom_to_string(atom);

        PC_ASSERT(co->error_except == NULL);
        co->error_except = error_except;

        pcintr_dump_c_stack(co->stack.exception.bt);
        co->stack.except = 0;

        if (!co->stack.exited) {
            co->stack.exited = 1;
            pcintr_notify_to_stop(co);
        }
    }

    if (!list_empty(&co->msgs) && co->msg_pending == 0) {
        PC_ASSERT(co->state == CO_STATE_READY);
        struct pcintr_stack_frame *frame;
        frame = pcintr_stack_get_bottom_frame(stack);
        PC_ASSERT(frame == NULL);
        pcintr_msg_t msg;
        msg = list_first_entry(&co->msgs, struct pcintr_msg, node);
        list_del(&msg->node);
        co->msg_pending = 1;
        pcintr_wakeup_target_with(co, msg, pcintr_on_msg);
        return;
    }

    if (!list_empty(&co->children)) {
        return;
    }

    if (co->stack.exited) {
        pcintr_revoke_all_dynamic_observers(&co->stack);
        PC_ASSERT(list_empty(&co->stack.dynamic_observers));
        pcintr_revoke_all_native_observers(&co->stack);
        PC_ASSERT(list_empty(&co->stack.native_observers));
        pcintr_revoke_all_common_observers(&co->stack);
        PC_ASSERT(list_empty(&co->stack.common_observers));
    }

    bool still_observed = pcintr_co_is_observed(co);
    if (!still_observed) {
        if (!co->stack.exited) {
            co->stack.exited = 1;
            pcintr_notify_to_stop(co);
        }
    }

    if (!list_empty(&co->msgs) && co->msg_pending == 0) {
        PC_ASSERT(co->state == CO_STATE_READY);
        struct pcintr_stack_frame *frame;
        frame = pcintr_stack_get_bottom_frame(stack);
        PC_ASSERT(frame == NULL);
        pcintr_msg_t msg;
        msg = list_first_entry(&co->msgs, struct pcintr_msg, node);
        list_del(&msg->node);
        co->msg_pending = 1;
        pcintr_wakeup_target_with(co, msg, pcintr_on_msg);
        return;
    }

    if (still_observed) {
        return;
    }

    if (!co->stack.exited) {
        co->stack.exited = 1;
        pcintr_notify_to_stop(co);
    }

    if (!list_empty(&co->msgs)) {
        return;
    }

    if (co->msg_pending) {
        return;
    }

// #define PRINT_DEBUG
    if (co->stack.last_msg_sent == 0) {
        co->stack.last_msg_sent = 1;

#ifdef PRINT_DEBUG              /* { */
        PC_DEBUGX("last msg was sent");
#endif                          /* } */
        pcintr_wakeup_target_with(co, pcintr_last_msg(), pcintr_on_last_msg);
        return;
    }

    if (co->stack.last_msg_read == 0) {
        return;
    }


#ifdef PRINT_DEBUG              /* { */
    PC_DEBUGX("last msg was processed");
#endif                          /* } */

    if (co->curator) {
        if (co->error_except) {
            // TODO: which is error, which is except?
            // currently, we treat all as except
            pcintr_post_callstate_except_event(co, co->error_except);
        }
        else {
            PC_ASSERT(co->val_from_return_or_exit);
            pcintr_post_callstate_success_event(co, co->val_from_return_or_exit);
        }
    }

    PC_ASSERT(co);
    PC_ASSERT(co->state == CO_STATE_READY);
    pcintr_run_exiting_co(co);
}

static void
execute_one_step_for_ready_co(struct pcinst *inst, pcintr_coroutine_t co)
{
    UNUSED_PARAM(inst);
    UNUSED_PARAM(co);

    pcintr_set_current_co(co);

    co->state = CO_STATE_RUNNING;
    co->execution_pending = 0;
    pcintr_execute_one_step_for_ready_co(co);
    pcintr_check_after_execution_full(inst, co);

    pcintr_set_current_co(NULL);
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

        execute_one_step_for_ready_co(inst, co);

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
        goto out_sleep;
    }

    struct pcintr_heap *heap = inst->intr_heap;
    if (!heap) {
        goto out_sleep;
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

