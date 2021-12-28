/**
 * @file atom-buckets.h
 * @author Vincent Wei
 * @date 2021/12/28
 * @brief The internal interfaces for atom buckets.
 *
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
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

#ifndef PURC_PRIVATE_ATOM_BUCKETS_H
#define PURC_PRIVATE_ATOM_BUCKETS_H

#include "config.h"
#include "purc-utils.h"

#include <assert.h>

enum {
    ATOM_BUCKET_DEF = 0,
    ATOM_BUCKET_HVML,   /* HVML tag names and attribute names */
    ATOM_BUCKET_HTML,   /* HTML tag names and attribute names */
    ATOM_BUCKET_XGML,   /* XGML tag names and attribute names */
    ATOM_BUCKET_ACTION, /* the update action names: merge, displace, ... */
    ATOM_BUCKET_EXCEPT, /* the error and exception names such as NoData */
    ATOM_BUCKET_EVENT,  /* the event names such as changed, attached, ... */

    /* XXX: change this if you append a new bucket. */
    PURC_VARIANT_TYPE_LAST = ATOM_BUCKET_UPDATE_ACTION,
};

#if PURC_VARIANT_TYPE_LAST >= PURC_ATOM_BUCKETS_NR
#error "Too many buckets; please adjust PURC_ATOM_BUCKET_BITS"
#endif

struct const_str_atom {
    const char *str;
    purc_atom_t atom;
};

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* PURC_PRIVATE_ATOM_BUCKETS_H */

