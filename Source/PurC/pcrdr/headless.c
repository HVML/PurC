/*
 * headless.c -- The implementation of HEADLESS method for PURCMC protocol.
 *      Created on 7 Mar 2022
 *
 * Copyright (C) 2022, 2023 FMSoft (http://www.fmsoft.cn)
 *
 * Authors:
 *  Vincent Wei (https://github.com/VincentWei), 2022, 2023
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
#include "config.h"
#include "purc-pcrdr.h"
#include "private/pcrdr.h"
#include "private/kvlist.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/ports.h"

#include "connect.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <assert.h>

#define LEN_BUFF_LONGLONGINT    128

#define NR_WORKSPACES           4
#define NR_TABBEDWINDOWS        8
#define NR_WIDGETS              16
#define NR_PLAINWINDOWS         64

#define __STRING(x) #x

#define RENDERER_FEATURES                                                    \
    PCRDR_PURCMC_PROTOCOL_NAME ":" PCRDR_PURCMC_PROTOCOL_VERSION_STRING "\n" \
    "HEADLESS:1.0\n"                                        \
    "HTML:5.3/XGML:1.0/XML:1.0\n"                           \
    "workspace:" __STRING(8)                                \
    "/tabbedWindow:" __STRING(8)                            \
    "/widgetInTabbedWindow:" __STRING(32)                   \
    "/plainWindow:" __STRING(256) "\n"                      \
    "vendor:FMSoft\n"                                       \
    "locale:en\n"                                           \
    "docLoadingMethod:direct"

struct tabbedwin_info {
    // the group identifier of the tabbedwin
    const char *group;

    // handle of this tabbedWindow; NULL for not used slot.
    void *handle;

    // number of widgets in this tabbedWindow
    int nr_widgets;

    // the active widget in this tabbedWindow
    int active_widget;

    // handles of all widgets in this tabbedWindow
    void *widgets[NR_WIDGETS];

    // handles of all DOM documents in all widgets.
    void *domdocs[NR_WIDGETS];
};

struct workspace_info {
    // handle of this workspace; NULL for not used slot
    void *handle;

    // name of the workspace
    char *name;

    // number of tabbed windows in this workspace
    int nr_tabbedwins;

    // number of plain windows in this workspace
    int nr_plainwins;

    // index of the active plain window in this workspace
    int active_plainwin;

    // information of all tabbed windows in this workspace.
    struct tabbedwin_info tabbedwins[NR_TABBEDWINDOWS];

    // handles of all plain windows in this workspace.
    void *plainwins[NR_PLAINWINDOWS];

    // handles of DOM documents in all plain windows.
    void *domdocs[NR_PLAINWINDOWS];

    // page identifier (plainwin:hello@main) -> owners;
    struct pcutils_kvlist        widget_owners;

    // widget group name (main) -> tabbedwindows;
    struct pcutils_kvlist        group_tabbedwin;
};

struct session_info {
    // number of workspaces
    int nr_workspaces;
    int active_workspace;

    // workspaces
    struct workspace_info workspaces[NR_WORKSPACES];
};

struct result_info {
    int         retCode;
    uint64_t    resultValue;
    pcrdr_msg_data_type data_type;
    purc_variant_t data;
};

struct pcrdr_prot_data {
    // FILE pointer to serialize the message.
    FILE                *fp;

    // requestId -> results;
    struct pcutils_kvlist        results;

    // pointer to the session.
    struct session_info *session;
};

static struct result_info *result_of_first_request(pcrdr_conn* conn)
{
    if (list_empty(&conn->pending_requests)) {
        return NULL;
    }

    struct pending_request *pr;
    pr = list_first_entry(&conn->pending_requests,
            struct pending_request, list);

    const char *request_id;
    request_id = purc_variant_get_string_const(pr->request_id);

    struct result_info **data;
    data = pcutils_kvlist_get(&conn->prot_data->results, request_id);
    if (data == NULL)
        return NULL;

    return *data;
}

static int my_wait_message(pcrdr_conn* conn, int timeout_ms)
{
    if (result_of_first_request(conn) == NULL) {
        if (timeout_ms > 1000) {
            pcutils_sleep(timeout_ms / 1000);
        }

        if (timeout_ms > 0) {
            unsigned int ms = timeout_ms % 1000;
            if (ms) {
                pcutils_usleep(ms * 1000);
            }
        }

        return 0;
    }

    // it's time to read a fake response message.
    return 1;
}

static ssize_t write_sent_to_log(void *ctxt, const void *buf, size_t count)
{
    pcrdr_conn *conn = (pcrdr_conn *)ctxt;

    if (fwrite(buf, 1, count, conn->prot_data->fp) == count) {
        conn->stats.bytes_sent += count;
        return count;
    }

    return -1;
}

static ssize_t write_recv_to_log(void *ctxt, const void *buf, size_t count)
{
    pcrdr_conn *conn = (pcrdr_conn *)ctxt;

    if (fwrite(buf, 1, count, conn->prot_data->fp) == count) {
        conn->stats.bytes_recv += count;
        return count;
    }

    return -1;
}

static pcrdr_msg *my_read_message(pcrdr_conn* conn)
{
    pcrdr_msg* msg = NULL;
    struct result_info *result;

    if ((result = result_of_first_request(conn)) == NULL) {
        purc_log_warn("There is not any result for the first request.\n");
        purc_set_error(PCRDR_ERROR_UNEXPECTED);
        return NULL;
    }

    struct pending_request *pr;
    pr = list_first_entry(&conn->pending_requests,
            struct pending_request, list);

    const char *request_id;
    request_id = purc_variant_get_string_const(pr->request_id);

    msg = pcrdr_make_response_message(
            request_id, NULL,
            result->retCode, (uint64_t)(uintptr_t)result->resultValue,
            PCRDR_MSG_DATA_TYPE_VOID, NULL, 0);
    msg->dataType = result->data_type;
    msg->data = result->data;

    pcutils_kvlist_remove(&conn->prot_data->results, request_id);
    free(result);

    if (msg == NULL) {
        purc_set_error(PCRDR_ERROR_NOMEM);
    }
    else {
        fputs("<<<STT\n", conn->prot_data->fp);
        pcrdr_serialize_message(msg, (pcrdr_cb_write)write_recv_to_log, conn);
        fputs("\n<<<END\n\n", conn->prot_data->fp);
        fflush(conn->prot_data->fp);
    }

    return msg;
}

typedef void (*request_handler)(struct pcrdr_prot_data *prot_data,
        const pcrdr_msg *msg, unsigned int op_id, struct result_info *result);

static int create_workspace(struct pcrdr_prot_data *prot_data, size_t slot,
        const char *name)
{
    struct workspace_info *workspace = prot_data->session->workspaces + slot;
    workspace->handle = &workspace->handle;
    workspace->name = strdup(name);

    pcutils_kvlist_init(&workspace->widget_owners, NULL);
    pcutils_kvlist_init(&workspace->group_tabbedwin, NULL);
    return 0;
}

static void destroy_workspace(struct pcrdr_prot_data *prot_data, size_t slot)
{
    struct workspace_info *workspace = prot_data->session->workspaces + slot;

    const char *name;
    void *next, *data;
    struct purc_page_ostack *ostack;

    assert(workspace->handle);

    /* TODO: generate window and/or widget destroyed events */
    kvlist_for_each_safe(&workspace->widget_owners, name, next, data) {
        ostack = *(struct purc_page_ostack **)data;

        purc_page_ostack_delete(&workspace->widget_owners, ostack);
    }

    pcutils_kvlist_cleanup(&workspace->widget_owners);

    pcutils_kvlist_cleanup(&workspace->group_tabbedwin);

    free(workspace->name);
    memset(workspace, 0, sizeof(struct workspace_info));
}

static void on_start_session(struct pcrdr_prot_data *prot_data,
        const pcrdr_msg *msg, unsigned int op_id, struct result_info *result)
{
    UNUSED_PARAM(op_id);

    if (msg->target != PCRDR_MSG_TARGET_SESSION) {
        result->retCode = PCRDR_SC_BAD_REQUEST;
        result->resultValue = 0;
        return;
    }

    if (prot_data->session) {
        result->retCode = PCRDR_SC_METHOD_NOT_ALLOWED;
        result->resultValue = 0;
        return;
    }

    prot_data->session = calloc(1, sizeof(*prot_data->session));
    if (prot_data->session == NULL) {
        result->retCode = PCRDR_SC_INSUFFICIENT_STORAGE;
        result->resultValue = 0;
        return;
    }

    /* create the default workspace */
    create_workspace(prot_data, 0, PCRDR_DEFAULT_WORKSPACE);
    prot_data->session->nr_workspaces = 1;
    prot_data->session->active_workspace = 0;

    result->retCode = PCRDR_SC_OK;
    result->resultValue = (uint64_t)(uintptr_t)prot_data->session;
}

static void on_end_session(struct pcrdr_prot_data *prot_data,
        const pcrdr_msg *msg, unsigned int op_id, struct result_info *result)
{
    UNUSED_PARAM(msg);
    UNUSED_PARAM(op_id);

    if (msg->target != PCRDR_MSG_TARGET_SESSION) {
        result->retCode = PCRDR_SC_BAD_REQUEST;
        result->resultValue = 0;
        return;
    }

    if (prot_data->session == 0) {
        result->retCode = PCRDR_SC_METHOD_NOT_ALLOWED;
        result->resultValue = 0;
        return;
    }

    if (msg->targetValue != 0 &&
            msg->targetValue != (uint64_t)(uintptr_t)prot_data->session) {
        result->retCode = PCRDR_SC_BAD_REQUEST;
        result->resultValue = msg->targetValue;
        return;
    }

    struct workspace_info *workspaces = prot_data->session->workspaces;
    for (int i = 0; i < NR_WORKSPACES; i++) {
        if (workspaces[i].handle) {
            destroy_workspace(prot_data, i);
        }
    }

    free(prot_data->session);
    prot_data->session = NULL;

    result->retCode = PCRDR_SC_OK;
    result->resultValue = msg->targetValue;
}

static uint64_t get_special_workspace_handle(struct session_info *session,
        pcrdr_resname_workspace_k v)
{
    void *handle = 0;

    switch (v) {
        case PCRDR_K_RESNAME_WORKSPACE_default:
            handle = session->workspaces[0].handle;
            break;
        case PCRDR_K_RESNAME_WORKSPACE_active:
            handle = session->workspaces[session->active_workspace].handle;
            break;
        case PCRDR_K_RESNAME_WORKSPACE_first:
            handle = session->workspaces[0].handle;
            break;
        case PCRDR_K_RESNAME_WORKSPACE_last:
            for (int i = NR_WORKSPACES - 1; i >= 0; i++) {
                if (session->workspaces[i].handle) {
                    handle = session->workspaces[i].handle;
                    break;
                }
            }
            break;
    }

    assert(handle != NULL);
    return (uint64_t)(uintptr_t)handle;
}

static int find_new_active_workspace(struct session_info *session, int removed)
{
    assert(removed > 0);

    int i;
    for (i = removed - 1; i >= 0; i--) {
        if (session->workspaces[i].handle) {
            break;
        }
    }

    return i;
}

static uint64_t get_special_plainwin_handle(struct workspace_info *workspace,
        pcrdr_resname_page_k v)
{
    void *handle = 0;

    switch (v) {
        case PCRDR_K_RESNAME_PAGE_active:
            handle = workspace->plainwins[workspace->active_plainwin];
            break;

        case PCRDR_K_RESNAME_PAGE_first:
            for (int i = 0; i < NR_PLAINWINDOWS; i++) {
                if (workspace->plainwins[i]) {
                    handle = workspace->plainwins[i];
                    break;
                }
            }
            break;

        case PCRDR_K_RESNAME_PAGE_last:
            for (int i = NR_PLAINWINDOWS - 1; i >= 0; i--) {
                if (workspace->plainwins[i]) {
                    handle = workspace->plainwins[i];
                    break;
                }
            }
            break;
    }

    return (uint64_t)(uintptr_t)handle;
}

static int find_new_active_plainwin(struct workspace_info *workspace)
{
    int active = -1;

    for (int i = 0; i < NR_PLAINWINDOWS; i++) {
        if (workspace->plainwins[i]) {
            active = i;
            break;
        }
    }

    return active;
}

static uint64_t get_special_widget_handle(struct tabbedwin_info *tabbedwin,
        pcrdr_resname_page_k v)
{
    void *handle = 0;

    switch (v) {
        case PCRDR_K_RESNAME_PAGE_active:
            handle = tabbedwin->widgets[tabbedwin->active_widget];
            break;

        case PCRDR_K_RESNAME_PAGE_first:
            for (int i = 0; i < NR_WIDGETS; i++) {
                if (tabbedwin->widgets[i]) {
                    handle = tabbedwin->widgets[i];
                    break;
                }
            }
            break;

        case PCRDR_K_RESNAME_PAGE_last:
            for (int i = NR_WIDGETS - 1; i >= 0; i--) {
                if (tabbedwin->widgets[i]) {
                    handle = tabbedwin->widgets[i];
                    break;
                }
            }
            break;
    }

    assert(handle != NULL);
    return (uint64_t)(uintptr_t)handle;
}

static int find_new_active_widget(struct tabbedwin_info *tabbedwin)
{
    int active = -1;

    for (int i = 0; i < NR_WIDGETS; i++) {
        if (tabbedwin->widgets[i]) {
            active = i;
            break;
        }
    }

    return active;
}

static void on_create_workspace(struct pcrdr_prot_data *prot_data,
        const pcrdr_msg *msg, unsigned int op_id, struct result_info *result)
{
    UNUSED_PARAM(op_id);

    if (msg->target != PCRDR_MSG_TARGET_SESSION) {
        result->retCode = PCRDR_SC_BAD_REQUEST;
        result->resultValue = 0;
        return;
    }

    if (prot_data->session == 0) {
        result->retCode = PCRDR_SC_TOO_EARLY;
        result->resultValue = 0;
        return;
    }

    if (msg->targetValue != 0 &&
            msg->targetValue != (uint64_t)(uintptr_t)prot_data->session) {
        result->retCode = PCRDR_SC_BAD_REQUEST;
        result->resultValue = 0;
        return;
    }

    if (prot_data->session->nr_workspaces >= NR_WORKSPACES) {
        result->retCode = PCRDR_SC_SERVICE_UNAVAILABLE;
        result->resultValue = 0;
        return;
    }

    /* Since PURCMC-120: use element for the name of worksapce */
    if (msg->elementType != PCRDR_MSG_ELEMENT_TYPE_ID) {
        result->retCode = PCRDR_SC_BAD_REQUEST;
        result->resultValue = 0;
        return;
    }

    const char *name = purc_variant_get_string_const(msg->elementValue);
    if (name == NULL) {
        result->retCode = PCRDR_SC_BAD_REQUEST;
        result->resultValue = 0;
        return;
    }

    if (name[0] == '_') {    // reserved name
        int v = pcrdr_check_reserved_workspace_name(name);
        if (v < 0) {
            result->retCode = PCRDR_SC_BAD_REQUEST;
            result->resultValue = 0;
            return;

        }

        result->retCode = PCRDR_SC_OK;
        result->resultValue = get_special_workspace_handle(prot_data->session,
                (pcrdr_resname_workspace_k)v);
        return;
    }

    int i;
    struct workspace_info *workspaces = prot_data->session->workspaces;
    for (i = 0; i < NR_WORKSPACES; i++) {
        if (workspaces[i].handle && strcmp(workspaces[i].name, name) == 0) {
            /* Since PURCMC-120, returns the exsiting workspace */
            goto done;
        }
    }

    for (i = 0; i < NR_WORKSPACES; i++) {
        if (workspaces[i].handle == NULL) {
            break;
        }
    }
    assert(i < NR_WORKSPACES);

    create_workspace(prot_data, i, name);
    prot_data->session->nr_workspaces++;
    prot_data->session->active_workspace = i;

done:
    result->retCode = PCRDR_SC_OK;
    result->resultValue = (uint64_t)(uintptr_t)workspaces[i].handle;
}

static void on_update_workspace(struct pcrdr_prot_data *prot_data,
        const pcrdr_msg *msg, unsigned int op_id, struct result_info *result)
{
    UNUSED_PARAM(op_id);

    if (msg->target != PCRDR_MSG_TARGET_SESSION) {
        result->retCode = PCRDR_SC_BAD_REQUEST;
        result->resultValue = 0;
        return;
    }

    if (prot_data->session == 0) {
        result->retCode = PCRDR_SC_TOO_EARLY;
        result->resultValue = 0;
        return;
    }

    if (msg->targetValue != 0 &&
            msg->targetValue != (uint64_t)(uintptr_t)prot_data->session) {
        result->retCode = PCRDR_SC_BAD_REQUEST;
        result->resultValue = msg->targetValue;
        return;
    }

    struct workspace_info *workspaces = prot_data->session->workspaces;
    int i;
    for (i = 0; i < NR_WORKSPACES; i++) {
        uint64_t handle = (uint64_t)(uintptr_t)workspaces[i].handle;
        if (handle == msg->targetValue) {
            break;
        }
    }

    if (i >= NR_WORKSPACES) {
        result->retCode = PCRDR_SC_NOT_FOUND;
        result->resultValue = msg->targetValue;
        return;
    }

    result->retCode = PCRDR_SC_OK;
    result->resultValue = msg->targetValue;
}

static void on_destroy_workspace(struct pcrdr_prot_data *prot_data,
        const pcrdr_msg *msg, unsigned int op_id, struct result_info *result)
{
    UNUSED_PARAM(op_id);

    if (msg->target != PCRDR_MSG_TARGET_SESSION) {
        result->retCode = PCRDR_SC_BAD_REQUEST;
        result->resultValue = 0;
        return;
    }

    if (prot_data->session == 0) {
        result->retCode = PCRDR_SC_TOO_EARLY;
        result->resultValue = 0;
        return;
    }

    if (msg->targetValue != 0 &&
            msg->targetValue != (uint64_t)(uintptr_t)prot_data->session) {
        result->retCode = PCRDR_SC_BAD_REQUEST;
        result->resultValue = msg->targetValue;
        return;
    }

    struct workspace_info *workspaces = prot_data->session->workspaces;
    int i;
    for (i = 0; i < NR_WORKSPACES; i++) {
        uint64_t handle = (uint64_t)(uintptr_t)workspaces[i].handle;
        if (handle == msg->targetValue) {
            break;
        }
    }

    if (i == 0) {
        result->retCode = PCRDR_SC_FORBIDDEN;
        result->resultValue = msg->targetValue;
        return;
    }

    if (i >= NR_WORKSPACES) {
        result->retCode = PCRDR_SC_NOT_FOUND;
        result->resultValue = msg->targetValue;
        return;
    }

    destroy_workspace(prot_data, i);
    prot_data->session->nr_workspaces--;
    prot_data->session->active_workspace = find_new_active_workspace(
            prot_data->session, i);

    result->retCode = PCRDR_SC_OK;
    result->resultValue = msg->targetValue;
}

static void on_create_plainwin(struct pcrdr_prot_data *prot_data,
        const pcrdr_msg *msg, unsigned int op_id, struct result_info *result)
{
    UNUSED_PARAM(op_id);

    if (msg->target != PCRDR_MSG_TARGET_WORKSPACE) {
        result->retCode = PCRDR_SC_BAD_REQUEST;
        result->resultValue = 0;
        return;
    }

    if (prot_data->session == 0) {
        result->retCode = PCRDR_SC_TOO_EARLY;
        result->resultValue = 0;
        return;
    }

    /* Since PURCMC-120, use element to specify the window name and group name:
        <window_name>[@<group_name>]
     */
    const char *name_group = NULL;
    if (msg->elementType == PCRDR_MSG_ELEMENT_TYPE_ID) {
        name_group = purc_variant_get_string_const(msg->elementValue);
    }

    if (name_group == NULL) {
        result->retCode = PCRDR_SC_BAD_REQUEST;
        result->resultValue = 0;
        return;
    }

    char idbuf[PURC_MAX_PLAINWIN_ID];
    char name[PURC_LEN_IDENTIFIER + 1];
    const char *group;
    group = purc_check_and_make_plainwin_id(idbuf, name, name_group);
    if (group == PURC_INVPTR) {
        result->retCode = PCRDR_SC_BAD_REQUEST;
        result->resultValue = 0;
        return;
    }

    int i = 0;
    struct workspace_info *workspaces = prot_data->session->workspaces;
    if (msg->targetValue != 0) {
        for (i = 0; i < NR_WORKSPACES; i++) {
            uint64_t workspace_handle;
            workspace_handle = (uint64_t)(uintptr_t)&workspaces[i].handle;
            if (workspace_handle == msg->targetValue) {
                break;
            }
        }

        if (i >= NR_WORKSPACES) {
            result->retCode = PCRDR_SC_NOT_FOUND;
            result->resultValue = msg->targetValue;
            return;
        }
    }

    /* Since PURCMC-120, support the special page name. */
    if (name[0] == '_') {    // reserved name
        int v = pcrdr_check_reserved_page_name(name);
        if (v < 0) {
            result->retCode = PCRDR_SC_BAD_REQUEST;
            result->resultValue = 0;
            return;

        }

        result->retCode = PCRDR_SC_OK;
        result->resultValue = get_special_plainwin_handle(workspaces + i,
                (pcrdr_resname_page_k)v);
        return;
    }

    /* Since PURCMC-120, returns the window handle if the window existed */
    void *data;
    data = pcutils_kvlist_get(&workspaces[i].widget_owners, idbuf);
    if (data != NULL) {
        struct purc_page_ostack *ostack = *(struct purc_page_ostack **)data;
        result->retCode = PCRDR_SC_OK;
        result->resultValue = (uint64_t)(uintptr_t)ostack;
        return;
    }

    if (workspaces[i].nr_plainwins >= NR_PLAINWINDOWS) {
        result->retCode = PCRDR_SC_SERVICE_UNAVAILABLE;
        result->resultValue = msg->targetValue;
        return;
    }

    int j;
    for (j = 0; j < NR_PLAINWINDOWS; j++) {
        if (workspaces[i].plainwins[j] == NULL) {

            struct purc_page_ostack *ostack;
            if ((ostack = purc_page_ostack_new(&workspaces[i].widget_owners,
                    idbuf, workspaces[i].plainwins + j)) == NULL) {
                result->retCode = PCRDR_SC_INSUFFICIENT_STORAGE;
                result->resultValue = 0;
                return;
            }

            workspaces[i].plainwins[j] = ostack;
            break;
        }
    }

    assert(j < NR_PLAINWINDOWS);

    workspaces[i].nr_plainwins++;
    workspaces[i].active_plainwin = j;
    result->retCode = PCRDR_SC_OK;
    result->resultValue = (uint64_t)(uintptr_t)workspaces[i].plainwins[j];
}

static void on_update_plainwin(struct pcrdr_prot_data *prot_data,
        const pcrdr_msg *msg, unsigned int op_id, struct result_info *result)
{
    UNUSED_PARAM(op_id);

    if (msg->target != PCRDR_MSG_TARGET_WORKSPACE ||
            msg->elementType != PCRDR_MSG_ELEMENT_TYPE_HANDLE) {
        result->retCode = PCRDR_SC_BAD_REQUEST;
        result->resultValue = 0;
        return;
    }

    if (prot_data->session == 0) {
        result->retCode = PCRDR_SC_TOO_EARLY;
        result->resultValue = 0;
        return;
    }

    int i = 0;
    struct workspace_info *workspaces = prot_data->session->workspaces;
    if (msg->targetValue != 0) {
        for (i = 0; i < NR_WORKSPACES; i++) {
            uint64_t workspace_handle;
            workspace_handle = (uint64_t)(uintptr_t)&workspaces[i].handle;
            if (workspace_handle == msg->targetValue) {
                break;
            }
        }

        if (i >= NR_WORKSPACES) {
            result->retCode = PCRDR_SC_NOT_FOUND;
            result->resultValue = msg->targetValue;
            return;
        }
    }

    uint64_t elem_handle = (uint64_t)strtoull(
            purc_variant_get_string_const(msg->elementValue), NULL, 16);

    int j;
    for (j = 0; j < NR_PLAINWINDOWS; j++) {
        uint64_t handle = (uint64_t)(uintptr_t)&workspaces[i].plainwins[j];
        if (handle == elem_handle) {
            break;
        }
    }

    if (j >= NR_PLAINWINDOWS) {
        result->retCode = PCRDR_SC_NOT_FOUND;
        result->resultValue = elem_handle;
        return;
    }

    result->retCode = PCRDR_SC_OK;
    result->resultValue = elem_handle;
}

static void on_destroy_plainwin(struct pcrdr_prot_data *prot_data,
        const pcrdr_msg *msg, unsigned int op_id, struct result_info *result)
{
    UNUSED_PARAM(op_id);

    if (msg->target != PCRDR_MSG_TARGET_WORKSPACE ||
            msg->elementType != PCRDR_MSG_ELEMENT_TYPE_HANDLE) {
        result->retCode = PCRDR_SC_BAD_REQUEST;
        result->resultValue = 0;
        return;
    }

    if (prot_data->session == 0) {
        result->retCode = PCRDR_SC_TOO_EARLY;
        result->resultValue = 0;
        return;
    }

    int i = 0;
    struct workspace_info *workspaces = prot_data->session->workspaces;
    if (msg->targetValue != 0) {
        for (i = 0; i < NR_WORKSPACES; i++) {
            uint64_t workspace_handle;
            workspace_handle = (uint64_t)(uintptr_t)&workspaces[i].handle;
            if (workspace_handle == msg->targetValue) {
                break;
            }
        }

        if (i >= NR_WORKSPACES) {
            result->retCode = PCRDR_SC_NOT_FOUND;
            result->resultValue = msg->targetValue;
            return;
        }
    }

    uint64_t elem_handle = (uint64_t)strtoull(
            purc_variant_get_string_const(msg->elementValue), NULL, 16);

    int j;
    for (j = 0; j < NR_PLAINWINDOWS; j++) {
        uint64_t handle = (uint64_t)(uintptr_t)workspaces[i].plainwins[j];
        if (handle == elem_handle) {
            break;
        }
    }

    if (j >= NR_PLAINWINDOWS) {
        result->retCode = PCRDR_SC_NOT_FOUND;
        result->resultValue = elem_handle;
        return;
    }

    /* TODO: generate window destroyed events */
    struct purc_page_ostack *ostack = workspaces[i].plainwins[j];
    purc_page_ostack_delete(&workspaces[i].widget_owners, ostack);

    workspaces[i].plainwins[j] = NULL;
    workspaces[i].domdocs[j] = NULL;
    workspaces[i].nr_plainwins--;
    workspaces[i].active_plainwin = find_new_active_plainwin(workspaces + i);

    result->retCode = PCRDR_SC_OK;
    result->resultValue = elem_handle;
}

static void on_reset_page_groups(struct pcrdr_prot_data *prot_data,
        const pcrdr_msg *msg, unsigned int op_id, struct result_info *result)
{
    UNUSED_PARAM(op_id);

    if (msg->target != PCRDR_MSG_TARGET_WORKSPACE) {
        result->retCode = PCRDR_SC_BAD_REQUEST;
        result->resultValue = 0;
        return;
    }

    if (prot_data->session == 0) {
        result->retCode = PCRDR_SC_TOO_EARLY;
        result->resultValue = 0;
        return;
    }

    int i = 0;
    struct workspace_info *workspaces = prot_data->session->workspaces;
    if (msg->targetValue != 0) {
        for (i = 0; i < NR_WORKSPACES; i++) {
            uint64_t workspace_handle;
            workspace_handle = (uint64_t)(uintptr_t)&workspaces[i].handle;
            if (workspace_handle == msg->targetValue) {
                break;
            }
        }

        if (i >= NR_WORKSPACES) {
            result->retCode = PCRDR_SC_NOT_FOUND;
            result->resultValue = msg->targetValue;
            return;
        }
    }

    result->retCode = PCRDR_SC_OK;
    result->resultValue = 0;
}

static void on_add_page_groups(struct pcrdr_prot_data *prot_data,
        const pcrdr_msg *msg, unsigned int op_id, struct result_info *result)
{
    UNUSED_PARAM(op_id);

    if (msg->target != PCRDR_MSG_TARGET_WORKSPACE ||
            msg->elementType != PCRDR_MSG_ELEMENT_TYPE_HANDLE) {
        result->retCode = PCRDR_SC_BAD_REQUEST;
        result->resultValue = msg->targetValue;
        return;
    }

    if (prot_data->session == 0) {
        result->retCode = PCRDR_SC_TOO_EARLY;
        result->resultValue = msg->targetValue;
        return;
    }

    int i;
    struct workspace_info *workspaces = prot_data->session->workspaces;
    if (msg->targetValue != 0) {
        for (i = 0; i < NR_WORKSPACES; i++) {
            uint64_t workspace_handle;
            workspace_handle = (uint64_t)(uintptr_t)&workspaces[i].handle;
            if (workspace_handle == msg->targetValue) {
                break;
            }
        }

        if (i >= NR_WORKSPACES) {
            result->retCode = PCRDR_SC_NOT_FOUND;
            result->resultValue = msg->targetValue;
            return;
        }
    }

    result->retCode = PCRDR_SC_OK;
    result->resultValue = 0;
}

static void on_remove_page_group(struct pcrdr_prot_data *prot_data,
        const pcrdr_msg *msg, unsigned int op_id, struct result_info *result)
{
    UNUSED_PARAM(op_id);

    if (msg->target != PCRDR_MSG_TARGET_WORKSPACE ||
            msg->elementType != PCRDR_MSG_ELEMENT_TYPE_HANDLE) {
        result->retCode = PCRDR_SC_BAD_REQUEST;
        result->resultValue = 0;
        return;
    }

    if (prot_data->session == 0) {
        result->retCode = PCRDR_SC_TOO_EARLY;
        result->resultValue = 0;
        return;
    }

    int i = 0;
    struct workspace_info *workspaces = prot_data->session->workspaces;
    if (msg->targetValue != 0) {
        for (i = 0; i < NR_WORKSPACES; i++) {
            uint64_t workspace_handle;
            workspace_handle = (uint64_t)(uintptr_t)&workspaces[i].handle;
            if (workspace_handle == msg->targetValue) {
                break;
            }
        }

        if (i >= NR_WORKSPACES) {
            result->retCode = PCRDR_SC_NOT_FOUND;
            result->resultValue = msg->targetValue;
            return;
        }
    }

    result->retCode = PCRDR_SC_OK;
    result->resultValue = 0;
}

static struct tabbedwin_info *
create_or_get_tabbedwin(struct workspace_info *workspace, const char *group)
{
    void *data = pcutils_kvlist_get(&workspace->group_tabbedwin, group);
    if (data) {
        return *(struct tabbedwin_info **)data;
    }

    struct tabbedwin_info *tabbedwin = NULL;
    int j;
    for (j = 0; j < NR_TABBEDWINDOWS; j++) {
        tabbedwin = workspace->tabbedwins + j;
        if (tabbedwin->handle == NULL) {
            tabbedwin->handle = tabbedwin;
            tabbedwin->group = pcutils_kvlist_set_ex(
                    &workspace->group_tabbedwin, group, &tabbedwin);
            break;
        }
    }

    if (j >= NR_TABBEDWINDOWS)
        return NULL;

    return tabbedwin;
}

static void on_create_widget(struct pcrdr_prot_data *prot_data,
        const pcrdr_msg *msg, unsigned int op_id, struct result_info *result)
{
    UNUSED_PARAM(op_id);

    if (msg->target != PCRDR_MSG_TARGET_WORKSPACE ||
            msg->elementType != PCRDR_MSG_ELEMENT_TYPE_ID) {
        result->retCode = PCRDR_SC_BAD_REQUEST;
        result->resultValue = 0;
        return;
    }

    if (prot_data->session == 0) {
        result->retCode = PCRDR_SC_TOO_EARLY;
        result->resultValue = 0;
        return;
    }

    int i =0 ;
    struct workspace_info *workspaces = prot_data->session->workspaces;
    if (msg->targetValue != 0) {
        for (i = 0; i < NR_WORKSPACES; i++) {
            uint64_t workspace_handle;
            workspace_handle = (uint64_t)(uintptr_t)&workspaces[i].handle;
            if (workspace_handle == msg->targetValue) {
                break;
            }
        }

        if (i >= NR_WORKSPACES) {
            result->retCode = PCRDR_SC_NOT_FOUND;
            result->resultValue = msg->targetValue;
            return;
        }
    }

    const char *name_group = NULL;
    /* Since PURCMC-120, use element to specify the widget name and group name:
            <widget_name>@<group_name>
     */
    name_group = purc_variant_get_string_const(msg->elementValue);
    if (name_group == NULL) {
        result->retCode = PCRDR_SC_BAD_REQUEST;
        result->resultValue = 0;
        return;
    }

    char idbuf[PURC_MAX_WIDGET_ID];
    char name[PURC_LEN_IDENTIFIER + 1];
    const char *group = purc_check_and_make_widget_id(idbuf, name, name_group);
    if (group == NULL) {
        result->retCode = PCRDR_SC_BAD_REQUEST;
        result->resultValue = 0;
        return;
    }

    /* Since PURCMC-120, support the special page name. */
    if (name[0] == '_') {    // reserved name
        int v = pcrdr_check_reserved_page_name(name);
        if (v < 0) {
            result->retCode = PCRDR_SC_BAD_REQUEST;
            result->resultValue = 0;
            return;

        }

        struct tabbedwin_info *tabbedwin = NULL;
        void *data = pcutils_kvlist_get(&workspaces[i].group_tabbedwin, group);
        if (data) {
            tabbedwin = *(struct tabbedwin_info **)data;
        }

        if (tabbedwin == NULL) {
            result->retCode = PCRDR_SC_NOT_FOUND;
            result->resultValue = 0;
            return;
        }

        result->retCode = PCRDR_SC_OK;
        result->resultValue = get_special_widget_handle(tabbedwin,
                (pcrdr_resname_page_k)v);
        return;
    }

    /* Since PURCMC-120, returns the widget handle if the widget existed */
    void **data;
    data = pcutils_kvlist_get(&workspaces[i].widget_owners, idbuf);
    if (data != NULL) {
        struct purc_page_ostack *ostack = *(struct purc_page_ostack **)data;
        result->retCode = PCRDR_SC_OK;
        result->resultValue = (uint64_t)(uintptr_t)ostack;
        return;
    }

    struct tabbedwin_info *tabbedwin;
    tabbedwin = create_or_get_tabbedwin(workspaces + i, group);
    if (tabbedwin == NULL || tabbedwin->nr_widgets >= NR_WIDGETS) {
        result->retCode = PCRDR_SC_SERVICE_UNAVAILABLE;
        result->resultValue = msg->targetValue;
        return;
    }

    int k;
    for (k = 0; k < NR_WIDGETS; k++) {
        if (tabbedwin->widgets[k] == NULL) {
            struct purc_page_ostack *ostack;
            if ((ostack = purc_page_ostack_new(&workspaces[i].widget_owners,
                    idbuf, tabbedwin->widgets + k)) == NULL) {
                result->retCode = PCRDR_SC_INSUFFICIENT_STORAGE;
                result->resultValue = 0;
                return;
            }

            tabbedwin->widgets[k] = ostack;
            break;
        }
    }

    assert(k < NR_WIDGETS);

    tabbedwin->nr_widgets++;
    tabbedwin->active_widget = k;
    result->retCode = PCRDR_SC_OK;
    result->resultValue =
        (uint64_t)(uintptr_t)tabbedwin->widgets[k];
}

static void on_update_widget(struct pcrdr_prot_data *prot_data,
        const pcrdr_msg *msg, unsigned int op_id, struct result_info *result)
{
    UNUSED_PARAM(op_id);

    if (msg->target != PCRDR_MSG_TARGET_WORKSPACE ||
            msg->elementType != PCRDR_MSG_ELEMENT_TYPE_HANDLE) {
        result->retCode = PCRDR_SC_BAD_REQUEST;
        result->resultValue = 0;
        return;
    }

    if (prot_data->session == 0) {
        result->retCode = PCRDR_SC_TOO_EARLY;
        result->resultValue = 0;
        return;
    }

    int i = 0;
    struct workspace_info *workspaces = prot_data->session->workspaces;
    if (msg->targetValue != 0) {
        for (i = 0; i < NR_WORKSPACES; i++) {
            uint64_t handle;
            handle = (uint64_t)(uintptr_t)&workspaces[i].handle;
            if (handle == msg->targetValue) {
                break;
            }
        }

        if (i >= NR_WORKSPACES) {
            result->retCode = PCRDR_SC_NOT_FOUND;
            result->resultValue = msg->targetValue;
            return;
        }
    }

    uint64_t widget = (uint64_t)strtoull(
            purc_variant_get_string_const(msg->elementValue), NULL, 16);

    int j, k;
    for (j = 0; j < NR_TABBEDWINDOWS; j++) {
        struct tabbedwin_info *twin = workspaces[i].tabbedwins + j;
        for (k = 0; k < NR_WIDGETS; k++) {
            uint64_t handle = (uint64_t)(uintptr_t)twin->widgets[k];
            if (handle == widget) {
                goto found;
            }
        }
    }

found:
    if (j >= NR_TABBEDWINDOWS || k >= NR_WIDGETS) {
        result->retCode = PCRDR_SC_NOT_FOUND;
        result->resultValue = widget;
        return;
    }

    result->retCode = PCRDR_SC_OK;
    result->resultValue = widget;
}

static void on_destroy_widget(struct pcrdr_prot_data *prot_data,
        const pcrdr_msg *msg, unsigned int op_id, struct result_info *result)
{
    UNUSED_PARAM(op_id);

    if (msg->target != PCRDR_MSG_TARGET_WORKSPACE ||
            msg->elementType != PCRDR_MSG_ELEMENT_TYPE_HANDLE) {
        result->retCode = PCRDR_SC_BAD_REQUEST;
        result->resultValue = 0;
        return;
    }

    if (prot_data->session == 0) {
        result->retCode = PCRDR_SC_TOO_EARLY;
        result->resultValue = 0;
        return;
    }

    int i = 0;
    struct workspace_info *workspaces = prot_data->session->workspaces;
    if (msg->targetValue != 0) {
        for (i = 0; i < NR_WORKSPACES; i++) {
            uint64_t handle;
            handle = (uint64_t)(uintptr_t)&workspaces[i].handle;
            if (handle == msg->targetValue) {
                break;
            }
        }

        if (i >= NR_WORKSPACES) {
            result->retCode = PCRDR_SC_NOT_FOUND;
            result->resultValue = msg->targetValue;
            return;
        }
    }

    uint64_t widget = (uint64_t)strtoull(
            purc_variant_get_string_const(msg->elementValue), NULL, 16);

    int j, k;
    for (j = 0; j < NR_TABBEDWINDOWS; j++) {
        struct tabbedwin_info *twin = workspaces[i].tabbedwins + j;
        for (k = 0; k < NR_WIDGETS; k++) {
            uint64_t handle = (uint64_t)(uintptr_t)twin->widgets[k];
            if (handle == widget) {
                goto found;
            }
        }
    }

found:
    if (j >= NR_TABBEDWINDOWS || k >= NR_WIDGETS) {
        result->retCode = PCRDR_SC_NOT_FOUND;
        result->resultValue = widget;
        return;
    }

    /* TODO: generate DOM document and/or widget destroyed events */
    struct purc_page_ostack *ostack = workspaces[i].tabbedwins[j].widgets[k];
    purc_page_ostack_delete(&workspaces[i].widget_owners, ostack);

    workspaces[i].tabbedwins[j].widgets[k] = NULL;
    workspaces[i].tabbedwins[j].domdocs[k] = NULL;
    workspaces[i].tabbedwins[j].nr_widgets--;
    workspaces[i].tabbedwins[j].active_widget = find_new_active_widget(
            workspaces[i].tabbedwins + j);

    result->retCode = PCRDR_SC_OK;
    result->resultValue = widget;
}

static void **find_domdoc_ptr(struct pcrdr_prot_data *prot_data,
        const pcrdr_msg *msg, struct result_info *result, void **widget)
{
    if (msg->target != PCRDR_MSG_TARGET_PLAINWINDOW &&
            msg->target != PCRDR_MSG_TARGET_WIDGET) {
        result->retCode = PCRDR_SC_BAD_REQUEST;
        result->resultValue = 0;
        return NULL;
    }

    if (prot_data->session == 0) {
        result->retCode = PCRDR_SC_TOO_EARLY;
        result->resultValue = 0;
        return NULL;
    }

    void **domdocs;

    if (msg->target == PCRDR_MSG_TARGET_PLAINWINDOW) {
        int i, j;
        struct workspace_info *workspaces = prot_data->session->workspaces;
        for (i = 0; i < NR_WORKSPACES; i++) {
            if (workspaces[i].handle) {
                for (j = 0; j < NR_PLAINWINDOWS; j++) {
                    uint64_t handle =
                        (uint64_t)(uintptr_t)workspaces[i].plainwins[j];
                    if (handle == msg->targetValue) {
                        if (widget)
                            *widget = workspaces[i].plainwins[j];
                        goto found_pw;
                    }
                }
            }
        }

found_pw:
        if (i >= NR_WORKSPACES || j >= NR_PLAINWINDOWS) {
            result->retCode = PCRDR_SC_NOT_FOUND;
            result->resultValue = msg->targetValue;
            return NULL;
        }

        domdocs = &workspaces[i].domdocs[j];
    }
    else if (msg->target == PCRDR_MSG_TARGET_WIDGET) {
        int i, j, k;
        struct workspace_info *workspaces = prot_data->session->workspaces;
        for (i = 0; i < NR_WORKSPACES; i++) {
            if (workspaces[i].handle) {
                for (j = 0; j < NR_TABBEDWINDOWS; j++) {
                    for (k = 0; k < NR_WIDGETS; k++) {
                        uint64_t handle =
                            (uint64_t)(uintptr_t)
                            workspaces[i].tabbedwins[j].widgets[k];
                        if (handle == msg->targetValue) {
                            if (widget)
                                *widget =
                                    workspaces[i].tabbedwins[j].widgets[k];
                            goto found_tp;
                        }
                    }
                }
            }
        }

found_tp:
        if (i >= NR_WORKSPACES || j >= NR_TABBEDWINDOWS || k >= NR_WIDGETS) {
            result->retCode = PCRDR_SC_NOT_FOUND;
            result->resultValue = msg->targetValue;
            return NULL;
        }

        domdocs = &workspaces[i].tabbedwins[j].domdocs[k];
    }
    else {
        result->retCode = PCRDR_SC_BAD_REQUEST;
        result->resultValue = 0;
        return NULL;
    }

    return domdocs;
}

static void on_load_from_url(struct pcrdr_prot_data *prot_data,
        const pcrdr_msg *msg, unsigned int op_id, struct result_info *result)
{
    UNUSED_PARAM(prot_data);
    UNUSED_PARAM(msg);
    UNUSED_PARAM(op_id);

    result->retCode = PCRDR_SC_NOT_ACCEPTABLE;
    result->resultValue = 0;
}

static void on_load(struct pcrdr_prot_data *prot_data,
        const pcrdr_msg *msg, unsigned int op_id, struct result_info *result)
{
    void **domdocs;
    struct purc_page_ostack *ostack = NULL;

    UNUSED_PARAM(op_id);

    /* Since PURCMC-120, element must specify the handle of coroutine */
    if (msg->elementType != PCRDR_MSG_ELEMENT_TYPE_HANDLE) {
        result->retCode = PCRDR_SC_BAD_REQUEST;
        result->resultValue = 0;
        return;
    }

    if ((domdocs = find_domdoc_ptr(prot_data, msg, result,
                    (void **)&ostack)) == NULL) {
        return;
    }

    *domdocs = domdocs;

    uint64_t handle = (uint64_t)strtoull(
            purc_variant_get_string_const(msg->elementValue), NULL, 16);
    struct purc_page_owner owner = { NULL, handle }, suppressed;
    suppressed = purc_page_ostack_register(ostack, owner);
    if (suppressed.corh) {
        char buff[LEN_BUFF_LONGLONGINT];
        int n = snprintf(buff, sizeof(buff),
                "%llx", (unsigned long long int)suppressed.corh);
        assert(n < (int)sizeof(buff));
        (void)n;

        result->data_type = PCRDR_MSG_DATA_TYPE_PLAIN;
        result->data = purc_variant_make_string(buff, false);
    }

    result->retCode = PCRDR_SC_OK;
    result->resultValue = (uint64_t)(uintptr_t)domdocs;
}

static void on_write_begin(struct pcrdr_prot_data *prot_data,
        const pcrdr_msg *msg, unsigned int op_id, struct result_info *result)
{
    void **domdocs;
    struct purc_page_ostack *ostack;

    UNUSED_PARAM(op_id);

    /* Since PURCMC-120, element must specify the handle of coroutine */
    if (msg->elementType != PCRDR_MSG_ELEMENT_TYPE_HANDLE) {
        result->retCode = PCRDR_SC_BAD_REQUEST;
        result->resultValue = 0;
        return;
    }

    if ((domdocs = find_domdoc_ptr(prot_data, msg, result,
                    (void **)&ostack)) == NULL) {
        return;
    }

    *domdocs = domdocs;

    uint64_t handle = (uint64_t)strtoull(
            purc_variant_get_string_const(msg->elementValue), NULL, 16);
    struct purc_page_owner owner = { NULL, handle }, suppressed;
    suppressed = purc_page_ostack_register(ostack, owner);
    if (suppressed.corh) {
        char buff[LEN_BUFF_LONGLONGINT];
        int n = snprintf(buff, sizeof(buff),
                "%llx", (unsigned long long int)suppressed.corh);
        assert(n < (int)sizeof(buff));
        (void)n;

        result->data_type = PCRDR_MSG_DATA_TYPE_PLAIN;
        result->data = purc_variant_make_string(buff, false);
    }

    result->retCode = PCRDR_SC_OK;
    result->resultValue = msg->targetValue;
}

static void on_write_more(struct pcrdr_prot_data *prot_data,
        const pcrdr_msg *msg, unsigned int op_id, struct result_info *result)
{
    void **domdocs = NULL;

    UNUSED_PARAM(op_id);
    if ((domdocs = find_domdoc_ptr(prot_data, msg, result, NULL)) == NULL) {
        return;
    }

    if (*domdocs == NULL) {
        result->retCode = PCRDR_SC_PRECONDITION_FAILED;
        result->resultValue = msg->targetValue;
        return;
    }

    *domdocs = domdocs;

    result->retCode = PCRDR_SC_OK;
    result->resultValue = msg->targetValue;
}

static void on_write_end(struct pcrdr_prot_data *prot_data,
        const pcrdr_msg *msg, unsigned int op_id, struct result_info *result)
{
    void **domdocs = NULL;

    UNUSED_PARAM(op_id);
    if ((domdocs = find_domdoc_ptr(prot_data, msg, result, NULL)) == NULL) {
        return;
    }

    if (*domdocs == NULL) {
        result->retCode = PCRDR_SC_PRECONDITION_FAILED;
        result->resultValue = msg->targetValue;
        return;
    }

    *domdocs = domdocs;

    result->retCode = PCRDR_SC_OK;
    result->resultValue = msg->targetValue;
}

static void on_register(struct pcrdr_prot_data *prot_data,
        const pcrdr_msg *msg, unsigned int op_id, struct result_info *result)
{
    void **domdocs = NULL;
    struct purc_page_ostack *ostack;

    UNUSED_PARAM(op_id);

    if (msg->elementType != PCRDR_MSG_ELEMENT_TYPE_HANDLE) {
        result->retCode = PCRDR_SC_BAD_REQUEST;
        result->resultValue = 0;
        return;
    }

    if ((domdocs = find_domdoc_ptr(prot_data, msg, result,
                    (void **)&ostack)) == NULL) {
        return;
    }

    if (*domdocs == NULL) {
        result->retCode = PCRDR_SC_PRECONDITION_FAILED;
        result->resultValue = msg->targetValue;
        return;
    }

    uint64_t handle = (uint64_t)strtoull(
            purc_variant_get_string_const(msg->elementValue), NULL, 16);
    struct purc_page_owner owner = { NULL, handle }, suppressed;
    suppressed = purc_page_ostack_register(ostack, owner);
    result->retCode = PCRDR_SC_OK;
    result->resultValue = suppressed.corh;
}

static void on_revoke(struct pcrdr_prot_data *prot_data,
        const pcrdr_msg *msg, unsigned int op_id, struct result_info *result)
{
    void **domdocs = NULL;
    struct purc_page_ostack *ostack;

    UNUSED_PARAM(op_id);

    if (msg->elementType != PCRDR_MSG_ELEMENT_TYPE_HANDLE) {
        result->retCode = PCRDR_SC_BAD_REQUEST;
        result->resultValue = 0;
        return;
    }

    if ((domdocs = find_domdoc_ptr(prot_data, msg, result,
                    (void **)&ostack)) == NULL) {
        return;
    }

    if (*domdocs == NULL) {
        result->retCode = PCRDR_SC_PRECONDITION_FAILED;
        result->resultValue = msg->targetValue;
        return;
    }

    uint64_t handle = (uint64_t)strtoull(
            purc_variant_get_string_const(msg->elementValue), NULL, 16);
    struct purc_page_owner owner = { NULL, handle }, reloaded;
    reloaded = purc_page_ostack_revoke(ostack, owner);
    result->retCode = PCRDR_SC_OK;
    result->resultValue = reloaded.corh;
}

static void on_operate_dom(struct pcrdr_prot_data *prot_data,
        const pcrdr_msg *msg, unsigned int op_id, struct result_info *result)
{
    UNUSED_PARAM(op_id);

    if (msg->target != PCRDR_MSG_TARGET_DOM ||
            msg->targetValue == 0) {
        result->retCode = PCRDR_SC_BAD_REQUEST;
        result->resultValue = 0;
        return;
    }

    if (prot_data->session == 0) {
        result->retCode = PCRDR_SC_TOO_EARLY;
        result->resultValue = 0;
        return;
    }

    bool found = false;
    struct workspace_info *workspaces = prot_data->session->workspaces;
    for (int i = 0; i < NR_WORKSPACES; i++) {
        for (int j = 0; j < NR_PLAINWINDOWS; j++) {
            uint64_t handle =
                (uint64_t)(uintptr_t)&workspaces[i].domdocs[j];
            if (handle == msg->targetValue) {
                found = true;
                goto found;
            }
        }

        for (int j = 0; j < NR_TABBEDWINDOWS; j++) {

            if (workspaces[i].tabbedwins[j].handle == NULL)
                continue;

            for (int k = 0; k < NR_WIDGETS; k++) {
                uint64_t handle =
                    (uint64_t)(uintptr_t)
                    &workspaces[i].tabbedwins[j].domdocs[k];
                if (handle == msg->targetValue) {
                    found = true;
                    goto found;
                }
            }
        }
    }

found:
    if (!found) {
        result->retCode = PCRDR_SC_NOT_FOUND;
        result->resultValue = msg->targetValue;
        return;
    }

    result->retCode = PCRDR_SC_OK;
    result->resultValue = msg->targetValue;
}

static void on_call_method(struct pcrdr_prot_data *prot_data,
        const pcrdr_msg *msg, unsigned int op_id, struct result_info *result)
{
    UNUSED_PARAM(prot_data);
    UNUSED_PARAM(msg);
    UNUSED_PARAM(op_id);

    /* always return a true value */
    result->retCode = PCRDR_SC_OK;
    result->data_type = PCRDR_MSG_DATA_TYPE_JSON;
    result->data = purc_variant_make_boolean(true);
}

static void on_get_property(struct pcrdr_prot_data *prot_data,
        const pcrdr_msg *msg, unsigned int op_id, struct result_info *result)
{
    UNUSED_PARAM(op_id);

    if (msg->target != PCRDR_MSG_TARGET_SESSION) {
        result->retCode = PCRDR_SC_BAD_REQUEST;
        result->resultValue = 0;
        return;
    }

    if (prot_data->session == 0) {
        result->retCode = PCRDR_SC_TOO_EARLY;
        result->resultValue = 0;
        return;
    }

    if (msg->targetValue != 0 &&
            msg->targetValue != (uint64_t)(uintptr_t)prot_data->session) {
        result->retCode = PCRDR_SC_BAD_REQUEST;
        result->resultValue = 0;
        return;
    }

    const char *property = NULL;
    if (msg->property) {
       property = purc_variant_get_string_const(msg->property);
    }

    if (property == NULL) {
        result->retCode = PCRDR_SC_BAD_REQUEST;
        result->resultValue = 0;
        return;
    }

    /* only support `workspaceList` on session */
    if (strcmp(property, "workspaceList")) {
        result->retCode = PCRDR_SC_NOT_FOUND;
        result->resultValue = 0;
        return;
    }

    result->data = purc_variant_make_object_0();
    int i;
    struct workspace_info *workspaces = prot_data->session->workspaces;
    for (i = 0; i < NR_WORKSPACES; i++) {
        if (workspaces[i].handle) {

            purc_variant_t value = purc_variant_make_object_0();
            purc_variant_t handle;
            char buff[LEN_BUFF_LONGLONGINT];
            snprintf(buff, sizeof(buff), "%llx",
                    (unsigned long long)(uintptr_t)workspaces[i].handle);

            handle = purc_variant_make_string(buff, false);
            purc_variant_object_set_by_static_ckey(value,
                    "handle", handle);
            purc_variant_unref(handle);

            purc_variant_object_set_by_static_ckey(result->data,
                    workspaces[i].name, value);
            purc_variant_unref(value);
        }
    }

    result->retCode = PCRDR_SC_OK;
    result->data_type = PCRDR_MSG_DATA_TYPE_JSON;
}

static void on_set_property(struct pcrdr_prot_data *prot_data,
        const pcrdr_msg *msg, unsigned int op_id, struct result_info *result)
{
    UNUSED_PARAM(prot_data);
    UNUSED_PARAM(msg);
    UNUSED_PARAM(op_id);

    /* always return a true value */
    result->retCode = PCRDR_SC_OK;
    result->data_type = PCRDR_MSG_DATA_TYPE_JSON;
    result->data = purc_variant_make_boolean(true);
}

static request_handler handlers[] = {
    on_start_session,
    on_end_session,
    on_create_workspace,
    on_update_workspace,
    on_destroy_workspace,
    on_create_plainwin,
    on_update_plainwin,
    on_destroy_plainwin,
    on_reset_page_groups,
    on_add_page_groups,
    on_remove_page_group,
    on_create_widget,
    on_update_widget,
    on_destroy_widget,
    on_load_from_url,
    on_load,
    on_write_begin,
    on_write_more,
    on_write_end,
    on_register,
    on_revoke,
    on_operate_dom,
    on_operate_dom,
    on_operate_dom,
    on_operate_dom,
    on_operate_dom,
    on_operate_dom,
    on_operate_dom,
    on_operate_dom,
    on_call_method,
    on_get_property,
    on_set_property,
};

/* make sure the number of operation handlers matches the enumulators */
#define _COMPILE_TIME_ASSERT(name, x)           \
       typedef int _dummy_ ## name[(x) * 2 - 1]
_COMPILE_TIME_ASSERT(ops,
        PCA_TABLESIZE(handlers) == PCRDR_NR_OPERATIONS);
#undef _COMPILE_TIME_ASSERT

static int evaluate_result(struct pcrdr_prot_data *prot_data,
        const pcrdr_msg *msg)
{
    purc_atom_t op_atom;
    struct result_info *result;

    result = calloc(1, sizeof(*result));

    op_atom = pcrdr_check_operation(
            purc_variant_get_string_const(msg->operation));
    if (op_atom == 0) {
        result->retCode = PCRDR_SC_BAD_REQUEST;
        result->resultValue = 0;
        goto done;
    }

    unsigned int op_id;
    if (pcrdr_operation_from_atom(op_atom, &op_id) == NULL) {
        result->retCode = PCRDR_SC_BAD_REQUEST;
        result->resultValue = 0;
        goto done;
    }

    handlers[op_id](prot_data, msg, op_id, result);

done:
    pcutils_kvlist_set(&prot_data->results,
            purc_variant_get_string_const(msg->requestId),
            &result);

    return 0;
}

static int my_send_message(pcrdr_conn* conn, pcrdr_msg *msg)
{
    fputs(">>>STT\n", conn->prot_data->fp);
    if (pcrdr_serialize_message(msg, (pcrdr_cb_write)write_sent_to_log,
                conn) < 0) {
        goto failed;
    }
    fputs("\n>>>END\n\n", conn->prot_data->fp);
    fflush(conn->prot_data->fp);

    evaluate_result(conn->prot_data, msg);
    return 0;

failed:
    return -1;
}

static int my_ping_peer(pcrdr_conn* conn)
{
    UNUSED_PARAM(conn);
    return 0;
}

static int my_disconnect(pcrdr_conn* conn)
{
    const char *name;
    void *next, *data;
    struct result_info *result;

    kvlist_for_each_safe(&conn->prot_data->results, name, next, data) {
        result = *(struct result_info **)data;

        pcutils_kvlist_remove(&conn->prot_data->results, name);
        free(result);
    }

    pcutils_kvlist_cleanup(&conn->prot_data->results);
    fclose(conn->prot_data->fp);
    if (conn->prot_data->session)
        free(conn->prot_data->session);
    free(conn->prot_data);
    return 0;
}

#define SCHEME_LOCAL_FILE  "file://"

pcrdr_msg *pcrdr_headless_connect(const char* renderer_uri,
        const char* app_name, const char* runner_name, pcrdr_conn** conn)
{
    char buff[PATH_MAX + 1];
    const char *logfile;
    pcrdr_msg *msg = NULL;
    int err_code = PCRDR_ERROR_NOMEM;

    *conn = NULL;
    if (!purc_is_valid_app_name(app_name) ||
            !purc_is_valid_runner_name(runner_name)) {
        err_code = PURC_ERROR_INVALID_VALUE;
        goto failed;
    }

    if ((*conn = calloc(1, sizeof(pcrdr_conn))) == NULL) {
        PC_DEBUG("Failed to allocate space for connection: %s\n",
                strerror (errno));
        err_code = PCRDR_ERROR_NOMEM;
        goto failed;
    }

    if (((*conn)->prot_data =
                calloc(1, sizeof(struct pcrdr_prot_data))) == NULL) {
        PC_DEBUG("Failed to allocate space for protocol data: %s\n",
                strerror (errno));
        err_code = PCRDR_ERROR_NOMEM;
        goto failed;
    }

    if (renderer_uri && strlen(renderer_uri) > sizeof(SCHEME_LOCAL_FILE)) {
        logfile = renderer_uri + sizeof(SCHEME_LOCAL_FILE) - 1;
    }
    else {
        int n = snprintf(buff, sizeof(buff),
                PCRDR_HEADLESS_LOGFILE_PATH_FORMAT, app_name, runner_name);

        if (n < 0) {
            purc_set_error(PURC_ERROR_OUTPUT);
            goto failed;
        }
        else if ((size_t)n >= sizeof(buff)) {
            purc_set_error(PURC_ERROR_TOO_SMALL_BUFF);
            goto failed;
        }

        logfile = buff;
    }

    (*conn)->prot_data->fp = fopen(logfile, "a");
    if ((*conn)->prot_data->fp == NULL) {
        PC_DEBUG("Failed to open logfile: %s\n", logfile);
        purc_set_error(purc_error_from_errno(errno));
        goto failed;
    }

    pcutils_kvlist_init(&(*conn)->prot_data->results, NULL);

    msg = pcrdr_make_response_message("0", NULL,
            PCRDR_SC_OK, 0,
            PCRDR_MSG_DATA_TYPE_PLAIN, RENDERER_FEATURES,
            sizeof (RENDERER_FEATURES) - 1);
    if (msg == NULL) {
        purc_set_error(PCRDR_ERROR_NOMEM);
        goto failed;
    }
    else {
        fputs("<<<STT\n", (*conn)->prot_data->fp);
        pcrdr_serialize_message(msg, (pcrdr_cb_write)write_recv_to_log,
                (*conn));
        fputs("\n<<<END\n\n", (*conn)->prot_data->fp);
        fflush((*conn)->prot_data->fp);
    }

    (*conn)->prot = PURC_RDRCOMM_HEADLESS;
    (*conn)->type = CT_PLAIN_FILE;
    (*conn)->fd = -1;
    (*conn)->timeout_ms = 10;   /* 10 milliseconds */
    (*conn)->srv_host_name = NULL;
    (*conn)->own_host_name = strdup(PCRDR_LOCALHOST);
    (*conn)->app_name = app_name;
    (*conn)->runner_name = runner_name;

    (*conn)->wait_message = my_wait_message;
    (*conn)->read_message = my_read_message;
    (*conn)->send_message = my_send_message;
    (*conn)->ping_peer = my_ping_peer;
    (*conn)->disconnect = my_disconnect;

    list_head_init (&(*conn)->pending_requests);
    return msg;

failed:
    if (*conn) {
        if ((*conn)->prot_data) {
            if ((*conn)->prot_data->fp) {
               fclose((*conn)->prot_data->fp);
            }
           free((*conn)->prot_data);
        }

        if ((*conn)->own_host_name)
           free((*conn)->own_host_name);
        free(*conn);
        *conn = NULL;
    }

    purc_set_error(err_code);
    return NULL;
}

