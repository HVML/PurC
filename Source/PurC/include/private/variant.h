/**
 * @file variant-type.h
 * @author 
 * @date 2021/07/02
 * @brief The API for variant.
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



#ifndef _VARIANT_TYPES_H_
#define _VARIANT_TYPES_H_

#include "purc_variant.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

// define errors for variant
#define PURC_ERROR_VARIANT_NO_MEMORY        (PURC_ERROR_FIRST_VARIANT + 0)
#define PURC_ERROR_VARIANT_INVALID_VALUE    (PURC_ERROR_FIRST_VARIANT + 1)
#define PURC_ERROR_VARIANT_INVALID_TYPE     (PURC_ERROR_FIRST_VARIANT + 2)

#define MAX(a, b)   (a) > (b)? (a): (b);

#define PCVARIANT_FLAG_NOREF     0x0001
#define PCVARIANT_FLAG_NOFREE    0x0002

// for variant const, registered in thread instance
struct purc_variant_const {
    struct purc_variant_t pcvariant_null;
    struct purc_variant_t pcvariant_undefined;
    struct purc_variant_t pcvariant_false;
    struct purc_variant_t pcvariant_true;
}


struct purc_variant {

    /* variant type */
    enum variant_type type:8;

    /* real length for short string and byte sequence */
    unsigned int size:8;        

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
        uintptr_t   sz_ptr[2];

        /* for short string and byte sequence; the real space size of `bytes` is `max(sizeof(long double), sizeof(void*) * 2)` */
        uint8_t     bytes[0];
    };
};


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* _VARIANT_TYPES_H_ */
