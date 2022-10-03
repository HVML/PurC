/*
 * thread.c -- The implementation of THREAD protocol.
 *      Created on 8 Mar 2022
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
#include "private/sorted-array.h"
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

struct pcrdr_prot_data {
    purc_atom_t rdr_atom;
};

static int my_wait_message(pcrdr_conn* conn, int timeout_ms)
{
    size_t count = 0;
    UNUSED_PARAM(conn);

    if (purc_inst_holding_messages_count(&count))
        return -1;

    if (count == 0) {
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

    return 1;
}

static pcrdr_msg *my_read_message(pcrdr_conn* conn)
{
    pcrdr_msg* msg = NULL;

    UNUSED_PARAM(conn);

    msg = purc_inst_take_away_message(0);
    if (msg == NULL) {
        purc_set_error(PCRDR_ERROR_UNEXPECTED);
        return NULL;
    }

    return msg;
}

static int my_send_message(pcrdr_conn* conn, pcrdr_msg *msg)
{
    if (purc_inst_move_message(conn->prot_data->rdr_atom, msg) > 0)
        return 0;

    return -1;
}

static int my_ping_peer(pcrdr_conn* conn)
{
    UNUSED_PARAM(conn);
    return 0;
}

static int my_disconnect(pcrdr_conn* conn)
{
    free(conn->prot_data);
    return 0;
}

#define SCHEMA_LOCAL_FILE  "file://"

pcrdr_msg *pcrdr_thread_connect(const char* renderer_uri,
        const char* app_name, const char* runner_name, pcrdr_conn** conn)
{
    pcrdr_msg *msg = NULL;
    int err_code = PCRDR_ERROR_NOMEM;

    *conn = NULL;
    if (!purc_is_valid_endpoint_name(renderer_uri) ||
            !purc_is_valid_app_name(app_name) ||
            !purc_is_valid_runner_name(runner_name)) {
        err_code = PURC_EXCEPT_INVALID_VALUE;
        goto failed;
    }

    purc_atom_t rdr_atom = purc_atom_try_string_ex(
            PURC_ATOM_BUCKET_DEF, renderer_uri);
    if (rdr_atom == 0) {
        err_code = PCRDR_ERROR_BAD_CONNECTION;
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

    (*conn)->prot = PURC_RDRCOMM_THREAD;
    (*conn)->type = CT_MOVE_BUFFER;
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

    (*conn)->prot_data->rdr_atom = rdr_atom;

    list_head_init (&(*conn)->pending_requests);

    /* read the initial response message from the rendere thread */
    int left_ms = PCRDR_DEF_TIME_EXPECTED * 1000;
    while (left_ms > 0) {
        if (my_wait_message(*conn, 10) == 0)
            left_ms -= 10;
        else
            break;
    }

    if (left_ms <= 0) {
        err_code = PCRDR_ERROR_TIMEOUT;
        goto failed;
    }

    msg = purc_inst_take_away_message(0);
    if (msg == NULL) {
        err_code = PCRDR_ERROR_UNEXPECTED;
        goto failed;
    }

    return msg;

failed:
    if (*conn) {
        if ((*conn)->prot_data) {
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

