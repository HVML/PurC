/**
 * @file request.c
 * @author Xue Shuming
 * @date 2022/08/05
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
#include "private/pcrdr.h"
#include "pcrdr/connect.h"
#include "purc-runloop.h"

#include "../ops.h"

#include <pthread.h>
#include <unistd.h>

#define EVENT_SEPARATOR          ':'
#define REQUEST_EVENT_HANDER     "_request_event_handler"

#define ARG_KEY_DATA_TYPE       "dataType"
#define ARG_KEY_DATA            "data"
#define ARG_KEY_ELEMENT         "element"
#define ARG_KEY_NAME            "name"

struct ctxt_for_request {
    struct pcvdom_node           *curr;

    purc_variant_t                on;
    purc_variant_t                to;
    purc_variant_t                as;
    purc_variant_t                at;
    purc_variant_t                with;

    unsigned int                  synchronously:1;
    unsigned int                  is_noreturn:1;
    unsigned int                  bound:1;
    purc_variant_t                request_id;
};

static void
ctxt_for_request_destroy(struct ctxt_for_request *ctxt)
{
    if (ctxt) {
        PURC_VARIANT_SAFE_CLEAR(ctxt->on);
        PURC_VARIANT_SAFE_CLEAR(ctxt->to);
        PURC_VARIANT_SAFE_CLEAR(ctxt->as);
        PURC_VARIANT_SAFE_CLEAR(ctxt->with);
        PURC_VARIANT_SAFE_CLEAR(ctxt->request_id);
        free(ctxt);
    }
}

static void
ctxt_destroy(void *ctxt)
{
    ctxt_for_request_destroy((struct ctxt_for_request*)ctxt);
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

    if (pcintr_request_id_is_match(observer->observed, msg->elementValue) ||
            purc_variant_is_equal_to(observer->observed, msg->elementValue)) {
        goto match_observed;
    }
    else {
        goto out;
    }

match_observed:
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(MSG, RESPONSE)) == type) {
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

    pcintr_stack_frame_t frame = (pcintr_stack_frame_t)data;

    purc_variant_t payload = msg->data;
    pcintr_set_question_var(frame, payload);

    pcintr_resume(cor, msg);
    pcintr_set_current_co(NULL);
    return 0;
}

static bool
is_css_selector(const char *s)
{
    if (s && (s[0] == '.' || s[0] == '#')) {
        return true;
    }
    return false;
}

static bool
is_rdr(purc_variant_t v)
{
    purc_variant_t rdr = purc_get_runner_variable(PURC_PREDEF_VARNAME_RDR);
    if (rdr && rdr == v) {
        return true;
    }
    return false;
}

static int
request_crtn_by_rid_cid(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        purc_atom_t dest_rid, purc_atom_t dest_cid, const char *token)
{
    int ret = 0;
    struct ctxt_for_request *ctxt = (struct ctxt_for_request*)frame->ctxt;

    const char *uri = purc_atom_to_string(co->cid);
    purc_variant_t source_uri = purc_variant_make_string(uri, false);
    const char *sub_type = purc_variant_get_string_const(ctxt->to);

    struct pcinst *inst = pcinst_current();
    dest_rid = (dest_rid == 0) ? inst->endpoint_atom : dest_rid;
    if (dest_cid && (inst->endpoint_atom == dest_rid)) {
        ctxt->request_id = purc_variant_make_ulongint(dest_cid);
        pcintr_post_event_by_ctype(0, dest_cid,
                PCRDR_MSG_EVENT_REDUCE_OPT_KEEP,
                source_uri,
                ctxt->request_id,
                MSG_TYPE_REQUEST,
                sub_type,
                ctxt->with,
                ctxt->request_id
                );
    }
    else {
        ctxt->request_id = pcintr_request_id_create(PCINTR_REQUEST_ID_TYPE_CRTN,
                dest_rid, dest_cid, token);
        pcintr_post_event_by_ctype(dest_rid, dest_cid,
                PCRDR_MSG_EVENT_REDUCE_OPT_KEEP,
                source_uri,
                ctxt->request_id,
                MSG_TYPE_REQUEST,
                sub_type,
                ctxt->with,
                ctxt->request_id
                );
    }

    purc_variant_unref(source_uri);

    if (ctxt->is_noreturn || !ctxt->synchronously) {
        goto out;
    }

    purc_variant_t observed = pcintr_request_id_create(
            PCINTR_REQUEST_ID_TYPE_CRTN, dest_rid, dest_cid, token);
    pcintr_yield(
            CO_STAGE_FIRST_RUN | CO_STAGE_OBSERVING,
            CO_STATE_STOPPED,
            observed,
            MSG_TYPE_RESPONSE,
            MSG_SUB_TYPE_ASTERISK,
            is_observer_match,
            observer_handle,
            frame,
            true
        );
    purc_variant_unref(observed);

out:
    return ret;
}

static int
request_chan_by_rid(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        const char *uri, purc_atom_t dest_rid, const char *chan)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);
    UNUSED_PARAM(uri);
    UNUSED_PARAM(dest_rid);
    UNUSED_PARAM(chan);

    int ret = -1;
    purc_variant_t source_uri = PURC_VARIANT_INVALID;
    struct ctxt_for_request *ctxt = (struct ctxt_for_request*)frame->ctxt;
    const char *s_to = purc_variant_get_string_const(ctxt->to);
    if (strcasecmp(s_to, CHAN_METHOD_POST) != 0) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "invalid channel operation '%s'", s_to);
        goto out;
    }

    source_uri = purc_variant_make_string(uri, false);
    ctxt->request_id = pcintr_request_id_create(PCINTR_REQUEST_ID_TYPE_CHAN,
            dest_rid, 0, chan);
    pcintr_post_event_by_ctype(dest_rid, 0,
            PCRDR_MSG_EVENT_REDUCE_OPT_KEEP,
            source_uri,
            ctxt->request_id,
            MSG_TYPE_REQUEST_CHAN,
            s_to,
            ctxt->with,
            ctxt->request_id
            );

out:
    if (source_uri) {
        purc_variant_unref(source_uri);
    }
    return ret;
}

static int
request_crtn_by_uri(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        const char *uri, char *host_name, char *app_name,
        char *runner_name, enum HVML_RUN_RES_TYPE res_type, char *res_name)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);
    UNUSED_PARAM(uri);
    UNUSED_PARAM(host_name);
    UNUSED_PARAM(app_name);
    UNUSED_PARAM(runner_name);
    UNUSED_PARAM(res_type);
    UNUSED_PARAM(res_name);

    int ret = -1;
    purc_atom_t dest_rid;
    struct pcinst *curr_inst = pcinst_current();

    if (strcmp(host_name, PCINTR_HVML_RUN_CURR_ID) != 0 &&
            strcmp(host_name, PCRDR_LOCALHOST) != 0) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "invalid host_name '%s' vs '%s'", host_name, PCRDR_LOCALHOST);
        goto out;
    }

    if (strcmp(app_name, PCINTR_HVML_RUN_CURR_ID) != 0 &&
            strcmp(app_name, curr_inst->app_name) != 0) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "invalid app_name '%s' vs '%s'", app_name, curr_inst->app_name);
        goto out;
    }

    if (strcmp(runner_name, PCINTR_HVML_RUN_CURR_ID) == 0 ||
            strcmp(runner_name, curr_inst->runner_name) == 0) {
        dest_rid = curr_inst->endpoint_atom;
    }
    else {
        char endpoint_name[PURC_LEN_ENDPOINT_NAME + 1];
        purc_assemble_endpoint_name_ex(PCRDR_LOCALHOST,
                curr_inst->app_name, runner_name,
                endpoint_name, sizeof(endpoint_name) - 1);
        dest_rid = purc_atom_try_string_ex(PURC_ATOM_BUCKET_DEF,
                endpoint_name);
    }

    if (res_type == HVML_RUN_RES_TYPE_CHAN) {
        ret = request_chan_by_rid(co, frame, uri, dest_rid, res_name);
        goto out;
    }
    else if (res_type == HVML_RUN_RES_TYPE_CRTN) {
        ret = request_crtn_by_rid_cid(co, frame, dest_rid, 0, res_name);
        goto out;
    }

    purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
    PC_WARN("not implemented on '%s' for request.\n", uri);
out:
    return ret;
}

static int
request_elements(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        const char *selector)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(selector);

    int ret = -1;
    struct ctxt_for_request *ctxt = (struct ctxt_for_request*)frame->ctxt;
    const char *s_on = purc_variant_get_string_const(ctxt->on);
    const char *s_to = purc_variant_get_string_const(ctxt->to);
    const char *request_id = ctxt->is_noreturn ? PCINTR_RDR_NORETURN_REQUEST_ID
        : NULL;

    if (!ctxt->synchronously) {
        purc_set_error_with_info(PURC_ERROR_NOT_IMPLEMENTED,
                "Not implement asynchronously request for $RDR");
        goto out;
    }

    purc_variant_t v = pcintr_rdr_call_method(&co->stack, request_id,
            s_on + 1, s_to, ctxt->with);
    if (!v && ctxt->is_noreturn) {
        v = purc_variant_make_null();
    }

    if (v) {
        pcintr_set_question_var(frame, v);
        purc_variant_unref(v);
    }

    ret = 0;
out:
    return ret;
}

static int
request_rdr(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        purc_variant_t rdr)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);
    UNUSED_PARAM(rdr);
    int ret = 0;

    pcrdr_msg *response_msg = NULL;
    pcrdr_msg_target target = PCRDR_MSG_TARGET_WORKSPACE;
    uint64_t target_value = co->target_workspace_handle;
    pcrdr_msg_element_type element_type = PCRDR_MSG_ELEMENT_TYPE_VOID;
    const char *element = NULL;
    pcrdr_msg_data_type data_type;

    struct pcinst* inst = pcinst_current();
    struct pcrdr_conn *conn = inst->conn_to_rdr;
    const char *operation = NULL;
    struct ctxt_for_request *ctxt = (struct ctxt_for_request*)frame->ctxt;
    const char *request_id = ctxt->is_noreturn ? PCINTR_RDR_NORETURN_REQUEST_ID
        : NULL;
    purc_variant_t data = PURC_VARIANT_INVALID;
    purc_variant_t arg = ctxt->with;
    const char *page_name = NULL;

    if (!arg) {
        purc_set_error_with_info(PURC_ERROR_ARGUMENT_MISSED,
                "Argument missed for request $RDR");
        goto out;
    }
    else if (!purc_variant_is_object(arg)) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "Invalid param type '%s' for $RDR", pcvariant_typename(ctxt->with));
        goto out;
    }

    data = purc_variant_object_get_by_ckey(arg, ARG_KEY_DATA);
    if (!data) {
        purc_set_error_with_info(PURC_ERROR_ARGUMENT_MISSED,
                "Argument missed for request $RDR");
        goto out;
    }
    else if (purc_variant_is_object(data)) {
        data_type = PCRDR_MSG_DATA_TYPE_JSON;
    }
    else if (purc_variant_is_string(data)) {
        data_type = PCRDR_MSG_DATA_TYPE_PLAIN;

        purc_variant_t dt;
        if ((dt = purc_variant_object_get_by_ckey(arg, ARG_KEY_DATA_TYPE))) {
            const char *tmp = purc_variant_get_string_const(dt);
            /* TODO: maybe we can optimize this with atom */
            if (strcasecmp(tmp, PCRDR_MSG_DATA_TYPE_NAME_HTML) == 0) {
                data_type = PCRDR_MSG_DATA_TYPE_HTML;
            }
            else if (strcasecmp(tmp, PCRDR_MSG_DATA_TYPE_NAME_XGML) == 0) {
                data_type = PCRDR_MSG_DATA_TYPE_XGML;
            }
            else if (strcasecmp(tmp, PCRDR_MSG_DATA_TYPE_NAME_SVG) == 0) {
                data_type = PCRDR_MSG_DATA_TYPE_SVG;
            }
            else if (strcasecmp(tmp, PCRDR_MSG_DATA_TYPE_NAME_MATHML) == 0) {
                data_type = PCRDR_MSG_DATA_TYPE_MATHML;
            }
            else if (strcasecmp(tmp, PCRDR_MSG_DATA_TYPE_NAME_XML) == 0) {
                data_type = PCRDR_MSG_DATA_TYPE_XML;
            }
        }
    }
    else {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "Invalid param type '%s' for $RDR", pcvariant_typename(data));
        goto out;
    }

    operation = purc_variant_get_string_const(ctxt->to);
    if (!operation[0]) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "Invalid method '%s' for $RDR", operation);
        goto out;
    }

    purc_atom_t method = purc_atom_try_string_ex(ATOM_BUCKET_HVML, operation);
    if (!method) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "Invalid method '%s' for $RDR", operation);
        goto out;
    }

    if (!ctxt->synchronously) {
        purc_set_error_with_info(PURC_ERROR_NOT_IMPLEMENTED,
                "Not implement asynchronously request for $RDR");
        goto out;
    }

    /* TODO: maybe we can shorten the name `PCHVML_KEYWORD_ENUM` */
    if ((pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, SETPAGEGROUPS)) == method) ||
            (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, ADDPAGEGROUPS)) == method)) {
        target = PCRDR_MSG_TARGET_WORKSPACE;
        target_value = inst->rdr_caps->session_handle;
    }
    else if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, CALLMETHOD)) == method) {
        target = PCRDR_MSG_TARGET_SESSION;
        purc_variant_t elem = purc_variant_object_get_by_ckey(arg, ARG_KEY_ELEMENT);
        if (!elem || !purc_variant_is_string(elem)) {
            purc_set_error_with_info(PURC_ERROR_ARGUMENT_MISSED,
                "Argument missed for request $RDR '%s'", operation);
            goto out;
        }
        element_type = PCRDR_MSG_ELEMENT_TYPE_ID;
        element = purc_variant_get_string_const(elem);
    }
    else if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, CREATEPLAINWINDOW)) == method) {
        target = PCRDR_MSG_TARGET_WORKSPACE;
        purc_variant_t n = purc_variant_object_get_by_ckey(data, ARG_KEY_NAME);
        if (n && purc_variant_is_string(n)) {
            page_name = purc_variant_get_string_const(n);
        }
        else {
            purc_clr_error();
        }
    }
    else if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, CREATEWIDGET)) == method) {
        target = PCRDR_MSG_TARGET_WORKSPACE;
        purc_variant_t elem = purc_variant_object_get_by_ckey(arg, ARG_KEY_ELEMENT);
        if (!elem || !purc_variant_is_string(elem)) {
            purc_set_error_with_info(PURC_ERROR_ARGUMENT_MISSED,
                "Argument missed for request $RDR '%s'", operation);
            goto out;
        }
        element_type = PCRDR_MSG_ELEMENT_TYPE_ID;
        element = purc_variant_get_string_const(elem);
        purc_variant_t n = purc_variant_object_get_by_ckey(data, ARG_KEY_NAME);
        if (n && purc_variant_is_string(n)) {
            page_name = purc_variant_get_string_const(n);
        }
        else {
            purc_clr_error();
        }
    }
    else {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "Invalid operation '%s' for $RDR", operation);
        goto out;
    }

    response_msg = pcintr_rdr_send_request_and_wait_response(conn, target,
            target_value, operation, request_id, element_type, element, NULL,
            data_type, data, 0);

    purc_variant_t v = PURC_VARIANT_INVALID;
    if (ctxt->is_noreturn) {
        v = purc_variant_make_null();
        ret = 0;
    }
    else if (response_msg == NULL) {
        goto out;
    }
    else {
        int ret_code = response_msg->retCode;
        PC_DEBUG("request $RDR ret_code=%d\n", ret_code);
        if (ret_code == PCRDR_SC_OK) {
            uint64_t page_handle = response_msg->resultValue;
            if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, CREATEPLAINWINDOW))
                    == method) {
                pcrdr_save_page_handle(inst->conn_to_rdr, NULL,
                        NULL, page_name, PCRDR_PAGE_TYPE_PLAINWIN,
                        page_handle, co->target_workspace_handle, 0);
            }
            else if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, CREATEWIDGET))
                    == method) {
                pcrdr_save_page_handle(inst->conn_to_rdr, NULL,
                        NULL, page_name, PCRDR_PAGE_TYPE_WIDGET,
                        page_handle, co->target_workspace_handle, 0);
            }

            if (response_msg->data) {
                v = purc_variant_ref(response_msg->data);
            }
            else {
                v = purc_variant_make_null();
            }
            ret = 0;
        }
        else {
            purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
        }
        pcrdr_release_message(response_msg);
    }

    if (v) {
        pcintr_set_question_var(frame, v);
        purc_variant_unref(v);
    }

out:
    return ret;
}

static int
post_process(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);

    int ret = 0;
    struct ctxt_for_request *ctxt = (struct ctxt_for_request*)frame->ctxt;
    purc_variant_t on = ctxt->on;
    purc_variant_t to = ctxt->to;
    purc_atom_t dest_cid = 0;
    if (!on || !to || !purc_variant_is_string(to)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        ret = -1;
        goto out;
    }

    if (purc_variant_is_ulongint(on)) {
        uint64_t u64;
        purc_variant_cast_to_ulongint(on, &u64, true);
        dest_cid = (purc_atom_t) u64;
        ret = request_crtn_by_rid_cid(co, frame, 0, dest_cid, NULL);
    }
    else if (pcintr_is_crtn_observed(on)) {
        dest_cid = pcintr_crtn_observed_get_cid(on);
        ret = request_crtn_by_rid_cid(co, frame, 0, dest_cid, NULL);
    }
    else if (purc_variant_is_string(on)) {
        const char *s_on = purc_variant_get_string_const(on);
        char host_name[PURC_LEN_HOST_NAME + 1];
        char app_name[PURC_LEN_APP_NAME + 1];
        char runner_name[PURC_LEN_RUNNER_NAME + 1];
        char res_name[PURC_LEN_IDENTIFIER + 1];
        enum HVML_RUN_RES_TYPE res_type = HVML_RUN_RES_TYPE_INVALID;

        if (is_css_selector(s_on)) {
            ret = request_elements(co, frame, s_on);
        }
        else if (pcintr_parse_hvml_run_uri(s_on, host_name, app_name,
                    runner_name, &res_type, res_name)) {
            ret = request_crtn_by_uri(co, frame, s_on, host_name, app_name,
                    runner_name, res_type, res_name);
        }
        else {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            ret = -1;
            PC_WARN("not implemented on '%s' for request.\n",
                    purc_variant_get_string_const(on));
        }
    }
    else if (is_rdr(on)) {
        ret = request_rdr(co, frame, on);
    }
    else {
        purc_set_error(PURC_ERROR_NOT_SUPPORTED);
        ret = -1;
        PC_WARN("not supported on with type '%s' for request.\n",
                pcvariant_typename(on));
        goto out;
    }

out:
    if (ret == 0 && ctxt->request_id && ctxt->as
            && !ctxt->synchronously && !ctxt->is_noreturn) {
        const char *name = purc_variant_get_string_const(ctxt->as);
        ret = pcintr_bind_named_variable(&co->stack,
                frame, name, ctxt->at, false, true, ctxt->request_id);
        if (ret == 0) {
            ctxt->bound = 1;
        }
    }
    return ret;
}

static int
process_attr_on(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_request *ctxt;
    ctxt = (struct ctxt_for_request*)frame->ctxt;
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
process_attr_to(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_request *ctxt;
    ctxt = (struct ctxt_for_request*)frame->ctxt;
    if (val == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> undefined",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    PURC_VARIANT_SAFE_CLEAR(ctxt->to);
    ctxt->to = purc_variant_ref(val);

    return 0;
}

static int
process_attr_as(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_request *ctxt;
    ctxt = (struct ctxt_for_request*)frame->ctxt;
    if (val == PURC_VARIANT_INVALID || !purc_variant_is_string(val)) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> undefined",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    PURC_VARIANT_SAFE_CLEAR(ctxt->as);
    ctxt->as = purc_variant_ref(val);

    return 0;
}

static int
process_attr_at(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_request *ctxt;
    ctxt = (struct ctxt_for_request*)frame->ctxt;
    if (val == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> undefined",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    PURC_VARIANT_SAFE_CLEAR(ctxt->at);
    ctxt->at = purc_variant_ref(val);

    return 0;
}


static int
process_attr_with(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_request *ctxt;
    ctxt = (struct ctxt_for_request*)frame->ctxt;
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
attr_found_val(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val,
        struct pcvdom_attr *attr,
        void *ud)
{
    UNUSED_PARAM(attr);
    UNUSED_PARAM(ud);

    struct ctxt_for_request *ctxt;
    ctxt = (struct ctxt_for_request*)frame->ctxt;

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, ON)) == name) {
        return process_attr_on(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, TO)) == name) {
        return process_attr_to(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, AS)) == name) {
        return process_attr_as(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, AT)) == name) {
        return process_attr_at(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, WITH)) == name) {
        return process_attr_with(frame, element, name, val);
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
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, NORETURN)) == name
            || pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, NO_RETURN)) == name) {
        ctxt->is_noreturn = 1;
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

    struct ctxt_for_request *ctxt = frame->ctxt;
    if (!ctxt) {
        ctxt = (struct ctxt_for_request*)calloc(1, sizeof(*ctxt));
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

    frame->attr_vars = purc_variant_make_object(0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    if (frame->attr_vars == PURC_VARIANT_INVALID)
        return ctxt;

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

    r = post_process(stack->co, frame);
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

    struct ctxt_for_request *ctxt;
    ctxt = (struct ctxt_for_request*)frame->ctxt;
    if (ctxt) {
        ctxt_for_request_destroy(ctxt);
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

    struct ctxt_for_request *ctxt;
    ctxt = (struct ctxt_for_request*)frame->ctxt;

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

struct pcintr_element_ops* pcintr_get_request_ops(void)
{
    return &ops;
}


