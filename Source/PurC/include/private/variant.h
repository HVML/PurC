/*
 * @file variant.h
 * @author Vincent Wei (https://github.com/VincentWei)
 * @date 2021/07/05
 * @brief The structures for PurC variant
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

#ifndef PURC_PRIVATE_VARIANT_H
#define PURC_PRIVATE_VARIANT_H

#include "purc.h"

#include "config.h"

#define PCVARIANT_TYPE_UNDEFINED        0
#define PCVARIANT_TYPE_NULL             1
#define PCVARIANT_TYPE_BOOLEAN          2
#define PCVARIANT_TYPE_NUMBER           3
#define PCVARIANT_TYPE_LONGINT          4
#define PCVARIANT_TYPE_LONGUINT         5
#define PCVARIANT_TYPE_LONGDOUBLE       6
#define PCVARIANT_TYPE_STRING           7
#define PCVARIANT_TYPE_BYTESEQ          8
#define PCVARIANT_TYPE_DYNAMIC          9
#define PCVARIANT_TYPE_NATIVE          10

#define PCVARIANT_TYPE_OBJECT          11
#define PCVARIANT_TYPE_ARRAY           12
#define PCVARIANT_TYPE_SET             13


#define PCVARIANT_FLAG_NOREF     0x0001
#define PCVARIANT_FLAG_NOFREE    0x0002
#define PCVARIANT_FLAG_LONG      0x0004

struct purc_variant {
    /* variant type */
    unsigned int type:8;

    /* real length for short string and byte sequence */
    unsigned int size:8;  // not-counting-null-terminator if string

    /* flags */
    unsigned int flags:16;

    /* reference count */
    unsigned int refc;

    /* value */
    union {
        /* for boolean */
        bool        b;

        /* for number */
        double      d;

        /* for long integer */
        int64_t     i64;

        /* for unsigned long integer */
        uint64_t    u64;

        /* for long double */
        long double ld;

        /* for dynamic and native variant (two pointers) */
        void*       ptr2[2];

        /* for long string, long byte sequence, array, and object (one for size and the other for pointer). */
        uintptr_t   sz_ptr[2];  // not-counting-null-terminator if long string

        /* for short string and byte sequence; the real space size of `bytes` is `max(sizeof(long double), sizeof(void*) * 2)` */
        uint8_t     bytes[0];
    } u;
};

#endif /* not defined PURC_PRIVATE_VARIANT_H */


