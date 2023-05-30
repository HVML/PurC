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
#define MAX_INMEM_MESSAGE_SIZE      (1024 * 40)

/* 1 MiB throttle threshold per client */
#define SOCK_THROTTLE_THLD          (1024 * 1024)

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

typedef enum us_state {
    US_OK = 0,
    US_ERR = (1 << 0),
    US_CLOSE = (1 << 1), US_READING = (1 << 2),
    US_SENDING = (1 << 3),
    US_THROTTLING = (1 << 4),
    US_WATING_FOR_HEADER = (1 << 5),
    US_WATING_FOR_PAYLOAD = (1 << 6),
} us_state;

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

    /* time got the first frame of the current packet */
    struct timespec     ts;

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
    int                 msg_type;
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

/*
 * Clear pending data.
 */
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
        ext->status = US_ERR | US_CLOSE;
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
    if (ext->sz_pending >= SOCK_THROTTLE_THLD)
        ext->status |= US_THROTTLING;

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
        ext->status = US_ERR | US_CLOSE;
        return -1;
    }

    /* did not send all of it... buffer it for a later attempt */
    if ((bytes > 0 && (size_t)bytes < len) ||
            (bytes == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))) {
        us_queue_data(stream, buffer + bytes, len - bytes);

        if (ext->status & US_SENDING && stream->ext0.msg_ops->on_pending)
            stream->ext0.msg_ops->on_pending(stream);
    }

    return bytes;
}

/*
 * Send the queued up data to the given socket.
 *
 * On error, -1 is returned and the connection status is set.
 * On success, the number of bytes sent is returned.
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
            ext->status = US_ERR | US_CLOSE;
            return -1;
        }
        else if (bytes == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return -1;
        }
    }

    return total_bytes;
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

/*
 * Tries to read a frame header; return values:
 *  1: a frame header read;
 *  0: no data;
 *  -1: error
 */
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
            return 1;
        }
    }
    else if (n < 0) {
        PC_ERROR("Failed to read payload from Unix socket: %s\n",
                strerror(errno));
        ext->status = US_ERR | US_CLOSE;
        return -1;
    }
    else {
        ext->status |= US_READING;
        /* no data */
    }

    return 0;
}

/*
 * Tries to read a payload; return values:
 *  0: payload read or no data;
 *  1: got whole message;
 *  -1: error
 */
static int try_to_read_payload(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext0.data;
    ssize_t n;

    switch (ext->header.op) {
    case US_OPCODE_TEXT:
    case US_OPCODE_BIN:
        assert(ext->sz_read_message == 0);
        assert(ext->header.sz_payload > ext->sz_read_payload);

        n = us_read_socket(stream,
                ext->message + ext->sz_read_payload,
                ext->header.sz_payload - ext->sz_read_payload);
        if (n > 0) {
            ext->sz_read_payload += n;
            if (ext->sz_read_payload == ext->header.sz_payload) {
                ext->sz_read_payload = 0;
               if (ext->header.fragmented == 0) {
                   return 1;
               }
            }
        }
        else if (n < 0) {
            PC_ERROR("Failed to read payload from Unix socket: %s\n",
                    strerror(errno));
            ext->status = US_ERR | US_CLOSE;
            return -1;
        }
        else {
            ext->status |= US_READING;
            /* no data */
        }
        break;

    case US_OPCODE_CONTINUATION:
    case US_OPCODE_END:
        if (ext->message == NULL ||
                (ext->sz_read_message + ext->header.sz_payload) > ext->sz_message) {
            ext->status = US_ERR;
            return -1;
        }

        assert(ext->header.sz_payload > ext->sz_read_payload);

        n = us_read_socket(stream,
                ext->message + ext->sz_read_message + ext->sz_read_payload,
                ext->header.sz_payload - ext->sz_read_payload);
        if (n > 0) {
            ext->sz_read_payload += n;
            if (ext->sz_read_payload == ext->header.sz_payload) {
                ext->sz_read_message += ext->header.sz_payload;
                ext->sz_read_payload = 0;

                if (ext->header.op == US_OPCODE_END) {
                    assert(ext->sz_read_message <= ext->sz_message);

                    if (ext->sz_read_message != ext->sz_message) {
                        PC_WARN("Not all data read\n");
                        ext->sz_message = ext->sz_read_message;
                    }
                    return 1;
                }
            }
        }
        else if (n < 0) {
            PC_ERROR("Failed to read payload from Unix socket: %s\n",
                    strerror(errno));
            ext->status = US_ERR | US_CLOSE;
            return -1;
        }
        else {
            ext->status = US_READING;
            /* no data */
        }
        break;

    default:
        ext->status = US_ERR;
        return -1;
    }

    return 0;
}

static int us_handle_reads(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext0.data;
    int retv;

    /* if it is not waiting for payload, read a frame header */
    if (ext->status & US_WATING_FOR_HEADER) {
        retv = try_to_read_header(stream);
        if (retv <= 0) {
            goto failed;
        }

        switch (ext->header.op) {
        case US_OPCODE_PING:
            retv = stream->ext0.msg_ops->on_message(stream, MT_PING, NULL, 0);
            break;

        case US_OPCODE_CLOSE:
            retv = stream->ext0.msg_ops->on_message(stream, MT_CLOSE, NULL, 0);
            ext->status = US_CLOSE;
            break;

        case US_OPCODE_TEXT:
        case US_OPCODE_BIN: {
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
                ext->status = US_ERR | US_CLOSE;
                goto failed;
            }

            /* always reserve a space for null character */
            ext->message = malloc(ext->sz_message + 1);
            if (ext->message == NULL) {
                PC_ERROR("Failed to allocate memory for packet (size: %u)\n",
                        (unsigned)ext->sz_message);
                ext->status = US_ERR | US_CLOSE;
                goto failed;
            }

            us_update_mem_stats(ext);
            ext->status &= ~US_WATING_FOR_HEADER;
            ext->status |=  US_WATING_FOR_PAYLOAD;
            break;
        }

        case US_OPCODE_CONTINUATION:
        case US_OPCODE_END:
            if (ext->header.sz_payload == 0) {
                ext->status = US_ERR;
                goto failed;
            }

        case US_OPCODE_PONG:
            retv = stream->ext0.msg_ops->on_message(stream, MT_PONG, NULL, 0);
            break;

        default:
            PC_ERROR("Unknown frame opcode: %d\n", ext->header.op);
            ext->status = US_ERR;
            goto failed;
        }
    }
    else if (ext->status & US_WATING_FOR_PAYLOAD) {

        retv = try_to_read_payload(stream);
        if (retv > 0) {
            ext->status &= ~US_WATING_FOR_PAYLOAD;
            ext->status |= US_WATING_FOR_HEADER;

            if (ext->msg_type == MT_TEXT) {
                ext->message[ext->sz_message] = 0;
                ext->sz_message++;
            }

            retv = stream->ext0.msg_ops->on_message(stream, ext->msg_type,
                    ext->message, ext->sz_message);
            free(ext->message);
            ext->message = NULL;
            ext->sz_message = 0;
            ext->sz_read_payload = 0;
            ext->sz_read_message = 0;
            us_update_mem_stats(ext);
        }
        else if (retv == 0) {
            // do nothing
        }
        else if (retv < 0) {
            goto failed;
        }
    }

    return 0;

failed:
    stream->ext0.msg_ops->on_error(stream, 0);

    if (ext->status & US_CLOSE) {
        // TODO: us_cleanup_client(stream);
    }

    return -1;
}

/*
 * Handle write.
 * 0: ok;
 * <0: socket closed
*/
static int us_handle_writes(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext0.data;

    us_write_pending(stream);
    if (list_empty(&ext->pending)) {
        ext->status &= ~US_SENDING;
    }

    if (ext->status & US_ERR) {
        stream->ext0.msg_ops->on_error(stream, 0);
    }

    if ((ext->status & US_CLOSE)) {
        // TODO: us_cleanup_client(stream);
        return -1;
    }

    return 0;
}

/*
 * Send a PING message to the server.
 *
 * return zero on success; none-zero on error.
 */
static int us_ping_server(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext0.data;
    us_frame_header header;

    header.op = US_OPCODE_PING;
    header.fragmented = 0;
    header.sz_payload = 0;
    us_write_sock(stream, &header, sizeof(header));

    if (ext->status & US_ERR)
        return -1;
    return 0;
}

/*
 * Send a PONG message to the server.
 *
 * return zero on success; none-zero on error.
 */
static int us_pong_server(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext0.data;
    us_frame_header header;

    header.op = US_OPCODE_PONG;
    header.fragmented = 0;
    header.sz_payload = 0;
    us_write_sock(stream, &header, sizeof(header));

    if (ext->status & US_ERR)
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

    if (ext->status & US_ERR)
        return -1;
    return 0;
}

/*
 * Send a message
 *
 * return zero on success; none-zero on error.
 */
static int us_send_data(struct pcdvobjs_stream *stream,
        bool text_or_binary, const char *data, size_t sz)
{
    struct stream_extended_data *ext = stream->ext0.data;
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

    if (ext->status & US_ERR) {
        PC_ERROR("Error when sending data: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

static int on_message(struct pcdvobjs_stream *stream,
        int type, const char *buf, size_t len)
{
    int retv = 0;
    (void)buf;
    (void)len;

    switch (type) {
        case MT_TEXT:
            // TODO: fire a `message` event
            break;

        case MT_BINARY:
            // TODO: fire a `message` event
            break;

        case MT_PING:
            retv = us_pong_server(stream);
            break;

        case MT_PONG:
            break;

        case MT_CLOSE:
            // TODO: release resource and fire a `close` event
            // TODO: retv = us_cleanup(stream);
            break;
    }

    return retv;
}


static int read_message(struct pcdvobjs_stream *stream,
        char **buf, size_t *len, int *type)
{
    (void)stream;
    (void)buf;
    (void)len;
    (void)type;

    return 0;
}

static int send_text(struct pcdvobjs_stream *stream,
            const char *text, size_t len)
{
    (void)stream;
    (void)text;
    (void)len;

    return 0;
}

static int send_binary(struct pcdvobjs_stream *stream,
            const void *data, size_t len)
{
    (void)stream;
    (void)data;
    (void)len;

    return 0;
}

static purc_nvariant_method
property_getter(void *entity, const char *name)
{
    struct pcdvobjs_stream *stream = entity;
    purc_nvariant_method method = NULL;

    if (name == NULL) {
        goto failed;
    }

    if (strcmp(name, "send") == 0) {
        // method = send_getter;
    }

    if (method == NULL && stream->ext0.super_ops->property_getter) {
        return stream->ext0.super_ops->property_getter(entity, name);
    }

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
    struct stream_extended_data *ext = stream->ext0.data;
    const struct purc_native_ops *super_ops = stream->ext0.super_ops;

    us_clear_pending_data(ext);
    free(stream->ext0.msg_ops);
    free(stream->ext0.data);

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
        goto failed;
    }

    ext = calloc(1, sizeof(*ext));
    if (ext == NULL) {
        goto failed;
    }

    list_head_init(&ext->pending);
    ext->sz_header = sizeof(ext->header);

    strcpy(stream->ext0.signature, STREAM_EXT_SIG_MSG);

    msg_ops = calloc(1, sizeof(*msg_ops));

    if (msg_ops) {
        msg_ops->on_message = on_message;
        msg_ops->read_message = read_message;
        msg_ops->send_text = send_text;
        msg_ops->send_binary = send_binary;

        stream->ext0.data = ext;
        stream->ext0.super_ops = super_ops;
        stream->ext0.msg_ops = msg_ops;
    }
    else {
        goto failed;
    }

    return &msg_entity_ops;

failed:
    if (msg_ops)
        free(msg_ops);
    if (ext)
        free(ext);

    return NULL;
}

