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
#define NAME_KEY                "name"
#define TITLE_KEY               "title"
#define CLASS_KEY               "class"
#define LAYOUT_STYLE_KEY        "layoutStyle"
#define TOOLKIT_STYLE_KEY       "toolkitStyle"

#define BUFF_MIN                1024
#define BUFF_MAX                1024 * 1024 * 4
#define LEN_BUFF_LONGLONGINT    128

#define DEF_LEN_ONE_WRITE       1024 * 10

#define RDR_KEY_METHOD          "method"
#define RDR_KEY_ARG             "arg"

static struct pcintr_rdr_data_type {
    const char *type_name;
    pcrdr_msg_data_type type;
} pcintr_rdr_data_types[] = {
    { PCRDR_MSG_DATA_TYPE_NAME_VOID, PCRDR_MSG_DATA_TYPE_VOID },
    { PCRDR_MSG_DATA_TYPE_NAME_JSON, PCRDR_MSG_DATA_TYPE_JSON },
    { PCRDR_MSG_DATA_TYPE_NAME_PLAIN, PCRDR_MSG_DATA_TYPE_PLAIN },
    { PCRDR_MSG_DATA_TYPE_NAME_HTML, PCRDR_MSG_DATA_TYPE_HTML },
    { PCRDR_MSG_DATA_TYPE_NAME_SVG, PCRDR_MSG_DATA_TYPE_SVG },
    { PCRDR_MSG_DATA_TYPE_NAME_MATHML, PCRDR_MSG_DATA_TYPE_MATHML },
    { PCRDR_MSG_DATA_TYPE_NAME_XGML, PCRDR_MSG_DATA_TYPE_XGML },
    { PCRDR_MSG_DATA_TYPE_NAME_XML, PCRDR_MSG_DATA_TYPE_XML },
};

/* Make sure the size of doc_types matches the number of document types */
#define _COMPILE_TIME_ASSERT(name, x)               \
       typedef int _dummy_ ## name[(x) * 2 - 1]

_COMPILE_TIME_ASSERT(types,
        PCA_TABLESIZE(pcintr_rdr_data_types) == PCRDR_MSG_DATA_TYPE_NR);

#undef _COMPILE_TIME_ASSERT

pcrdr_msg_data_type
pcintr_rdr_retrieve_data_type(const char *type_name)
{
    if (UNLIKELY(type_name == NULL)) {
        goto fallback;
    }

    for (size_t i = 0; i < PCA_TABLESIZE(pcintr_rdr_data_types); i++) {
        if (strcmp(type_name, pcintr_rdr_data_types[i].type_name) == 0) {
            return pcintr_rdr_data_types[i].type;
        }
    }

fallback:
    return PCRDR_MSG_DATA_TYPE_VOID;   // fallback
}

static bool
object_set(purc_variant_t object, const char *key, const char *value)
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

pcrdr_msg *pcintr_rdr_send_request_and_wait_response(struct pcrdr_conn *conn,
        pcrdr_msg_target target, uint64_t target_value, const char *operation,
        const char *request_id, pcrdr_msg_element_type element_type,
        const char *element, const char *property,
        pcrdr_msg_data_type data_type, purc_variant_t data, size_t data_len)
{
    pcrdr_msg *response_msg = NULL;
    pcrdr_msg *msg = pcrdr_make_request_message(
            target,                             /* target */
            target_value,                       /* target_value */
            operation,                          /* operation */
            request_id,                         /* request_id */
            NULL,                               /* source_uri */
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
    if (data_len > 0) {
        msg->textLen = data_len;
    }

    if (request_id && strcmp(request_id, PCINTR_RDR_NORETURN_REQUEST_ID) == 0) {
        pcrdr_send_request(conn, msg, PCRDR_TIME_DEF_EXPECTED, NULL, NULL);
    }
    else {
        pcrdr_send_request_and_wait_response(conn,
                msg, PCRDR_TIME_DEF_EXPECTED, &response_msg);
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

uint64_t pcintr_rdr_create_workspace(struct pcrdr_conn *conn,
        uint64_t session, const char *name, const char *title)
{
    uint64_t workspace = 0;
    pcrdr_msg *response_msg = NULL;

    const char *operation = PCRDR_OPERATION_CREATEWORKSPACE;
    pcrdr_msg_target target = PCRDR_MSG_TARGET_SESSION;
    uint64_t target_value = session;
    pcrdr_msg_element_type element_type = PCRDR_MSG_ELEMENT_TYPE_VOID;
    pcrdr_msg_data_type data_type = PCRDR_MSG_DATA_TYPE_JSON;
    purc_variant_t data = PURC_VARIANT_INVALID;

    data = purc_variant_make_object(0, NULL, NULL);
    if (data == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    if (!object_set(data, NAME_KEY, name)) {
        goto failed;
    }

    if (title && !object_set(data, TITLE_KEY, title)) {
        goto failed;
    }

    response_msg = pcintr_rdr_send_request_and_wait_response(conn, target,
            target_value, operation, NULL, element_type, NULL, NULL, data_type,
            data, 0);

    if (response_msg == NULL) {
        goto failed;
    }

    int ret_code = response_msg->retCode;
    if (ret_code == PCRDR_SC_OK) {
        workspace = response_msg->resultValue;
    }

    pcrdr_release_message(response_msg);

    if (ret_code != PCRDR_SC_OK) {
        purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
        goto failed;
    }

    return workspace;

failed:
    if (data != PURC_VARIANT_INVALID) {
        purc_variant_unref(data);
    }

    return 0;
}

bool pcintr_rdr_destroy_workspace(struct pcrdr_conn *conn,
        uint64_t session, uint64_t workspace)
{
    pcrdr_msg *response_msg = NULL;

    const char *operation = PCRDR_OPERATION_DESTROYWORKSPACE;
    pcrdr_msg_target target = PCRDR_MSG_TARGET_SESSION;
    uint64_t target_value = session;
    pcrdr_msg_element_type element_type = PCRDR_MSG_ELEMENT_TYPE_HANDLE;
    pcrdr_msg_data_type data_type = PCRDR_MSG_DATA_TYPE_VOID;
    purc_variant_t data = PURC_VARIANT_INVALID;

    char element[LEN_BUFF_LONGLONGINT];
    int n = snprintf(element, sizeof(element),
            "%llx", (unsigned long long int)workspace);
    if (n < 0) {
        purc_set_error(PURC_ERROR_BAD_STDC_CALL);
        goto failed;
    }
    else if ((size_t)n >= sizeof (element)) {
        PC_DEBUG ("Too small elementer to serialize message.\n");
        purc_set_error(PURC_ERROR_TOO_SMALL_BUFF);
        goto failed;
    }

    response_msg = pcintr_rdr_send_request_and_wait_response(conn, target,
            target_value, operation, NULL, element_type, element, NULL,
            data_type, data, 0);

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
    if (data != PURC_VARIANT_INVALID) {
        purc_variant_unref(data);
    }

    if (response_msg) {
        pcrdr_release_message(response_msg);
    }

    return false;
}

bool pcintr_rdr_update_workspace(struct pcrdr_conn *conn,
        uint64_t session, uint64_t workspace,
        const char *property, const char *value)
{
    pcrdr_msg *response_msg = NULL;

    const char *operation = PCRDR_OPERATION_UPDATEWORKSPACE;
    pcrdr_msg_target target = PCRDR_MSG_TARGET_SESSION;
    uint64_t target_value = session;
    pcrdr_msg_element_type element_type = PCRDR_MSG_ELEMENT_TYPE_HANDLE;
    pcrdr_msg_data_type data_type = PCRDR_MSG_DATA_TYPE_PLAIN;
    purc_variant_t data = PURC_VARIANT_INVALID;

    data = purc_variant_make_string(value, false);
    if (data == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    char element[LEN_BUFF_LONGLONGINT];
    int n = snprintf(element, sizeof(element),
            "%llx", (unsigned long long int)workspace);
    if (n < 0) {
        purc_set_error(PURC_ERROR_BAD_STDC_CALL);
        goto failed;
    }
    else if ((size_t)n >= sizeof (element)) {
        PC_DEBUG ("Too small elementer to serialize message.\n");
        purc_set_error(PURC_ERROR_TOO_SMALL_BUFF);
        goto failed;
    }

    response_msg = pcintr_rdr_send_request_and_wait_response(conn, target,
            target_value, operation, NULL, element_type, element, property,
            data_type, data, 0);

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
    if (data != PURC_VARIANT_INVALID) {
        purc_variant_unref(data);
    }

    if (response_msg) {
        pcrdr_release_message(response_msg);
    }

    return false;
}

uint64_t pcintr_rdr_start_session(struct pcrdr_conn *conn, const char *protocol,
        uint64_t protocol_version, const char *host_name, const char *app_name,
        const char *runner_name)
{
    uint64_t handle = 0;
    pcrdr_msg *response_msg = NULL;

    const char *operation = PCRDR_OPERATION_STARTSESSION;
    pcrdr_msg_target target = PCRDR_MSG_TARGET_SESSION;
    uint64_t target_value = 0;
    pcrdr_msg_element_type element_type = PCRDR_MSG_ELEMENT_TYPE_VOID;
    pcrdr_msg_data_type data_type = PCRDR_MSG_DATA_TYPE_JSON;
    purc_variant_t data = PURC_VARIANT_INVALID;


    purc_variant_t vs[10] = { NULL };
    vs[0] = purc_variant_make_string_static("protocolName", false);
    vs[1] = purc_variant_make_string_static(protocol, false);
    vs[2] = purc_variant_make_string_static("protocolVersion", false);
    vs[3] = purc_variant_make_ulongint(protocol_version);
    vs[4] = purc_variant_make_string_static("hostName", false);
    vs[5] = purc_variant_make_string_static(host_name, false);
    vs[6] = purc_variant_make_string_static("appName", false);
    vs[7] = purc_variant_make_string_static(app_name, false);
    vs[8] = purc_variant_make_string_static("runnerName", false);
    vs[9] = purc_variant_make_string_static(runner_name, false);

    data = purc_variant_make_object(0, NULL, NULL);
    if (data == PURC_VARIANT_INVALID || vs[9] == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }
    for (int i = 0; i < 5; i++) {
        purc_variant_object_set(data, vs[i * 2], vs[i * 2 + 1]);
        purc_variant_unref(vs[i * 2]);
        purc_variant_unref(vs[i * 2 + 1]);
    }

    response_msg = pcintr_rdr_send_request_and_wait_response(conn, target,
            target_value, operation, NULL, element_type, NULL, NULL, data_type,
            data, 0);

    if (response_msg == NULL) {
        goto out;
    }

    int ret_code = response_msg->retCode;
    if (ret_code == PCRDR_SC_OK) {
        handle = response_msg->resultValue;
    }

    pcrdr_release_message(response_msg);

    if (ret_code != PCRDR_SC_OK) {
        purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
        goto out;
    }

out:
    if (data != PURC_VARIANT_INVALID) {
        purc_variant_unref(data);
    }

    return handle;
}

uint64_t pcintr_rdr_retrieve_workspace(struct pcrdr_conn *conn,
        uint64_t session, const char *workspace_name)
{
    uint64_t handle = 0;
    pcrdr_msg *response_msg = NULL;

    response_msg = pcintr_rdr_send_request_and_wait_response(conn,
            PCRDR_MSG_TARGET_SESSION, session, PCRDR_OPERATION_GETPROPERTY,
            NULL, PCRDR_MSG_ELEMENT_TYPE_VOID, NULL,
            "workspaceList",
            PCRDR_MSG_DATA_TYPE_VOID, PURC_VARIANT_INVALID, 0);
    if (response_msg == NULL) {
        goto done;
    }

    /* the retunred data will be like
     *  {
     *      "workspace0": { "handle": "895677", "layouter": true, "active": false },
     *      "workspace1": { "handle": "875643", "layouter": false, "active": true }
     *  }
     */
    if (response_msg->retCode != PCRDR_SC_OK) {
        purc_log_error("Failed request: %s  (%d)\n",
                PCRDR_OPERATION_GETPROPERTY, response_msg->retCode);
        purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
        goto done;
    }

    if (response_msg->dataType == PCRDR_MSG_DATA_TYPE_JSON &&
            purc_variant_is_object(response_msg->data)) {
        purc_variant_t tmp;
        tmp = purc_variant_object_get_by_ckey(response_msg->data,
                workspace_name);
        if (tmp && purc_variant_is_object(tmp)) {
            tmp = purc_variant_object_get_by_ckey(tmp, "handle");
            const char *str = purc_variant_get_string_const(tmp);
            if (str) {
                handle = strtoull(str, NULL, 16);
            }
        }
    }

done:
    if (response_msg) {
        pcrdr_release_message(response_msg);
    }

    return handle;
}

uint64_t
pcintr_rdr_create_page(struct pcrdr_conn *conn, uint64_t workspace,
        pcrdr_page_type_k page_type, const char *target_group,
        const char *page_name, const char *title, const char *classes,
        const char *layout_style, purc_variant_t toolkit_style)
{
    uint64_t page_handle = 0;
    pcrdr_msg *response_msg = NULL;

    const char *operation;
    if (page_type == PCRDR_PAGE_TYPE_PLAINWIN) {
        operation = PCRDR_OPERATION_CREATEPLAINWINDOW;
    }
    else {
        if (target_group == NULL) {
            purc_log_error("NO target group specified when creating widget!\n");
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            return page_handle;
        }
        operation = PCRDR_OPERATION_CREATEWIDGET;
    }

    pcrdr_msg_element_type element_type;
    const char *element = NULL;
    if (target_group) {
        element_type = PCRDR_MSG_ELEMENT_TYPE_ID;
        element = target_group;
    }
    else {
        element_type = PCRDR_MSG_ELEMENT_TYPE_VOID;
    }

    pcrdr_msg_data_type data_type = PCRDR_MSG_DATA_TYPE_JSON;
    purc_variant_t data = PURC_VARIANT_INVALID;
    pcrdr_msg_target target = PCRDR_MSG_TARGET_WORKSPACE;
    uint64_t target_value = workspace;

    data = purc_variant_make_object_0();
    if (data == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    if (!object_set(data, NAME_KEY, page_name)) {
        goto failed;
    }

    if (title && !object_set(data, TITLE_KEY, title)) {
        goto failed;
    }

    if (classes && !object_set(data, CLASS_KEY, classes)) {
        goto failed;
    }

    if (layout_style && !object_set(data, LAYOUT_STYLE_KEY, layout_style)) {
        goto failed;
    }

    if (toolkit_style) {
        if (!purc_variant_object_set_by_static_ckey(data,
                TOOLKIT_STYLE_KEY, toolkit_style))
            goto failed;
    }

    response_msg = pcintr_rdr_send_request_and_wait_response(conn, target,
            target_value, operation, NULL, element_type, element, NULL, data_type,
            data, 0);
    if (response_msg == NULL) {
        // pcintr_rdr_send_request_and_wait_response unref data
        goto failed;
    }

    if (response_msg->retCode == PCRDR_SC_OK) {
        page_handle = response_msg->resultValue;
    }
    else {
        purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
        purc_log_error("Failed request: %s (%d)\n",
                operation, response_msg->retCode);
    }

    pcrdr_release_message(response_msg);
    return page_handle;

failed:
    if (data != PURC_VARIANT_INVALID) {
        purc_variant_unref(data);
    }

    return 0;
}

bool pcintr_rdr_destroy_page(struct pcrdr_conn *conn, uint64_t workspace,
        pcrdr_page_type_k page_type, uint64_t plain_window)
{
    pcrdr_msg *response_msg = NULL;

    const char *operation;
    if (page_type == PCRDR_PAGE_TYPE_PLAINWIN)
        operation = PCRDR_OPERATION_DESTROYPLAINWINDOW;
    else
        operation = PCRDR_OPERATION_DESTROYWIDGET;
    pcrdr_msg_target target;
    uint64_t target_value;
    pcrdr_msg_element_type element_type = PCRDR_MSG_ELEMENT_TYPE_HANDLE;
    pcrdr_msg_data_type data_type = PCRDR_MSG_DATA_TYPE_VOID;
    purc_variant_t data = PURC_VARIANT_INVALID;

    target = PCRDR_MSG_TARGET_WORKSPACE;
    target_value = workspace;

    char element[LEN_BUFF_LONGLONGINT];
    int n = snprintf(element, sizeof(element),
            "%llx", (unsigned long long int)plain_window);
    if (n < 0) {
        purc_set_error(PURC_ERROR_BAD_STDC_CALL);
        goto failed;
    }
    else if ((size_t)n >= sizeof (element)) {
        PC_DEBUG ("Too small elementer to serialize message.\n");
        purc_set_error(PURC_ERROR_TOO_SMALL_BUFF);
        goto failed;
    }

    response_msg = pcintr_rdr_send_request_and_wait_response(conn, target,
            target_value, operation, NULL, element_type, element, NULL,
            data_type, data, 0);

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
    if (data != PURC_VARIANT_INVALID) {
        purc_variant_unref(data);
    }

    if (response_msg) {
        pcrdr_release_message(response_msg);
    }

    return false;
}

// property: title, class, style
bool
pcintr_rdr_update_page(struct pcrdr_conn *conn, uint64_t workspace,
        pcrdr_page_type_k page_type, uint64_t plain_window,
        const char *property, purc_variant_t value)
{
    pcrdr_msg *response_msg = NULL;

    const char *operation;
    if (page_type == PCRDR_PAGE_TYPE_PLAINWIN)
        operation = PCRDR_OPERATION_UPDATEPLAINWINDOW;
    else
        operation = PCRDR_OPERATION_UPDATEWIDGET;
    pcrdr_msg_target target;
    uint64_t target_value;
    pcrdr_msg_element_type element_type = PCRDR_MSG_ELEMENT_TYPE_HANDLE;
    pcrdr_msg_data_type data_type;
    purc_variant_t data = PURC_VARIANT_INVALID;

    target = PCRDR_MSG_TARGET_WORKSPACE;
    target_value = workspace;

    if (purc_variant_get_string_const(value)) {
        data_type = PCRDR_MSG_DATA_TYPE_PLAIN;
    }
    else {
        data_type = PCRDR_MSG_DATA_TYPE_JSON;
    }

    char element[LEN_BUFF_LONGLONGINT];
    int n = snprintf(element, sizeof(element),
            "%llx", (unsigned long long int)plain_window);
    if (n < 0) {
        purc_set_error(PURC_ERROR_BAD_STDC_CALL);
        goto failed;
    }
    else if ((size_t)n >= sizeof (element)) {
        PC_DEBUG ("Too small elementer to serialize message.\n");
        purc_set_error(PURC_ERROR_TOO_SMALL_BUFF);
        goto failed;
    }

    response_msg = pcintr_rdr_send_request_and_wait_response(conn, target,
            target_value, operation, NULL, element_type, element, property,
            data_type, value, 0);
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
    if (data != PURC_VARIANT_INVALID) {
        purc_variant_unref(data);
    }

    if (response_msg) {
        pcrdr_release_message(response_msg);
    }

    return false;
}

bool pcintr_rdr_set_page_groups(struct pcrdr_conn *conn,
        uint64_t workspace, const char *layout_html)
{
    pcrdr_msg *response_msg = NULL;

    const char *operation = PCRDR_OPERATION_SETPAGEGROUPS;
    pcrdr_msg_target target;
    uint64_t target_value;
    pcrdr_msg_element_type element_type = PCRDR_MSG_ELEMENT_TYPE_VOID;
    pcrdr_msg_data_type data_type = PCRDR_MSG_DATA_TYPE_PLAIN;
    purc_variant_t data = PURC_VARIANT_INVALID;

    target = PCRDR_MSG_TARGET_WORKSPACE;
    target_value = workspace;

    data = purc_variant_make_string_static(layout_html, false);
    if (data == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    response_msg = pcintr_rdr_send_request_and_wait_response(conn, target,
            target_value, operation, NULL, element_type, NULL, NULL,
            data_type, data, 0);

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
    if (data != PURC_VARIANT_INVALID) {
        purc_variant_unref(data);
    }

    if (response_msg) {
        pcrdr_release_message(response_msg);
    }

    return false;
}

bool pcintr_rdr_add_page_groups(struct pcrdr_conn *conn,
        uint64_t workspace, const char *page_groups)
{
    bool retv = false;
    pcrdr_msg *response_msg = NULL;

    const char *operation = PCRDR_OPERATION_ADDPAGEGROUPS;
    pcrdr_msg_target target;
    uint64_t target_value;
    pcrdr_msg_element_type element_type = PCRDR_MSG_ELEMENT_TYPE_VOID;
    pcrdr_msg_data_type data_type = PCRDR_MSG_DATA_TYPE_HTML;

    purc_variant_t data = purc_variant_make_string_static(page_groups, false);
    if (data == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    target = PCRDR_MSG_TARGET_WORKSPACE;
    target_value = workspace;

    response_msg = pcintr_rdr_send_request_and_wait_response(conn, target,
            target_value, operation, NULL, element_type, NULL, NULL, data_type,
            data, 0);
    if (response_msg == NULL) {
        goto failed;
    }

    int ret_code = response_msg->retCode;
    if (ret_code == PCRDR_SC_OK) {
        retv = true;
    }

    pcrdr_release_message(response_msg);

    if (ret_code != PCRDR_SC_OK) {
        purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
        goto failed;
    }

    return retv;

failed:
    if (data != PURC_VARIANT_INVALID) {
        purc_variant_unref(data);
    }

    return 0;
}

bool pcintr_rdr_remove_page_group(struct pcrdr_conn *conn,
        uint64_t workspace, const char *page_group_id)
{
    pcrdr_msg *response_msg = NULL;

    const char *operation = PCRDR_OPERATION_REMOVEPAGEGROUP;
    pcrdr_msg_target target;
    uint64_t target_value;
    pcrdr_msg_element_type element_type = PCRDR_MSG_ELEMENT_TYPE_HANDLE;
    pcrdr_msg_data_type data_type = PCRDR_MSG_DATA_TYPE_VOID;
    purc_variant_t data = PURC_VARIANT_INVALID;

    target = PCRDR_MSG_TARGET_WORKSPACE;
    target_value = workspace;

    response_msg = pcintr_rdr_send_request_and_wait_response(conn, target,
            target_value, operation, NULL, element_type, page_group_id, NULL,
            data_type, data, 0);
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
    if (data != PURC_VARIANT_INVALID) {
        purc_variant_unref(data);
    }

    if (response_msg) {
        pcrdr_release_message(response_msg);
    }

    return false;
}

bool
pcintr_attach_to_renderer(pcintr_coroutine_t cor,
        pcrdr_page_type_k page_type, const char *target_workspace,
        const char *target_group, const char *page_name,
        purc_renderer_extra_info *extra_info)
{
    struct pcinst *inst = pcinst_current();
    if (inst == NULL || inst->rdr_caps == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return false;
    }

    struct pcrdr_conn *conn_to_rdr = inst->conn_to_rdr;
    struct renderer_capabilities *rdr_caps = inst->rdr_caps;
    uint64_t session_handle = rdr_caps->session_handle;

    uint64_t workspace = 0;
    if (rdr_caps->workspace != 0 && target_workspace) {
        workspace = pcintr_rdr_retrieve_workspace(conn_to_rdr, session_handle,
                target_workspace);
        if (!workspace) {
            purc_log_error("Failed to retrieve workspace: %s.\n",
                    target_workspace);
            purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
            return false;
        }
    }

    if (extra_info && extra_info->page_groups) {
        if (!pcintr_rdr_add_page_groups(conn_to_rdr, workspace,
                extra_info->page_groups)) {
            purc_log_error("Failed to add page groups to renderer.\n");
            purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
            return false;
        }
    }

    char buff[16];
    if (page_name == NULL) {
        static unsigned nr_pages;
        sprintf(buff, "page-%u", nr_pages);
        nr_pages++;
        page_name = buff;
    }

    uint64_t page;
    page = pcintr_rdr_create_page(conn_to_rdr, workspace,
            page_type, target_group, page_name,
            extra_info ? extra_info->title : NULL,
            extra_info ? extra_info->klass : NULL,
            extra_info ? extra_info->layout_style : NULL,
            extra_info ? extra_info->toolkit_style : NULL);
    if (!page) {
        purc_log_error("Failed to create page: %s.\n", page_name);
        purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
        return false;
    }

    cor->target_workspace_handle = workspace;
    cor->target_page_type = page_type;
    cor->target_page_handle = page;

    return true;
}

static pcrdr_msg *
rdr_page_control_load_large_page(struct pcrdr_conn *conn,
        pcrdr_msg_target target, uint64_t target_value,
        pcrdr_msg_data_type data_type,
        const char *doc_content, size_t len_content)
{
    pcrdr_msg *response_msg = NULL;
    pcrdr_msg_element_type element_type = PCRDR_MSG_ELEMENT_TYPE_VOID;
    purc_variant_t data;
    size_t len_wrotten;
    size_t len_to_write = 0;

    // writeBegin
    const char *start = doc_content;
    const char *end;
    pcutils_string_check_utf8_len(start, DEF_LEN_ONE_WRITE, NULL, &end);
    if (end > start) {
        len_to_write = end - start;

        data = purc_variant_make_string_static(start, false);
        len_wrotten = len_to_write;
    }
    else {
        LOG_ERROR("No valid character in document content\n");
        goto failed;
    }

    response_msg = pcintr_rdr_send_request_and_wait_response(
            conn, target, target_value, PCRDR_OPERATION_WRITEBEGIN, NULL,
            element_type, NULL, NULL, data_type, data, len_to_write);
    if (response_msg == NULL) {
        goto failed;
    }

    if (response_msg->retCode != PCRDR_SC_OK) {
        PC_ERROR("failed to write content to rdr\n");
        goto failed;
    }

    if (len_wrotten == len_content) {
        goto done;
    }

writting:
    len_to_write = 0;
    if (len_wrotten + DEF_LEN_ONE_WRITE > len_content) {
        // writeEnd
        data = purc_variant_make_string_static(doc_content + len_wrotten,
                false);
        response_msg = pcintr_rdr_send_request_and_wait_response(
                conn, target, target_value, PCRDR_OPERATION_WRITEEND, NULL,
                element_type, NULL, NULL, data_type, data, 0);
        if (response_msg == NULL) {
            goto failed;
        }

        if (response_msg->retCode != PCRDR_SC_OK) {
            PC_ERROR("failed to write content to rdr\n");
            goto failed;
        }
        goto done;
    }
    else {
        // writeMore
        start = doc_content + len_wrotten;
        pcutils_string_check_utf8_len(start, DEF_LEN_ONE_WRITE, NULL, &end);
        if (end > start) {
            len_to_write = end - start;
            len_wrotten += len_to_write;
            data = purc_variant_make_string_static(start, false);
        }
        else {
            PC_WARN("no valid character for rdr\n");
            goto failed;
        }

        response_msg = pcintr_rdr_send_request_and_wait_response(
                conn, target, target_value, PCRDR_OPERATION_WRITEMORE, NULL,
                element_type, NULL, NULL, data_type, data, len_to_write);
        if (response_msg == NULL) {
            goto failed;
        }

        if (response_msg->retCode != PCRDR_SC_OK) {
            PC_ERROR("failed to write content to rdr\n");
            goto failed;
        }

        if (len_wrotten == len_content) {
            goto done;
        }
        goto writting;
    }

done:
    return response_msg;

failed:
    if (response_msg) {
        pcrdr_release_message(response_msg);
    }
    return NULL;
}

bool
pcintr_rdr_page_control_load(pcintr_stack_t stack)
{
    if (stack->co->target_page_handle == 0) {
        return true;
    }

    int ret_code;
    pcrdr_msg *response_msg = NULL;

    purc_document_t doc = stack->doc;

    const char *operation = PCRDR_OPERATION_LOAD;
    pcrdr_msg_target target;
    uint64_t target_value;
    const pcrdr_msg_element_type element_type = PCRDR_MSG_ELEMENT_TYPE_VOID;
    pcrdr_msg_data_type data_type = doc->def_text_type;// VW
    purc_variant_t req_data = PURC_VARIANT_INVALID;
    purc_rwstream_t out = NULL;

    switch (stack->co->target_page_type) {
    case PCRDR_PAGE_TYPE_NULL:
        goto failed;
        break;

    case PCRDR_PAGE_TYPE_PLAINWIN:
        target = PCRDR_MSG_TARGET_PLAINWINDOW;
        break;

    case PCRDR_PAGE_TYPE_WIDGET:
        target = PCRDR_MSG_TARGET_WIDGET;
        break;

    default:
        PC_ASSERT(0); // TODO
        break;
    }
    target_value = stack->co->target_page_handle;

    struct pcinst *inst = pcinst_current();

    if (pcrdr_conn_type(inst->conn_to_rdr) == CT_MOVE_BUFFER) {
        /* XXX: pass the document entity directly
           when the connection type is move buffer. */
        req_data = purc_variant_make_native(doc, NULL);
        response_msg = pcintr_rdr_send_request_and_wait_response(
                inst->conn_to_rdr, target, target_value, operation, NULL,
                element_type, NULL, NULL,
                PCRDR_MSG_DATA_TYPE_JSON, req_data, 0);
    }
    else {
        unsigned opt = 0;

        out = purc_rwstream_new_buffer(BUFF_MIN, BUFF_MAX);
        if (out == NULL) {
            goto failed;
        }

        opt |= PCDOC_SERIALIZE_OPT_UNDEF;
        opt |= PCDOC_SERIALIZE_OPT_SKIP_WS_NODES;
        opt |= PCDOC_SERIALIZE_OPT_WITHOUT_TEXT_INDENT;
        opt |= PCDOC_SERIALIZE_OPT_FULL_DOCTYPE;
        opt |= PCDOC_SERIALIZE_OPT_WITH_HVML_HANDLE;

        if (0 != purc_document_serialize_contents_to_stream(doc, opt, out)) {
            goto failed;
        }

        size_t sz_content = 0;
        size_t sz_buff = 0;
        char *p = (char*)purc_rwstream_get_mem_buffer_ex(out, &sz_content,
                &sz_buff,true);
        req_data = purc_variant_make_string_reuse_buff(p, sz_content, false);
        if (req_data == PURC_VARIANT_INVALID) {
            free(p);
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }

        if (sz_content > DEF_LEN_ONE_WRITE) {
            response_msg = rdr_page_control_load_large_page(inst->conn_to_rdr,
                        target, target_value, data_type,
                        p, sz_content);
        }
        else {
            response_msg = pcintr_rdr_send_request_and_wait_response(
                    inst->conn_to_rdr, target, target_value, operation, NULL,
                    element_type, NULL, NULL, data_type, req_data, 0);
        }

        if (out) {
            purc_rwstream_destroy(out);
            out = NULL;
        }
    }

    if (response_msg == NULL) {
        goto failed;
    }

    ret_code = response_msg->retCode;
    if (ret_code == PCRDR_SC_OK) {
        stack->co->target_dom_handle = response_msg->resultValue;
    }

    pcrdr_release_message(response_msg);

    if (ret_code != PCRDR_SC_OK) {
        purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
        goto failed;
    }

    return true;

failed:
    if (out) {
        purc_rwstream_destroy(out);
    }

    /* VW: double free here
    if (req_data != PURC_VARIANT_INVALID) {
        purc_variant_unref(req_data);
    } */

    return false;
}

static const char *rdr_ops[] = {
    PCRDR_OPERATION_STARTSESSION,              // "startSession"
    PCRDR_OPERATION_ENDSESSION,                // "endSession"
    PCRDR_OPERATION_CREATEWORKSPACE,           // "createWorkspace"
    PCRDR_OPERATION_UPDATEWORKSPACE,           // "updateWorkspace"
    PCRDR_OPERATION_DESTROYWORKSPACE,          // "destroyWorkspace"
    PCRDR_OPERATION_CREATEPLAINWINDOW,         // "createPlainWindow"
    PCRDR_OPERATION_UPDATEPLAINWINDOW,         // "updatePlainWindow"
    PCRDR_OPERATION_DESTROYPLAINWINDOW,        // "destroyPlainWindow"
    PCRDR_OPERATION_SETPAGEGROUPS,             // "setPageGroups"
    PCRDR_OPERATION_ADDPAGEGROUPS,             // "addPageGroups"
    PCRDR_OPERATION_REMOVEPAGEGROUP,           // "removePageGroup"
    PCRDR_OPERATION_CREATEWIDGET,              // "createWidget"
    PCRDR_OPERATION_UPDATEWIDGET,              // "updateWidget"
    PCRDR_OPERATION_DESTROYWIDGET,             // "destroyWidget"
    PCRDR_OPERATION_LOAD,                      // "load"
    PCRDR_OPERATION_WRITEBEGIN,                // "writeBegin"
    PCRDR_OPERATION_WRITEMORE,                 // "writeMore"
    PCRDR_OPERATION_WRITEEND,                  // "writeEnd"
    PCRDR_OPERATION_APPEND,                    // "append"
    PCRDR_OPERATION_PREPEND,                   // "prepend"
    PCRDR_OPERATION_INSERTBEFORE,              // "insertBefore"
    PCRDR_OPERATION_INSERTAFTER,               // "insertAfter"
    PCRDR_OPERATION_DISPLACE,                  // "displace"
    PCRDR_OPERATION_UPDATE,                    // "update"
    PCRDR_OPERATION_ERASE,                     // "erase"
    PCRDR_OPERATION_CLEAR,                     // "clear"
    PCRDR_OPERATION_CALLMETHOD,                // "callMethod"
    PCRDR_OPERATION_GETPROPERTY,               // "getProperty"
    PCRDR_OPERATION_SETPROPERTY,               // "setProperty"
};

/* make sure the number of operations matches the enumulators */
#define _COMPILE_TIME_ASSERT(name, x)           \
       typedef int _dummy_ ## name[(x) * 2 - 1]
_COMPILE_TIME_ASSERT(ops,
        PCA_TABLESIZE(rdr_ops) == PCRDR_NR_OPERATIONS);
#undef _COMPILE_TIME_ASSERT

int
pcintr_doc_op_to_rdr_op(pcdoc_operation_k op)
{
    switch (op) {
    case PCDOC_OP_APPEND:
        return PCRDR_K_OPERATION_APPEND;

    case PCDOC_OP_PREPEND:
        return PCRDR_K_OPERATION_PREPEND;

    case PCDOC_OP_INSERTBEFORE:
        return PCRDR_K_OPERATION_INSERTBEFORE;

    case PCDOC_OP_INSERTAFTER:
        return PCRDR_K_OPERATION_INSERTAFTER;

    case PCDOC_OP_DISPLACE:
        return PCRDR_K_OPERATION_DISPLACE;

    case PCDOC_OP_UPDATE:
        return PCRDR_K_OPERATION_UPDATE;

    case PCDOC_OP_ERASE:
        return PCRDR_K_OPERATION_ERASE;

    case PCDOC_OP_CLEAR:
        return PCRDR_K_OPERATION_CLEAR;

    case PCDOC_OP_UNKNOWN:
    default:
        break;
    }
    return 0;
}

pcrdr_msg *
pcintr_rdr_send_dom_req(pcintr_stack_t stack, int op, const char *request_id,
        pcrdr_msg_element_type element_type, const char *css_selector,
        pcdoc_element_t element, const char* property,
        pcrdr_msg_data_type data_type, purc_variant_t data)
{
    if (!stack) {
        return NULL;
    }

    pcintr_coroutine_t co = stack->co;
    if (co->target_page_handle == 0 || co->target_dom_handle == 0) {
        if (!co->stack.inherit) {
            return NULL;
        }

        pcintr_coroutine_t parent = pcintr_coroutine_get_by_id(co->curator);
        if (!parent || parent->stack.doc != co->stack.doc) {
            return NULL;
        }

        if (parent->target_page_handle == 0
                || parent->target_page_handle == 0) {
            return NULL;
        }

        co->target_workspace_handle = parent->target_workspace_handle;
        co->target_page_type = parent->target_page_type;
        co->target_page_handle = parent->target_page_handle;
        co->target_dom_handle = parent->target_dom_handle;
    }

    if (co->stage != CO_STAGE_OBSERVING && !co->stack.inherit) {
        return NULL;
    }

    const char *operation = rdr_ops[op];
    if (property && op == PCDOC_OP_DISPLACE) {
        // VW: use 'update' operation when displace property
        operation = PCRDR_OPERATION_UPDATE;
    }

    pcrdr_msg *response_msg = NULL;

    pcrdr_msg_target target = PCRDR_MSG_TARGET_DOM;
    uint64_t target_value = stack->co->target_dom_handle;

    char elem[LEN_BUFF_LONGLONGINT];
    int n;
    if (element_type == PCRDR_MSG_ELEMENT_TYPE_HANDLE) {
        n = snprintf(elem, sizeof(elem),
                "%llx", (unsigned long long int)(uint64_t)element);
    }
    else if (element_type == PCRDR_MSG_ELEMENT_TYPE_ID
            || element_type == PCRDR_MSG_ELEMENT_TYPE_ID){
        n = snprintf(elem, sizeof(elem), "%s", css_selector);
    }
    else {
        n = -1;
    }

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
    response_msg = pcintr_rdr_send_request_and_wait_response(inst->conn_to_rdr,
        target, target_value, operation, request_id, element_type, elem,
        property, data_type, data, 0);

    if (response_msg == NULL) {
        goto failed;
    }

    int ret_code = response_msg->retCode;
    if (ret_code != PCRDR_SC_OK) {
        purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
        goto failed;
    }

    return response_msg;

failed:
    if (response_msg != NULL) {
        pcrdr_release_message(response_msg);
    }
    return NULL;
}

pcrdr_msg *
pcintr_rdr_send_dom_req_raw(pcintr_stack_t stack, int op,
        pcrdr_msg_element_type element_type, const char *css_selector,
        pcdoc_element_t element, const char* property,
        pcrdr_msg_data_type data_type, const char *data, size_t len)
{
    if (!stack) {
        return NULL;
    }

    pcintr_coroutine_t co = stack->co;
    if (co->target_page_handle == 0 || co->target_dom_handle == 0) {
        if (!co->stack.inherit) {
            return NULL;
        }

        pcintr_coroutine_t parent = pcintr_coroutine_get_by_id(co->curator);
        if (!parent || parent->stack.doc != co->stack.doc) {
            return NULL;
        }

        if (parent->target_page_handle == 0
                || parent->target_page_handle == 0) {
            return NULL;
        }

        co->target_workspace_handle = parent->target_workspace_handle;
        co->target_page_type = parent->target_page_type;
        co->target_page_handle = parent->target_page_handle;
        co->target_dom_handle = parent->target_dom_handle;
    }

    if (co->stage != CO_STAGE_OBSERVING && !co->stack.inherit) {
        return NULL;
    }

    pcrdr_msg *ret = NULL;
    purc_variant_t req_data = PURC_VARIANT_INVALID;
    if (data_type == PCRDR_MSG_DATA_TYPE_JSON) {
        req_data = purc_variant_make_from_json_string(data, len);
        if (req_data == PURC_VARIANT_INVALID) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }
    }
    else {  /* VW: for other data types */
        req_data = purc_variant_make_string(data, false);
        if (req_data == PURC_VARIANT_INVALID) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }
    }

    ret = pcintr_rdr_send_dom_req(stack, op, NULL, element_type, css_selector,
            element, property, data_type, req_data);
    return ret;

failed:
    if (req_data != PURC_VARIANT_INVALID) {
        purc_variant_unref(req_data);
    }
    return ret;
}

bool
pcintr_rdr_send_dom_req_simple(pcintr_stack_t stack, int op,
        pcdoc_element_t element, const char *property,
        pcrdr_msg_data_type data_type, purc_variant_t data)
{
    pcrdr_msg *response_msg = pcintr_rdr_send_dom_req(stack, op,
            NULL, PCRDR_MSG_ELEMENT_TYPE_HANDLE, NULL,
            element, property, data_type, data);
    if (response_msg != NULL) {
        pcrdr_release_message(response_msg);
        return true;
    }
    return false;
}

bool
pcintr_rdr_send_dom_req_simple_raw(pcintr_stack_t stack,
        int op, pcdoc_element_t element,
        const char *property, pcrdr_msg_data_type data_type,
        const char *data, size_t len)
{
    if (data && len == 0) {
        len = strlen(data);
    }

    if (len == 0) {
        data = " ";
        len = 1;
    }
    pcrdr_msg *response_msg = pcintr_rdr_send_dom_req_raw(stack, op,
            PCRDR_MSG_ELEMENT_TYPE_HANDLE, NULL,
            element, property, data_type, data, len);

    if (response_msg != NULL) {
        pcrdr_release_message(response_msg);
        return true;
    }
    return false;
}

purc_variant_t
pcintr_rdr_call_method(pcintr_stack_t stack, const char *request_id,
        const char *css_selector, const char *method, purc_variant_t arg)
{
    purc_variant_t ret = PURC_VARIANT_INVALID;
    purc_variant_t m = PURC_VARIANT_INVALID;
    pcrdr_msg_data_type data_type = PCRDR_MSG_DATA_TYPE_JSON;
    purc_variant_t data = purc_variant_make_object(0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    if (!data) {
        goto out;
    }

    m = purc_variant_make_string(method, false);
    if (!m) {
        goto out;
    }

    if (!purc_variant_object_set_by_static_ckey(data, RDR_KEY_METHOD, m)) {
        goto out;
    }

    if (arg &&
            !purc_variant_object_set_by_static_ckey(data, RDR_KEY_ARG, arg)) {
        goto out;
    }

    pcrdr_msg *response_msg = pcintr_rdr_send_dom_req(stack,
            PCRDR_K_OPERATION_CALLMETHOD, request_id, PCRDR_MSG_ELEMENT_TYPE_ID,
            css_selector, NULL, NULL, data_type, data);
    if (response_msg != NULL) {
        ret = purc_variant_ref(response_msg->data);
        pcrdr_release_message(response_msg);
    }

out:
    if (m) {
        purc_variant_unref(m);
    }
    return ret;
}

