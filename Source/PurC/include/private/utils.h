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

#include "config.h"

#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>

#if OS(LINUX) || OS(UNIX)
#include <limits.h>
#include <stdio.h>
#include <libgen.h>
#endif // OS(LINUX) || OS(UNIX)

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

void pcutils_atom_init_once(void) WTF_INTERNAL;
void pcutils_atom_term_once(void) WTF_INTERNAL;

void *pcutils_calloc_a(size_t len, ...) WTF_INTERNAL;

/*
 * This is an OpenSSL-compatible implementation of the RSA Data Security, Inc.
 * MD5 Message-Digest Algorithm (RFC 1321).
 *
 * Homepage:
 * http://openwall.info/wiki/people/solar/software/public-domain-source-code/md5
 *
 * Author:
 * Alexander Peslyak, better known as Solar Designer <solar at openwall.com>
 *
 * This software was written by Alexander Peslyak in 2001.  No copyright is
 * claimed, and the software is hereby placed in the public domain.
 * In case this attempt to disclaim copyright and place the software in the
 * public domain is deemed null and void, then the software is
 * Copyright (c) 2001 Alexander Peslyak and it is hereby released to the
 * general public under the following terms:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted.
 *
 * There's ABSOLUTELY NO WARRANTY, express or implied.
 *
 * See md5.c for more information.
 */

#define MD5_DIGEST_SIZE          (16)

typedef struct pcutils_md5_ctx {
    uint32_t lo, hi;
    uint32_t a, b, c, d;
    unsigned char buffer[64];
} pcutils_md5_ctx_t;

void pcutils_md5_begin (pcutils_md5_ctx_t *ctx);
void pcutils_md5_hash (const void *data, size_t length, pcutils_md5_ctx_t *ctx);
void pcutils_md5_end (unsigned char *resbuf, pcutils_md5_ctx_t *ctx);

/* \a digest should be long enough (at least 16) to store the returned digest */
void pcutils_md5digest (const char *string, unsigned char *digest);
int pcutils_md5sum (const char *file, unsigned char *md5_buf);

/* hex must be long enough to hold the heximal characters */
void pcutils_bin2hex (const unsigned char *bin, int len, char *hex);

/* bin must be long enough to hold the bytes.
   return the number of bytes converted, <= 0 for error */
int pcutils_hex2bin (const char *hex, unsigned char *bin);

int pcutils_parse_int64(const char *buf, size_t len, int64_t *retval);
int pcutils_parse_uint64(const char *buf, size_t len, uint64_t *retval);
int pcutils_parse_double(const char *buf, size_t len, double *retval);
int pcutils_parse_long_double(const char *buf, size_t len, long double *retval);

#ifdef __cplusplus
}
#endif

#endif /* not defined PURC_PRIVATE_UTILS_H */

