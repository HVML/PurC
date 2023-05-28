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

struct stream_extended;

typedef struct pcdvobjs_stream {
    enum pcdvobjs_stream_type type;
    struct purc_broken_down_url *url;
    purc_rwstream_t stm4r;      /* stream for read */
    purc_rwstream_t stm4w;      /* stream for write */
    purc_variant_t option;
    purc_variant_t observed;    /* not inc ref */
    uintptr_t monitor4r, monitor4w;
    int fd4r, fd4w;

    pid_t cpid;                 /* only for pipe, the pid of child */
    purc_atom_t cid;

    struct stream_extended *ext;
} pcdvobjs_stream;

enum {
    MSG_DATA_TYPE_UNKNOWN = 0,
    MSG_DATA_TYPE_TEXT,
    MSG_DATA_TYPE_BINARY,
    MSG_DATA_TYPE_PING,
    MSG_DATA_TYPE_PONG,
    MSG_DATA_TYPE_CLOSE,
};

#define SIGNATURE_MSG       "MSG"

struct stream_messaging_ops {
    union {
        char         signature[0];  // always contains "MSG"
        unsigned int placeholder;
    };

    /* All operations return:
       - 0 for whole message read;
       - 1 for calling again (nonblock).
       - -1 for errors; */

    int (*read_message)(struct pcdvobjs_stream *stream,
            char **buf, size_t *len, int *type);
    int (*send_text)(struct pcdvobjs_stream *stream,
            const char *text, size_t len);
    int (*send_binary)(struct pcdvobjs_stream *stream,
            const void *data, size_t len);
};

PCA_EXTERN_C_BEGIN

const struct purc_native_ops *
dvobjs_extend_stream_by_message(struct pcdvobjs_stream *stream,
        const struct purc_native_ops *basic_ops, purc_variant_t extra_opts)
    WTF_INTERNAL;

const struct purc_native_ops *
dvobjs_extend_stream_by_hbdbus(struct pcdvobjs_stream *stream,
        const struct purc_native_ops *basic_ops, purc_variant_t extra_opts)
    WTF_INTERNAL;

PCA_EXTERN_C_END

#endif  /* PURC_DVOBJS_STREAM_H */

