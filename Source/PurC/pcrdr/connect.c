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
#include "purc-rwstream.h"
#include "purc-pcrdr.h"
#include "private/pcrdr.h"
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

    struct pending_request *pr, *n;
    list_for_each_entry_safe(pr, n, &conn->pending_requests, list) {
        if (pr->response_handler) {
            pr->response_handler(conn,
                    purc_variant_get_string_const(pr->request_id),
                    PCRDR_RESPONSE_CANCELLED, pr->context, NULL);
        }
        list_del(&pr->list);
        purc_variant_unref(pr->request_id);
        free(pr);
    }

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

    /* send endSession request to renderer */
    pcrdr_msg *msg = pcrdr_make_request_message(PCRDR_MSG_TARGET_SESSION, 0,
            PCRDR_OPERATION_ENDSESSION, NULL,
            PCRDR_MSG_ELEMENT_TYPE_VOID, NULL, NULL,
            PCRDR_MSG_DATA_TYPE_VOID, NULL, 0);
    if (msg) {
        pcrdr_send_request(conn, msg, PCRDR_TIME_DEF_EXPECTED, NULL, NULL);
        pcrdr_release_message(msg);
    }

    err_code = conn->disconnect (conn);
    pcrdr_free_connection (conn);

    if (err_code) {
        purc_set_error (err_code);
        err_code = -1;
    }

    return err_code;
}

int pcrdr_send_request(pcrdr_conn* conn, pcrdr_msg *request_msg,
        int seconds_expected,
        void *context, pcrdr_response_handler response_handler)
{
    struct pending_request *pr;

    if (request_msg == NULL ||
            request_msg->type != PCRDR_MSG_TYPE_REQUEST ||
            request_msg->requestId == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    if ((pr = malloc(sizeof(*pr))) == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }

    if (conn->send_message(conn, request_msg) < 0) {
        free(pr);
        return -1;
    }

    pr->request_id = purc_variant_ref(request_msg->requestId);
    pr->request_target = request_msg->target;
    pr->request_target_value = request_msg->targetValue;
    pr->response_handler = response_handler;
    pr->context = context;
    if (seconds_expected <= 0 || seconds_expected > 3600)
        pr->time_expected = purc_get_monotoic_time() + 3600;
    else
        pr->time_expected = purc_get_monotoic_time() + seconds_expected;
    list_add_tail(&pr->list, &conn->pending_requests);

    return 0;
}

static inline int
variant_strcmp(purc_variant_t a, purc_variant_t b)
{
    if (a == b)
        return 0;

    return strcmp(
            purc_variant_get_string_const(a),
            purc_variant_get_string_const(b));
}

static int
handle_response_message(pcrdr_conn* conn, const pcrdr_msg *msg)
{
    int retval = 0;

    if (!list_empty(&conn->pending_requests)) {
        struct pending_request *pr;
        pr = list_first_entry(&conn->pending_requests,
                struct pending_request, list);

        if (variant_strcmp(msg->requestId, pr->request_id) == 0) {
            if (pr->response_handler && pr->response_handler(conn,
                        purc_variant_get_string_const(msg->requestId),
                        PCRDR_RESPONSE_RESULT, pr->context, msg) < 0) {
                retval = -1;
            }

            list_del(&pr->list);
            purc_variant_unref(pr->request_id);
            free(pr);
        }
        else {
            purc_set_error(PCRDR_ERROR_UNEXPECTED);
            retval = -1;
        }
    }
    else {
        purc_set_error(PCRDR_ERROR_UNEXPECTED);
        retval = -1;
    }

    return retval;
}

static int
check_timeout_requests(pcrdr_conn *conn)
{
    struct pending_request *pr, *n;
    time_t now = purc_get_monotoic_time();

    list_for_each_entry_safe(pr, n, &conn->pending_requests, list) {
        if (now >= pr->time_expected) {
            if (pr->response_handler) {
                pr->response_handler(conn,
                    purc_variant_get_string_const(pr->request_id),
                        PCRDR_RESPONSE_TIMEOUT, pr->context, NULL);
            }

            list_del(&pr->list);
            purc_variant_unref(pr->request_id);
            free(pr);
        }
    }

    return 0;
}

static int
send_default_response_msg(pcrdr_conn *conn, purc_variant_t request_id)
{
    int retval = 0;
    pcrdr_msg msg;

    msg.type = PCRDR_MSG_TYPE_RESPONSE;
    msg.requestId = request_id;
    msg.retCode = PCRDR_SC_SERVICE_UNAVAILABLE;
    msg.resultValue = 0;
    msg.dataType = PCRDR_MSG_DATA_TYPE_VOID;
    msg.data = NULL;

    if (conn->send_message(conn, &msg) < 0) {
        retval = -1;
    }

    return retval;
}

int pcrdr_read_and_dispatch_message(pcrdr_conn *conn)
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
            retval = send_default_response_msg(conn, msg->requestId);
        }
        break;

    case PCRDR_MSG_TYPE_RESPONSE:
        retval = handle_response_message(conn, msg);
        break;

    default:
        purc_set_error (PCRDR_ERROR_BAD_MESSAGE);
        retval = -1;
        break;
    }

    check_timeout_requests(conn);

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

    check_timeout_requests(conn);
    return retval;
}

#define MSG_POINTER_INVALID     ((pcrdr_msg *)(-1))

static int
my_sync_response_handler(pcrdr_conn* conn,
        const char *request_id, int state,
        void *context, const pcrdr_msg *response_msg)
{
    pcrdr_msg **msg_buff = context;

    (void)conn;
    (void)state;
    (void)request_id;

    if (state == PCRDR_RESPONSE_RESULT)
        *msg_buff = (pcrdr_msg *)response_msg;
    else
        *msg_buff = MSG_POINTER_INVALID;

    return 0;
}

int pcrdr_send_request_and_wait_response(pcrdr_conn* conn,
        pcrdr_msg *request_msg,
        int seconds_expected, pcrdr_msg **response_msg)
{
    struct pending_request *pr;
    int retval = 0;

    *response_msg = NULL;

    if (request_msg == NULL ||
            request_msg->type != PCRDR_MSG_TYPE_REQUEST ||
            request_msg->requestId == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    if ((pr = malloc(sizeof(*pr))) == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }

    if (conn->send_message(conn, request_msg) < 0) {
        free(pr);
        return -1;
    }

    pr->request_id = purc_variant_ref(request_msg->requestId);
    pr->response_handler = my_sync_response_handler;
    pr->context = response_msg;
    if (seconds_expected <= 0 || seconds_expected > 3600)
        pr->time_expected = purc_get_monotoic_time() + 3600;
    else
        pr->time_expected = purc_get_monotoic_time() + seconds_expected;
    list_add_tail(&pr->list, &conn->pending_requests);

    while (*response_msg == NULL) {

        int timeout_ms = pr->time_expected - purc_get_monotoic_time();
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
                    retval = send_default_response_msg(conn, msg->requestId);
                }
                break;

            case PCRDR_MSG_TYPE_RESPONSE:
                retval = handle_response_message(conn, msg);
                break;

            default:
                purc_set_error (PCRDR_ERROR_BAD_MESSAGE);
                retval = -1;
                break;
            }

            if (msg != *response_msg)
                pcrdr_release_message(msg);
        }
        else {
            purc_set_error(PCRDR_ERROR_TIMEOUT);
            retval = -1;
            break;
        }

        check_timeout_requests(conn);

        if (*response_msg == MSG_POINTER_INVALID) {
            purc_set_error(PCRDR_ERROR_TIMEOUT);
            retval = -1;
        }

        if (retval < 0)
            break;
    }

done:
    return retval;
}

