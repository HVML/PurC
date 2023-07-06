/**
 * @file load.c
 * @author Xu Xiaohong
 * @date 2022/06/26
 * @brief
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

#include "purc.h"

#include "../internal.h"

#include "private/debug.h"
#include "private/instance.h"
#include "purc-runloop.h"

#include "../ops.h"

#include <pthread.h>
#include <unistd.h>

#define EVENT_SEPARATOR          ':'
#define LOAD_EVENT_HANDER  "_load_event_handler"

struct ctxt_for_load {
    struct pcvdom_node           *curr;

    purc_variant_t                on;
    purc_variant_t                from;
    purc_variant_t                with;

    enum VIA                      via;
    const char                   *from_uri;
    purc_variant_t                sync_id;
    pcintr_coroutine_t            co;

    int                           ret_code;
    int                           err;
    purc_rwstream_t               resp;
    char                         *mime_type;

    purc_variant_t                within;
    purc_variant_t                as;
    const char                   *s_as;

    purc_variant_t                at;
    const char                   *s_at;

    purc_variant_t                onto;

    char               endpoint_name_within[PURC_LEN_ENDPOINT_NAME + 1];
    purc_atom_t        endpoint_atom_within;

    unsigned int                  synchronously:1;
    purc_variant_t                request_id;
};

static void
ctxt_for_load_destroy(struct ctxt_for_load *ctxt)
{
    if (ctxt) {
        PURC_VARIANT_SAFE_CLEAR(ctxt->on);
        PURC_VARIANT_SAFE_CLEAR(ctxt->from);
        PURC_VARIANT_SAFE_CLEAR(ctxt->with);
        PURC_VARIANT_SAFE_CLEAR(ctxt->sync_id);
        PURC_VARIANT_SAFE_CLEAR(ctxt->within);
        PURC_VARIANT_SAFE_CLEAR(ctxt->as);
        PURC_VARIANT_SAFE_CLEAR(ctxt->at);
        PURC_VARIANT_SAFE_CLEAR(ctxt->onto);
        PURC_VARIANT_SAFE_CLEAR(ctxt->request_id);
        if (ctxt->resp) {
            purc_rwstream_destroy(ctxt->resp);
            ctxt->resp = NULL;
        }
        if (ctxt->mime_type) {
            free(ctxt->mime_type);
            ctxt->mime_type = NULL;
        }
        if (ctxt->endpoint_atom_within) {
            purc_atom_remove_string_ex(PURC_ATOM_BUCKET_DEF,
                    ctxt->endpoint_name_within);
            ctxt->endpoint_atom_within = 0;
        }
        free(ctxt);
    }
}

static void
ctxt_destroy(void *ctxt)
{
    ctxt_for_load_destroy((struct ctxt_for_load*)ctxt);
}

#if 0
struct load_data {
    pcintr_coroutine_t        co;
    struct pcvdom_element    *vdom_element;
    purc_variant_t            async_id;
    purc_variant_t            within;
    purc_variant_t            with;
    purc_variant_t            onto;
    purc_variant_t            as;
    purc_variant_t            at;

    struct pcintr_cancel      cancel;

    int                       ret_code;
    int                       err;
    purc_rwstream_t           resp;
    char                     *mime_type;

};

static void load_data_release(struct load_data *data)
{
    if (data) {
        PURC_VARIANT_SAFE_CLEAR(data->async_id);
        PURC_VARIANT_SAFE_CLEAR(data->within);
        PURC_VARIANT_SAFE_CLEAR(data->with);
        PURC_VARIANT_SAFE_CLEAR(data->onto);
        PURC_VARIANT_SAFE_CLEAR(data->as);
        PURC_VARIANT_SAFE_CLEAR(data->at);
        data->co = NULL;
        data->vdom_element = NULL;
        if (data->resp) {
            purc_rwstream_destroy(data->resp);
            data->resp = NULL;
        }
        if (data->mime_type) {
            free(data->mime_type);
            data->mime_type = NULL;
        }
    }
}

static void load_data_destroy(struct load_data *data)
{
    if (data) {
        load_data_release(data);
        free(data);
    }
}
#endif

static bool
is_observer_match(pcintr_coroutine_t co,
        struct pcintr_observer *observer, pcrdr_msg *msg,
        purc_variant_t observed, const char *type, const char *sub_type)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(observer);
    UNUSED_PARAM(msg);
    UNUSED_PARAM(observed);
    UNUSED_PARAM(type);
    UNUSED_PARAM(sub_type);
    bool match = false;
    if (purc_variant_is_equal_to(observer->observed, msg->elementValue) ||
            pcintr_crtn_observed_is_match(observer->observed, msg->elementValue)) {
        goto match_observed;
    }
    else {
        goto out;
    }


match_observed:
    if (type && strcmp(type, MSG_TYPE_CALL_STATE) == 0) {
        match = true;
        goto out;
    }

out:
    return match;
}

static int
observer_handle(pcintr_coroutine_t cor, struct pcintr_observer *observer,
        pcrdr_msg *msg, const char *type, const char *sub_type, void *data)
{
    UNUSED_PARAM(cor);
    UNUSED_PARAM(observer);
    UNUSED_PARAM(msg);
    UNUSED_PARAM(type);
    UNUSED_PARAM(sub_type);
    UNUSED_PARAM(data);
    UNUSED_PARAM(msg);

    pcintr_set_current_co(cor);

    pcintr_stack_frame_t frame = (pcintr_stack_frame_t)data;

    if (0 == strcmp(sub_type, MSG_SUB_TYPE_SUCCESS)) {
        purc_variant_t payload = msg->data;

        pcintr_set_question_var(frame, payload);
    }
    else if (0 == strcmp(sub_type, MSG_SUB_TYPE_EXCEPT)) {
        purc_variant_t payload = msg->data;

        const char *s = purc_variant_get_string_const(payload);
        purc_set_error_with_info(PURC_ERROR_UNKNOWN,
                "sub coroutine failed with except: %s", s);
    }

    pcintr_resume(cor, msg);
    pcintr_set_current_co(NULL);
    return 0;
}

static int
post_process(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        purc_vdom_t vdom, const char *body_id)
{
    UNUSED_PARAM(co);

    struct ctxt_for_load *ctxt;
    ctxt = (struct ctxt_for_load*)frame->ctxt;

    const char *runner_name = ctxt->within ?
        purc_variant_get_string_const(ctxt->within) : NULL;
    const char *as = ctxt->as ? purc_variant_get_string_const(ctxt->as) : NULL;
    const char *onto = ctxt->onto ?
        purc_variant_get_string_const(ctxt->onto) : NULL;
    purc_atom_t child_cid = pcintr_schedule_child_co(vdom, co->cid,
            runner_name, onto, ctxt->with, body_id, false);

    if (!child_cid)
        return -1;

    ctxt->request_id = pcintr_crtn_observed_create(child_cid);

    if (as) {
        pcintr_bind_named_variable(&co->stack, frame, as, ctxt->at, false,
                false, ctxt->request_id);
    }

    if (ctxt->synchronously) {
        pcintr_yield(
                CO_STAGE_FIRST_RUN | CO_STAGE_OBSERVING,
                CO_STATE_STOPPED,
                ctxt->request_id,
                MSG_TYPE_CALL_STATE,
                MSG_SUB_TYPE_ASTERISK,
                is_observer_match,
                observer_handle,
                frame,
                true
                );
        return 0;
    }

    // ASYNC nothing to do
    return 0;
}


static void on_fetch_sync_complete(purc_variant_t request_id, void *ud,
        const struct pcfetcher_resp_header *resp_header,
        purc_rwstream_t resp)
{
    UNUSED_PARAM(request_id);
    UNUSED_PARAM(ud);
    UNUSED_PARAM(resp_header);
    UNUSED_PARAM(resp);

    pcintr_stack_frame_t frame;
    frame = (pcintr_stack_frame_t)ud;
    struct ctxt_for_load *ctxt;
    ctxt = (struct ctxt_for_load *)frame->ctxt;

    PC_DEBUG("load_async|callback|ret_code=%d\n", resp_header->ret_code);
    PC_DEBUG("load_async|callback|mime_type=%s\n", resp_header->mime_type);
    PC_DEBUG("load_async|callback|sz_resp=%ld\n", resp_header->sz_resp);

    ctxt->ret_code = resp_header->ret_code;
    ctxt->resp = resp;
    if (resp_header->mime_type) {
        ctxt->mime_type = strdup(resp_header->mime_type);
    }

    if (ctxt->co->stack.exited) {
        return;
    }

    pcintr_coroutine_post_event(ctxt->co->cid,
        PCRDR_MSG_EVENT_REDUCE_OPT_KEEP,
        ctxt->sync_id, MSG_TYPE_FETCHER_STATE, MSG_SUB_TYPE_SUCCESS,
        PURC_VARIANT_INVALID, ctxt->sync_id);
}

static bool
is_fetch_observer_match(pcintr_coroutine_t co,
        struct pcintr_observer *observer, pcrdr_msg *msg,
        purc_variant_t observed, const char *type, const char *sub_type)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(observer);
    UNUSED_PARAM(msg);
    UNUSED_PARAM(observed);
    UNUSED_PARAM(type);
    UNUSED_PARAM(sub_type);
    bool match = false;
    if (!purc_variant_is_equal_to(observer->observed, msg->elementValue)) {
        goto out;
    }

    if (type && strcmp(type, MSG_TYPE_FETCHER_STATE) == 0) {
        match = true;
        goto out;
    }

out:
    return match;
}

static purc_vdom_t
load_vdom(purc_rwstream_t rws)
{
    return purc_load_hvml_from_rwstream(rws);
}

static int
fetch_observer_handle(pcintr_coroutine_t cor, struct pcintr_observer *observer,
        pcrdr_msg *msg, const char *type, const char *sub_type, void *data)
{
    UNUSED_PARAM(cor);
    UNUSED_PARAM(observer);
    UNUSED_PARAM(msg);
    UNUSED_PARAM(type);
    UNUSED_PARAM(sub_type);
    UNUSED_PARAM(data);
    UNUSED_PARAM(msg);

    pcintr_set_current_co(cor);

    struct pcintr_stack_frame *frame;
    frame = (struct pcintr_stack_frame*)data;

    struct ctxt_for_load *ctxt;
    ctxt = (struct ctxt_for_load *)frame->ctxt;

    if (ctxt->ret_code == RESP_CODE_USER_STOP) {
        frame->next_step = NEXT_STEP_ON_POPPING;
        goto out;
    }

    if (!ctxt->resp || ctxt->ret_code != 200) {
        if (frame->silently) {
            frame->next_step = NEXT_STEP_ON_POPPING;
            goto out;
        }
        frame->next_step = NEXT_STEP_ON_POPPING;
        // FIXME: what error to set
        purc_set_error_with_info(PURC_ERROR_REQUEST_FAILED, "%d",
                ctxt->ret_code);
        goto out;
    }

    purc_vdom_t vdom = load_vdom(ctxt->resp);
    if (!vdom) {
        frame->next_step = NEXT_STEP_ON_POPPING;
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE ,
                "load vdom from on/from failed");
        goto out;
    }

    int r = post_process(cor, frame, vdom, NULL);
    if (r) {
        frame->next_step = NEXT_STEP_ON_POPPING;
        goto out;
    }

    /* do not resume becase 'post_process'  will yield again */
    if (ctxt->synchronously) {
        pcintr_set_current_co(NULL);
        return 0;
    }

out:
    pcintr_resume(cor, msg);
    pcintr_set_current_co(NULL);
    return 0;
}

static int
process_from_sync(pcintr_coroutine_t co, pcintr_stack_frame_t frame)
{
    pcintr_stack_t stack = &co->stack;

    struct ctxt_for_load *ctxt;
    ctxt = (struct ctxt_for_load*)frame->ctxt;

    enum pcfetcher_request_method method;
    method = pcintr_method_from_via(ctxt->via);

    purc_variant_t params = PURC_VARIANT_INVALID;

    ctxt->co = co;
    purc_variant_t v = pcintr_load_from_uri_async(stack, ctxt->from_uri,
            method, params, on_fetch_sync_complete, frame, PURC_VARIANT_INVALID);
    if (v == PURC_VARIANT_INVALID)
        return -1;

    ctxt->sync_id = purc_variant_ref(v);

    pcintr_yield(
            CO_STAGE_FIRST_RUN | CO_STAGE_OBSERVING,
            CO_STATE_STOPPED,
            ctxt->sync_id,
            MSG_TYPE_FETCHER_STATE,
            MSG_SUB_TYPE_ASTERISK,
            is_fetch_observer_match,
            fetch_observer_handle,
            frame,
            true
            );

    purc_clr_error();

    return 0;
}

#if 0
static void load_data_cancel(void *ud)
{
    struct load_data *data;
    data = (struct load_data*)ud;

    pcfetcher_cancel_async(data->async_id);
}

static void on_fetch_async_complete(purc_variant_t request_id, void *ud,
        const struct pcfetcher_resp_header *resp_header,
        purc_rwstream_t resp)
{
    UNUSED_PARAM(request_id);

    PC_DEBUG("load_async|callback|ret_code=%d\n", resp_header->ret_code);
    PC_DEBUG("load_async|callback|mime_type=%s\n", resp_header->mime_type);
    PC_DEBUG("load_async|callback|sz_resp=%ld\n", resp_header->sz_resp);

    struct load_data *data;
    data = (struct load_data*)ud;

    pcintr_coroutine_t co = data->co;

    data->ret_code = resp_header->ret_code;
    data->resp = resp;
    if (resp_header->mime_type) {
        data->mime_type = strdup(resp_header->mime_type);
    }

    if (co->stack.exited) {
        return;
    }

    purc_variant_t payload = purc_variant_make_native(data, NULL);
    pcintr_coroutine_post_event(co->cid,
        PCRDR_MSG_EVENT_REDUCE_OPT_KEEP,
        data->async_id,
        MSG_TYPE_FETCHER_STATE, MSG_SUB_TYPE_SUCCESS,
        payload, data->async_id);
    purc_variant_unref(payload);
}

static bool
is_fetch_async_observer_match(pcintr_coroutine_t cor,
        struct pcintr_observer *observer, pcrdr_msg *msg,
        purc_variant_t observed, purc_atom_t type, const char *sub_type)
{
    UNUSED_PARAM(cor);
    UNUSED_PARAM(observer);
    UNUSED_PARAM(msg);
    UNUSED_PARAM(observed);
    UNUSED_PARAM(type);
    UNUSED_PARAM(sub_type);
    bool match = false;
    if (!purc_variant_is_equal_to(observer->observed, msg->elementValue)) {
        goto out;
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(MSG, FETCHERSTATE)) == type) {
        match = true;
        goto out;
    }

out:
    return match;
}

static void on_fetch_async_resume_on_frame_pseudo(pcintr_coroutine_t co,
        struct load_data *data)
{
    pcintr_stack_t stack = &co->stack;
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    if (data->ret_code == RESP_CODE_USER_STOP)
        return;

    if (!data->resp || data->ret_code != 200) {
        if (frame->silently) {
            return;
        }

        // FIXME: what error to set?
        purc_set_error_with_info(PURC_ERROR_REQUEST_FAILED, "%d",
                data->ret_code);
        return;
    }

    purc_vdom_t vdom = load_vdom(data->resp);
    if (!vdom) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE ,
                "load vdom from on/from failed");
    }
    else {
        const char *runner_name = data->within ?
            purc_variant_get_string_const(data->within) : NULL;
        const char *as = data->as ? purc_variant_get_string_const(data->as) : NULL;
        const char *onto = data->onto ?
            purc_variant_get_string_const(data->onto) : NULL;
        purc_atom_t child_cid = pcintr_schedule_child_co(vdom, co->cid,
                runner_name, onto, data->with, NULL, false);

        if (child_cid && as) {
            purc_variant_t request_id = pcintr_crtn_observed_create(child_cid);
            pcintr_bind_named_variable(&co->stack, frame, as, data->at, false,
                    false, request_id);
            purc_variant_unref(request_id);
        }
    }

}

static void on_fetch_async_resume(void *ud)
{
    struct load_data *data;
    data = (struct load_data*)ud;

    pcintr_coroutine_t co = pcintr_get_coroutine();

    pcintr_unregister_cancel(&data->cancel);

    pcintr_push_stack_frame_pseudo(data->vdom_element);
    on_fetch_async_resume_on_frame_pseudo(co, data);
    pcintr_pop_stack_frame_pseudo();

    load_data_destroy(data);
}


static int
fetch_async_observer_handle(pcintr_coroutine_t cor,
        struct pcintr_observer *observer, pcrdr_msg *msg,
        purc_atom_t type, const char *sub_type, void *data)
{
    UNUSED_PARAM(observer);
    UNUSED_PARAM(type);
    UNUSED_PARAM(sub_type);
    UNUSED_PARAM(data);

    pcintr_set_current_co(cor);
    struct load_data *payload = purc_variant_native_get_entity(msg->data);
    on_fetch_async_resume(payload);
    pcintr_set_current_co(NULL);
    return 0;
}

static int
process_from_async(pcintr_coroutine_t co, pcintr_stack_frame_t frame)
{
    pcintr_stack_t stack = &co->stack;

    struct ctxt_for_load *ctxt;
    ctxt = (struct ctxt_for_load*)frame->ctxt;

    struct load_data *data;
    data = (struct load_data *)calloc(1, sizeof(*data));
    if (!data) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }
    pcintr_cancel_init(&data->cancel, data, load_data_cancel);

    data->co              = co;
    data->vdom_element    = frame->pos;
    if (ctxt->within) {
        data->within = purc_variant_ref(ctxt->within);
    }
    if (ctxt->with) {
        data->with = purc_variant_ref(ctxt->with);
    }
    if (ctxt->onto) {
        data->onto = purc_variant_ref(ctxt->onto);
    }
    if (ctxt->as) {
        data->as = purc_variant_ref(ctxt->as);
    }
    if (ctxt->at) {
        data->at = purc_variant_ref(ctxt->at);
    }

    enum pcfetcher_request_method method;
    method = pcintr_method_from_via(ctxt->via);

    purc_variant_t params = PURC_VARIANT_INVALID;

    data->async_id = pcintr_load_from_uri_async(stack, ctxt->from_uri,
            method, params, on_fetch_async_complete, data, PURC_VARIANT_INVALID);

    if (data->async_id == PURC_VARIANT_INVALID) {
        load_data_destroy(data);
        return -1;
    }

    data->async_id = purc_variant_ref(data->async_id);

    ctxt->sync_id = purc_variant_ref(data->async_id);

    pcintr_register_inner_observer(
            stack,
            CO_STAGE_FIRST_RUN | CO_STAGE_OBSERVING,
            CO_STATE_READY | CO_STATE_OBSERVING,
            data->async_id,
            MSG_TYPE_FETCHER_STATE,
            MSG_SUB_TYPE_SUCCESS,
            is_fetch_async_observer_match,
            fetch_async_observer_handle,
            NULL,
            true
        );

    pcintr_register_cancel(&data->cancel);

    return 0;
}
#endif


static int
process_attr_on(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_load *ctxt;
    ctxt = (struct ctxt_for_load*)frame->ctxt;
    if (val == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> undefined",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    PURC_VARIANT_SAFE_CLEAR(ctxt->on);
    ctxt->on = purc_variant_ref(val);

    return 0;
}

static int
process_attr_from(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_load *ctxt;
    ctxt = (struct ctxt_for_load*)frame->ctxt;
    if (val == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> undefined",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    PURC_VARIANT_SAFE_CLEAR(ctxt->from);
    ctxt->from = purc_variant_ref(val);

    return 0;
}

static int
process_attr_with(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_load *ctxt;
    ctxt = (struct ctxt_for_load*)frame->ctxt;
    if (val == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> undefined",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    PURC_VARIANT_SAFE_CLEAR(ctxt->with);
    ctxt->with = purc_variant_ref(val);

    return 0;
}

static int
process_attr_within(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_load *ctxt;
    ctxt = (struct ctxt_for_load*)frame->ctxt;
    if (val == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> undefined",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    if (!purc_variant_is_string(val)) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> is not string",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }

    char app_name[PURC_LEN_APP_NAME + 1];
    char runner_name[PURC_LEN_RUNNER_NAME + 1];

    const char *s = purc_variant_get_string_const(val);

    int r;
    r = purc_extract_app_name(s, app_name) &&
        purc_extract_runner_name(s, runner_name);

    if (r == 0) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> is not valid",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }

    PURC_VARIANT_SAFE_CLEAR(ctxt->within);
    ctxt->within = purc_variant_ref(val);

    return 0;
}

static int
process_attr_via(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_load *ctxt;
    ctxt = (struct ctxt_for_load*)frame->ctxt;
    if (val == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> undefined",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    if (!purc_variant_is_string(val)) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> is not string",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }

    const char *s_val = purc_variant_get_string_const(val);
    if (!s_val)
        return -1;

    if (strcmp(s_val, "LOAD") == 0) {
        ctxt->via = VIA_LOAD;
        return 0;
    }

    if (strcmp(s_val, "GET") == 0) {
        ctxt->via = VIA_GET;
        return 0;
    }

    if (strcmp(s_val, "POST") == 0) {
        ctxt->via = VIA_POST;
        return 0;
    }

    if (strcmp(s_val, "DELETE") == 0) {
        ctxt->via = VIA_DELETE;
        return 0;
    }

    return 0;
}

static int
process_attr_as(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_load *ctxt;
    ctxt = (struct ctxt_for_load*)frame->ctxt;
    if (val == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> undefined",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    if (!purc_variant_is_string(val)) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> is not string",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }

    PURC_VARIANT_SAFE_CLEAR(ctxt->as);
    ctxt->as = purc_variant_ref(val);
    ctxt->s_as = purc_variant_get_string_const(ctxt->as);

    return 0;
}

static int
process_attr_at(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_load *ctxt;
    ctxt = (struct ctxt_for_load*)frame->ctxt;
    if (val == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> undefined",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    if (!purc_variant_is_string(val)) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> is not string",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }

    PURC_VARIANT_SAFE_CLEAR(ctxt->at);
    ctxt->at = purc_variant_ref(val);
    ctxt->s_at = purc_variant_get_string_const(ctxt->at);

    return 0;
}

static int
process_attr_onto(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_load *ctxt;
    ctxt = (struct ctxt_for_load*)frame->ctxt;
    if (val == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> undefined",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    if (!purc_variant_is_string(val)) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> is not string",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }

    PURC_VARIANT_SAFE_CLEAR(ctxt->onto);
    ctxt->onto = purc_variant_ref(val);

    return 0;
}

static int
attr_found_val(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val,
        struct pcvdom_attr *attr,
        void *ud)
{
    UNUSED_PARAM(attr);
    UNUSED_PARAM(ud);

    struct ctxt_for_load *ctxt;
    ctxt = (struct ctxt_for_load*)frame->ctxt;

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, ON)) == name) {
        return process_attr_on(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, FROM)) == name) {
        return process_attr_from(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, WITH)) == name) {
        return process_attr_with(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, WITHIN)) == name) {
        return process_attr_within(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, VIA)) == name) {
        return process_attr_via(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, AS)) == name) {
        return process_attr_as(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, AT)) == name) {
        return process_attr_at(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, ONTO)) == name) {
        return process_attr_onto(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, SYNCHRONOUSLY)) == name) {
        ctxt->synchronously = 1;
        return 0;
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, SYNC)) == name) {
        ctxt->synchronously = 1;
        return 0;
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, ASYNCHRONOUSLY)) == name) {
        ctxt->synchronously = 0;
        return 0;
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, ASYNC)) == name) {
        ctxt->synchronously = 0;
        return 0;
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, SILENTLY)) == name) {
        return 0;
    }

    /* ignore other attr */
    return 0;
}

static void*
after_pushed(pcintr_stack_t stack, pcvdom_element_t pos)
{
    if (stack->except)
        return NULL;

    pcintr_check_insertion_mode_for_normal_element(stack);

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    struct ctxt_for_load *ctxt = frame->ctxt;
    if (!ctxt) {
        ctxt = (struct ctxt_for_load*)calloc(1, sizeof(*ctxt));
        if (!ctxt) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            return NULL;
        }

        ctxt->synchronously = 1;

        frame->ctxt = ctxt;
        frame->ctxt_destroy = ctxt_destroy;

        frame->pos = pos; // ATTENTION!!
    }

    if (0 != pcintr_stack_frame_eval_attr_and_content(stack, frame, false)) {
        return NULL;
    }

    struct pcvdom_element *element = frame->pos;

    int r;
    r = pcintr_walk_attrs(frame, element, stack, attr_found_val);
    if (r)
        return ctxt;

    if (!ctxt->with) {
        purc_variant_t caret = pcintr_get_symbol_var(frame,
                PURC_SYMBOL_VAR_CARET);
        if (caret && !purc_variant_is_undefined(caret)) {
            ctxt->with = caret;
            purc_variant_ref(ctxt->with);
        }
    }

    purc_vdom_t vdom = NULL;
    const char *body_id = NULL;

    if (ctxt->on && purc_variant_is_string(ctxt->on)) {
        const char *hvml = purc_variant_get_string_const(ctxt->on);
        vdom = purc_load_hvml_from_string(hvml);
    }

    if (vdom) {
        goto process;
    }

    if (!vdom && ctxt->from && purc_variant_is_string(ctxt->from)) {
        const char *from = purc_variant_get_string_const(ctxt->from);
        if (from[0] == 0) {
            vdom = stack->co->stack.vdom;
            goto process;
        }
        else if (from[0] == '#') {
            vdom = stack->co->stack.vdom;
            body_id = from + 1;
            goto process;
        }
        else {
            ctxt->from_uri = from;
#if 0
            if (ctxt->synchronously) {
                process_from_sync(stack->co, frame);
            }
            else {
                process_from_async(stack->co, frame);
            }
#else
            process_from_sync(stack->co, frame);
#endif
            return ctxt;
        }
    }

    if (!vdom) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE ,
                "load vdom from on/from failed");
        return ctxt;
    }

process:
    r = post_process(stack->co, frame, vdom, body_id);
    if (r)
        return ctxt;

    return ctxt;
}

static bool
on_popping(pcintr_stack_t stack, void* ud)
{
    UNUSED_PARAM(ud);

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    if (frame->ctxt == NULL)
        return true;

    struct ctxt_for_load *ctxt;
    ctxt = (struct ctxt_for_load*)frame->ctxt;
    if (ctxt) {
        ctxt_for_load_destroy(ctxt);
        frame->ctxt = NULL;
    }

    return true;
}

static void
on_element(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_element *element)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);
    UNUSED_PARAM(element);
}

static void
on_content(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_content *content)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);
    UNUSED_PARAM(content);
}

static void
on_comment(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_comment *comment)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);
    UNUSED_PARAM(comment);
}

static pcvdom_element_t
select_child(pcintr_stack_t stack, void* ud)
{
    UNUSED_PARAM(ud);

    pcintr_coroutine_t co = stack->co;
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    if (stack->back_anchor == frame)
        stack->back_anchor = NULL;

    if (frame->ctxt == NULL)
        return NULL;

    if (stack->back_anchor)
        return NULL;

    struct ctxt_for_load *ctxt;
    ctxt = (struct ctxt_for_load*)frame->ctxt;

    struct pcvdom_node *curr;

again:
    curr = ctxt->curr;

    if (curr == NULL) {
        struct pcvdom_element *element = frame->pos;
        struct pcvdom_node *node = &element->node;
        node = pcvdom_node_first_child(node);
        curr = node;
    }
    else {
        curr = pcvdom_node_next_sibling(curr);
    }

    ctxt->curr = curr;

    if (curr == NULL) {
        purc_clr_error();
        return NULL;
    }

    switch (curr->type) {
        case PCVDOM_NODE_DOCUMENT:
            purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
            break;
        case PCVDOM_NODE_ELEMENT:
            {
                pcvdom_element_t element = PCVDOM_ELEMENT_FROM_NODE(curr);
                on_element(co, frame, element);
                return element;
            }
        case PCVDOM_NODE_CONTENT:
            on_content(co, frame, PCVDOM_CONTENT_FROM_NODE(curr));
            goto again;
        case PCVDOM_NODE_COMMENT:
            on_comment(co, frame, PCVDOM_COMMENT_FROM_NODE(curr));
            goto again;
        default:
            purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
    }

    purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    return NULL; // NOTE: never reached here!!!
}

static struct pcintr_element_ops
ops = {
    .after_pushed       = after_pushed,
    .on_popping         = on_popping,
    .rerun              = NULL,
    .select_child       = select_child,
};

struct pcintr_element_ops* pcintr_get_load_ops(void)
{
    return &ops;
}


