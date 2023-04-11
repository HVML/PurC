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

#define LEN_BUFF_LONGLONGINT    128

#define NR_WORKSPACES           8
#define NR_TABBEDWINDOWS        8
#define NR_WIDGETS              32
#define NR_PLAINWINDOWS         256

#define __STRING(x) #x

#define RENDERER_FEATURES                                   \
    "HEADLESS:" PCRDR_PURCMC_PROTOCOL_VERSION_STRING "\n"   \
    "HTML:5.3/XGML:1.0/XML:1.0\n"                           \
    "workspace:" __STRING(8)                                \
    "/tabbedWindow:" __STRING(8)                            \
    "/widgetInTabbedWindow:" __STRING(32)                   \
    "/plainWindow:" __STRING(256)

struct tabbed_window_info {
    // the group identifier of the tabbed window
    const char *group;

    // handle of this tabbedWindow; NULL for not used slot.
    void *handle;

    // number of tabpages in this tabbedWindow
    int nr_widgets;

    // handles of all tabpages/widgets in this tabbedWindow
    void *tabpages[NR_WIDGETS];

    // handles of all DOM documents in all tabpages.
    void *domdocs[NR_WIDGETS];
};

struct workspace_info {
    // handle of this workspace; NULL for not used slot
    void *handle;

    // name of the workspace
    char *name;

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

    // plainwin/widget name (plainwin:hello@main) -> owners;
    struct kvlist        widget_owners;

    // widget group name (main) -> tabbedwindows;
    struct kvlist        group_tabbedwin;
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
    pcrdr_msg_data_type data_type;
    purc_variant_t data;
};

struct pcrdr_prot_data {
    // FILE pointer to serialize the message.
    FILE                *fp;

    // requestId -> results;
    struct kvlist        results;

    // FILE pointer to serialize the message.
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

    return msg;
}

typedef void (*request_handler)(struct pcrdr_prot_data *prot_data,
        const pcrdr_msg *msg, unsigned int op_id, struct result_info *result);

#define SZ_INITIAL_OSTACK   2

struct widget_ostack {
    const char *id;
    void *widget;
    uint64_t *owners;
    size_t alloc_size;
    size_t nr_owners;
};

static struct widget_ostack *create_widget_ostack(
        struct workspace_info *workspace, const char *id, void *widget)
{
    struct widget_ostack *ostack;
    ostack = calloc(1, sizeof(*ostack));
    if (ostack) {
         ostack->owners = calloc(SZ_INITIAL_OSTACK, sizeof(uintptr_t));
         if (ostack->owners == NULL)
             goto failed;

         ostack->widget = widget;
         ostack->alloc_size = SZ_INITIAL_OSTACK;
         ostack->nr_owners = 0;
    }

    ostack->id = pcutils_kvlist_set_ex(&workspace->widget_owners, id, &ostack);
    return ostack;

failed:
    free(ostack);
    return NULL;
}

static void destroy_widget_ostack(struct widget_ostack *ostack)
{
    free(ostack->owners);
    free(ostack);
}

static uint64_t
widget_ostack_register(struct widget_ostack *ostack, uint64_t owner)
{
    assert(owner != 0);

    for (size_t i = 0; i < ostack->nr_owners; i++) {
        if (owner == ostack->owners[i]) {
            purc_log_warn("RDR/HEADLESS: Duplicated owner (%llu) in stack\n",
                    (unsigned long long)owner);
            return 0;
        }
    }

    if (ostack->alloc_size < ostack->nr_owners + 1) {
        size_t new_size;
        new_size = pcutils_get_next_fibonacci_number(ostack->alloc_size);
        ostack->owners = realloc(ostack->owners, sizeof(uint64_t) * new_size);

        if (ostack->owners == NULL)
            goto failed;

        ostack->alloc_size = new_size;
    }

    ostack->owners[ostack->nr_owners] = owner;
    ostack->nr_owners++;
    return ostack->owners[ostack->nr_owners - 1];

failed:
    purc_log_error("RDR/HEADLESS: Memory failure in %s\n", __func__);
    return 0;
}

static uint64_t
widget_ostack_revoke(struct widget_ostack *ostack, uint64_t owner)
{
    if (ostack->nr_owners == 0) {
        purc_log_warn("RDR/HEADLESS: Empty owner stack\n");
        return 0;
    }

    size_t i;
    for (i = 0; i < ostack->nr_owners; i++) {
        if (owner == ostack->owners[i]) {
            break;
        }
    }

    if (i == ostack->nr_owners) {
        purc_log_warn("RDR/HEADLESS: Not registered owner (%llu)\n",
                (unsigned long long)owner);
        return 0;
    }

    ostack->nr_owners--;
    if (i == ostack->nr_owners) {
        if (i == 0)
            return 0;

        return ostack->owners[i - 1];
    }

    for (; i < ostack->nr_owners; i++) {
        ostack->owners[i] = ostack->owners[i + 1];
    }

    return 0;
}

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
    struct widget_ostack *ostack;

    assert(workspace->handle);

    /* TODO: generate window and/or widget destroyed events */
    kvlist_for_each_safe(&workspace->widget_owners, name, next, data) {
        ostack = *(struct widget_ostack **)data;

        pcutils_kvlist_delete(&workspace->widget_owners, name);
        destroy_widget_ostack(ostack);
    }

    pcutils_kvlist_free(&workspace->widget_owners);

    pcutils_kvlist_free(&workspace->group_tabbedwin);

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

    /* Since 120: use element for the name of worksapce */
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

    int i;
    struct workspace_info *workspaces = prot_data->session->workspaces;
    for (i = 0; i < NR_WORKSPACES; i++) {
        if (workspaces[i].handle && strcmp(workspaces[i].name, name) == 0) {
            /* Since 120, returns the exsiting workspace */
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

    if (i >= NR_WORKSPACES) {
        result->retCode = PCRDR_SC_NOT_FOUND;
        result->resultValue = msg->targetValue;
        return;
    }

    destroy_workspace(prot_data, i);
    prot_data->session->nr_workspaces--;

    result->retCode = PCRDR_SC_OK;
    result->resultValue = msg->targetValue;
}

/* plainwin:hello@main */
#define PREFIX_PLAINWIN         "plainwin:"
#define PREFIX_WIDGET           "widget:"
#define SEP_GROUP_NAME          "@"

#define MAX_PLAINWIN_ID     \
    (sizeof(PREFIX_PLAINWIN) + PURC_LEN_IDENTIFIER * 2 + 2)
#define MAX_WIDGET_ID     \
    (sizeof(PREFIX_WIDGET) + PURC_LEN_IDENTIFIER * 2 + 2)

static int check_and_make_plainwin_id(char *id_buf,
        const char *group, const char *name)
{
    if (group && !purc_is_valid_identifier(group))
        return -1;

    if (name == NULL || !purc_is_valid_identifier(name))
        return -1;

    strcpy(id_buf, PREFIX_PLAINWIN);
    strcat(id_buf, name);
    if (group) {
        strcat(id_buf, SEP_GROUP_NAME);
        strcat(id_buf, group);
    }

    return 0;
}

static int check_and_make_widget_id(char *id_buf,
        const char *group, const char *name)
{
    if (group == NULL || !purc_is_valid_identifier(group))
        return -1;

    if (name == NULL || !purc_is_valid_identifier(name))
        return -1;

    strcpy(id_buf, PREFIX_WIDGET);
    strcat(id_buf, name);
    strcat(id_buf, SEP_GROUP_NAME);
    strcat(id_buf, group);

    return 0;
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

    const char *group = NULL;
    const char *name = NULL;
    if (msg->elementType == PCRDR_MSG_ELEMENT_TYPE_ID) {
        group = purc_variant_get_string_const(msg->elementValue);
    }

    /* Since 120, use property to specify the window name */
    name = purc_variant_get_string_const(msg->property);
    if (name == NULL) {
        result->retCode = PCRDR_SC_BAD_REQUEST;
        result->resultValue = 0;
        return;
    }

    char idbuf[MAX_PLAINWIN_ID];
    if (check_and_make_plainwin_id(idbuf, group, name)) {
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

    /* Since 120, returns the window handle if the window existed */
    void **data;
    data = pcutils_kvlist_get(&workspaces[i].widget_owners, idbuf);
    if (data != NULL) {
        struct widget_ostack *ostack = *(struct widget_ostack **)data;
        result->retCode = PCRDR_SC_OK;
        result->resultValue = (uint64_t)(uintptr_t)ostack;
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

            struct widget_ostack *ostack;
            if ((ostack = create_widget_ostack(workspaces + i,
                    idbuf, workspaces[i].plain_windows + j)) == NULL) {
                result->retCode = PCRDR_SC_INSUFFICIENT_STORAGE;
                result->resultValue = 0;
                return;
            }

            workspaces[i].plain_windows[j] = ostack;
            break;
        }
    }

    assert(j < NR_PLAINWINDOWS);

    workspaces[i].nr_plain_windows++;
    result->retCode = PCRDR_SC_OK;
    result->resultValue = (uint64_t)(uintptr_t)workspaces[i].plain_windows[j];
    return;
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
        uint64_t handle = (uint64_t)(uintptr_t)&workspaces[i].plain_windows[j];
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
        uint64_t handle = (uint64_t)(uintptr_t)workspaces[i].plain_windows[j];
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
    struct widget_ostack *ostack = workspaces[i].plain_windows[j];
    pcutils_kvlist_delete(&workspaces[i].widget_owners, ostack->id);
    destroy_widget_ostack(ostack);

    workspaces[i].plain_windows[j] = NULL;
    workspaces[i].domdocs[j] = NULL;
    workspaces[i].nr_plain_windows--;

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

static struct tabbed_window_info *
create_or_get_tabbedwin(struct workspace_info *workspace, const char *group)
{
    void *data = pcutils_kvlist_get(&workspace->group_tabbedwin, group);
    if (data) {
        return *(struct tabbed_window_info **)data;
    }

    struct tabbed_window_info *tabbedwin = NULL;
    int j;
    for (j = 0; j < NR_TABBEDWINDOWS; j++) {
        tabbedwin = workspace->tabbed_windows + j;
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

    const char *group = NULL;
    const char *name = NULL;

    /* Since 120, use property to specify the window name */
    group = purc_variant_get_string_const(msg->elementValue);
    name = purc_variant_get_string_const(msg->property);
    if (group == NULL || name == NULL) {
        result->retCode = PCRDR_SC_BAD_REQUEST;
        result->resultValue = 0;
        return;
    }

    char idbuf[MAX_WIDGET_ID];
    if (check_and_make_widget_id(idbuf, group, name)) {
        result->retCode = PCRDR_SC_BAD_REQUEST;
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

    /* Since 120, returns the widget handle if the widget existed */
    void **data;
    data = pcutils_kvlist_get(&workspaces[i].widget_owners, idbuf);
    if (data != NULL) {
        struct widget_ostack *ostack = *(struct widget_ostack **)data;
        result->retCode = PCRDR_SC_OK;
        result->resultValue = (uint64_t)(uintptr_t)ostack;
        return;
    }

    struct tabbed_window_info* tabbedwin;
    tabbedwin = create_or_get_tabbedwin(workspaces + i, group);
    if (tabbedwin == NULL || tabbedwin->nr_widgets >= NR_WIDGETS) {
        result->retCode = PCRDR_SC_SERVICE_UNAVAILABLE;
        result->resultValue = msg->targetValue;
        return;
    }

    int k;
    for (k = 0; k < NR_WIDGETS; k++) {
        if (tabbedwin->tabpages[k] == NULL) {
            struct widget_ostack *ostack;
            if ((ostack = create_widget_ostack(workspaces + i,
                    idbuf, tabbedwin->tabpages + k)) == NULL) {
                result->retCode = PCRDR_SC_INSUFFICIENT_STORAGE;
                result->resultValue = 0;
                return;
            }

            tabbedwin->tabpages[k] = ostack;
            break;
        }
    }

    assert(k < NR_WIDGETS);

    tabbedwin->nr_widgets++;
    result->retCode = PCRDR_SC_OK;
    result->resultValue =
        (uint64_t)(uintptr_t)tabbedwin->tabpages[k];
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
        struct tabbed_window_info *twin = workspaces[i].tabbed_windows + j;
        for (k = 0; k < NR_WIDGETS; k++) {
            uint64_t handle = (uint64_t)(uintptr_t)twin->tabpages[k];
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
        struct tabbed_window_info *twin = workspaces[i].tabbed_windows + j;
        for (k = 0; k < NR_WIDGETS; k++) {
            uint64_t handle = (uint64_t)(uintptr_t)twin->tabpages[k];
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

    /* TODO: generate DOM document and/or tabpage destroyed events */
    struct widget_ostack *ostack = workspaces[i].tabbed_windows[j].tabpages[k];
    pcutils_kvlist_delete(&workspaces[i].widget_owners, ostack->id);
    destroy_widget_ostack(ostack);

    workspaces[i].tabbed_windows[j].tabpages[k] = NULL;
    workspaces[i].tabbed_windows[j].domdocs[k] = NULL;
    workspaces[i].tabbed_windows[j].nr_widgets--;

    result->retCode = PCRDR_SC_OK;
    result->resultValue = widget;
}

static void **find_domdoc_ptr(struct pcrdr_prot_data *prot_data,
        const pcrdr_msg *msg, struct result_info *result, void **widget)
{
    if (msg->target != PCRDR_MSG_TARGET_PLAINWINDOW &&
            msg->target != PCRDR_MSG_TARGET_WIDGET) {
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
                    if (widget)
                        *widget = &workspaces[i].plain_windows[j];
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
    else if (msg->target == PCRDR_MSG_TARGET_WIDGET) {
        int i, j, k;
        struct workspace_info *workspaces = prot_data->session->workspaces;
        for (i = 0; i < NR_WORKSPACES; i++) {
            for (j = 0; j < NR_TABBEDWINDOWS; j++) {
                for (k = 0; k < NR_WIDGETS; k++) {
                    uint64_t handle =
                        (uint64_t)(uintptr_t)
                        &workspaces[i].tabbed_windows[j].tabpages[k];
                    if (handle == msg->targetValue) {
                        if (widget)
                            *widget =
                                &workspaces[i].tabbed_windows[j].tabpages[k];
                        goto found_tp;
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
    struct widget_ostack *ostack;

    UNUSED_PARAM(op_id);

    /* Since 120, element must specify the handle of coroutine */
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
    uint64_t suppressed = widget_ostack_register(ostack, handle);
    result->data_type = PCRDR_MSG_DATA_TYPE_JSON;
    result->data = purc_variant_make_ulongint(suppressed);

    result->retCode = PCRDR_SC_OK;
    result->resultValue = (uint64_t)(uintptr_t)domdocs;
}

static void on_write_begin(struct pcrdr_prot_data *prot_data,
        const pcrdr_msg *msg, unsigned int op_id, struct result_info *result)
{
    void **domdocs;
    struct widget_ostack *ostack;

    UNUSED_PARAM(op_id);

    /* Since 120, element must specify the handle of coroutine */
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
    uint64_t suppressed = widget_ostack_register(ostack, handle);
    result->data_type = PCRDR_MSG_DATA_TYPE_JSON;
    result->data = purc_variant_make_ulongint(suppressed);

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
    struct widget_ostack *ostack;

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
    uint64_t suppressed = widget_ostack_register(ostack, handle);
    result->data_type = PCRDR_MSG_DATA_TYPE_JSON;
    result->data = purc_variant_make_ulongint(suppressed);

    result->retCode = PCRDR_SC_OK;
    result->resultValue = 0;
}

static void on_revoke(struct pcrdr_prot_data *prot_data,
        const pcrdr_msg *msg, unsigned int op_id, struct result_info *result)
{
    void **domdocs = NULL;
    struct widget_ostack *ostack;

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
    uint64_t reloaded = widget_ostack_revoke(ostack, handle);
    result->data_type = PCRDR_MSG_DATA_TYPE_JSON;
    result->data = purc_variant_make_ulongint(reloaded);

    result->retCode = PCRDR_SC_OK;
    result->resultValue = 0;
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

            for (int k = 0; k < NR_WIDGETS; k++) {
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

static void on_call_method(struct pcrdr_prot_data *prot_data,
        const pcrdr_msg *msg, unsigned int op_id, struct result_info *result)
{
    UNUSED_PARAM(prot_data);
    UNUSED_PARAM(msg);
    UNUSED_PARAM(op_id);

    result->retCode = PCRDR_SC_NOT_IMPLEMENTED;
    result->resultValue = 0;
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

    result->retCode = PCRDR_SC_NOT_IMPLEMENTED;
    result->resultValue = 0;
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
    on_create_widget,
    on_update_widget,
    on_destroy_widget,
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
            PCRDR_MSG_DATA_TYPE_PLAIN, RENDERER_FEATURES,
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
    list_head_init (&(*conn)->page_handles);
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

