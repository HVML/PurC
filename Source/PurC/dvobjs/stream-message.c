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
    US_CLOSE = (1 << 1),
    US_READING = (1 << 2),
    US_SENDING = (1 << 3),
    US_THROTTLING = (1 << 4),
    US_WATING_FOR_PAYLOAD = (1 << 5),
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
    /* time got the first frame of the current packet */
    struct timespec     ts;

    /* the status of the client */
    us_state            state;

    /* fields for pending data to write */
    size_t              sz_pending;
    struct list_head    pending;

    /* current frame header */
    us_frame_header     header;
    size_t              sz_read_hdr;

    /* fields for current reading payload */
    uint32_t            sz_payload;         /* total size of current payload */
    uint32_t            sz_read_payload;    /* read size of current payload */
    char               *payload;            /* payload data */
};

static int on_message(struct pcdvobjs_stream *stream,
        const char *buf, size_t len, int type)
{
    (void)stream;
    (void)buf;
    (void)len;
    (void)type;

    return 0;
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

static void clear_pending_data(struct stream_extended_data *ext)
{
    struct list_head *p, *n;

    list_for_each_safe(p, n, &ext->pending) {
        list_del(p);
        free(p);
    }

    ext->sz_pending = 0;
}

static void on_release(void *entity)
{
    struct pcdvobjs_stream *stream = entity;
    struct stream_extended_data *ext = stream->ext0.data;
    const struct purc_native_ops *super_ops = stream->ext0.super_ops;

    clear_pending_data(ext);
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

