/*
 * @file rdr.c
 * @author XueShuming
 * @date 2022/03/09
 * @brief The impl of the interaction between interpreter and renderer.
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

#include "private/errors.h"
#include "private/instance.h"
#include "private/utils.h"
#include "private/variant.h"
#include "private/pcrdr.h"

#include <string.h>

#define ID_KEY                  "id"
#define TITLE_KEY               "title"
#define STYLE_KEY               "style"
#define LEVEL_KEY               "level"
#define CLASS_KEY               "class"

#define BUFF_MIN                1024
#define BUFF_MAX                1024 * 4
#define LEN_BUFF_LONGLONGINT    128

static inline
pcrdr_msg *make_request_msg(pcrdr_msg_target target, uint64_t target_value,
        const char *operation)
{
    return pcrdr_make_request_message(
            target,                             /* target */
            target_value,                       /* target_value */
            operation,                          /* operation */
            NULL,                               /* request_id */
            PCRDR_MSG_ELEMENT_TYPE_VOID,        /* element_type */
            NULL,                               /* element */
            NULL,                               /* property */
            PCRDR_MSG_DATA_TYPE_VOID,           /* data_tuype */
            NULL,                               /* data */
            0                                   /* data_len */
            );
}

static bool
object_set(purc_variant_t object, const char* key, const char* value)
{
    purc_variant_t k = purc_variant_make_string_static(key, false);
    if (k == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return false;
    }

    purc_variant_t v = purc_variant_make_string_static(value, false);
    if (k == PURC_VARIANT_INVALID) {
        purc_variant_unref(k);
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return false;
    }

    purc_variant_object_set(object, k, v);
    purc_variant_unref(k);
    purc_variant_unref(v);
    return true;
}

pcrdr_msg *pcintr_rdr_send_request(struct pcrdr_conn *conn,
        pcrdr_msg_target target, uint64_t target_value, const char *operation,
        pcrdr_msg_element_type element_type, const char *element,
        const char *property, pcrdr_msg_data_type data_type,
        purc_variant_t data)
{
    pcrdr_msg *response_msg = NULL;
    pcrdr_msg* msg = pcrdr_make_request_message(
            target,                             /* target */
            target_value,                       /* target_value */
            operation,                          /* operation */
            NULL,                               /* request_id */
            element_type,                       /* element_type */
            element,                            /* element */
            property,                           /* property */
            PCRDR_MSG_DATA_TYPE_VOID,           /* data_type */
            NULL,                               /* data */
            0                                   /* data_len */
            );
    if (msg == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    msg->dataType = data_type;
    msg->data = data;

    if (pcrdr_send_request_and_wait_response(conn,
            msg, PCRDR_TIME_DEF_EXPECTED, &response_msg) < 0) {
        goto failed;
    }
    pcrdr_release_message(msg);
    msg = NULL;

    return response_msg;

failed:
    if (msg) {
        pcrdr_release_message(msg);
    }

    return NULL;
}

uintptr_t create_target_workspace(
        struct pcrdr_conn *conn_to_rdr,
        uintptr_t session_handle,
        const char *target_workspace,
        purc_renderer_extra_info *extra_info
        )
{
    pcrdr_msg *msg = NULL;
    pcrdr_msg *response_msg = NULL;
    purc_variant_t req_data = PURC_VARIANT_INVALID;
    uintptr_t workspace_handle = 0;

    const char *operation = PCRDR_OPERATION_CREATEWORKSPACE;
    pcrdr_msg_target target = PCRDR_MSG_TARGET_SESSION;
    uint64_t target_value = session_handle;
    msg = make_request_msg(target, target_value, operation);
    if (msg == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    req_data = purc_variant_make_object(0, NULL, NULL);
    if (req_data == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    if (!object_set(req_data, ID_KEY, target_workspace)) {
        goto failed;
    }

    if (extra_info->workspace_title
            && !object_set(req_data, TITLE_KEY, extra_info->workspace_title)) {
        goto failed;
    }

    if (extra_info->workspace_classes
            && !object_set(req_data, CLASS_KEY, extra_info->workspace_classes)) {
        goto failed;
    }

    if (extra_info->workspace_styles
            && !object_set(req_data, CLASS_KEY, extra_info->workspace_styles)) {
        goto failed;
    }

    msg->dataType = PCRDR_MSG_DATA_TYPE_EJSON;
    msg->data = req_data;
    req_data = NULL;

    if (pcrdr_send_request_and_wait_response(conn_to_rdr,
            msg, PCRDR_TIME_DEF_EXPECTED, &response_msg) < 0) {
        goto failed;
    }
    pcrdr_release_message(msg);
    msg = NULL;

    int ret_code = response_msg->retCode;
    if (ret_code == PCRDR_SC_OK) {
        workspace_handle = response_msg->resultValue;
    }

    pcrdr_release_message(response_msg);

    if (ret_code != PCRDR_SC_OK) {
        purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
        goto failed;
    }

    return workspace_handle;

failed:
    if (req_data != PURC_VARIANT_INVALID) {
        purc_variant_unref(req_data);
    }

    if (msg) {
        pcrdr_release_message(msg);
    }


    return 0;
}


uintptr_t create_tabbed_window(
        struct pcrdr_conn *conn_to_rdr,
        uintptr_t workspace_handle,
        uintptr_t session_handle,
        const char *target_window,
        const char *target_level,
        purc_renderer_extra_info *extra_info
        )
{
    UNUSED_PARAM(target_level);

    pcrdr_msg *msg = NULL;
    pcrdr_msg *response_msg = NULL;
    purc_variant_t req_data = PURC_VARIANT_INVALID;
    uintptr_t window_handle = 0;

    const char *operation = PCRDR_OPERATION_CREATEPLAINWINDOW;
    pcrdr_msg_target target;
    uint64_t target_value;
    if (workspace_handle) {
        target = PCRDR_MSG_TARGET_WORKSPACE;
        target_value = workspace_handle;
    }
    else {
        target = PCRDR_MSG_TARGET_SESSION;
        target_value = session_handle;
    }

    msg = make_request_msg(target, target_value, operation);
    if (msg == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    req_data = purc_variant_make_object(0, NULL, NULL);
    if (req_data == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    if (!object_set(req_data, ID_KEY, target_window)) {
        goto failed;
    }

    if (extra_info->title
            && !object_set(req_data, TITLE_KEY, extra_info->title)) {
        goto failed;
    }

    if (extra_info->classes
            && !object_set(req_data, CLASS_KEY, extra_info->classes)) {
        goto failed;
    }

    if (extra_info->styles
            && !object_set(req_data, CLASS_KEY, extra_info->styles)) {
        goto failed;
    }

    msg->dataType = PCRDR_MSG_DATA_TYPE_EJSON;
    msg->data = req_data;
    req_data = NULL;

    if (pcrdr_send_request_and_wait_response(conn_to_rdr,
            msg, PCRDR_TIME_DEF_EXPECTED, &response_msg) < 0) {
        goto failed;
    }
    pcrdr_release_message(msg);
    msg = NULL;

    int ret_code = response_msg->retCode;
    if (ret_code == PCRDR_SC_OK) {
        window_handle = response_msg->resultValue;
    }

    pcrdr_release_message(response_msg);

    if (ret_code != PCRDR_SC_OK) {
        purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
        goto failed;
    }

    return window_handle;

failed:
    if (req_data != PURC_VARIANT_INVALID) {
        purc_variant_unref(req_data);
    }

    if (msg) {
        pcrdr_release_message(msg);
    }

    return 0;
}

uintptr_t create_tabpage(
        struct pcrdr_conn *conn_to_rdr,
        uintptr_t window_handle,
        const char *target_tabpage,
        purc_renderer_extra_info *extra_info
        )
{
    UNUSED_PARAM(extra_info);

    pcrdr_msg *msg = NULL;
    pcrdr_msg *response_msg = NULL;
    purc_variant_t req_data = PURC_VARIANT_INVALID;
    uintptr_t page_handle = 0;

    const char *operation = PCRDR_OPERATION_CREATETABPAGE;
    pcrdr_msg_target target = PCRDR_MSG_TARGET_TABBEDWINDOW;
    uint64_t target_value = window_handle;
    msg = make_request_msg(target, target_value, operation);
    if (msg == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    req_data = purc_variant_make_object(0, NULL, NULL);
    if (req_data == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    if (!object_set(req_data, ID_KEY, target_tabpage)) {
        goto failed;
    }

    if (extra_info->tabpage_title
            && !object_set(req_data, TITLE_KEY, extra_info->tabpage_title)) {
        goto failed;
    }

    msg->dataType = PCRDR_MSG_DATA_TYPE_EJSON;
    msg->data = req_data;
    req_data = NULL;

    if (pcrdr_send_request_and_wait_response(conn_to_rdr,
            msg, PCRDR_TIME_DEF_EXPECTED, &response_msg) < 0) {
        goto failed;
    }
    pcrdr_release_message(msg);
    msg = NULL;

    int ret_code = response_msg->retCode;
    if (ret_code == PCRDR_SC_OK) {
        page_handle = response_msg->resultValue;
    }

    pcrdr_release_message(response_msg);

    if (ret_code != PCRDR_SC_OK) {
        purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
        goto failed;
    }

    return page_handle;

failed:
    if (req_data != PURC_VARIANT_INVALID) {
        purc_variant_unref(req_data);
    }

    if (msg) {
        pcrdr_release_message(msg);
    }

    return 0;
}

uintptr_t create_plain_window(
        struct pcrdr_conn *conn_to_rdr,
        uintptr_t workspace_handle,
        uintptr_t session_handle,
        const char *target_window,
        const char *target_level,
        purc_renderer_extra_info *extra_info
        )
{
    UNUSED_PARAM(target_level);

    pcrdr_msg *msg = NULL;
    pcrdr_msg *response_msg = NULL;
    purc_variant_t req_data = PURC_VARIANT_INVALID;
    uintptr_t window_handle = 0;

    const char *operation = PCRDR_OPERATION_CREATEPLAINWINDOW;
    pcrdr_msg_target target;
    uint64_t target_value;
    if (workspace_handle) {
        target = PCRDR_MSG_TARGET_WORKSPACE;
        target_value = workspace_handle;
    }
    else {
        target = PCRDR_MSG_TARGET_SESSION;
        target_value = session_handle;
    }

    msg = make_request_msg(target, target_value, operation);
    if (msg == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    req_data = purc_variant_make_object(0, NULL, NULL);
    if (req_data == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    if (!object_set(req_data, ID_KEY, target_window)) {
        goto failed;
    }

    if (extra_info->title
            && !object_set(req_data, TITLE_KEY, extra_info->title)) {
        goto failed;
    }

    if (extra_info->classes
            && !object_set(req_data, CLASS_KEY, extra_info->classes)) {
        goto failed;
    }

    if (extra_info->styles
            && !object_set(req_data, CLASS_KEY, extra_info->styles)) {
        goto failed;
    }

    msg->dataType = PCRDR_MSG_DATA_TYPE_EJSON;
    msg->data = req_data;
    req_data = NULL;

    if (pcrdr_send_request_and_wait_response(conn_to_rdr,
            msg, PCRDR_TIME_DEF_EXPECTED, &response_msg) < 0) {
        goto failed;
    }
    pcrdr_release_message(msg);
    msg = NULL;

    int ret_code = response_msg->retCode;
    if (ret_code == PCRDR_SC_OK) {
        window_handle = response_msg->resultValue;
    }

    pcrdr_release_message(response_msg);

    if (ret_code != PCRDR_SC_OK) {
        purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
        goto failed;
    }

    return window_handle;

failed:
    if (req_data != PURC_VARIANT_INVALID) {
        purc_variant_unref(req_data);
    }

    if (msg) {
        pcrdr_release_message(msg);
    }

    return 0;
}

bool
purc_attach_vdom_to_renderer(purc_vdom_t vdom,
        const char *target_workspace,
        const char *target_window,
        const char *target_tabpage,
        const char *target_level,
        purc_renderer_extra_info *extra_info)
{
    if (!vdom) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return false;
    }

    struct pcinst *inst = pcinst_current();
    if (inst == NULL || inst->rdr_caps == NULL || target_window == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return false;
    }

    struct pcrdr_conn *conn_to_rdr = inst->conn_to_rdr;
    struct renderer_capabilities *rdr_caps = inst->rdr_caps;
    uintptr_t session_handle = rdr_caps->session_handle;

    uintptr_t workspace = 0;
    if (target_workspace && rdr_caps->workspace != 0) {
        workspace = create_target_workspace(
                conn_to_rdr,
                session_handle,
                target_workspace,
                extra_info);
        if (!workspace) {
            purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
            return false;
        }
    }

    uintptr_t window = 0;
    uintptr_t tabpage = 0;
    if (target_tabpage) {
        window = create_tabbed_window(
                conn_to_rdr,
                workspace,
                session_handle,
                target_window, target_level, extra_info);
        if (!window) {
            purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
            return false;
        }

        tabpage = create_tabpage(conn_to_rdr, window,
                target_tabpage, extra_info);
        if (!tabpage) {
            purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
            return false;
        }
    }
    else {
        window = create_plain_window(conn_to_rdr, workspace, session_handle,
                target_window, target_level, extra_info);
        if (!window) {
            purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
            return false;
        }
    }

    pcvdom_document_set_target_workspace(vdom, workspace);
    pcvdom_document_set_target_window(vdom, window);
    pcvdom_document_set_target_tabpage(vdom, tabpage);

    return true;
}


bool
pcintr_rdr_page_control_load(pcintr_stack_t stack)
{
    if (!pcvdom_document_is_attached_rdr(stack->vdom)) {
        return true;
    }

    pcrdr_msg *msg = NULL;
    pcrdr_msg *response_msg = NULL;
    purc_variant_t req_data = PURC_VARIANT_INVALID;

    purc_vdom_t vdom = stack->vdom;
    pchtml_html_document_t *doc = stack->doc;
    int opt = 0;
    purc_rwstream_t out = NULL;


    const char *operation = PCRDR_OPERATION_LOAD;
    pcrdr_msg_target target = PCRDR_MSG_TARGET_TABPAGE;
    uint64_t target_value = pcvdom_document_get_target_tabpage(vdom);
    if (target_value == 0) {
        target = PCRDR_MSG_TARGET_PLAINWINDOW;
        target_value = pcvdom_document_get_target_window(vdom);
    }

    msg = make_request_msg(target, target_value, operation);
    if (msg == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    out = purc_rwstream_new_buffer(BUFF_MIN, BUFF_MAX);
    if (out == NULL) {
        goto failed;
    }

    opt |= PCHTML_HTML_SERIALIZE_OPT_UNDEF;
    opt |= PCHTML_HTML_SERIALIZE_OPT_SKIP_WS_NODES;
    opt |= PCHTML_HTML_SERIALIZE_OPT_WITHOUT_TEXT_INDENT;
    opt |= PCHTML_HTML_SERIALIZE_OPT_FULL_DOCTYPE;
    opt |= PCHTML_HTML_SERIALIZE_OPT_WITH_HVML_HANDLE;

    if(0 != pchtml_doc_write_to_stream_ex(doc, opt, out)) {
        goto failed;
    }

    size_t sz_content = 0;
    size_t sz_buff = 0;
    char* p = (char*)purc_rwstream_get_mem_buffer_ex(out, &sz_content,
            &sz_buff,true);
    req_data = purc_variant_make_string_reuse_buff(p, sz_content, false);
    if (req_data == PURC_VARIANT_INVALID) {
        free(p);
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    msg->dataType = PCRDR_MSG_DATA_TYPE_TEXT;
    msg->data = req_data;
    req_data = NULL;

    struct pcinst *inst = pcinst_current();
    struct pcrdr_conn *conn_to_rdr = inst->conn_to_rdr;
    if (pcrdr_send_request_and_wait_response(conn_to_rdr,
            msg, PCRDR_TIME_DEF_EXPECTED, &response_msg) < 0) {
        goto failed;
    }
    pcrdr_release_message(msg);
    msg = NULL;

    int ret_code = response_msg->retCode;
    if (ret_code == PCRDR_SC_OK) {
        pcvdom_document_set_target_dom(vdom, response_msg->resultValue);
    }

    pcrdr_release_message(response_msg);
    purc_rwstream_destroy(out);
    out = NULL;

    if (ret_code != PCRDR_SC_OK) {
        purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
        goto failed;
    }

    return true;

failed:
    if (out) {
        purc_rwstream_destroy(out);
    }

    if (req_data != PURC_VARIANT_INVALID) {
        purc_variant_unref(req_data);
    }

    if (msg) {
        pcrdr_release_message(msg);
    }

    return false;
}

bool
pcintr_rdr_send_dom_request(pcintr_stack_t stack, const char *operation,
        pcdom_element_t *element, const char* property,
        pcrdr_msg_data_type data_type, purc_variant_t data)
{
    if (!stack || !pcvdom_document_is_attached_rdr(stack->vdom)
            || stack->stage != STACK_STAGE_EVENT_LOOP) {
        return true;
    }

    pcrdr_msg *response_msg = NULL;
    purc_variant_t req_data = PURC_VARIANT_INVALID;

    pcrdr_msg_target target = PCRDR_MSG_TARGET_DOM;
    uint64_t target_value = pcvdom_document_get_target_dom(stack->vdom);
    pcrdr_msg_element_type element_type = PCRDR_MSG_ELEMENT_TYPE_HANDLE;

    char elem[LEN_BUFF_LONGLONGINT];
    int n = snprintf(elem, sizeof(elem),
            "%llx", (unsigned long long int)(uintptr_t)element);
    if (n < 0) {
        purc_set_error(PURC_ERROR_BAD_STDC_CALL);
        goto failed;
    }
    else if ((size_t)n >= sizeof (elem)) {
        PC_DEBUG ("Too small elemer to serialize message.\n");
        purc_set_error(PURC_ERROR_TOO_SMALL_BUFF);
        goto failed;
    }

    struct pcinst *inst = pcinst_current();
    response_msg = pcintr_rdr_send_request(inst->conn_to_rdr,
        target, target_value, operation, element_type, elem,
        property, data_type, data);

    if (response_msg == NULL) {
        goto failed;
    }

    int ret_code = response_msg->retCode;
    if (ret_code != PCRDR_SC_OK) {
        purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
        goto failed;
    }

    pcrdr_release_message(response_msg);
    return true;

failed:
    if (req_data != PURC_VARIANT_INVALID) {
        purc_variant_unref(req_data);
    }

    return false;
}

bool
pcintr_rdr_send_dom_request_ex(pcintr_stack_t stack, const char *operation,
        pcdom_element_t *element, const char* property,
        pcrdr_msg_data_type data_type, const char* data)
{
    if (!stack || !pcvdom_document_is_attached_rdr(stack->vdom)
            || stack->stage != STACK_STAGE_EVENT_LOOP) {
        return true;
    }

    purc_variant_t req_data = PURC_VARIANT_INVALID;
    if (data_type == PCRDR_MSG_DATA_TYPE_TEXT) {
        req_data = purc_variant_make_string(data, false);
        if (req_data == PURC_VARIANT_INVALID) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }
    }
    else if (data_type == PCRDR_MSG_DATA_TYPE_EJSON) {
        req_data = purc_variant_make_from_json_string(data, strlen(data));
        if (req_data == PURC_VARIANT_INVALID) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }
    }

    return pcintr_rdr_send_dom_request(stack, operation, element,
            property, data_type, req_data);

failed:
    if (req_data != PURC_VARIANT_INVALID) {
        purc_variant_unref(req_data);
    }
    return false;
}

static purc_variant_t
serialize_node(pcdom_node_t *node)
{
    purc_rwstream_t out = purc_rwstream_new_buffer(BUFF_MIN, BUFF_MAX);
    if (out == NULL) {
        goto failed;
    }

    int opt = 0;
    opt |= PCHTML_HTML_SERIALIZE_OPT_UNDEF;
    opt |= PCHTML_HTML_SERIALIZE_OPT_SKIP_WS_NODES;
    opt |= PCHTML_HTML_SERIALIZE_OPT_WITHOUT_TEXT_INDENT;
    opt |= PCHTML_HTML_SERIALIZE_OPT_FULL_DOCTYPE;
    opt |= PCHTML_HTML_SERIALIZE_OPT_WITH_HVML_HANDLE;

    if(0 != pcdom_node_write_to_stream_ex(node, opt, out)) {
        goto failed;
    }

    size_t sz_content = 0;
    size_t sz_buff = 0;
    char* p = (char*)purc_rwstream_get_mem_buffer_ex(out, &sz_content,
            &sz_buff,true);
    purc_variant_t v = purc_variant_make_string_reuse_buff(p,
            sz_content, false);
    if (v == PURC_VARIANT_INVALID) {
        free(p);
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    return v;

failed:
    return PURC_VARIANT_INVALID;
}

bool
pcintr_rdr_dom_append_child(pcintr_stack_t stack, pcdom_element_t *element,
        pcdom_node_t *child)
{
    if (!stack || !pcvdom_document_is_attached_rdr(stack->vdom)
            || stack->stage != STACK_STAGE_EVENT_LOOP) {
        return true;
    }
    purc_variant_t data = serialize_node(child);
    if (data == PURC_VARIANT_INVALID) {
        return false;
    }

    return pcintr_rdr_send_dom_request(stack, PCRDR_OPERATION_APPEND,
            element, NULL, PCRDR_MSG_DATA_TYPE_TEXT, data);
}

bool
pcintr_rdr_dom_displace_child(pcintr_stack_t stack, pcdom_element_t *element,
        pcdom_node_t *child)
{
    if (!stack || !pcvdom_document_is_attached_rdr(stack->vdom)
            || stack->stage != STACK_STAGE_EVENT_LOOP) {
        return true;
    }

    purc_variant_t data = serialize_node(child);
    if (data == PURC_VARIANT_INVALID) {
        return false;
    }

    return pcintr_rdr_send_dom_request(stack, PCRDR_OPERATION_DISPLACE,
            element, NULL, PCRDR_MSG_DATA_TYPE_TEXT, data);
}

