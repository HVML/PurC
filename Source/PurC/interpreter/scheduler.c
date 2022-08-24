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
                    hvml, MSG_TYPE_IDLE, NULL,
                    PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
        }
    }
}

static void
handle_rdr_conn_lost(struct pcinst *inst)
{
    struct pcintr_heap *heap = inst->intr_heap;
    struct rb_root *coroutines = &heap->coroutines;
    struct rb_node *p, *n;
    struct rb_node *first = pcutils_rbtree_first(coroutines);
    pcutils_rbtree_for_each_safe(first, p, n) {
        pcintr_coroutine_t co = container_of(p, struct pcintr_coroutine,
                node);
        pcintr_stack_t stack = &co->stack;
        purc_variant_t hvml = pcintr_get_coroutine_variable(stack->co,
                BUILTIN_VAR_CRTN);

        stack->co->target_workspace_handle = 0;
        stack->co->target_page_handle = 0;
        stack->co->target_dom_handle = 0;

        // broadcast rdrState:connLost;
        pcintr_coroutine_post_event(stack->co->cid,
                PCRDR_MSG_EVENT_REDUCE_OPT_OVERLAY,
                hvml, MSG_TYPE_RDR_STATE, MSG_SUB_TYPE_CONN_LOST,
                PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    }

    // FIXME:
    // pcrdr_disconnect(inst->conn_to_rdr);
    pcrdr_free_connection(inst->conn_to_rdr);
    inst->conn_to_rdr = NULL;
}


void
pcintr_check_after_execution_full(struct pcinst *inst, pcintr_coroutine_t co)
{
    bool one_run = false;
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
            return;
        default:
            break;
    }

    if (inst->errcode) {
        //PC_ASSERT(stack->except == 0);
        pcintr_exception_copy(&stack->exception);
        stack->except = 1;
        pcinst_clear_error(inst);
        PC_ASSERT(inst->errcode == 0);
#ifndef NDEBUG                     /* { */
        pcintr_dump_stack(stack);
#endif                             /* } */
        if (co->owner->cond_handler) {
            struct purc_cor_term_info term_info;
            term_info.doc = stack->doc;
            term_info.except = purc_variant_make_string(
                    purc_atom_to_string(stack->exception.error_except), false);
            co->owner->cond_handler(PURC_COND_COR_TERMINATED, co, &term_info);
            purc_variant_unref(term_info.except);
        }
        PC_ASSERT(inst->errcode == 0);
    }

    if (frame) {
        if (frame->next_step != NEXT_STEP_ON_POPPING) {
            pcintr_coroutine_set_state(co, CO_STATE_READY);
            return;
        }

        pcvdom_element_t elem = frame->pos;
        enum pchvml_tag_id tag_id = elem->tag_id;
        if (tag_id != PCHVML_TAG_HVML) {
            pcintr_coroutine_set_state(co, CO_STATE_READY);
            return;
        }
        // CO_STAGE_FIRST_RUN or
        // observing finished (only HVML tag in stack)
        one_run = true;
    }

    /* send doc to rdr */
    if (stack->co->stage == CO_STAGE_FIRST_RUN &&
            !pcintr_rdr_page_control_load(stack))
    {
        PC_ASSERT(0); // TODO:
        // stack->exited = 1;
        return;
    }


    if (one_run) {
        // repeat this call when an observing finished.
        if (co->owner->cond_handler) {
            struct purc_cor_run_info run_info = { 0, };
            run_info.run_idx = co->run_idx;
            run_info.doc = stack->doc;
            run_info.result = pcintr_coroutine_get_result(co);
            co->owner->cond_handler(PURC_COND_COR_ONE_RUN, co, &run_info);
        }
        co->run_idx++;
    }

    if (stack->co->stage != CO_STAGE_OBSERVING) {
        stack->co->stage = CO_STAGE_OBSERVING;
        // POST corState:observing
        if (co->curator) {
            purc_variant_t request_id =  purc_variant_make_ulongint(co->cid);
            pcintr_coroutine_post_event(co->curator, // target->cid,
                    PCRDR_MSG_EVENT_REDUCE_OPT_KEEP,
                    request_id,
                    MSG_TYPE_CORSTATE, MSG_SUB_TYPE_OBSERVING,
                    PURC_VARIANT_INVALID, request_id);
            purc_variant_unref(request_id);
        }
    }
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
        pcintr_revoke_all_hvml_observers(&co->stack);
        PC_ASSERT(list_empty(&co->stack.hvml_observers));

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

 //#define PRINT_DEBUG
    if (co->stack.last_msg_sent == 0) {
        co->stack.last_msg_sent = 1;

#ifdef PRINT_DEBUG              /* { */
        PC_DEBUGX("last msg was sent");
#endif                          /* } */
        pcintr_coroutine_post_event(co->cid,
                PCRDR_MSG_EVENT_REDUCE_OPT_KEEP,
                PURC_VARIANT_INVALID,
                MSG_TYPE_LAST_MSG, NULL,
                PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
        return;
    }

    if (co->stack.last_msg_read == 0) {
        return;
    }


#ifdef PRINT_DEBUG              /* { */
    PC_DEBUGX("last msg was processed");
#endif                          /* } */

    if (co->curator) {
        purc_variant_t request_id =  purc_variant_make_ulongint(co->cid);
        purc_variant_t result = pcintr_coroutine_get_result(co);
        if (co->error_except) {
            // TODO: which is error, which is except?
            // currently, we treat all as except
            // XXX: curator may live in another thread!
            purc_variant_t payload = purc_variant_make_string(
                    co->error_except, false);
            pcintr_coroutine_post_event(co->curator,
                    PCRDR_MSG_EVENT_REDUCE_OPT_KEEP,
                    request_id,
                    MSG_TYPE_CALL_STATE, MSG_SUB_TYPE_EXCEPT,
                    payload, request_id);
            purc_variant_unref(payload);
        }
        else {
            // XXX: curator may live in another thread!
            pcintr_coroutine_post_event(co->curator, // target->cid,
                    PCRDR_MSG_EVENT_REDUCE_OPT_KEEP,
                    request_id,
                    MSG_TYPE_CALL_STATE, MSG_SUB_TYPE_SUCCESS,
                    result, request_id);
        }
        pcintr_coroutine_post_event(co->curator, // target->cid,
                PCRDR_MSG_EVENT_REDUCE_OPT_KEEP,
                request_id,
                MSG_TYPE_CORSTATE, MSG_SUB_TYPE_EXITED,
                result, request_id);
        purc_variant_unref(request_id);
    }
}

static void
execute_one_step_for_ready_co(struct pcinst *inst, pcintr_coroutine_t co)
{
    UNUSED_PARAM(inst);
    UNUSED_PARAM(co);

    pcintr_set_current_co(co);

    pcintr_coroutine_set_state(co, CO_STATE_RUNNING);
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
check_and_dispatch_event_from_conn(struct pcinst *inst)
{
    struct pcrdr_conn *conn =  purc_get_conn_to_renderer();

    if (conn) {
        pcrdr_event_handler handle = pcrdr_conn_get_event_handler(conn);
        if (!handle) {
            pcrdr_conn_set_event_handler(conn, pcintr_conn_event_handler);
        }

        int last_err = purc_get_last_error();
        purc_clr_error();

        pcrdr_wait_and_dispatch_message(conn, 0);

        int err = purc_get_last_error();
        if (err == PCRDR_ERROR_IO || err == PCRDR_ERROR_PEER_CLOSED) {
            handle_rdr_conn_lost(inst);
        }
        purc_set_error(last_err);
    }
}

static int
handle_event_by_observer_list(purc_coroutine_t co, struct list_head *list,
        pcrdr_msg *msg, purc_atom_t event_type,
        const char *event_sub_type, bool *event_observed, bool *busy)
{
    int ret = PURC_ERROR_INCOMPLETED;
    purc_variant_t observed = msg->elementValue;
    struct pcintr_observer *observer, *next;
    list_for_each_entry_safe(observer, next, list, node) {
        bool match = observer->is_match(observer, msg, observed, event_type,
                event_sub_type);
        if ((co->stage & observer->cor_stage) &&
                (co->state & observer->cor_state) && match) {
            ret = observer->handle(co, observer, msg, event_type,
                    event_sub_type, observer->handle_data);
            if (observer->auto_remove) {
                pcintr_revoke_observer(observer);
            }
            *busy = true;
        }
        if (match) {
            *event_observed = true;
        }
    }
    return ret;
}

bool
handle_coroutine_event(pcintr_coroutine_t co)
{
    int handle_ret = PURC_ERROR_INCOMPLETED;
    bool busy = false;
    bool msg_observed = false;
    char *type = NULL;
    purc_atom_t event_type = 0;
    const char *event_sub_type = NULL;

    if (co->state == CO_STATE_READY || co->state == CO_STATE_RUNNING) {
        goto out;
    }

    pcrdr_msg *msg = pcinst_msg_queue_get_msg(co->mq);

    if (msg && msg->eventName) {
        const char *event = purc_variant_get_string_const(msg->eventName);
        const char *separator = strchr(event, MSG_EVENT_SEPARATOR);
        if (separator) {
            event_sub_type = separator + 1;
        }

        size_t nr_type = separator - event;
        if (nr_type) {
            type = strndup(event, separator - event);
            if (!type) {
                purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
                goto out;
            }

            event_type = purc_atom_try_string_ex(ATOM_BUCKET_MSG, type);
            if (!event_type) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                PC_WARN("unknown event '%s'\n", event);
                goto out;
            }
        }
    }

    // observer
    if (msg) {
        handle_ret = handle_event_by_observer_list(co,
                &co->stack.intr_observers, msg, event_type, event_sub_type,
                &msg_observed, &busy);

        if (handle_ret == PURC_ERROR_OK) {
            pcrdr_release_message(msg);
            msg = NULL;
        }
        else {
            handle_ret = handle_event_by_observer_list(co,
                    &co->stack.hvml_observers, msg, event_type, event_sub_type,
                    &msg_observed, &busy);

            if (handle_ret == PURC_ERROR_OK) {
                pcrdr_release_message(msg);
                msg = NULL;
            }
        }
    }

    if (!list_empty(&co->tasks)) {
        struct pcintr_observer_task *task =
            list_first_entry(&co->tasks, struct pcintr_observer_task, ln);
        if (task && (co->stage & task->cor_stage) != 0  &&
                (co->state & task->cor_state) != 0) {
            list_del(&task->ln);
            pcintr_handle_task(task);
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
    if (type) {
        free(type);
    }
    return busy;
}

static bool
dispatch_event(struct pcinst *inst)
{
    UNUSED_PARAM(inst);

    bool is_busy = false;
    check_and_dispatch_event_from_conn(inst);

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

int pcintr_yield(
        int                       cor_stage,
        int                       cor_state,
        purc_variant_t            observed,
        const char               *event_type,
        const char               *event_sub_type,
        observer_match_fn         observer_is_match,
        observer_handle_fn        observer_handle,
        void                     *observer_handle_data,
        bool                      observer_auto_remove
        )
{
    pcintr_coroutine_t co = pcintr_get_coroutine();
    pcintr_stack_t stack = &co->stack;

    struct pcintr_observer *observer = pcintr_register_inner_observer(
            stack, cor_stage, cor_state,
            observed, event_type, event_sub_type,
            observer_is_match, observer_handle, observer_handle_data,
            observer_auto_remove
            );
    if (observer == NULL) {
        return -1;
    }

    pcintr_coroutine_set_state(co, CO_STATE_STOPPED);
    return 0;
}

void pcintr_resume(pcintr_coroutine_t co, pcrdr_msg *msg)
{
    UNUSED_PARAM(msg);
    PC_ASSERT(co);
    PC_ASSERT(co->state == CO_STATE_STOPPED);

    pcintr_stack_t stack = &co->stack;
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame);

    pcintr_coroutine_set_state(co, CO_STATE_RUNNING);
    pcintr_check_after_execution_full(pcinst_current(), co);
}

static int
serial_element(const char *buf, size_t len, void *ctxt)
{
    purc_rwstream_t rws = (purc_rwstream_t) ctxt;
    purc_rwstream_write(rws, buf, len);
    return 0;
}

static int
serial_symbol_vars(const char *symbol, int id,
        struct pcintr_stack_frame *frame, purc_rwstream_t stm)
{
    purc_rwstream_write(stm, symbol, strlen(symbol));
    size_t len_expected = 0;
    purc_variant_serialize(frame->symbol_vars[id],
            stm, 0,
            PCVARIANT_SERIALIZE_OPT_REAL_EJSON |
            PCVARIANT_SERIALIZE_OPT_BSEQUENCE_BASE64 |
            PCVARIANT_SERIALIZE_OPT_PLAIN,
            &len_expected);
    purc_rwstream_write(stm, "\n", 1);
    return 0;
}

#define DUMP_BUF_SIZE    128

static int
dump_stack_frame(pcintr_stack_t stack,
        struct pcintr_stack_frame *frame, purc_rwstream_t stm, int level)
{
    UNUSED_PARAM(stack);
    char buf[DUMP_BUF_SIZE];
    pcvdom_element_t elem = frame->pos;
    if (!elem) {
        goto out;
    }

    snprintf(buf, DUMP_BUF_SIZE, "\n%02d:\nframe = %p\n", level, frame);
    purc_rwstream_write(stm, buf, strlen(buf));

    snprintf(buf, DUMP_BUF_SIZE, "elem = ");
    purc_rwstream_write(stm, buf, strlen(buf));
    pcvdom_util_node_serialize_alone(&elem->node, serial_element, stm);

    struct pcvdom_node *child = pcvdom_node_first_child(&elem->node);
    if (child && child->type == PCVDOM_NODE_CONTENT) {
        snprintf(buf, DUMP_BUF_SIZE, "content = ");
        purc_rwstream_write(stm, buf, strlen(buf));
        pcvdom_util_node_serialize_alone(child, serial_element, stm);
    }
    else {
        snprintf(buf, DUMP_BUF_SIZE, "content = \n");
    }

    serial_symbol_vars("$< = ", PURC_SYMBOL_VAR_LESS_THAN, frame, stm);
    serial_symbol_vars("$@ = ", PURC_SYMBOL_VAR_AT_SIGN, frame, stm);
    serial_symbol_vars("$! = ", PURC_SYMBOL_VAR_EXCLAMATION, frame, stm);
    serial_symbol_vars("$: = ", PURC_SYMBOL_VAR_COLON, frame, stm);
    serial_symbol_vars("$= = ", PURC_SYMBOL_VAR_EQUAL, frame, stm);
    serial_symbol_vars("$% = ", PURC_SYMBOL_VAR_PERCENT_SIGN, frame, stm);
    serial_symbol_vars("$^ = ", PURC_SYMBOL_VAR_CARET, frame, stm);

out:
    return 0;
}

int
purc_coroutine_dump_stack(purc_coroutine_t cor, purc_rwstream_t stm)
{
    int ret = 0;
    if (!cor || !stm) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    pcintr_stack_t stack = &cor->stack;
    struct pcintr_stack_frame *p = pcintr_stack_get_bottom_frame(stack);
    int level = 0;
    while (p && p->pos) {
        ret = dump_stack_frame(stack, p, stm, level);
        if (ret != 0) {
            goto out;
        }
        p = pcintr_stack_frame_get_parent(p);
        level++;
    }

out:
    return ret;
}
