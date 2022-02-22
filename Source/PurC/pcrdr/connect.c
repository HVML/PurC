/*
 * connect.c - The implementation of API to manage renderer connections.
 *
 * Copyright (c) 2021, 2022 FMSoft (http://www.fmsoft.cn)
 *
 * Authors:
 *  Vincent Wei (https://github.com/VincentWei), 2021, 2022
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
#include "purc/purc-rwstream.h"
#include "purc-pcrdr.h"
#include "private/kvlist.h"
#include "private/debug.h"
#include "private/utils.h"
#include "connect.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <assert.h>

pcrdr_request_handler pcrdr_conn_get_request_handler (pcrdr_conn *conn)
{
    return conn->request_handler;
}

pcrdr_request_handler pcrdr_conn_set_request_handler (pcrdr_conn *conn,
        pcrdr_request_handler request_handler)
{
    pcrdr_request_handler old = conn->request_handler;
    conn->request_handler = request_handler;

    return old;
}

pcrdr_event_handler pcrdr_conn_get_event_handler (pcrdr_conn *conn)
{
    return conn->event_handler;
}

pcrdr_event_handler pcrdr_conn_set_event_handler (pcrdr_conn *conn,
        pcrdr_event_handler event_handler)
{
    pcrdr_event_handler old = conn->event_handler;
    conn->event_handler = event_handler;

    return old;
}

void *pcrdr_conn_get_user_data (pcrdr_conn *conn)
{
    return conn->user_data;
}

void *pcrdr_conn_set_user_data (pcrdr_conn *conn, void *user_data)
{
    void *old = conn->user_data;
    conn->user_data = user_data;

    return old;
}

int pcrdr_conn_get_last_ret_code (pcrdr_conn *conn)
{
    return conn->last_ret_code;
}

const char* pcrdr_conn_srv_host_name (pcrdr_conn* conn)
{
    return conn->srv_host_name;
}

const char* pcrdr_conn_own_host_name (pcrdr_conn* conn)
{
    return conn->own_host_name;
}

const char* pcrdr_conn_app_name (pcrdr_conn* conn)
{
    return conn->app_name;
}

const char* pcrdr_conn_runner_name (pcrdr_conn* conn)
{
    return conn->runner_name;
}

int pcrdr_conn_socket_fd (pcrdr_conn* conn)
{
    return conn->fd;
}

int pcrdr_conn_socket_type (pcrdr_conn* conn)
{
    return conn->type;
}

purc_rdrprot_t pcrdr_conn_protocol (pcrdr_conn* conn)
{
    return (purc_rdrprot_t)conn->prot;
}

int pcrdr_free_connection (pcrdr_conn* conn)
{
    assert (conn);

    if (conn->srv_host_name)
        free (conn->srv_host_name);
    free (conn->own_host_name);
    free (conn->app_name);
    free (conn->runner_name);

    pcutils_kvlist_free (&conn->call_list);

    free (conn);

    return 0;
}

int pcrdr_ping_renderer (pcrdr_conn* conn)
{
    return conn->ping_peer(conn);
}

int pcrdr_disconnect (pcrdr_conn* conn)
{
    int err_code = 0;

    err_code = conn->disconnect (conn);
    pcrdr_free_connection (conn);

    if (err_code) {
        purc_set_error (err_code);
        err_code = -1;
    }

    return err_code;
}

int pcrdr_send_request(pcrdr_conn* conn, pcrdr_msg *request_msg,
        int seconds_expected, pcrdr_response_handler response_handler)
{
    if (conn->pending_request_id) {
        purc_set_error(PCRDR_ERROR_PENDING_REQUEST);
        return -1;
    }

    if (request_msg == NULL ||
            request_msg->type != PCRDR_MSG_TYPE_REQUEST ||
            request_msg->requestId == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    if (conn->send_message(conn, request_msg) < 0) {
        return -1;
    }

    conn->pending_request_id = strdup(request_msg->requestId);
    conn->response_handler = response_handler;
    if (seconds_expected <= 0 || seconds_expected > 3600)
        conn->time_expected = pcrdr_get_monotoic_time() + 3600;
    else
        conn->time_expected = pcrdr_get_monotoic_time() + seconds_expected;

    return 0;
}

int pcrdr_read_and_dispatch_message(pcrdr_conn* conn)
{
    pcrdr_msg* msg;
    int retval = 0;

    msg = conn->read_message(conn);
    if (msg == NULL) {
        retval = -1;
        goto done;
    }

    switch (msg->type) {
    case PCRDR_MSG_TYPE_VOID:
        // do nothiing
        break;

    case PCRDR_MSG_TYPE_EVENT:
        if (conn->event_handler) {
            conn->event_handler (conn, msg);
        }
        break;

    case PCRDR_MSG_TYPE_REQUEST:
        if (conn->request_handler) {
            conn->request_handler (conn, msg);
        }
        else {
            // TODO: send a fallback response message
        }
        break;

    case PCRDR_MSG_TYPE_RESPONSE:
        if (strcmp(msg->requestId, conn->pending_request_id) == 0) {
            if (conn->response_handler (conn, msg->requestId, msg) < 0)
                retval = -1;
            free (conn->pending_request_id);
            conn->pending_request_id = NULL;
        }
        else {
            purc_set_error (PCRDR_ERROR_PENDING_REQUEST);
            retval = -1;

            free (conn->pending_request_id);
            conn->pending_request_id = NULL;
        }
        break;

    default:
        purc_set_error (PCRDR_ERROR_BAD_MESSAGE);
        retval = -1;
        break;
    }

done:
    if (msg)
        pcrdr_release_message (msg);

    return retval;
}

int pcrdr_wait_and_dispatch_message (pcrdr_conn* conn, int timeout_ms)
{
    int retval;

    retval = conn->wait_message(conn, timeout_ms);

    if (retval < 0) {
        retval = -1;
        purc_set_error(PCRDR_ERROR_BAD_SYSTEM_CALL);
    }
    else if (retval > 0) {
        retval = pcrdr_read_and_dispatch_message (conn);
    }
    else {
        retval = -1;
        purc_set_error(PCRDR_ERROR_TIMEOUT);
    }

    return retval;
}

int pcrdr_send_request_and_wait_response(pcrdr_conn* conn,
        const pcrdr_msg *request_msg,
        int seconds_expected, pcrdr_msg **response_msg)
{
    int retval = 0;

    *response_msg = NULL;

    if (conn->pending_request_id) {
        purc_set_error(PCRDR_ERROR_PENDING_REQUEST);
        return -1;
    }

    if (request_msg == NULL ||
            request_msg->type != PCRDR_MSG_TYPE_REQUEST ||
            request_msg->requestId == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    if (conn->send_message(conn, request_msg) < 0) {
        return -1;
    }

    conn->pending_request_id = NULL;
    conn->response_handler = NULL;
    if (seconds_expected <= 0 || seconds_expected > 3600)
        conn->time_expected = pcrdr_get_monotoic_time() + 3600;
    else
        conn->time_expected = pcrdr_get_monotoic_time() + seconds_expected;

    while (1 /* hibus_get_monotoic_time () < conn->time_expected */) {

        int timeout_ms = conn->time_expected - pcrdr_get_monotoic_time ();
        timeout_ms *= 1000;

        retval = conn->wait_message(conn, timeout_ms);

        if (retval < 0) {
            retval = -1;
            purc_set_error(PCRDR_ERROR_BAD_SYSTEM_CALL);
        }
        else if (retval > 0) {
            pcrdr_msg* msg;

            msg = conn->read_message(conn);
            if (msg == NULL) {
                retval = -1;
                goto done;
            }

            switch (msg->type) {
            case PCRDR_MSG_TYPE_VOID:
                // do nothiing
                break;

            case PCRDR_MSG_TYPE_EVENT:
                if (conn->event_handler) {
                    conn->event_handler (conn, msg);
                }
                break;

            case PCRDR_MSG_TYPE_REQUEST:
                if (conn->request_handler) {
                    conn->request_handler (conn, msg);
                }
                else {
                    // TODO: send a fallback response message
                }
                break;

            case PCRDR_MSG_TYPE_RESPONSE:
                if (strcmp(msg->requestId, request_msg->requestId) == 0) {
                    *response_msg = msg;
                    goto done;
                }
                else {
                    purc_set_error (PCRDR_ERROR_PENDING_REQUEST);
                    retval = -1;
                    goto done;
                }
                break;

            default:
                purc_set_error (PCRDR_ERROR_BAD_MESSAGE);
                retval = -1;
                break;
            }
        }
        else {
            purc_set_error (PCRDR_ERROR_TIMEOUT);
            retval = -1;
            break;
        }
    }

done:
    return retval;
}

