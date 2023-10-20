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

#undef NDEBUG

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include <purc/purc.h>

#include "endpoint.h"
#include "util/kvlist.h"

struct pcmcth_endpoint {
    time_t  t_created;
    time_t  t_living;

    purc_atom_t rid;
    const char* uri;

    pcmcth_session *session;

    /* AVL node for the AVL tree sorted by living time */
    struct avl_node avl;
};

purc_atom_t get_endpoint_rid(pcmcth_endpoint* endpoint)
{
    return endpoint->rid;
}

const char *get_endpoint_uri(pcmcth_endpoint* endpoint)
{
    return endpoint->uri;
}

int
comp_living_time(const void *k1, const void *k2, void *ptr)
{
    const pcmcth_endpoint *e1 = k1;
    const pcmcth_endpoint *e2 = k2;

    (void)ptr;
    return e1->t_living - e2->t_living;
}

void remove_all_living_endpoints(struct avl_tree *avl)
{
    pcmcth_endpoint *endpoint, *tmp;

    avl_remove_all_elements(avl, endpoint, avl, tmp) {
        // TODO:
    }
}

pcmcth_endpoint* retrieve_endpoint(pcmcth_renderer* rdr, const char *uri)
{
    void *data;
    data = kvlist_get(&rdr->endpoint_list, uri);
    if (data == NULL)
        return NULL;

    return *(pcmcth_endpoint **)data;
}

pcmcth_endpoint* new_endpoint(pcmcth_renderer* rdr, const char *uri)
{
    int ec = PCRDR_SUCCESS;
    pcmcth_endpoint* endpoint = NULL;

    if (retrieve_endpoint(rdr, uri)) {
        ec = PCRDR_ERROR_DUPLICATED;
        goto failed;
    }

    purc_atom_t rid = purc_atom_try_string_ex(
            PURC_ATOM_BUCKET_DEF, uri);
    if (rid == 0) {
        ec = PCRDR_ERROR_INVALID_VALUE;
        goto failed;
    }

    endpoint = (pcmcth_endpoint *)calloc(1, sizeof(pcmcth_endpoint));
    if (endpoint == NULL) {
        ec = PCRDR_ERROR_NOMEM;
        goto failed;
    }

    endpoint->t_created = purc_get_monotoic_time();
    endpoint->t_living = endpoint->t_created;
    endpoint->avl.key = endpoint;
    endpoint->rid = rid;

    if (!(endpoint->uri = kvlist_set_ex(&rdr->endpoint_list, uri, &endpoint))) {
        ec = PCRDR_ERROR_NOMEM;
        goto failed;
    }

    if (avl_insert(&rdr->living_avl, &endpoint->avl)) {
        ec = PCRDR_ERROR_NOMEM;
        goto failed;
    }

    rdr->nr_endpoints++;
    if (rdr->master_rid == 0) {
        rdr->master_rid = rid;
    }
    return endpoint;

failed:
    if (endpoint) {
        if (endpoint->uri)
            kvlist_delete(&rdr->endpoint_list, endpoint->uri);
        free(endpoint);
    }

    purc_set_error(ec);
    return NULL;
}

int del_endpoint(pcmcth_renderer* rdr, pcmcth_endpoint* endpoint, int cause)
{
    (void)cause;

    if (endpoint->session) {
        rdr->cbs.remove_session(endpoint->session);
        endpoint->session = NULL;
    }

    avl_delete(&rdr->living_avl, &endpoint->avl);

    purc_log_info("Removing endpoint (%s)\n", endpoint->uri);
    kvlist_delete(&rdr->endpoint_list, endpoint->uri);
    free(endpoint);
    rdr->nr_endpoints--;
    return 0;
}

void
update_endpoint_living_time(pcmcth_renderer *rdr, pcmcth_endpoint* endpoint)
{
    time_t t_curr = purc_get_monotoic_time();

    if (UNLIKELY(endpoint->t_living != t_curr)) {
        endpoint->t_living = t_curr;
        avl_delete(&rdr->living_avl, &endpoint->avl);
        avl_insert(&rdr->living_avl, &endpoint->avl);
    }
}

int check_no_responding_endpoints(pcmcth_renderer *rdr)
{
    int n = 0;
    time_t t_curr = purc_get_monotoic_time();
    pcmcth_endpoint *endpoint, *tmp;

    purc_log_info ("Checking no responding endpoints...\n");

    avl_for_each_element_safe (&rdr->living_avl, endpoint, avl, tmp) {
        if (t_curr > endpoint->t_living + PCRDR_MAX_NO_RESPONDING_TIME) {

            purc_log_info("Removing no-responding client: %s\n", endpoint->uri);
            del_endpoint(rdr, endpoint, CDE_NO_RESPONDING);
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

static int send_simple_response(pcmcth_renderer* rdr, pcmcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    (void)rdr;
    int ret = PCRDR_SC_OK;
    pcrdr_msg *my_msg = pcrdr_clone_message(msg);
    if (my_msg == NULL) {
        ret = PCRDR_SC_INSUFFICIENT_STORAGE;
        goto out;
    }

    if (purc_inst_move_message(endpoint->rid, my_msg) == 0) {
        LOG_ERROR("Failed to move message to %u\n", endpoint->rid);
        ret = PCRDR_SC_INTERNAL_SERVER_ERROR;
    }

    pcrdr_release_message(my_msg);

out:
    if (msg->data) {
        purc_variant_unref(msg->data);
    }
    return ret;
}

int send_initial_response(pcmcth_renderer* rdr, pcmcth_endpoint* endpoint)
{
    (void)rdr;
    int retv = PCRDR_SC_OK;
    pcrdr_msg *msg = NULL;

    msg = pcrdr_make_response_message(PCRDR_REQUESTID_INITIAL, NULL,
            PCRDR_SC_OK, 0,
            PCRDR_MSG_DATA_TYPE_PLAIN, rdr->features, rdr->len_features);
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

typedef int (*request_handler)(pcmcth_renderer* rdr, pcmcth_endpoint* endpoint,
        const pcrdr_msg *msg);

static int on_start_session(pcmcth_renderer* rdr, pcmcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    pcrdr_msg response = { };
    pcmcth_session *info = NULL;

    LOG_DEBUG("called (time %lu)\n", time(NULL));
    endpoint->session = NULL;
    int retv;
    info = rdr->cbs.create_session(rdr, endpoint);
    if (info == NULL) {
        retv = PCRDR_SC_INSUFFICIENT_STORAGE;
    }
    else {
        retv = PCRDR_SC_OK;
        endpoint->session = info;
    }

    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = msg->requestId;
    response.sourceURI = PURC_VARIANT_INVALID;
    response.retCode = retv;
    response.resultValue = (uint64_t)info;
    response.dataType = PCRDR_MSG_DATA_TYPE_VOID;

    return send_simple_response(rdr, endpoint, &response);
}

static int on_end_session(pcmcth_renderer* rdr, pcmcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    pcrdr_msg response = { };

    if (endpoint->session) {
        rdr->cbs.remove_session(endpoint->session);
        endpoint->session = NULL;
    }

    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = msg->requestId;
    response.sourceURI = PURC_VARIANT_INVALID;
    response.retCode = PCRDR_SC_OK;
    response.resultValue = 0;
    response.dataType = PCRDR_MSG_DATA_TYPE_VOID;

    return send_simple_response(rdr, endpoint, &response);
}

static int on_create_workspace(pcmcth_renderer* rdr, pcmcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    int retv = PCRDR_SC_OK;
    pcrdr_msg response = { };
    pcmcth_workspace* workspace = NULL;

    if (rdr->cbs.create_workspace == NULL) {
        retv = PCRDR_SC_NOT_IMPLEMENTED;
        goto done;
    }

    /* Since PURCMC-120: use element for the name of worksapce */
    if (msg->elementType != PCRDR_MSG_ELEMENT_TYPE_ID) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto done;
    }

    const char *name = purc_variant_get_string_const(msg->elementValue);
    if (name == NULL) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto done;
    }

    if (name[0] == '_') {    // reserved name
        int v = pcrdr_check_reserved_workspace_name(name);
        if (v < 0) {
            retv = PCRDR_SC_BAD_REQUEST;
            goto done;
        }

        if (rdr->cbs.get_special_workspace == NULL) {
            retv = PCRDR_SC_NOT_IMPLEMENTED;
            goto done;
        }

        retv = PCRDR_SC_OK;
        workspace = rdr->cbs.get_special_workspace(endpoint->session,
                (pcrdr_resname_workspace_k)v);
        goto done;
    }

    if (rdr->cbs.find_workspace) {
        workspace = rdr->cbs.find_workspace(endpoint->session, name);
        if (workspace) {
            retv = PCRDR_SC_OK;
            goto done;
        }
    }

    const char* title = NULL;
    purc_variant_t tmp;

    if (msg->dataType != PCRDR_MSG_DATA_TYPE_JSON ||
            !purc_variant_is_object(msg->data)) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto done;
    }

    if ((tmp = purc_variant_object_get_by_ckey(msg->data, "title"))) {
        title = purc_variant_get_string_const(tmp);
    }

    workspace = rdr->cbs.create_workspace(endpoint->session,
            name, title, msg->data, &retv);

done:
    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = msg->requestId;
    response.sourceURI = PURC_VARIANT_INVALID;
    response.retCode = retv;
    response.resultValue = (uint64_t)(uintptr_t)workspace;
    response.dataType = PCRDR_MSG_DATA_TYPE_VOID;

    return send_simple_response(rdr, endpoint, &response);
}

static int on_update_workspace(pcmcth_renderer* rdr, pcmcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    int retv = PCRDR_SC_OK;
    pcmcth_workspace *workspace = NULL;
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
        workspace = (pcmcth_workspace *)(uintptr_t)handle;
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

failed:
    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = msg->requestId;
    response.sourceURI = PURC_VARIANT_INVALID;
    response.retCode = retv;
    response.resultValue = (uint64_t)(uintptr_t)workspace;
    response.dataType = PCRDR_MSG_DATA_TYPE_VOID;

    return send_simple_response(rdr, endpoint, &response);
}

static int on_destroy_workspace(pcmcth_renderer* rdr, pcmcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    int retv = PCRDR_SC_OK;
    pcmcth_workspace *workspace = NULL;
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
        workspace = (pcmcth_workspace *)(uintptr_t)handle;
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

failed:
    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = msg->requestId;
    response.sourceURI = PURC_VARIANT_INVALID;
    response.retCode = retv;
    response.resultValue = (uint64_t)(uintptr_t)workspace;
    response.dataType = PCRDR_MSG_DATA_TYPE_VOID;

    return send_simple_response(rdr, endpoint, &response);
}

static int on_set_page_groups(pcmcth_renderer* rdr, pcmcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    int retv = PCRDR_SC_OK;
    pcrdr_msg response = { };
    pcmcth_workspace* workspace = NULL;

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

failed:
    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = msg->requestId;
    response.sourceURI = PURC_VARIANT_INVALID;
    response.retCode = retv;
    response.resultValue = (uint64_t)(uintptr_t)workspace;
    response.dataType = PCRDR_MSG_DATA_TYPE_VOID;

    return send_simple_response(rdr, endpoint, &response);
}

static int on_add_page_groups(pcmcth_renderer* rdr, pcmcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    int retv = PCRDR_SC_OK;
    pcmcth_workspace *workspace = NULL;
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

failed:
    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = msg->requestId;
    response.sourceURI = PURC_VARIANT_INVALID;
    response.retCode = retv;
    response.resultValue = (uint64_t)(uintptr_t)workspace;
    response.dataType = PCRDR_MSG_DATA_TYPE_VOID;

    return send_simple_response(rdr, endpoint, &response);
}

static int on_remove_page_group(pcmcth_renderer* rdr, pcmcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    int retv = PCRDR_SC_OK;
    pcmcth_workspace *workspace = NULL;
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

failed:
    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = msg->requestId;
    response.sourceURI = PURC_VARIANT_INVALID;
    response.retCode = retv;
    response.resultValue = (uint64_t)(uintptr_t)workspace;
    response.dataType = PCRDR_MSG_DATA_TYPE_VOID;

    return send_simple_response(rdr, endpoint, &response);
}

static int on_create_plain_window(pcmcth_renderer* rdr, pcmcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    int retv = PCRDR_SC_OK;
    pcrdr_msg response = { };

    pcmcth_workspace* workspace = NULL;
    pcmcth_page* win = NULL;

    if (msg->target == PCRDR_MSG_TARGET_WORKSPACE) {
        workspace = (void *)(uintptr_t)msg->targetValue;
    }
    else {
        retv = PCRDR_SC_BAD_REQUEST;
        goto done;
    }

    /* Since PURCMC-120, use element to specify the window name and group name:
        <window_name>[@<group_name>]
     */
    const char *name_group = NULL;
    if (msg->elementType == PCRDR_MSG_ELEMENT_TYPE_ID) {
        name_group = purc_variant_get_string_const(msg->elementValue);
    }

    if (name_group == NULL) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto done;
    }

    char idbuf[PURC_MAX_WIDGET_ID];
    char name[PURC_LEN_IDENTIFIER + 1];
    const char *group;
    group = purc_check_and_make_plainwin_id(idbuf, name, name_group);
    if (group == PURC_INVPTR) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto done;
    }

    /* Since PURCMC-120, support the special page name. */
    if (name[0] == '_') {    // reserved name
        int v = pcrdr_check_reserved_page_name(name);
        if (v < 0) {
            retv = PCRDR_SC_BAD_REQUEST;
            goto done;
        }

        /* support for reserved name */
        if (rdr->cbs.get_special_plainwin) {
            win = rdr->cbs.get_special_plainwin(endpoint->session, workspace,
                    group, (pcrdr_resname_page_k)v);
            retv = PCRDR_SC_OK;
            goto done;
        }
    }

    win = rdr->cbs.find_page(endpoint->session, workspace, idbuf);
    if (win) {
        retv = PCRDR_SC_OK;
        goto done;
    }

    purc_variant_t tmp;
    const char* class = NULL;
    const char* title = NULL;
    const char* layout_style = NULL;
    purc_variant_t toolkit_style = PURC_VARIANT_INVALID;

    if (msg->dataType == PCRDR_MSG_DATA_TYPE_JSON
           && purc_variant_is_object(msg->data)) {
        if ((tmp = purc_variant_object_get_by_ckey(msg->data, "class"))) {
            class = purc_variant_get_string_const(tmp);
        }

        if ((tmp = purc_variant_object_get_by_ckey(msg->data, "title"))) {
            title = purc_variant_get_string_const(tmp);
        }

        if ((tmp = purc_variant_object_get_by_ckey(msg->data,
                        "layoutStyle"))) {
            layout_style = purc_variant_get_string_const(tmp);
        }

        toolkit_style =
            purc_variant_object_get_by_ckey(msg->data, "toolkitStyle");
    }

    win = rdr->cbs.create_plainwin(endpoint->session, workspace,
            idbuf, group, name, class, title, layout_style,
            toolkit_style, &retv);

done:
    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = msg->requestId;
    response.sourceURI = PURC_VARIANT_INVALID;
    response.retCode = retv;
    response.resultValue = (uint64_t)(uintptr_t)win;
    response.dataType = PCRDR_MSG_DATA_TYPE_VOID;

    return send_simple_response(rdr, endpoint, &response);
}

static int on_update_plain_window(pcmcth_renderer* rdr, pcmcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    int retv = PCRDR_SC_OK;
    pcmcth_workspace *workspace = NULL;
    pcmcth_page *win = NULL;
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

failed:
    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = msg->requestId;
    response.sourceURI = PURC_VARIANT_INVALID;
    response.retCode = retv;
    response.resultValue = (uint64_t)win;
    response.dataType = PCRDR_MSG_DATA_TYPE_VOID;

    return send_simple_response(rdr, endpoint, &response);
}

static int
on_destroy_plain_window(pcmcth_renderer* rdr, pcmcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    int retv = PCRDR_SC_OK;
    pcmcth_workspace *workspace = NULL;
    pcmcth_page *win = NULL;
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
        win = (pcmcth_page *)(uintptr_t)handle;
    }

    if (win == NULL) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    retv = rdr->cbs.destroy_plainwin(endpoint->session, workspace, win);

failed:
    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = msg->requestId;
    response.sourceURI = PURC_VARIANT_INVALID;
    response.retCode = retv;
    response.resultValue = (uint64_t)(uintptr_t)win;
    response.dataType = PCRDR_MSG_DATA_TYPE_VOID;

    return send_simple_response(rdr, endpoint, &response);
}

static int on_create_widget(pcmcth_renderer* rdr, pcmcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    int retv = PCRDR_SC_OK;
    pcrdr_msg response = { };
    pcmcth_workspace* workspace = NULL;
    pcmcth_page* page = NULL;

    if (rdr->cbs.create_widget == NULL) {
        retv = PCRDR_SC_NOT_IMPLEMENTED;
        goto done;
    }

    if (msg->target == PCRDR_MSG_TARGET_WORKSPACE) {
        workspace = (void *)(uintptr_t)msg->targetValue;
    }
    else {
        retv = PCRDR_SC_BAD_REQUEST;
        goto done;
    }

    if (msg->dataType != PCRDR_MSG_DATA_TYPE_JSON ||
            !purc_variant_is_object(msg->data)) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto done;
    }

    const char* name_group = NULL;
    /* Since PURCMC-120, use element to specify the widget name and group name:
            <widget_name>@<group_name>
     */
    if (msg->elementType == PCRDR_MSG_ELEMENT_TYPE_ID) {
        name_group = purc_variant_get_string_const(msg->elementValue);
    }

    if (name_group == NULL) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto done;
    }

    char idbuf[PURC_MAX_WIDGET_ID];
    char name[PURC_LEN_IDENTIFIER + 1];
    const char *group = purc_check_and_make_widget_id(idbuf, name, name_group);
    if (group == NULL) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto done;
    }

    /* Since PURCMC-120, support the special page name. */
    if (name[0] == '_') {    // reserved name
        int v = pcrdr_check_reserved_page_name(name);
        if (v < 0) {
            retv = PCRDR_SC_BAD_REQUEST;
            goto done;
        }

        if (rdr->cbs.get_special_widget) {
            page = rdr->cbs.get_special_widget(endpoint->session, workspace,
                    group, (pcrdr_resname_page_k)v);
            retv = PCRDR_SC_OK;
            goto done;
        }
    }

    /* Since PURCMC-120, returns the page if it exists already. */
    page = rdr->cbs.find_page(endpoint->session, workspace, idbuf);
    if (page) {
        retv = PCRDR_SC_OK;
        goto done;
    }

    const char* class = NULL;
    const char* title = NULL;
    const char* layout_style = NULL;
    purc_variant_t toolkit_style;
    purc_variant_t tmp;

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

    page = rdr->cbs.create_widget(endpoint->session, workspace,
            idbuf, group, name, class, title, layout_style,
            toolkit_style, &retv);

done:
    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = msg->requestId;
    response.sourceURI = PURC_VARIANT_INVALID;
    response.retCode = retv;
    response.resultValue = (uint64_t)(uintptr_t)page;
    response.dataType = PCRDR_MSG_DATA_TYPE_VOID;

    return send_simple_response(rdr, endpoint, &response);
}

static int on_update_widget(pcmcth_renderer* rdr, pcmcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    int retv = PCRDR_SC_OK;
    pcmcth_workspace *workspace = NULL;
    pcmcth_page *page = NULL;
    pcrdr_msg response = { };

    if (rdr->cbs.create_widget == NULL || rdr->cbs.update_widget == NULL) {
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

    retv = rdr->cbs.update_widget(endpoint->session, workspace,
            page, property, msg->data);

failed:
    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = msg->requestId;
    response.sourceURI = PURC_VARIANT_INVALID;
    response.retCode = retv;
    response.resultValue = (uint64_t)(uintptr_t)page;
    response.dataType = PCRDR_MSG_DATA_TYPE_VOID;

    return send_simple_response(rdr, endpoint, &response);
}

static int on_destroy_widget(pcmcth_renderer* rdr, pcmcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    int retv = PCRDR_SC_OK;
    pcmcth_workspace *workspace = NULL;
    pcmcth_page *page = NULL;
    pcrdr_msg response = { };

    if (rdr->cbs.create_widget == NULL || rdr->cbs.destroy_widget == NULL) {
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

    retv = rdr->cbs.destroy_widget(endpoint->session, workspace, page);

failed:
    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = msg->requestId;
    response.sourceURI = PURC_VARIANT_INVALID;
    response.retCode = retv;
    response.resultValue = (uint64_t)(uintptr_t)page;
    response.dataType = PCRDR_MSG_DATA_TYPE_VOID;

    return send_simple_response(rdr, endpoint, &response);
}

#define LEN_BUFF_LONGLONGINT 128

static int on_load(pcmcth_renderer* rdr, pcmcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    pcrdr_msg response = { };
    int retv = PCRDR_SC_OK;
    pcmcth_page *page = NULL;
    pcmcth_udom *dom = NULL;
    char suppressed[LEN_BUFF_LONGLONGINT] = { };

    void *edom;
    if (msg->data == PURC_VARIANT_INVALID ||
            !purc_variant_is_native(msg->data) ||
            (edom = purc_variant_native_get_entity(msg->data)) == NULL) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    if (msg->target == PCRDR_MSG_TARGET_PLAINWINDOW ||
            msg->target == PCRDR_MSG_TARGET_WIDGET) {
        page = (void *)(uintptr_t)msg->targetValue;
    }

    if (page == NULL) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    /* Since PURCMC-120, pass the coroutine handle */
    if (msg->elementType != PCRDR_MSG_ELEMENT_TYPE_HANDLE) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    uint64_t crtn = (uint64_t)strtoull(
            purc_variant_get_string_const(msg->elementValue), NULL, 16);
    if (crtn == 0) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }
    dom = rdr->cbs.load_edom(endpoint->session, page, msg->data,
            crtn, suppressed, &retv);

failed:
    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = msg->requestId;
    response.sourceURI = PURC_VARIANT_INVALID;
    response.retCode = retv;
    response.resultValue = (uint64_t)(uintptr_t)dom;
    if (suppressed[0]) {
        response.dataType = PCRDR_MSG_DATA_TYPE_PLAIN;
        response.data = purc_variant_make_string(suppressed, false);
    }
    else {
        response.dataType = PCRDR_MSG_DATA_TYPE_VOID;
    }

    return send_simple_response(rdr, endpoint, &response);
}

static int on_register(pcmcth_renderer* rdr, pcmcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    pcrdr_msg response = { };
    int retv = PCRDR_SC_OK;
    pcmcth_page *page = 0;
    uint64_t suppressed = 0;

    if (msg->target == PCRDR_MSG_TARGET_PLAINWINDOW ||
            msg->target == PCRDR_MSG_TARGET_WIDGET) {
        page = (void *)(uintptr_t)msg->targetValue;
    }

    if (page == NULL) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    if (msg->elementType != PCRDR_MSG_ELEMENT_TYPE_HANDLE) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    uint64_t crtn = (uint64_t)strtoull(
            purc_variant_get_string_const(msg->elementValue), NULL, 16);
    if (crtn == 0) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    suppressed = rdr->cbs.register_crtn(endpoint->session, page, crtn, &retv);

failed:
    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = msg->requestId;
    response.sourceURI = PURC_VARIANT_INVALID;
    response.retCode = retv;
    response.resultValue = suppressed;
    response.dataType = PCRDR_MSG_DATA_TYPE_VOID;

    return send_simple_response(rdr, endpoint, &response);
}

static int on_revoke(pcmcth_renderer* rdr, pcmcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    pcrdr_msg response = { };
    int retv = PCRDR_SC_OK;
    pcmcth_page *page = 0;
    uint64_t to_reload = 0;

    if (msg->target == PCRDR_MSG_TARGET_PLAINWINDOW ||
            msg->target == PCRDR_MSG_TARGET_WIDGET) {
        page = (void *)(uintptr_t)msg->targetValue;
    }

    if (page == NULL) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    if (msg->elementType != PCRDR_MSG_ELEMENT_TYPE_HANDLE) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    uint64_t crtn = (uint64_t)strtoull(
            purc_variant_get_string_const(msg->elementValue), NULL, 16);
    if (crtn == 0) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    to_reload = rdr->cbs.revoke_crtn(endpoint->session, page, crtn, &retv);

failed:
    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = msg->requestId;
    response.sourceURI = PURC_VARIANT_INVALID;
    response.retCode = retv;
    response.resultValue = to_reload;
    response.dataType = PCRDR_MSG_DATA_TYPE_VOID;

    return send_simple_response(rdr, endpoint, &response);
}

static int update_dom(pcmcth_renderer* rdr, pcmcth_endpoint* endpoint,
        const pcrdr_msg *msg, int op)
{
    int retv;
    pcmcth_udom *dom = NULL;
    pcrdr_msg response = { };

    if (msg->target == PCRDR_MSG_TARGET_DOM) {
        dom = (pcmcth_udom *)(uintptr_t)msg->targetValue;
    }
    else {
        retv = PCRDR_SC_BAD_REQUEST;
        goto done;
    }

    if (dom == NULL) {
        retv = PCRDR_SC_NOT_FOUND;
        goto done;
    }

    uint64_t element_handle = 0;
    if (msg->elementType == PCRDR_MSG_ELEMENT_TYPE_HANDLE) {
        const char *element_value =
            purc_variant_get_string_const(msg->elementValue);
        if (element_value == NULL) {
            retv = PCRDR_SC_BAD_REQUEST;
            goto done;
        }

        element_handle = strtoull(element_value, NULL, 16);
    }

    if (msg->data != PURC_VARIANT_INVALID &&
            !purc_variant_is_native(msg->data)) {
        LOG_DEBUG("Not a native entity for message data: %p\n", msg->data);
        retv = PCRDR_SC_BAD_REQUEST;
        goto done;
    }

    retv = rdr->cbs.update_udom(endpoint->session, dom,
            op, element_handle, purc_variant_get_string_const(msg->property),
            msg->data);

done:
    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = msg->requestId;
    response.sourceURI = PURC_VARIANT_INVALID;
    response.retCode = retv;
    response.resultValue = (uint64_t)(uintptr_t)dom;
    response.dataType = PCRDR_MSG_DATA_TYPE_VOID;
    return send_simple_response(rdr, endpoint, &response);
}

static int on_append(pcmcth_renderer* rdr, pcmcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    return update_dom(rdr, endpoint, msg, PCRDR_K_OPERATION_APPEND);
}

static int on_prepend(pcmcth_renderer* rdr, pcmcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    return update_dom(rdr, endpoint, msg, PCRDR_K_OPERATION_PREPEND);
}

static int on_insert_after(pcmcth_renderer* rdr, pcmcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    return update_dom(rdr, endpoint, msg, PCRDR_K_OPERATION_INSERTAFTER);
}

static int on_insert_before(pcmcth_renderer* rdr, pcmcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    return update_dom(rdr, endpoint, msg, PCRDR_K_OPERATION_INSERTBEFORE);
}

static int on_displace(pcmcth_renderer* rdr, pcmcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    return update_dom(rdr, endpoint, msg, PCRDR_K_OPERATION_DISPLACE);
}

static int on_clear(pcmcth_renderer* rdr, pcmcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    return update_dom(rdr, endpoint, msg, PCRDR_K_OPERATION_CLEAR);
}

static int on_erase(pcmcth_renderer* rdr, pcmcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    return update_dom(rdr, endpoint, msg, PCRDR_K_OPERATION_ERASE);
}

static int on_update(pcmcth_renderer* rdr, pcmcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    return update_dom(rdr, endpoint, msg, PCRDR_K_OPERATION_UPDATE);
}

static int on_call_method(pcmcth_renderer* rdr, pcmcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    int retv = PCRDR_SC_OK;
    pcrdr_msg response = { };
    purc_variant_t result = PURC_VARIANT_INVALID;

    const char *method = NULL;
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
    const char *element_value;
    element_value = purc_variant_get_string_const(msg->elementValue);

    if (msg->target == PCRDR_MSG_TARGET_DOM) {
        if (rdr->cbs.call_method_in_udom == NULL) {
            retv = PCRDR_SC_NOT_IMPLEMENTED;
            goto failed;
        }

        pcmcth_udom *dom = NULL;
        dom = (pcmcth_udom *)(uintptr_t)msg->targetValue;
        if (dom == NULL) {
            retv = PCRDR_SC_BAD_REQUEST;
            goto failed;
        }

        uint64_t element_handle = 0;
        if (msg->elementType != PCRDR_MSG_ELEMENT_TYPE_HANDLE ||
                element_value == NULL) {
            retv = PCRDR_SC_BAD_REQUEST;
            goto failed;
        }

        element_handle = strtoull(element_value, NULL, 16);
        result = rdr->cbs.call_method_in_udom(endpoint->session,
                dom, element_handle, method, arg, &retv);

    }
    else if (msg->target < PCRDR_MSG_TARGET_DOM) {
        if (rdr->cbs.call_method_in_session == NULL) {
            retv = PCRDR_SC_NOT_IMPLEMENTED;
            goto failed;
        }

        result = rdr->cbs.call_method_in_session(endpoint->session,
                msg->target, msg->targetValue,
                msg->elementType, element_value,
                purc_variant_get_string_const(msg->property),
                method, arg, &retv);
    }
    else {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

failed:
    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = msg->requestId;
    response.sourceURI = PURC_VARIANT_INVALID;
    response.retCode = retv;
    response.resultValue = msg->targetValue;
    response.dataType = result ?
        PCRDR_MSG_DATA_TYPE_JSON : PCRDR_MSG_DATA_TYPE_VOID;
    response.data = result;

    return send_simple_response(rdr, endpoint, &response);
}

static int on_get_property(pcmcth_renderer* rdr, pcmcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    int retv = PCRDR_SC_OK;
    pcrdr_msg response = { };
    purc_variant_t result = PURC_VARIANT_INVALID;

    if (msg->dataType != PCRDR_MSG_DATA_TYPE_JSON) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
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
        if (rdr->cbs.get_property_in_udom == NULL) {
            retv = PCRDR_SC_NOT_IMPLEMENTED;
            goto failed;
        }

        pcmcth_udom *dom = NULL;
        dom = (pcmcth_udom *)(uintptr_t)msg->targetValue;
        if (dom == NULL) {
            retv = PCRDR_SC_BAD_REQUEST;
            goto failed;
        }

        uint64_t element_handle = 0;
        if (msg->elementType != PCRDR_MSG_ELEMENT_TYPE_HANDLE ||
                element_value == NULL) {
            retv = PCRDR_SC_BAD_REQUEST;
            goto failed;
        }

        element_handle = strtoull(element_value, NULL, 16);
        result = rdr->cbs.get_property_in_udom(endpoint->session,
                dom, element_handle, property, &retv);
    }
    else if (msg->target < PCRDR_MSG_TARGET_DOM) {
        if (rdr->cbs.get_property_in_session == NULL) {
            retv = PCRDR_SC_NOT_IMPLEMENTED;
            goto failed;
        }

        result = rdr->cbs.get_property_in_session(endpoint->session,
                msg->target, msg->targetValue,
                msg->elementType, element_value,
                property, &retv);
    }
    else {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

failed:
    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = msg->requestId;
    response.sourceURI = PURC_VARIANT_INVALID;
    response.retCode = retv;
    response.resultValue = msg->targetValue;
    response.dataType = result ?
        PCRDR_MSG_DATA_TYPE_JSON : PCRDR_MSG_DATA_TYPE_VOID;
    response.data = result;

    return send_simple_response(rdr, endpoint, &response);
}

static int on_set_property(pcmcth_renderer* rdr, pcmcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    int retv = PCRDR_SC_OK;
    pcrdr_msg response = { };
    purc_variant_t result = PURC_VARIANT_INVALID;

    if (msg->dataType == PCRDR_MSG_DATA_TYPE_VOID) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
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
        if (rdr->cbs.set_property_in_udom == NULL) {
            retv = PCRDR_SC_NOT_IMPLEMENTED;
            goto failed;
        }

        pcmcth_udom *dom = NULL;
        dom = (pcmcth_udom *)(uintptr_t)msg->targetValue;
        if (dom == NULL) {
            retv = PCRDR_SC_BAD_REQUEST;
            goto failed;
        }

        uint64_t element_handle = 0;
        if (msg->elementType != PCRDR_MSG_ELEMENT_TYPE_HANDLE ||
                element_value == NULL) {
            retv = PCRDR_SC_BAD_REQUEST;
            goto failed;
        }

        element_handle = strtoull(element_value, NULL, 16);
        result = rdr->cbs.set_property_in_udom(endpoint->session,
                dom, element_handle, property, msg->data, &retv);

    }
    else if (msg->target < PCRDR_MSG_TARGET_DOM) {
        if (rdr->cbs.set_property_in_session == NULL) {
            retv = PCRDR_SC_NOT_IMPLEMENTED;
            goto failed;
        }

        result = rdr->cbs.set_property_in_session(endpoint->session,
                msg->target, msg->targetValue,
                msg->elementType, element_value,
                property, msg->data, &retv);
    }
    else {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

failed:
    response.type = PCRDR_MSG_TYPE_RESPONSE;
    response.requestId = msg->requestId;
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
    { PCRDR_OPERATION_AUTHENTICATE, NULL },
    { PCRDR_OPERATION_CALLMETHOD, on_call_method },
    { PCRDR_OPERATION_CLEAR, on_clear },
    { PCRDR_OPERATION_CREATEPLAINWINDOW, on_create_plain_window },
    { PCRDR_OPERATION_CREATEWIDGET, on_create_widget },
    { PCRDR_OPERATION_CREATEWORKSPACE, on_create_workspace },
    { PCRDR_OPERATION_DESTROYPLAINWINDOW, on_destroy_plain_window },
    { PCRDR_OPERATION_DESTROYWIDGET, on_destroy_widget },
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
    { PCRDR_OPERATION_REGISTER, on_register },
    { PCRDR_OPERATION_REVOKE, on_revoke },
    { PCRDR_OPERATION_SETPAGEGROUPS, on_set_page_groups },
    { PCRDR_OPERATION_SETPROPERTY, on_set_property },
    { PCRDR_OPERATION_STARTSESSION, on_start_session },
    { PCRDR_OPERATION_UPDATE, on_update },
    { PCRDR_OPERATION_UPDATEPLAINWINDOW, on_update_plain_window },
    { PCRDR_OPERATION_UPDATEWIDGET, on_update_widget },
    { PCRDR_OPERATION_UPDATEWORKSPACE, on_update_workspace },
    { PCRDR_OPERATION_WRITEBEGIN, NULL },
    { PCRDR_OPERATION_WRITEEND, NULL },
    { PCRDR_OPERATION_WRITEMORE, NULL },
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
    static const ssize_t max = sizeof(handlers)/sizeof(handlers[0]) - 1;

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

int on_endpoint_message(pcmcth_renderer* rdr, pcmcth_endpoint* endpoint,
        const pcrdr_msg *msg)
{
    if (msg->type == PCRDR_MSG_TYPE_REQUEST) {
        request_handler handler = find_request_handler(
                purc_variant_get_string_const(msg->operation));

        LOG_DEBUG("Got a request message: %s (handler: %p)\n",
                purc_variant_get_string_const(msg->operation), handler);

        if (handler == NOT_FOUND_HANDLER) {
            pcrdr_msg response = { };
            response.type = PCRDR_MSG_TYPE_RESPONSE;
            response.requestId = msg->requestId;
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
            response.requestId = msg->requestId;
            response.sourceURI = PURC_VARIANT_INVALID;
            response.retCode = PCRDR_SC_NOT_IMPLEMENTED;
            response.resultValue = 0;
            response.dataType = PCRDR_MSG_DATA_TYPE_VOID;

            return send_simple_response(rdr, endpoint, &response);
        }
    }
    else if (msg->type == PCRDR_MSG_TYPE_EVENT) {
        // TODO
        purc_log_warn("Got an event message: %s\n",
                purc_variant_get_string_const(msg->eventName));
    }
    else {
        // TODO
        purc_log_warn("Got an unknown message: %d\n", msg->type);
    }

    return PCRDR_SC_OK;
}

