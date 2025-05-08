/**
 * @file update.c
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
#include "private/dvobjs.h"
#include "private/instance.h"
#include "pcrdr/connect.h"
#include "purc-runloop.h"
#include "private/stringbuilder.h"
#include "private/atom-buckets.h"

#include "html/interfaces/document.h"

#include "../ops.h"

#include <pthread.h>
#include <unistd.h>

#define OP_STR_UNKNOWN              "unknown"

#define AT_KEY_CONTENT              "content"
#define AT_KEY_TEXT_CONTENT         "textContent"
#define AT_KEY_ATTR                 "attr."

#define KEYWORD_ADD                 "add"
#define KEYWORD_UPPERCASE_ADD       "ADD"

enum update_action {
    UPDATE_ACTION_DISPLACE,
    UPDATE_ACTION_APPEND,
    UPDATE_ACTION_PREPEND,
    UPDATE_ACTION_MERGE,
    UPDATE_ACTION_REMOVE,
    UPDATE_ACTION_INSERTBEFORE,
    UPDATE_ACTION_INSERTAFTER,
    UPDATE_ACTION_ADD,
    UPDATE_ACTION_UNITE,
    UPDATE_ACTION_INTERSECT,
    UPDATE_ACTION_SUBTRACT,
    UPDATE_ACTION_XOR,
    UPDATE_ACTION_OVERWRITE,
    UPDATE_ACTION_UNKNOWN
};

struct ctxt_for_update {
    struct pcvdom_node           *curr;

    enum VIA                      via;
    purc_variant_t                on;
    purc_variant_t                to;
    purc_variant_t                at;
    purc_variant_t                from;
    purc_variant_t                from_result;
    purc_variant_t                with;
    enum pchvml_attr_operator     attr_op;
    pcintr_attribute_op           attr_op_eval;

    purc_variant_t                literal;
    purc_variant_t                template_data_type;

    purc_variant_t                sync_id;
    purc_variant_t                params;
    pcintr_coroutine_t            co;

    int                           ret_code;
    int                           err;
    purc_rwstream_t               resp;
    enum update_action           action;
    bool                          individually;
    bool                          wholly;
};

static int
update_elements(pcintr_stack_t stack, purc_variant_t elems,
        purc_variant_t pos, enum update_action action,
        purc_variant_t src, pcintr_attribute_op attr_op_eval,
        purc_variant_t template_data_type, enum update_action operator);

static int
update_container(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        purc_variant_t dest, purc_variant_t pos, enum update_action action,
        purc_variant_t src, pcintr_attribute_op attr_op_eval, bool individually,
        bool wholly);

static void
ctxt_for_update_destroy(struct ctxt_for_update *ctxt)
{
    if (ctxt) {
        PURC_VARIANT_SAFE_CLEAR(ctxt->on);
        PURC_VARIANT_SAFE_CLEAR(ctxt->to);
        PURC_VARIANT_SAFE_CLEAR(ctxt->at);
        PURC_VARIANT_SAFE_CLEAR(ctxt->from);
        PURC_VARIANT_SAFE_CLEAR(ctxt->from_result);
        PURC_VARIANT_SAFE_CLEAR(ctxt->with);
        PURC_VARIANT_SAFE_CLEAR(ctxt->literal);
        PURC_VARIANT_SAFE_CLEAR(ctxt->template_data_type);
        PURC_VARIANT_SAFE_CLEAR(ctxt->sync_id);
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
    ctxt_for_update_destroy((struct ctxt_for_update*)ctxt);
}

static purc_variant_t
get_source_by_with(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
    purc_variant_t with)
{
    UNUSED_PARAM(frame);
    UNUSED_PARAM(co);
    if (purc_variant_is_string(with)) {
        purc_variant_ref(with);
        return with;
    }
    else if (purc_variant_is_native(with)) {
        purc_variant_t type = pcintr_template_get_type(with);
        if (type) {
            struct ctxt_for_update *ctxt;
            ctxt = (struct ctxt_for_update*)frame->ctxt;
            ctxt->template_data_type = purc_variant_ref(type);
        }
        return pcintr_template_expansion(with);
    }
    else {
        purc_variant_ref(with);
        return with;
    }
}

static void on_sync_complete(
        struct pcfetcher_session *session,
        purc_variant_t request_id,
        void *ud,
        enum pcfetcher_resp_type type,
        const char *data, size_t sz_data)
{
    UNUSED_PARAM(session);
    UNUSED_PARAM(request_id);
    UNUSED_PARAM(ud);

    pcintr_stack_frame_t frame;
    frame = (pcintr_stack_frame_t)ud;
    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;

    switch (type) {
    case PCFETCHER_RESP_TYPE_HEADER:
    {
        struct pcfetcher_resp_header *resp_header =
            (struct pcfetcher_resp_header *)(void *)data;
        ctxt->ret_code = resp_header->ret_code;
        PC_DEBUG("load_async|callback|ret_code=%d\n", resp_header->ret_code);
        PC_DEBUG("load_async|callback|mime_type=%s\n", resp_header->mime_type);
        PC_DEBUG("load_async|callback|sz_resp=%ld\n", resp_header->sz_resp);
        break;
    }

    case PCFETCHER_RESP_TYPE_DATA:
    {
        if (ctxt->resp == NULL) {
            ctxt->resp = purc_rwstream_new_buffer(sz_data, 0);
        }
        purc_rwstream_write(ctxt->resp, data, sz_data);
        break;
    }

    case PCFETCHER_RESP_TYPE_ERROR:
    {
        struct pcfetcher_resp_header *resp_header =
            (struct pcfetcher_resp_header *)(void *)data;
        ctxt->ret_code = resp_header->ret_code;

        if (ctxt->co->stack.exited) {
            return;
        }

        if (ctxt->resp) {
            purc_rwstream_seek(ctxt->resp, 0, SEEK_SET);
        }
        pcintr_coroutine_post_event(ctxt->co->cid,
            PCRDR_MSG_EVENT_REDUCE_OPT_KEEP,
            ctxt->sync_id, MSG_TYPE_FETCHER_STATE, MSG_SUB_TYPE_SUCCESS,
            PURC_VARIANT_INVALID, ctxt->sync_id);
        break;
    }

    case PCFETCHER_RESP_TYPE_FINISH:
    {
        if (ctxt->co->stack.exited) {
            return;
        }
        if (ctxt->resp) {
            purc_rwstream_seek(ctxt->resp, 0, SEEK_SET);
        }

        pcintr_coroutine_post_event(ctxt->co->cid,
            PCRDR_MSG_EVENT_REDUCE_OPT_KEEP,
            ctxt->sync_id, MSG_TYPE_FETCHER_STATE, MSG_SUB_TYPE_SUCCESS,
            PURC_VARIANT_INVALID, ctxt->sync_id);
        break;
    }
    }
}

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

    struct pcintr_stack_frame *frame;
    frame = (struct pcintr_stack_frame*)data;

    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;

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
    if (ret == PURC_VARIANT_INVALID) {
        frame->next_step = NEXT_STEP_ON_POPPING;
        goto out;
    }

    ctxt->from_result = ret;

out:
    pcintr_resume(cor, msg);
    pcintr_set_current_co(NULL);
    return 0;
}

static purc_variant_t
params_from_with(struct ctxt_for_update *ctxt)
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
get_source_by_from(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct ctxt_for_update *ctxt)
{
    UNUSED_PARAM(frame);

    const char* uri = purc_variant_get_string_const(ctxt->from);

    enum pcfetcher_method method;
    method = pcintr_method_from_via(ctxt->via);

    purc_variant_t params;
    params = params_from_with(ctxt);

    ctxt->co = co;
    purc_variant_t v = pcintr_load_from_uri_async(&co->stack, uri,
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

static const char *
get_op_str(enum update_action action)
{
    switch (action) {
    case UPDATE_ACTION_DISPLACE:
        return "displace";
    case UPDATE_ACTION_APPEND:
        return "append";
    case UPDATE_ACTION_PREPEND:
        return "prepend";
    case UPDATE_ACTION_MERGE:
        return "merge";
    case UPDATE_ACTION_REMOVE:
        return "remove";
    case UPDATE_ACTION_INSERTBEFORE:
        return "insertBefore";
    case UPDATE_ACTION_INSERTAFTER:
        return "insertAfter";
    case UPDATE_ACTION_ADD:
        return "add";
    case UPDATE_ACTION_UNITE:
        return "unite";
    case UPDATE_ACTION_INTERSECT:
        return "intersect";
    case UPDATE_ACTION_SUBTRACT:
        return "subtract";
    case UPDATE_ACTION_XOR:
        return "xor";
    case UPDATE_ACTION_OVERWRITE:
        return "OVERWRITE";
    default:
        return "UNKNOWN";
    }
}

static bool
is_support_attr_op(purc_variant_t src, enum pchvml_attr_operator attr_op)
{
    if (attr_op == PCHVML_ATTRIBUTE_OPERATOR) {
        return true;
    }
    enum purc_variant_type type = purc_variant_get_type(src);
    switch (type) {
    case PURC_VARIANT_TYPE_STRING:
    case PURC_VARIANT_TYPE_NUMBER:
    case PURC_VARIANT_TYPE_LONGINT:
    case PURC_VARIANT_TYPE_ULONGINT:
    case PURC_VARIANT_TYPE_LONGDOUBLE:
        return true;
    default:
        return false;
    }
}

static inline bool
is_atrribute_operator(enum pchvml_attr_operator attr_op)
{
    if (attr_op == PCHVML_ATTRIBUTE_OPERATOR) {
        return true;
    }
    return false;
}

static purc_variant_t
parse_object_key(purc_variant_t key)
{
    purc_variant_t ret = PURC_VARIANT_INVALID;
    if (!purc_variant_is_string(key)) {
        goto out;
    }

    const char *s_key = purc_variant_get_string_const(key);
    if (s_key[0] != '.') {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    s_key += 1;
    ret = purc_variant_make_string(s_key, true);

out:
    return ret;
}

static UNUSED_FUNCTION int
update_variant_object(purc_variant_t dst, purc_variant_t src,
        purc_variant_t key, enum update_action op,
        enum pchvml_attr_operator attr_op,
        pcintr_attribute_op attr_op_eval, bool silently, bool wholly)
{
    UNUSED_PARAM(wholly);
    int ret = -1;
    switch (op) {
    case UPDATE_ACTION_DISPLACE:
        if (key) {
            if (!is_support_attr_op(src, attr_op)) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                break;
            }

            purc_variant_t k = parse_object_key(key);
            if (k == PURC_VARIANT_INVALID) {
                break;
            }

            if (purc_variant_is_undefined(src)) {
                if (purc_variant_object_remove(dst, k, true)) {
                    ret = 0;
                }
                purc_variant_unref(k);
                break;
            }

            purc_variant_t o = purc_variant_object_get_ex(dst, k, true);
            purc_variant_t v = attr_op_eval(o, src);
            if (!v) {
                purc_variant_unref(k);
                break;
            }

            bool ok = purc_variant_object_set(dst, k, v);
            purc_variant_unref(v);
            purc_variant_unref(k);

            if (ok) {
                ret = 0;
            }
        }
        else {
            if (!is_atrribute_operator(attr_op)) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                break;
            }
            ret = pcvariant_container_displace(dst, src, silently);
        }
        break;

    case UPDATE_ACTION_REMOVE:
        {
            if (!is_atrribute_operator(attr_op)) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                break;
            }
            if (key) {
                purc_variant_t k = parse_object_key(key);
                if (!k) {
                    break;
                }
                if (purc_variant_object_remove(dst, k, silently)) {
                    ret = 0;
                }
                purc_variant_unref(k);
            }
            else {
                purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
            }
        }
        break;

    case UPDATE_ACTION_MERGE:
    case UPDATE_ACTION_UNITE:
        {
            if (!is_atrribute_operator(attr_op) || key) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                break;
            }
            ssize_t sz = purc_variant_object_unite(dst, src,
                        PCVRNT_CR_METHOD_OVERWRITE);
            if (sz >= 0) {
                ret = 0;
            }
        }
        break;

    case UPDATE_ACTION_INTERSECT:
        {
            if (!is_atrribute_operator(attr_op) || key) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                break;
            }
            if (-1 != purc_variant_object_intersect(dst, src)) {
                ret = 0;
            }
        }
        break;

    case UPDATE_ACTION_SUBTRACT:
        {
            if (!is_atrribute_operator(attr_op) || key) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                break;
            }
            if (-1 == purc_variant_object_subtract(dst, src)) {
                ret = 0;
            }
        }
        break;

    case UPDATE_ACTION_XOR:
        {
            if (!is_atrribute_operator(attr_op) || key) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                break;
            }
            if (-1 != purc_variant_object_xor(dst, src)) {
                ret = 0;
            }
        }
        break;

    case UPDATE_ACTION_OVERWRITE:
        {
            if (!is_atrribute_operator(attr_op) || key) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                break;
            }
            if (-1 == purc_variant_object_overwrite(dst, src,
                        PCVRNT_NR_METHOD_IGNORE)) {
                ret = 0;
            }
        }
        break;

    case UPDATE_ACTION_APPEND:
    case UPDATE_ACTION_PREPEND:
    case UPDATE_ACTION_INSERTBEFORE:
    case UPDATE_ACTION_INSERTAFTER:
    case UPDATE_ACTION_ADD:
    case UPDATE_ACTION_UNKNOWN:
    default:
        purc_set_error(PURC_ERROR_NOT_ALLOWED);
        break;
    }

    return ret;
}

static int
array_append(purc_variant_t dst, purc_variant_t src, bool silently, bool wholly)
{
    UNUSED_PARAM(silently);
    int ret = -1;
    if (wholly || !pcvariant_is_linear_container(src)) {
        if (purc_variant_array_append(dst, src)) {
            ret = 0;
        }
        goto out;
    }

    size_t nr_items = purc_variant_linear_container_get_size(src);
    for (size_t i = 0; i < nr_items; i++) {
        purc_variant_t v = purc_variant_linear_container_get(src, i);
        if (!purc_variant_array_append(dst, v)) {
            goto out;
        }
    }
    ret = 0;

out:
    return ret;
}

static int
array_prepend(purc_variant_t dst, purc_variant_t src, bool silently, bool wholly)
{
    UNUSED_PARAM(silently);
    int ret = -1;
    if (wholly || !pcvariant_is_linear_container(src)) {
        if (purc_variant_array_prepend(dst, src)) {
            ret = 0;
        }
        goto out;
    }

    size_t nr_items = purc_variant_linear_container_get_size(src);
    for (size_t i = nr_items; i > 0; i--) {
        purc_variant_t v = purc_variant_linear_container_get(src, i - 1);
        if (!purc_variant_array_prepend(dst, v)) {
            goto out;
        }
    }
    ret = 0;

out:
    return ret;
}

static int
array_insert_before(purc_variant_t dst, int idx, purc_variant_t src,
        bool silently, bool wholly)
{
    UNUSED_PARAM(silently);
    int ret = -1;
    if (wholly || !pcvariant_is_linear_container(src)) {
        if (purc_variant_array_insert_before(dst, idx, src)) {
            ret = 0;
        }
        goto out;
    }

    size_t nr_items = purc_variant_linear_container_get_size(src);
    for (size_t i = nr_items; i > 0; i--) {
        purc_variant_t v = purc_variant_linear_container_get(src, i - 1);
        if (!purc_variant_array_insert_before(dst, idx, v)) {
            goto out;
        }
    }
    ret = 0;

out:
    return ret;
}

static int
array_insert_after(purc_variant_t dst, int idx, purc_variant_t src,
        bool silently, bool wholly)
{
    UNUSED_PARAM(silently);
    int ret = -1;
    if (wholly || !pcvariant_is_linear_container(src)) {
        if (purc_variant_array_insert_after(dst, idx, src)) {
            ret = 0;
        }
        goto out;
    }

    size_t nr_items = purc_variant_linear_container_get_size(src);
    for (size_t i = nr_items; i > 0; i--) {
        purc_variant_t v = purc_variant_linear_container_get(src, i - 1);
        if (!purc_variant_array_insert_after(dst, idx, v)) {
            goto out;
        }
    }
    ret = 0;

out:
    return ret;
}

static UNUSED_FUNCTION int
update_variant_array(purc_variant_t dst, purc_variant_t src,
        int idx, enum update_action op,
        enum pchvml_attr_operator attr_op,
        pcintr_attribute_op attr_op_eval, bool silently, bool wholly)
{
    UNUSED_PARAM(wholly);
    int ret = -1;
    switch (op) {
    case UPDATE_ACTION_DISPLACE:
        if (idx >= 0) {
            if (!is_support_attr_op(src, attr_op)) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                break;
            }
            if (purc_variant_is_undefined(src)) {
                if (purc_variant_array_remove(dst, idx)) {
                    ret = 0;
                }
                break;
            }

            purc_variant_t o = purc_variant_array_get(dst, idx);
            purc_variant_t v = attr_op_eval(o, src);
            if (!v) {
                break;
            }

            bool ok = purc_variant_array_set(dst, idx, v);
            purc_variant_unref(v);

            if (ok) {
                ret = 0;
            }
        }
        else if (pcvariant_container_displace(dst, src, silently)) {
            ret = 0;
        }
        break;

    case UPDATE_ACTION_APPEND:
    case UPDATE_ACTION_ADD:
        {
            if (!is_atrribute_operator(attr_op) || idx >= 0) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                break;
            }

            ret = array_append(dst, src, silently, wholly);
        }
        break;

    case UPDATE_ACTION_PREPEND:
        {
            if (!is_atrribute_operator(attr_op) || idx >= 0) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                break;
            }
            ret = array_prepend(dst, src, silently, wholly);
        }
        break;

    case UPDATE_ACTION_REMOVE:
        {
            if (!is_atrribute_operator(attr_op)) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                break;
            }
            bool r = false;
            if (idx >= 0) {
                r = purc_variant_array_remove(dst, idx);
            }
            else {
                purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
            }
            if (r) {
                ret = 0;
            }
        }
        break;

    case UPDATE_ACTION_INSERTBEFORE:
        {
            if (!is_atrribute_operator(attr_op) || idx < 0) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                break;
            }

            ret = array_insert_before(dst, idx, src, silently, wholly);
        }
        break;

    case UPDATE_ACTION_INSERTAFTER:
        {
            if (!is_atrribute_operator(attr_op) || idx < 0) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                break;
            }

            ret = array_insert_after(dst, idx, src, silently, wholly);
        }
        break;

    case UPDATE_ACTION_MERGE:
    case UPDATE_ACTION_UNITE:
    case UPDATE_ACTION_INTERSECT:
    case UPDATE_ACTION_SUBTRACT:
    case UPDATE_ACTION_XOR:
    case UPDATE_ACTION_OVERWRITE:
    case UPDATE_ACTION_UNKNOWN:
    default:
        purc_set_error(PURC_ERROR_NOT_ALLOWED);
        break;
    }

    return ret;
}

static int
set_add(purc_variant_t dst, purc_variant_t src, bool silently, bool wholly)
{
    UNUSED_PARAM(silently);
    int ret = -1;
    if (wholly || !pcvariant_is_linear_container(src)) {
        if (-1 != purc_variant_set_add(dst, src,
                        PCVRNT_CR_METHOD_OVERWRITE)) {
            ret = 0;
        }
        goto out;
    }

    size_t nr_items = purc_variant_linear_container_get_size(src);
    for (size_t i = 0; i < nr_items; i++) {
        purc_variant_t v = purc_variant_linear_container_get(src, i);
        if (-1 == purc_variant_set_add(dst, v,
                        PCVRNT_CR_METHOD_OVERWRITE)) {
            goto out;
        }
    }
    ret = 0;

out:
    return ret;
}

static int
set_remove(purc_variant_t dst, purc_variant_t src, bool silently, bool wholly)
{
    UNUSED_PARAM(silently);
    int ret = -1;
    if (wholly || !pcvariant_is_linear_container(src)) {
        if (-1 != purc_variant_set_remove(dst, src,
                        PCVRNT_NR_METHOD_IGNORE)) {
            ret = 0;
        }
        goto out;
    }

    size_t nr_items = purc_variant_linear_container_get_size(src);
    for (size_t i = 0; i < nr_items; i++) {
        purc_variant_t v = purc_variant_linear_container_get(src, i);
        if (-1 == purc_variant_set_remove(dst, v,
                        PCVRNT_NR_METHOD_IGNORE)) {
            goto out;
        }
    }
    ret = 0;

out:
    return ret;
}

static UNUSED_FUNCTION int
update_variant_set(purc_variant_t dst, purc_variant_t src,
        int idx, enum update_action op,
        enum pchvml_attr_operator attr_op,
        pcintr_attribute_op attr_op_eval, bool silently, bool wholly)
{
    UNUSED_PARAM(wholly);
    int ret = -1;
    switch (op) {
    case UPDATE_ACTION_DISPLACE:
        if (idx >= 0) {
            if (!is_support_attr_op(src, attr_op)) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                break;
            }

            if (purc_variant_is_undefined(src)) {
                purc_variant_t v = purc_variant_set_remove_by_index(dst, idx);
                if (v) {
                    purc_variant_unref(v);
                    ret = 0;
                }
                break;
            }

            purc_variant_t o = purc_variant_set_get_by_index(dst, idx);
            purc_variant_t v = attr_op_eval(o, src);
            if (!v) {
                break;
            }

            bool ok = purc_variant_set_set_by_index(dst, idx, v);
            purc_variant_unref(v);

            if (ok) {
                ret = 0;
            }
        }
        else if (pcvariant_container_displace(dst, src, silently)){
            ret = 0;
        }
        break;

    case UPDATE_ACTION_ADD:
        {
            ret = set_add(dst, src, silently, wholly);
        }
        break;

    case UPDATE_ACTION_REMOVE:
        {
            if (!is_atrribute_operator(attr_op)) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                break;
            }
            if (idx >= 0) {
                purc_variant_t v = purc_variant_set_remove_by_index(dst, idx);
                if (v) {
                    ret = 0;
                    purc_variant_unref(v);
                }
            }
            else if (src) {
                ret = set_remove(dst, src, silently, wholly);
            }
            else {
                purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
            }
        }
        break;

    case UPDATE_ACTION_MERGE:
    case UPDATE_ACTION_UNITE:
        {
            if (!is_atrribute_operator(attr_op) || idx >= 0) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                break;
            }
            if (-1 != purc_variant_set_unite(dst, src,
                        PCVRNT_CR_METHOD_OVERWRITE)) {
                ret = 0;
            }
        }
        break;

    case UPDATE_ACTION_INTERSECT:
        {
            if (!is_atrribute_operator(attr_op) || idx >= 0) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                break;
            }
            if (-1 != purc_variant_set_intersect(dst, src)) {
                ret = 0;
            }
        }
        break;

    case UPDATE_ACTION_SUBTRACT:
        {
            if (!is_atrribute_operator(attr_op) || idx >= 0) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                break;
            }
            if (-1 == purc_variant_set_subtract(dst, src)) {
                ret = 0;
            }
        }
        break;

    case UPDATE_ACTION_XOR:
        {
            if (!is_atrribute_operator(attr_op) || idx >= 0) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                break;
            }
            if (-1 != purc_variant_set_xor(dst, src)) {
                ret = 0;
            }
        }
        break;

    case UPDATE_ACTION_OVERWRITE:
        {
            if (!is_atrribute_operator(attr_op) || idx >= 0) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                break;
            }
            if (-1 == purc_variant_set_overwrite(dst, src,
                        PCVRNT_NR_METHOD_IGNORE)) {
                ret = 0;
            }
        }
        break;

    case UPDATE_ACTION_APPEND:
    case UPDATE_ACTION_PREPEND:
    case UPDATE_ACTION_INSERTBEFORE:
    case UPDATE_ACTION_INSERTAFTER:
    case UPDATE_ACTION_UNKNOWN:
    default:
        purc_set_error(PURC_ERROR_NOT_ALLOWED);
        break;
    }
    return ret;
}

static UNUSED_FUNCTION int
update_variant_tuple(purc_variant_t dst, purc_variant_t src,
        int idx, enum update_action op,
        enum pchvml_attr_operator attr_op,
        pcintr_attribute_op attr_op_eval, bool silently, bool wholly)
{
    UNUSED_PARAM(silently);
    UNUSED_PARAM(wholly);
    int ret = -1;
    switch (op) {
    case UPDATE_ACTION_DISPLACE:
        if (idx >= 0) {
            if (!is_support_attr_op(src, attr_op)) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                break;
            }

            purc_variant_t o = purc_variant_tuple_get(dst, idx);
            purc_variant_t v = attr_op_eval(o, src);
            if (!v) {
                break;
            }

            bool ok = purc_variant_tuple_set(dst, idx, v);
            purc_variant_unref(v);

            if (ok) {
                ret = 0;
            }
        }
        else {
            purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        }
        break;

    case UPDATE_ACTION_REMOVE:
    case UPDATE_ACTION_APPEND:
    case UPDATE_ACTION_PREPEND:
    case UPDATE_ACTION_INSERTBEFORE:
    case UPDATE_ACTION_INSERTAFTER:
    case UPDATE_ACTION_ADD:
    case UPDATE_ACTION_MERGE:
    case UPDATE_ACTION_UNITE:
    case UPDATE_ACTION_INTERSECT:
    case UPDATE_ACTION_SUBTRACT:
    case UPDATE_ACTION_XOR:
    case UPDATE_ACTION_OVERWRITE:
    case UPDATE_ACTION_UNKNOWN:
    default:
        purc_set_error(PURC_ERROR_NOT_ALLOWED);
        break;
    }

    return ret;
}

static int
update_object(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        purc_variant_t dest, purc_variant_t pos, enum update_action action,
        purc_variant_t src, pcintr_attribute_op attr_op_eval, bool individually,
        bool wholly)
{
    UNUSED_PARAM(co);
    purc_variant_t ultimate = PURC_VARIANT_INVALID;

    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
    struct pcvdom_element *element = frame->pos;
    const char *op = get_op_str(action);
    int ret = -1;

    if (individually) {
        ssize_t sz = purc_variant_object_get_size(dest);
        if (sz <= 0) {
            ret = 0;
            goto out;
        }

        purc_variant_t k, v;
        UNUSED_VARIABLE(k);
        foreach_key_value_in_variant_object(dest, k, v)
            ret = update_container(co, frame, v, pos, action, src, attr_op_eval, false,
                    wholly);
            if (ret != 0) {
                goto out;
            }
        end_foreach;

        goto out;
    }

    if (pos) {
        if (!purc_variant_is_string(pos)) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto out;
        }
        purc_variant_t k = parse_object_key(pos);
        if (!k) {
            goto out;
        }
        ultimate = purc_variant_object_get_ex(dest, k, true);
        if (!ultimate && frame->silently) {
            purc_variant_unref(k);
            goto out;
        }
        purc_variant_unref(k);
    }
    else {
        ultimate = dest;
    }

    if (ultimate == dest) {
        ret = update_variant_object(dest, src, pos, ctxt->action, ctxt->attr_op,
                attr_op_eval, frame->silently, wholly);
    }
    else {
        switch (ctxt->action) {
        case UPDATE_ACTION_DISPLACE:
        case UPDATE_ACTION_REMOVE:
            ret = update_variant_object(dest, src, pos, ctxt->action, ctxt->attr_op,
                    attr_op_eval, frame->silently, wholly);
            break;
        case UPDATE_ACTION_APPEND:
        case UPDATE_ACTION_PREPEND:
        case UPDATE_ACTION_INSERTBEFORE:
        case UPDATE_ACTION_INSERTAFTER:
        case UPDATE_ACTION_ADD:
        case UPDATE_ACTION_MERGE:
        case UPDATE_ACTION_UNITE:
        case UPDATE_ACTION_INTERSECT:
        case UPDATE_ACTION_SUBTRACT:
        case UPDATE_ACTION_XOR:
        case UPDATE_ACTION_OVERWRITE:
            ret = update_container(co, frame, ultimate, PURC_VARIANT_INVALID,
                    action, src, attr_op_eval, false, wholly);
            break;
        case UPDATE_ACTION_UNKNOWN:
        default:
            purc_set_error_with_info(PURC_ERROR_NOT_ALLOWED,
                    "vdom attribute '%s'='%s' for element <%s>",
                    pchvml_keyword_str(PCHVML_KEYWORD_ENUM(HVML, TO)),
                    op, element->tag_name);
        }
    }

out:
    return ret;
}

static int
update_array(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        purc_variant_t dest, purc_variant_t pos, enum update_action action,
        purc_variant_t src, pcintr_attribute_op attr_op_eval, bool individually,
        bool wholly)
{
    UNUSED_PARAM(attr_op_eval);
    UNUSED_PARAM(co);

    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;

    purc_variant_t ultimate = PURC_VARIANT_INVALID;

    struct pcvdom_element *element = frame->pos;
    const char *op = get_op_str(action);

    int ret = -1;
    size_t idx = -1;

    if (individually) {
        ssize_t sz = purc_variant_array_get_size(dest);
        if (sz <= 0) {
            ret = 0;
            goto out;
        }

        purc_variant_t val;
        size_t idx;
        UNUSED_VARIABLE(idx);
        foreach_value_in_variant_array(dest, val, idx)
            ret = update_container(co, frame, val, pos, action, src, attr_op_eval, false,
                    wholly);
            if (ret != 0) {
                goto out;
            }
        end_foreach;

        goto out;
    }

    if (pos) {
        idx = (size_t) purc_variant_numerify(pos);
        ultimate = purc_variant_array_get(dest, idx);
        if (!ultimate) {
            ret = 0;
            goto out;
        }
    }
    else {
        ultimate = dest;
    }

    if (ultimate == dest) {
        ret = update_variant_array(dest, src, idx, ctxt->action, ctxt->attr_op,
                attr_op_eval, frame->silently, wholly);
    }
    else {
        switch (ctxt->action) {
        case UPDATE_ACTION_DISPLACE:
        case UPDATE_ACTION_REMOVE:
        case UPDATE_ACTION_INSERTBEFORE:
        case UPDATE_ACTION_INSERTAFTER:
            ret = update_variant_array(dest, src, idx, ctxt->action, ctxt->attr_op,
                    attr_op_eval, frame->silently, wholly);
            break;
        case UPDATE_ACTION_APPEND:
        case UPDATE_ACTION_ADD:
        case UPDATE_ACTION_PREPEND:
        case UPDATE_ACTION_MERGE:
        case UPDATE_ACTION_UNITE:
        case UPDATE_ACTION_INTERSECT:
        case UPDATE_ACTION_SUBTRACT:
        case UPDATE_ACTION_XOR:
        case UPDATE_ACTION_OVERWRITE:
            ret = update_container(co, frame, ultimate, PURC_VARIANT_INVALID,
                    action, src, attr_op_eval, false, wholly);
            break;
        case UPDATE_ACTION_UNKNOWN:
        default:
            purc_set_error_with_info(PURC_ERROR_NOT_ALLOWED,
                    "vdom attribute '%s'='%s' for element <%s>",
                    pchvml_keyword_str(PCHVML_KEYWORD_ENUM(HVML, TO)),
                    op, element->tag_name);
        }
    }

out:
    return ret;
}

static int
update_set(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        purc_variant_t dest, purc_variant_t pos, enum update_action action,
        purc_variant_t src, pcintr_attribute_op attr_op_eval, bool individually,
        bool wholly)
{
    UNUSED_PARAM(attr_op_eval);
    UNUSED_PARAM(co);
    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
    struct pcvdom_element *element = frame->pos;
    purc_variant_t ultimate = PURC_VARIANT_INVALID;
    const char *op = get_op_str(action);

    int ret = -1;
    size_t idx = -1;
    if (individually) {
        ssize_t sz = purc_variant_set_get_size(dest);
        if (sz <= 0) {
            ret = 0;
            goto out;
        }

        purc_variant_t v;
        foreach_value_in_variant_set_order(dest, v)
            ret = update_container(co, frame, v, pos, action, src, attr_op_eval, false,
                    wholly);
            if (ret != 0) {
                goto out;
            }
        end_foreach;

        goto out;
    }

    if (pos) {
        idx = (size_t) purc_variant_numerify(pos);
        ultimate = purc_variant_set_get_by_index(dest, idx);
        if (!ultimate) {
            ret = 0;
            goto out;
        }
    }
    else {
        ultimate = dest;
    }

    if (ultimate == dest) {
        ret = update_variant_set(dest, src, idx, ctxt->action, ctxt->attr_op,
                attr_op_eval, frame->silently, wholly);
    }
    else {
        switch (ctxt->action) {
        case UPDATE_ACTION_DISPLACE:
        case UPDATE_ACTION_REMOVE:
            ret = update_variant_set(dest, src, idx, ctxt->action, ctxt->attr_op,
                    attr_op_eval, frame->silently, wholly);
            break;
        case UPDATE_ACTION_APPEND:
        case UPDATE_ACTION_PREPEND:
        case UPDATE_ACTION_INSERTBEFORE:
        case UPDATE_ACTION_INSERTAFTER:
        case UPDATE_ACTION_ADD:
        case UPDATE_ACTION_MERGE:
        case UPDATE_ACTION_UNITE:
        case UPDATE_ACTION_INTERSECT:
        case UPDATE_ACTION_SUBTRACT:
        case UPDATE_ACTION_XOR:
        case UPDATE_ACTION_OVERWRITE:
            ret = update_container(co, frame, ultimate, PURC_VARIANT_INVALID,
                    action, src, attr_op_eval, false, wholly);
            break;
        case UPDATE_ACTION_UNKNOWN:
        default:
            purc_set_error_with_info(PURC_ERROR_NOT_ALLOWED,
                    "vdom attribute '%s'='%s' for element <%s>",
                    pchvml_keyword_str(PCHVML_KEYWORD_ENUM(HVML, TO)),
                    op, element->tag_name);
        }
    }

out:
    return ret;
}

static int
update_tuple(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        purc_variant_t dest, purc_variant_t pos, enum update_action action,
        purc_variant_t src, pcintr_attribute_op attr_op_eval, bool individually, 
        bool wholly)
{
    UNUSED_PARAM(attr_op_eval);
    UNUSED_PARAM(co);
    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;

    struct pcvdom_element *element = frame->pos;
    purc_variant_t ultimate = PURC_VARIANT_INVALID;
    const char *op = get_op_str(action);
    int ret = -1;
    size_t idx = -1;

    if (individually) {
        ssize_t sz = purc_variant_tuple_get_size(dest);
        if (sz <= 0) {
            ret = 0;
            goto out;
        }

        purc_variant_t val;
        for (ssize_t i = 0; i < sz; i++) {
            val = purc_variant_tuple_get(dest, i);
            if (val) {
                ret = update_container(co, frame, val, pos, action, src, attr_op_eval, false,
                        wholly);
                if (ret != 0) {
                    goto out;
                }
            }
        }

        goto out;
    }

    if (pos) {
        idx = (size_t) purc_variant_numerify(pos);
        ultimate = purc_variant_tuple_get(dest, idx);
        if (!ultimate) {
            ret = 0;
            goto out;
        }
    }
    else {
        ultimate = dest;
    }

    if (ultimate == dest) {
        ret = update_variant_tuple(dest, src, idx, ctxt->action, ctxt->attr_op,
                attr_op_eval, frame->silently, wholly);
    }
    else {
        switch (ctxt->action) {
        case UPDATE_ACTION_DISPLACE:
        case UPDATE_ACTION_REMOVE:
            ret = update_variant_tuple(dest, src, idx, ctxt->action, ctxt->attr_op,
                    attr_op_eval, frame->silently, wholly);
            break;
        case UPDATE_ACTION_APPEND:
        case UPDATE_ACTION_PREPEND:
        case UPDATE_ACTION_INSERTBEFORE:
        case UPDATE_ACTION_INSERTAFTER:
        case UPDATE_ACTION_ADD:
        case UPDATE_ACTION_MERGE:
        case UPDATE_ACTION_UNITE:
        case UPDATE_ACTION_INTERSECT:
        case UPDATE_ACTION_SUBTRACT:
        case UPDATE_ACTION_XOR:
        case UPDATE_ACTION_OVERWRITE:
            ret = update_container(co, frame, ultimate, PURC_VARIANT_INVALID,
                    action, src, attr_op_eval, false, wholly);
            break;
        case UPDATE_ACTION_UNKNOWN:
        default:
            purc_set_error_with_info(PURC_ERROR_NOT_ALLOWED,
                    "vdom attribute '%s'='%s' for element <%s>",
                    pchvml_keyword_str(PCHVML_KEYWORD_ENUM(HVML, TO)),
                    op, element->tag_name);
        }
    }

out:
    return ret;
}

int
update_container(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        purc_variant_t dest, purc_variant_t pos, enum update_action action,
        purc_variant_t src, pcintr_attribute_op attr_op_eval, bool individually,
        bool wholly)
{
    int ret = -1;
    enum purc_variant_type type = purc_variant_get_type(dest);
    switch (type) {
    case PURC_VARIANT_TYPE_OBJECT:
        ret = update_object(co, frame, dest, pos, action, src, attr_op_eval,
                individually, wholly);
        break;

    case PURC_VARIANT_TYPE_ARRAY:
        ret = update_array(co, frame, dest, pos, action, src, attr_op_eval,
                individually, wholly);
        break;

    case PURC_VARIANT_TYPE_SET:
        ret = update_set(co, frame, dest, pos, action, src, attr_op_eval,
                individually, wholly);
        break;

    case PURC_VARIANT_TYPE_TUPLE:
        ret = update_tuple(co, frame, dest, pos, action, src, attr_op_eval,
                individually, wholly);
        break;

    default:
        purc_set_error(PURC_ERROR_NOT_ALLOWED);
        break;
    }
    return ret;
}

static pcdoc_operation_k convert_operation(enum update_action operator)
{
    pcdoc_operation_k op;

    switch (operator) {
    case UPDATE_ACTION_APPEND:
    case UPDATE_ACTION_ADD:
        op = PCDOC_OP_APPEND;
        break;
    case UPDATE_ACTION_PREPEND:
        op = PCDOC_OP_PREPEND;
        break;
    case UPDATE_ACTION_INSERTBEFORE:
        op = PCDOC_OP_INSERTBEFORE;
        break;
    case UPDATE_ACTION_INSERTAFTER:
        op = PCDOC_OP_INSERTAFTER;
        break;
    case UPDATE_ACTION_DISPLACE:
        op = PCDOC_OP_DISPLACE;
        break;
    default:
        op = PCDOC_OP_UNKNOWN;
        break;
    }

    return op;
}

static bool
is_no_return()
{
    struct pcinst* inst = pcinst_current();
    struct pcrdr_conn *conn = inst->curr_conn ? inst->curr_conn :
        inst->conn_to_rdr;
    return conn && (conn->prot != PURC_RDRCOMM_THREAD);
}

static int
update_elem_child(pcintr_stack_t stack, pcdoc_element_t target,
        enum update_action action, purc_variant_t src,
        pcintr_attribute_op attr_op_eval, purc_variant_t template_data_type,
        enum update_action operator)
{
    UNUSED_PARAM(stack);
    UNUSED_PARAM(action);

    char *t = NULL;
    const char *s = "undefined";
    if (purc_variant_is_undefined(src)) {
        s = "undefined";
    }
    else if (purc_variant_is_string(src)) {
        s = purc_variant_get_string_const(src);
    }
    else {
        ssize_t n = purc_variant_stringify_alloc(&t, src);
        if (n<=0) {
            return -1;
        }
        s = t;
    }

    UNUSED_PARAM(attr_op_eval);

    pcdoc_operation_k op = convert_operation(operator);
    if (op != PCDOC_OP_UNKNOWN) {
        size_t len = strlen(s);
        if (len > 0) {
            pcintr_util_new_content(stack->doc, target, op, s, 0,
                    template_data_type, true, is_no_return());
        }
        if (t) {
            free(t);
        }

        return 0;
    }

    if (t)
        free(t);

    return -1;
}

static int
update_elem_content(pcintr_stack_t stack, pcdoc_element_t target,
        enum update_action action, purc_variant_t src,
        pcintr_attribute_op attr_op_eval, enum update_action operator)
{
    UNUSED_PARAM(stack);
    UNUSED_PARAM(action);
    UNUSED_PARAM(attr_op_eval);

    pcdoc_operation_k op = convert_operation(operator);
    if (op == PCDOC_OP_UNKNOWN) {
        return -1;
    }

    if (purc_variant_is_string(src)) {
        size_t len;
        const char *s = purc_variant_get_string_const_ex(src, &len);

        pcintr_util_new_text_content(stack->doc, target, op, s, len, true,
                is_no_return());
        return 0;
    }
    else {
        char *buf = NULL;
        int total = purc_variant_stringify_alloc(&buf, src);
        if (buf) {
            pcintr_util_new_text_content(stack->doc, target, op, buf, total, true,
                    is_no_return());
            free(buf);
            return 0;
        }
    }
    PRINT_VARIANT(src);
    return -1;
}

static int
displace_elem_attr(pcintr_stack_t stack, pcdoc_element_t target,
        const char *pos, purc_variant_t src,
        pcintr_attribute_op attr_op_eval)
{
    UNUSED_PARAM(stack);
    const char *origin = NULL;
    size_t len;
    pcdoc_element_get_attribute(stack->doc, target,
            (const char*)pos, &origin, &len);

    purc_variant_t v;
    if (origin) {
        purc_variant_t l = purc_variant_make_string_static(origin, true);
        if (l == PURC_VARIANT_INVALID)
            return -1;

        v = attr_op_eval(l, src);
        purc_variant_unref(l);
        if (v == PURC_VARIANT_INVALID)
            return -1;
    }
    else {
        v = purc_variant_ref(src);
    }

    int r;
    if (purc_variant_is_string(v)) {
        size_t sz;
        const char *s = purc_variant_get_string_const_ex(v, &sz);
        if (!s) {
            purc_variant_unref(v);
            return -1;
        }

        /* The new value is the same as the original value */
        if (origin == s) {
            purc_variant_unref(v);
            return 0;
        }

        r = pcintr_util_set_attribute(stack->doc, target,
                PCDOC_OP_DISPLACE, pos, s, sz, true, is_no_return());
        purc_variant_unref(v);
    }
    else {
        char *s = pcvariant_to_string(v);
        if (!s) {
            purc_variant_unref(v);
            return -1;
        }
        r = pcintr_util_set_attribute(stack->doc, target,
                PCDOC_OP_DISPLACE, pos, s, strlen(s), true, is_no_return());
        purc_variant_unref(v);
        free(s);
    }
    return r ? -1 : 0;
}

static int
update_elem_attr(pcintr_stack_t stack, pcdoc_element_t target,
        const char *pos, enum update_action action, purc_variant_t src,
        pcintr_attribute_op attr_op_eval)
{
    UNUSED_PARAM(stack);
    if (purc_variant_is_string(src) || pcvariant_is_of_number(src)) {
        if (action == UPDATE_ACTION_DISPLACE) {
            // +=, -=, *=, /=, %=, ~=, ^=, $=
            return displace_elem_attr(stack, target, pos, src, attr_op_eval);
        }
        return -1;
    }
    char *sv = pcvariant_to_string(src);

    pcintr_util_set_attribute(stack->doc, target,
            PCDOC_OP_DISPLACE, pos, sv, 0, true, is_no_return());
    free(sv);

    return 0;
}

static int
update_elem(pcintr_stack_t stack, pcdoc_element_t target,
        purc_variant_t pos, enum update_action action, purc_variant_t src,
        pcintr_attribute_op attr_op_eval, purc_variant_t template_data_type,
        enum update_action operator)
{
    const char *s_pos = NULL;
    if (pos != PURC_VARIANT_INVALID) {
        s_pos = purc_variant_get_string_const(pos);
    }

    if (!s_pos || strcmp(s_pos, AT_KEY_CONTENT) == 0) {
        return update_elem_child(stack, target, action, src, attr_op_eval,
                template_data_type, operator);
    }
    if (strcmp(s_pos, AT_KEY_TEXT_CONTENT) == 0) {
        return update_elem_content(stack, target, action, src, attr_op_eval, operator);
    }
    if (strncmp(s_pos, AT_KEY_ATTR, 5) == 0) {
        s_pos += 5;
        return update_elem_attr(stack, target, s_pos, action, src, attr_op_eval);
    }

    return -1;
}

int
update_elements(pcintr_stack_t stack, purc_variant_t elems,
        purc_variant_t pos, enum update_action action,
        purc_variant_t src, pcintr_attribute_op attr_op_eval,
        purc_variant_t template_data_type, enum update_action operator)
{
    size_t idx = 0;
    while (1) {
        pcdoc_element_t target;
        target = pcdvobjs_get_element_from_elements(elems, idx++);
        if (!target)
            break;
        int r = update_elem(stack, target, pos, action, src, attr_op_eval,
                template_data_type, operator);
        if (r)
            return -1;
    }

    return 0;
}

static enum update_action
to_operator(const char *action)
{
    enum update_action ret = UPDATE_ACTION_UNKNOWN;
    purc_atom_t op = purc_atom_try_string_ex(ATOM_BUCKET_HVML, action);
    if (!op && strcmp(action, KEYWORD_ADD) == 0) {
        op = purc_atom_try_string_ex(ATOM_BUCKET_HVML, KEYWORD_UPPERCASE_ADD);
    }

    if (!op) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, DISPLACE)) == op) {
        ret = UPDATE_ACTION_DISPLACE;
    }
    else if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, APPEND)) == op) {
        ret = UPDATE_ACTION_APPEND;
    }
    else if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, PREPEND)) == op) {
        ret = UPDATE_ACTION_PREPEND;
    }
    else if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, MERGE)) == op) {
        ret = UPDATE_ACTION_MERGE;
    }
    else if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, REMOVE)) == op) {
        ret = UPDATE_ACTION_REMOVE;
    }
    else if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, INSERTBEFORE)) == op) {
        ret = UPDATE_ACTION_INSERTBEFORE;
    }
    else if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, INSERTAFTER)) == op) {
        ret = UPDATE_ACTION_INSERTAFTER;
    }
    else if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, ADD)) == op) {
        ret = UPDATE_ACTION_ADD;
    }
    else if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, UNITE)) == op) {
        ret = UPDATE_ACTION_UNITE;
    }
    else if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, INTERSECT)) == op) {
        ret = UPDATE_ACTION_INTERSECT;
    }
    else if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, SUBTRACT)) == op) {
        ret = UPDATE_ACTION_SUBTRACT;
    }
    else if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, XOR)) == op) {
        ret = UPDATE_ACTION_XOR;
    }
    else if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, OVERWRITE)) == op) {
        ret = UPDATE_ACTION_OVERWRITE;
    }

out:
    return ret;
}

static int
process_elem_coll(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        purc_variant_t dst, purc_variant_t src, purc_variant_t dst_pos,
        enum update_action action, pcintr_attribute_op attr_op_eval,
        purc_variant_t template_data_type, bool individually, bool wholly)
{
    UNUSED_PARAM(individually);
    UNUSED_PARAM(wholly);

    int ret = -1;
    purc_variant_t pos = PURC_VARIANT_INVALID;
    size_t nr_dst_pos = dst_pos ? purc_variant_array_get_size(dst_pos) : 0;
    struct ctxt_for_update *ctxt = (struct ctxt_for_update*)frame->ctxt;

    switch (nr_dst_pos) {
    case 0:
        ret = update_elements(&co->stack, dst, pos, action, src, attr_op_eval,
                template_data_type, ctxt->action);
        break;

    case 1:
        pos = purc_variant_array_get(dst_pos, 0);
        ret = update_elements(&co->stack, dst, pos, action, src, attr_op_eval,
                template_data_type, ctxt->action);
        break;

    default:
    {
        bool is_array = src ? purc_variant_is_array(src) : false;
        size_t nr_array = is_array ? purc_variant_array_get_size(src) : 0;
        for (size_t i = 0; i < nr_dst_pos; i++) {
            purc_variant_t new_pos = purc_variant_array_get(dst_pos, i);
            purc_variant_t new_src = src;
            if (is_array) {
                if (i < nr_array) {
                    new_src = purc_variant_array_get(src, i);
                }
                else {
                    new_src = purc_variant_array_get(src, nr_array - 1);
                }
            }

            ret = update_elements(&co->stack, dst, new_pos, action, new_src,
                    attr_op_eval, template_data_type, ctxt->action);
            if (ret) {
                goto out;
            }
        }
        break;
    }
    }

out:
    return ret;
}

static int
process_container(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        purc_variant_t dst, purc_variant_t src, purc_variant_t dst_pos,
        enum update_action action, pcintr_attribute_op attr_op_eval,
        purc_variant_t template_data_type, bool individually, bool wholly)
{
    UNUSED_PARAM(template_data_type);

    int ret = -1;
    purc_variant_t pos = PURC_VARIANT_INVALID;
    size_t nr_dst_pos = dst_pos ? purc_variant_array_get_size(dst_pos) : 0;

    switch (nr_dst_pos) {
    case 0:
        ret = update_container(co, frame, dst, pos, action, src, attr_op_eval,
                individually, wholly);
        break;

    case 1:
        pos = purc_variant_array_get(dst_pos, 0);
        ret = update_container(co, frame, dst, pos, action, src, attr_op_eval,
                individually, wholly);
        break;

    default:
    {
        bool is_array = src ? purc_variant_is_array(src) : false;
        size_t nr_array = is_array ? purc_variant_array_get_size(src) : 0;
        for (size_t i = 0; i < nr_dst_pos; i++) {
            purc_variant_t new_pos = purc_variant_array_get(dst_pos, i);
            purc_variant_t new_src = src;
            if (is_array) {
                if (i < nr_array) {
                    new_src = purc_variant_array_get(src, i);
                }
                else {
                    new_src = purc_variant_array_get(src, nr_array - 1);
                }
            }

            ret = update_container(co, frame, dst, new_pos, action, new_src,
                    attr_op_eval, individually, wholly);
            if (ret) {
                goto out;
            }
        }
        break;
    }
    }

out:
    return ret;
}

static int
process(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        purc_variant_t src,
        pcintr_attribute_op attr_op_eval)
{
    UNUSED_PARAM(co);
    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
    purc_variant_t on  = ctxt->on;
    enum update_action action  = ctxt->action;
    purc_variant_t pos  = ctxt->at;
    purc_variant_t template_data_type  = ctxt->template_data_type;
    purc_variant_t at_array = PURC_VARIANT_INVALID;
    size_t nr_array = 0;
    int ret = -1;

    at_array = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    if (!at_array) {
        goto out;
    }

    if (pos && purc_variant_is_string(pos)) {
        const char *s_pos = purc_variant_get_string_const(pos);
        size_t length = 0;

        const char *dest = pcutils_get_next_token(s_pos, " \t\n", &length);
        while (dest) {
            purc_variant_t new_at;
            if (dest[0] == '[' && dest[length - 1] == ']') {
                new_at = purc_variant_make_string_ex(dest + 1, length - 2, false);
            }
            else {
                new_at = purc_variant_make_string_ex(dest, length, false);
            }
            purc_variant_array_append(at_array, new_at);
            purc_variant_unref(new_at);

            dest = pcutils_get_next_token(dest + length, " \t\n", &length);
        }
        nr_array = purc_variant_array_get_size(at_array);
    }

    if (nr_array == 0 && pos) {
        purc_variant_array_append(at_array, pos);
    }

    /* FIXME: what if array of elements? */
    enum purc_variant_type type = purc_variant_get_type(on);
    if (type == PURC_VARIANT_TYPE_NATIVE) {
        if (pcdvobjs_is_elements(on)) {
            ret =  process_elem_coll(co, frame, on, src, at_array, ctxt->action,
                    attr_op_eval, template_data_type, ctxt->individually,
                    ctxt->wholly);
            goto out;
        }
    }
    else if (type == PURC_VARIANT_TYPE_STRING) {
        const char *s = purc_variant_get_string_const(on);

        purc_document_t doc = co->stack.doc;
        purc_variant_t elems;

        size_t op_len = 0;
        const char *op = pcutils_trim_spaces(s, &op_len);
        if (op && op[0] == '>') {
            purc_variant_t pos = pcintr_get_at_var(frame);
            pcdoc_element_t ancestor = pcdvobjs_get_element_from_elements(pos, 0);
            char *sel = strndup(op + 1, op_len - 1);
            elems = pcdvobjs_elem_coll_query(doc, ancestor, sel);
            free(sel);
        }
        else {
            elems = pcdvobjs_elem_coll_query(doc, NULL, s);
        }

        if (elems) {
            pcdoc_element_t elem;
            elem = pcdvobjs_get_element_from_elements(elems, 0);
            if (elem) {
                ret =  process_elem_coll(co, frame, elems, src, at_array,
                        ctxt->action, attr_op_eval, template_data_type,
                        ctxt->individually, ctxt->wholly);
            }
            purc_variant_unref(elems);
            goto out;
        }
    }

    ret = process_container(co, frame, on, src, at_array, action, attr_op_eval,
            template_data_type, ctxt->individually, ctxt->wholly);

out:
    if (ret == 0) {
        pcintr_set_question_var(frame, on);
    }
    else {
        int err = purc_get_last_error();
        if (frame->silently && pcinst_is_ignorable_error(err)) {
            purc_variant_t v = purc_variant_make_undefined();
            pcintr_set_question_var(frame, v);
            purc_variant_unref(v);

            purc_clr_error();
            ret = 0;
        }
    }

    if (at_array) {
        purc_variant_unref(at_array);
    }
    return ret;
}

static int
process_attr_via(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
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
process_attr_on(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
    if (ctxt->on != PURC_VARIANT_INVALID) {
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
    ctxt->on = val;
    purc_variant_ref(val);

    return 0;
}

static int
process_attr_to(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
    if (ctxt->to != PURC_VARIANT_INVALID) {
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
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }
    const char *s_action = purc_variant_get_string_const(val);
    if (strcmp(s_action, "displace")) {
        if (ctxt->attr_op != PCHVML_ATTRIBUTE_OPERATOR) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            return -1;
        }
    }

    ctxt->to = val;
    purc_variant_ref(val);

    return 0;
}

static int
process_attr_with(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val,
        struct pcvdom_attr *attr)
{
    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
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
    if (ctxt->from != PURC_VARIANT_INVALID) {
        if (attr->op != PCHVML_ATTRIBUTE_OPERATOR) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            return -1;
        }
    }

    ctxt->with = val;
    purc_variant_ref(val);

    ctxt->attr_op   = attr->op;
    ctxt->attr_op_eval = pcintr_attribute_get_op(attr->op);
    if (!ctxt->attr_op_eval)
        return -1;

    return 0;
}

static int
process_attr_from(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
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

    if (ctxt->with != PURC_VARIANT_INVALID) {
        if (!purc_variant_is_string(ctxt->with)) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            return -1;
        }
        if (ctxt->attr_op != PCHVML_ATTRIBUTE_OPERATOR) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            return -1;
        }
    }

    ctxt->from = val;
    purc_variant_ref(val);

    return 0;
}

static int
process_attr_at(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
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
attr_found_val(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val,
        struct pcvdom_attr *attr,
        void *ud)
{
    UNUSED_PARAM(ud);

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, WITH)) == name) {
        return process_attr_with(frame, element, name, val, attr);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, VIA)) == name) {
        return process_attr_via(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, ON)) == name) {
        return process_attr_on(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, TO)) == name) {
        return process_attr_to(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, FROM)) == name) {
        return process_attr_from(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, AT)) == name) {
        return process_attr_at(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, SILENTLY)) == name) {
        return 0;
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, INDIVIDUALLY)) == name) {
        struct ctxt_for_update *ctxt;
        ctxt = (struct ctxt_for_update*)frame->ctxt;
        ctxt->individually = true;
        return 0;
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, WHOLLY)) == name) {
        struct ctxt_for_update *ctxt;
        ctxt = (struct ctxt_for_update*)frame->ctxt;
        ctxt->wholly = true;
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

    struct ctxt_for_update *ctxt = frame->ctxt;
    if (!ctxt) {
        ctxt = (struct ctxt_for_update*)calloc(1, sizeof(*ctxt));
        if (!ctxt) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            return NULL;
        }
        ctxt->attr_op = PCHVML_ATTRIBUTE_OPERATOR;

        frame->ctxt = ctxt;
        frame->ctxt_destroy = ctxt_destroy;

        frame->pos = pos; // ATTENTION!!
    }

    if (0 != pcintr_stack_frame_eval_attr_and_content(stack, frame, false)) {
        return NULL;
    }

    if (pcintr_common_handle_attr_in(stack->co, frame)) {
        return NULL;
    }

    struct pcvdom_element *element = frame->pos;

    int r;
    r = pcintr_walk_attrs(frame, element, stack, attr_found_val);
    if (r)
        return ctxt;

    ctxt->action = UPDATE_ACTION_DISPLACE;
    if (ctxt->to != PURC_VARIANT_INVALID) {
        const char *s_action = purc_variant_get_string_const(ctxt->to);
        ctxt->action = to_operator(s_action);
        if (ctxt->action == UPDATE_ACTION_UNKNOWN) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            return ctxt;
        }
    }

    // +=, -=, *=, /=, %=, ~=, ^=, $= only support displace
    if (ctxt->attr_op != PCHVML_ATTRIBUTE_OPERATOR
            && ctxt->action != UPDATE_ACTION_DISPLACE) {
        purc_set_error(PURC_ERROR_NOT_SUPPORTED);
        return ctxt;
    }

    if (ctxt->on == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_ARGUMENT_MISSED,
                "lack of vdom attribute 'on' for element <%s>",
                element->tag_name);
        return ctxt;
    }

    purc_variant_t content = pcintr_get_symbol_var(frame, PURC_SYMBOL_VAR_CARET);
    if (content && !purc_variant_is_undefined(content)) {
        ctxt->literal = purc_variant_ref(content);
    }

    // FIXME
    // load from network
    purc_variant_t from = ctxt->from;
    if (from != PURC_VARIANT_INVALID && purc_variant_is_string(from)) {
        get_source_by_from(stack->co, frame, ctxt);
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

    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
    if (ctxt) {
        ctxt_for_update_destroy(ctxt);
        frame->ctxt = NULL;
    }

    return true;
}

static int
on_element(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_element *element)
{
    UNUSED_PARAM(frame);
    UNUSED_PARAM(element);

    pcintr_stack_t stack = &co->stack;

    if (stack->except)
        return 0;

    return 0;
}

static int
on_content(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_content *content)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);
    UNUSED_PARAM(content);
    return 0;
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
logic_process(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    pcintr_stack_t stack = &co->stack;

    if (stack->except)
        return 0;

    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;

    if (ctxt->from) {
        if (ctxt->from_result != PURC_VARIANT_INVALID) {
            PURC_VARIANT_SAFE_CLEAR(frame->ctnt_var);
            frame->ctnt_var = ctxt->from_result;
            purc_variant_ref(ctxt->from_result);

            return process(co, frame, ctxt->from_result, ctxt->attr_op_eval);
        }
    }
    if (!ctxt->from && ctxt->with) {
        purc_variant_t src;
        src = get_source_by_with(co, frame, ctxt->with);

        PURC_VARIANT_SAFE_CLEAR(frame->ctnt_var);
        frame->ctnt_var = src;
        purc_variant_ref(src);

        int r = process(co, frame, src, ctxt->attr_op_eval);
        purc_variant_unref(src);
        return r ? -1 : 0;
    }
    if (ctxt->literal != PURC_VARIANT_INVALID) {
        pcintr_attribute_op attr_op_eval;
        attr_op_eval = pcintr_attribute_get_op(PCHVML_ATTRIBUTE_OPERATOR);
        if (!attr_op_eval) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            return -1;
        }
        PURC_VARIANT_SAFE_CLEAR(frame->ctnt_var);
        frame->ctnt_var = ctxt->literal;
        purc_variant_ref(ctxt->literal);
        return process(co, frame, ctxt->literal, attr_op_eval);
    }
    if (ctxt->on && ctxt->action == UPDATE_ACTION_REMOVE) {
        return process(co, frame, PURC_VARIANT_INVALID, ctxt->attr_op_eval);
    }

    struct pcvdom_element *element = frame->pos;

    purc_set_error_with_info(PURC_ERROR_ARGUMENT_MISSED,
                "lack of vdom attribute 'with/from' for element <%s>",
                element->tag_name);

    return -1;
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

    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;

    bool is_first = false;
    struct pcvdom_node *curr;

again:
    curr = ctxt->curr;

    if (curr == NULL) {
        struct pcvdom_element *element = frame->pos;
        struct pcvdom_node *node = &element->node;
        node = pcvdom_node_first_child(node);
        curr = node;
        is_first = true;
        purc_clr_error();
    }
    else {
        curr = pcvdom_node_next_sibling(curr);
        purc_clr_error();
    }

    ctxt->curr = curr;

    if (is_first) {
        logic_process(co, frame);
    }

    if (curr == NULL) {
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

struct pcintr_element_ops* pcintr_get_update_ops(void)
{
    return &ops;
}

