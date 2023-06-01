/**
 * @file stream.h
 * @author Vincent Wei
 * @date 2023/05/28
 * @brief The internal interfaces for dvobjs/stream
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
 *
 */

#ifndef PURC_DVOBJS_STREAM_H
#define PURC_DVOBJS_STREAM_H

#include "purc-macros.h"
#include "purc-variant.h"
#include "purc-rwstream.h"

#include "private/debug.h"
#include "private/errors.h"

enum pcdvobjs_stream_type {
    STREAM_TYPE_FILE_STDIN,
    STREAM_TYPE_FILE_STDOUT,
    STREAM_TYPE_FILE_STDERR,
    STREAM_TYPE_FILE,
    STREAM_TYPE_PIPE,
    STREAM_TYPE_FIFO,
    STREAM_TYPE_UNIX,
    STREAM_TYPE_TCP,
    STREAM_TYPE_UDP,
};

struct pcdvobjs_stream;
struct stream_extended_data;

enum stream_message_type {
    MT_UNKNOWN = 0,
    MT_TEXT,
    MT_BINARY,
    MT_PING,
    MT_PONG,
    MT_CLOSE
};

struct stream_messaging_ops {
    int (*send_data)(struct pcdvobjs_stream *stream,
            bool text_or_bin, const char *text, size_t len);
    int (*on_error)(struct pcdvobjs_stream *stream, int errcode);
    void (*mark_closing)(struct pcdvobjs_stream *stream);

    /* the following operations can be overridden by extended layer */
    int (*on_message)(struct pcdvobjs_stream *stream, int type,
            const char *buf, size_t len);
    void (*cleanup)(struct pcdvobjs_stream *stream);
};

#define NATIVE_ENTITY_NAME_STREAM       "stream"
#define STREAM_EXT_SIG_MSG              "MSG"
#define STREAM_EXT_SIG_HBS              "HBS"

struct stream_extended {
    char signature[4];

    struct stream_extended_data    *data;
    const struct purc_native_ops   *super_ops;
    union {
        struct stream_messaging_ops    *msg_ops;
        struct stream_hbdbus_ops       *bus_ops;
    };
};

typedef struct pcdvobjs_stream {
    enum pcdvobjs_stream_type type;
    struct purc_broken_down_url *url;
    purc_rwstream_t stm4r;      /* stream for read */
    purc_rwstream_t stm4w;      /* stream for write */
    purc_variant_t observed;    /* not inc ref */
    uintptr_t monitor4r, monitor4w;
    int fd4r, fd4w;

    pid_t cpid;                 /* only for pipe, the pid of child */
    purc_atom_t cid;

    struct stream_extended ext0;   /* for presentation layer */
    struct stream_extended ext1;   /* for application layer */
} pcdvobjs_stream;

PCA_EXTERN_C_BEGIN

const struct purc_native_ops *
dvobjs_extend_stream_by_message(struct pcdvobjs_stream *stream,
        const struct purc_native_ops *super_ops, purc_variant_t extra_opts)
    WTF_INTERNAL;

const struct purc_native_ops *
dvobjs_extend_stream_by_hbdbus(struct pcdvobjs_stream *stream,
        const struct purc_native_ops *super_ops, purc_variant_t extra_opts)
    WTF_INTERNAL;

PCA_EXTERN_C_END

#endif  /* PURC_DVOBJS_STREAM_H */

