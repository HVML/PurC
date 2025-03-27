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

#define _GNU_SOURCE
// #undef NDEBUG

#include "purc.h"
#include "config.h"
#include "internal.h"

#include "private/errors.h"
#include "private/instance.h"
#include "private/utils.h"
#include "private/variant.h"
#include "private/pcrdr.h"
#include "pcrdr/connect.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define ID_KEY                  "id"
#define NAME_KEY                "name"
#define TITLE_KEY               "title"
#define CLASS_KEY               "class"
#define LAYOUT_STYLE_KEY        "layoutStyle"
#define TOOLKIT_STYLE_KEY       "toolkitStyle"
#define TRANSITION_STYLE_KEY    "transitionStyle"
#define KEEP_CONTENTS_KEY       "keepContents"

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

pcrdr_msg *pcintr_rdr_send_request_and_wait_response_ex(struct pcrdr_conn *conn,
        pcrdr_msg_target target, uint64_t target_value, const char *operation,
        const char *request_id, pcrdr_msg_element_type element_type,
        const char *element, const char *property,
        pcrdr_msg_data_type data_type, purc_variant_t data, size_t data_len,
        int seconds_expected)
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
    if (data) {
        msg->data = purc_variant_ref(data);
    }
    if (data_len > 0) {
        msg->textLen = data_len;
    }

    if (request_id && strcmp(request_id, PCINTR_RDR_NORETURN_REQUEST_ID) == 0) {
        pcrdr_send_request(conn, msg, seconds_expected, NULL, NULL);
    }
    else {
        pcrdr_send_request_and_wait_response(conn,
                msg, seconds_expected, &response_msg);
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

pcrdr_msg *pcintr_rdr_send_request_and_wait_response(struct pcrdr_conn *conn,
        pcrdr_msg_target target, uint64_t target_value, const char *operation,
        const char *request_id, pcrdr_msg_element_type element_type,
        const char *element, const char *property,
        pcrdr_msg_data_type data_type, purc_variant_t data, size_t data_len)
{
    return pcintr_rdr_send_request_and_wait_response_ex(conn,
        target, target_value, operation, request_id, element_type,
        element, property, data_type, data, data_len, PCRDR_TIME_DEF_EXPECTED);
}


/* Since PURCMC 120, the operation `createWorkspace` returns the handle
   to the workspace if the workspace which is specified by the name exists. */
static uint64_t pcintr_rdr_retrieve_workspace(struct pcrdr_conn *conn,
        uint64_t session, const char *workspace_name)
{
    uint64_t handle = 0;
    pcrdr_msg *response_msg = NULL;

    /* createWorkspace */
    response_msg = pcintr_rdr_send_request_and_wait_response(conn,
            PCRDR_MSG_TARGET_SESSION, session, PCRDR_OPERATION_CREATEWORKSPACE,
            NULL, PCRDR_MSG_ELEMENT_TYPE_ID, workspace_name, NULL,
            PCRDR_MSG_DATA_TYPE_VOID, PURC_VARIANT_INVALID, 0);
    if (response_msg == NULL) {
        goto done;
    }

    if (response_msg->retCode != PCRDR_SC_OK) {
        purc_log_error("Failed request: %s  (%d)\n",
                PCRDR_OPERATION_GETPROPERTY, response_msg->retCode);
        purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
        goto done;
    }

    handle = response_msg->resultValue;

done:
    if (response_msg) {
        pcrdr_release_message(response_msg);
    }

    return handle;
}

#define MAX_PAGE_ID             (PURC_LEN_IDENTIFIER * 2 + 2)
#define SEP_GROUP_NAME          "@"

static uint64_t
pcintr_rdr_create_page(struct pcrdr_conn *conn, uint64_t workspace,
        pcrdr_page_type_k page_type, const char *target_group,
        const char *page_name, purc_variant_t data)
{
    uint64_t page_handle = 0;
    pcrdr_msg *response_msg = NULL;

    const char *operation;
    if (page_type == PCRDR_PAGE_TYPE_PLAINWIN) {
        operation = PCRDR_OPERATION_CREATEPLAINWINDOW;
    }
    else if (page_type == PCRDR_PAGE_TYPE_WIDGET) {
        if (target_group == NULL) {
            purc_log_error("No target group specified when creating widget!\n");
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto out;
        }
        operation = PCRDR_OPERATION_CREATEWIDGET;
    }
    else {
        assert(0);
    }

    if (!purc_is_valid_identifier(page_name) ||
            (target_group && !purc_is_valid_identifier(target_group))) {
        purc_log_error("Bad page name or group name: %s@%s!\n",
                page_name, target_group);
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    pcrdr_msg_element_type element_type = PCRDR_MSG_ELEMENT_TYPE_ID;
    char element_value[MAX_PAGE_ID];
    strcpy(element_value, page_name);
    if (target_group) {
        strcat(element_value, SEP_GROUP_NAME);
        strcat(element_value, target_group);
    }

    pcrdr_msg_target target = PCRDR_MSG_TARGET_WORKSPACE;
    uint64_t target_value = workspace;

    pcrdr_msg_data_type data_type = PCRDR_MSG_DATA_TYPE_VOID;
    if (data) {
        data_type = PCRDR_MSG_DATA_TYPE_JSON;
    }

    /* createPlainWindow or createWidget */
    response_msg = pcintr_rdr_send_request_and_wait_response(conn, target,
            target_value, operation, NULL, element_type, element_value, NULL,
            data_type, data, 0);
    if (response_msg == NULL) {
        // pcintr_rdr_send_request_and_wait_response unref data
        goto out;
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

out:
    return page_handle;
}

static bool pcintr_rdr_add_page_groups(struct pcrdr_conn *conn,
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
        goto out;
    }

    target = PCRDR_MSG_TARGET_WORKSPACE;
    target_value = workspace;

    /* addPageGroups */
    response_msg = pcintr_rdr_send_request_and_wait_response(conn, target,
            target_value, operation, NULL, element_type, NULL, NULL, data_type,
            data, 0);
    if (response_msg == NULL) {
        goto out;
    }

    int ret_code = response_msg->retCode;
    pcrdr_release_message(response_msg);

    if (ret_code == PCRDR_SC_OK) {
        retv = true;
    }
    else {
        purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
        goto out;
    }

out:
    if (data) {
        purc_variant_unref(data);
    }
    return retv;
}

bool
pcintr_attach_to_renderer(struct pcrdr_conn *conn, pcintr_coroutine_t cor,
        pcrdr_page_type_k page_type, const char *target_workspace,
        const char *target_group, const char *page_name,
        purc_renderer_extra_info *extra_info)
{
    assert(page_type == PCRDR_PAGE_TYPE_PLAINWIN ||
            page_type == PCRDR_PAGE_TYPE_WIDGET);

    if (conn == NULL) {
        purc_log_error("Lost the connection to renderer.\n");
        purc_set_error(PURC_ERROR_CONNECTION_ABORTED);
        goto failed;
    }

    assert(conn->caps);

    struct renderer_capabilities *rdr_caps = conn->caps;
    uint64_t session_handle = rdr_caps->session_handle;
    uint64_t workspace = 0;
    if (rdr_caps->workspace != 0 && target_workspace) {
        workspace = pcintr_rdr_retrieve_workspace(conn, session_handle,
                target_workspace);
        if (!workspace) {
            purc_log_error("Failed to retrieve workspace: %s.\n",
                    target_workspace);
            purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
            goto failed;
        }
    }

    if (extra_info && extra_info->page_groups) {
        if (!pcintr_rdr_add_page_groups(conn, workspace,
                extra_info->page_groups)) {
            purc_log_error("Failed to add page groups to renderer.\n");
            purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
            goto failed;
        }
    }

    /* Since PURCMC 120, use `main` as the default page name. */
    if (page_name == NULL) {
        page_name = PCRDR_DEFAULT_PAGENAME;
    }

    uint64_t page = 0;
    purc_variant_t data = PURC_VARIANT_INVALID;
    if (extra_info) {
        int errors = 0;

        data = purc_variant_make_object_0();
        if (data) {
            if (extra_info->title) {
                if (!object_set(data, TITLE_KEY, extra_info->title)) {
                    errors++;
                }
            }

            if (extra_info->klass) {
                if (!object_set(data, CLASS_KEY, extra_info->klass)) {
                    errors++;
                }
            }

            if (extra_info->layout_style) {
                if (!object_set(data, LAYOUT_STYLE_KEY,
                            extra_info->layout_style)) {
                    errors++;
                }
            }

            if (extra_info->toolkit_style) {
                if (!purc_variant_object_set_by_static_ckey(data,
                            TOOLKIT_STYLE_KEY,
                            extra_info->toolkit_style)) {
                    errors++;
                }
            }

            if (extra_info->transition_style) {
                if (!object_set(data, TRANSITION_STYLE_KEY,
                            extra_info->transition_style)) {
                    errors++;
                }
            }

            if (extra_info->keep_contents) {
                if (!purc_variant_object_set_by_static_ckey(data,
                            KEEP_CONTENTS_KEY,
                            extra_info->keep_contents)) {
                    errors++;
                }
            }
        }
        else
            errors++;

        if (errors > 0) {
            purc_log_error("Failed to create data for page.\n");
            if (data) {
                purc_variant_unref(data);
            }
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }
    }

    page = pcintr_rdr_create_page(conn, workspace,
            page_type, target_group, page_name, data);
    if (data)
        purc_variant_unref(data);
    if (!page) {
        purc_log_error("Failed to create page: %s.\n", page_name);
        goto failed;
    }

    struct pcintr_coroutine_rdr_conn *rdr_conn;
    rdr_conn = pcintr_coroutine_create_or_get_rdr_conn(cor, conn);
    rdr_conn->workspace_handle = workspace;
    rdr_conn->page_handle = page;

    cor->target_page_type = page_type;
    return true;

failed:
    return false;
}

static void
check_response_for_suppressed(struct pcinst *inst,
        pcintr_coroutine_t co_loaded, const pcrdr_msg *response)
{
    /* Check the attached data for suppressed coroutine */
    if (response->dataType == PCRDR_MSG_DATA_TYPE_PLAIN &&
            response->data != PURC_VARIANT_INVALID) {
        const char *plain = purc_variant_get_string_const(response->data);

        uint64_t crtn_handle;
        if (plain) {
            crtn_handle = strtoull(plain, NULL, 16);
            if (crtn_handle) {
                pcintr_suppress_crtn_doc(inst, co_loaded, crtn_handle);
            }
        }
    }
}

#define PCRDR_TIME_LARGE_EXPECTED       10

static pcrdr_msg *
rdr_page_control_load_large_page(struct pcrdr_conn *conn,
        pcintr_coroutine_t co_loaded,
        pcrdr_msg_target target, uint64_t target_value, const char *elem,
        pcrdr_msg_data_type data_type,
        const char *doc_content, size_t len_content)
{
    pcrdr_msg *response_msg = NULL;
    purc_variant_t data = PURC_VARIANT_INVALID;
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

    /* writeBegin */
    response_msg = pcintr_rdr_send_request_and_wait_response_ex(
            conn, target, target_value, PCRDR_OPERATION_WRITEBEGIN, NULL,
            PCRDR_MSG_ELEMENT_TYPE_HANDLE, elem, NULL,
            data_type, data, len_to_write, PCRDR_TIME_LARGE_EXPECTED);
    purc_variant_unref(data);

    if (response_msg == NULL) {
        PC_ERROR("Failed to send request to renderer expired=%d\n", PCRDR_TIME_LARGE_EXPECTED);
        goto failed;
    }

    if (response_msg->retCode != PCRDR_SC_OK) {
        PC_ERROR("Failed to write content to renderer\n");
        goto failed;
    }

    check_response_for_suppressed(pcinst_current(), co_loaded, response_msg);

    if (len_wrotten == len_content) {
        goto done;
    }

    pcrdr_release_message(response_msg);
    response_msg = NULL;

writting:
    len_to_write = 0;
    if (len_wrotten + DEF_LEN_ONE_WRITE > len_content) {
        // writeEnd
        data = purc_variant_make_string_static(doc_content + len_wrotten,
                false);
        /* writeEnd */
        response_msg = pcintr_rdr_send_request_and_wait_response_ex(
                conn, target, target_value, PCRDR_OPERATION_WRITEEND, NULL,
                PCRDR_MSG_ELEMENT_TYPE_VOID, NULL, NULL, data_type, data, 0,
                PCRDR_TIME_LARGE_EXPECTED);
        purc_variant_unref(data);
        if (response_msg == NULL) {
            PC_ERROR("Failed to send request to renderer\n");
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

        /* writeMore */
        response_msg = pcintr_rdr_send_request_and_wait_response_ex(
                conn, target, target_value, PCRDR_OPERATION_WRITEMORE, NULL,
                PCRDR_MSG_ELEMENT_TYPE_VOID, NULL, NULL,
                data_type, data, len_to_write, PCRDR_TIME_LARGE_EXPECTED);
        purc_variant_unref(data);
        if (response_msg == NULL) {
            PC_ERROR("Failed to send request to renderer\n");
            goto failed;
        }

        if (response_msg->retCode != PCRDR_SC_OK) {
            PC_ERROR("failed to write content to rdr\n");
            goto failed;
        }

        if (len_wrotten == len_content) {
            goto done;
        }

        pcrdr_release_message(response_msg);
        response_msg = NULL;
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
pcintr_rdr_page_control_load(struct pcinst *inst, pcrdr_conn *conn,
        pcintr_coroutine_t cor)
{
    PC_INFO("rdr page control load, tickcount is %ld\n", pcintr_tick_count());

    purc_rwstream_t out = NULL;

    /* supress update opeations */
    if (cor && cor->supressed) {
        goto failed;
    }

    pcrdr_msg *response_msg = NULL;
    purc_document_t doc = cor->stack.doc;

    pcrdr_msg_target target;
    uint64_t target_value;
    pcrdr_msg_data_type data_type = doc->def_text_type;// VW
    purc_variant_t req_data = PURC_VARIANT_INVALID;
    struct pcintr_coroutine_rdr_conn *rdr_conn;
    rdr_conn = pcintr_coroutine_get_rdr_conn(cor, conn);

    switch (cor->target_page_type) {
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
    target_value = rdr_conn->page_handle;

    /* Since 0.9.22 */
    if (conn->caps->js_to_inject) {
        pcdoc_element_t head = purc_document_head(doc);
        if (head) {
            pcdoc_element_t script = pcdoc_element_new_element(doc, head,
                    PCDOC_OP_APPEND, "script", false);
            if (script) {
                if (pcdoc_element_set_attribute(doc, script, PCDOC_OP_UPDATE,
                        "src", conn->caps->js_to_inject,
                        strlen(conn->caps->js_to_inject))) {
                    PC_WARN("Failed to set the src attribute for injecting JS\n");
                }
            }
            else {
                PC_WARN("Failed to create <scritp> element for injecting JS\n");
            }
        }
        else {
            PC_WARN("Failed to get <head> element in the doc\n");
        }
    }

    const pcrdr_msg_element_type element_type = PCRDR_MSG_ELEMENT_TYPE_HANDLE;
    char elem[LEN_BUFF_LONGLONGINT];
    int n = snprintf(elem, sizeof(elem),
            "%llx", (unsigned long long int)(uint64_t)(uintptr_t)cor);
    assert(n < (int)sizeof(elem));
    (void)n;

    int conn_type = pcrdr_conn_type(conn);
    if (conn_type == CT_MOVE_BUFFER) {
        PC_INFO("rdr page control load, tickcount is %ld to move buffer\n",
                pcintr_tick_count());
        /* XXX: pass the document entity directly
           when the connection type is move buffer. */
        req_data = purc_variant_make_native(doc, NULL);

        /* load */
        response_msg = pcintr_rdr_send_request_and_wait_response(
                conn, target, target_value,
                PCRDR_OPERATION_LOAD, NULL,
                element_type, elem, NULL,
                PCRDR_MSG_DATA_TYPE_JSON, req_data, 0);
        purc_variant_unref(req_data);

        if (response_msg) {
            check_response_for_suppressed(inst, cor, response_msg);
        }
    }
    /* Use rdr_caps->doc_loading_method since PURCMC 170 */
    else if (conn->caps->doc_loading_method == PCRDR_K_DLM_url) {

        char *path;
        int path_len;
        /* try to use /app/<app_name>/exported/tmp/ first */
        if ((path_len = asprintf(&path, PCRDR_PATH_FORMAT_DOC,
                        inst->app_name)) < 0) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }

        if (access(path, W_OK | X_OK) == 0) {
            char file_name[PURC_LEN_RUNNER_NAME + 32];
            snprintf(file_name, sizeof(file_name), "%s-XXXXXX.html",
                    inst->runner_name);
            path = realloc(path, path_len + sizeof(file_name));
            if (path == NULL) {
                purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
                goto failed;
            }

            strcat(path, file_name);
        }
        else {
            /* use /tmp instead; make sure the buffer is enough. */
            path_len += PURC_LEN_RUNNER_NAME + 32;
            path = realloc(path, path_len);
            snprintf(path, path_len, "/tmp/%s-%s-XXXXXX.html",
                    inst->app_name, inst->runner_name);
        }

        int fd = mkstemps(path, 5);
        if (fd < 0) {
            PC_ERROR("Failed to open a temp file: %s (%d): %s\n",
                    path, fd, strerror(errno));
            free(path);
            purc_set_error(purc_error_from_errno(errno));
            goto failed;
        }

        out = purc_rwstream_new_from_unix_fd(fd);
        if (out == NULL) {
            free(path);
            goto failed;
        }

        unsigned opt = 0;
        opt |= PCDOC_SERIALIZE_OPT_UNDEF;
        opt |= PCDOC_SERIALIZE_OPT_SKIP_WS_NODES;
        opt |= PCDOC_SERIALIZE_OPT_WITHOUT_TEXT_INDENT;
        opt |= PCDOC_SERIALIZE_OPT_FULL_DOCTYPE;
        opt |= PCDOC_SERIALIZE_OPT_WITH_HVML_HANDLE;

        if (0 != purc_document_serialize_contents_to_stream(doc, opt, out)) {
            free(path);
            goto failed;
        }
        purc_rwstream_destroy(out);
        close(fd);
        out = NULL;

        char *url;
        int url_len = asprintf(&url,
                "hvml://localhost/_filesystem/_file/-%s", path);
        free(path);
        if (url_len < 0) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }

        PC_INFO("rdr page control load, tickcount is %ld to rdr url=%s\n",
                pcintr_tick_count(), url);

        data_type = PCRDR_MSG_DATA_TYPE_PLAIN;
        req_data = purc_variant_make_string_reuse_buff(url, url_len + 1, false);
        if (req_data == PURC_VARIANT_INVALID) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }

        /* loadFromURL */
        response_msg = pcintr_rdr_send_request_and_wait_response(
                conn, target, target_value,
                PCRDR_OPERATION_LOADFROMURL, NULL,
                element_type, elem, NULL, data_type, req_data, 0);
        if (response_msg) {
            check_response_for_suppressed(inst, cor, response_msg);
        }

        purc_variant_unref(req_data);
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
                &sz_buff, true);
#if 0
        FILE *fp = fopen("/tmp/renderer.html", "w");
        fwrite(p, 1, sz_content, fp);
        fclose(fp);
#endif
        req_data = purc_variant_make_string_reuse_buff(p, sz_content, false);
        if (req_data == PURC_VARIANT_INVALID) {
            free(p);
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }

        PC_INFO("rdr page control load, tickcount is %ld to rdr sz_content=%ld\n",
                pcintr_tick_count(), sz_content);
        if (sz_content > DEF_LEN_ONE_WRITE) {
            response_msg = rdr_page_control_load_large_page(conn,
                    cor, target, target_value, elem,
                    data_type, p, sz_content);
        }
        else {
            /* load */
            response_msg = pcintr_rdr_send_request_and_wait_response(
                    conn, target, target_value,
                    PCRDR_OPERATION_LOAD, NULL,
                    element_type, elem, NULL, data_type, req_data, 0);
            if (response_msg) {
                check_response_for_suppressed(inst, cor, response_msg);
            }
        }
        purc_variant_unref(req_data);

        purc_rwstream_destroy(out);
        out = NULL;
    }

    if (response_msg == NULL) {
        rdr_conn->dom_handle = 0;
        goto failed;
    }

    int ret_code = response_msg->retCode;
    uint64_t result = response_msg->resultValue;
    pcrdr_release_message(response_msg);

    if (ret_code == PCRDR_SC_OK) {
        rdr_conn->dom_handle = result;
    }
    else {
        rdr_conn->dom_handle = 0;
        purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
        goto failed;
    }

    PC_INFO("rdr page control load, tickcount is %ld success\n", pcintr_tick_count());
    return true;

failed:
    if (out) {
        purc_rwstream_destroy(out);
    }

    /* VW: double free here
    if (req_data != PURC_VARIANT_INVALID) {
        purc_variant_unref(req_data);
    } */

    PC_WARN("rdr page control load, tickcount is %ld failed\n", pcintr_tick_count());
    return false;
}

int
pcintr_rdr_page_control_register(struct pcinst *inst, pcrdr_conn *conn,
        pcintr_coroutine_t cor)
{
    int ret = PCRDR_ERROR_SERVER_REFUSED;
    pcrdr_msg_target target;
    switch (cor->target_page_type) {
    case PCRDR_PAGE_TYPE_PLAINWIN:
        target = PCRDR_MSG_TARGET_PLAINWINDOW;
        break;

    case PCRDR_PAGE_TYPE_WIDGET:
        target = PCRDR_MSG_TARGET_WIDGET;
        break;

    default:
        assert(0);
        break;
    }

    char elem[LEN_BUFF_LONGLONGINT];
    int n = snprintf(elem, sizeof(elem),
            "%llx", (unsigned long long int)(uint64_t)(uintptr_t)cor);
    assert(n < (int)sizeof(elem));
    (void)n;

    struct pcintr_coroutine_rdr_conn *rdr_conn;
    rdr_conn = pcintr_coroutine_get_rdr_conn(cor, conn);
    pcrdr_msg *response_msg;

    pcrdr_msg_data_type data_type = PCRDR_MSG_DATA_TYPE_VOID;
    purc_variant_t data = PURC_VARIANT_INVALID;
    if (cor->transition_style || cor->keep_contents) {
        int errors = 0;
        data_type = PCRDR_MSG_DATA_TYPE_JSON;
        data = purc_variant_make_object_0();
        if (cor->transition_style) {
            if (!object_set(data, TRANSITION_STYLE_KEY,
                        cor->transition_style)) {
                errors++;
            }
        }

        if (cor->keep_contents) {
            if (!purc_variant_object_set_by_static_ckey(data,
                        KEEP_CONTENTS_KEY,
                        cor->keep_contents)) {
                errors++;
            }
        }

        if (errors > 0) {
            purc_log_error("Failed to create data for page.\n");
            if (data) {
                purc_variant_unref(data);
            }
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }
    }

    /* register */
    response_msg = pcintr_rdr_send_request_and_wait_response(
            conn, target, rdr_conn->page_handle,
            PCRDR_OPERATION_REGISTER, NULL,
            PCRDR_MSG_ELEMENT_TYPE_HANDLE, elem, NULL,
            data_type, data, 0);
    if (data) {
        purc_variant_unref(data);
    }

    if (response_msg == NULL) {
        goto failed;
    }

    int ret_code = response_msg->retCode;
    if (ret_code == PCRDR_SC_OK && response_msg->resultValue != 0) {
        pcintr_suppress_crtn_doc(inst, cor, response_msg->resultValue);
    }

    pcrdr_release_message(response_msg);

    if (ret_code == PCRDR_SC_OK) {
        ret = 0;
    }
    else if (ret_code == PCRDR_SC_NOT_IMPLEMENTED) {
        ret = PCRDR_ERROR_NOT_IMPLEMENTED;
    }
    else {
        purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
        ret = PCRDR_ERROR_SERVER_REFUSED;
    }

failed:
    return ret;
}

int
pcintr_rdr_page_control_revoke(struct pcinst *inst, pcrdr_conn *conn,
        pcintr_coroutine_t cor)
{
    int ret = PCRDR_ERROR_SERVER_REFUSED;
    pcrdr_msg_target target = PCRDR_MSG_TARGET_PLAINWINDOW;
    switch (cor->target_page_type) {
    case PCRDR_PAGE_TYPE_PLAINWIN:
        target = PCRDR_MSG_TARGET_PLAINWINDOW;
        break;

    case PCRDR_PAGE_TYPE_WIDGET:
        target = PCRDR_MSG_TARGET_WIDGET;
        break;

    default:
        assert(0);
        break;
    }

    char elem[LEN_BUFF_LONGLONGINT];
    int n = snprintf(elem, sizeof(elem),
            "%llx", (unsigned long long int)(uint64_t)(uintptr_t)cor);
    assert(n < (int)sizeof(elem));
    (void)n;

    struct pcintr_coroutine_rdr_conn *rdr_conn;
    rdr_conn = pcintr_coroutine_get_rdr_conn(cor, conn);

    pcrdr_msg_data_type data_type = PCRDR_MSG_DATA_TYPE_VOID;
    purc_variant_t data = PURC_VARIANT_INVALID;

    pcrdr_msg *response_msg;
    /* revoke */
    response_msg = pcintr_rdr_send_request_and_wait_response(
            conn, target, rdr_conn->page_handle,
            PCRDR_OPERATION_REVOKE, NULL,
            PCRDR_MSG_ELEMENT_TYPE_HANDLE, elem, NULL,
            data_type, data, 0);
    if (data) {
        purc_variant_unref(data);
    }
    if (response_msg == NULL) {
        goto failed;
    }

    int ret_code = response_msg->retCode;
    uint64_t result = response_msg->resultValue;
    pcrdr_release_message(response_msg);

    if (ret_code == PCRDR_SC_OK) {
        ret = 0;
        if (result != 0) {
            pcintr_reload_crtn_doc(inst, conn, cor, result);
        }
    }
    else if (ret_code == PCRDR_SC_NOT_IMPLEMENTED) {
        ret = PCRDR_ERROR_NOT_IMPLEMENTED;
    }
    else {
        purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
        ret = PCRDR_ERROR_SERVER_REFUSED;
    }

failed:
    return ret;
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
    PCRDR_OPERATION_LOADFROMURL,               // "loadFromURL" (Since 0.9.18)
    PCRDR_OPERATION_LOAD,                      // "load"
    PCRDR_OPERATION_WRITEBEGIN,                // "writeBegin"
    PCRDR_OPERATION_WRITEMORE,                 // "writeMore"
    PCRDR_OPERATION_WRITEEND,                  // "writeEnd"
    PCRDR_OPERATION_REGISTER,                  // "register"
    PCRDR_OPERATION_REVOKE,                    // "revoke"
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

static pcrdr_msg *
pcintr_rdr_send_dom_req(struct pcinst *inst,
        pcintr_coroutine_t co, int op, const char *request_id,
        pcrdr_msg_element_type element_type, const char *css_selector,
        pcdoc_element_t element,  pcdoc_element_t ref_elem, const char* property,
        pcrdr_msg_data_type data_type, purc_variant_t data)
{
    struct pcintr_coroutine_rdr_conn *rdr_conn;
    struct pcrdr_conn *pconn, *qconn;
    struct pcrdr_conn *curr_conn;

    pcrdr_msg *result_msg = NULL;
    pcrdr_msg *response_msg = NULL;
    pcrdr_msg_target target = PCRDR_MSG_TARGET_DOM;
    uint64_t target_value;
    char elem[LEN_BUFF_LONGLONGINT];
    int n;

    if (!co || co->stack.doc->ldc == 0 ) {
        return NULL;
    }

    /* supress update opeations */
    if (co && co->supressed) {
        return NULL;
    }

    struct list_head *conns = &inst->conns;
    const char *operation = rdr_ops[op];
    if (property && op == PCRDR_K_OPERATION_DISPLACE) {
        // VW: use 'update' operation when displace property
        operation = PCRDR_OPERATION_UPDATE;
    }

    if (element_type == PCRDR_MSG_ELEMENT_TYPE_HANDLE) {
        n = snprintf(elem, sizeof(elem),
                "%llx", (unsigned long long int)(uint64_t)element);
    }
    else if (element_type == PCRDR_MSG_ELEMENT_TYPE_ID
            || element_type == PCRDR_MSG_ELEMENT_TYPE_CSS
            || element_type == PCRDR_MSG_ELEMENT_TYPE_CLASS
            || element_type == PCRDR_MSG_ELEMENT_TYPE_TAG
            ){
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

    /* get current conn */
    curr_conn = inst->curr_conn ? inst->curr_conn : inst->conn_to_rdr;

    list_for_each_entry_safe(pconn, qconn, conns, ln) {
        rdr_conn = pcintr_coroutine_get_rdr_conn(co, pconn);
        bool is_current = (pconn == curr_conn);
        const char *req_id = is_current ? request_id :
            PCINTR_RDR_NORETURN_REQUEST_ID;

        if (!rdr_conn || rdr_conn->page_handle == 0 || rdr_conn->dom_handle == 0) {
            continue;
        }

        target_value = rdr_conn->dom_handle;
        if (pcrdr_conn_type(pconn) == CT_MOVE_BUFFER) {
        /* XXX: Pass a reference entity instead of the original data
           if the connection type was move buffer.

        Note that for different operation, the reference entity varies:

      - `append`: the last child element of the target element before this op.
      - `prepend`: the first child element of the tgarget elment before this op.
      - `insertBefore`: the previous sibling of the target element before this op.
      - `insertAfter`: the next sibling of the target element before this op.
      - `displace`: the target element itself.
      - `update`: the target element itself.
      - `erase`: the target element itself.
      - `clear`: the target element itself.

        TODO: Currently, we pass element itself.  */
            if (strcmp(PCRDR_OPERATION_CALLMETHOD, operation) == 0) {
                response_msg = pcintr_rdr_send_request_and_wait_response(
                        pconn, target, target_value, operation,
                        req_id, element_type, elem, property,
                        data_type, data, 0);
            }
            else {
                purc_variant_t req_data = PURC_VARIANT_INVALID;
                pcrdr_msg_data_type req_data_type = PCRDR_MSG_DATA_TYPE_JSON;
                if (ref_elem) {
                    req_data = purc_variant_make_native(ref_elem, NULL);
                }
                else if (data) {
                    req_data = purc_variant_ref(data);
                    req_data_type = data_type;
                }
                /* dom operation */
                response_msg = pcintr_rdr_send_request_and_wait_response(
                        pconn, target, target_value, operation,
                        req_id, element_type, elem, property,
                        req_data_type, req_data, 0);
                if (req_data) {
                    purc_variant_unref(req_data);
                }
            }
        }
        else {
            /* dom operation */
            response_msg = pcintr_rdr_send_request_and_wait_response(
                    pconn, target, target_value, operation,
                    req_id, element_type, elem, property,
                    data_type, data, 0);
        }

        if (is_current && response_msg) {
            int ret_code = response_msg->retCode;
            if (ret_code != PCRDR_SC_OK) {
                purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
                goto failed;
            }

            result_msg = response_msg;
        }
        else {
            if (response_msg != NULL) {
                pcrdr_release_message(response_msg);
            }
        }
    }

    return result_msg;
failed:
    if (response_msg != NULL) {
        pcrdr_release_message(response_msg);
    }
    return NULL;
}

static pcrdr_msg *
pcintr_rdr_send_dom_req_raw(struct pcinst *inst,
        pcintr_coroutine_t co, int op, const char *request_id,
        pcrdr_msg_element_type element_type, const char *css_selector,
        pcdoc_element_t element, pcdoc_element_t ref_elem, const char* property,
        pcrdr_msg_data_type data_type, const char *data, size_t len)
{
    pcrdr_msg *ret = NULL;
    if (!co) {
        goto out;
    }

    purc_variant_t req_data = PURC_VARIANT_INVALID;
    if (data_type == PCRDR_MSG_DATA_TYPE_JSON) {
        req_data = purc_variant_make_from_json_string(data, len);
        if (req_data == PURC_VARIANT_INVALID) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto out;
        }
    }
    else {  /* VW: for other data types */
        req_data = purc_variant_make_string(data, false);
        if (req_data == PURC_VARIANT_INVALID) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto out;
        }
    }

    ret = pcintr_rdr_send_dom_req(inst, co, op, request_id,
            element_type, css_selector, element, ref_elem, property,
            data_type, req_data);
    purc_variant_unref(req_data);

out:
    return ret;
}

bool
pcintr_rdr_send_dom_req_simple_raw(struct pcinst *inst,
        pcintr_coroutine_t co, int op, const char *request_id,
        pcdoc_element_t element, pcdoc_element_t ref_elem,
        const char *property, pcrdr_msg_data_type data_type,
        const char *data, size_t len)
{
    /* supress update opeations */
    if (co && co->supressed) {
        goto out;
    }

    if (data && len == 0) {
        len = strlen(data);
    }

    if (len == 0) {
        data = " ";
        len = 1;
    }
    pcrdr_msg *response_msg = pcintr_rdr_send_dom_req_raw(inst,
            co, op, request_id, PCRDR_MSG_ELEMENT_TYPE_HANDLE, NULL,
            element, ref_elem, property, data_type, data, len);

    if (response_msg != NULL) {
        pcrdr_release_message(response_msg);
        return true;
    }

out:
    return false;
}

purc_variant_t
pcintr_rdr_call_method(struct pcinst *inst,
        pcintr_coroutine_t co, const char *request_id,
        const char *css_selector, const char *method, purc_variant_t arg)
{
    purc_variant_t ret = PURC_VARIANT_INVALID;
    purc_variant_t m = PURC_VARIANT_INVALID;
    pcrdr_msg_data_type data_type = PCRDR_MSG_DATA_TYPE_JSON;
    purc_variant_t data = purc_variant_make_object(0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);

    /* supress update opeations */
    if (co && co->supressed) {
        goto out;
    }

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

    pcrdr_msg *response_msg;
    if (css_selector[0] == '#' && purc_is_valid_css_identifier(css_selector + 1)) {
        response_msg = pcintr_rdr_send_dom_req(inst, co,
            PCRDR_K_OPERATION_CALLMETHOD, request_id, PCRDR_MSG_ELEMENT_TYPE_ID,
            css_selector + 1, NULL, NULL, NULL, data_type, data);
    }
    else if (css_selector[0] == '.' && purc_is_valid_css_identifier(css_selector + 1)) {
        response_msg = pcintr_rdr_send_dom_req(inst, co,
                PCRDR_K_OPERATION_CALLMETHOD, request_id, PCRDR_MSG_ELEMENT_TYPE_CSS,
                css_selector, NULL, NULL, NULL, data_type, data);
    }
    else if (purc_is_valid_css_identifier(css_selector)) {
        response_msg = pcintr_rdr_send_dom_req(inst, co,
                PCRDR_K_OPERATION_CALLMETHOD, request_id, PCRDR_MSG_ELEMENT_TYPE_TAG,
                css_selector, NULL, NULL, NULL, data_type, data);
    }
    else {
        response_msg = pcintr_rdr_send_dom_req(inst, co,
                PCRDR_K_OPERATION_CALLMETHOD, request_id, PCRDR_MSG_ELEMENT_TYPE_CSS,
                css_selector, NULL, NULL, NULL, data_type, data);
    }

    if (response_msg != NULL) {
        if ((response_msg->retCode == PCRDR_SC_OK) && response_msg->data) {
            ret = purc_variant_ref(response_msg->data);
        }
        pcrdr_release_message(response_msg);
    }

out:
    if (m) {
        purc_variant_unref(m);
    }
    if (data) {
        purc_variant_unref(data);
    }
    return ret;
}

purc_variant_t
pcintr_rdr_set_property(struct pcinst *inst,
        pcintr_coroutine_t co, const char *request_id,
        const char *css_selector, const char *property, purc_variant_t value)
{
    purc_variant_t ret = PURC_VARIANT_INVALID;
    pcrdr_msg_data_type data_type = PCRDR_MSG_DATA_TYPE_PLAIN;
    purc_variant_t data = value;

    /* supress update opeations */
    if (co && co->supressed) {
        goto out;
    }

    pcrdr_msg *response_msg;
    if (css_selector[0] == '#' && purc_is_valid_css_identifier(css_selector + 1)) {
        response_msg = pcintr_rdr_send_dom_req(inst, co,
            PCRDR_K_OPERATION_SETPROPERTY, request_id, PCRDR_MSG_ELEMENT_TYPE_ID,
            css_selector + 1, NULL, NULL, property, data_type, data);
    }
    else if (css_selector[0] == '.' && purc_is_valid_css_identifier(css_selector + 1)) {
        response_msg = pcintr_rdr_send_dom_req(inst, co,
                PCRDR_K_OPERATION_SETPROPERTY, request_id, PCRDR_MSG_ELEMENT_TYPE_CSS,
                css_selector, NULL, NULL, property, data_type, data);
    }
    else if (purc_is_valid_css_identifier(css_selector)) {
        response_msg = pcintr_rdr_send_dom_req(inst, co,
                PCRDR_K_OPERATION_SETPROPERTY, request_id, PCRDR_MSG_ELEMENT_TYPE_TAG,
                css_selector, NULL, NULL, property, data_type, data);
    }
    else {
        response_msg = pcintr_rdr_send_dom_req(inst, co,
                PCRDR_K_OPERATION_SETPROPERTY, request_id, PCRDR_MSG_ELEMENT_TYPE_CSS,
                css_selector, NULL, NULL, property, data_type, data);
    }

    if (response_msg != NULL) {
        if ((response_msg->retCode == PCRDR_SC_OK) && response_msg->data) {
            ret = purc_variant_ref(response_msg->data);
        }
        pcrdr_release_message(response_msg);
    }

out:
    return ret;
}

purc_variant_t
pcintr_rdr_get_property(struct pcinst *inst,
        pcintr_coroutine_t co, const char *request_id,
        const char *css_selector, const char *property)
{
    purc_variant_t ret = PURC_VARIANT_INVALID;
    pcrdr_msg_data_type data_type = PCRDR_MSG_DATA_TYPE_VOID;
    purc_variant_t data = PURC_VARIANT_INVALID;

    /* supress update opeations */
    if (co && co->supressed) {
        goto out;
    }

    pcrdr_msg *response_msg;
    if (css_selector[0] == '#' && purc_is_valid_css_identifier(css_selector + 1)) {
        response_msg = pcintr_rdr_send_dom_req(inst, co,
            PCRDR_K_OPERATION_GETPROPERTY, request_id, PCRDR_MSG_ELEMENT_TYPE_ID,
            css_selector + 1, NULL, NULL, property, data_type, data);
    }
    else if (css_selector[0] == '.' && purc_is_valid_css_identifier(css_selector + 1)) {
        response_msg = pcintr_rdr_send_dom_req(inst, co,
                PCRDR_K_OPERATION_GETPROPERTY, request_id, PCRDR_MSG_ELEMENT_TYPE_CSS,
                css_selector, NULL, NULL, property, data_type, data);
    }
    else if (purc_is_valid_css_identifier(css_selector)) {
        response_msg = pcintr_rdr_send_dom_req(inst, co,
                PCRDR_K_OPERATION_GETPROPERTY, request_id, PCRDR_MSG_ELEMENT_TYPE_TAG,
                css_selector, NULL, NULL, property, data_type, data);
    }
    else {
        response_msg = pcintr_rdr_send_dom_req(inst, co,
                PCRDR_K_OPERATION_GETPROPERTY, request_id, PCRDR_MSG_ELEMENT_TYPE_CSS,
                css_selector, NULL, NULL, property, data_type, data);
    }

    if (response_msg != NULL) {
        if ((response_msg->retCode == PCRDR_SC_OK) && response_msg->data) {
            ret = purc_variant_ref(response_msg->data);
        }
        pcrdr_release_message(response_msg);
    }

out:
    return ret;
}

#define ARG_KEY_DATA_TYPE       "dataType"
#define ARG_KEY_DATA            "data"
#define ARG_KEY_PROPERTY        "property"
#define ARG_KEY_NAME            "name"


purc_variant_t
pcintr_rdr_send_rdr_request(struct pcinst *inst, pcintr_coroutine_t co,
        pcrdr_conn *dst_conn, purc_variant_t arg, purc_variant_t op,
        unsigned int is_noreturn)
{
    UNUSED_PARAM(dst_conn);
    purc_variant_t result = PURC_VARIANT_INVALID;

    pcrdr_msg *response_msg = NULL;
    struct pcintr_coroutine_rdr_conn *rdr_conn;
    struct pcrdr_conn *pconn, *qconn;
    struct pcrdr_conn *curr_conn;

    pcrdr_msg_target target = PCRDR_MSG_TARGET_WORKSPACE;
    uint64_t target_value = 0;
    pcrdr_msg_element_type element_type = PCRDR_MSG_ELEMENT_TYPE_VOID;
    const char *element = NULL;
    const char *property = NULL;
    pcrdr_msg_data_type data_type;
    char element_buf[LEN_BUFF_LONGLONGINT];
    bool use_page_handle = false;

    struct list_head *conns = &inst->conns;

    const char *operation = NULL;
    const char *request_id = is_noreturn ? PCINTR_RDR_NORETURN_REQUEST_ID
        : NULL;
    purc_variant_t data = PURC_VARIANT_INVALID;


    if (!arg) {
        purc_set_error_with_info(PURC_ERROR_ARGUMENT_MISSED,
                "Argument missed for request $RDR");
        goto out;
    }
    else if (!purc_variant_is_object(arg)) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "Invalid param type '%s' for $RDR", pcvariant_typename(arg));
        goto out;
    }

    data = purc_variant_object_get_by_ckey(arg, ARG_KEY_DATA);
    if (!data) {
        purc_set_error_with_info(PURC_ERROR_ARGUMENT_MISSED,
                "Argument missed for request to $RDR");
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
            if (tmp == NULL) {
                purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                        "Argument missed for request to $RDR");
                goto out;
            }

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
        else {
            /* clear no such key error */
            purc_clr_error();
        }
    }
    else {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "Invalid param type '%s' for $RDR", pcvariant_typename(data));
        goto out;
    }

    operation = purc_variant_get_string_const(op);
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

    /* TODO: maybe we can shorten the name `PCHVML_KEYWORD_ENUM` */
    if ((pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, SETPAGEGROUPS)) == method) ||
            (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, ADDPAGEGROUPS)) == method)) {
        /* set/add page groups in the current workspace */
        target = PCRDR_MSG_TARGET_WORKSPACE;
    }
    else if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, CALLMETHOD)) == method) {
        /* to call a method in the current session on a page,
           we must use `name` to specify the full page name with page type:
           `plainwin:main`
         */
        purc_variant_t v;
        v = purc_variant_object_get_by_ckey(arg, ARG_KEY_NAME);
        if (!v || !purc_variant_is_string(v)) {
            purc_set_error_with_info(PURC_ERROR_ARGUMENT_MISSED,
                "Argument missed for request to $RDR '%s'", operation);
            goto out;
        }
        element_type = PCRDR_MSG_ELEMENT_TYPE_ID;
        element = purc_variant_get_string_const(v);
    }
    else if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, CREATEPLAINWINDOW)) == method) {
        /* to create a plain window in the current session,
           we must use `name` to specify the page name like
           `userWin@main/userGroups`
         */
        target = PCRDR_MSG_TARGET_WORKSPACE;
        purc_variant_t v = purc_variant_object_get_by_ckey(arg, ARG_KEY_NAME);
        if (!v || !purc_variant_is_string(v)) {
            purc_set_error_with_info(PURC_ERROR_ARGUMENT_MISSED,
                "Argument missed for request to $RDR '%s'", operation);
            goto out;
        }

        element_type = PCRDR_MSG_ELEMENT_TYPE_ID;
        element = purc_variant_get_string_const(v);
    }
    else if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, CREATEWIDGET)) == method) {
        /* to create a widget in the current session,
           use `name` to specify the page name like
           `userWidget@main/userGroups`
         */
        target = PCRDR_MSG_TARGET_WORKSPACE;
        purc_variant_t v = purc_variant_object_get_by_ckey(arg, ARG_KEY_NAME);
        if (!v || !purc_variant_is_string(v)) {
            purc_set_error_with_info(PURC_ERROR_ARGUMENT_MISSED,
                "Argument missed for request to $RDR '%s'", operation);
            goto out;
        }

        element_type = PCRDR_MSG_ELEMENT_TYPE_ID;
        element = purc_variant_get_string_const(v);
    }
    else if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, UPDATEPLAINWINDOW)) == method) {
        target = PCRDR_MSG_TARGET_WORKSPACE;
        purc_variant_t v = purc_variant_object_get_by_ckey(arg, ARG_KEY_NAME);
        if (!v) {
            /*  */
            if (co->target_page_type == PCRDR_PAGE_TYPE_PLAINWIN) {
                use_page_handle = true;
                purc_clr_error();
            }
            else {
                purc_set_error_with_info(PURC_ERROR_ARGUMENT_MISSED,
                    "Argument missed for request to $RDR '%s'", operation);
                goto out;
            }
        }
        else if (purc_variant_is_string(v)) {
            element_type = PCRDR_MSG_ELEMENT_TYPE_ID;
            element = purc_variant_get_string_const(v);
        }
        else {
            purc_set_error_with_info(PURC_ERROR_ARGUMENT_MISSED,
                "Argument missed for request to $RDR '%s'", operation);
            goto out;
        }

        v = purc_variant_object_get_by_ckey(arg, ARG_KEY_PROPERTY);
        purc_clr_error();
        if (v && purc_variant_is_string(v)) {
            property = purc_variant_get_string_const(v);
        }
    }
    else {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "Invalid operation '%s' to $RDR", operation);
        goto out;
    }

    /* get current conn */
    if (dst_conn) {
        curr_conn = dst_conn;
    }
    else {
        curr_conn = inst->curr_conn ? inst->curr_conn : inst->conn_to_rdr;
    }

    list_for_each_entry_safe(pconn, qconn, conns, ln) {
        if (dst_conn && pconn != dst_conn) {
            continue;
        }

        rdr_conn = pcintr_coroutine_get_rdr_conn(co, pconn);
        bool is_current = (pconn == curr_conn);
        target_value = rdr_conn->workspace_handle;
        const char *req_id = is_current ? request_id :
            PCINTR_RDR_NORETURN_REQUEST_ID;

        if (use_page_handle) {
            element_type = PCRDR_MSG_ELEMENT_TYPE_HANDLE;
            snprintf(element_buf, sizeof(element_buf),
                    "%llx", (unsigned long long int)rdr_conn->page_handle);
            element = element_buf;
        }

        /* \$RDR operation : createPlainWindow, setPageGroups and so on  */
        response_msg = pcintr_rdr_send_request_and_wait_response(pconn, target,
                target_value, operation, req_id, element_type, element,
                property, data_type, data, 0);

        if (!is_current) {
            if (response_msg != NULL) {
                pcrdr_release_message(response_msg);
            }
            continue;
        }

        if (is_noreturn) {
            result = purc_variant_make_null();
        }
        else if (response_msg) {
            int ret_code = response_msg->retCode;
            PC_DEBUG("request $RDR ret_code=%d\n", ret_code);
            if (ret_code == PCRDR_SC_OK) {
                if (response_msg->data) {
                    result = purc_variant_ref(response_msg->data);
                }
                else {
                    result = purc_variant_make_null();
                }
            }
            else {
                purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
            }
            pcrdr_release_message(response_msg);
        }
    }

out:
    return result;
}

