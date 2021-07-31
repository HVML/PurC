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

#if 1
#define PRINT_STATE(state_name)
#else
#define PRINT_STATE(state_name)                                           \
    fprintf(stderr, "in %s|wc=%c\n", pcejson_ejson_state_desc(state_name), \
            ejson->wc);
#endif

#define BEGIN_STATE(state_name)                                  \
    case state_name:                                             \
    {                                                \
        enum ejson_state current_state = state_name;             \
        UNUSED_PARAM(current_state);                             \
        PRINT_STATE(current_state);

#define END_STATE()                                             \
        break;                                                  \
    }

#define RECONSUME_IN(new_state)                                 \
    do {                                                        \
        ejson->state = new_state;                               \
        goto next_state;                                         \
    } while (false)

#define RECONSUME_IN_NEXT(new_state)                            \
    do {                                                        \
        ejson->state = new_state;                               \
        ejson->need_reconsume = true;                           \
    } while (false)

#define ADVANCE_TO(new_state)                                    \
    do {                                                        \
        ejson->state = new_state;                               \
        goto next_input;                                       \
    } while (false)

#define SWITCH_TO(new_state)                                    \
    do {                                                        \
        ejson->state = new_state;                               \
    } while (false)


enum ejson_state {
    EJSON_INIT_STATE,
    EJSON_FINISHED_STATE,
    EJSON_OBJECT_STATE,
    EJSON_AFTER_OBJECT_STATE,
    EJSON_ARRAY_STATE,
    EJSON_AFTER_ARRAY_STATE,
    EJSON_BEFORE_NAME_STATE,
    EJSON_AFTER_NAME_STATE,
    EJSON_BEFORE_VALUE_STATE,
    EJSON_AFTER_VALUE_STATE,
    EJSON_NAME_UNQUOTED_STATE,
    EJSON_NAME_SINGLE_QUOTED_STATE,
    EJSON_NAME_DOUBLE_QUOTED_STATE,
    EJSON_VALUE_SINGLE_QUOTED_STATE,
    EJSON_VALUE_DOUBLE_QUOTED_STATE,
    EJSON_AFTER_VALUE_DOUBLE_QUOTED_STATE,
    EJSON_VALUE_TWO_DOUBLE_QUOTED_STATE,
    EJSON_VALUE_THREE_DOUBLE_QUOTED_STATE,
    EJSON_KEYWORD_STATE,
    EJSON_AFTER_KEYWORD_STATE,
    EJSON_BYTE_SEQUENCE_STATE,
    EJSON_AFTER_BYTE_SEQUENCE_STATE,
    EJSON_HEX_BYTE_SEQUENCE_STATE,
    EJSON_BINARY_BYTE_SEQUENCE_STATE,
    EJSON_BASE64_BYTE_SEQUENCE_STATE,
    EJSON_VALUE_NUMBER_STATE,
    EJSON_AFTER_VALUE_NUMBER_STATE,
    EJSON_VALUE_NUMBER_INTEGER_STATE,
    EJSON_VALUE_NUMBER_FRACTION_STATE,
    EJSON_VALUE_NUMBER_EXPONENT_STATE,
    EJSON_VALUE_NUMBER_EXPONENT_INTEGER_STATE,
    EJSON_VALUE_NUMBER_SUFFIX_INTEGER_STATE,
    EJSON_VALUE_NUMBER_INFINITY_STATE,
    EJSON_VALUE_NAN_STATE,
    EJSON_STRING_ESCAPE_STATE,
    EJSON_STRING_ESCAPE_FOUR_HEXADECIMAL_DIGITS_STATE
};

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


struct pcejson {
    enum ejson_state state;
    enum ejson_state return_state;
    int32_t depth;
    uint32_t flags;
    struct pcutils_stack* stack;
    struct pcutils_stack* vcm_stack;
    purc_rwstream_t tmp_buff;
    purc_rwstream_t tmp_buff2;
    char c[8];
    int c_len;
    wchar_t wc;
    bool need_reconsume;
};

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

/**
 * Init pcejson
 */
void pcejson_init_once (void);

/**
 * Create ejson parser.
 */
struct pcejson* pcejson_create (int32_t depth, uint32_t flags);

/**
 * Destroy ejson parser.
 */
void pcejson_destroy (struct pcejson* parser);

/**
 * Reset ejson parser.
 */
void pcejson_reset (struct pcejson* parser, int32_t depth, uint32_t flags);

/**
 * Parse ejson.
 */
int pcejson_parse (struct pcvcm_node** vcm_tree, struct pcejson** parser,
        purc_rwstream_t rwstream);

/**
 * Create a new pcejson token.
 */
struct pcejson_token* pcejson_token_new (enum ejson_token_type type,
        const uint8_t* bytes, size_t nr_bytes);

/**
 * Destory pcejson token.
 */
void pcejson_token_destroy (struct pcejson_token* token);


/**
 * Get one pcejson token from rwstream.
 */
struct pcejson_token* pcejson_next_token (struct pcejson* ejson,
        purc_rwstream_t rws);

/**
 * Get ejson desc message
 */
const char* pcejson_ejson_state_desc (enum ejson_state state);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* not defined PURC_PRIVATE_EJSON_H */

