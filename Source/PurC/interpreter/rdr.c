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
        pcrdr_msg_element_type element_type, const char *element,
        const char *property, pcrdr_msg_data_type data_type,
        purc_variant_t data)
{
    pcrdr_msg *response_msg = NULL;
    pcrdr_msg *msg = pcrdr_make_request_message(
            target,                             /* target */
            target_value,                       /* target_value */
            operation,                          /* operation */
            NULL,                               /* request_id */
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
            target_value, operation, element_type, NULL, NULL, data_type,
            data);

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
            target_value, operation, element_type, element, NULL,
            data_type, data);

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
            target_value, operation, element_type, element, property,
            data_type, data);

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

uint64_t pcintr_rdr_retrieve_workspace(struct pcrdr_conn *conn,
        uint64_t session, const char *workspace_name)
{
    uint64_t handle = 0;
    pcrdr_msg *response_msg = NULL;

    response_msg = pcintr_rdr_send_request_and_wait_response(conn,
            PCRDR_MSG_TARGET_SESSION, session, PCRDR_OPERATION_GETPROPERTY,
            PCRDR_MSG_ELEMENT_TYPE_VOID, NULL,
            "workspaceList",
            PCRDR_MSG_DATA_TYPE_VOID, PURC_VARIANT_INVALID);
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
        pcrdr_page_type page_type, const char *target_group,
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
            target_value, operation, element_type, element, NULL, data_type,
            data);
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
        pcrdr_page_type page_type, uint64_t plain_window)
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
            target_value, operation, element_type, element, NULL,
            data_type, data);

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
        pcrdr_page_type page_type, uint64_t plain_window,
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
            target_value, operation, element_type, element, property,
            data_type, value);
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
            target_value, operation, element_type, NULL, NULL,
            data_type, data);

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
            target_value, operation, element_type, NULL, NULL, data_type,
            data);
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
            target_value, operation, element_type, page_group_id, NULL,
            data_type, data);
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
        pcrdr_page_type page_type, const char *target_workspace,
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

bool
pcintr_rdr_page_control_load(pcintr_stack_t stack)
{
    if (stack->co->target_page_handle == 0) {
        return true;
    }
    pcrdr_msg *response_msg = NULL;

    purc_document_t doc = stack->doc;

    const char *operation = PCRDR_OPERATION_LOAD;
    pcrdr_msg_target target;
    uint64_t target_value;
    pcrdr_msg_element_type element_type = PCRDR_MSG_ELEMENT_TYPE_VOID;
    pcrdr_msg_data_type data_type = doc->def_text_type;// VW
    purc_variant_t req_data = PURC_VARIANT_INVALID;

    unsigned opt = 0;
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

    struct pcinst *inst = pcinst_current();
    response_msg = pcintr_rdr_send_request_and_wait_response(inst->conn_to_rdr,
        target, target_value, operation, element_type, NULL,
        NULL, data_type, req_data);

    if (response_msg == NULL) {
        goto failed;
    }

    int ret_code = response_msg->retCode;
    if (ret_code == PCRDR_SC_OK) {
        stack->co->target_dom_handle = response_msg->resultValue;
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

    /* VW: double free here
    if (req_data != PURC_VARIANT_INVALID) {
        purc_variant_unref(req_data);
    } */

    return false;
}

static const char *rdr_ops[] = {
    PCRDR_OPERATION_APPEND,
    PCRDR_OPERATION_PREPEND,
    PCRDR_OPERATION_INSERTBEFORE,
    PCRDR_OPERATION_INSERTAFTER,
    PCRDR_OPERATION_DISPLACE,
    PCRDR_OPERATION_UPDATE,
    PCRDR_OPERATION_ERASE,
    PCRDR_OPERATION_CLEAR,
    "",     // unknown
};

pcrdr_msg *
pcintr_rdr_send_dom_req(pcintr_stack_t stack, pcdoc_operation op,
        pcdoc_element_t element, const char* property,
        pcrdr_msg_data_type data_type, purc_variant_t data)
{
    if (!stack || stack->co->target_page_handle == 0
            || stack->co->stage != CO_STAGE_OBSERVING) {
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
    pcrdr_msg_element_type element_type = PCRDR_MSG_ELEMENT_TYPE_HANDLE;

    char elem[LEN_BUFF_LONGLONGINT];
    int n = snprintf(elem, sizeof(elem),
            "%llx", (unsigned long long int)(uint64_t)element);
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

    return response_msg;

failed:
    if (response_msg != NULL) {
        pcrdr_release_message(response_msg);
    }
    return NULL;
}

pcrdr_msg *
pcintr_rdr_send_dom_req_raw(pcintr_stack_t stack, pcdoc_operation op,
        pcdoc_element_t element, const char* property,
        pcrdr_msg_data_type data_type, const char *data, size_t len)
{
    if (!stack || stack->co->target_page_handle == 0
            || stack->co->stage != CO_STAGE_OBSERVING) {
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

    ret = pcintr_rdr_send_dom_req(stack, op, element,
            property, data_type, req_data);
    return ret;

failed:
    if (req_data != PURC_VARIANT_INVALID) {
        purc_variant_unref(req_data);
    }
    return ret;
}

bool
pcintr_rdr_send_dom_req_simple(pcintr_stack_t stack, pcdoc_operation op,
        pcdoc_element_t element, const char *property,
        pcrdr_msg_data_type data_type, purc_variant_t data)
{
    pcrdr_msg *response_msg = pcintr_rdr_send_dom_req(stack, op,
            element, property, data_type, data);
    if (response_msg != NULL) {
        pcrdr_release_message(response_msg);
        return true;
    }
    return false;
}

bool
pcintr_rdr_send_dom_req_simple_raw(pcintr_stack_t stack,
        pcdoc_operation op, pcdoc_element_t element,
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
            element, property, data_type, data, len);

    if (response_msg != NULL) {
        pcrdr_release_message(response_msg);
        return true;
    }
    return false;
}

