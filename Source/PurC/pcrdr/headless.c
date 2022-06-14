/*
 * headless.c -- The implementation of HEADLESS protocol.
 *      Created on 7 Mar 2022
 *
 * Copyright (C) 2022 FMSoft (http://www.fmsoft.cn)
 *
 * Authors:
 *  Vincent Wei (https://github.com/VincentWei), 2022
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

#define NR_WORKSPACES           8
#define NR_TABBEDWINDOWS        8
#define NR_TABPAGES             32
#define NR_PLAINWINDOWS         256
#define NR_WINDOWLEVELS         2
#define NAME_WINDOW_LEVEL_0     "normal"
#define NAME_WINDOW_LEVEL_1     "topmost"

#define __STRING(x) #x

#define RENDERER_FEATURES                           \
    "HEADLESS:100\n"                                \
    "HTML:5.3/XGML:1.0/XML:1.0\n"                   \
    "workspace:" __STRING(8)                        \
    "/tabbedWindow:" __STRING(8)                    \
    "/tabbedPage:" __STRING(32)                     \
    "/plainWindow:" __STRING(256)                   \
    "/windowLevel:" __STRING(2) "\n"                \
    "windowLevels:" "normal" "," "topmost"

struct tabbed_window_info {
    // handle of this tabbedWindow; NULL for not used slot.
    void *handle;

    // number of tabpages in this tabbedWindow
    int nr_tabpages;

    // handles of all tabpages in this tabbedWindow
    void *tabpages[NR_TABPAGES];

    // handles of all DOM documents in all tabpages.
    void *domdocs[NR_TABPAGES];
};

struct workspace_info {
    // handle of this workspace; NULL for not used slot
    void *handle;

    // number of tabbed windows in this workspace
    int nr_tabbed_windows;

    // number of plain windows in this workspace
    int nr_plain_windows;

    // information of all tabbed windows in this workspace.
    struct tabbed_window_info tabbed_windows[NR_TABBEDWINDOWS];

    // handles of all plain windows in this workspace.
    void *plain_windows[NR_PLAINWINDOWS];

    // handles of DOM documents in all plain windows.
    void *domdocs[NR_PLAINWINDOWS];
};

struct session_info {
    // number of workspaces
    int nr_workspaces;

    // workspaces
    struct workspace_info workspaces[NR_WORKSPACES];
};

struct result_info {
    int         retCode;
    uint64_t    resultValue;
};

struct pcrdr_prot_data {
    // FILE pointer to serialize the message.
    FILE                *fp;

    // requestId -> results;
    struct kvlist        results;

    // FILE pointer to serialize the message.
    struct session_info *session;
};

static int my_wait_message(pcrdr_conn* conn, int timeout_ms)
{
    if (list_empty(&conn->pending_requests)) {
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

static ssize_t write_to_log(void *ctxt, const void *buf, size_t count)
{
    FILE *fp = (FILE *)ctxt;

    if (fwrite(buf, 1, count, fp) == count)
        return 0;

    return -1;
}

static pcrdr_msg *my_read_message(pcrdr_conn* conn)
{
    pcrdr_msg* msg = NULL;
    struct result_info **data;

    if (list_empty(&conn->pending_requests)) {
        purc_set_error(PCRDR_ERROR_UNEXPECTED);
        return NULL;
    }

    struct pending_request *pr;
    pr = list_first_entry(&conn->pending_requests,
            struct pending_request, list);

    const char *request_id;
    request_id = purc_variant_get_string_const(pr->request_id);

    data = pcutils_kvlist_get(&conn->prot_data->results, request_id);
    if (data) {
        struct result_info *result = *data;
        msg = pcrdr_make_response_message(
                purc_variant_get_string_const(pr->request_id), NULL,
                result->retCode, (uint64_t)(uintptr_t)result->resultValue,
                PCRDR_MSG_DATA_TYPE_VOID, NULL, 0);

        pcutils_kvlist_delete(&conn->prot_data->results, request_id);
        free(result);

        if (msg == NULL) {
            purc_set_error(PCRDR_ERROR_NOMEM);
        }
        else {
            fputs("<<<\n", conn->prot_data->fp);
            pcrdr_serialize_message(msg,
                        (pcrdr_cb_write)write_to_log, conn->prot_data->fp);
            fputs("\n<<<END\n", conn->prot_data->fp);
        }
    }
    else {
        purc_set_error(PCRDR_ERROR_UNEXPECTED);
        return NULL;
    }

    return msg;
}

typedef void (*request_handler)(struct pcrdr_prot_data *prot_data,
        const pcrdr_msg *msg, unsigned int op_id, struct result_info *result);

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

    free(prot_data->session);
    prot_data->session = NULL;

    result->retCode = PCRDR_SC_OK;
    result->resultValue = msg->targetValue;
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

    int i;
    struct workspace_info *workspaces = prot_data->session->workspaces;
    for (i = 0; i < NR_WORKSPACES; i++) {
        if (workspaces[i].handle == NULL) {
            workspaces[i].handle = &workspaces[i].handle;
            break;
        }
    }

    assert(i < NR_WORKSPACES);

    result->retCode = PCRDR_SC_OK;
    result->resultValue = (uint64_t)(uintptr_t)workspaces[i].handle;
    prot_data->session->nr_workspaces++;
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

    if (i >= NR_WORKSPACES) {
        result->retCode = PCRDR_SC_NOT_FOUND;
        result->resultValue = msg->targetValue;
        return;
    }

    /* TODO: generate window and/or tabpage destroyed events */
    memset(workspaces + i, 0, sizeof(struct workspace_info));
    prot_data->session->nr_workspaces--;

    result->retCode = PCRDR_SC_OK;
    result->resultValue = msg->targetValue;
}

static void on_create_plain_window(struct pcrdr_prot_data *prot_data,
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

    int i;
    struct workspace_info *workspaces = prot_data->session->workspaces;
    for (i = 0; i < NR_WORKSPACES; i++) {
        uint64_t workspace_handle = (uint64_t)(uintptr_t)&workspaces[i].handle;
        if (workspace_handle == msg->targetValue) {
            break;
        }
    }

    if (i >= NR_WORKSPACES) {
        result->retCode = PCRDR_SC_NOT_FOUND;
        result->resultValue = msg->targetValue;
        return;
    }

    if (workspaces[i].nr_plain_windows >= NR_PLAINWINDOWS) {
        result->retCode = PCRDR_SC_SERVICE_UNAVAILABLE;
        result->resultValue = msg->targetValue;
        return;
    }

    int j;
    for (j = 0; j < NR_PLAINWINDOWS; j++) {
        if (workspaces[i].plain_windows[j] == NULL) {
            workspaces[i].plain_windows[j] = &workspaces[i].plain_windows[j];
            break;
        }
    }

    assert(j < NR_PLAINWINDOWS);

    workspaces[i].nr_plain_windows++;
    result->retCode = PCRDR_SC_OK;
    result->resultValue = (uint64_t)(uintptr_t)workspaces[i].plain_windows[j];
}

static void on_update_plain_window(struct pcrdr_prot_data *prot_data,
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

    int i;
    struct workspace_info *workspaces = prot_data->session->workspaces;
    for (i = 0; i < NR_WORKSPACES; i++) {
        uint64_t workspace_handle = (uint64_t)(uintptr_t)&workspaces[i].handle;
        if (workspace_handle == msg->targetValue) {
            break;
        }
    }

    if (i >= NR_WORKSPACES) {
        result->retCode = PCRDR_SC_NOT_FOUND;
        result->resultValue = msg->targetValue;
        return;
    }

    uint64_t elementHandle = (uint64_t)strtoull(
            purc_variant_get_string_const(msg->elementValue), NULL, 16);

    int j;
    for (j = 0; j < NR_PLAINWINDOWS; j++) {
        uint64_t handle = (uint64_t)(uintptr_t)&workspaces[i].plain_windows[j];
        if (handle == elementHandle) {
            break;
        }
    }

    if (j >= NR_PLAINWINDOWS) {
        result->retCode = PCRDR_SC_NOT_FOUND;
        result->resultValue = elementHandle;
        return;
    }

    result->retCode = PCRDR_SC_OK;
    result->resultValue = elementHandle;
}

static void on_destroy_plain_window(struct pcrdr_prot_data *prot_data,
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

    int i;
    struct workspace_info *workspaces = prot_data->session->workspaces;
    for (i = 0; i < NR_WORKSPACES; i++) {
        uint64_t workspace_handle = (uint64_t)(uintptr_t)&workspaces[i].handle;
        if (workspace_handle == msg->targetValue) {
            break;
        }
    }

    if (i >= NR_WORKSPACES) {
        result->retCode = PCRDR_SC_NOT_FOUND;
        result->resultValue = msg->targetValue;
        return;
    }

    uint64_t elementHandle = (uint64_t)strtoull(
            purc_variant_get_string_const(msg->elementValue), NULL, 16);

    int j;
    for (j = 0; j < NR_PLAINWINDOWS; j++) {
        uint64_t handle = (uint64_t)(uintptr_t)&workspaces[i].plain_windows[j];
        if (handle == elementHandle) {
            break;
        }
    }

    if (j >= NR_PLAINWINDOWS) {
        result->retCode = PCRDR_SC_NOT_FOUND;
        result->resultValue = elementHandle;
        return;
    }

    /* TODO: generate DOM document and/or window destroyed events */
    workspaces[i].plain_windows[j] = NULL;
    workspaces[i].domdocs[j] = NULL;
    workspaces[i].nr_plain_windows--;

    result->retCode = PCRDR_SC_OK;
    result->resultValue = elementHandle;
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

    int i;
    struct workspace_info *workspaces = prot_data->session->workspaces;
    for (i = 0; i < NR_WORKSPACES; i++) {
        uint64_t workspace_handle = (uint64_t)(uintptr_t)&workspaces[i].handle;
        if (workspace_handle == msg->targetValue) {
            break;
        }
    }

    if (i >= NR_WORKSPACES) {
        result->retCode = PCRDR_SC_NOT_FOUND;
        result->resultValue = msg->targetValue;
        return;
    }

    if (workspaces[i].nr_tabbed_windows >= NR_TABBEDWINDOWS) {
        result->retCode = PCRDR_SC_SERVICE_UNAVAILABLE;
        result->resultValue = msg->targetValue;
        return;
    }

    int j;
    for (j = 0; j < NR_TABBEDWINDOWS; j++) {
        if (workspaces[i].tabbed_windows[j].handle == NULL) {
            workspaces[i].tabbed_windows[j].handle =
                &workspaces[i].tabbed_windows[j].handle;
            break;
        }
    }

    assert(j < NR_PLAINWINDOWS);

    workspaces[i].nr_tabbed_windows++;
    result->retCode = PCRDR_SC_OK;
    result->resultValue =
        (uint64_t)(uintptr_t)workspaces[i].tabbed_windows[j].handle;
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
    for (i = 0; i < NR_WORKSPACES; i++) {
        uint64_t workspace_handle = (uint64_t)(uintptr_t)&workspaces[i].handle;
        if (workspace_handle == msg->targetValue) {
            break;
        }
    }

    if (i >= NR_WORKSPACES) {
        result->retCode = PCRDR_SC_NOT_FOUND;
        result->resultValue = msg->targetValue;
        return;
    }

    uint64_t elementHandle = (uint64_t)strtoull(
            purc_variant_get_string_const(msg->elementValue), NULL, 16);

    int j;
    for (j = 0; j < NR_TABBEDWINDOWS; j++) {
        uint64_t handle =
            (uint64_t)(uintptr_t)&workspaces[i].tabbed_windows[j].handle;
        if (handle == elementHandle) {
            break;
        }
    }

    if (j >= NR_TABBEDWINDOWS) {
        result->retCode = PCRDR_SC_NOT_FOUND;
        result->resultValue = elementHandle;
        return;
    }

    result->retCode = PCRDR_SC_OK;
    result->resultValue = elementHandle;
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

    int i;
    struct workspace_info *workspaces = prot_data->session->workspaces;
    for (i = 0; i < NR_WORKSPACES; i++) {
        uint64_t workspace_handle = (uint64_t)(uintptr_t)&workspaces[i].handle;
        if (workspace_handle == msg->targetValue) {
            break;
        }
    }

    if (i >= NR_WORKSPACES) {
        result->retCode = PCRDR_SC_NOT_FOUND;
        result->resultValue = msg->targetValue;
        return;
    }

    uint64_t elementHandle = (uint64_t)strtoull(
            purc_variant_get_string_const(msg->elementValue), NULL, 16);

    int j;
    for (j = 0; j < NR_TABBEDWINDOWS; j++) {
        uint64_t handle =
            (uint64_t)(uintptr_t)&workspaces[i].tabbed_windows[j].handle;
        if (handle == elementHandle) {
            break;
        }
    }

    if (j >= NR_TABBEDWINDOWS) {
        result->retCode = PCRDR_SC_NOT_FOUND;
        result->resultValue = elementHandle;
        return;
    }

    /* TODO: generate DOM document and/or tabpages destroyed events */
    memset(workspaces[i].tabbed_windows + j, 0,
            sizeof(struct tabbed_window_info));
    workspaces[i].nr_tabbed_windows--;

    result->retCode = PCRDR_SC_OK;
    result->resultValue = elementHandle;
}

static void on_create_tabpage(struct pcrdr_prot_data *prot_data,
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

    int i, j;
    struct workspace_info *workspaces = prot_data->session->workspaces;
    for (i = 0; i < NR_WORKSPACES; i++) {
        for (j = 0; j < NR_TABBEDWINDOWS; j++) {
            uint64_t handle =
                (uint64_t)(uintptr_t)&workspaces[i].tabbed_windows[j].handle;
            if (handle == msg->targetValue) {
                goto found;
            }
        }
    }

found:
    if (i >= NR_WORKSPACES || j >= NR_TABBEDWINDOWS) {
        result->retCode = PCRDR_SC_NOT_FOUND;
        result->resultValue = msg->targetValue;
        return;
    }

    if (workspaces[i].tabbed_windows[j].nr_tabpages >= NR_TABPAGES) {
        result->retCode = PCRDR_SC_SERVICE_UNAVAILABLE;
        result->resultValue = msg->targetValue;
        return;
    }

    int k;
    for (k = 0; k < NR_TABPAGES; k++) {
        if (workspaces[i].tabbed_windows[j].tabpages[k] == NULL) {
            workspaces[i].tabbed_windows[j].tabpages[k] =
                &workspaces[i].tabbed_windows[j].tabpages[k];
            break;
        }
    }

    assert(k < NR_TABPAGES);

    workspaces[i].tabbed_windows[j].nr_tabpages++;
    result->retCode = PCRDR_SC_OK;
    result->resultValue =
        (uint64_t)(uintptr_t)workspaces[i].tabbed_windows[j].tabpages[k];
}

static void on_update_tabpage(struct pcrdr_prot_data *prot_data,
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

    int i, j;
    struct workspace_info *workspaces = prot_data->session->workspaces;
    for (i = 0; i < NR_WORKSPACES; i++) {
        for (j = 0; j < NR_TABBEDWINDOWS; j++) {
            uint64_t handle =
                (uint64_t)(uintptr_t)&workspaces[i].tabbed_windows[j].handle;
            if (handle == msg->targetValue) {
                goto found;
            }
        }
    }

found:
    if (i >= NR_WORKSPACES || j >= NR_TABBEDWINDOWS) {
        result->retCode = PCRDR_SC_NOT_FOUND;
        result->resultValue = msg->targetValue;
        return;
    }

    uint64_t elementHandle = (uint64_t)strtoull(
            purc_variant_get_string_const(msg->elementValue), NULL, 16);

    int k;
    for (k = 0; k < NR_TABPAGES; k++) {
        uint64_t handle =
            (uint64_t)(uintptr_t)workspaces[i].tabbed_windows[j].tabpages[k];
        if (handle == elementHandle) {
            break;
        }
    }

    if (k >= NR_TABPAGES) {
        result->retCode = PCRDR_SC_NOT_FOUND;
        result->resultValue = elementHandle;
        return;
    }

    result->retCode = PCRDR_SC_OK;
    result->resultValue = elementHandle;
}

static void on_destroy_tabpage(struct pcrdr_prot_data *prot_data,
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

    int i, j;
    struct workspace_info *workspaces = prot_data->session->workspaces;
    for (i = 0; i < NR_WORKSPACES; i++) {
        for (j = 0; j < NR_TABBEDWINDOWS; j++) {
            uint64_t handle =
                (uint64_t)(uintptr_t)&workspaces[i].tabbed_windows[j].handle;
            if (handle == msg->targetValue) {
                goto found;
            }
        }
    }

found:
    if (i >= NR_WORKSPACES || j >= NR_TABBEDWINDOWS) {
        result->retCode = PCRDR_SC_NOT_FOUND;
        result->resultValue = msg->targetValue;
        return;
    }

    uint64_t elementHandle = (uint64_t)strtoull(
            purc_variant_get_string_const(msg->elementValue), NULL, 16);

    int k;
    for (k = 0; k < NR_TABPAGES; k++) {
        uint64_t handle =
            (uint64_t)(uintptr_t)workspaces[i].tabbed_windows[j].tabpages[k];
        if (handle == elementHandle) {
            break;
        }
    }

    if (k >= NR_TABPAGES) {
        result->retCode = PCRDR_SC_NOT_FOUND;
        result->resultValue = elementHandle;
        return;
    }

    /* TODO: generate DOM document and/or tabpage destroyed events */
    workspaces[i].tabbed_windows[j].tabpages[k] = NULL;
    workspaces[i].tabbed_windows[j].domdocs[k] = NULL;
    workspaces[i].tabbed_windows[j].nr_tabpages--;

    result->retCode = PCRDR_SC_OK;
    result->resultValue = elementHandle;
}

static void **find_domdoc_ptr(struct pcrdr_prot_data *prot_data,
        const pcrdr_msg *msg, struct result_info *result)
{
    if (msg->target != PCRDR_MSG_TARGET_PLAINWINDOW &&
            msg->target != PCRDR_MSG_TARGET_PAGE) {
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
            for (j = 0; j < NR_PLAINWINDOWS; j++) {
                uint64_t handle =
                    (uint64_t)(uintptr_t)&workspaces[i].plain_windows[j];
                if (handle == msg->targetValue) {
                    goto found_pw;
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
    else if (msg->target == PCRDR_MSG_TARGET_PAGE) {
        int i, j, k;
        struct workspace_info *workspaces = prot_data->session->workspaces;
        for (i = 0; i < NR_WORKSPACES; i++) {
            for (j = 0; j < NR_TABBEDWINDOWS; j++) {
                for (k = 0; k < NR_TABPAGES; k++) {
                    uint64_t handle =
                        (uint64_t)(uintptr_t)
                        &workspaces[i].tabbed_windows[j].tabpages[k];
                    if (handle == msg->targetValue) {
                        goto found_tp;
                    }
                }
            }
        }

found_tp:
        if (i >= NR_WORKSPACES || j >= NR_TABBEDWINDOWS || k >= NR_TABPAGES) {
            result->retCode = PCRDR_SC_NOT_FOUND;
            result->resultValue = msg->targetValue;
            return NULL;
        }

        domdocs = &workspaces[i].tabbed_windows[j].domdocs[k];
    }
    else {
        result->retCode = PCRDR_SC_BAD_REQUEST;
        result->resultValue = 0;
        return NULL;
    }

    return domdocs;
}

static void on_load(struct pcrdr_prot_data *prot_data,
        const pcrdr_msg *msg, unsigned int op_id, struct result_info *result)
{
    void **domdocs;

    UNUSED_PARAM(op_id);
    if ((domdocs = find_domdoc_ptr(prot_data, msg, result)) == NULL) {
        return;
    }

    *domdocs = domdocs;

    result->retCode = PCRDR_SC_OK;
    result->resultValue = (uint64_t)(uintptr_t)domdocs;
}

static void on_write_begin(struct pcrdr_prot_data *prot_data,
        const pcrdr_msg *msg, unsigned int op_id, struct result_info *result)
{
    void **domdocs = NULL;

    UNUSED_PARAM(op_id);
    if (find_domdoc_ptr(prot_data, msg, result) == NULL) {
        return;
    }

    *domdocs = domdocs;

    result->retCode = PCRDR_SC_OK;
    result->resultValue = msg->targetValue;
}

static void on_write_more(struct pcrdr_prot_data *prot_data,
        const pcrdr_msg *msg, unsigned int op_id, struct result_info *result)
{
    void **domdocs = NULL;

    UNUSED_PARAM(op_id);
    if (find_domdoc_ptr(prot_data, msg, result) == NULL) {
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
    if (find_domdoc_ptr(prot_data, msg, result) == NULL) {
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

            if (workspaces[i].tabbed_windows[j].handle == NULL)
                continue;

            for (int k = 0; k < NR_TABPAGES; k++) {
                uint64_t handle =
                    (uint64_t)(uintptr_t)
                    &workspaces[i].tabbed_windows[j].domdocs[k];
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

static request_handler handlers[] = {
    on_start_session,
    on_end_session,
    on_create_workspace,
    on_update_workspace,
    on_destroy_workspace,
    on_create_plain_window,
    on_update_plain_window,
    on_destroy_plain_window,
    on_reset_page_groups,
    on_add_page_groups,
    on_remove_page_group,
    on_create_tabpage,
    on_update_tabpage,
    on_destroy_tabpage,
    on_load,
    on_write_begin,
    on_write_more,
    on_write_end,
    on_operate_dom,
    on_operate_dom,
    on_operate_dom,
    on_operate_dom,
    on_operate_dom,
    on_operate_dom,
    on_operate_dom,
    on_operate_dom,
    on_operate_dom, /* TODO: for callMethod */
    on_operate_dom, /* TODO: for getProperty */
    on_operate_dom, /* TODO: for setProperty */
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

    result = malloc(sizeof(*result));

    op_atom = pcrdr_check_operation(
            purc_variant_get_string_const(msg->operation));
    if (op_atom == 0) {
        result->retCode = PCRDR_SC_BAD_REQUEST;
        result->resultValue = 0;
        goto done;
    }

    unsigned int op_id;
    if (pcrdr_operation_from_atom(op_atom, &op_id) == NULL) {
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
    fputs(">>>\n", conn->prot_data->fp);
    if (pcrdr_serialize_message(msg,
                (pcrdr_cb_write)write_to_log, conn->prot_data->fp) < 0) {
        goto failed;
    }
    fputs("\n>>>END\n", conn->prot_data->fp);

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

        pcutils_kvlist_delete(&conn->prot_data->results, name);
        free(result);
    }

    pcutils_kvlist_free(&conn->prot_data->results);
    fclose(conn->prot_data->fp);
    if (conn->prot_data->session)
        free(conn->prot_data->session);
    free(conn->prot_data);
    return 0;
}

#define SCHEMA_LOCAL_FILE  "file://"

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
        err_code = PURC_EXCEPT_INVALID_VALUE;
        goto failed;
    }

    if ((*conn = calloc(1, sizeof(pcrdr_conn))) == NULL) {
        PC_DEBUG ("Failed to allocate space for connection: %s\n",
                strerror (errno));
        err_code = PCRDR_ERROR_NOMEM;
        goto failed;
    }

    if (((*conn)->prot_data =
                calloc(1, sizeof(struct pcrdr_prot_data))) == NULL) {
        PC_DEBUG ("Failed to allocate space for protocol data: %s\n",
                strerror (errno));
        err_code = PCRDR_ERROR_NOMEM;
        goto failed;
    }

    if (renderer_uri && strlen(renderer_uri) > sizeof(SCHEMA_LOCAL_FILE)) {
        logfile = renderer_uri + sizeof(SCHEMA_LOCAL_FILE) - 1;
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
        purc_set_error(PURC_ERROR_BAD_STDC_CALL);
        goto failed;
    }

    pcutils_kvlist_init(&(*conn)->prot_data->results, NULL);

    msg = pcrdr_make_response_message("0", NULL,
            PCRDR_SC_OK, 0,
            PCRDR_MSG_DATA_TYPE_TEXT, RENDERER_FEATURES,
            sizeof (RENDERER_FEATURES) - 1);
    if (msg == NULL) {
        purc_set_error(PCRDR_ERROR_NOMEM);
        goto failed;
    }
    else {
        fputs("<<<\n", (*conn)->prot_data->fp);
        pcrdr_serialize_message(msg,
                    (pcrdr_cb_write)write_to_log, (*conn)->prot_data->fp);
        fputs("\n<<<END\n", (*conn)->prot_data->fp);
    }

    (*conn)->prot = PURC_RDRPROT_HEADLESS;
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

