/**
 * @file variant.h
 * @author 
 * @date 2021/07/02
 * @brief The internal interfaces for variant.
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

#ifndef PURC_PRIVATE_H
#define PURC_PRIVATE_H

#include "config.h"
#include "purc-variant.h"

#include <assert.h>

#if HAVE(GLIB)
    #include <gmodule.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define PCVARIANT_FLAG_NOREF    (0x01 << 0)
#define PCVARIANT_FLAG_NOFREE   (0x01 << 1)
#define PCVARIANT_FLAG_LONG     (0x01 << 15)    // for long string or sequence
#define PCVARIANT_FLAG_SIGNED   (0x01 << 15)    // for signed int

#define PVT(t) (PURC_VARIANT_TYPE##t)

// fix me: if we need `assert` in both debug and release build, better approach?
#define PURC_VARIANT_ASSERT(s) assert(s)

#define MAX_RESERVED_VARIANTS  32

// structure for variant
struct purc_variant {

    /* variant type */
    unsigned int type:8;

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

        /* for long string, long byte sequence, array, and object (sz_ptr[0] for pointer, sz_ptr[1] for size). */
        uintptr_t   sz_ptr[2];

        /* for short string and byte sequence; the real space size of `bytes` is `max(sizeof(long double), sizeof(void*) * 2)` */
        uint8_t     bytes[0];
    };
};

struct pcvariant_heap {
    struct purc_variant v_undefined;
    struct purc_variant v_null;
    struct purc_variant v_false;
    struct purc_variant v_true;

    struct purc_variant_stat stat;

    purc_variant_t nr_reserved [MAX_RESERVED_VARIANTS];
    int readpos;
    int writepos;
};

// initialize variant module
bool pcvariant_init_module(void) WTF_INTERNAL;

#if HAVE(GLIB)
static inline void * pcvariant_alloc_mem(size_t size)           \
                { return (void *)g_slice_alloc((gsize)size); }
static inline void * pcvariant_alloc_mem_0(size_t size)         \
                { return (void *)g_slice_alloc0((gsize)size); }
static inline void pcvariant_free_mem(size_t size, void *ptr)   \
                { return g_slice_free1((gsize)size, (gpointer)ptr); }
#else
static inline void * pcvariant_alloc_mem(size_t size)           \
                { return malloc(size); }
static inline void * pcvariant_alloc_mem_0(size_t size)         \
                { return (void *)calloc(size); }
static inline void pcvariant_free_mem(size_t size, void *ptr)   \
                { return free(ptr); }
#endif

// for release the resource in a variant
typedef void (* pcvariant_release_fn)(purc_variant_t value);

// for custom serialization function.
typedef int (* pcvariant_to_json_string_fn)(purc_variant_t * value, purc_rwstream *rw, int level, int flags);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* PURC_PRIVATE_H */
