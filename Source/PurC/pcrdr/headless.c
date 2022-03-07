/*
 * purcmc.c -- The implementation of HEADLESS protocol.
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
#define NR_TABBEDPAGES          32
#define NR_PLAINWINDOWS         256
#define NR_WINDOWLEVELS         2
#define NAME_WINDOW_LEVEL_0     "normal"
#define NAME_WINDOW_LEVEL_1     "topmost"

#define __STRING(x) #x

#define RENDERER_FEATURES                           \
    "HEADLESS:100\n"                                \
    "HTML:5.3/XGML:1.0/XML:1.0\n"                   \
    "workspace:" __STRING(8)            \
    "/tabbedWindow:" __STRING(8)     \
    "/tabbedPage:" __STRING(32)         \
    "/plainWindow:" __STRING(256)       \
    "/windowLevel:" __STRING(2) "\n"  \
    "windowLevels:" "normal" "," "topmost"

struct tabbed_window_info {
    // handle of this tabbedWindow; NULL for not used slot.
    void *handle;

    // number of tabpages in this tabbedWindow
    int nr_tabpages;

    // handles of all tabpages in this tabbedWindow
    void *tabpages[NR_TABBEDPAGES];

    // handles of all DOM documents in all tabpages.
    void *domdocs[NR_TABBEDPAGES];
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
                purc_variant_get_string_const(pr->request_id),
                result->retCode, result->resultValue,
                PCRDR_MSG_DATA_TYPE_VOID, NULL, 0);

        pcutils_kvlist_delete(&conn->prot_data->results, request_id);
        free(result);

        if (msg == NULL) {
            purc_set_error(PCRDR_ERROR_NOMEM);
        }
        else {
            fputs("<<<\n", conn->prot_data->fp);
            pcrdr_serialize_message(msg,
                        (cb_write)write_to_log, conn->prot_data->fp);
            fputs("\n\n", conn->prot_data->fp);
        }
    }
    else {
        purc_set_error(PCRDR_ERROR_UNEXPECTED);
        return NULL;
    }

    return msg;
}

static int evaluate_result(struct pcrdr_prot_data *prot_data,
        const pcrdr_msg *msg)
{
    struct result_info *result;

    result = malloc(sizeof(*result));
    result->retCode = PCRDR_SC_OK;
    result->resultValue = 0;

    pcutils_kvlist_set(&prot_data->results,
            purc_variant_get_string_const(msg->requestId),
            &result);

    return 0;
}

static int my_send_message(pcrdr_conn* conn, pcrdr_msg *msg)
{
    fputs(">>>\n", conn->prot_data->fp);
    if (pcrdr_serialize_message(msg,
                (cb_write)write_to_log, conn->prot_data->fp) < 0) {
        goto failed;
    }
    fputs("\n\n", conn->prot_data->fp);

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
    free(conn->prot_data);
    return 0;
}

#define SCHEMA_LOCAL_FILE  "file://"

/* returns 0 if all OK, -1 on error */
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

    // TODO: open log file here.
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

    msg = pcrdr_make_response_message("0",
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
                    (cb_write)write_to_log, (*conn)->prot_data->fp);
        fputs("\n\n", (*conn)->prot_data->fp);
    }

    (*conn)->prot = PURC_RDRPROT_HEADLESS;
    (*conn)->type = CT_PLAIN_FILE;
    (*conn)->fd = -1;
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

