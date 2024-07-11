/*
 * connect.c - The implementation of API to manage renderer connections.
 *
 * Copyright (C) 2021, 2022 FMSoft (http://www.fmsoft.cn)
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

pcrdr_extra_message_source
pcrdr_conn_get_extra_message_source(pcrdr_conn* conn, void **ctxt)
{
    if (ctxt)
        *ctxt = conn->source_ctxt;

    return conn->source_fn;
}

pcrdr_extra_message_source
pcrdr_conn_set_extra_message_source(pcrdr_conn* conn,
        pcrdr_extra_message_source source_fn, void *ctxt, void **old_ctxt)
{
    pcrdr_extra_message_source old = conn->source_fn;

    if (old_ctxt)
        *old_ctxt = conn->source_ctxt;

    conn->source_fn = source_fn;
    conn->source_ctxt = ctxt;

    return old;
}

pcrdr_request_handler pcrdr_conn_get_request_handler(pcrdr_conn *conn)
{
    return conn->request_handler;
}

pcrdr_request_handler pcrdr_conn_set_request_handler(pcrdr_conn *conn,
        pcrdr_request_handler request_handler)
{
    pcrdr_request_handler old = conn->request_handler;
    conn->request_handler = request_handler;

    return old;
}

pcrdr_event_handler pcrdr_conn_get_event_handler(pcrdr_conn *conn)
{
    return conn->event_handler;
}

pcrdr_event_handler pcrdr_conn_set_event_handler(pcrdr_conn *conn,
        pcrdr_event_handler event_handler)
{
    pcrdr_event_handler old = conn->event_handler;
    conn->event_handler = event_handler;

    return old;
}

void *pcrdr_conn_get_user_data(pcrdr_conn *conn)
{
    return conn->user_data;
}

void *pcrdr_conn_set_user_data(pcrdr_conn *conn, void *user_data)
{
    void *old = conn->user_data;
    conn->user_data = user_data;

    return old;
}

const char* pcrdr_conn_srv_host_name(pcrdr_conn* conn)
{
    return conn->srv_host_name;
}

const char* pcrdr_conn_own_host_name(pcrdr_conn* conn)
{
    return conn->own_host_name;
}

const char* pcrdr_conn_app_name(pcrdr_conn* conn)
{
    return conn->app_name;
}

const char* pcrdr_conn_runner_name(pcrdr_conn* conn)
{
    return conn->runner_name;
}

int pcrdr_conn_fd(pcrdr_conn* conn)
{
    return conn->fd;
}

int pcrdr_conn_type(pcrdr_conn* conn)
{
    return conn->type;
}

purc_rdrcomm_k pcrdr_conn_comm_method(pcrdr_conn* conn)
{
    return (purc_rdrcomm_k)conn->prot;
}

const struct pcrdr_conn_stats *pcrdr_conn_stats(pcrdr_conn* conn)
{
    conn->stats.duration_seconds =
        (uint64_t)purc_monotonic_time_after(conn->stats.start_time);
    return &conn->stats;
}

int pcrdr_conn_set_poll_timeout(pcrdr_conn* conn, int timeout_ms)
{
    if (timeout_ms < 0)
        return -1;

    int old = conn->timeout_ms;
    conn->timeout_ms = timeout_ms;
    return old;
}

size_t pcrdr_conn_pending_requests_count(pcrdr_conn* conn)
{
    size_t n = 0;
    struct pending_request *pr;
    list_for_each_entry(pr, &conn->pending_requests, list) {
        n++;
    }

    return n;
}

int pcrdr_free_connection(pcrdr_conn* conn)
{
    assert(conn);

    if (conn->name) {
        free(conn->name);
    }

    if (conn->id) {
        purc_atom_remove_string_ex(ATOM_BUCKET_RDRID, conn->uid);
        free(conn->uid);
    }

    if (conn->srv_host_name) {
        free(conn->srv_host_name);
    }
    free(conn->own_host_name);

    if (conn->uri) {
        purc_atom_remove_string_ex(ATOM_BUCKET_RDRID, conn->uri);
        free(conn->uri);
    }

    if (conn->caps) {
        pcrdr_release_renderer_capabilities(conn->caps);
    }

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

    free(conn);

    return 0;
}

int pcrdr_ping_renderer(pcrdr_conn* conn)
{
    return conn->ping_peer(conn);
}

int pcrdr_disconnect(pcrdr_conn* conn)
{
    int err_code = 0;

    /* send endSession request to renderer */
    pcrdr_msg *msg = pcrdr_make_request_message(PCRDR_MSG_TARGET_SESSION, 0,
            PCRDR_OPERATION_ENDSESSION, NULL, NULL,
            PCRDR_MSG_ELEMENT_TYPE_VOID, NULL, NULL,
            PCRDR_MSG_DATA_TYPE_VOID, NULL, 0);
    if (msg) {
        if (conn->type == CT_MOVE_BUFFER) {
            msg->sourceURI = purc_variant_make_string(
                    purc_get_endpoint(NULL), false);
        }
        pcrdr_send_request(conn, msg, PCRDR_TIME_DEF_EXPECTED, NULL, NULL);
        pcrdr_release_message(msg);
    }

    err_code = conn->disconnect(conn);
    pcrdr_free_connection(conn);

    if (err_code) {
        purc_set_error(err_code);
        err_code = -1;
    }

    return err_code;
}

int
pcrdr_set_handler_for_response_from_extra_source(pcrdr_conn* conn,
        purc_variant_t request_id, int seconds_expected, void *context,
        pcrdr_response_handler response_handler)
{
    if (purc_variant_get_string_const(request_id) == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    if (strcmp(PCRDR_REQUESTID_NORETURN,
                purc_variant_get_string_const(request_id)) == 0) {
        /* for request without return */
        return 0;
    }

    struct pending_request *pr;
    if ((pr = malloc(sizeof(*pr))) == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }

    pr->request_id = purc_variant_ref(request_id);
    pr->response_handler = response_handler;
    pr->context = context;
    if (seconds_expected <= 0 || seconds_expected > 3600)
        pr->time_expected = purc_get_monotoic_time() + 3600;
    else
        pr->time_expected = purc_get_monotoic_time() + seconds_expected;
    list_add_tail(&pr->list, &conn->pending_requests);

    return 0;
}

int pcrdr_send_request(pcrdr_conn* conn, pcrdr_msg *request_msg,
        int seconds_expected,
        void *context, pcrdr_response_handler response_handler)
{
    if (request_msg == NULL ||
            request_msg->type != PCRDR_MSG_TYPE_REQUEST ||
            request_msg->requestId == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    conn->stats.nr_requests_sent++;
    if (conn->send_message(conn, request_msg) < 0) {
        return -1;
    }

    return pcrdr_set_handler_for_response_from_extra_source(conn,
        request_msg->requestId, seconds_expected, context,
        response_handler);
}

int
pcrdr_send_event(pcrdr_conn *conn, pcrdr_msg *event_msg)
{
    if (event_msg == NULL ||
            event_msg->type != PCRDR_MSG_TYPE_EVENT) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    conn->stats.nr_events_sent++;
    if (conn->send_message(conn, event_msg) < 0) {
        return -1;
    }

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
    int retval = -1;

    if (!list_empty(&conn->pending_requests)) {
        struct pending_request *pr;
        pr = list_first_entry(&conn->pending_requests,
                struct pending_request, list);

        if (variant_strcmp(msg->requestId, pr->request_id) == 0) {
            const char *request_id =
                purc_variant_get_string_const(msg->requestId);
            if (pr->response_handler && pr->response_handler(conn,
                        request_id,
                        PCRDR_RESPONSE_RESULT, pr->context, msg) < 0) {
                purc_log_warn("response handler for %s returned failure\n",
                        request_id);
            }

            retval = 0;
            list_del(&pr->list);
            purc_variant_unref(pr->request_id);
            free(pr);
        }
        else if (strcmp("-",
                    purc_variant_get_string_const(msg->requestId))== 0) {
            /* FIXME: */
            purc_log_warn("ignore noreturn request\n");
            retval = 0;
        }
        else {
            purc_log_error("response not matched the first pending request\n");
            purc_set_error(PCRDR_ERROR_UNEXPECTED);
        }
    }
    else {
        purc_log_error("no pending request?\n");
        purc_set_error(PCRDR_ERROR_UNEXPECTED);
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

    if (strcmp(PCRDR_REQUESTID_NORETURN,
                purc_variant_get_string_const(request_id)) == 0) {
        /* for request without return */
        return 0;
    }

    msg.type = PCRDR_MSG_TYPE_RESPONSE;
    msg.requestId = request_id;
    msg.retCode = PCRDR_SC_SERVICE_UNAVAILABLE;
    msg.resultValue = 0;
    msg.dataType = PCRDR_MSG_DATA_TYPE_VOID;
    msg.data = NULL;

    conn->stats.nr_responses_sent++;
    if (conn->send_message(conn, &msg) < 0) {
        retval = -1;
    }

    return retval;
}

static int dispatch_message(pcrdr_conn *conn, pcrdr_msg *msg)
{
    int retval = 0;

    switch (msg->type) {
    case PCRDR_MSG_TYPE_VOID:
        PC_WARN("Got a void message.\n");
        break;

    case PCRDR_MSG_TYPE_EVENT:
        conn->stats.nr_events_recv++;
        if (conn->event_handler) {
            conn->event_handler(conn, msg);
        }
        else {
            PC_WARN("Got an event (%s) but not event handler set.\n",
                    purc_variant_get_string_const(msg->eventName));
        }
        break;

    case PCRDR_MSG_TYPE_REQUEST:
        conn->stats.nr_requests_recv++;
        if (conn->request_handler) {
            conn->request_handler(conn, msg);
        }
        else {
            PC_WARN("Got a request (%s) but not request handler set.\n",
                    purc_variant_get_string_const(msg->operation));
            retval = send_default_response_msg(conn, msg->requestId);
        }
        break;

    case PCRDR_MSG_TYPE_RESPONSE:
        conn->stats.nr_responses_recv++;
        retval = handle_response_message(conn, msg);
        break;

    default:
        purc_set_error(PCRDR_ERROR_BAD_MESSAGE);
        retval = -1;
        break;
    }

    /* when the message is not reserved by upper layer */
    if (msg->__padding1 == NULL)
        pcrdr_release_message(msg);
    return retval;
}

#define IDLE_EVENT      "rdrState:idle"
static inline void update_current_conn(pcrdr_conn *conn, const pcrdr_msg *msg)
{
    struct pcinst *inst = pcinst_current();
    if (inst->curr_conn == conn) {
        return;
    }

    if (msg->type != PCRDR_MSG_TYPE_EVENT || !msg->eventName
            || !purc_variant_is_string(msg->eventName)) {
        return;
    }

    const char *name = purc_variant_get_string_const(msg->eventName);
    if (strcmp(name, "rdrState:idle") != 0) {
        inst->curr_conn = conn;
    }
}

int pcrdr_read_and_dispatch_message(pcrdr_conn *conn)
{
    pcrdr_msg* msg;

    msg = conn->read_message(conn);
    if (msg == NULL) {
        return -1;
    }

    update_current_conn(conn, msg);
    dispatch_message(conn, msg);

    /* check extra source again */
    if (conn->source_fn) {
        msg = conn->source_fn(conn, conn->source_ctxt);
        if (msg) {
            dispatch_message(conn, msg);
        }
    }

    check_timeout_requests(conn);
    return 0;
}

int pcrdr_wait_and_dispatch_message(pcrdr_conn* conn, int timeout_ms)
{
    int retval;

    /* check extra source first */
    if (conn->source_fn) {
        pcrdr_msg *msg = conn->source_fn(conn, conn->source_ctxt);
        if (msg) {
            dispatch_message(conn, msg);
        }
    }

    retval = conn->wait_message(conn, timeout_ms);

    if (retval < 0) {
        retval = -1;
        purc_set_error(PCRDR_ERROR_BAD_SYSTEM_CALL);
    }
    else if (retval > 0) {
        retval = pcrdr_read_and_dispatch_message(conn);
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

    if (state == PCRDR_RESPONSE_RESULT) {
        pcrdr_msg *tmp;
        *msg_buff = tmp = (pcrdr_msg *)response_msg;
        tmp->__padding1 = (void *)response_msg;   /* mark as reserved */
    }
    else
        *msg_buff = MSG_POINTER_INVALID;

    return 0;
}

int
pcrdr_wait_response_for_specific_request(pcrdr_conn* conn,
        purc_variant_t request_id,
        int seconds_expected, pcrdr_msg **response_msg)
{
    int retval;

    /* VW: In order to suppress the dangling-pointer warning of GCC 12,
       we have to allocate this struct in heap,
       while it could have been allocated on the stack. */
    struct pending_request *pr = calloc(1, sizeof(*pr));
    assert(pr);

    pr->request_id = purc_variant_ref(request_id);
    pr->response_handler = my_sync_response_handler;
    pr->context = response_msg;
    if (seconds_expected <= 0 || seconds_expected > 3600)
        pr->time_expected = purc_get_monotoic_time() + 3600;
    else
        pr->time_expected = purc_get_monotoic_time() + seconds_expected;
    list_add(&pr->list, &conn->pending_requests);

    while (*response_msg == NULL) {
        pcrdr_msg *msg;

        if (conn->source_fn) {
            msg = conn->source_fn(conn, conn->source_ctxt);
            if (msg) {
                dispatch_message(conn, msg);
            }
        }

        retval = conn->wait_message(conn, conn->timeout_ms);

        if (retval < 0) {
            purc_set_error(PCRDR_ERROR_BAD_SYSTEM_CALL);
            retval = -1;
            break;
        }
        else if (retval > 0) {
            msg = conn->read_message(conn);
            if (msg == NULL) {
                retval = -1;
                break;
            }

            dispatch_message(conn, msg);
            /* check extra source again */
            if (conn->source_fn) {
                msg = conn->source_fn(conn, conn->source_ctxt);
                if (msg) {
                    dispatch_message(conn, msg);
                }
            }
        }
        else {
            /* do noting */
        }

        check_timeout_requests(conn);

        /* the response may have been marked timeout */
        if (*response_msg == MSG_POINTER_INVALID) {
            purc_set_error(PCRDR_ERROR_TIMEOUT);
            retval = -1;
            break;
        }
    }

    if (*response_msg == NULL) {
        list_del(&pr->list);
        purc_variant_unref(pr->request_id);
        free(pr);
    }
    else if (*response_msg == MSG_POINTER_INVALID) {
        *response_msg = NULL;   /* reset response messge to NULL */
    }

    return retval;
}

int pcrdr_send_request_and_wait_response(pcrdr_conn* conn,
        pcrdr_msg *request_msg,
        int seconds_expected, pcrdr_msg **response_msg)
{
    *response_msg = NULL;

    if (request_msg == NULL ||
            request_msg->type != PCRDR_MSG_TYPE_REQUEST ||
            request_msg->requestId == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    conn->stats.nr_requests_sent++;
    if (conn->send_message(conn, request_msg) < 0) {
        return -1;
    }

    return pcrdr_wait_response_for_specific_request(conn,
            request_msg->requestId, seconds_expected, response_msg);
}

