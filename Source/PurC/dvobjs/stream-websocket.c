/*
 * @file stream-websocket.c
 *
 * @date 2023/10/12
 * @brief The implementation of `websocket` protocol for stream object.
 *
 * Copyright (C) 2023 FMSoft <https://www.fmsoft.cn>
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

#define _GNU_SOURCE
#include "config.h"
#include "stream.h"

#include "purc-variant.h"
#include "purc-runloop.h"
#include "purc-dvobjs.h"

#include "private/debug.h"
#include "private/dvobjs.h"
#include "private/list.h"
#include "private/interpreter.h"

#include <errno.h>

#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

#define MAX_FRAME_PAYLOAD_SIZE      (1024 * 4)
#define MAX_INMEM_MESSAGE_SIZE      (1024 * 64)

/* 512 KiB throttle threshold per stream */
#define SOCK_THROTTLE_THLD          (1024 * 512)

#define PING_NO_RESPONSE_SECONDS            30
#define MAX_PINGS_TO_FORCE_CLOSING          3

#define EVENT_TYPE_MESSAGE                  "message"
#   define EVENT_SUBTYPE_TEXT               "text"
#   define EVENT_SUBTYPE_BINARY             "binary"
#define EVENT_TYPE_CLOSE                    "close"
#define EVENT_TYPE_ERROR                    "error"
#   define EVENT_SUBTYPE_MESSAGE            "message"

/* The frame operation codes for WebSocket */
typedef enum ws_opcode {
    WS_OPCODE_CONTINUATION = 0x00,
    WS_OPCODE_TEXT = 0x01,
    WS_OPCODE_BIN = 0x02,
    WS_OPCODE_END = 0x03,
    WS_OPCODE_CLOSE = 0x08,
    WS_OPCODE_PING = 0x09,
    WS_OPCODE_PONG = 0x0A,
} ws_opcode;

/* The frame header for WebSocket */
typedef struct ws_frame_header {
    unsigned int fin;
    unsigned int rsv;
    unsigned int op;
    unsigned int mask;
    unsigned int sz_payload;
} ws_frame_header;

#define WS_OK                   0x00000000
#define WS_READING              0x00001000
#define WS_SENDING              0x00002000
#define WS_CLOSING              0x00004000
#define WS_THROTTLING           0x00008000
#define WS_WAITING4PAYLOAD      0x00010000

#define WS_ERR_ANY              0x00000FFF
#define WS_ERR_OOM              0x00000101
#define WS_ERR_IO               0x00000102
#define WS_ERR_MSG              0x00000104

typedef struct ws_pending_data {
    struct list_head list;

    /* the size of data */
    size_t  szdata;
    /* the size of sent */
    size_t  szsent;
    /* pointer to the pending data */
    unsigned char data[0];
} ws_pending_data;

struct stream_extended_data {
    /* the status of the client */
    unsigned            status;
    int                 msg_type;

    /* the time last got data from the peer */
    struct timespec     last_live_ts;

    size_t              sz_used_mem;
    size_t              sz_peak_used_mem;

    /* fields for pending data to write */
    size_t              sz_pending;
    struct list_head    pending;

    /* current frame header */
    ws_frame_header     header;
    size_t              sz_header;
    size_t              sz_read_header;

    /* fields for current reading message */
    size_t              sz_message;         /* total size of current message */
    size_t              sz_read_payload;    /* read size of current payload */
    size_t              sz_read_message;    /* read size of current message */
    char               *message;            /* message data */
};

static inline void ws_update_mem_stats(struct stream_extended_data *ext)
{
    ext->sz_used_mem = ext->sz_pending + ext->sz_message;
    if (ext->sz_used_mem > ext->sz_peak_used_mem)
        ext->sz_peak_used_mem = ext->sz_used_mem;
}

static int ws_status_to_pcerr(struct stream_extended_data *ext)
{
    if (ext->status & WS_ERR_OOM) {
        return PURC_ERROR_OUT_OF_MEMORY;
    }
    else if (ext->status & WS_ERR_IO) {
        return PURC_ERROR_BROKEN_PIPE;
    }
    else if (ext->status & WS_ERR_MSG) {
        return PURC_ERROR_NOT_DESIRED_ENTITY;
    }

    return PURC_ERROR_OK;
}

/* Clear pending data. */
static void ws_clear_pending_data(struct stream_extended_data *ext)
{
    struct list_head *p, *n;

    list_for_each_safe(p, n, &ext->pending) {
        list_del(p);
        free(p);
    }

    ext->sz_pending = 0;
    ws_update_mem_stats(ext);
}

static void cleanup_extension(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext0.data;

    if (stream->ext0.data) {
        pcintr_coroutine_post_event(stream->cid,
                PCRDR_MSG_EVENT_REDUCE_OPT_KEEP, stream->observed,
                EVENT_TYPE_CLOSE, NULL,
                PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);

        if (stream->monitor4r) {
            purc_runloop_remove_fd_monitor(purc_runloop_get_current(),
                    stream->monitor4r);
            stream->monitor4r = 0;
        }

        if (stream->monitor4w) {
            purc_runloop_remove_fd_monitor(purc_runloop_get_current(),
                    stream->monitor4w);
            stream->monitor4w = 0;
        }

        if (stream->fd4r >= 0) {
            close(stream->fd4r);
        }

        if (stream->fd4w >= 0 && stream->fd4w != stream->fd4r) {
            close(stream->fd4w);
        }
        stream->fd4r = -1;
        stream->fd4w = -1;

        ws_clear_pending_data(ext);
        if (ext->message)
            free(ext->message);
        free(ext);
        stream->ext0.data = NULL;

        free(stream->ext0.msg_ops);
        stream->ext0.msg_ops = NULL;
    }

}

/*
 * Queue new data.
 *
 * On success, true is returned.
 * On error, false is returned and the connection status is set.
 */
static bool ws_queue_data(struct pcdvobjs_stream *stream,
        const char *buf, size_t len)
{
    struct stream_extended_data *ext = stream->ext0.data;
    ws_pending_data *pending_data;

    if ((pending_data = malloc(sizeof(ws_pending_data) + len)) == NULL) {
        ws_clear_pending_data(ext);
        ext->status = WS_ERR_OOM | WS_CLOSING;
        return false;
    }

    memcpy(pending_data->data, buf, len);
    pending_data->szdata = len;
    pending_data->szsent = 0;

    list_add_tail(&pending_data->list, &ext->pending);
    ext->sz_pending += len;
    ws_update_mem_stats(ext);
    ext->status |= WS_SENDING;

    /* the connection probably is too slow, so stop queueing until everything
     * is sent */
    if (ext->sz_pending >= SOCK_THROTTLE_THLD) {
        ext->status |= WS_THROTTLING;
    }

    return true;
}

/*
 * Send the given buffer to the given socket.
 *
 * On error, -1 is returned and the connection status is set.
 * On success, the number of bytes sent is returned.
 */
static ssize_t ws_write_data(struct pcdvobjs_stream *stream,
        const char *buffer, size_t len)
{
    struct stream_extended_data *ext = stream->ext0.data;
    ssize_t bytes = 0;

    bytes = write(stream->fd4w, buffer, len);
    if (bytes == -1 && errno == EPIPE) {
        ext->status = WS_ERR_IO | WS_CLOSING;
        return -1;
    }

    /* did not send all of it... buffer it for a later attempt */
    if ((bytes > 0 && (size_t)bytes < len) ||
            (bytes == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))) {

        if (bytes == -1)
            bytes = 0;

        ws_queue_data(stream, buffer + bytes, len - bytes);
    }

    return bytes;
}

/*
 * Send the queued up data to the given socket.
 *
 * On error, -1 is returned and the connection status is set.
 * On success, the number of bytes sent is returned (maybe zero).
 */
static ssize_t ws_write_pending(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext0.data;
    ssize_t total_bytes = 0;
    struct list_head *p, *n;

    list_for_each_safe(p, n, &ext->pending) {
        ssize_t bytes;
        ws_pending_data *pending = (ws_pending_data *)p;

        bytes = write(stream->fd4w, pending->data + pending->szsent,
                pending->szdata - pending->szsent);

        if (bytes > 0) {
            pending->szsent += bytes;
            if (pending->szsent >= pending->szdata) {
                list_del(p);
                free(p);
            }
            else {
                break;
            }

            total_bytes += bytes;
            ext->sz_pending -= bytes;
            ws_update_mem_stats(ext);
        }
        else if (bytes == -1 && errno == EPIPE) {
            ext->status = WS_ERR_IO | WS_CLOSING;
            goto failed;
        }
        else if (bytes == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            break;
        }
    }

    return total_bytes;

failed:
    return -1;
}

/*
 * A wrapper of the system call write or send.
 *
 * On error, -1 is returned and the connection status is set as error.
 * On success, the number of bytes sent is returned.
 */
static ssize_t ws_write_sock(struct pcdvobjs_stream *stream,
        const void *buffer, size_t len)
{
    struct stream_extended_data *ext = stream->ext0.data;
    ssize_t bytes = 0;

    /* attempt to send the whole buffer */
    if (list_empty(&ext->pending)) {
        bytes = ws_write_data(stream, buffer, len);
    }
    /* the pending list not empty, just append new data if we're not
     * throttling the connection */
    else if (ext->sz_pending < SOCK_THROTTLE_THLD) {
        if (ws_queue_data(stream, buffer, len))
            return bytes;
    }
    /* send from pending buffer */
    else {
        bytes = ws_write_pending(stream);
    }

    return bytes;
}

/*
 * Tries to read from a socket. Returns for following values:
 *
 *  + 0: there is no data on the socket if the socket was marked as noblocking
 *  + > 0: the number of bytes read from the socket.
 *  + -1: for errors.
 */
static ssize_t ws_read_socket(struct pcdvobjs_stream *stream,
        void *buff, size_t sz)
{
    ssize_t bytes;

again:
    bytes = read(stream->fd4r, buff, sz);
    if (bytes == -1) {
        if (errno == EINTR) {
            goto again;
        }
        else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }
    }

    return bytes;
}

enum {
    READ_ERROR = -1,
    READ_NONE,
    READ_SOME,
    READ_WHOLE,
};

/* Tries to read a frame header. */
static int try_to_read_header(struct pcdvobjs_stream *stream)
{
    /* TODO */
    (void) stream;
    return READ_ERROR;
}

/*
 * Tries to read a payload. */
static int try_to_read_payload(struct pcdvobjs_stream *stream)
{
    /* TODO */
    (void) stream;
    return READ_ERROR;
}

static bool
ws_handle_reads(int fd, purc_runloop_io_event event, void *ctxt)
{
    (void)fd;
    (void)event;
    (void)ctxt;
    /* TODO */
    return false;
}

static bool
ws_handle_writes(int fd, purc_runloop_io_event event, void *ctxt)
{
    (void)fd;
    (void)event;
    struct pcdvobjs_stream *stream = ctxt;
    struct stream_extended_data *ext = stream->ext0.data;

    if (ext->status & WS_CLOSING) {
        pcintr_coroutine_post_event(stream->cid,
                PCRDR_MSG_EVENT_REDUCE_OPT_OVERLAY, stream->observed,
                EVENT_TYPE_CLOSE, NULL,
                PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
        cleanup_extension(stream);
        return false;
    }

    ws_write_pending(stream);
    if (list_empty(&ext->pending)) {
        ext->status &= ~WS_SENDING;
    }

    if (ext->status & WS_ERR_ANY) {
        stream->ext0.msg_ops->on_error(stream, ws_status_to_pcerr(ext));
    }

    return true;
}

/*
 * Send a PING message to the peer.
 *
 * return zero on success; none-zero on error.
 */
static int ws_ping_peer(struct pcdvobjs_stream *stream)
{
    (void) stream;
    /* TODO */
    return -1;
}

/*
 * Send a PONG message to the peer.
 *
 * return zero on success; none-zero on error.
 */
static int ws_pong_peer(struct pcdvobjs_stream *stream)
{
    (void) stream;
    /* TODO */
    return -1;
}

/*
 * Send a CLOSE message to the server
 *
 * return zero on success; none-zero on error.
 */
static int ws_notify_to_close(struct pcdvobjs_stream *stream)
{
    (void) stream;
    /* TODO */
    return -1;
}

static void mark_closing(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext0.data;
    if (ext->sz_pending == 0) {
        ws_notify_to_close(stream);
    }

    ext->status = WS_CLOSING;
}

static int ws_can_send_data(struct stream_extended_data *ext, size_t sz)
{
    if (sz > MAX_FRAME_PAYLOAD_SIZE) {
        size_t frames = sz / MAX_FRAME_PAYLOAD_SIZE + 1;
        if (ext->sz_pending + sz + (frames * ext->sz_header) >=
                SOCK_THROTTLE_THLD) {
            goto failed;
        }
    }
    else {
        if (ext->sz_pending + sz + ext->sz_header >= SOCK_THROTTLE_THLD) {
            goto failed;
        }
    }

    return 0;

failed:
    return -1;
}

/*
 * Send a message
 *
 * return zero on success; none-zero on error.
 */
static int send_data(struct pcdvobjs_stream *stream,
        bool text_or_binary, const char *data, size_t sz)
{
    (void) stream;
    (void) text_or_binary;
    (void) data;
    (void) sz;
    return PURC_ERROR_OK;
}

static int on_error(struct pcdvobjs_stream *stream, int errcode)
{
    purc_variant_t data = purc_variant_make_object_0();
    if (data) {
        purc_variant_t tmp;

        tmp = purc_variant_make_number(errcode);
        if (tmp) {
            purc_variant_object_set_by_static_ckey(data, "errCode",
                    tmp);
            purc_variant_unref(tmp);
        }

        tmp = purc_variant_make_string_static(
            purc_get_error_message(errcode), false);
        if (tmp) {
            purc_variant_object_set_by_static_ckey(data, "errMsg",
                    tmp);
            purc_variant_unref(tmp);
        }
    }

    pcintr_coroutine_post_event(stream->cid,
            PCRDR_MSG_EVENT_REDUCE_OPT_KEEP, stream->observed,
            EVENT_TYPE_ERROR, EVENT_SUBTYPE_MESSAGE,
            data, PURC_VARIANT_INVALID);
    return 0;
}

static int on_message(struct pcdvobjs_stream *stream, int type,
        const char *buf, size_t len)
{
    int retv = 0;
    purc_variant_t data = PURC_VARIANT_INVALID;

    switch (type) {
        case MT_TEXT:
            // fire a `message:text` event
            data = purc_variant_make_string(buf, true);
            pcintr_coroutine_post_event(stream->cid,
                    PCRDR_MSG_EVENT_REDUCE_OPT_KEEP, stream->observed,
                    EVENT_TYPE_MESSAGE, EVENT_SUBTYPE_TEXT,
                    data, PURC_VARIANT_INVALID);
            break;

        case MT_BINARY:
            // fire a `message:binary` event
            data = purc_variant_make_byte_sequence(buf, len);
            pcintr_coroutine_post_event(stream->cid,
                    PCRDR_MSG_EVENT_REDUCE_OPT_KEEP, stream->observed,
                    EVENT_TYPE_MESSAGE, EVENT_SUBTYPE_BINARY,
                    data, PURC_VARIANT_INVALID);
            break;

        case MT_PING:
            retv = ws_pong_peer(stream);
            break;

        case MT_PONG:
            break;

        case MT_CLOSE:
            pcintr_coroutine_post_event(stream->cid,
                    PCRDR_MSG_EVENT_REDUCE_OPT_OVERLAY, stream->observed,
                    EVENT_TYPE_CLOSE, NULL,
                    PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
            cleanup_extension(stream);
            break;
    }

    return retv;
}

static purc_variant_t
send_getter(void *entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    struct pcdvobjs_stream *stream = entity;

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    bool text_or_binary = true;
    const void *data = NULL;
    size_t len;
    if (purc_variant_is_string(argv[0])) {
        data = purc_variant_get_string_const_ex(argv[0], &len);
    }
    else if (purc_variant_is_bsequence(argv[0])) {
        text_or_binary = false;
        data = purc_variant_get_bytes_const(argv[0], &len);
    }

    if (data == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    int retv;
    if ((retv = send_data(stream, text_or_binary, data, len))) {
        purc_set_error(retv);
        goto failed;
    }

    return purc_variant_make_boolean(true);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY) {
        return purc_variant_make_boolean(false);
    }

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
close_getter(void *entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    struct pcdvobjs_stream *stream = entity;
    struct stream_extended_data *ext = stream->ext0.data;

    if (ext == NULL) {
        purc_set_error(PURC_ERROR_ENTITY_GONE);
        goto failed;
    }

    cleanup_extension(stream);
    return purc_variant_make_boolean(true);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY) {
        return purc_variant_make_boolean(false);
    }

    return PURC_VARIANT_INVALID;
}

static purc_nvariant_method
property_getter(void *entity, const char *name)
{
    (void)entity;
    purc_nvariant_method method = NULL;

    if (name == NULL) {
        goto failed;
    }

    if (strcmp(name, "send") == 0) {
        method = send_getter;
    }
    else if (strcmp(name, "close") == 0) {
        method = close_getter;
    }
    else {
        goto failed;
    }

    /* override the getters of parent */
    return method;

failed:
    purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    return NULL;
}

static bool on_observe(void *entity, const char *event_name,
        const char *event_subname)
{
    (void)entity;
    (void)event_name;
    (void)event_subname;

    return true;
}

static bool on_forget(void *entity, const char *event_name,
        const char *event_subname)
{
    (void)entity;
    (void)event_name;
    (void)event_subname;

    return true;
}

static void on_release(void *entity)
{
    struct pcdvobjs_stream *stream = entity;
    const struct purc_native_ops *super_ops = stream->ext0.super_ops;

    cleanup_extension(stream);

    if (super_ops->on_release) {
        return super_ops->on_release(entity);
    }
}

static const struct purc_native_ops msg_entity_ops = {
    .property_getter = property_getter,
    .on_observe = on_observe,
    .on_forget = on_forget,
    .on_release = on_release,
};

const struct purc_native_ops *
dvobjs_extend_stream_by_websocket(struct pcdvobjs_stream *stream,
        const struct purc_native_ops *super_ops, purc_variant_t extra_opts)
{
    (void)extra_opts;
    struct stream_extended_data *ext = NULL;
    struct stream_messaging_ops *msg_ops = NULL;

    if (super_ops == NULL || stream->ext0.signature[0]) {
        PC_ERROR("This stream has already extended by a Layer 0: %s\n",
                stream->ext0.signature);
        purc_set_error(PURC_ERROR_CONFLICT);
        goto failed;
    }

    if (fcntl(stream->fd4r, F_SETFL,
                fcntl(stream->fd4r, F_GETFL, 0) | O_NONBLOCK) == -1) {
        PC_ERROR("Unable to set socket as non-blocking: %s.", strerror(errno));
        purc_set_error(PURC_EXCEPT_IO_FAILURE);
        goto failed;
    }

    ext = calloc(1, sizeof(*ext));
    if (ext == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    list_head_init(&ext->pending);
    ext->sz_header = sizeof(ext->header);

    strcpy(stream->ext0.signature, STREAM_EXT_SIG_MSG);

    msg_ops = calloc(1, sizeof(*msg_ops));

    if (msg_ops) {
        msg_ops->send_data = send_data;
        msg_ops->on_error = on_error;
        msg_ops->mark_closing = mark_closing;

        msg_ops->on_message = on_message;
        msg_ops->cleanup = cleanup_extension;

        stream->ext0.data = ext;
        stream->ext0.super_ops = super_ops;
        stream->ext0.msg_ops = msg_ops;
    }
    else {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    stream->monitor4r = purc_runloop_add_fd_monitor(
            purc_runloop_get_current(), stream->fd4r, PCRUNLOOP_IO_IN,
            ws_handle_reads , stream);
    if (stream->monitor4r) {
        pcintr_coroutine_t co = pcintr_get_coroutine();
        if (co) {
            stream->cid = co->cid;
        }
    }
    else {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    stream->monitor4w = purc_runloop_add_fd_monitor(
            purc_runloop_get_current(), stream->fd4w, PCRUNLOOP_IO_OUT,
            ws_handle_writes, stream);
    if (stream->monitor4w) {
        pcintr_coroutine_t co = pcintr_get_coroutine();
        if (co) {
            stream->cid = co->cid;
        }
    }
    else {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    /* destroy rwstreams */
    if (stream->stm4r) {
        purc_rwstream_destroy(stream->stm4r);
    }

    if (stream->stm4w && stream->stm4w != stream->stm4r) {
        purc_rwstream_destroy(stream->stm4w);
    }

    stream->stm4w = NULL;
    stream->stm4r = NULL;

    PC_INFO("This socket is extended by Layer 0 protocol: message\n");
    return &msg_entity_ops;

failed:
    if (stream->monitor4r) {
        purc_runloop_remove_fd_monitor(purc_runloop_get_current(),
                stream->monitor4r);
        stream->monitor4r = 0;
    }

    if (msg_ops)
        free(msg_ops);
    if (ext)
        free(ext);

    return NULL;
}

