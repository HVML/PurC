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
#include "private/msg-queue.h"

#include <stdlib.h>
#include <string.h>

#include <sys/time.h>

#define SCHEDULE_SLEEP          10000           // usec
#define IDLE_EVENT_TIMEOUT      100             // ms

#define BUILDIN_VAR_HVML        "HVML"

#define MSG_TYPE_IDLE           "idle"
#define MSG_TYPE_CALL_STATE     "callState"

#define MSG_SUB_TYPE_SUCCESS    "success"
#define MSG_SUB_TYPE_EXCEPT     "except"


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
                    hvml, MSG_TYPE_IDLE, NULL, PURC_VARIANT_INVALID,
                    PURC_VARIANT_INVALID);
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
            break;
        case CO_STATE_STOPPED:
            PC_ASSERT(frame && frame->type == STACK_FRAME_TYPE_NORMAL);
            PC_ASSERT(inst->errcode == 0);
            PC_ASSERT(co->yielded_ctxt);
            PC_ASSERT(co->continuation);
            return;
        default:
            break;
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
        pcintr_coroutine_set_state(co, CO_STATE_READY);
        if (co->execution_pending == 0) {
            co->execution_pending = 1;
            //pcintr_wakeup_target(co, run_ready_co);
        }
        return;
    }

    PC_ASSERT(co->yielded_ctxt == NULL);
    PC_ASSERT(co->continuation == NULL);

    /* send doc to rdr */
    if (stack->co->stage == CO_STAGE_FIRST_RUN &&
            !pcintr_rdr_page_control_load(stack))
    {
        PC_ASSERT(0); // TODO:
        // stack->exited = 1;
        return;
    }

    // VW: pcintr_dump_document(stack);
    //pcintr_dump_document(stack);
    if (co->owner->cond_handler) {
        co->owner->cond_handler(PURC_COND_COR_AFTER_FIRSTRUN, co, stack->doc);
    }
    stack->co->stage = CO_STAGE_OBSERVING;
    pcintr_coroutine_set_state(co, CO_STATE_OBSERVING);


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
        //PC_ASSERT(co->state == CO_STATE_READY);
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
        pcintr_coroutine_set_state(co, CO_STATE_EXITED);
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
            pcintr_coroutine_t target = pcintr_coroutine_get_by_id(co->curator);
            purc_variant_t payload = purc_variant_make_string(
                    co->error_except, false);
            pcintr_coroutine_post_event(target->cid,
                    PCRDR_MSG_EVENT_REDUCE_OPT_KEEP,
                    target->wait_request_id,
                    MSG_TYPE_CALL_STATE, MSG_SUB_TYPE_EXCEPT,
                    payload, target->wait_request_id);
            purc_variant_unref(payload);
        }
        else {
            PC_ASSERT(co->val_from_return_or_exit);
            pcintr_coroutine_t target = pcintr_coroutine_get_by_id(co->curator);
            pcintr_coroutine_post_event(target->cid,
                    PCRDR_MSG_EVENT_REDUCE_OPT_KEEP,
                    target->wait_request_id,
                    MSG_TYPE_CALL_STATE, MSG_SUB_TYPE_SUCCESS,
                    co->val_from_return_or_exit, target->wait_request_id);
        }
    }

    PC_ASSERT(co);
    pcintr_run_exiting_co(co);
}

static void
execute_one_step_for_ready_co(struct pcinst *inst, pcintr_coroutine_t co)
{
    UNUSED_PARAM(inst);
    UNUSED_PARAM(co);

    pcintr_set_current_co(co);

    pcintr_coroutine_set_state(co, CO_STATE_RUNNING);
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

        if (co->state == CO_STATE_READY) {
            nr_ready++;
        }
    }
    return nr_ready;
}


void
check_and_dispatch_event_from_conn()
{
    struct pcrdr_conn *conn =  purc_get_conn_to_renderer();

    if (conn) {
        pcrdr_event_handler handle = pcrdr_conn_get_event_handler(conn);
        if (!handle) {
            pcrdr_conn_set_event_handler(conn, pcintr_conn_event_handler);
        }
        pcrdr_wait_and_dispatch_message(conn, 1);
        purc_clr_error();
    }
}

size_t
handle_coroutine_event(pcintr_coroutine_t co)
{
    int handle_ret = PURC_ERROR_INCOMPLETED;
    pcrdr_msg *msg = pcinst_msg_queue_get_msg(co->mq);
    bool remove_handler = false;

    struct list_head *handlers = &co->event_handlers;
    struct list_head *p, *n;
    list_for_each_safe(p, n, handlers) {
        struct pcintr_event_handler *handler;
        handler = list_entry(p, struct pcintr_event_handler, ln);

        // verify coroutine stage and state
        if ((co->stage & handler->cor_stage) == 0  ||
                (co->state & handler->cor_state) == 0) {
            continue;
        }

        if ((msg == NULL) && !handler->support_null_event) {
            continue;
        }

        handle_ret = handler->handle(handler, co, msg, &remove_handler);

        if (remove_handler) {
            pcintr_coroutine_remove_event_hander(handler);
        }

        if (handle_ret == PURC_ERROR_OK && msg) {
            pcrdr_release_message(msg);
            msg = NULL;
        }
    }

    if (msg) {
        pcinst_msg_queue_append(co->mq, msg);
    }
    return pcinst_msg_queue_count(co->mq);
}


static size_t
dispatch_event(struct pcinst *inst, size_t *nr_stopped, size_t *nr_observing)
{
    UNUSED_PARAM(inst);
    UNUSED_PARAM(nr_stopped);
    UNUSED_PARAM(nr_observing);

    size_t nr_stop = 0;
    size_t nr_observe = 0;
    size_t nr_event = 0;

    check_and_dispatch_event_from_conn();

    struct pcintr_heap *heap = inst->intr_heap;
    struct rb_root *coroutines = &heap->coroutines;
    struct rb_node *p, *n;
    struct rb_node *first = pcutils_rbtree_first(coroutines);
    pcutils_rbtree_for_each_safe(first, p, n) {
        pcintr_coroutine_t co;
        co = container_of(p, struct pcintr_coroutine, node);
        nr_event += handle_coroutine_event(co);

        if (co->state == CO_STATE_STOPPED) {
            nr_stop++;
        }
        if (co->stage == CO_STAGE_OBSERVING) {
            nr_observe++;
        }
    }

    *nr_stopped = nr_stop;
    *nr_observing = nr_observe;
    return nr_event;
}

void
pcintr_schedule(void *ctxt)
{
    static int i = 0;
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
    i++;
    return;
}

struct pcintr_event_handler *
pcintr_coroutine_add_event_handler(pcintr_coroutine_t co,  const char *name,
        int stage, int state, void *data, event_handle_fn fn,
        bool support_null_event)
{
    struct pcintr_event_handler *handler =
        (struct pcintr_event_handler *)calloc(1, sizeof(*handler));
    if (!handler) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }

    handler->name = name ? strdup(name) : NULL;
    handler->cor_stage = stage;
    handler->cor_state = state;
    handler->data = data;
    handler->handle = fn;
    handler->support_null_event = support_null_event;

    list_add_tail(&handler->ln, &co->event_handlers);
out:
    return handler;
}

int
pcintr_coroutine_remove_event_hander(struct pcintr_event_handler *handler)
{
    list_del(&handler->ln);
    if (handler->name) {
        free(handler->name);
    }
    free(handler);
    return 0;
}

int
pcintr_coroutine_clear_event_handlers(pcintr_coroutine_t co)
{
    struct list_head *handlers = &co->event_handlers;
    struct list_head *p, *n;
    list_for_each_safe(p, n, handlers) {
        struct pcintr_event_handler *handler;
        handler = list_entry(p, struct pcintr_event_handler, ln);
        pcintr_coroutine_remove_event_hander(handler);
    }
    return 0;
}

