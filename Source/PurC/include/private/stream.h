/**
 * @file stream.h
 * @author Vincent Wei
 * @date 2025/03/03
 * @brief The internal interfaces for dvobjs/stream;
 *        derived from dvobjs/stream.h
 *
 * Copyright (C) 2025 FMSoft <https://www.fmsoft.cn>
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

#ifndef PURC_PRIVATTE_STREAM_H
#define PURC_PRIVATTE_STREAM_H

#include "purc-macros.h"
#include "purc-variant.h"
#include "purc-rwstream.h"

#include "private/debug.h"
#include "private/errors.h"

enum pcdvobjs_stream_type {
    STREAM_TYPE_FILE,
    STREAM_TYPE_PIPE,
    STREAM_TYPE_FIFO,
    STREAM_TYPE_UNIX,
    STREAM_TYPE_INET,
};

struct pcdvobjs_stream;
struct stream_extended_data;

enum stream_socket_role {
    SR_CLIENT = 0,
    SR_SERVER,
};

enum stream_message_type {
    MT_UNKNOWN = 0,
    MT_TEXT,
    MT_BINARY,
    MT_PING,
    MT_PONG,
    MT_CLOSE
};

enum stream_inet_socket_family {
    ISF_UNSPEC = 0,
    ISF_INET4,
    ISF_INET6,
};

struct stream_messaging_ops {
    int (*send_message)(struct pcdvobjs_stream *stream,
            bool text_or_bin, const char *text, size_t len);
    void (*shut_off)(struct pcdvobjs_stream *stream);

    /* the following operations can be overridden by extended layer */
    int (*on_message)(struct pcdvobjs_stream *stream, int type,
            char *msg, size_t len, int *owner_taken);
    int (*on_error)(struct pcdvobjs_stream *stream, int errcode);
    void (*cleanup)(struct pcdvobjs_stream *stream);

    /* When runloop is not available, the creator of the stream entity
       should call the following functions to give the chance of the stream
       to read/write data from/to socket. */
    bool (*on_readable)(int fd, int event, void *stream);
    bool (*on_writable)(int fd, int event, void *stream);

    /* When runloop is not available, the creator of the stream entity
       should call the following function to give the chance of the stream
       to ping the peer. */
    void (*on_ping_timer)(void *, const char *, void *stream);
};

#define NATIVE_ENTITY_NAME_STREAM       "stream"
#define STREAM_EXT_SIG_MSG              "MSG"
#define STREAM_EXT_SIG_HBS              "HBS"

struct stream_extended {
    char signature[4];

    struct stream_extended_data    *data;
    struct purc_native_ops         *super_ops;
    union {
        struct stream_messaging_ops    *msg_ops;
        struct stream_hbdbus_ops       *bus_ops;
    };
};

struct pcdvobjs_socket;
typedef struct pcdvobjs_stream {
    enum pcdvobjs_stream_type type;
    struct purc_broken_down_url *url;
    purc_rwstream_t stm4r;      /* stream for read */
    purc_rwstream_t stm4w;      /* stream for write */
    purc_variant_t observed;    /* not inc ref */

    #define NR_STREAM_MONITORS      2
    uintptr_t monitors[0];
    uintptr_t monitor4r, monitor4w;
    int ioevents4r, ioevents4w;
    int fd4r, fd4w;

    pid_t cpid;             /* only for pipe, the pid of child */
    purc_atom_t cid;

    char *peer_addr;        /* the address of the connection peer; 0.9.22 */
    char *peer_port;        /* the port of the connection peer; 0.9.22 */

    struct stream_extended ext0;   /* for presentation layer */
    struct stream_extended ext1;   /* for application layer */

    /* If the stream is accepted from a stream socket. */
    struct pcdvobjs_socket  *socket;
} pcdvobjs_stream;

PCA_EXTERN_C_BEGIN

purc_variant_t
dvobjs_create_stream_from_url(const char *url, purc_variant_t option,
        const char *ext_prot, purc_variant_t extra_opts)
    WTF_INTERNAL;

purc_variant_t
dvobjs_create_stream_from_fd(int fd, purc_variant_t option,
        const char *ext_prot, purc_variant_t extra_opts)
    WTF_INTERNAL;

PCA_EXTERN_C_END

#endif  /* PURC_PRIVATTE_STREAM_H */

