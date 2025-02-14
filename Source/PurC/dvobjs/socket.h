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
#include <openssl/ssl.h>
#endif

#define DEF_BACKLOG     32
#define NATIVE_ENTITY_NAME_SOCKET       "socket"

enum pcdvobjs_socket_type {
    SOCKET_TYPE_STREAM,
    SOCKET_TYPE_DGRAM,
};

struct pcdvobjs_socket;

typedef struct pcdvobjs_socket {
    enum pcdvobjs_socket_type type;
    struct purc_broken_down_url *url;
    purc_variant_t observed;    /* not inc ref */
    uintptr_t monitor;
    int fd;
    purc_atom_t cid;

#if HAVE(OPENSSL)
    SSL_CTX            *ssl_ctxt;   /* per-server */
#endif
} pcdvobjs_socket;

PCA_EXTERN_C_BEGIN

PCA_EXTERN_C_END

#endif  /* PURC_DVOBJS_SOCKET_H */

