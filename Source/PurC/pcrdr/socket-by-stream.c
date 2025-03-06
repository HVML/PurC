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

#include "connect.h"

#include "private/list.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/stream.h"

#include "purc-pcrdr.h"
#include "purc-utils.h"
#include "purc-runloop.h"

#include <poll.h>
#include <errno.h>

#define SCHEMA_WEBSOCKET            "ws"
#define SCHEMA_SECURE_WEBSOCKET     "wss"
#define SCHEMA_LOCAL_SOCKET         "local"
#define SCHEMA_UNIX_SOCKET          "unix"
#define SCHEMA_INET_SOCKET          "inet"
#define USERNAME_INHERITED          "_inherited"

#define STREAM_PROTOCOL_MESSAGE     "message"
#define STREAM_PROTOCOL_WEBSOCKET   "websocket"

#define STREAM_EXT_SIG_PMC          "PMC"

struct pcrdr_prot_data {
    int                     errcode;
    purc_variant_t          dvobj;
    struct pcdvobjs_stream *stream;
    struct list_head        msgs;

    /* saved super operations */
    int (*on_message_super)(struct pcdvobjs_stream *stream, int type,
            char *msg, size_t len, int *owner_taken);
    int (*on_error_super)(struct pcdvobjs_stream *stream, int errcode);
    void (*cleanup_super)(struct pcdvobjs_stream *stream);
};

/* the header of the struct pcrdr_msg */
struct pcrdr_msg_hdr {
    unsigned int            refcnt;
    purc_atom_t             origin;
    struct list_head        ln;
};

static int my_wait_message(pcrdr_conn* conn, int timeout_ms)
{
#if 0
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
#else
     struct pollfd fds[] = {
         { conn->fd, POLLIN | POLLHUP | POLLERR, 0 },
         { conn->fd, POLLOUT | POLLERR, 0 },
     };

     int r = poll(fds, PCA_TABLESIZE(fds), timeout_ms < 0 ? -1 : timeout_ms);
     if (r < 0) {
         PC_ERROR("Failed poll(): %s\n", strerror(errno));
         goto done;
     }
     else if (r == 0) {
         PC_DEBUG("Timeout: %s\n", __func__);
         goto done;
     }

     struct pcdvobjs_stream *stream = conn->prot_data->stream;
     int events = 0;

     if (fds[0].revents & POLLIN) {
         events |= PCRUNLOOP_IO_IN;
     }
     if (fds[0].revents & POLLHUP) {
         events |= PCRUNLOOP_IO_HUP;
     }
     if (fds[0].revents & POLLERR) {
         events |= PCRUNLOOP_IO_ERR;
     }

     bool ret_read = false, ret_write = false;
     if (conn->prot_data && stream->ext0.msg_ops) {
         ret_read = stream->ext0.msg_ops->on_readable(conn->fd, events, stream);
     }

     events = 0;
     if (fds[1].revents & POLLOUT) {
         events |= PCRUNLOOP_IO_OUT;
     }
     if (fds[1].revents & POLLERR) {
         events |= PCRUNLOOP_IO_ERR;
     }

     if (conn->prot_data && stream->ext0.msg_ops) {
         ret_write = stream->ext0.msg_ops->on_writable(conn->fd, events, stream);
     }

     if (!ret_read || !ret_write) {
         PC_ERROR("Failed read of write: %s\n", strerror(errno));
         return -1;
     }

done:
     return r;
#endif
}

static pcrdr_msg *my_read_message_timeout(pcrdr_conn* conn, int max_wait)
{
    int total_wait = 0;;
    pcrdr_msg *msg = NULL;

    while (list_empty(&conn->prot_data->msgs)) {
        int r = my_wait_message(conn, conn->timeout_ms);
        if (r < 0) {
            PC_ERROR("Failed my_wait_message: %u\n", conn->timeout_ms);
            goto error;
        }
        else if (r == 0) {
            total_wait += conn->timeout_ms;
        }

        if (conn->prot_data->errcode) {
            PC_ERROR("Failed read/write: %d\n", conn->prot_data->errcode);
            goto error;
        }

        if (max_wait > 0 && total_wait >= max_wait) {
            PC_ERROR("Timeout: %d/%d\n", total_wait, max_wait);
            purc_set_error(PCRDR_ERROR_TIMEOUT);
            goto error;
        }
    }

    PC_DEBUG("in function: %s\n", __func__);
    struct pcrdr_msg_hdr *hdr;
    hdr = list_first_entry(&conn->prot_data->msgs, struct pcrdr_msg_hdr, ln);
    msg = (pcrdr_msg *)hdr;
    list_del(&hdr->ln);

error:
    return msg;
}

static pcrdr_msg *my_read_message(pcrdr_conn* conn)
{
    return my_read_message_timeout(conn, 0);
}

static int my_send_message(pcrdr_conn* conn, pcrdr_msg *msg)
{
    int retv = -1;
    purc_rwstream_t buffer = NULL;

    if (!(conn->prot_data && conn->prot_data->stream->ext0.msg_ops))
        return -1;

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
    if (conn->prot_data && conn->prot_data->stream->ext0.msg_ops) {
        struct pcdvobjs_stream *stream = conn->prot_data->stream;
        stream->ext0.msg_ops->on_ping_timer(NULL, NULL, stream);
        return 0;
    }

    return -1;
}

static int my_disconnect(pcrdr_conn* conn)
{
    if (conn->prot_data && conn->prot_data->stream->ext0.msg_ops) {
        struct pcdvobjs_stream *stream = conn->prot_data->stream;
        stream->ext0.msg_ops->shut_off(stream);
        purc_variant_unref(conn->prot_data->dvobj);
        return 0;
    }

    return -1;
}

static int on_message(struct pcdvobjs_stream *stream, int type,
            char *payload, size_t len, int *owner_taken)
{
    pcrdr_conn *conn = (pcrdr_conn *)stream->ext1.data;
    assert(conn);

    if (type != MT_TEXT) {
        /* call the method of Layer 0. */
        return conn->prot_data->on_message_super(stream, type, payload, len,
                owner_taken);
    }

    pcrdr_msg *msg = NULL;
    if (pcrdr_parse_packet(payload, len, &msg) < 0)
        return -1;

    struct pcrdr_msg_hdr *hdr = (struct pcrdr_msg_hdr *)msg;
    list_add_tail(&hdr->ln, &conn->prot_data->msgs);
    return 0;
}

static int on_error(struct pcdvobjs_stream *stream, int errcode)
{
    pcrdr_conn *conn = (pcrdr_conn *)stream->ext1.data;
    assert(conn);

    conn->prot_data->errcode = errcode;

    PC_ERROR("%s: Got an error: %d\n", __func__, errcode);

    /* call the method of Layer 0. */
    return conn->prot_data->on_error_super(stream, errcode);
}

static void cleanup_extension(struct pcdvobjs_stream *stream)
{
    pcrdr_conn *conn = (pcrdr_conn *)stream->ext1.data;
    assert(conn);

    struct list_head *p, *n;
    list_for_each_safe(p, n, &conn->prot_data->msgs) {
        struct pcrdr_msg_hdr *hdr;

        hdr = list_entry(p, struct pcrdr_msg_hdr, ln);
        list_del(p);
        pcrdr_release_message((pcrdr_msg *)hdr);
    }

    stream->ext1.data = NULL;

    if (conn->prot_data) {
        if (conn->prot_data->cleanup_super)
            conn->prot_data->cleanup_super(stream);
        free(conn->prot_data);
        conn->prot_data = NULL;
    }
}

static void on_release_stream_vrt(void *entity)
{
    struct pcdvobjs_stream *stream = entity;
    struct purc_native_ops *super_ops = stream->ext1.super_ops;

    cleanup_extension(stream);
    if (super_ops->on_release) {
        return super_ops->on_release(entity);
    }
}

static struct purc_native_ops purcmc_ops = {
    .property_getter = NULL,
    .on_observe = NULL,
    .on_forget = NULL,
    .on_release = on_release_stream_vrt,
};

#define NORMALIZE_BOOL_PROPERTY(name)                                   \
    tmp = purc_variant_object_get_by_ckey(extra_opts, #name);           \
    if (tmp && !purc_variant_is_boolean(tmp)) {                         \
        /* It must be a string */                                       \
        const char *str = purc_variant_get_string_const(tmp);           \
        bool name;                                                      \
        if (strcasecmp(str, "true") == 0 ||                             \
                strcasecmp(str, "yes") == 0 ||                          \
                strcasecmp(str, "1") == 0)                              \
            name = true;                                                \
        else if (strcasecmp(str, "false") == 0 ||                       \
                strcasecmp(str, "no") == 0 ||                           \
                strcasecmp(str, "0") == 0)                              \
            name = false;                                               \
        else                                                            \
            name = purc_variant_booleanize(tmp);                        \
                                                                        \
        tmp = purc_variant_make_boolean(name);                          \
        purc_variant_object_set_by_ckey(extra_opts, #name, tmp);        \
        purc_variant_unref(tmp);                                        \
    }

#define NORMALIZE_UINT_PROPERTY(name)                                   \
    do {                                                                \
        uint64_t name = 0;                                              \
        tmp = purc_variant_object_get_by_ckey(extra_opts, #name);       \
        if (tmp && purc_variant_cast_to_ulongint(tmp, &name, true)) {   \
            tmp = purc_variant_make_ulongint(name);                     \
            purc_variant_object_set_by_ckey(extra_opts, #name, tmp);    \
            purc_variant_unref(tmp);                                    \
        }                                                               \
        else {                                                          \
            purc_variant_object_remove_by_ckey(extra_opts, #name, true);\
        }                                                               \
    } while (0)

static void normalize_extra_options(purc_variant_t extra_opts)
{
    purc_variant_t tmp;

    NORMALIZE_BOOL_PROPERTY(secure);
    NORMALIZE_BOOL_PROPERTY(handshake);
    NORMALIZE_UINT_PROPERTY(maxframepayloadsize);
    NORMALIZE_UINT_PROPERTY(maxmessagesize);
    NORMALIZE_UINT_PROPERTY(noresptimetoping);
    NORMALIZE_UINT_PROPERTY(noresptimetoclose);

    purc_clr_error();
}

pcrdr_msg *
pcrdr_socket_connect(const char* renderer_uri,
        const char* app_name, const char* runner_name, pcrdr_conn** conn)
{
    pcrdr_msg *msg = NULL;
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

    normalize_extra_options(extra_opts);

    purc_variant_t stream_vrt;
    if (fd >= 0) {
        stream_vrt = dvobjs_create_stream_from_fd(fd, PURC_VARIANT_INVALID,
                prot, extra_opts);
        if (stream_vrt == PURC_VARIANT_INVALID) {
            PC_DEBUG ("Failed to create DVOBJ stream from fd: %d\n", fd);
            goto failed;
        }
    }
    else {
        stream_vrt = dvobjs_create_stream_from_url(url, PURC_VARIANT_INVALID,
                prot, extra_opts);
        if (stream_vrt == PURC_VARIANT_INVALID) {
            PC_DEBUG("Failed to create DVOBJ stream from url: %s\n", url);
            goto failed;
        }
    }

    if ((*conn = calloc(1, sizeof(pcrdr_conn))) == NULL) {
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

    prot_data->dvobj = stream_vrt;
    struct pcdvobjs_stream *stream;
    stream = purc_variant_native_get_entity(stream_vrt);
    prot_data->stream = stream;

    list_head_init(&prot_data->msgs);

    struct purc_native_ops *super_ops;
    super_ops = purc_variant_native_set_ops(stream_vrt, &purcmc_ops);

    assert(stream->ext0.data);
    assert(stream->ext0.msg_ops);

    /* extend the stream with PURCMC protocol */
    strcpy(stream->ext1.signature, STREAM_EXT_SIG_PMC);
    stream->ext1.data = (struct stream_extended_data *)*conn;
    stream->ext1.super_ops = super_ops;
    stream->ext1.bus_ops = NULL;

    /* override the some methods of Layer 0 */
    prot_data->on_message_super = stream->ext0.msg_ops->on_message;
    stream->ext0.msg_ops->on_message = on_message;
    prot_data->on_error_super = stream->ext0.msg_ops->on_error;
    stream->ext0.msg_ops->on_error = on_error;
    prot_data->cleanup_super = stream->ext0.msg_ops->cleanup;
    stream->ext0.msg_ops->cleanup = cleanup_extension;

    (*conn)->prot = PURC_RDRCOMM_SOCKET;
    (*conn)->type = stream->type == STREAM_TYPE_INET ? CT_INET_SOCKET :
        CT_UNIX_SOCKET;
    (*conn)->fd = stream->fd4r;
    (*conn)->timeout_ms = 10;   /* 10 milliseconds */
    (*conn)->srv_host_name = stream->peer_addr ? strdup(stream->peer_addr) :
        strdup("localhost");
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

    /* Got the initial message from the renderer */
    msg = my_read_message_timeout(*conn, 5000);  /* five seconds */
    if (msg)
        return msg;
    else
        PC_ERROR("Failed to get the initial message from the renderer in 5s.\n");

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

