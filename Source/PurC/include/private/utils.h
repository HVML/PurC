/*
 * @file utils.h
 * @author Vincent Wei (https://github.com/VincentWei)
 * @date 2021/07/05
 * @brief The internal utility interfaces.
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

#ifndef PURC_PRIVATE_UTILS_H
#define PURC_PRIVATE_UTILS_H

#include "purc-utils.h"

#include "config.h"

#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>

#if OS(LINUX) || OS(UNIX)
#include <limits.h>
#include <stdio.h>
#endif // OS(LINUX) || OS(UNIX)

#if OS(WINDOWS)
#   define PATH_SEP '\\'
#   define PATH_SEP_STR "\\"
#else
#   define PATH_SEP '/'
#   define PATH_SEP_STR "/"
#endif

#define IS_PATH_SEP(c) ((c) == PATH_SEP)

#define pcutils_html_whitespace(onechar, action, logic)   \
    (onechar action ' '  logic                            \
     onechar action '\t' logic                            \
     onechar action '\n' logic                            \
     onechar action '\f' logic                            \
     onechar action '\r')

static inline size_t
pcutils_power(size_t t, size_t k)
{
    size_t res = 1;

    while (k) {
        if (k & 1) {
            res *= t;
        }

        t *= t;
        k >>= 1;
    }

    return res;
}

static inline size_t
pcutils_hash_hash(const unsigned char *key, size_t key_size)
{
    size_t hash, i;

    for (hash = i = 0; i < key_size; i++) {
        hash += key[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }

    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    return hash;
}

/*
 * calloc_a(size_t len, [void **addr, size_t len,...], NULL)
 *
 * allocate a block of memory big enough to hold multiple aligned objects.
 * the pointer to the full object (starting with the first chunk) is returned,
 * all other pointers are stored in the locations behind extra addr arguments.
 * the last argument needs to be a NULL pointer
 */
#define calloc_a(len, ...) pcutils_calloc_a(len, ##__VA_ARGS__, NULL)

#ifdef __cplusplus
extern "C" {
#endif

void *pcutils_calloc_a(size_t len, ...) WTF_INTERNAL;

typedef enum {
    PURC_K_ALGO_CRC32_UNKNOWN = -1,

#define PURC_ALGO_CRC32             "CRC-32"
    PURC_K_ALGO_CRC32 = 0,
#define PURC_ALGO_CRC32_BZIP2       "CRC-32/BZIP2"
    PURC_K_ALGO_CRC32_BZIP2,
#define PURC_ALGO_CRC32_MPEG2       "CRC-32/MPEG-2"
    PURC_K_ALGO_CRC32_MPEG2,
#define PURC_ALGO_CRC32_POSIX       "CRC-32/POSIX"
    PURC_K_ALGO_CRC32_POSIX,
#define PURC_ALGO_CRC32_XFER        "CRC-32/XFER"
    PURC_K_ALGO_CRC32_XFER,
#define PURC_ALGO_CRC32_ISCSI       "CRC-32/ISCSI"
    PURC_K_ALGO_CRC32_ISCSI,
#define PURC_ALGO_CRC32C            "CRC-32C"
    PURC_K_ALGO_CRC32C,
#define PURC_ALGO_CRC32_BASE91_D    "CRC-32/BASE91-D"
    PURC_K_ALGO_CRC32_BASE91_D,
#define PURC_ALGO_CRC32D            "CRC-32D"
    PURC_K_ALGO_CRC32D,
#define PURC_ALGO_CRC32_JAMCRC      "CRC-32/JAMCRC"
    PURC_K_ALGO_CRC32_JAMCRC,
#define PURC_ALGO_CRC32_AIXM        "CRC-32/AIXM"
    PURC_K_ALGO_CRC32_AIXM,
#define PURC_ALGO_CRC32Q            "CRC-32Q"
    PURC_K_ALGO_CRC32Q,
} purc_crc32_algo_t;

typedef struct pcutils_crc32_ctxt {
    uint32_t    poly;
    uint32_t    init;
    uint32_t    xorout;
    uint32_t    crc32;

    bool        refin;
    bool        refout;

    union {
        const uint32_t *table_static;
        uint32_t       *table_alloc;
    };
} pcutils_crc32_ctxt;

void
pcutils_crc32_begin(pcutils_crc32_ctxt *ctxt, purc_crc32_algo_t algo);

void
pcutils_crc32_update(pcutils_crc32_ctxt *ctxt, const void *data, size_t sz);

void
pcutils_crc32_end(pcutils_crc32_ctxt *ctxt, uint32_t* crc32);

pcutils_crc32_ctxt *
pcutils_crc32_begin_custom(uint32_t poly, uint32_t init, uint32_t xorout,
        bool refin, bool refout);

void
pcutils_crc32_end_custom(pcutils_crc32_ctxt *ctxt, uint32_t* crc32);

int pcutils_parse_int32(const char *buf, size_t len, int32_t *retval);
int pcutils_parse_uint32(const char *buf, size_t len, uint32_t *retval);
int pcutils_parse_int64(const char *buf, size_t len, int64_t *retval);
int pcutils_parse_uint64(const char *buf, size_t len, uint64_t *retval);
int pcutils_parse_double(const char *buf, size_t len, double *retval);
int pcutils_parse_long_double(const char *buf, size_t len, long double *retval);

#define DECL_MYSTRING(name) struct pcutils_mystring name = { NULL, 0, 0 }

#ifdef __cplusplus
}
#endif

#include <math.h>
#include <float.h>

/* securely comparison of floating-point variables */
static inline bool pcutils_equal_doubles(double a, double b)
{
    double max_val = fabs(a) > fabs(b) ? fabs(a) : fabs(b);
    return (fabs(a - b) <= max_val * DBL_EPSILON);
}

/* securely comparison of floating-point variables */
static inline bool pcutils_equal_longdoubles(long double a, long double b)
{
    long double max_val = fabsl(a) > fabsl(b) ? fabsl(a) : fabsl(b);
    return (fabsl(a - b) <= max_val * LDBL_EPSILON);
}

#endif /* not defined PURC_PRIVATE_UTILS_H */

