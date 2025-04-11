/**
 * @file socket.h
 * @author Vincent Wei
 * @date 2025/02/05
 * @brief The internal interfaces for dvobjs/socket
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

#ifndef PURC_DVOBJS_SOCKET_H
#define PURC_DVOBJS_SOCKET_H

#include "config.h"
#include "purc-macros.h"
#include "purc-variant.h"
#include "purc-rwstream.h"

#include "private/debug.h"
#include "private/errors.h"

#if HAVE(OPENSSL)
#include "private/openssl-shared-context.h"
#endif

#define DEF_BACKLOG     32
#define NATIVE_ENTITY_NAME_SOCKET       "socket"

enum pcdvobjs_socket_type {
    SOCKET_TYPE_STREAM_LOCAL,
    SOCKET_TYPE_STREAM_INET4,
    SOCKET_TYPE_STREAM_INET6,
    SOCKET_TYPE_STREAM_MIN = SOCKET_TYPE_STREAM_LOCAL,
    SOCKET_TYPE_STREAM_MAX = SOCKET_TYPE_STREAM_INET6,

    SOCKET_TYPE_DGRAM_LOCAL,
    SOCKET_TYPE_DGRAM_INET4,
    SOCKET_TYPE_DGRAM_INET6,
    SOCKET_TYPE_DGRAM_MIN = SOCKET_TYPE_DGRAM_LOCAL,
    SOCKET_TYPE_DGRAM_MAX = SOCKET_TYPE_DGRAM_INET6,
};

struct pcdvobjs_socket;

typedef struct pcdvobjs_socket {
    enum pcdvobjs_socket_type   type;
#if HAVE(OPENSSL)
    unsigned                        ssl_refc;
    SSL_CTX                        *ssl_ctx;
    struct openssl_shctx_wrapper   *ssl_shctx_wrapper;
#endif

    struct purc_broken_down_url *url;
    purc_variant_t              observed;    /* not inc ref */
    uintptr_t                   monitor;

    int                         fd;
    purc_atom_t                 cid;
} pcdvobjs_socket;

PCA_EXTERN_C_BEGIN

#if HAVE(OPENSSL)
void pcdvobjs_socket_ssl_ctx_delete(struct pcdvobjs_socket *socket);

static inline SSL_CTX *
pcdvobjs_socket_ssl_ctx_acquire(struct pcdvobjs_socket *socket) {
    if (socket->ssl_ctx) {
        socket->ssl_refc++;
    }

    return socket->ssl_ctx;
}

static inline void
pcdvobjs_socket_ssl_ctx_release(struct pcdvobjs_socket *socket) {
    if (socket->ssl_ctx) {
        socket->ssl_refc--;
        if (socket->ssl_refc == 0) {
            pcdvobjs_socket_ssl_ctx_delete(socket);
        }
    }
}

#endif

PCA_EXTERN_C_END

#endif  /* PURC_DVOBJS_SOCKET_H */

