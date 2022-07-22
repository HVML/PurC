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

#define BUILTIN_VAR_CRTN        PURC_PREDEF_VARNAME_CRTN

#define YIELD_EVENT_HANDLER     "_yield_event_handler"

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
                    BUILTIN_VAR_CRTN);
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
        co->owner->cond_handler(PURC_COND_COR_OBSERVING, co, stack->doc);
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

    if (still_observed) {
        return;
    }

    if (!co->stack.exited) {
        co->stack.exited = 1;
        pcintr_notify_to_stop(co);
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
        purc_variant_t elementValue = purc_variant_make_native(co, NULL);
        pcintr_coroutine_post_event(co->cid,
                PCRDR_MSG_EVENT_REDUCE_OPT_KEEP,
                elementValue,           /* elementValue must set */
                MSG_TYPE_LAST_MSG, NULL,
                PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
        purc_variant_unref(elementValue);
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
            // XXX: curator may live in another thread!
            purc_variant_t payload = purc_variant_make_string(
                    co->error_except, false);
            purc_variant_t elementValue =  purc_variant_make_ulongint(co->cid);
            pcintr_coroutine_post_event(co->curator,
                    PCRDR_MSG_EVENT_REDUCE_OPT_KEEP,
                    elementValue,
                    MSG_TYPE_CALL_STATE, MSG_SUB_TYPE_EXCEPT,
                    payload, elementValue);
            purc_variant_unref(payload);
            purc_variant_unref(elementValue);
        }
        else {
            PC_ASSERT(co->val_from_return_or_exit);
            // XXX: curator may live in another thread!
            purc_variant_t elementValue =  purc_variant_make_ulongint(co->cid);
            pcintr_coroutine_post_event(co->curator, // target->cid,
                    PCRDR_MSG_EVENT_REDUCE_OPT_KEEP,
                    elementValue,
                    MSG_TYPE_CALL_STATE, MSG_SUB_TYPE_SUCCESS,
                    co->val_from_return_or_exit, elementValue);
            purc_variant_unref(elementValue);
        }
    }
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
// return whether busy
static bool
execute_one_step(struct pcinst *inst)
{
    struct pcintr_heap *heap = inst->intr_heap;
    struct rb_root *coroutines = &heap->coroutines;
    struct rb_node *p, *n;
    struct rb_node *first = pcutils_rbtree_first(coroutines);
    bool busy = false;
    pcutils_rbtree_for_each_safe(first, p, n) {
        pcintr_coroutine_t co = container_of(p, struct pcintr_coroutine,
                node);
        if (co->state != CO_STATE_READY) {
            continue;
        }

        execute_one_step_for_ready_co(inst, co);
        busy = true;
    }
    return busy;
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
        pcrdr_wait_and_dispatch_message(conn, 0);
        purc_clr_error();
    }
}

/* return whether busy */
bool
handle_coroutine_event(pcintr_coroutine_t co)
{
    bool busy = false;
    int handle_ret = PURC_ERROR_INCOMPLETED;
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(&co->stack);
    if (frame != NULL && co->state != CO_STATE_STOPPED) {
        goto out;
    }

    pcrdr_msg *msg = pcinst_msg_queue_get_msg(co->mq);
    bool remove_handler = false;
    bool performed = false;
    bool msg_observed = false;

    struct list_head *handlers = &co->event_handlers;
    struct list_head *p, *n;
    list_for_each_safe(p, n, handlers) {
        struct pcintr_event_handler *handler;
        handler = list_entry(p, struct pcintr_event_handler, ln);

        bool matched = false;
        bool observed = false;
        if (msg || handler->support_null_event) {
            matched = handler->is_match(handler, co, msg, &observed);
        }

        if (observed) {
            msg_observed = true;
        }

        // verify coroutine stage and state
        if ((co->stage & handler->cor_stage) == 0  ||
                (co->state & handler->cor_state) == 0 ||
                (!matched)) {
            continue;
        }

        handle_ret = handler->handle(handler, co, msg, &remove_handler,
                &performed);

        if (remove_handler) {
            pcintr_coroutine_remove_event_hander(handler);
        }

        if (handle_ret == PURC_ERROR_OK && msg) {
            pcrdr_release_message(msg);
            msg = NULL;
        }

        if (performed) {
            busy = true;
        }
    }

    if (!msg) {
        goto out;
    }

    if (co->sleep_handler) {
        struct pcintr_event_handler *handler = co->sleep_handler;
        bool observed = false;
        bool matched = handler->is_match(handler, co, msg, &observed);
        if (observed) {
            msg_observed = true;
        }
        if ((co->stage & handler->cor_stage) != 0  &&
                (co->state & handler->cor_state) != 0 &&
                (matched || msg_observed)) {

            bool remove_handler = false;
            int handle_ret = handler->handle(handler, co, msg, &remove_handler,
                    &performed);

            if (remove_handler) {
                pcintr_event_handler_destroy(handler);
                co->sleep_handler = NULL;
            }

            if (handle_ret == PURC_ERROR_OK) {
                pcrdr_release_message(msg);
                msg = NULL;
            }
        }
    }

    if (!msg) {
        goto out;
    }

    if (msg_observed) {
        pcinst_msg_queue_append(co->mq, msg);
    }
    else {
        pcrdr_release_message(msg);
    }

out:
    return busy;
}

static bool
dispatch_event(struct pcinst *inst)
{
    UNUSED_PARAM(inst);

    bool is_busy = false;
    check_and_dispatch_event_from_conn();

    bool co_is_busy = false;
    struct pcintr_heap *heap = inst->intr_heap;
    struct rb_root *coroutines = &heap->coroutines;
    struct rb_node *p, *n;
    struct rb_node *first = pcutils_rbtree_first(coroutines);
    pcutils_rbtree_for_each_safe(first, p, n) {
        pcintr_coroutine_t co;
        co = container_of(p, struct pcintr_coroutine, node);
        co_is_busy = handle_coroutine_event(co);

        if (co->stack.exited && co->stack.last_msg_read) {
            pcintr_run_exiting_co(co);
        }

        if (co_is_busy) {
            is_busy = true;
        }
    }

    return is_busy;
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


    // 1. exec one step for all ready coroutines and
    // return whether step is busy
    bool step_is_busy = execute_one_step(inst);

    // 2. dispatch event for observing / stopped coroutines
    bool event_is_busy = dispatch_event(inst);

    // 3. its busy, goto next scheduler without sleep
    if (step_is_busy || event_is_busy) {
        pcintr_update_timestamp(inst);
        goto out;
    }

    // 5. broadcast idle event
    double now = pcintr_get_current_time();
    if (now - IDLE_EVENT_TIMEOUT > heap->timestamp) {
        broadcast_idle_event(inst);
        pcintr_update_timestamp(inst);
    }

out_sleep:
    pcutils_usleep(SCHEDULE_SLEEP);

out:
    i++;
    return;
}

static bool
default_event_match(struct pcintr_event_handler *handler, pcintr_coroutine_t co,
        pcrdr_msg *msg, bool *observed)
{
    UNUSED_PARAM(handler);
    UNUSED_PARAM(co);
    UNUSED_PARAM(msg);
    *observed = false;
    return true;
}


struct pcintr_event_handler *
pcintr_event_handler_create(const char *name,
        int stage, int state, void *data, event_handle_fn fn,
        event_match_fn is_match_fn, bool support_null_event)
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
    handler->is_match = is_match_fn ? is_match_fn : default_event_match;
    handler->support_null_event = support_null_event;
out:
    return handler;
}

void
pcintr_event_handler_destroy(struct pcintr_event_handler *handler)
{
    if (handler->name) {
        free(handler->name);
    }
    free(handler);
}


struct pcintr_event_handler *
pcintr_coroutine_add_event_handler(pcintr_coroutine_t co,  const char *name,
        int stage, int state, void *data, event_handle_fn fn,
        event_match_fn is_match_fn, bool support_null_event)
{
    struct pcintr_event_handler *handler =
        pcintr_event_handler_create(name, stage, state,
                data, fn, is_match_fn, support_null_event);
    if (!handler) {
        goto out;
    }

    list_add_tail(&handler->ln, &co->event_handlers);
out:
    return handler;
}

int
pcintr_coroutine_remove_event_hander(struct pcintr_event_handler *handler)
{
    list_del(&handler->ln);
    pcintr_event_handler_destroy(handler);
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

bool is_yield_event_handler_match(struct pcintr_event_handler *handler,
        pcintr_coroutine_t co, pcrdr_msg *msg, bool *observed)
{
    UNUSED_PARAM(handler);
    bool match = false;
    if (co->wait_request_id) {
        match = purc_variant_is_equal_to(co->wait_request_id, msg->requestId);
        goto out;
    }

    if (purc_variant_is_equal_to(co->wait_element_value, msg->elementValue) &&
            purc_variant_is_equal_to(co->wait_event_name, msg->eventName)) {
        match =  true;
        goto out;
    }

out:
    *observed = match;
    return match;
}

static int
yield_event_handle(struct pcintr_event_handler *handler,
        pcintr_coroutine_t co, pcrdr_msg *msg, bool *remove_handler,
        bool *performed)
{
    UNUSED_PARAM(handler);
    UNUSED_PARAM(msg);

    *remove_handler = true;
    *performed = true;

    pcintr_set_current_co(co);
    pcintr_resume(co, msg);
    pcintr_set_current_co(NULL);

    return PURC_ERROR_OK;
}

void pcintr_yield(void *ctxt, void (*continuation)(void *ctxt, pcrdr_msg *msg),
        purc_variant_t request_id, purc_variant_t element_value,
        purc_variant_t event_name, bool custom_event_handler)
{
    UNUSED_PARAM(request_id);
    UNUSED_PARAM(event_name);
    PC_ASSERT(ctxt);
    PC_ASSERT(continuation);
    pcintr_coroutine_t co = pcintr_get_coroutine();
    PC_ASSERT(co);
    PC_ASSERT(co->state == CO_STATE_RUNNING);
    PC_ASSERT(co->yielded_ctxt == NULL);
    PC_ASSERT(co->continuation == NULL);
    pcintr_stack_t stack = &co->stack;
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame);

    pcintr_coroutine_set_state(co, CO_STATE_STOPPED);
    co->yielded_ctxt = ctxt;
    co->continuation = continuation;
    if (request_id) {
        co->wait_request_id = request_id;
        purc_variant_ref(co->wait_request_id);
    }

    if (element_value) {
        co->wait_element_value = element_value;
        purc_variant_ref(co->wait_element_value);
    }

    if (event_name) {
        co->wait_event_name = event_name;
        purc_variant_ref(co->wait_event_name);
    }

    if (!custom_event_handler) {
        pcintr_coroutine_add_event_handler(
                co,  YIELD_EVENT_HANDLER,
                CO_STAGE_FIRST_RUN | CO_STAGE_OBSERVING, CO_STATE_STOPPED,
                ctxt, yield_event_handle, is_yield_event_handler_match, false);
    }
}

void pcintr_resume(pcintr_coroutine_t co, pcrdr_msg *msg)
{
    PC_ASSERT(co);
    PC_ASSERT(co->state == CO_STATE_STOPPED);
    PC_ASSERT(co->yielded_ctxt);
    PC_ASSERT(co->continuation);
    pcintr_stack_t stack = &co->stack;
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame);

    void *ctxt = co->yielded_ctxt;
    void (*continuation)(void *ctxt, pcrdr_msg *msg) = co->continuation;

    pcintr_coroutine_set_state(co, CO_STATE_RUNNING);
    co->yielded_ctxt = NULL;
    co->continuation = NULL;
    if (co->wait_request_id) {
        purc_variant_unref(co->wait_request_id);
        co->wait_request_id = NULL;
    }

    if (co->wait_element_value) {
        purc_variant_unref(co->wait_element_value);
        co->wait_element_value = NULL;
    }

    if (co->wait_event_name) {
        purc_variant_unref(co->wait_event_name);
        co->wait_event_name = NULL;
    }

    continuation(ctxt, msg);
    pcintr_check_after_execution_full(pcinst_current(), co);
}

