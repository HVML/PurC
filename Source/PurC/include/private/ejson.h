/*
 * @file ejson.h
 * @author XueShuming
 * @date 2021/07/19
 * @brief The interfaces eJSON parser.
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

#ifndef PURC_PRIVATE_EJSON_H
#define PURC_PRIVATE_EJSON_H

#include "purc-rwstream.h"
#include "private/stack.h"
#include "private/vcm.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define PCEJSON_DEFAULT_DEPTH 32

enum ejson_token_type {
    EJSON_TOKEN_START_OBJECT,
    EJSON_TOKEN_END_OBJECT,
    EJSON_TOKEN_START_ARRAY,
    EJSON_TOKEN_END_ARRAY,
    EJSON_TOKEN_KEY,
    EJSON_TOKEN_STRING,
    EJSON_TOKEN_NULL,
    EJSON_TOKEN_BOOLEAN,
    EJSON_TOKEN_NUMBER,
    EJSON_TOKEN_LONG_INT,
    EJSON_TOKEN_ULONG_INT,
    EJSON_TOKEN_LONG_DOUBLE,
    EJSON_TOKEN_COMMA,
    EJSON_TOKEN_TEXT,
    EJSON_TOKEN_BYTE_SQUENCE,
    EJSON_TOKEN_INFINITY,
    EJSON_TOKEN_NAN,
    EJSON_TOKEN_EOF,
};


struct pcejson;

struct pcejson_token {
    enum ejson_token_type type;
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

        /* for long string, long byte sequence, array, object,
         * and set (sz_ptr[0] for size, sz_ptr[1] for pointer).
         */
        uintptr_t   sz_ptr[2];
    };

};

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/*
 * Init pcejson
 */
void pcejson_init_once (void);

/*
 * Create ejson parser.
 */
struct pcejson* pcejson_create (uint32_t depth, uint32_t flags);

/*
 * Destroy ejson parser.
 */
void pcejson_destroy (struct pcejson* parser);

/*
 * Reset ejson parser.
 */
void pcejson_reset (struct pcejson* parser, uint32_t depth, uint32_t flags);

/*
 * Parse ejson.
 */
int pcejson_parse (struct pcvcm_node** vcm_tree, struct pcejson** parser,
                   purc_rwstream_t rwstream, uint32_t depth);

/*
 * Create a new pcejson token.
 */
struct pcejson_token* pcejson_token_new (enum ejson_token_type type,
                                         const uint8_t* bytes, size_t nr_bytes);

/*
 * Destory pcejson token.
 */
void pcejson_token_destroy (struct pcejson_token* token);

/*
 * Get one pcejson token from rwstream.
 */
struct pcejson_token* pcejson_next_token (struct pcejson* ejson,
                                          purc_rwstream_t rws);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* not defined PURC_PRIVATE_EJSON_H */

