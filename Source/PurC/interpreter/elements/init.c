/**
 * @file init.c
 * @author Xu Xiaohong
 * @date 2021/12/06
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
#include "purc-runloop.h"

#include "../ops.h"

#include <dlfcn.h>
#include <pthread.h>
#include <unistd.h>


#define INIT_ASYNC_EVENT_HANDLER        "__init_async_event_handler"

struct ctxt_for_init {
    struct pcvdom_node           *curr;

    purc_variant_t                as;
    purc_variant_t                at;
    purc_variant_t                from;
    purc_variant_t                with;
    purc_variant_t                against;

    purc_variant_t                literal;

    const char                   *from_uri;
    purc_variant_t                sync_id;
    pcintr_coroutine_t            co;

    int                           ret_code;
    int                           err;
    purc_rwstream_t               resp;

    enum VIA                      via;
    purc_variant_t                v_for;
    purc_variant_t                params;

    unsigned int                  under_head:1;
    unsigned int                  temporarily:1;
    unsigned int                  async:1;
    unsigned int                  casesensitively:1;
    unsigned int                  uniquely:1;
};

struct fetcher_for_init {
    pcintr_stack_t                stack;
    struct pcvdom_element         *element;
    purc_variant_t                name;
    unsigned int                  under_head:1;
    pthread_t                     current;
};

static void
ctxt_for_init_destroy(struct ctxt_for_init *ctxt)
{
    if (ctxt) {
        PURC_VARIANT_SAFE_CLEAR(ctxt->as);
        PURC_VARIANT_SAFE_CLEAR(ctxt->at);
        PURC_VARIANT_SAFE_CLEAR(ctxt->from);
        PURC_VARIANT_SAFE_CLEAR(ctxt->with);
        PURC_VARIANT_SAFE_CLEAR(ctxt->against);
        PURC_VARIANT_SAFE_CLEAR(ctxt->literal);
        PURC_VARIANT_SAFE_CLEAR(ctxt->sync_id);
        PURC_VARIANT_SAFE_CLEAR(ctxt->v_for);
        PURC_VARIANT_SAFE_CLEAR(ctxt->params);
        if (ctxt->resp) {
            purc_rwstream_destroy(ctxt->resp);
            ctxt->resp = NULL;
        }
        free(ctxt);
    }
}

static void
ctxt_destroy(void *ctxt)
{
    ctxt_for_init_destroy((struct ctxt_for_init*)ctxt);
}

static int
_bind_src(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        purc_variant_t as,
        purc_variant_t at,
        bool under_head,
        bool temporarily,
        purc_variant_t src)
{
    UNUSED_PARAM(under_head);
    int ret = 0;
    if (as) {
        const char *name = purc_variant_get_string_const(as);
        ret = pcintr_bind_named_variable(&co->stack,
            frame, name, at, temporarily, true, src);
    }
    else {
        pcintr_set_question_var(frame, src);
    }
    return ret;
}

static int
_init_set_with(purc_variant_t set, purc_variant_t arr)
{
    if (!purc_variant_is_array(arr)) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "array is required to initialize uniq-set");
        return -1;
    }

    purc_variant_t v;
    size_t idx;
    foreach_value_in_variant_array(arr, v, idx) {
        UNUSED_PARAM(idx);

        ssize_t r;
        r = purc_variant_set_add(set, v, PCVRNT_CR_METHOD_OVERWRITE);
        if (r == -1) {
            return -1;
        }
    }
    end_foreach;

    return 0;
}

static purc_variant_t
_generate_src(purc_variant_t against, bool uniquely, bool caseless,
        purc_variant_t val)
{
    if (uniquely) {
        const char *s_against = NULL;
        if (against != PURC_VARIANT_INVALID) {
            s_against = purc_variant_get_string_const(against);
        }
        purc_variant_t set;
        set = purc_variant_make_set_by_ckey_ex(0, s_against, caseless,
                PURC_VARIANT_INVALID);
        if (set == PURC_VARIANT_INVALID)
            return PURC_VARIANT_INVALID;

        if (_init_set_with(set, val)) {
            purc_variant_unref(set);
            return PURC_VARIANT_INVALID;
        }

        return set;
    }
    else {
        return purc_variant_ref(val);
    }
}

static int
post_process(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        purc_variant_t src)
{
    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;

    bool caseless = ctxt->casesensitively ? false : true;
    src = _generate_src(ctxt->against, ctxt->uniquely, caseless, src);
    if (src == PURC_VARIANT_INVALID)
        return -1;

    int r = _bind_src(co, frame,
            ctxt->as, ctxt->at,
            ctxt->under_head, ctxt->temporarily,
            src);
    purc_variant_unref(src);

    return r ? -1 : 0;
}

static int
process_attr_as(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;
    if (ctxt->as != PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_DUPLICATED,
                "vdom attribute '%s' for element <%s>",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    if (val == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> undefined",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    ctxt->as = val;
    purc_variant_ref(val);

    return 0;
}

static int
process_attr_at(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;
    if (ctxt->at != PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_DUPLICATED,
                "vdom attribute '%s' for element <%s>",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    if (val == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> undefined",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    ctxt->at = val;
    purc_variant_ref(val);

    return 0;
}

static int
process_attr_from(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;
    if (ctxt->from != PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_DUPLICATED,
                "vdom attribute '%s' for element <%s>",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    if (ctxt->with != PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_NOT_SUPPORTED,
                "vdom attribute '%s' for element <%s> conflicts with '%s'",
                purc_atom_to_string(name), element->tag_name,
                pchvml_keyword_str(PCHVML_KEYWORD_ENUM(HVML, FROM)));
        return -1;
    }
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
    ctxt->from = purc_variant_ref(val);
    ctxt->from_uri = purc_variant_get_string_const(ctxt->from);

    return 0;
}

static int
process_attr_for(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;
    if (ctxt->v_for != PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_DUPLICATED,
                "vdom attribute '%s' for element <%s>",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
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
    ctxt->v_for = purc_variant_ref(val);

    return 0;
}

static int
process_attr_with(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;
    if (ctxt->with != PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_DUPLICATED,
                "vdom attribute '%s' for element <%s>",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    if (val == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> undefined",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    ctxt->with = val;
    purc_variant_ref(val);

    return 0;
}

static int
process_attr_against(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;
    if (ctxt->against != PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_DUPLICATED,
                "vdom attribute '%s' for element <%s>",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
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
    ctxt->against = purc_variant_ref(val);

    return 0;
}

static int
process_attr_via(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;
    if (val == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> undefined",
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

    purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
            "unknown vdom attribute '%s = %s' for element <%s>",
            purc_atom_to_string(name), s_val, element->tag_name);
    return -1;
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

    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, AS)) == name) {
        return process_attr_as(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, AT)) == name) {
        return process_attr_at(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, UNIQUELY)) == name
            || pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, UNIQ)) == name) {
        ctxt->uniquely = 1;
        return 0;
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, CASESENSITIVELY)) == name
            || pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, CASE)) == name) {
        ctxt->casesensitively= 1;
        return 0;
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, CASEINSENSITIVELY)) == name
            || pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, CASELESS)) == name) {
        ctxt->casesensitively= 0;
        return 0;
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, FROM)) == name) {
        return process_attr_from(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, WITH)) == name) {
        return process_attr_with(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, AGAINST)) == name) {
        return process_attr_against(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, VIA)) == name) {
        return process_attr_via(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, FOR)) == name) {
        return process_attr_for(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, TEMPORARILY)) == name ||
            pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, TEMP)) == name) {
        ctxt->temporarily = 1;
        if (ctxt->async) {
            purc_log_warn("'asynchronously' is ignored because of 'temporarily'");
            ctxt->async = 0;
        }
        return 0;
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, ASYNCHRONOUSLY)) == name
            || pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, ASYNC)) == name) {
        ctxt->async = 1;
        if (ctxt->temporarily) {
            purc_log_warn("'asynchronously' is ignored because of 'temporarily'");
            ctxt->async = 0;
        }
        return 0;
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, SYNCHRONOUSLY)) == name
            || pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, SYNC)) == name) {
        ctxt->async = 0;
        return 0;
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, SILENTLY)) == name) {
        return 0;
    }

    /* ignore other attr */
    return 0;
}

static void on_sync_complete(purc_variant_t request_id, void *ud,
        const struct pcfetcher_resp_header *resp_header,
        purc_rwstream_t resp)
{
    UNUSED_PARAM(request_id);
    UNUSED_PARAM(ud);
    UNUSED_PARAM(resp_header);
    UNUSED_PARAM(resp);

    pcintr_stack_frame_t frame;
    frame = (pcintr_stack_frame_t)ud;
    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;

    PC_DEBUG("load_async|callback|ret_code=%d\n", resp_header->ret_code);
    PC_DEBUG("load_async|callback|mime_type=%s\n", resp_header->mime_type);
    PC_DEBUG("load_async|callback|sz_resp=%ld\n", resp_header->sz_resp);

    ctxt->ret_code = resp_header->ret_code;
    ctxt->resp = resp;

    if (ctxt->co->stack.exited) {
        return;
    }

    pcintr_coroutine_post_event(ctxt->co->cid,
        PCRDR_MSG_EVENT_REDUCE_OPT_KEEP,
        ctxt->sync_id, MSG_TYPE_FETCHER_STATE, MSG_SUB_TYPE_SUCCESS,
        PURC_VARIANT_INVALID, ctxt->sync_id);
}

static bool
is_observer_match(pcintr_coroutine_t co,
        struct pcintr_observer *observer, pcrdr_msg *msg,
        purc_variant_t observed, purc_atom_t type, const char *sub_type)
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

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(MSG, FETCHERSTATE)) == type) {
        match = true;
        goto out;
    }

out:
    return match;
}

static int
observer_handle(pcintr_coroutine_t cor, struct pcintr_observer *observer,
        pcrdr_msg *msg, purc_atom_t type, const char *sub_type, void *data)
{
    UNUSED_PARAM(cor);
    UNUSED_PARAM(observer);
    UNUSED_PARAM(msg);
    UNUSED_PARAM(type);
    UNUSED_PARAM(sub_type);
    UNUSED_PARAM(data);
    UNUSED_PARAM(msg);

    pcintr_set_current_co(cor);

    int r;
    struct pcintr_stack_frame *frame;
    frame = (struct pcintr_stack_frame*)data;

    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;

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

    purc_variant_t ret = purc_variant_load_from_json_stream(ctxt->resp);
    PRINT_VARIANT(ret);
    if (ret == PURC_VARIANT_INVALID) {
        frame->next_step = NEXT_STEP_ON_POPPING;
        goto out;
    }

    r = post_process(cor, frame, ret);
    PURC_VARIANT_SAFE_CLEAR(ret);
    if (r) {
        frame->next_step = NEXT_STEP_ON_POPPING;
    }

out:
    pcintr_resume(cor, msg);
    pcintr_set_current_co(NULL);
    return 0;
}

static purc_variant_t
params_from_with(struct ctxt_for_init *ctxt)
{
    purc_variant_t with = ctxt->with;

    purc_variant_t params;
    if (with == PURC_VARIANT_INVALID) {
        params = purc_variant_make_object_0();
    }
    else if (purc_variant_is_object(with)) {
        params = purc_variant_ref(with);
    }
    else {
        // TODO VW: raise exceptioin for no suitable value.
        params = purc_variant_make_object_0();
    }

    PURC_VARIANT_SAFE_CLEAR(ctxt->params);
    ctxt->params = params;

    return params;
}

static int
process_from_sync(pcintr_coroutine_t co, pcintr_stack_frame_t frame)
{
    pcintr_stack_t stack = &co->stack;

    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;

    enum pcfetcher_request_method method;
    method = pcintr_method_from_via(ctxt->via);

    purc_variant_t params;
    params = params_from_with(ctxt);

    ctxt->co = co;
    purc_variant_t v = pcintr_load_from_uri_async(stack, ctxt->from_uri,
            method, params, on_sync_complete, frame, PURC_VARIANT_INVALID);
    if (v == PURC_VARIANT_INVALID)
        return -1;

    ctxt->sync_id = purc_variant_ref(v);

    pcintr_yield(
            CO_STAGE_FIRST_RUN | CO_STAGE_OBSERVING,
            CO_STATE_STOPPED,
            ctxt->sync_id,
            MSG_TYPE_FETCHER_STATE,
            MSG_SUB_TYPE_ASTERISK,
            is_observer_match,
            observer_handle,
            frame,
            true
            );

    purc_clr_error();

    return 0;
}

struct load_data {
    pcintr_coroutine_t        co;
    struct pcvdom_element    *vdom_element;
    purc_variant_t            async_id;

    struct pcintr_cancel      cancel;

    int                       ret_code;
    int                       err;
    purc_rwstream_t           resp;

    purc_variant_t            as;
    purc_variant_t            at;
    purc_variant_t            against;
    unsigned int              under_head:1;
    unsigned int              temporarily:1;
    unsigned int              casesensitively:1;
    unsigned int              uniquely:1;
};

static void load_data_release(struct load_data *data)
{
    if (data) {
        PURC_VARIANT_SAFE_CLEAR(data->async_id);
        data->co = NULL;
        data->vdom_element = NULL;
        PURC_VARIANT_SAFE_CLEAR(data->as);
        PURC_VARIANT_SAFE_CLEAR(data->at);
        PURC_VARIANT_SAFE_CLEAR(data->against);
        if (data->resp) {
            purc_rwstream_destroy(data->resp);
            data->resp = NULL;
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

static void on_async_resume_on_frame_pseudo(pcintr_coroutine_t co,
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

    purc_variant_t ret = purc_variant_load_from_json_stream(data->resp);
    PRINT_VARIANT(ret);
    if (ret == PURC_VARIANT_INVALID)
        return;

    bool caseless = data->casesensitively ? false : true;
    purc_variant_t src;
    src = _generate_src(data->against, data->uniquely, caseless, ret);
    if (src != PURC_VARIANT_INVALID) {
        _bind_src(co, frame, // NULL
                data->as, data->at,
                data->under_head, data->temporarily,
                src);
        purc_variant_unref(src);
    }

    PURC_VARIANT_SAFE_CLEAR(ret);
}

static void on_async_resume(void *ud)
{
    struct load_data *data;
    data = (struct load_data*)ud;

    pcintr_coroutine_t co = pcintr_get_coroutine();

    pcintr_unregister_cancel(&data->cancel);

    pcintr_push_stack_frame_pseudo(data->vdom_element);
    on_async_resume_on_frame_pseudo(co, data);
    pcintr_pop_stack_frame_pseudo();

    load_data_destroy(data);
}

static bool
is_async_observer_match(pcintr_coroutine_t cor, struct pcintr_observer *observer,
        pcrdr_msg *msg, purc_variant_t observed, purc_atom_t type,
        const char *sub_type)
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

static int
async_observer_handle(pcintr_coroutine_t cor, struct pcintr_observer *observer,
        pcrdr_msg *msg, purc_atom_t type, const char *sub_type, void *data)
{
    UNUSED_PARAM(observer);
    UNUSED_PARAM(type);
    UNUSED_PARAM(sub_type);
    UNUSED_PARAM(data);

    pcintr_set_current_co(cor);
    struct load_data *payload = purc_variant_native_get_entity(msg->data);
    on_async_resume(payload);
    pcintr_set_current_co(NULL);
    return 0;
}


static void on_async_complete(purc_variant_t request_id, void *ud,
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

static void load_data_cancel(void *ud)
{
    struct load_data *data;
    data = (struct load_data*)ud;

    pcfetcher_cancel_async(data->async_id);
}

static int
process_from_async(pcintr_coroutine_t co, pcintr_stack_frame_t frame)
{
    pcintr_stack_t stack = &co->stack;

    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;

    struct load_data *data;
    data = (struct load_data*)calloc(1, sizeof(*data));
    if (!data) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }
    pcintr_cancel_init(&data->cancel, data, load_data_cancel);

    data->co              = co;
    data->vdom_element    = frame->pos;
    data->as              = purc_variant_ref(ctxt->as);
    data->under_head      = ctxt->under_head;
    data->temporarily     = ctxt->temporarily;
    data->casesensitively = ctxt->casesensitively;
    data->uniquely        = ctxt->uniquely;
    if (ctxt->at)
        data->at          = purc_variant_ref(ctxt->at);
    if (ctxt->against)
        data->against     = purc_variant_ref(ctxt->against);

    enum pcfetcher_request_method method;
    method = pcintr_method_from_via(ctxt->via);

    purc_variant_t params;
    params = params_from_with(ctxt);

    pcvarmgr_t mgr = pcintr_get_named_variable_mgr_by_at(stack, frame, ctxt->at,
        ctxt->temporarily, true);
    const char *name = purc_variant_get_string_const(ctxt->as);
    purc_variant_t dest = pcintr_get_named_var_for_event(stack, name, mgr);
    if (!dest) {
        return -1;
    }

    data->async_id = pcintr_load_from_uri_async(stack, ctxt->from_uri,
            method, params, on_async_complete, data, dest);
    purc_variant_unref(dest);

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
            is_async_observer_match,
            async_observer_handle,
            NULL,
            true
        );

    pcintr_register_cancel(&data->cancel);

    return 0;
}

static int
process_via(pcintr_coroutine_t co)
{
    pcintr_stack_t stack = &co->stack;

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;

    const char *s_from = NULL;
    const char *s_for = NULL;

    if (ctxt->from != PURC_VARIANT_INVALID &&
            purc_variant_is_string(ctxt->from))
    {
        s_from = purc_variant_get_string_const(ctxt->from);
    }

    if (ctxt->v_for != PURC_VARIANT_INVALID &&
            purc_variant_is_string(ctxt->v_for))
    {
        s_for = purc_variant_get_string_const(ctxt->v_for);
    }

    purc_variant_t v;
    if (0) {
        v = purc_variant_load_dvobj_from_so (s_from, s_for);
    }
    else {
        void *handle = NULL;
        if (s_from) {
            handle = pcintr_load_module(s_from,
                    PURC_ENVV_DVOBJS_PATH, "libpurc-dvobj-");
            if (!handle)
                return -1;
        }

        purc_variant_t (*load)(const char *, int *);

        load = dlsym(handle, EXOBJ_LOAD_ENTRY);
        if (dlerror() != NULL) {
            pcintr_unload_module(handle);
            pcinst_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
            return -1;
        }

        int ver_code;
        v = load(s_for, &ver_code);
        // TODO: check ver_code;

        pcintr_unload_module(handle);
    }

    if (v == PURC_VARIANT_INVALID) {
        // FIXME: who's responsible for error code
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "failed to load external variant");
        return -1;
    }

    int r;
    PRINT_VARIANT(v);
    r = _bind_src(co, frame,
            ctxt->as, ctxt->at,
            ctxt->under_head, ctxt->temporarily,
            v);
    purc_variant_unref(v);
    return r ? -1 : 0;
}

static int
process_from(pcintr_coroutine_t co)
{
    pcintr_stack_t stack = &co->stack;

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;

    if (ctxt->async) {
        return process_from_async(co, frame);
    }
    else {
        return process_from_sync(co, frame);
    }
}

static void*
after_pushed(pcintr_stack_t stack, pcvdom_element_t pos)
{
    if (stack->except)
        return NULL;

    pcintr_check_insertion_mode_for_normal_element(stack);

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    struct ctxt_for_init *ctxt = frame->ctxt;
    if (!ctxt) {
        ctxt = (struct ctxt_for_init*)calloc(1, sizeof(*ctxt));
        if (!ctxt) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            return NULL;
        }

        ctxt->casesensitively = 1;
        frame->ctxt = ctxt;
        frame->ctxt_destroy = ctxt_destroy;

        frame->pos = pos; // ATTENTION!!
    }

    if (0 != pcintr_stack_frame_eval_attr_and_content(stack, frame, false)) {
        return NULL;
    }

    frame->attr_vars = purc_variant_make_object(0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    if (frame->attr_vars == PURC_VARIANT_INVALID)
        return ctxt;

    struct pcvdom_element *element = frame->pos;

    int r;
    r = pcintr_walk_attrs(frame, element, stack, attr_found_val);
    if (r)
        return ctxt;

    if (ctxt->temporarily) {
        ctxt->async = 0;
    }

    while ((element=pcvdom_element_parent(element))) {
        if (element->tag_id == PCHVML_TAG_HEAD) {
            ctxt->under_head = 1;
        }
    }

    purc_clr_error(); // pcvdom_element_parent

    if (ctxt->as == PURC_VARIANT_INVALID) {
        ctxt->async = 0;
    }

    if (ctxt->via == VIA_LOAD) {
        r = process_via(stack->co);
        return ctxt;
    }

    if (ctxt->from_uri) {
        r = process_from(stack->co);
        return ctxt;
    }


    if (ctxt->with) {
        r = pcintr_set_question_var(frame, ctxt->with);
        if (r == 0)
            post_process(stack->co, frame, ctxt->with);
        return ctxt;
    }

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

    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;
    if (ctxt) {
        ctxt_for_init_destroy(ctxt);
        frame->ctxt = NULL;
    }

    return true;
}

static int
on_element(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_element *element)
{
    UNUSED_PARAM(element);

    pcintr_stack_t stack = &co->stack;
    if (stack->except)
        return 0;

    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;

    if (ctxt->with && !ctxt->from)
        return 0;

    if (ctxt->from || ctxt->with) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "no element is permitted "
                "since `from/with` attribute already set");
        return -1;
    }

    return 0;
}

static int
on_content(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_content *content)
{

    pcintr_stack_t stack = &co->stack;
    if (stack->except)
        return 0;

    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;

    if (ctxt->from) {
        if (!ctxt->async)
            return 0;
    }
    else if (ctxt->with) {
        return 0;
    }

    struct pcvcm_node *vcm = content->vcm;
    if (!vcm)
        return 0;

    // FIXME
    if ((ctxt->from && !ctxt->async) || ctxt->with) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "no content is permitted "
                "since there's no `from/with` attribute");
        return -1;
    }

    purc_variant_t v = pcintr_get_symbol_var(frame, PURC_SYMBOL_VAR_CARET);
    if (!v || purc_variant_is_undefined(v)) {
        return -1;
    }

    PURC_VARIANT_SAFE_CLEAR(ctxt->literal);
    ctxt->literal = purc_variant_ref(v);

    PURC_VARIANT_SAFE_CLEAR(frame->ctnt_var);
    frame->ctnt_var = purc_variant_ref(ctxt->literal);

    return post_process(co, frame, frame->ctnt_var);
}

static int
on_comment(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_comment *comment)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);
    UNUSED_PARAM(comment);
    return 0;
}

static int
on_child_finished(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    pcintr_stack_t stack = &co->stack;
    if (stack->except)
        return 0;

    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;

    if (ctxt->from && ctxt->async && ctxt->literal == PURC_VARIANT_INVALID) {
        purc_variant_t v = purc_variant_make_null();
        int r = post_process(co, frame, v);
        purc_variant_unref(v);
        return r;
    }

    return 0;
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

    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;

    struct pcvdom_node *curr;

    if (ctxt->via == VIA_LOAD)
        return NULL;

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
        on_child_finished(co, frame);
        return NULL;
    }

    switch (curr->type) {
        case PCVDOM_NODE_DOCUMENT:
            purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
            break;
        case PCVDOM_NODE_ELEMENT:
            {
                pcvdom_element_t element = PCVDOM_ELEMENT_FROM_NODE(curr);
                if (on_element(co, frame, element))
                    return NULL;
                return element;
            }
        case PCVDOM_NODE_CONTENT:
            if (on_content(co, frame, PCVDOM_CONTENT_FROM_NODE(curr)))
                return NULL;
            goto again;
        case PCVDOM_NODE_COMMENT:
            if (on_comment(co, frame, PCVDOM_COMMENT_FROM_NODE(curr)))
                return NULL;
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

struct pcintr_element_ops* pcintr_get_init_ops(void)
{
    return &ops;
}

