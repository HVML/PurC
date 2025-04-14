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

#include "purc-utils.h"

#include <assert.h>

enum pcatom_bucket {
    ATOM_BUCKET_FIRST = 0,

    ATOM_BUCKET_DEF = ATOM_BUCKET_FIRST,
    ATOM_BUCKET_EXCEPT, /* the error and exception names such as NoData */
    ATOM_BUCKET_HVML,   /* HVML tag names and attribute names */
    ATOM_BUCKET_HTML,   /* HTML tag names and attribute names */
    ATOM_BUCKET_XGML,   /* XGML tag names and attribute names */
    ATOM_BUCKET_ACTION, /* the update actions: merge, displace, ... */
    ATOM_BUCKET_EVENT,  /* the event types such as changed, attached, ... */
    ATOM_BUCKET_RDROP,  /* the renderer operations: startSession, load, ... */
    ATOM_BUCKET_DVOBJ,  /* the keywords of DVObjs: all, default, ... */
    ATOM_BUCKET_RDRID,  /* the renderer unique id */

    /* XXX: change this if you add a new atom bucket. */
    ATOM_BUCKET_LAST = ATOM_BUCKET_RDRID,
};

/* Make sure ATOM_BUCKET_LAST is less than PURC_ATOM_BUCKETS_NR */
#define _COMPILE_TIME_ASSERT(name, x)               \
       typedef int _dummy_ ## name[(x) * 2 - 1]

_COMPILE_TIME_ASSERT(buckets, ATOM_BUCKET_LAST < PURC_ATOM_BUCKET_USER);
_COMPILE_TIME_ASSERT(bucket_except,
        ATOM_BUCKET_EXCEPT == PURC_ATOM_BUCKET_EXCEPT);

#undef _COMPILE_TIME_ASSERT

#define  ATOM_BUCKET_CUSTOM PURC_ATOM_BUCKET_USER

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

