/*
** @file endpoint.c
** @author Vincent Wei
** @date 2022/10/02
** @brief The endpoint management (copied from PurC Midnight Commander).
**
** Copyright (c) 2022 FMSoft (http://www.fmsoft.cn)
**
** Author: Vincent Wei (https://github.com/VincentWei)
**
** This file is a part of purc, which is an HVML interpreter with
** a command line interface (CLI).
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include <purc/purc.h>

#include "endpoint.h"
#include "util/kvlist.h"

struct purcth_endpoint {
    time_t  t_created;
    time_t  t_living;

    purc_atom_t rid;
    const char* uri;

    purcth_session *session;

    /* AVL node for the AVL tree sorted by living time */
    struct avl_node avl;
};

int
comp_living_time(const void *k1, const void *k2, void *ptr)
{
    const purcth_endpoint *e1 = k1;
    const purcth_endpoint *e2 = k2;

    (void)ptr;
    return e1->t_living - e2->t_living;
}

void remove_all_living_endpoints(struct avl_tree *avl)
{
    purcth_endpoint *endpoint, *tmp;

    avl_remove_all_elements(avl, endpoint, avl, tmp) {
        // TODO:
    }
}

purcth_endpoint* new_endpoint(purcth_renderer* rdr, const char *uri)
{
    purcth_endpoint* endpoint = NULL;

    endpoint = (purcth_endpoint *)calloc(sizeof (purcth_endpoint), 1);
    if (endpoint == NULL)
        return NULL;

    endpoint->t_created = purc_get_monotoic_time();
    endpoint->t_living = endpoint->t_created;
    endpoint->avl.key = endpoint;

    if (!(endpoint->uri = kvlist_set_ex(&rdr->endpoint_list, uri, &endpoint))) {
        purc_log_error ("Failed to store the endpoint: %s\n", uri);
        return NULL;
    }

    if (avl_insert(&rdr->living_avl, &endpoint->avl)) {
        purc_log_error("Failed to insert to the living AVL tree: %s\n",
                uri);
        return NULL;
    }

    rdr->nr_endpoints++;
    return endpoint;
}

int del_endpoint(purcth_renderer* rdr, purcth_endpoint* endpoint, int cause)
{
    (void)cause;

    if (endpoint->session) {
        rdr->cbs.remove_session(endpoint->session);
        endpoint->session = NULL;
    }

    if (endpoint->avl.key) {
        avl_delete(&rdr->living_avl, &endpoint->avl);
    }

    purc_log_warn("Removing endpoint (%s)\n", endpoint->uri);
    kvlist_delete(&rdr->endpoint_list, endpoint->uri);
    free(endpoint);
    return 0;
}

int check_no_responding_endpoints (purcth_renderer *rdr)
{
    int n = 0;
    time_t t_curr = purc_get_monotoic_time ();
    purcth_endpoint *endpoint, *tmp;

    purc_log_info ("Checking no responding endpoints...\n");

    avl_for_each_element_safe (&rdr->living_avl, endpoint, avl, tmp) {
        if (t_curr > endpoint->t_living + PCRDR_MAX_NO_RESPONDING_TIME) {

            purc_log_info("Removing no-responding client: %s\n", endpoint->uri);
            del_endpoint(rdr, endpoint, CDE_NO_RESPONDING);
            rdr->nr_endpoints--;
            n++;
        }
        else if (t_curr > endpoint->t_living + PCRDR_MAX_PING_TIME) {
            purc_log_info ("Ping client: %s\n", endpoint->uri);
        }
        else {
            purc_log_info ("Skip left endpoints since (%s): %ld\n",
                    endpoint->uri, endpoint->t_living);
            break;
        }
    }

    purc_log_info ("Total endpoints removed: %d\n", n);
    return n;
}

static int send_simple_response(purcth_renderer* rdr, purcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    (void)rdr;
    pcrdr_msg *my_msg = pcrdr_clone_message(msg);
    if (my_msg == NULL)
        return PCRDR_SC_INSUFFICIENT_STORAGE;

    if (purc_inst_move_message(endpoint->rid, my_msg) == 0)
        return PCRDR_SC_INTERNAL_SERVER_ERROR;

    pcrdr_release_message(my_msg);
    return PCRDR_SC_OK;
}

int send_initial_response(purcth_renderer* rdr, purcth_endpoint* endpoint)
{
    (void)rdr;
    int retv = PCRDR_SC_OK;
    pcrdr_msg *msg = NULL;

    msg = pcrdr_make_response_message("0", NULL,
            PCRDR_SC_OK, 0,
            PCRDR_MSG_DATA_TYPE_PLAIN, FOIL_RDR_FEATURES,
            sizeof (FOIL_RDR_FEATURES) - 1);
    if (msg == NULL) {
        retv = PCRDR_SC_INTERNAL_SERVER_ERROR;
        goto failed;
    }

    if (purc_inst_move_message(endpoint->rid, msg) == 0) {
        retv = PCRDR_SC_INTERNAL_SERVER_ERROR;
        goto failed;
    }

    pcrdr_release_message(msg);

failed:
    return retv;
}

typedef int (*request_handler)(purcth_renderer* rdr, purcth_endpoint* endpoint,
        const pcrdr_msg *msg);

static int on_start_session(purcth_renderer* rdr, purcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    pcrdr_msg response = { };
    purcth_session *info = NULL;

    endpoint->session = NULL;
    int retv;
    info = rdr->cbs.create_session(rdr, endpoint);
    if (info == NULL) {
        retv = PCRDR_SC_INSUFFICIENT_STORAGE;
    }
    else {
        endpoint->session = info;
    }

    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = purc_variant_ref(msg->requestId);
    response.sourceURI = PURC_VARIANT_INVALID;
    response.retCode = retv;
    response.resultValue = (uint64_t)info;
    response.dataType = PCRDR_MSG_DATA_TYPE_VOID;

    return send_simple_response(rdr, endpoint, &response);
}

static int on_end_session(purcth_renderer* rdr, purcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    pcrdr_msg response = { };

    if (endpoint->session) {
        rdr->cbs.remove_session(endpoint->session);
        endpoint->session = NULL;
    }

    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = purc_variant_ref(msg->requestId);
    response.sourceURI = PURC_VARIANT_INVALID;
    response.retCode = PCRDR_SC_OK;
    response.resultValue = 0;
    response.dataType = PCRDR_MSG_DATA_TYPE_VOID;

    return send_simple_response(rdr, endpoint, &response);
}

static int on_create_workspace(purcth_renderer* rdr, purcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    int retv = PCRDR_SC_OK;
    pcrdr_msg response = { };
    purcth_workspace* workspace = NULL;

    if (rdr->cbs.create_workspace == NULL) {
        retv = PCRDR_SC_NOT_IMPLEMENTED;
        goto failed;
    }

    const char* name = NULL;
    const char* title = NULL;
    purc_variant_t tmp;

    if (msg->dataType != PCRDR_MSG_DATA_TYPE_JSON ||
            !purc_variant_is_object(msg->data)) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    if ((tmp = purc_variant_object_get_by_ckey(msg->data, "name"))) {
        name = purc_variant_get_string_const(tmp);
        if (name == NULL || !purc_is_valid_identifier(name)) {
            retv = PCRDR_SC_BAD_REQUEST;
            goto failed;
        }
    }

    if ((tmp = purc_variant_object_get_by_ckey(msg->data, "title"))) {
        title = purc_variant_get_string_const(tmp);
    }

    workspace = rdr->cbs.create_workspace(endpoint->session,
            name, title, msg->data, &retv);
    if (retv == 0) {
        rdr->cbs.pend_response(endpoint->session,
                purc_variant_get_string_const(msg->operation),
                purc_variant_get_string_const(msg->requestId),
                workspace);
        return PCRDR_SC_OK;
    }

failed:
    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = purc_variant_ref(msg->requestId);
    response.sourceURI = PURC_VARIANT_INVALID;
    response.retCode = retv;
    response.resultValue = (uint64_t)(uintptr_t)workspace;
    response.dataType = PCRDR_MSG_DATA_TYPE_VOID;

    return send_simple_response(rdr, endpoint, &response);
}

static int on_update_workspace(purcth_renderer* rdr, purcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    int retv = PCRDR_SC_OK;
    purcth_workspace *workspace;
    pcrdr_msg response = { };

    if (rdr->cbs.create_workspace == NULL || rdr->cbs.update_workspace) {
        retv = PCRDR_SC_NOT_IMPLEMENTED;
        goto failed;
    }

    if (msg->elementType == PCRDR_MSG_ELEMENT_TYPE_HANDLE) {
        const char *element = purc_variant_get_string_const(msg->elementValue);
        if (element == NULL) {
            retv = PCRDR_SC_BAD_REQUEST;
            goto failed;
        }

        unsigned long long int handle;
        handle = strtoull(element, NULL, 16);
        workspace = (purcth_workspace *)(uintptr_t)handle;
        if (workspace == NULL) {
            retv = PCRDR_SC_BAD_REQUEST;
            goto failed;
        }
    }
    else {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    const char *property;
    property = purc_variant_get_string_const(msg->property);
    if (property == NULL ||
            !purc_is_valid_token(property, PURC_LEN_PROPERTY_NAME) ||
            msg->dataType != PCRDR_MSG_DATA_TYPE_PLAIN) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    retv = rdr->cbs.update_workspace(endpoint->session, workspace, property,
            purc_variant_get_string_const(msg->data));
    if (retv == 0) {
        rdr->cbs.pend_response(endpoint->session,
                purc_variant_get_string_const(msg->operation),
                purc_variant_get_string_const(msg->requestId),
                workspace);
        return PCRDR_SC_OK;
    }

failed:
    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = purc_variant_ref(msg->requestId);
    response.sourceURI = PURC_VARIANT_INVALID;
    response.retCode = retv;
    response.resultValue = (uint64_t)(uintptr_t)workspace;
    response.dataType = PCRDR_MSG_DATA_TYPE_VOID;

    return send_simple_response(rdr, endpoint, &response);
}

static int on_destroy_workspace(purcth_renderer* rdr, purcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    int retv = PCRDR_SC_OK;
    purcth_workspace *workspace;
    pcrdr_msg response = { };

    if (rdr->cbs.create_workspace == NULL || rdr->cbs.destroy_workspace) {
        retv = PCRDR_SC_NOT_IMPLEMENTED;
        goto failed;
    }

    if (msg->elementType == PCRDR_MSG_ELEMENT_TYPE_HANDLE) {
        const char *element = purc_variant_get_string_const(msg->elementValue);
        if (element == NULL) {
            retv = PCRDR_SC_BAD_REQUEST;
            goto failed;
        }

        unsigned long long int handle;
        handle = strtoull(element, NULL, 16);
        workspace = (purcth_workspace *)(uintptr_t)handle;
        if (workspace == NULL) {
            retv = PCRDR_SC_BAD_REQUEST;
            goto failed;
        }
    }
    else {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    retv = rdr->cbs.destroy_workspace(endpoint->session, workspace);
    if (retv == 0) {
        rdr->cbs.pend_response(endpoint->session,
                purc_variant_get_string_const(msg->operation),
                purc_variant_get_string_const(msg->requestId),
                workspace);
        return PCRDR_SC_OK;
    }

failed:
    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = purc_variant_ref(msg->requestId);
    response.sourceURI = PURC_VARIANT_INVALID;
    response.retCode = retv;
    response.resultValue = (uint64_t)(uintptr_t)workspace;
    response.dataType = PCRDR_MSG_DATA_TYPE_VOID;

    return send_simple_response(rdr, endpoint, &response);
}

static int on_set_page_groups(purcth_renderer* rdr, purcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    int retv = PCRDR_SC_OK;
    pcrdr_msg response = { };
    purcth_workspace* workspace = NULL;

    if (rdr->cbs.set_page_groups == NULL) {
        retv = PCRDR_SC_NOT_IMPLEMENTED;
        goto failed;
    }

    if (msg->target == PCRDR_MSG_TARGET_WORKSPACE) {
        workspace = (void *)(uintptr_t)msg->targetValue;
    }
    else {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    if (msg->dataType != PCRDR_MSG_DATA_TYPE_HTML ||
            msg->data == PURC_VARIANT_INVALID) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    const char *content;
    size_t length;
    content = purc_variant_get_string_const_ex(msg->data, &length);
    if (content == NULL || length == 0) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    retv = rdr->cbs.set_page_groups(endpoint->session, workspace,
            content, length);
    if (retv == 0) {
        rdr->cbs.pend_response(endpoint->session,
                purc_variant_get_string_const(msg->operation),
                purc_variant_get_string_const(msg->requestId),
                workspace);
        return PCRDR_SC_OK;
    }

failed:
    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = purc_variant_ref(msg->requestId);
    response.sourceURI = PURC_VARIANT_INVALID;
    response.retCode = retv;
    response.resultValue = (uint64_t)(uintptr_t)workspace;
    response.dataType = PCRDR_MSG_DATA_TYPE_VOID;

    return send_simple_response(rdr, endpoint, &response);
}

static int on_add_page_groups(purcth_renderer* rdr, purcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    int retv = PCRDR_SC_OK;
    purcth_workspace *workspace = NULL;
    pcrdr_msg response = { };

    if (rdr->cbs.set_page_groups == NULL ||
            rdr->cbs.add_page_groups == NULL) {
        retv = PCRDR_SC_NOT_IMPLEMENTED;
        goto failed;
    }

    if (msg->target == PCRDR_MSG_TARGET_WORKSPACE) {
        workspace = (void *)(uintptr_t)msg->targetValue;
    }
    else {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    if (msg->dataType != PCRDR_MSG_DATA_TYPE_HTML ||
            msg->data == PURC_VARIANT_INVALID) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    const char *content;
    size_t length;
    content = purc_variant_get_string_const_ex(msg->data, &length);
    if (content == NULL || length == 0) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    retv = rdr->cbs.add_page_groups(endpoint->session, workspace,
            content, length);
    if (retv == 0) {
        rdr->cbs.pend_response(endpoint->session,
                purc_variant_get_string_const(msg->operation),
                purc_variant_get_string_const(msg->requestId),
                workspace);
        return PCRDR_SC_OK;
    }

failed:
    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = purc_variant_ref(msg->requestId);
    response.sourceURI = PURC_VARIANT_INVALID;
    response.retCode = retv;
    response.resultValue = (uint64_t)(uintptr_t)workspace;
    response.dataType = PCRDR_MSG_DATA_TYPE_VOID;

    return send_simple_response(rdr, endpoint, &response);
}

static int on_remove_page_group(purcth_renderer* rdr, purcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    int retv = PCRDR_SC_OK;
    purcth_workspace *workspace = NULL;
    const char *gid;
    pcrdr_msg response = { };

    if (rdr->cbs.set_page_groups == NULL ||
            rdr->cbs.remove_page_group == NULL) {
        retv = PCRDR_SC_NOT_IMPLEMENTED;
        goto failed;
    }

    if (msg->target == PCRDR_MSG_TARGET_WORKSPACE) {
        workspace = (void *)(uintptr_t)msg->targetValue;
    }
    else {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    if (msg->elementType == PCRDR_MSG_ELEMENT_TYPE_ID) {
        gid = purc_variant_get_string_const(msg->elementValue);
        if (gid == NULL) {
            retv = PCRDR_SC_BAD_REQUEST;
            goto failed;
        }
    }
    else {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    retv = rdr->cbs.remove_page_group(endpoint->session, workspace, gid);
    if (retv == 0) {
        rdr->cbs.pend_response(endpoint->session,
                purc_variant_get_string_const(msg->operation),
                purc_variant_get_string_const(msg->requestId),
                workspace);
        return PCRDR_SC_OK;
    }

failed:
    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = purc_variant_ref(msg->requestId);
    response.sourceURI = PURC_VARIANT_INVALID;
    response.retCode = retv;
    response.resultValue = (uint64_t)(uintptr_t)workspace;
    response.dataType = PCRDR_MSG_DATA_TYPE_VOID;

    return send_simple_response(rdr, endpoint, &response);
}

static int on_create_plain_window(purcth_renderer* rdr, purcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    int retv = PCRDR_SC_OK;
    pcrdr_msg response = { };

    purcth_workspace* workspace = NULL;
    purcth_plainwin* win = NULL;

    const char* gid = NULL;
    const char* name = NULL;
    const char* class = NULL;
    const char* title = NULL;
    const char* layout_style = NULL;
    purc_variant_t toolkit_style;

    if (msg->target == PCRDR_MSG_TARGET_WORKSPACE) {
        workspace = (void *)(uintptr_t)msg->targetValue;
    }
    else {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    purc_variant_t tmp;

    if (msg->dataType != PCRDR_MSG_DATA_TYPE_JSON ||
            !purc_variant_is_object(msg->data)) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    if (msg->elementType == PCRDR_MSG_ELEMENT_TYPE_ID) {
        gid = purc_variant_get_string_const(msg->elementValue);
        if (gid == NULL) {
            retv = PCRDR_SC_BAD_REQUEST;
            goto failed;
        }
    }

    if ((tmp = purc_variant_object_get_by_ckey(msg->data, "name"))) {
        name = purc_variant_get_string_const(tmp);
    }

    if (name == NULL || !purc_is_valid_identifier(name)) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    if ((tmp = purc_variant_object_get_by_ckey(msg->data, "class"))) {
        class = purc_variant_get_string_const(tmp);
    }

    if ((tmp = purc_variant_object_get_by_ckey(msg->data, "title"))) {
        title = purc_variant_get_string_const(tmp);
    }

    if ((tmp = purc_variant_object_get_by_ckey(msg->data, "layoutStyle"))) {
        layout_style = purc_variant_get_string_const(tmp);
    }

    toolkit_style = purc_variant_object_get_by_ckey(msg->data, "toolkitStyle");

    const char *request_id = purc_variant_get_string_const(msg->requestId);
    win = rdr->cbs.create_plainwin(endpoint->session, workspace,
            request_id, gid, name, class, title, layout_style,
            toolkit_style, &retv);
    if (retv == 0) {
        rdr->cbs.pend_response(endpoint->session,
                purc_variant_get_string_const(msg->operation),
                request_id,
                win);
        return PCRDR_SC_OK;
    }

failed:
    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = purc_variant_ref(msg->requestId);
    response.sourceURI = PURC_VARIANT_INVALID;
    response.retCode = retv;
    response.resultValue = (uint64_t)(uintptr_t)win;
    response.dataType = PCRDR_MSG_DATA_TYPE_VOID;

    return send_simple_response(rdr, endpoint, &response);
}

static int on_update_plain_window(purcth_renderer* rdr, purcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    int retv = PCRDR_SC_OK;
    purcth_workspace *workspace = NULL;
    purcth_plainwin *win = NULL;
    pcrdr_msg response = { };

    if (msg->target == PCRDR_MSG_TARGET_WORKSPACE) {
        workspace = (void *)(uintptr_t)msg->targetValue;
    }
    else {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    if (msg->elementType == PCRDR_MSG_ELEMENT_TYPE_HANDLE) {
        const char *element = purc_variant_get_string_const(msg->elementValue);
        if (element == NULL) {
            retv = PCRDR_SC_BAD_REQUEST;
            goto failed;
        }

        unsigned long long int handle;
        handle = strtoull(element, NULL, 16);
        win = (void *)(uintptr_t)handle;
    }

    if (win == NULL) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    const char *property;
    property = purc_variant_get_string_const(msg->property);
    if (property == NULL ||
            !purc_is_valid_token(property, PURC_LEN_PROPERTY_NAME) ||
            msg->dataType == PCRDR_MSG_DATA_TYPE_VOID) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    retv = rdr->cbs.update_plainwin(endpoint->session, workspace, win,
            property, msg->data);
    if (retv == 0) {
        rdr->cbs.pend_response(endpoint->session,
                purc_variant_get_string_const(msg->operation),
                purc_variant_get_string_const(msg->requestId),
                win);
        return PCRDR_SC_OK;
    }

failed:
    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = purc_variant_ref(msg->requestId);
    response.sourceURI = PURC_VARIANT_INVALID;
    response.retCode = retv;
    response.resultValue = (uint64_t)win;
    response.dataType = PCRDR_MSG_DATA_TYPE_VOID;

    return send_simple_response(rdr, endpoint, &response);
}

static int on_destroy_plain_window(purcth_renderer* rdr, purcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    int retv = PCRDR_SC_OK;
    purcth_workspace *workspace = NULL;
    purcth_plainwin *win = NULL;
    pcrdr_msg response = { };

    if (msg->target == PCRDR_MSG_TARGET_WORKSPACE) {
        workspace = (void *)(uintptr_t)msg->targetValue;
    }
    else {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    if (msg->elementType == PCRDR_MSG_ELEMENT_TYPE_HANDLE) {
        const char *element = purc_variant_get_string_const(msg->elementValue);
        if (element == NULL) {
            retv = PCRDR_SC_BAD_REQUEST;
            goto failed;
        }

        unsigned long long int handle;
        handle = strtoull(element, NULL, 16);
        win = (purcth_plainwin *)(uintptr_t)handle;
    }

    if (win == NULL) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    retv = rdr->cbs.destroy_plainwin(endpoint->session, workspace, win);
    if (retv == 0) {
        rdr->cbs.pend_response(endpoint->session,
                purc_variant_get_string_const(msg->operation),
                purc_variant_get_string_const(msg->requestId),
                win);
        return PCRDR_SC_OK;
    }

failed:
    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = purc_variant_ref(msg->requestId);
    response.sourceURI = PURC_VARIANT_INVALID;
    response.retCode = retv;
    response.resultValue = (uint64_t)(uintptr_t)win;
    response.dataType = PCRDR_MSG_DATA_TYPE_VOID;

    return send_simple_response(rdr, endpoint, &response);
}

static int on_create_page(purcth_renderer* rdr, purcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    int retv = PCRDR_SC_OK;
    pcrdr_msg response = { };
    purcth_workspace* workspace = NULL;
    purcth_page* page = NULL;

    if (rdr->cbs.create_page == NULL) {
        retv = PCRDR_SC_NOT_IMPLEMENTED;
        goto failed;
    }

    if (msg->target == PCRDR_MSG_TARGET_WORKSPACE) {
        workspace = (void *)(uintptr_t)msg->targetValue;
    }
    else {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    if (msg->dataType != PCRDR_MSG_DATA_TYPE_JSON ||
            !purc_variant_is_object(msg->data)) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    const char* gid = NULL;
    if (msg->elementType == PCRDR_MSG_ELEMENT_TYPE_ID) {
        gid = purc_variant_get_string_const(msg->elementValue);
    }

    if (gid == NULL) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    const char* name = NULL;
    const char* class = NULL;
    const char* title = NULL;
    const char* layout_style = NULL;
    purc_variant_t toolkit_style;
    purc_variant_t tmp;

    if ((tmp = purc_variant_object_get_by_ckey(msg->data, "name"))) {
        name = purc_variant_get_string_const(tmp);
        if (name == NULL || !purc_is_valid_identifier(name)) {
            retv = PCRDR_SC_BAD_REQUEST;
            goto failed;
        }
    }

    if ((tmp = purc_variant_object_get_by_ckey(msg->data, "class"))) {
        class = purc_variant_get_string_const(tmp);
    }

    if ((tmp = purc_variant_object_get_by_ckey(msg->data, "title"))) {
        title = purc_variant_get_string_const(tmp);
    }

    if ((tmp = purc_variant_object_get_by_ckey(msg->data, "layoutStyle"))) {
        layout_style = purc_variant_get_string_const(tmp);
    }

    toolkit_style = purc_variant_object_get_by_ckey(msg->data, "toolkitStyle");

    const char *request_id = purc_variant_get_string_const(msg->requestId);
    page = rdr->cbs.create_page(endpoint->session, workspace,
            request_id, gid, name, class, title, layout_style,
            toolkit_style, &retv);
    if (retv == 0) {
        rdr->cbs.pend_response(endpoint->session,
                purc_variant_get_string_const(msg->operation),
                request_id,
                page);
        return PCRDR_SC_OK;
    }

failed:
    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = purc_variant_ref(msg->requestId);
    response.sourceURI = PURC_VARIANT_INVALID;
    response.retCode = retv;
    response.resultValue = (uint64_t)(uintptr_t)page;
    response.dataType = PCRDR_MSG_DATA_TYPE_VOID;

    return send_simple_response(rdr, endpoint, &response);
}

static int on_update_page(purcth_renderer* rdr, purcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    int retv = PCRDR_SC_OK;
    purcth_workspace *workspace = NULL;
    purcth_page *page = NULL;
    pcrdr_msg response = { };

    if (rdr->cbs.create_page == NULL || rdr->cbs.update_page == NULL) {
        retv = PCRDR_SC_NOT_IMPLEMENTED;
        goto failed;
    }

    if (msg->target == PCRDR_MSG_TARGET_WORKSPACE) {
        workspace = (void *)(uintptr_t)msg->targetValue;
    }
    else {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    if (msg->elementType == PCRDR_MSG_ELEMENT_TYPE_HANDLE) {
        const char *element = purc_variant_get_string_const(msg->elementValue);
        if (element == NULL) {
            retv = PCRDR_SC_BAD_REQUEST;
            goto failed;
        }

        unsigned long long int handle;
        handle = strtoull(element, NULL, 16);
        page = (void *)(uintptr_t)handle;
    }

    if (page == NULL) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    const char *property;
    property = purc_variant_get_string_const(msg->property);
    if (property == NULL ||
            !purc_is_valid_token(property, PURC_LEN_PROPERTY_NAME) ||
            msg->dataType == PCRDR_MSG_DATA_TYPE_VOID) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    retv = rdr->cbs.update_page(endpoint->session, workspace,
            page, property, msg->data);
    if (retv == 0) {
        rdr->cbs.pend_response(endpoint->session,
                purc_variant_get_string_const(msg->operation),
                purc_variant_get_string_const(msg->requestId),
                page);
        return PCRDR_SC_OK;
    }

failed:
    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = purc_variant_ref(msg->requestId);
    response.sourceURI = PURC_VARIANT_INVALID;
    response.retCode = retv;
    response.resultValue = (uint64_t)(uintptr_t)page;
    response.dataType = PCRDR_MSG_DATA_TYPE_VOID;

    return send_simple_response(rdr, endpoint, &response);
}

static int on_destroy_page(purcth_renderer* rdr, purcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    int retv = PCRDR_SC_OK;
    purcth_workspace *workspace = NULL;
    purcth_page *page = NULL;
    pcrdr_msg response = { };

    if (rdr->cbs.create_page == NULL || rdr->cbs.destroy_page == NULL) {
        retv = PCRDR_SC_NOT_IMPLEMENTED;
        goto failed;
    }

    if (msg->target == PCRDR_MSG_TARGET_WORKSPACE) {
        workspace = (void *)(uintptr_t)msg->targetValue;
    }
    else {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    if (msg->elementType == PCRDR_MSG_ELEMENT_TYPE_HANDLE) {
        const char *element = purc_variant_get_string_const(msg->elementValue);
        if (element == NULL) {
            retv = PCRDR_SC_BAD_REQUEST;
            goto failed;
        }

        unsigned long long int handle;
        handle = strtoull(element, NULL, 16);
        page = (void *)(uintptr_t)handle;
    }

    if (page == NULL) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    retv = rdr->cbs.destroy_page(endpoint->session, workspace, page);
    if (retv == 0) {
        rdr->cbs.pend_response(endpoint->session,
                purc_variant_get_string_const(msg->operation),
                purc_variant_get_string_const(msg->requestId),
                page);
        return PCRDR_SC_OK;
    }

failed:
    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = purc_variant_ref(msg->requestId);
    response.sourceURI = PURC_VARIANT_INVALID;
    response.retCode = retv;
    response.resultValue = (uint64_t)(uintptr_t)page;
    response.dataType = PCRDR_MSG_DATA_TYPE_VOID;

    return send_simple_response(rdr, endpoint, &response);
}

static int on_load(purcth_renderer* rdr, purcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    pcrdr_msg response = { };
    int retv = PCRDR_SC_OK;
    const char *doc_text;
    size_t doc_len;
    purcth_page *page = NULL;
    purcth_dom *dom = NULL;

    if (msg->dataType != PCRDR_MSG_DATA_TYPE_HTML ||
            msg->data == PURC_VARIANT_INVALID) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    doc_text = purc_variant_get_string_const_ex(msg->data, &doc_len);
    if (doc_text == NULL || doc_len == 0) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    if (msg->target == PCRDR_MSG_TARGET_PLAINWINDOW) {
        purcth_plainwin *win = (void *)(uintptr_t)msg->targetValue;
        page = rdr->cbs.get_plainwin_page(endpoint->session, win, &retv);

        if (page == NULL) {
            goto failed;
        }
    }
    else if (msg->target == PCRDR_MSG_TARGET_WIDGET) {
        page = (void *)(uintptr_t)msg->targetValue;
        if (page == NULL) {
            retv = PCRDR_SC_BAD_REQUEST;
            goto failed;
        }
    }

    dom = rdr->cbs.load(endpoint->session, page,
            PCRDR_K_OPERATION_LOAD, PCRDR_OPERATION_LOAD,
            purc_variant_get_string_const(msg->requestId),
            doc_text, doc_len, &retv);
    if (retv == 0) {
        rdr->cbs.pend_response(endpoint->session,
                purc_variant_get_string_const(msg->operation),
                purc_variant_get_string_const(msg->requestId),
                dom);
        return PCRDR_SC_OK;
    }

failed:
    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = purc_variant_ref(msg->requestId);
    response.sourceURI = PURC_VARIANT_INVALID;
    response.retCode = retv;
    response.resultValue = (uint64_t)(uintptr_t)dom;
    response.dataType = PCRDR_MSG_DATA_TYPE_VOID;

    return send_simple_response(rdr, endpoint, &response);
}

static inline int write_xxx(purcth_renderer* rdr, purcth_endpoint* endpoint,
        int op, const char* op_name, const pcrdr_msg *msg)
{
    pcrdr_msg response = { };
    int retv = PCRDR_SC_OK;
    const char *doc_text;
    size_t doc_len;
    purcth_page *page = NULL;
    purcth_dom *dom = NULL;

    if (msg->dataType != PCRDR_MSG_DATA_TYPE_HTML ||
            msg->data == PURC_VARIANT_INVALID) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    doc_text = purc_variant_get_string_const_ex(msg->data, &doc_len);
    if (doc_text == NULL || doc_len == 0) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    if (msg->target == PCRDR_MSG_TARGET_PLAINWINDOW) {
        purcth_plainwin *win = (void *)(uintptr_t)msg->targetValue;
        page = rdr->cbs.get_plainwin_page(endpoint->session, win, &retv);

        if (page == NULL) {
            goto failed;
        }
    }
    else if (msg->target == PCRDR_MSG_TARGET_WIDGET) {
        page = (void *)(uintptr_t)msg->targetValue;
        if (page == NULL) {
            retv = PCRDR_SC_BAD_REQUEST;
            goto failed;
        }
    }

    dom = rdr->cbs.write(endpoint->session, page, op, op_name,
            purc_variant_get_string_const(msg->requestId),
            doc_text, doc_len, &retv);
    if (retv == 0) {
        rdr->cbs.pend_response(endpoint->session,
                purc_variant_get_string_const(msg->operation),
                purc_variant_get_string_const(msg->requestId),
                dom);
        return PCRDR_SC_OK;
    }

failed:
    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = purc_variant_ref(msg->requestId);
    response.sourceURI = PURC_VARIANT_INVALID;
    response.retCode = retv;
    response.resultValue = (uint64_t)(uintptr_t)dom;
    response.dataType = PCRDR_MSG_DATA_TYPE_VOID;

    return send_simple_response(rdr, endpoint, &response);
}

static int on_write_begin(purcth_renderer* rdr, purcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    return write_xxx(rdr, endpoint,
            PCRDR_K_OPERATION_WRITEBEGIN,
            PCRDR_OPERATION_WRITEBEGIN,
            msg);
}

static int on_write_more(purcth_renderer* rdr, purcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    return write_xxx(rdr, endpoint,
            PCRDR_K_OPERATION_WRITEMORE,
            PCRDR_OPERATION_WRITEMORE,
            msg);
}

static int on_write_end(purcth_renderer* rdr, purcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    return write_xxx(rdr, endpoint,
            PCRDR_K_OPERATION_WRITEEND,
            PCRDR_OPERATION_WRITEEND,
            msg);
}

static int update_dom(purcth_renderer* rdr, purcth_endpoint* endpoint,
        const pcrdr_msg *msg, int op, const char *op_name)
{
    int retv;
    purcth_dom *dom = NULL;
    pcrdr_msg response = { };

    if (msg->target == PCRDR_MSG_TARGET_DOM) {
        dom = (purcth_dom *)(uintptr_t)msg->targetValue;
    }
    else {
        retv = PCRDR_SC_BAD_REQUEST;
        goto done;
    }

    if (dom == NULL) {
        retv = PCRDR_SC_NOT_FOUND;
        goto done;
    }

    const char *content = NULL;
    size_t content_len = 0;
    if (op != PCRDR_K_OPERATION_ERASE && op != PCRDR_K_OPERATION_CLEAR) {
        if (msg->dataType == PCRDR_MSG_DATA_TYPE_JSON ||
                msg->dataType == PCRDR_MSG_DATA_TYPE_VOID ||
                msg->data == PURC_VARIANT_INVALID) {
            retv = PCRDR_SC_BAD_REQUEST;
            goto done;
        }

        content = purc_variant_get_string_const_ex(msg->data, &content_len);
        if (content == NULL || content_len == 0) {
            retv = PCRDR_SC_BAD_REQUEST;
            goto done;
        }
    }

    const char *element_type = NULL;
    switch (msg->elementType) {
        case PCRDR_MSG_ELEMENT_TYPE_HANDLE:
            element_type = "handle";
            break;
        case PCRDR_MSG_ELEMENT_TYPE_HANDLES:
            element_type = "handles";
            break;
        case PCRDR_MSG_ELEMENT_TYPE_ID:
            element_type = "id";
            break;
        case PCRDR_MSG_ELEMENT_TYPE_CSS:
            element_type = "css";
            break;
        case PCRDR_MSG_ELEMENT_TYPE_XPATH:
            element_type = "xpath";
            break;
        default:
            element_type = "void";
            break;
    }

    const char *element_value = purc_variant_get_string_const(msg->elementValue);
    if (element_type == NULL || element_value == NULL) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto done;
    }

    const char *request_id = purc_variant_get_string_const(msg->requestId);
    retv = rdr->cbs.update_dom(endpoint->session, dom,
            op, op_name, request_id,
            element_type, element_value,
            purc_variant_get_string_const(msg->property),
            msg->dataType, content, content_len);
    if (retv == 0) {
        rdr->cbs.pend_response(endpoint->session,
                purc_variant_get_string_const(msg->operation),
                request_id,
                dom);
        return PCRDR_SC_OK;
    }

done:
    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = purc_variant_ref(msg->requestId);
    response.sourceURI = PURC_VARIANT_INVALID;
    response.retCode = retv;
    response.resultValue = (uint64_t)(uintptr_t)dom;
    response.dataType = PCRDR_MSG_DATA_TYPE_VOID;
    return send_simple_response(rdr, endpoint, &response);
}

static int on_append(purcth_renderer* rdr, purcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    return update_dom(rdr, endpoint, msg,
            PCRDR_K_OPERATION_APPEND,
            PCRDR_OPERATION_APPEND);
}

static int on_prepend(purcth_renderer* rdr, purcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    return update_dom(rdr, endpoint, msg,
            PCRDR_K_OPERATION_PREPEND,
            PCRDR_OPERATION_PREPEND);
}

static int on_insert_after(purcth_renderer* rdr, purcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    return update_dom(rdr, endpoint, msg,
            PCRDR_K_OPERATION_INSERTAFTER,
            PCRDR_OPERATION_INSERTAFTER);
}

static int on_insert_before(purcth_renderer* rdr, purcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    return update_dom(rdr, endpoint, msg,
            PCRDR_K_OPERATION_INSERTBEFORE,
            PCRDR_OPERATION_INSERTBEFORE);
}

static int on_displace(purcth_renderer* rdr, purcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    return update_dom(rdr, endpoint, msg,
            PCRDR_K_OPERATION_DISPLACE,
            PCRDR_OPERATION_DISPLACE);
}

static int on_clear(purcth_renderer* rdr, purcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    return update_dom(rdr, endpoint, msg,
            PCRDR_K_OPERATION_CLEAR,
            PCRDR_OPERATION_CLEAR);
}

static int on_erase(purcth_renderer* rdr, purcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    return update_dom(rdr, endpoint, msg,
            PCRDR_K_OPERATION_ERASE,
            PCRDR_OPERATION_ERASE);
}

static int on_update(purcth_renderer* rdr, purcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    return update_dom(rdr, endpoint, msg,
            PCRDR_K_OPERATION_UPDATE,
            PCRDR_OPERATION_UPDATE);
}

static int on_call_method(purcth_renderer* rdr, purcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    int retv = PCRDR_SC_OK;
    pcrdr_msg response = { };
    purc_variant_t result = PURC_VARIANT_INVALID;

    const char *request_id = purc_variant_get_string_const(msg->requestId);
    const char *method;
    purc_variant_t arg = PURC_VARIANT_INVALID;

    if (msg->dataType != PCRDR_MSG_DATA_TYPE_JSON) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    if ((arg = purc_variant_object_get_by_ckey(msg->data, "method"))) {
        method = purc_variant_get_string_const(arg);
    }
    if (method == NULL) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    arg = purc_variant_object_get_by_ckey(msg->data, "arg");

    const char *element_type = NULL;
    switch (msg->elementType) {
        case PCRDR_MSG_ELEMENT_TYPE_HANDLE:
            element_type = "handle";
            break;
        case PCRDR_MSG_ELEMENT_TYPE_HANDLES:
            element_type = "handles";
            break;
        case PCRDR_MSG_ELEMENT_TYPE_ID:
            element_type = "id";
            break;
        case PCRDR_MSG_ELEMENT_TYPE_CSS:
            element_type = "css";
            break;
        case PCRDR_MSG_ELEMENT_TYPE_XPATH:
            element_type = "xpath";
            break;
        default:
            element_type = "void";
            break;
    }

    const char *element_value;
    element_value = purc_variant_get_string_const(msg->elementValue);

    if (msg->target == PCRDR_MSG_TARGET_DOM) {
        if (rdr->cbs.call_method_in_dom == NULL) {
            retv = PCRDR_SC_NOT_IMPLEMENTED;
            goto failed;
        }

        purcth_dom *dom = NULL;
        dom = (purcth_dom *)(uintptr_t)msg->targetValue;
        if (dom == NULL) {
            retv = PCRDR_SC_BAD_REQUEST;
            goto failed;
        }

        result = rdr->cbs.call_method_in_dom(endpoint->session, request_id,
                dom, element_type, element_value,
                method, arg, &retv);

    }
    else if (msg->target < PCRDR_MSG_TARGET_DOM) {
        if (rdr->cbs.call_method_in_session == NULL) {
            retv = PCRDR_SC_NOT_IMPLEMENTED;
            goto failed;
        }

        result = rdr->cbs.call_method_in_session(endpoint->session,
                msg->target, msg->targetValue,
                element_type, element_value,
                purc_variant_get_string_const(msg->property),
                method, arg, &retv);
    }
    else {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    if (retv == 0) {
        rdr->cbs.pend_response(endpoint->session,
                purc_variant_get_string_const(msg->operation),
                request_id,
                (void *)(uintptr_t)msg->targetValue);
        return PCRDR_SC_OK;
    }

failed:
    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = purc_variant_ref(msg->requestId);
    response.sourceURI = PURC_VARIANT_INVALID;
    response.retCode = retv;
    response.resultValue = msg->targetValue;
    response.dataType = result ?
        PCRDR_MSG_DATA_TYPE_JSON : PCRDR_MSG_DATA_TYPE_VOID;
    response.data = result;

    return send_simple_response(rdr, endpoint, &response);
}

static int on_get_property(purcth_renderer* rdr, purcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    int retv = PCRDR_SC_OK;
    pcrdr_msg response = { };
    purc_variant_t result = PURC_VARIANT_INVALID;

    const char *request_id = purc_variant_get_string_const(msg->requestId);

    if (msg->dataType != PCRDR_MSG_DATA_TYPE_JSON) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    const char *element_type = NULL;
    switch (msg->elementType) {
        case PCRDR_MSG_ELEMENT_TYPE_HANDLE:
            element_type = "handle";
            break;
        case PCRDR_MSG_ELEMENT_TYPE_HANDLES:
            element_type = "handles";
            break;
        case PCRDR_MSG_ELEMENT_TYPE_ID:
            element_type = "id";
            break;
        case PCRDR_MSG_ELEMENT_TYPE_CSS:
            element_type = "css";
            break;
        case PCRDR_MSG_ELEMENT_TYPE_XPATH:
            element_type = "xpath";
            break;
        default:
            element_type = "void";
            break;
    }

    const char *element_value;
    element_value = purc_variant_get_string_const(msg->elementValue);

    const char *property;
    property = purc_variant_get_string_const(msg->property);
    if (property == NULL) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    if (msg->target == PCRDR_MSG_TARGET_DOM) {
        if (rdr->cbs.get_property_in_dom == NULL) {
            retv = PCRDR_SC_NOT_IMPLEMENTED;
            goto failed;
        }

        purcth_dom *dom = NULL;
        dom = (purcth_dom *)(uintptr_t)msg->targetValue;
        if (dom == NULL) {
            retv = PCRDR_SC_BAD_REQUEST;
            goto failed;
        }

        result = rdr->cbs.get_property_in_dom(endpoint->session, request_id,
                dom, element_type, element_value,
                property, &retv);
    }
    else if (msg->target < PCRDR_MSG_TARGET_DOM) {
        if (rdr->cbs.get_property_in_session == NULL) {
            retv = PCRDR_SC_NOT_IMPLEMENTED;
            goto failed;
        }

        result = rdr->cbs.get_property_in_session(endpoint->session,
                msg->target, msg->targetValue,
                element_type, element_value,
                property, &retv);
    }
    else {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    if (retv == 0) {
        rdr->cbs.pend_response(endpoint->session,
                purc_variant_get_string_const(msg->operation),
                request_id,
                (void *)(uintptr_t)msg->targetValue);
        return PCRDR_SC_OK;
    }

failed:
    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = purc_variant_ref(msg->requestId);
    response.sourceURI = PURC_VARIANT_INVALID;
    response.retCode = retv;
    response.resultValue = msg->targetValue;
    response.dataType = result ?
        PCRDR_MSG_DATA_TYPE_JSON : PCRDR_MSG_DATA_TYPE_VOID;
    response.data = result;

    return send_simple_response(rdr, endpoint, &response);
}

static int on_set_property(purcth_renderer* rdr, purcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    int retv = PCRDR_SC_OK;
    pcrdr_msg response = { };
    purc_variant_t result = PURC_VARIANT_INVALID;

    const char *request_id = purc_variant_get_string_const(msg->requestId);

    if (msg->dataType == PCRDR_MSG_DATA_TYPE_VOID) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    const char *element_type = NULL;
    switch (msg->elementType) {
        case PCRDR_MSG_ELEMENT_TYPE_HANDLE:
            element_type = "handle";
            break;
        case PCRDR_MSG_ELEMENT_TYPE_HANDLES:
            element_type = "handles";
            break;
        case PCRDR_MSG_ELEMENT_TYPE_ID:
            element_type = "id";
            break;
        case PCRDR_MSG_ELEMENT_TYPE_CSS:
            element_type = "css";
            break;
        case PCRDR_MSG_ELEMENT_TYPE_XPATH:
            element_type = "xpath";
            break;
        default:
            element_type = "void";
            break;
    }

    const char *element_value;
    element_value = purc_variant_get_string_const(msg->elementValue);

    const char *property;
    property = purc_variant_get_string_const(msg->property);
    if (property == NULL) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    if (msg->target == PCRDR_MSG_TARGET_DOM) {
        if (rdr->cbs.set_property_in_dom == NULL) {
            retv = PCRDR_SC_NOT_IMPLEMENTED;
            goto failed;
        }

        purcth_dom *dom = NULL;
        dom = (purcth_dom *)(uintptr_t)msg->targetValue;
        if (dom == NULL) {
            retv = PCRDR_SC_BAD_REQUEST;
            goto failed;
        }

        result = rdr->cbs.set_property_in_dom(endpoint->session, request_id,
                dom, element_type, element_value,
                property, msg->data, &retv);

    }
    else if (msg->target < PCRDR_MSG_TARGET_DOM) {
        if (rdr->cbs.set_property_in_session == NULL) {
            retv = PCRDR_SC_NOT_IMPLEMENTED;
            goto failed;
        }

        result = rdr->cbs.set_property_in_session(endpoint->session,
                msg->target, msg->targetValue,
                element_type, element_value,
                property, msg->data, &retv);
    }
    else {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    if (retv == 0) {
        rdr->cbs.pend_response(endpoint->session,
                purc_variant_get_string_const(msg->operation),
                request_id,
                (void *)(uintptr_t)msg->targetValue);
        return PCRDR_SC_OK;
    }

failed:
    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = purc_variant_ref(msg->requestId);
    response.sourceURI = PURC_VARIANT_INVALID;
    response.retCode = retv;
    response.resultValue = msg->targetValue;
    response.dataType = result ?
        PCRDR_MSG_DATA_TYPE_JSON : PCRDR_MSG_DATA_TYPE_VOID;
    response.data = result;

    return send_simple_response(rdr, endpoint, &response);
}

static struct request_handler {
    const char *operation;
    request_handler handler;
} handlers[] = {
    { PCRDR_OPERATION_ADDPAGEGROUPS, on_add_page_groups },
    { PCRDR_OPERATION_APPEND, on_append },
    { PCRDR_OPERATION_CALLMETHOD, on_call_method },
    { PCRDR_OPERATION_CLEAR, on_clear },
    { PCRDR_OPERATION_CREATEPLAINWINDOW, on_create_plain_window },
    { PCRDR_OPERATION_CREATEWIDGET, on_create_page },
    { PCRDR_OPERATION_CREATEWORKSPACE, on_create_workspace },
    { PCRDR_OPERATION_DESTROYPLAINWINDOW, on_destroy_plain_window },
    { PCRDR_OPERATION_DESTROYWIDGET, on_destroy_page },
    { PCRDR_OPERATION_DESTROYWORKSPACE, on_destroy_workspace },
    { PCRDR_OPERATION_DISPLACE, on_displace },
    { PCRDR_OPERATION_ENDSESSION, on_end_session },
    { PCRDR_OPERATION_ERASE, on_erase },
    { PCRDR_OPERATION_GETPROPERTY, on_get_property },
    { PCRDR_OPERATION_INSERTAFTER, on_insert_after },
    { PCRDR_OPERATION_INSERTBEFORE, on_insert_before },
    { PCRDR_OPERATION_LOAD, on_load },
    { PCRDR_OPERATION_PREPEND, on_prepend },
    { PCRDR_OPERATION_REMOVEPAGEGROUP, on_remove_page_group },
    { PCRDR_OPERATION_SETPAGEGROUPS, on_set_page_groups },
    { PCRDR_OPERATION_SETPROPERTY, on_set_property },
    { PCRDR_OPERATION_STARTSESSION, on_start_session },
    { PCRDR_OPERATION_UPDATE, on_update },
    { PCRDR_OPERATION_UPDATEPLAINWINDOW, on_update_plain_window },
    { PCRDR_OPERATION_UPDATEWIDGET, on_update_page },
    { PCRDR_OPERATION_UPDATEWORKSPACE, on_update_workspace },
    { PCRDR_OPERATION_WRITEBEGIN, on_write_begin },
    { PCRDR_OPERATION_WRITEEND, on_write_end },
    { PCRDR_OPERATION_WRITEMORE, on_write_more },
};

/* Make sure the number of handlers matches the number of operations */
#define _COMPILE_TIME_ASSERT(name, x)               \
       typedef int _dummy_ ## name[(x) * 2 - 1]
_COMPILE_TIME_ASSERT(hdl,
        sizeof(handlers)/sizeof(handlers[0]) == PCRDR_NR_OPERATIONS);
#undef _COMPILE_TIME_ASSERT

#define NOT_FOUND_HANDLER   ((request_handler)-1)

static request_handler find_request_handler(const char* operation)
{
    static ssize_t max = sizeof(handlers)/sizeof(handlers[0]) - 1;

    ssize_t low = 0, high = max, mid;
    while (low <= high) {
        int cmp;

        mid = (low + high) / 2;
        cmp = strcasecmp(operation, handlers[mid].operation);
        if (cmp == 0) {
            goto found;
        }
        else {
            if (cmp < 0) {
                high = mid - 1;
            }
            else {
                low = mid + 1;
            }
        }
    }

    return NOT_FOUND_HANDLER;

found:
    return handlers[mid].handler;
}

int on_got_message(purcth_renderer* rdr, purcth_endpoint* endpoint, const pcrdr_msg *msg)
{
    if (msg->type == PCRDR_MSG_TYPE_REQUEST) {
        request_handler handler = find_request_handler(
                purc_variant_get_string_const(msg->operation));

        purc_log_info("Got a request message: %s (handler: %p)\n",
                purc_variant_get_string_const(msg->operation), handler);

        if (handler == NOT_FOUND_HANDLER) {
            pcrdr_msg response = { };
            response.type = PCRDR_MSG_TYPE_RESPONSE;
            response.requestId = purc_variant_ref(msg->requestId);
            response.sourceURI = PURC_VARIANT_INVALID;
            response.retCode = PCRDR_SC_BAD_REQUEST;
            response.resultValue = 0;
            response.dataType = PCRDR_MSG_DATA_TYPE_VOID;

            return send_simple_response(rdr, endpoint, &response);
        }
        else if (handler) {
            return handler(rdr, endpoint, msg);
        }
        else {
            pcrdr_msg response = { };
            response.type = PCRDR_MSG_TYPE_RESPONSE;
            response.requestId = purc_variant_ref(msg->requestId);
            response.sourceURI = PURC_VARIANT_INVALID;
            response.retCode = PCRDR_SC_NOT_IMPLEMENTED;
            response.resultValue = 0;
            response.dataType = PCRDR_MSG_DATA_TYPE_VOID;

            return send_simple_response(rdr, endpoint, &response);
        }
    }
    else if (msg->type == PCRDR_MSG_TYPE_EVENT) {
        // TODO
        purc_log_info("Got an event message: %s\n",
                purc_variant_get_string_const(msg->eventName));
    }
    else {
        // TODO
        purc_log_info("Got an unknown message: %d\n", msg->type);
    }

    return PCRDR_SC_OK;
}

