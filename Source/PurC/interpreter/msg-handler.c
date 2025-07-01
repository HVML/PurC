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
#include "private/pcrdr.h"

#include <sys/time.h>

#define OBSERVER_EVENT_HANDER      "_observer_event_handler"
#define SUB_EXIT_EVENT_HANDER      "_sub_exit_event_handler"
#define LAST_MSG_EVENT_HANDER      "_last_msg_event_handler"

static void
destroy_task(struct pcintr_observer_task *task)
{
    if (!task) {
        return;
    }

    if (task->implicit_data) {
        purc_variant_unref(task->implicit_data);
    }

    if (task->observed) {
        purc_variant_unref(task->observed);
    }

    if (task->payload) {
        purc_variant_unref(task->payload);
    }

    if (task->event_name) {
        purc_variant_unref(task->event_name);
    }

    if (task->event_sub_name) {
        purc_variant_unref(task->event_sub_name);
    }

    if (task->source) {
        purc_variant_unref(task->source);
    }

    if (task->request_id) {
        purc_variant_unref(task->request_id);
    }

    free(task);
}

struct travel_elem_pointer {
    pcdoc_element_t elem;
    void *p;
};

static int
travel_elem_pointer_cb(purc_document_t doc, pcdoc_element_t element, void *ctxt)
{
    UNUSED_PARAM(doc);
    struct travel_elem_pointer *args = (struct travel_elem_pointer*)ctxt;

    if (element == args->p) {
        args->elem = element;
        return PCDOC_TRAVEL_STOP;
    }

    return PCDOC_TRAVEL_GOON;
}

static pcdoc_element_t
find_element_by_pointer(purc_document_t doc, void *p)
{
    pcdoc_element_t elem = NULL;
    if (!doc || !p) {
        goto out;
    }

    struct travel_elem_pointer data = {
        .elem = NULL,
        .p = p
    };

    pcdoc_travel_descendant_elements(doc, NULL, travel_elem_pointer_cb,
            &data, NULL);

    elem = data.elem;
out:
    return elem;
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
    frame->handle_event = 1;

    if (task->payload) {
        pcintr_set_question_var(frame, task->payload);
    }

    if (task->observed && purc_variant_is_native(task->observed)) {
        void *p = purc_variant_native_get_entity(task->observed);
        pcdoc_element_t e = find_element_by_pointer(stack->doc, p);
        if (e) {
            frame->edom_element = e;
        }
    }

    PC_ASSERT(frame->edom_element);
    pcintr_refresh_at_var(frame);

    purc_variant_t exclamation_var = pcintr_get_exclamation_var(frame);
    // set $! _eventName
    if (task->event_name) {
        purc_variant_object_set_by_static_ckey(exclamation_var,
                PCINTR_EXCLAMATION_EVENT_NAME, task->event_name);
    }

    // set $! _eventSubName
    if (task->event_sub_name) {
        purc_variant_object_set_by_static_ckey(exclamation_var,
                PCINTR_EXCLAMATION_EVENT_SUB_NAME, task->event_sub_name);
    }

    // set $! _eventSource
    if (task->source) {
        purc_variant_object_set_by_static_ckey(exclamation_var,
                PCINTR_EXCLAMATION_EVENT_SOURCE, task->source);
    }

    // set $! _eventRequestId
    if (task->request_id) {
        purc_variant_object_set_by_static_ckey(exclamation_var,
                PCINTR_EXCLAMATION_EVENT_REQUEST_ID, task->request_id);
    }

    if (task->implicit_data) {
        purc_variant_t k, v;
        foreach_key_value_in_variant_object(task->implicit_data, k, v) {
            purc_variant_object_set(exclamation_var, k, v);
        } end_foreach;
    }

    // scheduler by pcintr_schedule
    pcintr_coroutine_set_state(co, CO_STATE_READY);

    destroy_task(task);
}

static bool
is_last_msg_observer_match(pcintr_coroutine_t co,
        struct pcintr_observer *observer, pcrdr_msg *msg,
        purc_variant_t observed, const char *type, const char *sub_type)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(observed);
    UNUSED_PARAM(msg);
    UNUSED_PARAM(observer);
    UNUSED_PARAM(sub_type);
    bool match = false;
    if (strcmp(type, MSG_TYPE_LAST_MSG) == 0) {
        match = true;
        goto out;
    }

out:
    return match;
}

static int
last_msg_observer_handle(pcintr_coroutine_t cor, struct pcintr_observer *observer,
        pcrdr_msg *msg, const char *type, const char *sub_type, void *data)
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

    purc_atom_t cid = pcintr_request_id_get_cid(request_id);
    const char *res = pcintr_request_id_get_res(request_id);
    enum pcintr_request_id_type type = pcintr_request_id_get_type(request_id);
    switch (type) {
    case PCINTR_REQUEST_ID_TYPE_ELEMENTS:
        purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
        break;

    case PCINTR_REQUEST_ID_TYPE_CRTN:
    {
        pcintr_coroutine_t crtn;
        if (cid) {
            crtn = pcintr_coroutine_get_by_id(cid);
        }
        else {
            crtn = pcintr_get_crtn_by_token(inst, res);
        }
        if (!crtn) {
            purc_set_error_with_info(PURC_ERROR_NOT_SUPPORTED,
                    "Can not send request to coroutine '%s'", res);
            goto out;
        }
        if (!cid) {
            pcintr_request_id_set_cid(request_id, crtn->cid);
        }
        purc_variant_t v = purc_variant_make_ulongint(crtn->cid);
        pcintr_post_event(0, crtn->cid, msg->reduceOpt, msg->sourceURI, request_id,
            msg->eventName, msg->data, request_id);
        purc_variant_unref(v);
        break;
    }

    case PCINTR_REQUEST_ID_TYPE_CHAN:
    {
        ret = pcintr_chan_post(res, msg->data);
        break;
    }

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

static purc_vdom_t
find_vdom_by_target_window(uint64_t handle, pcintr_stack_t *pstack,
        bool ignore_supressed_co)
{
    pcintr_heap_t heap = pcintr_get_heap();
    if (heap == NULL) {
        return NULL;
    }

    size_t count = pcutils_sorted_array_count(heap->loaded_crtn_handles);
    for (size_t i = 0; i < count; i++) {
        pcintr_coroutine_t co = (pcintr_coroutine_t)pcutils_sorted_array_get(
                heap->loaded_crtn_handles, i, NULL);
        if (co->supressed && ignore_supressed_co) {
            continue;
        }
        if (pcintr_coroutine_is_match_page_handle(co, handle)) {
            if (pstack) {
                *pstack = &(co->stack);
            }
            return co->stack.vdom;
        }
    }

    return NULL;
}

static bool
is_crtn_observe_event(struct pcinst *inst,
        pcintr_coroutine_t co, const pcrdr_msg *msg, purc_variant_t source,
        const char *type, const char *sub_type)
{
    UNUSED_PARAM(inst);
    purc_variant_t observed = source;

    struct pcintr_observer *observer, *next;
    struct list_head *list = &co->stack.hvml_observers;

again:
    list_for_each_entry_safe(observer, next, list, node) {
        if (observer->is_match(co, observer, (pcrdr_msg *)msg, observed,
                    type, sub_type)) {
            return true;
        }
    }

    if (list != &co->stack.intr_observers) {
        list = &co->stack.intr_observers;
        goto again;
    }

    return false;
}

static void
on_plainwindow_event(struct pcinst *inst, pcrdr_conn *conn, const pcrdr_msg *msg,
        const char *type, const char *sub_type)
{
    UNUSED_PARAM(inst);
    UNUSED_PARAM(type);
    UNUSED_PARAM(sub_type);
    pcintr_stack_t stack = NULL;
    struct pcintr_coroutine_rdr_conn *rdr_conn = NULL;
    const char *event = purc_variant_get_string_const(msg->eventName);

    if (strcmp(event, MSG_TYPE_DESTROY) == 0) {
        purc_vdom_t vdom = find_vdom_by_target_window(
                (uint64_t)msg->targetValue, &stack, false);
        if (!vdom) {
            PC_WARN("can not found vdom for event %s\n", event);
            return;
        }

        rdr_conn = pcintr_coroutine_get_rdr_conn(stack->co, conn);
        if (rdr_conn) {
            pcintr_coroutine_destroy_rdr_conn(stack->co, rdr_conn);
        }
        purc_variant_t hvml = purc_variant_make_ulongint(stack->co->cid);
        pcintr_coroutine_post_event(stack->co->cid,
                PCRDR_MSG_EVENT_REDUCE_OPT_OVERLAY,
                hvml, MSG_TYPE_RDR_STATE, MSG_SUB_TYPE_PAGE_CLOSED,
                PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
        purc_variant_unref(hvml);
    }
    else if (strcmp(event, MSG_SUB_TYPE_PAGE_ACTIVATED) == 0) {
        purc_vdom_t vdom = find_vdom_by_target_window(
                (uint64_t)msg->targetValue, &stack, true);
        if (!vdom) {
            PC_WARN("can not found vdom for event %s\n", event);
            return;
        }

        purc_variant_t hvml = purc_variant_make_ulongint(stack->co->cid);
        pcintr_coroutine_post_event(stack->co->cid,
                PCRDR_MSG_EVENT_REDUCE_OPT_OVERLAY,
                hvml, MSG_TYPE_RDR_STATE, MSG_SUB_TYPE_PAGE_ACTIVATED,
                PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
        purc_variant_unref(hvml);
    }
    else if (strcmp(event, MSG_SUB_TYPE_PAGE_DEACTIVATED) == 0) {
        purc_vdom_t vdom = find_vdom_by_target_window(
                (uint64_t)msg->targetValue, &stack, true);
        if (!vdom) {
            PC_WARN("can not found vdom for event %s\n", event);
            return;
        }

        purc_variant_t hvml = purc_variant_make_ulongint(stack->co->cid);
        pcintr_coroutine_post_event(stack->co->cid,
                PCRDR_MSG_EVENT_REDUCE_OPT_OVERLAY,
                hvml, MSG_TYPE_RDR_STATE, MSG_SUB_TYPE_PAGE_DEACTIVATED,
                PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
        purc_variant_unref(hvml);
    }
}

static void
on_widget_event(struct pcinst *inst, pcrdr_conn *conn, const pcrdr_msg *msg,
        const char *type, const char *sub_type)
{
    UNUSED_PARAM(inst);
    UNUSED_PARAM(type);
    UNUSED_PARAM(sub_type);
    pcintr_stack_t stack = NULL;
    struct pcintr_coroutine_rdr_conn *rdr_conn = NULL;

    purc_vdom_t vdom = find_vdom_by_target_window(
            (uint64_t)msg->targetValue, &stack, false);

    const char *event = purc_variant_get_string_const(msg->eventName);
    if (!vdom) {
        PC_WARN("can not found vdom for event %s\n", event);
        return;
    }

    if (strcmp(event, MSG_TYPE_DESTROY) == 0) {
        rdr_conn = pcintr_coroutine_get_rdr_conn(stack->co, conn);
        if (rdr_conn) {
            pcintr_coroutine_destroy_rdr_conn(stack->co, rdr_conn);
        }
        purc_variant_t hvml = purc_variant_make_ulongint(stack->co->cid);
        pcintr_coroutine_post_event(stack->co->cid,
                PCRDR_MSG_EVENT_REDUCE_OPT_OVERLAY,
                hvml, MSG_TYPE_RDR_STATE, MSG_SUB_TYPE_PAGE_CLOSED,
                PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
        purc_variant_unref(hvml);
    }
}

static void
on_dom_event(struct pcinst *inst, pcrdr_conn *conn, const pcrdr_msg *msg,
        const char *type, const char *sub_type)
{
    UNUSED_PARAM(inst);
    UNUSED_PARAM(conn);
    UNUSED_PARAM(type);
    UNUSED_PARAM(sub_type);

    purc_variant_t source = PURC_VARIANT_INVALID;

    const char *element = purc_variant_get_string_const(
            msg->elementValue);
    if (element == NULL) {
        goto out;
    }

    if (msg->elementType == PCRDR_MSG_ELEMENT_TYPE_HANDLE) {
        unsigned long long int p = strtoull(element, NULL, 16);
        source = purc_variant_make_native((void*)(uint64_t)p, NULL);
    }
    else if (msg->elementType == PCRDR_MSG_ELEMENT_TYPE_ID) {
        size_t nr = strlen(element) + 1;
        char *buf = (char*)malloc(nr + 1);
        if (!buf) {
            goto out;
        }

        buf[0] = '#';
        strcpy(buf+1, element);
        source = purc_variant_make_string_reuse_buff(buf, nr + 1, true);
        if (!source) {
            free(buf);
            goto out;
        }
    }

    struct pcintr_heap *heap = inst->intr_heap;
    uint64_t handle = (uint64_t)msg->targetValue;
    size_t count = pcutils_sorted_array_count(heap->loaded_crtn_handles);
    for (size_t i = 0; i < count; i++) {
        pcintr_coroutine_t co = (pcintr_coroutine_t)pcutils_sorted_array_get(
                heap->loaded_crtn_handles, i, NULL);
        if (!pcintr_coroutine_is_match_dom_handle(co, handle) ||
                !is_crtn_observe_event(inst, co, msg, source,
                    type, sub_type)) {
            continue;
        }

        const char *uri = pcintr_coroutine_get_uri(co);
        if (!uri) {
            goto out;
        }

        purc_variant_t source_uri = purc_variant_make_string(uri, false);
        if (!source_uri) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto out;
        }

        pcintr_post_event(0, co->cid, msg->reduceOpt, source_uri, source,
                msg->eventName, msg->data, PURC_VARIANT_INVALID);
        purc_variant_unref(source_uri);

    }

out:
    if (source) {
        purc_variant_unref(source);
    }
}

#define KEY_COMM        "comm"
#define KEY_URI         "uri"

static void
broadcast_rdr_idle_event(struct pcinst *inst, purc_variant_t data)
{
    struct pcintr_heap *heap = inst->intr_heap;
    struct list_head *crtns = &heap->crtns;
    pcintr_coroutine_t p, q;
    list_for_each_entry_safe(p, q, crtns, ln) {
        pcintr_coroutine_t co = p;
        pcintr_stack_t stack = &co->stack;
        purc_variant_t hvml = pcintr_crtn_observed_create(co->cid);
        pcintr_coroutine_post_event(stack->co->cid,
                PCRDR_MSG_EVENT_REDUCE_OPT_OVERLAY,
                hvml, MSG_TYPE_RDR_STATE, MSG_TYPE_IDLE,
                data, PURC_VARIANT_INVALID);
        purc_variant_unref(hvml);
    }

    crtns = &heap->stopped_crtns;
    list_for_each_entry_safe(p, q, crtns, ln) {
        pcintr_coroutine_t co = p;
        pcintr_stack_t stack = &co->stack;
        purc_variant_t hvml = pcintr_crtn_observed_create(co->cid);
        pcintr_coroutine_post_event(stack->co->cid,
                PCRDR_MSG_EVENT_REDUCE_OPT_OVERLAY,
                hvml, MSG_TYPE_RDR_STATE, MSG_TYPE_IDLE,
                data, PURC_VARIANT_INVALID);
        purc_variant_unref(hvml);
    }
}

int
purc_connect_to_renderer_async(purc_instance_extra_info *extra_info);

static void
on_session_event(struct pcinst *inst, pcrdr_conn *conn, const pcrdr_msg *msg,
        const char *type, const char *sub_type)
{
    UNUSED_PARAM(inst);
    UNUSED_PARAM(conn);
    UNUSED_PARAM(sub_type);

    if (strcmp(type, MSG_TYPE_DUP_RENDERER) == 0) {
        purc_variant_t data = msg->data;
        purc_variant_t uri = purc_variant_object_get_by_ckey_ex(data, KEY_URI,
                true);
        if (!uri) {
            PC_WARN("Invalid '%s' event, '%s' not found.",
                    MSG_TYPE_NEW_RENDERER, KEY_URI);
            return;
        }

        const char *s_uri = purc_variant_get_string_const(uri);
        purc_atom_t atom = purc_atom_try_string_ex(ATOM_BUCKET_RDRID, s_uri);
        if (atom) {
            PC_WARN("Conflict uri '%s' , do nothing.", s_uri);
            return;
        }

        purc_variant_t comm = purc_variant_object_get_by_ckey_ex(data,
                KEY_COMM, true);
        if (!comm) {
            PC_WARN("Invalid '%s' event, '%s' not found.",
                    MSG_TYPE_NEW_RENDERER, KEY_COMM);
            return;
        }

        const char *s_comm = purc_variant_get_string_const(comm);
        PC_TIMESTAMP("receive dup renderer event url: %s app: %s runner: %s\n",
                s_uri, inst->app_name, inst->runner_name);

        purc_instance_extra_info extra_info = {0};
        if (strcasecmp(s_comm, PURC_RDRCOMM_NAME_HEADLESS) == 0) {
            extra_info.renderer_comm = PURC_RDRCOMM_HEADLESS;
        }
        else if (strcasecmp(s_comm, PURC_RDRCOMM_NAME_SOCKET) == 0) {
            extra_info.renderer_comm = PURC_RDRCOMM_SOCKET;
        }
        else if (strcasecmp(s_comm, PURC_RDRCOMM_NAME_THREAD) == 0) {
            extra_info.renderer_comm = PURC_RDRCOMM_THREAD;
        }
        /* XXX: Removed since 0.9.22
        else if (strcasecmp(s_comm, PURC_RDRCOMM_NAME_WEBSOCKET) == 0) {
            extra_info.renderer_comm = PURC_RDRCOMM_WEBSOCKET;
        } */
        else {
            PC_WARN("Invalid '%s' comm.", s_comm);
            return;
        }
        extra_info.renderer_uri = s_uri;
//        purc_connect_to_renderer(&extra_info);
        purc_connect_to_renderer_async(&extra_info);
    }
    else if (strcmp(type, MSG_TYPE_NEW_RENDERER) == 0) {
        purc_variant_t data = msg->data;
        purc_variant_t comm = purc_variant_object_get_by_ckey_ex(data,
                KEY_COMM, true);
        if (!comm) {
            PC_WARN("Invalid '%s' event, '%s' not found.",
                    MSG_TYPE_NEW_RENDERER, KEY_COMM);
            return;
        }

        purc_variant_t uri = purc_variant_object_get_by_ckey_ex(data,
                KEY_URI, true);
        if (!uri) {
            PC_WARN("Invalid '%s' event, '%s' not found.",
                    MSG_TYPE_NEW_RENDERER, KEY_URI);
            return;
        }

        const char *s_comm = purc_variant_get_string_const(comm);
        const char *s_uri = purc_variant_get_string_const(uri);
        PC_TIMESTAMP("receive switch new renderer event url: %s app: %s runner: %s\n",
                s_uri, inst->app_name, inst->runner_name);
        pcrdr_switch_renderer(inst, s_comm, s_uri);
    }
    else if (strcmp(type, MSG_TYPE_RDR_STATE) == 0
            && sub_type && strcmp(type, MSG_TYPE_IDLE)) {
        broadcast_rdr_idle_event(inst, msg->data);
    } else {
        // TODO: other event
    }
}

void
pcintr_conn_event_handler(pcrdr_conn *conn, const pcrdr_msg *msg)
{
    UNUSED_PARAM(conn);
    UNUSED_PARAM(msg);
    struct pcinst *inst = pcinst_current();

    char *type = NULL;
    const char *sub_type = NULL;
    if (msg && msg->eventName) {
        const char *event = purc_variant_get_string_const(msg->eventName);
        const char *separator = strchr(event, MSG_EVENT_SEPARATOR);
        if (separator) {
            sub_type = separator + 1;
        }

        size_t nr_type = separator - event;
        if (nr_type) {
            type = strndup(event, separator - event);
            if (!type) {
                purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
                goto out;
            }
        }
    }

    if (msg->target == PCRDR_MSG_TARGET_COROUTINE) {
        dispatch_coroutine_event_from_move_buffer(inst, msg);
        goto out;
    }
    else if (msg->target == PCRDR_MSG_TARGET_INSTANCE) {
        dispatch_inst_event_from_move_buffer(inst, msg);
        goto out;
    }

    switch (msg->target) {
    case PCRDR_MSG_TARGET_SESSION:
        on_session_event(inst, conn, msg, type, sub_type);
        break;

    case PCRDR_MSG_TARGET_WORKSPACE:
        //TODO
//        purc_set_error(PURC_ERROR_NOT_SUPPORTED);
        break;

    case PCRDR_MSG_TARGET_PLAINWINDOW:
        on_plainwindow_event(inst, conn, msg, type, sub_type);
        break;

    case PCRDR_MSG_TARGET_WIDGET:
        on_widget_event(inst, conn, msg, type, sub_type);
        break;

    case PCRDR_MSG_TARGET_DOM:
        on_dom_event(inst,  conn, msg, type, sub_type);
        break;

    case PCRDR_MSG_TARGET_USER:
        //TODO
//        purc_set_error(PURC_ERROR_NOT_SUPPORTED);
        break;

    default:
//        purc_set_error(PURC_ERROR_NOT_SUPPORTED);
        break;
    }

out:
    if (type) {
        free(type);
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
        if (rid == 0) {
            /* clear PURC_ERROR_ENTITY_NOT_FOUND */
            purc_clr_error();
        }
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
    if ((inst->endpoint_atom == rid) &&
            (msg->target == PCRDR_MSG_TARGET_COROUTINE)) {
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

    int len;
    if (event_sub_type) {
        len = snprintf(p, n, "%s:%s", event_type, event_sub_type);
    }
    else {
        len = snprintf(p, n, "%s", event_type);
    }

    purc_variant_t event_name = purc_variant_make_string_reuse_buff(p,
            len + 1, true);
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

