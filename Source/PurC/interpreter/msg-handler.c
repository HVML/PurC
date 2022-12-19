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

static bool
is_sub_exit_observer_match(struct pcintr_observer *observer, pcrdr_msg *msg,
        purc_variant_t observed, purc_atom_t type, const char *sub_type)
{
    UNUSED_PARAM(observed);
    UNUSED_PARAM(msg);
    UNUSED_PARAM(observer);
    UNUSED_PARAM(sub_type);
    bool match = false;
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(MSG, SUBEXIT)) == type) {
        match = true;
        goto out;
    }

out:
    return match;
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
sub_exit_observer_handle(pcintr_coroutine_t cor, struct pcintr_observer *observer,
        pcrdr_msg *msg, purc_atom_t type, const char *sub_type, void *data)
{
    UNUSED_PARAM(cor);
    UNUSED_PARAM(observer);
    UNUSED_PARAM(msg);
    UNUSED_PARAM(type);
    UNUSED_PARAM(sub_type);
    UNUSED_PARAM(data);
    UNUSED_PARAM(msg);

    on_sub_exit_event(cor, msg);
    return 0;
}

void
pcintr_coroutine_add_sub_exit_observer(pcintr_coroutine_t co)
{
    UNUSED_PARAM(co);

    /* just for observer->observed */
    purc_variant_t observed = purc_variant_make_ulongint(co->cid);
    pcintr_register_inner_observer(
            &co->stack,
            CO_STAGE_FIRST_RUN | CO_STAGE_OBSERVING,
            CO_STATE_READY | CO_STATE_OBSERVING,
            observed,
            MSG_TYPE_SUB_EXIT,
            NULL,
            is_sub_exit_observer_match,
            sub_exit_observer_handle,
            NULL,
            false
        );

    purc_variant_unref(observed);
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

static int
dispatch_coroutine_event_from_move_buffer(struct pcinst *inst,
        const pcrdr_msg *msg)
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
    struct list_head *crtns;
    pcintr_coroutine_t p, q;
    if (PURC_EVENT_TARGET_BROADCAST != msg_clone->targetValue) {
        crtns = &heap->crtns;
        list_for_each_entry_safe(p, q, crtns, ln) {
            pcintr_coroutine_t co = p;
            if (co->cid == msg->targetValue) {
                return pcinst_msg_queue_append(co->mq, msg_clone);
            }
        }

        crtns = &heap->stopped_crtns;
        list_for_each_entry_safe(p, q, crtns, ln) {
            pcintr_coroutine_t co = p;
            if (co->cid == msg->targetValue) {
                return pcinst_msg_queue_append(co->mq, msg_clone);
            }
        }
        pcrdr_release_message(msg_clone);
    }
    else {
        crtns = &heap->crtns;
        list_for_each_entry_safe(p, q, crtns, ln) {
            pcintr_coroutine_t co = p;
            pcrdr_msg *my_msg = pcrdr_clone_message(msg_clone);
            my_msg->targetValue = co->cid;
            pcinst_msg_queue_append(co->mq, my_msg);
        }

        crtns = &heap->stopped_crtns;
        list_for_each_entry_safe(p, q, crtns, ln) {
            pcintr_coroutine_t co = p;
            pcrdr_msg *my_msg = pcrdr_clone_message(msg_clone);
            my_msg->targetValue = co->cid;
            pcinst_msg_queue_append(co->mq, my_msg);
        }
        pcrdr_release_message(msg_clone);
    }
    return 0;
}

static int
dispatch_inst_event_from_move_buffer(struct pcinst *inst,
        const pcrdr_msg *msg)
{
    UNUSED_PARAM(inst);
    UNUSED_PARAM(msg);
    UNUSED_PARAM(inst);

    int ret = -1;
    struct pcintr_heap *heap = inst->intr_heap;
    if (!heap || inst->endpoint_atom != msg->targetValue) {
        goto out;
    }

    purc_variant_t request_id = msg->requestId;
    if (!pcintr_is_request_id(request_id)) {
        goto out;
    }

    const char *res = pcintr_request_id_get_res(request_id);
    enum pcintr_request_id_type type = pcintr_request_id_get_type(request_id);
    switch (type) {
    case PCINTR_REQUEST_ID_TYPE_ELEMENTS:
        purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
        break;

    case PCINTR_REQUEST_ID_TYPE_CRTN:
    {
        pcintr_coroutine_t crtn = pcintr_get_crtn_by_token(inst, res);
        if (!crtn) {
            goto out;
        }
        purc_variant_t v = purc_variant_make_ulongint(crtn->cid);
        pcintr_post_event(0, crtn->cid, msg->reduceOpt, msg->sourceURI, v,
            msg->eventName, msg->data, v);
        purc_variant_unref(v);
        break;
    }

    case PCINTR_REQUEST_ID_TYPE_CHAN:
        purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
        break;

    case PCINTR_REQUEST_ID_TYPE_RDR:
        purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
        break;

    case PCINTR_REQUEST_ID_TYPE_INVALID:
    default:
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        break;
    }


out:
    return ret;
}


static
purc_vdom_t find_vdom_by_target_vdom(uint64_t handle, pcintr_stack_t *pstack)
{
    pcintr_heap_t heap = pcintr_get_heap();
    if (heap == NULL) {
        return NULL;
    }

    struct list_head *crtns;
    pcintr_coroutine_t p, q;

    crtns = &heap->crtns;
    list_for_each_entry_safe(p, q, crtns, ln) {
        pcintr_coroutine_t co = p;
        if (handle == co->target_dom_handle) {
            if (pstack) {
                *pstack = &(co->stack);
            }
            return co->stack.vdom;
        }
    }

    crtns = &heap->stopped_crtns;
    list_for_each_entry_safe(p, q, crtns, ln) {
        pcintr_coroutine_t co = p;
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

    struct list_head *crtns;
    pcintr_coroutine_t p, q;

    crtns = &heap->crtns;
    list_for_each_entry_safe(p, q, crtns, ln) {
        pcintr_coroutine_t co = p;
        if (handle == co->target_page_handle) {
            if (pstack) {
                *pstack = &(co->stack);
            }
            return co->stack.vdom;
        }
    }

    crtns = &heap->stopped_crtns;
    list_for_each_entry_safe(p, q, crtns, ln) {
        pcintr_coroutine_t co = p;
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
        dispatch_coroutine_event_from_move_buffer(inst, msg);
        return;
    }
    else if (msg->target == PCRDR_MSG_TARGET_INSTANCE) {
        dispatch_inst_event_from_move_buffer(inst, msg);
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
            else if (msg->elementType == PCRDR_MSG_ELEMENT_TYPE_ID) {
                find_vdom_by_target_vdom((uint64_t)msg->targetValue, &stack);
                size_t nr = strlen(element) + 1;
                char *buf = (char*)malloc(nr + 1);
                if (!buf) {
                    goto out;
                }

                buf[0] = '#';
                strcpy(buf+1, element);
                source = purc_variant_make_string_reuse_buff(buf, nr, true);
                if (!source) {
                    free(buf);
                    goto out;
                }
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

    pcintr_post_event(0, stack->co->cid, msg->reduceOpt, source_uri, source,
            msg->eventName, msg->data, PURC_VARIANT_INVALID);
    purc_variant_unref(source_uri);

out:
    if (source) {
        purc_variant_unref(source);
    }
}

int
pcintr_post_event(purc_atom_t rid, purc_atom_t cid,
        pcrdr_msg_event_reduce_opt reduce_op, purc_variant_t source_uri,
        purc_variant_t element_value, purc_variant_t event_name,
        purc_variant_t data, purc_variant_t request_id)
{
    UNUSED_PARAM(source_uri);
    UNUSED_PARAM(event_name);
    UNUSED_PARAM(data);

    if (!event_name || ((rid == 0) && (cid == 0))) {
        return -1;
    }

    if (!rid) {
        rid = purc_get_rid_by_cid(cid);
    }

    pcrdr_msg *msg = pcinst_get_message();
    if (msg == NULL) {
        return -1;
    }

    msg->type = PCRDR_MSG_TYPE_EVENT;
    if (cid) {
        msg->target = PCRDR_MSG_TARGET_COROUTINE;
        msg->targetValue = cid;
    }
    else {
        msg->target = PCRDR_MSG_TARGET_INSTANCE;
        msg->targetValue = rid;
    }
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
    if (inst->endpoint_atom == rid) {
        ret = purc_inst_post_event(PURC_EVENT_TARGET_SELF, msg);
        goto out;
    }
    ret = purc_inst_post_event(rid, msg);

out:
    // clone messae set type error purc_variant_get_string_const
    if (purc_get_last_error() == PCVRNT_ERROR_INVALID_TYPE) {
        purc_clr_error();
    }
    return ret;
}

int
pcintr_post_event_by_ctype(purc_atom_t rid, purc_atom_t cid,
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

    int ret = pcintr_post_event(rid, cid, reduce_op, source_uri, element_value,
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

    int ret = pcintr_post_event_by_ctype(0, cid, reduce_op, source_uri,
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

