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
#include "private/stream.h"

enum pcdvobjs_stdio_type {
    STDIO_TYPE_STDIN,
    STDIO_TYPE_STDOUT,
    STDIO_TYPE_STDERR,
};

PCA_EXTERN_C_BEGIN

const struct purc_native_ops *
dvobjs_extend_stream_by_message(struct pcdvobjs_stream *stream,
        const struct purc_native_ops *super_ops, purc_variant_t extra_opts)
    WTF_INTERNAL;

const struct purc_native_ops *
dvobjs_extend_stream_by_hbdbus(struct pcdvobjs_stream *stream,
        const struct purc_native_ops *super_ops, purc_variant_t extra_opts)
    WTF_INTERNAL;

const struct purc_native_ops *
dvobjs_extend_stream_by_websocket(struct pcdvobjs_stream *stream,
        const struct purc_native_ops *super_ops, purc_variant_t extra_opts)
    WTF_INTERNAL;

purc_variant_t
dvobjs_create_stream_by_accepted(struct pcdvobjs_socket *socket,
        purc_atom_t schema, char *peer_addr, char *peer_port, int fd,
        purc_variant_t prot, purc_variant_t extra_opts)
    WTF_INTERNAL;

PCA_EXTERN_C_END

#endif  /* PURC_DVOBJS_STREAM_H */

