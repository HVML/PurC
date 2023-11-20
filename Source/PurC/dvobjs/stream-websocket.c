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
#include <netdb.h>

#if defined(__linux__) || defined(__CYGWIN__)
#  include <endian.h>
#if ((__GLIBC__ == 2) && (__GLIBC_MINOR__ < 9))
#if defined(__BYTE_ORDER) && (__BYTE_ORDER == __LITTLE_ENDIAN)
#  include <arpa/inet.h>
#  define htobe16(x) htons(x)
#  define htobe64(x) (((uint64_t)htonl(((uint32_t)(((uint64_t)(x)) >> 32)))) | \
   (((uint64_t)htonl(((uint32_t)(x)))) << 32))
#  define be16toh(x) ntohs(x)
#  define be32toh(x) ntohl(x)
#  define be64toh(x) (((uint64_t)ntohl(((uint32_t)(((uint64_t)(x)) >> 32)))) | \
   (((uint64_t)ntohl(((uint32_t)(x)))) << 32))
#else
#  error Byte Order not supported!
#endif
#endif
#elif defined(__sun__)
#  include <sys/byteorder.h>
#  define htobe16(x) BE_16(x)
#  define htobe64(x) BE_64(x)
#  define be16toh(x) BE_IN16(x)
#  define be32toh(x) BE_IN32(x)
#  define be64toh(x) BE_IN64(x)
#elif defined(__FreeBSD__) || defined(__NetBSD__)
#  include <sys/endian.h>
#elif defined(__OpenBSD__)
#  include <sys/types.h>
#  if !defined(be16toh)
#    define be16toh(x) betoh16(x)
#  endif
#  if !defined(be32toh)
#    define be32toh(x) betoh32(x)
#  endif
#  if !defined(be64toh)
#    define be64toh(x) betoh64(x)
#  endif
#elif defined(__APPLE__)
#  include <libkern/OSByteOrder.h>
#  define htobe16(x) OSSwapHostToBigInt16(x)
#  define htobe64(x) OSSwapHostToBigInt64(x)
#  define be16toh(x) OSSwapBigToHostInt16(x)
#  define be32toh(x) OSSwapBigToHostInt32(x)
#  define be64toh(x) OSSwapBigToHostInt64(x)
#else
#  error Platform not supported!
#endif

#define WS_MAGIC_STR        "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define WS_KEY_LEN          16
#define SHA_DIGEST_LEN      20

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
    uint64_t sz_ext_payload;
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
    char                header_buf[2];
    size_t              sz_header;
    size_t              sz_read_header;

    char                ext_payload_buf[9];
    size_t              sz_ext_payload;
    size_t              sz_read_ext_payload;

    char                mask[4];
    size_t              sz_mask;
    size_t              sz_read_mask;
    bool                read_mask_done;

    /* fields for current reading message */
    size_t              sz_message;         /* total size of current message */
    size_t              sz_read_message;    /* read size of current message */
    char               *message;            /* message data */

    size_t              sz_payload;         /* total size of current payload */
    size_t              sz_read_payload;    /* read size of current payload */
    char               *payload;            /* payload data */
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

static ssize_t ws_write(int fd, const void *buf, size_t length)
{
    /* TODO : ssl support */
    return write(fd, buf, length);
}

static ssize_t ws_read(int fd, void *buf, size_t length)
{
    /* TODO : ssl support */
    return read(fd, buf, length);
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

    bytes = ws_write(stream->fd4w, buffer, len);
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

        bytes = ws_write(stream->fd4w, pending->data + pending->szsent,
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
    bytes = ws_read(stream->fd4r, buff, sz);
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

static int ws_send_data_frame(struct pcdvobjs_stream *stream, int fin, int opcode,
        const void *data, ssize_t sz)
{
    int ret = PCRDR_ERROR_IO;
    int mask_int = 0;
    size_t size = sz;
    char *buf = NULL;
    char *p = NULL;
    size_t nr_buf = 0;
    ws_frame_header header;
    unsigned char mask[4] = { 0 };

    if (sz <= 0) {
        PC_DEBUG ("Invalid data size %ld.\n", sz);
        goto out;
    }

    header.fin = fin;
    header.rsv = 0;
    header.op = opcode;
    header.mask = 1; /* client must 1 */

    srand(time(NULL));
    mask_int = rand();
    memcpy(mask, &mask_int, 4);

    size = sz;
    if (size > 0xffff) {
        /* header(16b) + Extended payload length(64b) + mask(32b) + data */
        nr_buf = 2 + 8 + 4 + sz;
        header.sz_payload = 127;
    }
    else if (size > 125) {
        /* header(16b) + Extended payload length(16b) + mask(32b) + data */
        nr_buf = 2 + 2 + 4 + sz;
        header.sz_payload = 126;
    }
    else {
        /* header(16b) + data + mask(32b) */
        nr_buf = 2 + 4 + sz;
        header.sz_payload = sz;
    }

    buf = malloc(nr_buf + 1);
    buf[0] = 0;
    buf[1] = 0;
    if (fin) {
        buf[0] |= 0x80;
    }
    buf[0] |= (0xff & opcode);
    buf[1] = 0x80 | header.sz_payload;

    p = buf + 2;
    if (header.sz_payload == 127) {
        uint64_t v = htobe64(sz);
        memcpy(p, &v, 8);
        p = p + 8;
    }
    else if (header.sz_payload == 126) {
        uint16_t v = htobe16(sz);
        memcpy(p, &v, 2);
        p = p + 2;
    }

    /* mask */
    memcpy(p, &mask, 4);

    /* payload */
    p = p + 4;
    memcpy(p, data, sz);

    /* mask payload */
    for (ssize_t i = 0; i < sz; i++) {
        p[i] ^= mask[i % 4] & 0xff;
    }

    ws_write_sock(stream, buf, nr_buf);
    ret = 0;

out:
    if (buf) {
        free(buf);
    }
    return ret;
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
    ws_frame_header *header = &ext->header;
    char *buf = ext->header_buf;
    ssize_t n;

    assert(ext->sz_header > ext->sz_read_header);

    n = ws_read_socket(stream, buf + ext->sz_read_header,
            ext->sz_header - ext->sz_read_header);
    if (n > 0) {
        ext->sz_read_header += n;
        if (ext->sz_read_header == ext->sz_header) {
            ext->sz_read_header = 0;

            header->fin = buf[0] & 0x80 ? 1 : 0;
            header->rsv = buf[0] & 0x70;
            header->op = buf[0] & 0x0F;
            header->mask = buf[1] & 0x80;
            header->sz_payload = buf[1] & 0x7F;
            ext->read_mask_done = false;

            switch (header->sz_payload) {
            case 127:
                memset(ext->ext_payload_buf, 0, sizeof(ext->ext_payload_buf));
                ext->sz_ext_payload = sizeof(uint64_t);
                ext->sz_read_ext_payload = 0;
                break;
            case 126:
                memset(ext->ext_payload_buf, 0, sizeof(ext->ext_payload_buf));
                ext->sz_ext_payload = sizeof(uint16_t);
                ext->sz_read_ext_payload = 0;
                break;
            default:
                header->sz_ext_payload = header->sz_payload;
                ext->sz_payload = header->sz_ext_payload;
                ext->payload = malloc(ext->sz_payload + 1);
                if (ext->payload == NULL) {
                    PC_ERROR("Failed to allocate memory for packet (size: %u)\n",
                            (unsigned)ext->sz_payload);
                    ext->status = WS_ERR_IO | WS_CLOSING;
                    return READ_ERROR;
                }
                ext->sz_read_payload = 0;
                break;
            }
            return READ_WHOLE;
        }
        ext->status |= WS_READING;
    }
    else if (n < 0) {
        PC_ERROR("Failed to read frame header from WebSocket: %s\n",
                strerror(errno));
        ext->status = WS_ERR_IO | WS_CLOSING;
        return READ_ERROR;
    }
    else {
        /* no data */
        ext->status |= WS_READING;
        return READ_NONE;
    }

    return READ_SOME;
}

static int try_to_read_ext_payload_length(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext0.data;
    ws_frame_header *header = &ext->header;
    ssize_t n;

    char *buf = (char *)ext->ext_payload_buf;
    assert(ext->sz_ext_payload > ext->sz_read_ext_payload);

    n = ws_read_socket(stream, buf + ext->sz_read_ext_payload,
            ext->sz_ext_payload - ext->sz_read_ext_payload);
    if (n > 0) {
        ext->sz_read_ext_payload += n;
        if (ext->sz_read_ext_payload == ext->sz_ext_payload) {
            ext->sz_read_ext_payload = 0;
            if (ext->sz_ext_payload == sizeof(uint16_t)) {
                uint16_t v;
                memcpy(&v, ext->ext_payload_buf, 2);
                header->sz_ext_payload = be16toh(v);
            }
            else if (ext->sz_ext_payload == sizeof(uint64_t)) {
                uint64_t v;
                memcpy(&v, ext->ext_payload_buf, 8);
                header->sz_ext_payload = be64toh(v);
            }
            return READ_WHOLE;
        }
        ext->status |= WS_READING;
    }
    else if (n < 0) {
        PC_ERROR("Failed to read frame ext_payload from WebSocket: %s\n",
                strerror(errno));
        ext->status = WS_ERR_IO | WS_CLOSING;
        return READ_ERROR;
    }
    else {
        /* no data */
        ext->status |= WS_READING;
        return READ_NONE;
    }
    return READ_SOME;
}

static int try_to_read_mask(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext0.data;
    ssize_t n;

    char *buf = (char *)ext->mask;
    assert(ext->sz_mask > ext->sz_read_mask);

    n = ws_read_socket(stream, buf + ext->sz_read_mask,
            ext->sz_mask - ext->sz_read_mask);
    if (n > 0) {
        ext->sz_read_mask += n;
        if (ext->sz_read_mask == ext->sz_mask) {
            ext->sz_read_mask = 0;
            ext->read_mask_done = true;
            return READ_WHOLE;
        }
        ext->status |= WS_READING;
    }
    else if (n < 0) {
        PC_ERROR("Failed to read frame mask from WebSocket: %s\n",
                strerror(errno));
        ext->status = WS_ERR_IO | WS_CLOSING;
        return READ_ERROR;
    }
    else {
        /* no data */
        ext->status |= WS_READING;
        return READ_NONE;
    }
    return READ_SOME;
}

static int try_to_read_payload(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext0.data;
    ssize_t n;

    char *buf = (char *)ext->payload;
    assert(ext->sz_payload > ext->sz_read_payload);

    n = ws_read_socket(stream, buf + ext->sz_read_payload,
            ext->sz_payload - ext->sz_read_payload);
    if (n > 0) {
        ext->sz_read_payload += n;
        if (ext->sz_read_payload == ext->sz_payload) {
            ext->sz_read_payload = 0;
            return READ_WHOLE;
        }
        ext->status |= WS_READING;
    }
    else if (n < 0) {
        PC_ERROR("Failed to read frame payload from WebSocket: %s\n",
                strerror(errno));
        ext->status = WS_ERR_IO | WS_CLOSING;
        return READ_ERROR;
    }
    else {
        /* no data */
        ext->status |= WS_READING;
        return READ_NONE;
    }
    return READ_SOME;
}

/*
 * Tries to read a frame. */
static int try_to_read_frame(struct pcdvobjs_stream *stream)
{
    (void) stream;
    struct stream_extended_data *ext = stream->ext0.data;
    ws_frame_header *header = &ext->header;
    int retv;

    /* read extended payload length */
    if (header->sz_ext_payload == 0) {
        retv = try_to_read_ext_payload_length(stream);
        if (retv != READ_WHOLE) {
            return retv;
        }

        ext->sz_payload = header->sz_ext_payload;
        ext->payload = malloc(ext->sz_payload + 1);
        if (ext->payload == NULL) {
            PC_ERROR("Failed to allocate memory for packet (size: %u)\n",
                    (unsigned)ext->sz_payload);
            ext->status = WS_ERR_IO | WS_CLOSING;
            return READ_ERROR;
        }

        ext->sz_read_payload = 0;
    }

    /* read mask */
    if (header->mask && !ext->read_mask_done) {
        retv = try_to_read_mask(stream);
        if (retv != READ_WHOLE) {
            return retv;
        }
    }

    /* read websocket payload */
    return try_to_read_payload(stream);
}

static bool
ws_handle_reads(int fd, purc_runloop_io_event event, void *ctxt)
{
    (void)fd;
    (void)event;
    struct pcdvobjs_stream *stream = ctxt;
    struct stream_extended_data *ext = stream->ext0.data;
    int retv;

    clock_gettime(CLOCK_MONOTONIC, &ext->last_live_ts);

    do {
        if (ext->status & WS_CLOSING) {
            goto closing;
        }

        if (!(ext->status & WS_WAITING4PAYLOAD)) {
            retv = try_to_read_header(stream);
            if (retv == READ_NONE) {
                break;
            }
            else if (retv == READ_SOME) {
                continue;
            }
            else if (retv == READ_ERROR) {
                ext->status = WS_ERR_IO | WS_CLOSING;
                goto failed;
            }

            switch (ext->header.op) {
            case WS_OPCODE_PING:
                ext->msg_type = MT_PING;
                ext->status |= WS_WAITING4PAYLOAD;
                break;

            case WS_OPCODE_PONG:
                ext->msg_type = MT_PONG;
                ext->status |= WS_WAITING4PAYLOAD;
                break;

            case WS_OPCODE_CLOSE:
                ext->msg_type = MT_CLOSE;
                ext->status |= WS_WAITING4PAYLOAD;
                break;

            case WS_OPCODE_TEXT:
                ext->msg_type = MT_TEXT;
                ext->status |= WS_WAITING4PAYLOAD;
                break;

            case WS_OPCODE_BIN:
                ext->msg_type = MT_BINARY;
                ext->status |= WS_WAITING4PAYLOAD;
                break;

            case WS_OPCODE_CONTINUATION:
                ext->status |= WS_WAITING4PAYLOAD;
                break;

            default:
                PC_ERROR("Unknown frame opcode: %d\n", ext->header.op);
                ext->status = WS_ERR_MSG | WS_CLOSING;
                goto failed;
                break;
            }

            PC_INFO("Got a frame header: %d\n", ext->header.op);
        }
        else if (ext->status & WS_WAITING4PAYLOAD) {
            retv = try_to_read_frame(stream);
            if (retv == READ_NONE) {
                break;
            }
            else if (retv == READ_SOME) {
                continue;
            }
            else if (retv == READ_ERROR) {
                ext->status = WS_ERR_IO | WS_CLOSING;
                goto failed;
            }
            else if (retv == READ_WHOLE) {
                if (!ext->message) {
                    ext->sz_message = ext->sz_payload;
                    ext->message = malloc(ext->sz_message + 1);
                    ext->sz_read_message = 0;
                }
                else {
                    ext->sz_message += ext->sz_payload;
                    ext->message = realloc(ext->message, ext->sz_message + 1);
                }

                if (ext->message == NULL) {
                    PC_ERROR("failed to allocate memory for packet (size: %u)\n",
                            (unsigned)ext->sz_message);
                    ext->status = WS_ERR_IO | WS_CLOSING;
                    goto failed;
                }

                memcpy(ext->message + ext->sz_read_message, ext->payload,
                        ext->sz_payload);
                ext->sz_read_message += ext->sz_payload;
                free(ext->payload);

                ext->payload = NULL;
                ext->sz_payload = 0;
                ext->sz_read_payload = 0;

                if (ext->header.fin == 0) {
                    ext->status &= ~WS_WAITING4PAYLOAD;
                    continue;
                }

                /* whole message */
                switch (ext->header.op) {
                case WS_OPCODE_PING:
                    retv = stream->ext0.msg_ops->on_message(stream, MT_PING, NULL, 0);
                    break;

                case WS_OPCODE_PONG:
                    retv = stream->ext0.msg_ops->on_message(stream, MT_PONG, NULL, 0);
                    break;

                case WS_OPCODE_CLOSE:
                    retv = stream->ext0.msg_ops->on_message(stream, MT_CLOSE, NULL, 0);
                    ext->status = WS_CLOSING;
                    break;

                case WS_OPCODE_TEXT:
                    ext->message[ext->sz_message] = 0;
                    ext->sz_message++;
                    PC_INFO("Got a text payload: %s\n", ext->message);

                    retv = stream->ext0.msg_ops->on_message(stream,
                            ext->msg_type, ext->message, ext->sz_message);
                    free(ext->message);
                    ext->message = NULL;
                    ext->sz_message = 0;
                    ext->sz_read_payload = 0;
                    ext->sz_read_message = 0;
                    ws_update_mem_stats(ext);
                    break;

                case WS_OPCODE_BIN:
                    retv = stream->ext0.msg_ops->on_message(stream,
                            ext->msg_type, ext->message, ext->sz_message);
                    free(ext->message);
                    ext->message = NULL;
                    ext->sz_message = 0;
                    ext->sz_read_payload = 0;
                    ext->sz_read_message = 0;
                    ws_update_mem_stats(ext);
                    break;

                default:
                    /* never reach here */
                    PC_ERROR("Unknown frame opcode: %d\n", ext->header.op);
                    ext->status = WS_ERR_MSG | WS_CLOSING;
                    goto failed;
                    break;
                }
                break;
            }
        }
    } while (true);

    return true;

failed:
    stream->ext0.msg_ops->on_error(stream, ws_status_to_pcerr(ext));

closing:
    if (ext->status & WS_CLOSING) {
        pcintr_coroutine_post_event(stream->cid,
                PCRDR_MSG_EVENT_REDUCE_OPT_OVERLAY, stream->observed,
                EVENT_TYPE_CLOSE, NULL,
                PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
        cleanup_extension(stream);
    }

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

static int ws_send_ctrl_frame(struct pcdvobjs_stream *stream, char code)
{
    char data[6];
    int mask_int;

    srand(time(NULL));
    mask_int = rand();
    memcpy(data + 2, &mask_int, 4);

    data[0] = 0x80 | code;
    data[1] = 0x80;

    if (6 != ws_write_sock(stream, data, 6)) {
        return -1;
    }
    return 0;
}

#if 0
/*
 * Send a PING message to the peer.
 *
 * return zero on success; none-zero on error.
 */
static int ws_ping_peer(struct pcdvobjs_stream *stream)
{
    return ws_send_ctrl_frame(stream, WS_OPCODE_PING);
}
#endif

/*
 * Send a PONG message to the peer.
 *
 * return zero on success; none-zero on error.
 */
static int ws_pong_peer(struct pcdvobjs_stream *stream)
{
    return ws_send_ctrl_frame(stream, WS_OPCODE_PONG);
}

/*
 * Send a CLOSE message to the server
 *
 * return zero on success; none-zero on error.
 */
static int ws_notify_to_close(struct pcdvobjs_stream *stream)
{
    return ws_send_ctrl_frame(stream, WS_OPCODE_CLOSE);
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
    /* TODO */
    (void) stream;
    (void) text_or_binary;
    (void) data;
    (void) sz;
    struct stream_extended_data *ext = stream->ext0.data;

    if (ext == NULL) {
        return PURC_ERROR_ENTITY_GONE;
    }

    if (sz > MAX_INMEM_MESSAGE_SIZE) {
        return PURC_ERROR_TOO_LARGE_ENTITY;
    }

    if ((ext->status & WS_THROTTLING) || ws_can_send_data(ext, sz)) {
        return PURC_ERROR_AGAIN;
    }

    ext->status = WS_OK;

    if (sz > MAX_FRAME_PAYLOAD_SIZE) {
        unsigned int left = sz;
        int fin;
        int opcode;
        size_t sz_payload;

        do {
            if (left == sz) {
                fin = 0;
                opcode = WS_OPCODE_TEXT;
                sz_payload = PCRDR_MAX_FRAME_PAYLOAD_SIZE;
                left -= PCRDR_MAX_FRAME_PAYLOAD_SIZE;
            }
            else if (left > PCRDR_MAX_FRAME_PAYLOAD_SIZE) {
                fin = 0;
                opcode = WS_OPCODE_CONTINUATION;
                sz_payload = PCRDR_MAX_FRAME_PAYLOAD_SIZE;
                left -= PCRDR_MAX_FRAME_PAYLOAD_SIZE;
            }
            else {
                fin = 1;
                opcode = WS_OPCODE_CONTINUATION;
                sz_payload = left;
                left = 0;
            }

            ws_send_data_frame(stream, fin, opcode, data, sz_payload);
            data += sz_payload;
        } while (left > 0);
    }
    else {
        ws_send_data_frame(stream, 1, WS_OPCODE_TEXT, data, sz);
    }

    if (ext->status & WS_ERR_ANY) {
        PC_ERROR("Error when sending data: %s\n", strerror(errno));
        return ws_status_to_pcerr(ext);
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
    ext->sz_header = sizeof(ext->header_buf);
    memset(ext->header_buf, 0, ext->sz_header);

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

static int ws_open_connection(const char *host, const char *port)
{
    int fd = -1;
    struct addrinfo *addrinfo;
    struct addrinfo *p;
    struct addrinfo hints = { 0 };

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (0 != getaddrinfo(host, port, &hints, &addrinfo)) {
        PC_DEBUG ("Error while getting address info (%s:%s)\n",
                host, port);
        goto out;
    }

    for (p = addrinfo; p != NULL; p = p->ai_next) {
        if((fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            continue;
        }

        if (connect(fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(fd);
            continue;
        }
        break;
    }
    freeaddrinfo(addrinfo);

    if (p == NULL) {
        PC_DEBUG ("Connect to websocket server failed! (%s:%s)\n",
                host, port);
        goto out;
    }

out:
    return fd;
}

static void ws_sha1_digest(const char *s, int len, unsigned char *digest)
{
  pcutils_sha1_ctxt sha;

  pcutils_sha1_begin(&sha);
  pcutils_sha1_hash(&sha, (uint8_t *) s, len);
  pcutils_sha1_end(&sha, digest);
}

static int ws_verify_handshake(const char *ws_key, char *header)
{
    (void) header;
    int ret = -1;

    size_t klen = strlen(ws_key);
    size_t mlen = strlen(WS_MAGIC_STR);
    size_t len = klen + mlen;
    char *s = malloc (klen + mlen + 1);
    uint8_t digest[SHA_DIGEST_LEN] = { 0 };

    memset(digest, 0, sizeof *digest);
    memcpy(s, ws_key, klen);
    memcpy(s + klen, WS_MAGIC_STR, mlen + 1);
    ws_sha1_digest(s, len, digest);

    char *encode = pcutils_b64_encode_alloc((unsigned char *)digest,
            sizeof(digest));

    char *tmp = NULL;
    const char *line = header, *next = NULL;
    bool valid_status = false;
    bool valid_accept = false;
    bool valid_upgrade = false;
    bool valid_connection = false;

    while (line) {
        if ((next = strstr (line, "\r\n")) != NULL) {
            len = (next - line);
        }
        else {
            len = strlen (line);
        }

        if (len <= 0) {
            PC_DEBUG ("Bad http header during handshake\n");
            goto out;
        }

        tmp = malloc(len + 1);
        memcpy (tmp, line, len);
        tmp[len] = '\0';

        if(tmp[0] == 'H' && tmp[1] == 'T' && tmp[2] == 'T'
                && tmp[3] == 'P') {
            if(strcmp(tmp, "HTTP/1.1 101 Switching Protocols") != 0 &&
                    strcmp(tmp, "HTTP/1.0 101 Switching Protocols") != 0) {
                PC_DEBUG ("Peer protocol invalid : %s\n", tmp);
                goto out;
            }
            valid_status = true;
        }
        else {
            char *p = strchr(tmp, ' ');
            if (p) {
                *p = '\0';
            }

            if (strcmp(tmp, "Upgrade:") == 0 &&
                    strcasecmp(p + 1, "websocket") == 0) {
                valid_upgrade = true;
            }
            else if (strcmp(tmp, "Connection:") == 0 &&
                    strcasecmp(p + 1, "upgrade") == 0) {
                valid_connection = true;
            }
            else if (strcmp(tmp, "Sec-WebSocket-Accept:") == 0 &&
                    strcmp(p + 1, encode) == 0) {
                    valid_accept = true;
            }
        }

        free (tmp);
        tmp = NULL;

        line = next ? (next + 2) : NULL;
        if (strcmp(next, "\r\n\r\n") == 0) {
            break;
        }
    }

    if (!valid_status) {
        PC_DEBUG ("Bad http status during handshake\n");
        goto out;
    }

    if (!valid_accept) {
        PC_DEBUG ("Verify Sec-WebSocket-Accept failed during handshake\n");
        goto out;
    }

    if (!valid_upgrade) {
        PC_DEBUG ("Not found upgrade header during handshake\n");
        goto out;
    }

    if (!valid_connection) {
        PC_DEBUG ("Not found connection header during handshake\n");
        goto out;
    }

    ret = 0;
out:
    if (tmp) {
        free(tmp);
    }

    if (s) {
        free(s);
    }

    if (encode) {
        free(encode);
    }
    return ret;
}

static int ws_handshake(int fd, const char *host_name, const char *port )
{
    int ret = -1;

    /* generate Sec-WebSocket-Key */
    srand(time(NULL));
    char key[WS_KEY_LEN];
    for (int i = 0; i < WS_KEY_LEN; i++) {
        key[i] = rand() & 0xff;
    }
    char *ws_key =  pcutils_b64_encode_alloc ((unsigned char *) key, WS_KEY_LEN);
    char req_headers[1024] = { 0 };

    snprintf(req_headers, 1024,
            "GET / HTTP/1.1\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Host: %s:%s\r\n"
            "Sec-WebSocket-Key: %s\r\n"
            "Sec-WebSocket-Version: 13\r\n\r\n",
            host_name, port, ws_key);

    /* send to server */
    ws_write(fd, req_headers, strlen(req_headers));

    char buf[1024] = { 0 };

    char *p = buf;
    while (true) {
        if (ws_read(fd, p, 1) != 1) {
            PC_DEBUG ("Error receiving data during handshake\n");
            goto out;
        }
        p++;
        if (p - buf >= 4 && strcmp(p - 4, "\r\n\r\n") == 0) {
            break;
        }
    }

    ret = ws_verify_handshake(ws_key, buf);

out:
    if (ws_key) {
        free(ws_key);
    }
    return ret;
}

int dvobjs_extend_stream_websocket_connect(const char *host_name, int port)
{
    int fd;
    char s_port[10] = {0};
    if (port <=0 || port > 65535) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    sprintf(s_port, "%d", port);

    if ((fd = ws_open_connection(host_name, s_port)) < 0) {
        goto failed;
    }

    if (ws_handshake(fd, host_name, s_port) != 0) {
        goto failed;
    }

    return fd;

failed:
    return -1;
}

