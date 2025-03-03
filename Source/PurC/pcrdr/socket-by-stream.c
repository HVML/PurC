/*
 * socket-by-stream.c -- The implementation of socket method by extending
 *      the DVOBJ stream for PURCMC protocol.
 *
 * Copyright (C) 2025 FMSoft (http://www.fmsoft.cn)
 *
 * Authors:
 *  Vincent Wei (https://github.com/VincentWei), 2025
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
#include "private/list.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/stream.h"

#include "purc-pcrdr.h"
#include "purc-utils.h"
#include "connect.h"

#define SCHEMA_WEBSOCKET            "ws"
#define SCHEMA_SECURE_WEBSOCKET     "wss"
#define SCHEMA_LOCAL_SOCKET         "local"
#define SCHEMA_UNIX_SOCKET          "unix"
#define SCHEMA_INET_SOCKET          "inet"
#define USERNAME_INHERITED          "_inherited"

#define STREAM_PROTOCOL_MESSAGE     "message"
#define STREAM_PROTOCOL_WEBSOCKET   "websocket"

struct pcrdr_prot_data {
    purc_variant_t          dvobj;
    struct pcdvobjs_stream   *stream;
};

static int my_wait_message(pcrdr_conn* conn, int timeout_ms)
{
    fd_set rfds;
    struct timeval tv;

    FD_ZERO(&rfds);
    FD_SET(conn->fd, &rfds);

    if (timeout_ms >= 0) {
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        return select(conn->fd + 1, &rfds, NULL, NULL, &tv);
    }

    return select(conn->fd + 1, &rfds, NULL, NULL, NULL);
}

static pcrdr_msg *my_read_message(pcrdr_conn* conn)
{
    (void)conn;
    return NULL;
}

static int my_send_message(pcrdr_conn* conn, pcrdr_msg *msg)
{
    int retv = -1;
    purc_rwstream_t buffer = NULL;

    buffer = purc_rwstream_new_buffer(PCRDR_MIN_PACKET_BUFF_SIZE,
            PCRDR_MAX_INMEM_PAYLOAD_SIZE);

    if (pcrdr_serialize_message(msg,
                (pcrdr_cb_write)purc_rwstream_write, buffer) < 0) {
        goto done;
    }

    size_t packet_len;
    const char *packet = purc_rwstream_get_mem_buffer(buffer, &packet_len);

    if (conn->prot_data->stream->ext0.msg_ops->send_message(
            conn->prot_data->stream, true, packet, packet_len) < 0) {
        goto done;
    }

    conn->stats.bytes_sent += packet_len;
    retv = 0;

done:
    if (buffer) {
        purc_rwstream_destroy(buffer);
    }

    return retv;
}

static int my_ping_peer(pcrdr_conn* conn)
{
    (void)conn;
    return 0;
}

static int my_disconnect (pcrdr_conn* conn)
{
    (void)conn;
    return 0;
}

pcrdr_msg *
pcrdr_socket_connect(const char* renderer_uri,
        const char* app_name, const char* runner_name, pcrdr_conn** conn)
{
    struct purc_broken_down_url *bdurl = NULL;

    if (!purc_is_valid_app_name(app_name) ||
            !purc_is_valid_runner_name(runner_name)) {
        purc_set_error(PURC_EXCEPT_INVALID_VALUE);
        goto error;
    }

    bdurl = (struct purc_broken_down_url*)calloc(1,
            sizeof(struct purc_broken_down_url));
    if (!pcutils_url_break_down(bdurl, renderer_uri)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto error;
    }

    int fd = -1;
    if (bdurl->user && strcmp(bdurl->user, USERNAME_INHERITED) == 0) {
        /* The password filed of renderer URL is used as
           the inherited file descriptor. */
        if (bdurl->passwd) {
            char *endptr;
            long tmp = strtol(bdurl->passwd, &endptr, 10);
            if (*endptr != '\0') {
                PC_DEBUG("Bad file descriptor: %ld\n", tmp);
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                goto error;
            }

            if (tmp <= 0 || tmp > INT_MAX) {
                PC_DEBUG("Invalid file descriptor: %ld\n", tmp);
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                goto error;
            }

            fd = (int)tmp;
        }
    }

    purc_variant_t extra_opts = PURC_VARIANT_INVALID;
    const char *url = renderer_uri;
    pcrdr_msg *msg = NULL;

    if (bdurl->query)
        extra_opts = purc_make_object_from_query_string(bdurl->query, false);
    else
        extra_opts = purc_variant_make_object_0();

    if (extra_opts == PURC_VARIANT_INVALID) {
        goto failed;
    }

    const char *schema = NULL;
    const char *prot = STREAM_PROTOCOL_MESSAGE;
    if (strcasecmp(SCHEMA_SECURE_WEBSOCKET, bdurl->schema) == 0) {
        schema = SCHEMA_INET_SOCKET;
        prot = STREAM_PROTOCOL_WEBSOCKET;

        purc_variant_t tmp = purc_variant_make_boolean(true);
        purc_variant_object_set_by_static_ckey(extra_opts, "secure", tmp);
        purc_variant_unref(tmp);
    }
    else if (strcasecmp(SCHEMA_WEBSOCKET, bdurl->schema) == 0) {
        schema = SCHEMA_INET_SOCKET;
        prot = STREAM_PROTOCOL_WEBSOCKET;
    }
    else if (strcasecmp(SCHEMA_INET_SOCKET, bdurl->schema) == 0) {
        prot = STREAM_PROTOCOL_WEBSOCKET;
    }
    else if (strcasecmp(SCHEMA_LOCAL_SOCKET, bdurl->schema) == 0 ||
            strcasecmp(SCHEMA_UNIX_SOCKET, bdurl->schema) == 0) {
        prot = STREAM_PROTOCOL_MESSAGE;
    }

    if (prot == NULL) {
        purc_set_error(PURC_ERROR_NOT_SUPPORTED);
        goto error;
    }

    if (schema) {
        // rebuild the URL for websocket
        free(bdurl->schema);
        bdurl->schema = strdup(SCHEMA_INET_SOCKET);
        url = (const char *)pcutils_url_assemble(bdurl, true);
    }

    purc_variant_t stream;
    if (fd >= 0) {
        stream = dvobjs_create_stream_from_fd(fd, PURC_VARIANT_INVALID,
                prot, extra_opts);
        if (stream == NULL) {
            PC_DEBUG ("Failed to create DVOBJ stream from fd: %d\n", fd);
            goto failed;
        }
    }
    else {
        stream = dvobjs_create_stream_from_url(url, PURC_VARIANT_INVALID,
                prot, extra_opts);
        if (stream == NULL) {
            PC_DEBUG ("Failed to create DVOBJ stream from url: %s\n", url);
            goto failed;
        }
    }

    if ((*conn = calloc(1, sizeof (pcrdr_conn))) == NULL) {
        PC_DEBUG ("Failed to callocate space for connection\n");
        purc_set_error(PCRDR_ERROR_NOMEM);
        goto failed;
    }

    struct pcrdr_prot_data *prot_data;
    if ((prot_data =
                calloc(1, sizeof(struct pcrdr_prot_data))) == NULL) {
        PC_DEBUG("Failed to allocate space for protocol data\n");
        purc_set_error(PCRDR_ERROR_NOMEM);
        goto failed;
    }

    prot_data->dvobj = stream;
    prot_data->stream = purc_variant_native_get_entity(stream);
    assert(prot_data->stream->ext0.data);
    assert(prot_data->stream->ext0.msg_ops);

    (*conn)->prot = PURC_RDRCOMM_SOCKET;
    (*conn)->type = CT_WEB_SOCKET;
    (*conn)->fd = prot_data->stream->fd4r;
    (*conn)->timeout_ms = 10;   /* 10 milliseconds */
    (*conn)->srv_host_name = strdup(prot_data->stream->peer_addr);
    (*conn)->own_host_name = strdup(PCRDR_LOCALHOST);
    (*conn)->app_name = app_name;
    (*conn)->runner_name = runner_name;

    (*conn)->prot_data = prot_data;

    (*conn)->wait_message = my_wait_message;
    (*conn)->read_message = my_read_message;
    (*conn)->send_message = my_send_message;
    (*conn)->ping_peer = my_ping_peer;
    (*conn)->disconnect = my_disconnect;

    list_head_init(&(*conn)->pending_requests);

failed:
    if (url && url != renderer_uri)
        free((void *)url);

    if (msg)
        pcrdr_release_message(msg);

    if (*conn)
        pcrdr_disconnect(*conn);

    if (extra_opts)
        purc_variant_unref(extra_opts);

error:
    if (bdurl)
        pcutils_broken_down_url_delete(bdurl);

    return msg;
}

