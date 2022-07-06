/**
 * @file msg-handler.c
 * @author Xue Shuming
 * @date 2022/07/01
 * @brief The message handler for instance
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
 *
 */

#include "config.h"

#include "internal.h"
#include "ops.h"
#include "private/instance.h"
#include "private/msg-queue.h"
#include "private/interpreter.h"
#include "private/regex.h"

#define PLOG(...) do {                                                        \
    FILE *fp = fopen("/tmp/plog.log", "a+");                                  \
    fprintf(fp, ##__VA_ARGS__);                                               \
    fclose(fp);                                                               \
} while (0)


#define EXCLAMATION_EVENT_NAME     "_eventName"
#define EXCLAMATION_EVENT_SOURCE   "_eventSource"

static void
on_observer_matched(void *ud)
{
    struct pcintr_observer_matched_data *p;
    p = (struct pcintr_observer_matched_data*)ud;
    PC_ASSERT(p);

    pcintr_stack_t stack = pcintr_get_stack();
    PC_ASSERT(stack);
    pcintr_coroutine_t co = stack->co;

    PC_ASSERT(co->state == CO_STATE_RUN);
    co->state = CO_STATE_RUN;

    // FIXME:
    // push stack frame
    struct pcintr_stack_frame_normal *frame_normal;
    frame_normal = pcintr_push_stack_frame_normal(stack);
    PC_ASSERT(frame_normal);

    struct pcintr_stack_frame *frame;
    frame = &frame_normal->frame;

    frame->ops = pcintr_get_ops_by_element(p->pos);
    frame->scope = p->scope;
    frame->pos = p->pos;
    frame->silently = pcintr_is_element_silently(frame->pos) ? 1 : 0;
    frame->edom_element = p->edom_element;
    frame->next_step = NEXT_STEP_AFTER_PUSHED;

    if (p->payload) {
        pcintr_set_question_var(frame, p->payload);
        purc_variant_unref(p->payload);
    }

    if (p->at_symbol) {
        pcintr_set_at_var(frame, p->at_symbol);
        purc_variant_unref(p->at_symbol);
    }

    purc_variant_t exclamation_var = pcintr_get_exclamation_var(frame);
    // set $! _eventName
    if (p->event_name) {
        purc_variant_object_set_by_static_ckey(exclamation_var,
                EXCLAMATION_EVENT_NAME, p->event_name);
        purc_variant_unref(p->event_name);
    }

    // set $! _eventSource
    if (p->source) {
        purc_variant_object_set_by_static_ckey(exclamation_var,
                EXCLAMATION_EVENT_SOURCE, p->source);
        purc_variant_unref(p->source);
    }

    pcintr_execute_one_step_for_ready_co(co);

    free(p);
}

void
observer_matched(pcintr_stack_t stack, struct pcintr_observer *p,
        purc_variant_t payload, purc_variant_t source, purc_variant_t event_name)
{
    PC_ASSERT(stack);
    pcintr_coroutine_t co = stack->co;
    PC_ASSERT(&co->stack == stack);

    pcintr_coroutine_t cco = pcintr_get_coroutine();
    if (!cco) {
        pcintr_set_current_co(co);
    }

    struct pcintr_observer_matched_data *data;
    data = (struct pcintr_observer_matched_data*)calloc(1, sizeof(*data));
    PC_ASSERT(data);
    data->pos = p->pos;
    data->scope = p->scope;
    data->edom_element = p->edom_element;

    if (event_name) {
        data->event_name = event_name;
        purc_variant_ref(data->event_name);
    }

    if (source) {
        data->source = source;
        purc_variant_ref(data->source);
    }
    if (payload) {
        data->payload = payload;
        purc_variant_ref(data->payload);
    }

    if (p->at_symbol) {
        data->at_symbol = p->at_symbol;
        purc_variant_ref(data->at_symbol);
    }

    pcintr_post_msg(data, on_observer_matched);
    pcintr_check_after_execution();
    if (!cco) {
        pcintr_set_current_co(NULL);
    }
}

static void handle_vdom_event(pcintr_stack_t stack, purc_vdom_t vdom,
        purc_atom_t type, purc_variant_t sub_type, purc_variant_t data)
{
    UNUSED_PARAM(stack);
    UNUSED_PARAM(vdom);
    UNUSED_PARAM(sub_type);
    UNUSED_PARAM(data);
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(MSG, CLOSE)) == type) {
        // TODO : quit runner
        fprintf(stderr, "## event msg not handle : close\n");
    }
}

int
process_coroutine_event(pcintr_coroutine_t co, pcrdr_msg *msg)
{
    pcintr_stack_t stack = &co->stack;
    PC_ASSERT(stack);
    pcintr_coroutine_t cco = pcintr_get_coroutine();
    if (!cco) {
        pcintr_set_current_co(co);
    }

#if 0
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame == NULL);
#endif

    purc_variant_t msg_type = PURC_VARIANT_INVALID;
    purc_variant_t msg_sub_type = PURC_VARIANT_INVALID;
    const char *event = purc_variant_get_string_const(msg->eventName);
    if (!pcintr_parse_event(event, &msg_type, &msg_sub_type)) {
        return -1;
    }

    const char *msg_type_s = purc_variant_get_string_const(msg_type);
    PC_ASSERT(msg_type_s);

    const char *sub_type_s = NULL;
    if (msg_sub_type != PURC_VARIANT_INVALID) {
        sub_type_s = purc_variant_get_string_const(msg_sub_type);
    }

    purc_atom_t msg_type_atom = purc_atom_try_string_ex(ATOM_BUCKET_MSG,
            msg_type_s);
    PC_ASSERT(msg_type_atom);

    purc_variant_t observed = msg->elementValue;

    bool handle = false;
    struct list_head* list = pcintr_get_observer_list(stack, observed);
    struct pcintr_observer *p, *n;
    list_for_each_entry_safe(p, n, list, node) {
        if (pcintr_is_observer_match(p, observed, msg_type_atom, sub_type_s)) {
            handle = true;
            observer_matched(stack, p, msg->data, msg->sourceURI, msg->eventName);
        }
    }

    if (!handle && purc_variant_is_native(observed)) {
        void *dest = purc_variant_native_get_entity(observed);
        // window close event dispatch to vdom
        if (dest == stack->vdom) {
            handle_vdom_event(stack, stack->vdom, msg_type_atom,
                    msg_sub_type, msg->data);
        }
    }

    if (msg_type) {
        purc_variant_unref(msg_type);
    }
    if (msg_sub_type) {
        purc_variant_unref(msg_sub_type);
    }

    if (!cco) {
        pcintr_set_current_co(NULL);
    }
    return 0;
}

int
dispatch_coroutine_msg(pcintr_coroutine_t co, pcrdr_msg *msg)
{
    if (!co || !msg) {
        return 0;
    }

    switch (msg->type) {
    case PCRDR_MSG_TYPE_EVENT:
        return process_coroutine_event(co, msg);

    case PCRDR_MSG_TYPE_VOID:
        PC_ASSERT(0);
        break;

    case PCRDR_MSG_TYPE_REQUEST:
        PC_ASSERT(0);
        break;

    case PCRDR_MSG_TYPE_RESPONSE:
        PC_ASSERT(0);
        break;

    default:
        // NOTE: shouldn't happen, no way to recover gracefully, fail-fast
        PC_ASSERT(0);
    }
    return 0;
}

int
dispatch_move_buffer_msg(struct pcinst *inst, pcrdr_msg *msg)
{
    UNUSED_PARAM(inst);
    struct pcintr_heap *heap = inst->intr_heap;
    if (!heap) {
        return 0;
    }

    switch (msg->type) {
    case PCRDR_MSG_TYPE_EVENT:
    {
        purc_variant_t elementValue = msg->elementValue;
        if (!purc_variant_is_string(elementValue)) {
            PC_WARN("invalid elementvalue for broadcast event");
            return 0;
        }

        purc_variant_t observed = pcinst_get_session_variables(
                purc_variant_get_string_const(elementValue));
        if (!observed) {
            PC_WARN("can not found observed for broadcast event %s",
                    purc_variant_get_string_const(elementValue));
            return 0;
        }

        if (msg->elementValue) {
            purc_variant_unref(msg->elementValue);
        }
        msg->elementValue = observed;
        purc_variant_ref(msg->elementValue);

        // add msg to coroutine message queue
        struct rb_root *coroutines = &heap->coroutines;
        struct rb_node *p, *n;
        struct rb_node *first = pcutils_rbtree_first(coroutines);

        if (PURC_EVENT_TARGET_BROADCAST != msg->targetValue) {
            pcutils_rbtree_for_each_safe(first, p, n) {
                pcintr_coroutine_t co = container_of(p, struct pcintr_coroutine,
                        node);
                if (co->ident == msg->targetValue) {
                    return pcinst_msg_queue_append(co->mq, msg);
                }
            }
        }
        else {
            pcutils_rbtree_for_each_safe(first, p, n) {
                pcintr_coroutine_t co = container_of(p, struct pcintr_coroutine,
                        node);

                pcrdr_msg *my_msg = pcrdr_clone_message(msg);
                my_msg->targetValue = co->ident;
                pcinst_msg_queue_append(co->mq, my_msg);
            }
            pcinst_put_message(msg);
        }
    }
        break;

    case PCRDR_MSG_TYPE_VOID:
        PC_ASSERT(0);
        break;

    case PCRDR_MSG_TYPE_REQUEST:
        PC_ASSERT(0);
        break;

    case PCRDR_MSG_TYPE_RESPONSE:
        PC_ASSERT(0);
        break;

    default:
        // NOTE: shouldn't happen, no way to recover gracefully, fail-fast
        PC_ASSERT(0);
    }
    return 0;
}


void
handle_move_buffer_msg(void)
{
    size_t n;
    int r = purc_inst_holding_messages_count(&n);
    PC_ASSERT(r == 0);
    if (n <= 0) {
        return;
    }

    struct pcinst *inst = pcinst_current();

    for (size_t i = 0; i < n; i++) {
        pcrdr_msg *msg = purc_inst_take_away_message(0);
        if (msg == NULL) {
            PC_ASSERT(purc_get_last_error() == 0);
            return;
        }

        // TODO: how to handle dispatch failed
        dispatch_move_buffer_msg(inst, msg);
        pcinst_put_message(msg);
    }
}


void
handle_coroutine_msg(pcintr_coroutine_t co)
{
    UNUSED_PARAM(co);
    if (co == NULL
            || co->state == CO_STATE_WAIT
            || co->state == CO_STATE_RUN) {
        return;
    }

    struct pcinst_msg_queue *queue = co->mq;
    pcrdr_msg *msg = pcinst_msg_queue_get_msg(queue);
    while (msg) {
        dispatch_coroutine_msg(co, msg);
        pcinst_put_message(msg);
        msg = pcinst_msg_queue_get_msg(queue);
    }
}

void
pcintr_dispatch_msg(void)
{
    // handle msg from move buffer
    handle_move_buffer_msg();

    // handle msg from message queue of the current co
    struct pcintr_heap *heap = pcintr_get_heap();
    struct rb_root *coroutines = &heap->coroutines;
    struct rb_node *p, *n;
    struct rb_node *first = pcutils_rbtree_first(coroutines);
    pcutils_rbtree_for_each_safe(first, p, n) {
        pcintr_coroutine_t co;
        co = container_of(p, struct pcintr_coroutine, node);
        handle_coroutine_msg(co);
    }
}

int
pcintr_post_event(purc_atom_t co_id,
        pcrdr_msg_event_reduce_opt reduce_op, purc_variant_t source_uri,
        purc_variant_t observed, purc_variant_t event_name,
        purc_variant_t data)
{
    UNUSED_PARAM(source_uri);
    UNUSED_PARAM(event_name);
    UNUSED_PARAM(data);

    if (!event_name) {
        return -1;
    }

    pcrdr_msg *msg = pcinst_get_message();
    if (msg == NULL) {
        return -1;
    }

    msg->type = PCRDR_MSG_TYPE_EVENT;
    msg->target = PCRDR_MSG_TARGET_COROUTINE;
    msg->targetValue = co_id;
    msg->reduceOpt = reduce_op;

    if (source_uri) {
        msg->sourceURI = source_uri;
        purc_variant_ref(msg->sourceURI);
    }

    msg->eventName = event_name;
    purc_variant_ref(msg->eventName);

    msg->elementType = PCRDR_MSG_ELEMENT_TYPE_VARIANT;
    msg->elementValue = observed;
    purc_variant_ref(msg->elementValue);

    if (data) {
        msg->dataType = PCRDR_MSG_DATA_TYPE_JSON;
        msg->data = data;
        purc_variant_ref(msg->data);
    }

    return purc_inst_post_event(PURC_EVENT_TARGET_SELF, msg);
}

int
pcintr_post_event_by_ctype(purc_atom_t co_id,
        pcrdr_msg_event_reduce_opt reduce_op, purc_variant_t source_uri,
        purc_variant_t observed, const char *event_type,
        const char *event_sub_type, purc_variant_t data)
{
    if (!event_type) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }
    size_t n = strlen(event_type) + 1;
    if (event_sub_type) {
        n = n +  strlen(event_sub_type) + 2;
    }

    char *p = (char*)malloc(n);
    if (!p) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }
    if (event_sub_type) {
        sprintf(p, "%s:%s", event_type, event_sub_type);
    }
    else {
        sprintf(p, "%s", event_type);
    }

    purc_variant_t event_name = purc_variant_make_string_reuse_buff(p,
            strlen(p), true);
    if (!event_name) {
        free(p);
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }

    int ret = pcintr_post_event(co_id, reduce_op, source_uri, observed,
            event_name, data);
    purc_variant_unref(event_name);

    return ret;
}

