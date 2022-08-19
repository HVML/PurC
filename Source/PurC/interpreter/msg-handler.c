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

#include <sys/time.h>

#define EXCLAMATION_EVENT_NAME     "_eventName"
#define EXCLAMATION_EVENT_SOURCE   "_eventSource"
#define OBSERVER_EVENT_HANDER      "_observer_event_handler"
#define SUB_EXIT_EVENT_HANDER      "_sub_exit_event_handler"
#define LAST_MSG_EVENT_HANDER      "_last_msg_event_handler"

static void
destroy_task(struct pcintr_observer_task *task)
{
    if (!task) {
        return;
    }

    if (task->payload) {
        purc_variant_unref(task->payload);
    }

    if (task->event_name) {
        purc_variant_unref(task->event_name);
    }

    if (task->source) {
        purc_variant_unref(task->source);
    }

    free(task);
}

void
pcintr_handle_task(struct pcintr_observer_task *task)
{
    pcintr_stack_t stack = task->stack;
    PC_ASSERT(stack);
    pcintr_coroutine_t co = stack->co;

    //PC_ASSERT(co->state == CO_STATE_RUNNING);

    // FIXME:
    // push stack frame
    struct pcintr_stack_frame_normal *frame_normal;
    frame_normal = pcintr_push_stack_frame_normal(stack);
    PC_ASSERT(frame_normal);

    struct pcintr_stack_frame *frame;
    frame = &frame_normal->frame;

    frame->ops = pcintr_get_ops_by_element(task->pos);
    frame->scope = task->scope;
    frame->pos = task->pos;
    frame->silently = pcintr_is_element_silently(frame->pos) ? 1 : 0;
    frame->edom_element = task->edom_element;
    frame->next_step = NEXT_STEP_AFTER_PUSHED;

    if (task->payload) {
        pcintr_set_question_var(frame, task->payload);
    }

    PC_ASSERT(frame->edom_element);
    pcintr_refresh_at_var(frame);

    purc_variant_t exclamation_var = pcintr_get_exclamation_var(frame);
    // set $! _eventName
    if (task->event_name) {
        purc_variant_object_set_by_static_ckey(exclamation_var,
                EXCLAMATION_EVENT_NAME, task->event_name);
    }

    // set $! _eventSource
    if (task->source) {
        purc_variant_object_set_by_static_ckey(exclamation_var,
                EXCLAMATION_EVENT_SOURCE, task->source);
    }

    // scheduler by pcintr_schedule
    pcintr_coroutine_set_state(co, CO_STATE_READY);

    destroy_task(task);
}


int
process_coroutine_event(pcintr_coroutine_t co, pcrdr_msg *msg)
{
    pcintr_stack_t stack = &co->stack;
    PC_ASSERT(stack);

    if (!msg->elementValue) {
        return -1;
    }

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
    if (!msg_type_atom) {
        return -1;
    }

    purc_variant_t observed = msg->elementValue;

    struct list_head* list = &stack->hvml_observers;
    struct pcintr_observer *p, *n;
    list_for_each_entry_safe(p, n, list, node) {
        if (p->is_match(p, msg, observed, msg_type_atom, sub_type_s)) {
            p->handle(co, p, msg, msg_type_atom, sub_type_s, p->handle_data);
            if (p->auto_remove) {
                pcintr_revoke_observer(p);
            }
        }
    }

    if (msg_type) {
        purc_variant_unref(msg_type);
    }
    if (msg_sub_type) {
        purc_variant_unref(msg_sub_type);
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

static bool
is_observer_event_handler_match(struct pcintr_event_handler *handler,
        pcintr_coroutine_t co, pcrdr_msg *msg, bool *out_observed)
{
    UNUSED_PARAM(handler);
    if (msg == NULL) {
        return true;
    }

    if (!msg->elementValue) {
        return false;
    }

    purc_variant_t msg_type = PURC_VARIANT_INVALID;
    purc_variant_t msg_sub_type = PURC_VARIANT_INVALID;
    const char *event = purc_variant_get_string_const(msg->eventName);
    if (!pcintr_parse_event(event, &msg_type, &msg_sub_type)) {
        return false;
    }

    const char *msg_type_s = purc_variant_get_string_const(msg_type);

    const char *sub_type_s = NULL;
    if (msg_sub_type != PURC_VARIANT_INVALID) {
        sub_type_s = purc_variant_get_string_const(msg_sub_type);
    }

    bool match = false;
    purc_atom_t msg_type_atom = purc_atom_try_string_ex(ATOM_BUCKET_MSG,
            msg_type_s);
    if (!msg_type_atom) {
        goto out;
    }

    purc_variant_t observed = msg->elementValue;
    struct list_head* list = &co->stack.hvml_observers;
    struct pcintr_observer *p, *n;
    list_for_each_entry_safe(p, n, list, node) {
        if (p->is_match(p, msg, observed, msg_type_atom, sub_type_s)) {
            match = true;
            break;
        }
    }

out:
    if (msg_type) {
        purc_variant_unref(msg_type);
    }
    if (msg_sub_type) {
        purc_variant_unref(msg_sub_type);
    }

    *out_observed = match;
    return match;
}

static int
observer_event_handle(struct pcintr_event_handler *handler,
        pcintr_coroutine_t co, pcrdr_msg *msg, bool *remove_handler,
        bool *performed)
{
    UNUSED_PARAM(handler);
    UNUSED_PARAM(co);
    UNUSED_PARAM(msg);

    int ret = PURC_ERROR_INCOMPLETED;
    *performed = false;
    *remove_handler = false;
    if (list_empty(&co->tasks) && msg) {
        if (PURC_ERROR_OK == dispatch_coroutine_msg(co, msg)) {
            ret = PURC_ERROR_OK;
        }
    }

    if (!list_empty(&co->tasks)) {
        struct pcintr_observer_task *task =
            list_first_entry(&co->tasks, struct pcintr_observer_task, ln);
        if (task) {
            list_del(&task->ln);
            pcintr_handle_task(task);
            *performed = true;
        }
    }
    return ret;
}

void pcintr_coroutine_add_observer_event_handler(pcintr_coroutine_t co)
{
    struct pcintr_event_handler *handler = pcintr_coroutine_add_event_handler(
            co,  OBSERVER_EVENT_HANDER,
            CO_STAGE_OBSERVING, CO_STATE_OBSERVING,
            NULL, observer_event_handle, is_observer_event_handler_match, true);
    PC_ASSERT(handler);
}

static bool
is_sub_exit_event_handler_match(struct pcintr_event_handler *handler,
        pcintr_coroutine_t co, pcrdr_msg *msg, bool *observed)
{
    UNUSED_PARAM(handler);
    UNUSED_PARAM(co);

    const char *event_name = purc_variant_get_string_const(msg->eventName);
    if (strcmp(event_name, MSG_TYPE_SUB_EXIT) == 0) {
        *observed = true;
        return true;
    }
    return false;
}

static void
on_sub_exit_event(pcintr_coroutine_t co, pcrdr_msg *msg)
{
    // msg->elementValue  : child->cid
    // msg->data : result

    uint64_t ul = 0;
    if (!purc_variant_cast_to_ulongint(msg->elementValue, &ul, true)) {
        return;
    }

    purc_atom_t child_cid = (purc_atom_t) ul;
    struct list_head *children = &co->children;
    struct list_head *p, *n;
    list_for_each_safe(p, n, children) {
        pcintr_coroutine_child_t child;
        child = list_entry(p, struct pcintr_coroutine_child, ln);
        if (child->cid == child_cid) {
            list_del(&child->ln);
            free(child);
        }
    }

    pcintr_check_after_execution_full(pcinst_current(), co);
}

static int
sub_exit_event_handle(struct pcintr_event_handler *handler,
        pcintr_coroutine_t co, pcrdr_msg *msg, bool *remove_handler,
        bool *performed)
{
    UNUSED_PARAM(handler);
    *remove_handler = false;
    *performed = true;

    on_sub_exit_event(co, msg);
    return PURC_ERROR_OK;
}

void
pcintr_coroutine_add_sub_exit_event_handler(pcintr_coroutine_t co)
{
    UNUSED_PARAM(co);
    struct pcintr_event_handler *handler = pcintr_coroutine_add_event_handler(
            co,  SUB_EXIT_EVENT_HANDER,
            CO_STAGE_FIRST_RUN | CO_STAGE_OBSERVING,
            CO_STATE_READY | CO_STATE_OBSERVING,
            NULL, sub_exit_event_handle, is_sub_exit_event_handler_match, false);
    PC_ASSERT(handler);
}
static bool
is_last_msg_observer_match(struct pcintr_observer *observer, pcrdr_msg *msg,
        purc_variant_t observed, purc_atom_t type, const char *sub_type)
{
    UNUSED_PARAM(observed);
    UNUSED_PARAM(msg);
    UNUSED_PARAM(observer);
    UNUSED_PARAM(sub_type);
    bool match = false;
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(MSG, LASTMSG)) == type) {
        match = true;
        goto out;
    }

out:
    return match;
}

static int
last_msg_observer_handle(pcintr_coroutine_t cor, struct pcintr_observer *observer,
        pcrdr_msg *msg, purc_atom_t type, const char *sub_type, void *data)
{
    UNUSED_PARAM(cor);
    UNUSED_PARAM(observer);
    UNUSED_PARAM(msg);
    UNUSED_PARAM(type);
    UNUSED_PARAM(sub_type);
    UNUSED_PARAM(data);
    UNUSED_PARAM(msg);

    cor->stack.last_msg_read = 1;
    pcintr_coroutine_set_state(cor, CO_STATE_RUNNING);
    pcintr_check_after_execution_full(pcinst_current(), cor);
    return 0;
}

void
pcintr_coroutine_add_last_msg_observer(pcintr_coroutine_t co)
{
    UNUSED_PARAM(co);

    purc_variant_t observed = purc_variant_make_ulongint(co->cid);
    pcintr_register_inner_observer(
            &co->stack,
            CO_STAGE_FIRST_RUN | CO_STAGE_OBSERVING,
            CO_STATE_READY | CO_STATE_OBSERVING | CO_STATE_EXITED,
            observed,
            MSG_TYPE_LAST_MSG,
            NULL,
            is_last_msg_observer_match,
            last_msg_observer_handle,
            NULL,
            true
        );

    purc_variant_unref(observed);
}

int
pcintr_coroutine_clear_tasks(pcintr_coroutine_t co)
{
    if (list_empty(&co->tasks)) {
        return 0;
    }
    struct list_head *tasks = &co->tasks;
    struct pcintr_observer_task *p, *n;
    list_for_each_entry_safe(p, n, tasks, ln) {
        list_del(&p->ln);
        destroy_task(p);
    }
    return 0;
}

int
dispatch_move_buffer_event(struct pcinst *inst, const pcrdr_msg *msg)
{
    UNUSED_PARAM(inst);
    struct pcintr_heap *heap = inst->intr_heap;
    if (!heap) {
        return 0;
    }

    purc_variant_t elementValue = PURC_VARIANT_INVALID;

    if (msg->elementValue && purc_variant_is_string(msg->elementValue)) {
        elementValue = pcinst_get_session_variables(
                purc_variant_get_string_const(msg->elementValue));
        if (!elementValue) {
            PC_WARN("can not found elementValue for broadcast event %s",
                    purc_variant_get_string_const(msg->elementValue));
            return 0;
        }
    }

    pcrdr_msg *msg_clone = pcrdr_clone_message(msg);
    if (elementValue) {
        if (msg_clone->elementValue) {
            purc_variant_unref(msg_clone->elementValue);
        }
        msg_clone->elementValue = elementValue;
        purc_variant_ref(msg_clone->elementValue);
    }

    pcintr_update_timestamp(inst);

    // add msg to coroutine message queue
    struct rb_root *coroutines = &heap->coroutines;
    struct rb_node *p, *n;
    struct rb_node *first = pcutils_rbtree_first(coroutines);

    if (PURC_EVENT_TARGET_BROADCAST != msg_clone->targetValue) {
        pcutils_rbtree_for_each_safe(first, p, n) {
            pcintr_coroutine_t co = container_of(p, struct pcintr_coroutine,
                    node);
            if (co->cid == msg->targetValue) {
                return pcinst_msg_queue_append(co->mq, msg_clone);
            }
        }
    }
    else {
        pcutils_rbtree_for_each_safe(first, p, n) {
            pcintr_coroutine_t co = container_of(p, struct pcintr_coroutine,
                    node);

            pcrdr_msg *my_msg = pcrdr_clone_message(msg_clone);
            my_msg->targetValue = co->cid;
            pcinst_msg_queue_append(co->mq, my_msg);
        }
        pcrdr_release_message(msg_clone);
    }
    return 0;
}

static
purc_vdom_t find_vdom_by_target_vdom(uint64_t handle, pcintr_stack_t *pstack)
{
    pcintr_heap_t heap = pcintr_get_heap();
    if (heap == NULL) {
        return NULL;
    }

    struct rb_node *p, *n;
    struct rb_node *first = pcutils_rbtree_first(&heap->coroutines);
    pcutils_rbtree_for_each_safe(first, p, n) {
        pcintr_coroutine_t co;
        co = container_of(p, struct pcintr_coroutine, node);

        if (handle == co->target_dom_handle) {
            if (pstack) {
                *pstack = &(co->stack);
            }
            return co->stack.vdom;
        }
    }
    return NULL;
}

static purc_vdom_t
find_vdom_by_target_window(uint64_t handle, pcintr_stack_t *pstack)
{
    pcintr_heap_t heap = pcintr_get_heap();
    if (heap == NULL) {
        return NULL;
    }

    struct rb_node *p, *n;
    struct rb_node *first = pcutils_rbtree_first(&heap->coroutines);
    pcutils_rbtree_for_each_safe(first, p, n) {
        pcintr_coroutine_t co;
        co = container_of(p, struct pcintr_coroutine, node);

        if (handle == co->target_page_handle) {
            if (pstack) {
                *pstack = &(co->stack);
            }
            return co->stack.vdom;
        }
    }
    return NULL;
}


void
pcintr_conn_event_handler(pcrdr_conn *conn, const pcrdr_msg *msg)
{
    UNUSED_PARAM(conn);
    UNUSED_PARAM(msg);
    struct pcinst *inst = pcinst_current();

    if (msg->target == PCRDR_MSG_TARGET_COROUTINE) {
        dispatch_move_buffer_event(inst, msg);
        return;
    }

    pcintr_stack_t stack = NULL;
    purc_variant_t source = PURC_VARIANT_INVALID;
    switch (msg->target) {
    case PCRDR_MSG_TARGET_SESSION:
        //TODO
        break;

    case PCRDR_MSG_TARGET_WORKSPACE:
        //TODO
        break;

    case PCRDR_MSG_TARGET_PLAINWINDOW:
        {
            purc_vdom_t vdom = find_vdom_by_target_window(
                    (uint64_t)msg->targetValue, &stack);
            const char *event = purc_variant_get_string_const(msg->eventName);
            if (!vdom) {
                PC_WARN("can not found vdom for event %s\n", event);
                return;
            }
            if (strcmp(event, MSG_TYPE_DESTROY) == 0) {
                stack->co->target_workspace_handle = 0;
                stack->co->target_page_handle = 0;
                stack->co->target_dom_handle = 0;
                purc_variant_t hvml = pcintr_get_coroutine_variable(stack->co,
                        PURC_PREDEF_VARNAME_CRTN);
                pcintr_coroutine_post_event(stack->co->cid,
                        PCRDR_MSG_EVENT_REDUCE_OPT_OVERLAY,
                        hvml, MSG_TYPE_RDR_STATE, MSG_SUB_TYPE_PAGE_CLOSED,
                        PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
            }
            return;
        }
        break;

    case PCRDR_MSG_TARGET_WIDGET:
        {
            purc_vdom_t vdom = find_vdom_by_target_window(
                    (uint64_t)msg->targetValue, &stack);
            const char *event = purc_variant_get_string_const(msg->eventName);
            if (!vdom) {
                PC_WARN("can not found vdom for event %s\n", event);
                return;
            }
            if (strcmp(event, MSG_TYPE_DESTROY) == 0) {
                stack->co->target_workspace_handle = 0;
                stack->co->target_page_handle = 0;
                stack->co->target_dom_handle = 0;
                purc_variant_t hvml = pcintr_get_coroutine_variable(stack->co,
                        PURC_PREDEF_VARNAME_CRTN);
                pcintr_coroutine_post_event(stack->co->cid,
                        PCRDR_MSG_EVENT_REDUCE_OPT_OVERLAY,
                        hvml, MSG_TYPE_RDR_STATE, MSG_SUB_TYPE_PAGE_CLOSED,
                        PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
            }
        }
        break;

    case PCRDR_MSG_TARGET_DOM:
        {
            const char *element = purc_variant_get_string_const(
                    msg->elementValue);
            if (element == NULL) {
                goto out;
            }

            if (msg->elementType == PCRDR_MSG_ELEMENT_TYPE_HANDLE) {
                unsigned long long int p = strtoull(element, NULL, 16);
                find_vdom_by_target_vdom((uint64_t)msg->targetValue, &stack);
                source = purc_variant_make_native((void*)(uint64_t)p, NULL);
            }
        }
        break;

    case PCRDR_MSG_TARGET_USER:
        //TODO
        break;

    default:
        goto out;
    }

    // FIXME: soure_uri msg->sourcURI or  co->full_name
    const char *uri = pcintr_coroutine_get_uri(stack->co);
    if (!uri) {
        goto out;
    }

    purc_variant_t source_uri = purc_variant_make_string(uri, false);
    if (!source_uri) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }

    pcintr_post_event(stack->co->cid, msg->reduceOpt, source_uri, source,
            msg->eventName, msg->data, PURC_VARIANT_INVALID);
    purc_variant_unref(source_uri);

out:
    if (source) {
        purc_variant_unref(source);
    }
}

int
pcintr_post_event(purc_atom_t cid,
        pcrdr_msg_event_reduce_opt reduce_op, purc_variant_t source_uri,
        purc_variant_t element_value, purc_variant_t event_name,
        purc_variant_t data, purc_variant_t request_id)
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
    msg->targetValue = cid;
    msg->reduceOpt = reduce_op;

    if (source_uri) {
        msg->sourceURI = source_uri;
        purc_variant_ref(msg->sourceURI);
    }

    msg->eventName = event_name;
    purc_variant_ref(msg->eventName);

    if (element_value) {
        msg->elementType = PCRDR_MSG_ELEMENT_TYPE_VARIANT;
        msg->elementValue = element_value;
        purc_variant_ref(msg->elementValue);
    }

    if (data) {
        msg->dataType = PCRDR_MSG_DATA_TYPE_JSON;
        msg->data = data;
        purc_variant_ref(msg->data);
    }

    if (request_id) {
        msg->requestId = request_id;
        purc_variant_ref(msg->requestId);
    }

    int ret;
    // XXX: only broadcast self inst coroutine
    if (cid == PURC_EVENT_TARGET_BROADCAST) {
        ret = purc_inst_post_event(PURC_EVENT_TARGET_SELF, msg);
        goto out;
    }

    struct pcinst *inst = pcinst_current();
    purc_atom_t rid = purc_get_rid_by_cid(cid);
    if (inst->endpoint_atom == rid) {
        ret = purc_inst_post_event(PURC_EVENT_TARGET_SELF, msg);
        goto out;
    }
    ret = purc_inst_post_event(rid, msg);

out:
    // clone messae set type error purc_variant_get_string_const
    if (purc_get_last_error() == PCVARIANT_ERROR_INVALID_TYPE) {
        purc_clr_error();
    }
    return ret;
}

int
pcintr_post_event_by_ctype(purc_atom_t cid,
        pcrdr_msg_event_reduce_opt reduce_op, purc_variant_t source_uri,
        purc_variant_t element_value, const char *event_type,
        const char *event_sub_type, purc_variant_t data,
        purc_variant_t request_id)
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

    int ret = pcintr_post_event(cid, reduce_op, source_uri, element_value,
            event_name, data, request_id);
    purc_variant_unref(event_name);

    return ret;
}

int
pcintr_coroutine_post_event(purc_atom_t cid,
        pcrdr_msg_event_reduce_opt reduce_op,
        purc_variant_t element_value, const char *event_type,
        const char *event_sub_type, purc_variant_t data,
        purc_variant_t request_id)
{
    const char *uri = purc_atom_to_string(cid);
    if (!uri) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    purc_variant_t source_uri = purc_variant_make_string(uri, false);
    if (!source_uri) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }

    int ret = pcintr_post_event_by_ctype(cid, reduce_op, source_uri,
            element_value, event_type, event_sub_type, data, request_id);

    purc_variant_unref(source_uri);
    return ret;
}

double
pcintr_get_current_time(void)
{
    struct timeval now;
    gettimeofday(&now, 0);
    return now.tv_sec * 1000 + now.tv_usec / 1000;
}

void
pcintr_update_timestamp(struct pcinst *inst)
{
    inst->intr_heap->timestamp = pcintr_get_current_time();
}

