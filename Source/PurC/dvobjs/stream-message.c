/*
 * @file stream-message.c
 * @author Vincent Wei
 * @date 2023/05/28
 * @brief The implementation of `message` protocol for stream object.
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

/* The frame operation codes for UnixSocket */
typedef enum us_opcode {
    US_OPCODE_CONTINUATION = 0x00,
    US_OPCODE_TEXT = 0x01,
    US_OPCODE_BIN = 0x02,
    US_OPCODE_END = 0x03,
    US_OPCODE_CLOSE = 0x08,
    US_OPCODE_PING = 0x09,
    US_OPCODE_PONG = 0x0A,
} us_opcode;

/* The frame header for UnixSocket */
typedef struct us_frame_header {
    int op;
    unsigned int fragmented;
    unsigned int sz_payload;
    unsigned char payload[0];
} us_frame_header;

#define US_OK                   0x00000000
#define US_READING              0x00001000
#define US_SENDING              0x00002000
#define US_CLOSING              0x00004000
#define US_THROTTLING           0x00008000
#define US_WAITING4PAYLOAD      0x00010000

#define US_ERR_ANY              0x00000FFF
#define US_ERR_OOM              0x00000101
#define US_ERR_IO               0x00000102
#define US_ERR_MSG              0x00000104

typedef struct us_pending_data {
    struct list_head list;

    /* the size of data */
    size_t  szdata;
    /* the size of sent */
    size_t  szsent;
    /* pointer to the pending data */
    unsigned char data[0];
} us_pending_data;

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
    us_frame_header     header;
    size_t              sz_header;
    size_t              sz_read_header;

    /* fields for current reading message */
    size_t              sz_message;         /* total size of current message */
    size_t              sz_read_payload;    /* read size of current payload */
    size_t              sz_read_message;    /* read size of current message */
    char               *message;            /* message data */
};

static inline void us_update_mem_stats(struct stream_extended_data *ext)
{
    ext->sz_used_mem = ext->sz_pending + ext->sz_message;
    if (ext->sz_used_mem > ext->sz_peak_used_mem)
        ext->sz_peak_used_mem = ext->sz_used_mem;
}

static int us_status_to_pcerr(struct stream_extended_data *ext)
{
    if (ext->status & US_ERR_OOM) {
        return PURC_ERROR_OUT_OF_MEMORY;
    }
    else if (ext->status & US_ERR_IO) {
        return PURC_ERROR_BROKEN_PIPE;
    }
    else if (ext->status & US_ERR_MSG) {
        return PURC_ERROR_NOT_DESIRED_ENTITY;
    }

    return PURC_ERROR_OK;
}

/* Clear pending data. */
static void us_clear_pending_data(struct stream_extended_data *ext)
{
    struct list_head *p, *n;

    list_for_each_safe(p, n, &ext->pending) {
        list_del(p);
        free(p);
    }

    ext->sz_pending = 0;
    us_update_mem_stats(ext);
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

        us_clear_pending_data(ext);
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
static bool us_queue_data(struct pcdvobjs_stream *stream,
        const char *buf, size_t len)
{
    struct stream_extended_data *ext = stream->ext0.data;
    us_pending_data *pending_data;

    if ((pending_data = malloc(sizeof(us_pending_data) + len)) == NULL) {
        us_clear_pending_data(ext);
        ext->status = US_ERR_OOM | US_CLOSING;
        return false;
    }

    memcpy(pending_data->data, buf, len);
    pending_data->szdata = len;
    pending_data->szsent = 0;

    list_add_tail(&pending_data->list, &ext->pending);
    ext->sz_pending += len;
    us_update_mem_stats(ext);
    ext->status |= US_SENDING;

    /* the connection probably is too slow, so stop queueing until everything
     * is sent */
    if (ext->sz_pending >= SOCK_THROTTLE_THLD) {
        ext->status |= US_THROTTLING;
    }

    return true;
}

/*
 * Send the given buffer to the given socket.
 *
 * On error, -1 is returned and the connection status is set.
 * On success, the number of bytes sent is returned.
 */
static ssize_t us_write_data(struct pcdvobjs_stream *stream,
        const char *buffer, size_t len)
{
    struct stream_extended_data *ext = stream->ext0.data;
    ssize_t bytes = 0;

    bytes = write(stream->fd4w, buffer, len);
    if (bytes == -1 && errno == EPIPE) {
        ext->status = US_ERR_IO | US_CLOSING;
        return -1;
    }

    /* did not send all of it... buffer it for a later attempt */
    if ((bytes > 0 && (size_t)bytes < len) ||
            (bytes == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))) {

        if (bytes == -1)
            bytes = 0;

        us_queue_data(stream, buffer + bytes, len - bytes);
    }

    return bytes;
}

/*
 * Send the queued up data to the given socket.
 *
 * On error, -1 is returned and the connection status is set.
 * On success, the number of bytes sent is returned (maybe zero).
 */
static ssize_t us_write_pending(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext0.data;
    ssize_t total_bytes = 0;
    struct list_head *p, *n;

    list_for_each_safe(p, n, &ext->pending) {
        ssize_t bytes;
        us_pending_data *pending = (us_pending_data *)p;

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
            us_update_mem_stats(ext);
        }
        else if (bytes == -1 && errno == EPIPE) {
            ext->status = US_ERR_IO | US_CLOSING;
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
static ssize_t us_write_sock(struct pcdvobjs_stream *stream,
        const void *buffer, size_t len)
{
    struct stream_extended_data *ext = stream->ext0.data;
    ssize_t bytes = 0;

    /* attempt to send the whole buffer */
    if (list_empty(&ext->pending)) {
        bytes = us_write_data(stream, buffer, len);
    }
    /* the pending list not empty, just append new data if we're not
     * throttling the connection */
    else if (ext->sz_pending < SOCK_THROTTLE_THLD) {
        if (us_queue_data(stream, buffer, len))
            return bytes;
    }
    /* send from pending buffer */
    else {
        bytes = us_write_pending(stream);
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
static ssize_t us_read_socket(struct pcdvobjs_stream *stream,
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
    struct stream_extended_data *ext = stream->ext0.data;
    char *buf = (char *)&ext->header;
    ssize_t n;

    assert(ext->sz_header > ext->sz_read_header);

    n = us_read_socket(stream, buf + ext->sz_read_header,
            ext->sz_header - ext->sz_read_header);
    if (n > 0) {
        ext->sz_read_header += n;
        if (ext->sz_read_header == ext->sz_header) {
            ext->sz_read_header = 0;
            return READ_WHOLE;
        }
        ext->status |= US_READING;
    }
    else if (n < 0) {
        PC_ERROR("Failed to read frame header from Unix socket: %s\n",
                strerror(errno));
        ext->status = US_ERR_IO | US_CLOSING;
        return READ_ERROR;
    }
    else {
        /* no data */
        ext->status |= US_READING;
        return READ_NONE;
    }

    return READ_SOME;
}

/*
 * Tries to read a payload. */
static int try_to_read_payload(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext0.data;
    ssize_t n;

    switch (ext->header.op) {
    case US_OPCODE_TEXT:
    case US_OPCODE_BIN:
    case US_OPCODE_CONTINUATION:
    case US_OPCODE_END:
        assert(ext->header.sz_payload > ext->sz_read_payload);

        if (ext->message == NULL ||
                (ext->sz_read_message + ext->header.sz_payload) >
                    ext->sz_message) {
            ext->status = US_ERR_MSG | US_CLOSING;
            return READ_ERROR;
        }

        n = us_read_socket(stream,
                ext->message + ext->sz_read_message + ext->sz_read_payload,
                ext->header.sz_payload - ext->sz_read_payload);
        if (n > 0) {
            ext->sz_read_payload += n;

            PC_INFO("Read payload: %u/%u; message (%u/%u)\n",
                    (unsigned)ext->sz_read_payload,
                    (unsigned)ext->header.sz_payload,
                    (unsigned)ext->sz_read_message,
                    (unsigned)ext->sz_message);

            if (ext->sz_read_payload == ext->header.sz_payload) {
                ext->sz_read_payload = 0;
                ext->sz_read_message += ext->header.sz_payload;
                return READ_WHOLE;
            }
        }
        else if (n < 0) {
            PC_ERROR("Failed to read payload from Unix socket: %s\n",
                    strerror(errno));
            ext->status = US_ERR_IO | US_CLOSING;
            return READ_ERROR;
        }
        else {
            ext->status |= US_READING;
            return READ_NONE;
        }
        break;

    default:
        PC_ERROR("Unknown op code: %d\n", ext->header.op);
        ext->status = US_ERR_MSG | US_CLOSING;
        return READ_ERROR;
    }

    return READ_SOME;
}

static bool
us_handle_reads(int fd, purc_runloop_io_event event, void *ctxt)
{
    (void)fd;
    (void)event;
    struct pcdvobjs_stream *stream = ctxt;
    struct stream_extended_data *ext = stream->ext0.data;
    int retv;

#if 0
    double no_response_time = purc_get_elapsed_seconds(&ext->last_live_ts);
    if (no_response_time > PING_NO_RESPONSE_SECONDS) {
        ext->nr_nores_pings++;

        PC_WARN("The peer has no response for 30 seconds (%u times)\n",
                ext->nr_nores_pings);
        if (ext->nr_nores_pings > MAX_PINGS_TO_FORCE_CLOSING) {
            cleanup_extension(stream);
            return true;
        }
        else if (ext->sz_pending == 0) {
            us_ping_peer(stream);
        }
    }
    else {
        ext->nr_nores_pings = 0;
    }
#endif

    clock_gettime(CLOCK_MONOTONIC, &ext->last_live_ts);

    do {
        if (ext->status & US_CLOSING) {
            goto closing;
        }

        /* if it is not waiting for payload, read a frame header */
        if (!(ext->status & US_WAITING4PAYLOAD)) {
            retv = try_to_read_header(stream);
            if (retv == READ_NONE) {
                break;
            }
            else if (retv == READ_SOME) {
                continue;
            }
            else if (retv == READ_ERROR) {
                ext->status = US_ERR_IO | US_CLOSING;
                goto failed;
            }

            PC_INFO("Got a frame header: %d\n", ext->header.op);
            switch (ext->header.op) {
            case US_OPCODE_PING:
                ext->msg_type = MT_PING;
                retv = stream->ext0.msg_ops->on_message(stream, MT_PING, NULL, 0);
                break;

            case US_OPCODE_CLOSE:
                ext->msg_type = MT_CLOSE;
                retv = stream->ext0.msg_ops->on_message(stream, MT_CLOSE, NULL, 0);
                ext->status = US_CLOSING;
                break;

            case US_OPCODE_TEXT:
            case US_OPCODE_BIN: {
                ext->msg_type =
                    (ext->header.op == US_OPCODE_TEXT) ? MT_TEXT : MT_BINARY;
                if (ext->header.fragmented > 0 &&
                        ext->header.fragmented > ext->header.sz_payload) {
                    ext->sz_message = ext->header.fragmented;
                }
                else {
                    ext->sz_message = ext->header.sz_payload;
                }

                if (ext->sz_message > MAX_INMEM_MESSAGE_SIZE ||
                        ext->sz_message == 0 ||
                        ext->header.sz_payload == 0) {
                    ext->status = US_ERR_MSG | US_CLOSING;
                    goto failed;
                }

                /* always reserve a space for null character */
                ext->message = malloc(ext->sz_message + 1);
                if (ext->message == NULL) {
                    PC_ERROR("Failed to allocate memory for packet (size: %u)\n",
                            (unsigned)ext->sz_message);
                    ext->status = US_ERR_IO | US_CLOSING;
                    goto failed;
                }

                ext->sz_read_payload = 0;
                ext->sz_read_message = 0;
                us_update_mem_stats(ext);
                ext->status |= US_WAITING4PAYLOAD;
                break;
            }

            case US_OPCODE_CONTINUATION:
            case US_OPCODE_END:
                if (ext->header.sz_payload == 0) {
                    ext->status = US_ERR_MSG | US_CLOSING;
                    goto failed;
                }
                ext->status |= US_WAITING4PAYLOAD;
                break;

            case US_OPCODE_PONG:
                ext->msg_type = MT_PONG;
                retv = stream->ext0.msg_ops->on_message(stream, MT_PONG, NULL, 0);
                break;

            default:
                PC_ERROR("Unknown frame opcode: %d\n", ext->header.op);
                ext->status = US_ERR_MSG | US_CLOSING;
                goto failed;
                break;
            }
        }
        else if (ext->status & US_WAITING4PAYLOAD) {

            retv = try_to_read_payload(stream);
            PC_INFO("Got a new payload: %d\n", retv);
            if (retv == READ_WHOLE) {
                ext->status &= ~US_WAITING4PAYLOAD;

                if (ext->sz_read_message == ext->sz_message) {
                    if (ext->msg_type == MT_TEXT) {
                        ext->message[ext->sz_message] = 0;
                        ext->sz_message++;
                        PC_INFO("Got a text payload: %s\n", ext->message);
                    }

                    retv = stream->ext0.msg_ops->on_message(stream,
                            ext->msg_type, ext->message, ext->sz_message);
                    free(ext->message);
                    ext->message = NULL;
                    ext->sz_message = 0;
                    ext->sz_read_payload = 0;
                    ext->sz_read_message = 0;
                    us_update_mem_stats(ext);
                }
            }
            else if (retv == READ_NONE) {
                break;
            }
            else if (retv == READ_SOME) {
                continue;
            }
            else if (retv == READ_ERROR) {
                ext->status = US_ERR_IO | US_CLOSING;
                goto failed;
            }
        }
    } while (true);

    return true;

failed:
    stream->ext0.msg_ops->on_error(stream, us_status_to_pcerr(ext));

closing:
    if (ext->status & US_CLOSING) {
        pcintr_coroutine_post_event(stream->cid,
                PCRDR_MSG_EVENT_REDUCE_OPT_OVERLAY, stream->observed,
                EVENT_TYPE_CLOSE, NULL,
                PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
        cleanup_extension(stream);
    }

    return false;
}

static bool
us_handle_writes(int fd, purc_runloop_io_event event, void *ctxt)
{
    (void)fd;
    (void)event;
    struct pcdvobjs_stream *stream = ctxt;
    struct stream_extended_data *ext = stream->ext0.data;

    if (ext->status & US_CLOSING) {
        pcintr_coroutine_post_event(stream->cid,
                PCRDR_MSG_EVENT_REDUCE_OPT_OVERLAY, stream->observed,
                EVENT_TYPE_CLOSE, NULL,
                PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
        cleanup_extension(stream);
        return false;
    }

    us_write_pending(stream);
    if (list_empty(&ext->pending)) {
        ext->status &= ~US_SENDING;
    }

    if (ext->status & US_ERR_ANY) {
        stream->ext0.msg_ops->on_error(stream, us_status_to_pcerr(ext));
    }

    return true;
}

#if 0
/*
 * Send a PING message to the peer.
 *
 * return zero on success; none-zero on error.
 */
static int us_ping_peer(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext0.data;
    us_frame_header header;

    header.op = US_OPCODE_PING;
    header.fragmented = 0;
    header.sz_payload = 0;
    us_write_sock(stream, &header, sizeof(header));

    if (ext->status & US_ERR_ANY)
        return -1;
    return 0;
}
#endif

/*
 * Send a PONG message to the peer.
 *
 * return zero on success; none-zero on error.
 */
static int us_pong_peer(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext0.data;
    us_frame_header header;

    header.op = US_OPCODE_PONG;
    header.fragmented = 0;
    header.sz_payload = 0;
    us_write_sock(stream, &header, sizeof(header));

    if (ext->status & US_ERR_ANY)
        return -1;
    return 0;
}

/*
 * Send a CLOSE message to the server
 *
 * return zero on success; none-zero on error.
 */
static int us_notify_to_close(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext0.data;
    us_frame_header header;

    header.op = US_OPCODE_CLOSE;
    header.fragmented = 0;
    header.sz_payload = 0;
    us_write_sock(stream, &header, sizeof(header));

    if (ext->status & US_ERR_ANY)
        return -1;
    return 0;
}

static void mark_closing(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext0.data;
    if (ext->sz_pending == 0) {
        us_notify_to_close(stream);
    }

    ext->status = US_CLOSING;
}

static int us_can_send_data(struct stream_extended_data *ext, size_t sz)
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
    struct stream_extended_data *ext = stream->ext0.data;

    if (ext == NULL) {
        return PURC_ERROR_ENTITY_GONE;
    }

    if (sz > MAX_INMEM_MESSAGE_SIZE) {
        return PURC_ERROR_TOO_LARGE_ENTITY;
    }

    if ((ext->status & US_THROTTLING) || us_can_send_data(ext, sz)) {
        return PURC_ERROR_AGAIN;
    }

    ext->status = US_OK;

    us_frame_header header;

    if (sz > MAX_FRAME_PAYLOAD_SIZE) {
        const char* buff = data;
        unsigned int left = sz;

        do {
            if (left == sz) {
                header.op = text_or_binary ? US_OPCODE_TEXT : US_OPCODE_BIN;
                header.fragmented = sz;
                header.sz_payload = MAX_FRAME_PAYLOAD_SIZE;
                left -= MAX_FRAME_PAYLOAD_SIZE;
            }
            else if (left > MAX_FRAME_PAYLOAD_SIZE) {
                header.op = US_OPCODE_CONTINUATION;
                header.fragmented = 0;
                header.sz_payload = MAX_FRAME_PAYLOAD_SIZE;
                left -= MAX_FRAME_PAYLOAD_SIZE;
            }
            else {
                header.op = US_OPCODE_END;
                header.fragmented = 0;
                header.sz_payload = left;
                left = 0;
            }

            us_write_sock(stream, &header, sizeof(header));
            us_write_sock(stream, buff, header.sz_payload);
            buff += header.sz_payload;

        } while (left > 0);
    }
    else {
        header.op = text_or_binary ? US_OPCODE_TEXT : US_OPCODE_BIN;
        header.fragmented = 0;
        header.sz_payload = sz;
        us_write_sock(stream, &header, sizeof(header));
        us_write_sock(stream, data, sz);
    }

    if (ext->status & US_ERR_ANY) {
        PC_ERROR("Error when sending data: %s\n", strerror(errno));
        return us_status_to_pcerr(ext);
    }

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
            retv = us_pong_peer(stream);
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
dvobjs_extend_stream_by_message(struct pcdvobjs_stream *stream,
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
            us_handle_reads , stream);
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
            us_handle_writes, stream);
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

