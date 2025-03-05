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
#undef NDEBUG /* TODO: Remove this before merging to main branch. */

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

#define MIN_FRAME_PAYLOAD_SIZE      (1024 * 1)
#define DEF_FRAME_PAYLOAD_SIZE      (1024 * 4)
#define MIN_INMEM_MESSAGE_SIZE      (1024 * 8)
#define DEF_INMEM_MESSAGE_SIZE      (1024 * 64)
#define MIN_NO_RESPONSE_TIME_TO_PING        3
#define DEF_NO_RESPONSE_TIME_TO_PING        30
#define MIN_NO_RESPONSE_TIME_TO_CLOSE       6
#define DEF_NO_RESPONSE_TIME_TO_CLOSE       90

/* 512 KiB throttle threshold per stream */
#define SOCK_THROTTLE_THLD          (1024 * 512)

#define MIN_PING_TIMER_INTERVAL             (1 * 1000)      // 1 seconds

enum {
    K_EVENT_TYPE_MIN = 0,

#define EVENT_TYPE_MESSAGE                  "message"
    K_EVENT_TYPE_MESSAGE = K_EVENT_TYPE_MIN,
#define EVENT_TYPE_ERROR                    "error"
    K_EVENT_TYPE_ERROR,
#define EVENT_TYPE_CLOSE                    "close"
    K_EVENT_TYPE_CLOSE,

    K_EVENT_TYPE_MAX = K_EVENT_TYPE_CLOSE,
};

#define NR_EVENT_TYPES          (K_EVENT_TYPE_MAX - K_EVENT_TYPE_MIN + 1)

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

enum us_error_code {
    US_ERR_OOM      = 0x00000001,
    US_ERR_IO       = 0x00000002,
    US_ERR_MSG      = 0x00000003,
    US_ERR_LTNR     = 0x00000004,   /* Long time no response */
};

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
    pcintr_timer_t      ping_timer;

    /* configuration options */
    size_t              maxframepayloadsize;// The maximum frame payload size.
    size_t              maxmessagesize;     // The maximum message size.
    uint32_t            noresptimetoping;   // The maximum no response seconds
                                            // to send a PING message.
    uint32_t            noresptimetoclose;  // The maximum no response seconds
                                            // to close the socket.
    size_t              sz_used_mem;
    size_t              sz_peak_used_mem;

    purc_atom_t         event_cids[NR_EVENT_TYPES];

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
    enum us_error_code code = (enum us_error_code)(ext->status & US_ERR_ANY);
    switch (code) {
        case US_ERR_OOM:
            return PURC_ERROR_OUT_OF_MEMORY;
        case US_ERR_IO:
            return PURC_ERROR_IO_FAILURE;
        case US_ERR_MSG:
            return PURC_ERROR_PROTOCOL_VIOLATION;
        case US_ERR_LTNR:
            return PURC_ERROR_TIMEOUT;
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
        if (ext->ping_timer) {
            pcintr_timer_stop(ext->ping_timer);
            pcintr_timer_destroy(ext->ping_timer);
            ext->ping_timer = NULL;
        }

#if 0
        pcintr_coroutine_post_event(stream->cid,
                PCRDR_MSG_EVENT_REDUCE_OPT_KEEP, stream->observed,
                EVENT_TYPE_CLOSE, NULL,
                PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
#endif

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

static void us_handle_rwerr_close(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext0.data;

    if (ext->status & US_ERR_ANY) {
        stream->ext0.msg_ops->on_error(stream, us_status_to_pcerr(ext));
    }

    if (ext->status & US_CLOSING) {
        cleanup_extension(stream);
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
    else if (bytes == 0) {
        /* no data, peer has been closed */
        bytes = -1;
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
        PC_DEBUG("Got %zd bytes from Unix socket\n", n);
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

            PC_DEBUG("Read payload: %u/%u; message (%u/%u)\n",
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
us_handle_reads(int fd, int event, void *ctxt)
{
    (void)fd;
    (void)event;
    struct pcdvobjs_stream *stream = ctxt;
    struct stream_extended_data *ext = stream->ext0.data;
    int retv;

    clock_gettime(CLOCK_MONOTONIC, &ext->last_live_ts);

    do {
        if (ext->status & US_CLOSING) {
            goto failed;
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

            PC_DEBUG("Got a frame header: %d\n", ext->header.op);
            switch (ext->header.op) {
            case US_OPCODE_PING:
                ext->msg_type = MT_PING;
                retv = stream->ext0.msg_ops->on_message(stream, MT_PING,
                        NULL, 0, NULL);
                break;

            case US_OPCODE_CLOSE:
                ext->msg_type = MT_CLOSE;
                retv = stream->ext0.msg_ops->on_message(stream, MT_CLOSE,
                        NULL, 0, NULL);
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

                if (ext->sz_message > ext->maxmessagesize ||
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
                retv = stream->ext0.msg_ops->on_message(stream, MT_PONG,
                        NULL, 0, NULL);
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
            PC_DEBUG("Got a new payload: %d\n", retv);
            if (retv == READ_WHOLE) {
                ext->status &= ~US_WAITING4PAYLOAD;

                if (ext->sz_read_message == ext->sz_message) {
                    if (ext->msg_type == MT_TEXT) {
                        ext->message[ext->sz_message] = 0;
                        ext->sz_message++;
                        PC_DEBUG("Got a text payload: %s\n", ext->message);
                    }

                    int owner_taken = 0;
                    retv = stream->ext0.msg_ops->on_message(stream,
                            ext->msg_type, ext->message, ext->sz_message,
                            &owner_taken);
                    if (!owner_taken)
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
    us_handle_rwerr_close(stream);
    return false;
}

static bool
us_handle_writes(int fd, int event, void *ctxt)
{
    (void)fd;
    (void)event;
    struct pcdvobjs_stream *stream = ctxt;
    struct stream_extended_data *ext = stream->ext0.data;

    if (ext->status & US_CLOSING) {
        us_handle_rwerr_close(stream);
        return false;
    }

    us_write_pending(stream);
    if (list_empty(&ext->pending)) {
        ext->status &= ~US_SENDING;
    }

    if (ext->status & US_ERR_ANY) {
        us_handle_rwerr_close(stream);
    }

    return true;
}

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

static void shut_off(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext0.data;
    if (ext->sz_pending == 0) {
        us_notify_to_close(stream);
    }

    ext->status = US_CLOSING;
}

static int us_can_send_data(struct stream_extended_data *ext, size_t sz)
{
    if (sz > ext->maxframepayloadsize) {
        size_t frames = sz / ext->maxframepayloadsize + 1;
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

static void on_ping_timer(pcintr_timer_t timer, const char *id, void *data)
{
    (void)id;
    (void)timer;

    struct pcdvobjs_stream *stream = data;
    struct stream_extended_data *ext = stream->ext0.data;

    assert(timer == ext->ping_timer);

    double elapsed = purc_get_elapsed_seconds(&ext->last_live_ts, NULL);
    PC_DEBUG("ping timer elapsed: %f\n", elapsed);

    if (elapsed > ext->noresptimetoclose) {
        us_notify_to_close(stream);
        ext->status = US_ERR_LTNR | US_CLOSING;
        us_handle_rwerr_close(stream);
    }
    else if (elapsed > ext->noresptimetoping) {
        us_ping_peer(stream);
    }
}

static void us_start_ping_timer(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext0.data;

    assert(ext->ping_timer == NULL);

    clock_gettime(CLOCK_MONOTONIC, &ext->last_live_ts);

    purc_runloop_t runloop = purc_runloop_get_current();
    if (runloop) {
        ext->ping_timer = pcintr_timer_create(runloop, NULL,
            on_ping_timer, stream);
        if (ext->ping_timer == NULL) {
            PC_WARN("Failed to create PING timer\n");
        }
        else {
            uint32_t interval = ext->noresptimetoping / 3;
            if (interval < MIN_PING_TIMER_INTERVAL)
                interval = MIN_PING_TIMER_INTERVAL;
            pcintr_timer_set_interval(ext->ping_timer, interval);
            pcintr_timer_start(ext->ping_timer);
        }
    }
}

/*
 * Send a message
 *
 * return zero on success; none-zero on error.
 */
static int send_message(struct pcdvobjs_stream *stream,
        bool text_or_binary, const char *data, size_t sz)
{
    struct stream_extended_data *ext = stream->ext0.data;

    if (ext == NULL) {
        return PURC_ERROR_ENTITY_GONE;
    }

    if (sz > ext->maxmessagesize) {
        return PURC_ERROR_TOO_LARGE_ENTITY;
    }

    if ((ext->status & US_THROTTLING) || us_can_send_data(ext, sz)) {
        return PURC_ERROR_AGAIN;
    }

    ext->status = US_OK;

    us_frame_header header;

    if (sz > ext->maxframepayloadsize) {
        const char* buff = data;
        unsigned int left = sz;

        do {
            if (left == sz) {
                header.op = text_or_binary ? US_OPCODE_TEXT : US_OPCODE_BIN;
                header.fragmented = sz;
                header.sz_payload = ext->maxframepayloadsize;
                left -= ext->maxframepayloadsize;
            }
            else if (left > ext->maxframepayloadsize) {
                header.op = US_OPCODE_CONTINUATION;
                header.fragmented = 0;
                header.sz_payload = ext->maxframepayloadsize;
                left -= ext->maxframepayloadsize;
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
    struct stream_extended_data *ext = stream->ext0.data;

    purc_atom_t target = ext->event_cids[K_EVENT_TYPE_ERROR];
    if (target == 0)
        goto done;

    purc_variant_t data = purc_variant_make_object_0();
    if (data) {
        purc_variant_t tmp;

        tmp = purc_variant_make_number(errcode);
        if (tmp) {
            purc_variant_object_set_by_static_ckey(data, "code",
                    tmp);
            purc_variant_unref(tmp);
        }

        tmp = purc_variant_make_string_static(
            purc_get_error_message(errcode), false);
        if (tmp) {
            purc_variant_object_set_by_static_ckey(data, "postscript",
                    tmp);
            purc_variant_unref(tmp);
        }

        pcintr_coroutine_post_event(stream->cid,
                PCRDR_MSG_EVENT_REDUCE_OPT_KEEP, stream->observed,
                EVENT_TYPE_ERROR, NULL,
                data, PURC_VARIANT_INVALID);
    }

done:
    return 0;
}

static int on_message(struct pcdvobjs_stream *stream, int type,
        char *buf, size_t len, int *owner_taken)
{
    int retv = 0;
    struct stream_extended_data *ext = stream->ext0.data;
    purc_atom_t target = ext->event_cids[K_EVENT_TYPE_MESSAGE];
    purc_variant_t data = PURC_VARIANT_INVALID;
    const char *event = NULL;

    switch (type) {
        case MT_TEXT:
            // fire a `message` event
            data = purc_variant_make_string_reuse_buff(buf, len, true);
            event = EVENT_TYPE_MESSAGE;
            *owner_taken = 1;
            break;

        case MT_BINARY:
            // fire a `message` event
            data = purc_variant_make_byte_sequence_reuse_buff(buf, len, len);
            event = EVENT_TYPE_MESSAGE;
            *owner_taken = 1;
            break;

        case MT_PING:
            retv = us_pong_peer(stream);
            break;

        case MT_PONG:
            break;

        case MT_CLOSE:
            target = ext->event_cids[K_EVENT_TYPE_CLOSE];
            data = purc_variant_make_string_static("Bye", false);
            event = EVENT_TYPE_CLOSE;
            break;
    }

    if (event) {
        if (target)
            pcintr_coroutine_post_event(target,
                    PCRDR_MSG_EVENT_REDUCE_OPT_KEEP, stream->observed,
                    event, NULL,
                    data, PURC_VARIANT_INVALID);
        if (data)
            purc_variant_unref(data);
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
    if ((retv = send_message(stream, text_or_binary, data, len))) {
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
    struct pcdvobjs_stream *stream = entity;
    purc_nvariant_method method = NULL;

    assert(name);

    if (strcmp(name, "send") == 0) {
        method = send_getter;
    }
    else if (strcmp(name, "close") == 0) {
        method = close_getter;
    }
    else {
        struct purc_native_ops *super_ops = stream->ext0.super_ops;
        if (super_ops->property_getter)
            method = super_ops->property_getter(entity, name);

        if (method == NULL)
            goto failed;
    }

    /* override the getters of parent */
    return method;

failed:
    purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    return NULL;
}

static const char *message_events[] = {
#define EVENT_MASK_MESSAGE      (0x01 << K_EVENT_TYPE_MESSAGE)
    EVENT_TYPE_MESSAGE,
#define EVENT_MASK_ERROR        (0x01 << K_EVENT_TYPE_ERROR)
    EVENT_TYPE_ERROR,
#define EVENT_MASK_CLOSE        (0x01 << K_EVENT_TYPE_CLOSE)
    EVENT_TYPE_CLOSE,
};

static bool on_observe(void *entity, const char *event_name,
        const char *event_subname)
{
    struct pcdvobjs_stream *stream = (struct pcdvobjs_stream*)entity;
    pcintr_coroutine_t co = pcintr_get_coroutine();
    if (co == NULL)
        return false;

    int matched = pcdvobjs_match_events(event_name, event_subname,
            message_events, PCA_TABLESIZE(message_events));
    if (matched == -1)
        return false;

    struct stream_extended_data *ext = stream->ext0.data;
    if ((matched & EVENT_MASK_MESSAGE)) {
        ext->event_cids[K_EVENT_TYPE_MESSAGE] = co->cid;
    }
    if ((matched & EVENT_MASK_ERROR)) {
        ext->event_cids[K_EVENT_TYPE_ERROR] = co->cid;
    }
    if ((matched & EVENT_MASK_CLOSE)) {
        ext->event_cids[K_EVENT_TYPE_CLOSE] = co->cid;
    }

    return true;
}

static bool on_forget(void *entity, const char *event_name,
        const char *event_subname)
{
    struct pcdvobjs_stream *stream = (struct pcdvobjs_stream*)entity;
    assert(stream);

    pcintr_coroutine_t co = pcintr_get_coroutine();
    if (co == NULL)
        return false;

    int matched = pcdvobjs_match_events(event_name, event_subname,
            message_events, PCA_TABLESIZE(message_events));
    if (matched == -1)
        return false;

    struct stream_extended_data *ext = stream->ext0.data;
    if (ext) {
        if ((matched & EVENT_MASK_MESSAGE)) {
            ext->event_cids[K_EVENT_TYPE_MESSAGE] = 0;
        }
        if ((matched & EVENT_MASK_ERROR)) {
            ext->event_cids[K_EVENT_TYPE_ERROR] = 0;
        }
        if ((matched & EVENT_MASK_CLOSE)) {
            ext->event_cids[K_EVENT_TYPE_CLOSE] = 0;
        }
    }

    return true;
}

static void on_release(void *entity)
{
    struct pcdvobjs_stream *stream = entity;
    struct purc_native_ops *super_ops = stream->ext0.super_ops;

    cleanup_extension(stream);

    if (super_ops->on_release) {
        return super_ops->on_release(entity);
    }
}

static struct purc_native_ops msg_entity_ops = {
    .property_getter = property_getter,
    .on_observe = on_observe,
    .on_forget = on_forget,
    .on_release = on_release,
};

struct purc_native_ops *
dvobjs_extend_stream_by_message(struct pcdvobjs_stream *stream,
        struct purc_native_ops *super_ops, purc_variant_t extra_opts)
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

    purc_variant_t tmp;

    tmp = purc_variant_object_get_by_ckey(extra_opts, "maxframepayloadsize");
    uint64_t maxframepayloadsize = 0;
    if (tmp && !purc_variant_cast_to_ulongint(tmp, &maxframepayloadsize, false)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    tmp = purc_variant_object_get_by_ckey(extra_opts, "maxmessagesize");
    uint64_t maxmessagesize = 0;
    if (tmp && !purc_variant_cast_to_ulongint(tmp, &maxmessagesize, false)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    tmp = purc_variant_object_get_by_ckey(extra_opts, "noresptimetoping");
    uint32_t noresptimetoping = 0;
    if (tmp && !purc_variant_cast_to_uint32(tmp, &noresptimetoping, false)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    tmp = purc_variant_object_get_by_ckey(extra_opts, "noresptimetoclose");
    uint32_t noresptimetoclose = 0;
    if (tmp && !purc_variant_cast_to_uint32(tmp, &noresptimetoclose, false)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    /* Override the socket option to be have O_NONBLOCK */
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

    if (maxframepayloadsize == 0)
        ext->maxframepayloadsize = DEF_FRAME_PAYLOAD_SIZE;
    else if (maxframepayloadsize < MIN_FRAME_PAYLOAD_SIZE)
        ext->maxframepayloadsize = MIN_FRAME_PAYLOAD_SIZE;
    else
        ext->maxframepayloadsize = maxframepayloadsize;

    if (maxmessagesize == 0)
        ext->maxmessagesize = DEF_INMEM_MESSAGE_SIZE;
    else if (maxmessagesize < MIN_INMEM_MESSAGE_SIZE)
        ext->maxmessagesize = MIN_INMEM_MESSAGE_SIZE;
    else
        ext->maxmessagesize = maxmessagesize;

    if (noresptimetoping == 0)
        ext->noresptimetoping = DEF_NO_RESPONSE_TIME_TO_PING;
    else if (noresptimetoping < MIN_NO_RESPONSE_TIME_TO_PING)
        ext->noresptimetoping = MIN_NO_RESPONSE_TIME_TO_PING;
    else
        ext->noresptimetoping = noresptimetoping;

    if (noresptimetoclose == 0)
        ext->noresptimetoclose = DEF_NO_RESPONSE_TIME_TO_CLOSE;
    else if (noresptimetoclose < MIN_NO_RESPONSE_TIME_TO_CLOSE)
        ext->noresptimetoclose = MIN_NO_RESPONSE_TIME_TO_CLOSE;
    else
        ext->noresptimetoclose = noresptimetoclose;

    PC_DEBUG("Configuration: maxframepayloadsize(%zu/%zu), "
            "maxmessagesize(%zu/%zu), noresptimetoping(%u/%u), "
            "noresptimetoclose(%u/%u)\n",
            ext->maxframepayloadsize, (size_t)maxframepayloadsize,
            ext->maxmessagesize, (size_t)maxmessagesize,
            ext->noresptimetoping, noresptimetoping,
            ext->noresptimetoclose, noresptimetoclose);

    list_head_init(&ext->pending);
    ext->sz_header = sizeof(ext->header);

    strcpy(stream->ext0.signature, STREAM_EXT_SIG_MSG);

    msg_ops = calloc(1, sizeof(*msg_ops));

    if (msg_ops) {
        msg_ops->send_message = send_message;
        msg_ops->on_error = on_error;
        msg_ops->shut_off = shut_off;

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

    pcintr_coroutine_t co = pcintr_get_coroutine();
    if (co) {
        stream->monitor4r = purc_runloop_add_fd_monitor(
                purc_runloop_get_current(), stream->fd4r, PCRUNLOOP_IO_IN,
                us_handle_reads, stream);
        if (stream->monitor4r) {
            stream->cid = co->cid;
        }
        else {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }

        stream->monitor4w = purc_runloop_add_fd_monitor(
                purc_runloop_get_current(), stream->fd4w, PCRUNLOOP_IO_OUT,
                us_handle_writes, stream);
        if (stream->monitor4w) {
            stream->cid = co->cid;
        }
        else {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }
    }
    else {
        stream->ext0.msg_ops->on_readable = us_handle_reads;
        stream->ext0.msg_ops->on_writable = us_handle_writes;
        stream->ext0.msg_ops->on_ping_timer = on_ping_timer;
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

    us_start_ping_timer(stream);

    PC_DEBUG("This socket is extended by Layer 0 protocol: message\n");
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

