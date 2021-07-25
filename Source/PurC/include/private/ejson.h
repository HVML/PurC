/*
 * @file ejson.h
 * @author XueShuming
 * @date 2021/07/19
 * @brief The interfaces for N-ary trees.
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

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#if 1
#define PRINT_STATE(state_name)
#else
#define PRINT_STATE(state_name)                                           \
        fprintf(stderr, "in %s\n", pcejson_ejson_state_desc(state_name));
#endif

#define BEGIN_STATE(state_name)                                  \
    case state_name:                                             \
    state_name: {                                                \
        enum ejson_state current_state = state_name;             \
        UNUSED_PARAM(current_state);                             \
        PRINT_STATE(current_state);

#define END_STATE()                                             \
        break;                                                  \
    }

#define RETURN_IN_CURRENT_STATE(expression)                     \
    do {                                                        \
        ejson->state = current_state;                           \
        return expression;                                      \
    } while (false)

#define RECONSUME_IN(new_state)                                 \
    do {                                                        \
        ejson->state = new_state;                               \
        goto new_state;                                         \
    } while (false)


#define ADVANCE_TO(new_state)                                    \
    do {                                                         \
        len = purc_rwstream_read_utf8_char (rws, buf_utf8, &wc); \
        if (len <= 0) {                                          \
            return NULL;                                         \
        }                                                        \
        ejson->state = new_state;                                \
        goto new_state;                                          \
    } while (false)

#define SWITCH_TO(new_state)                                    \
    do {                                                        \
        ejson->state = new_state;                               \
    } while (false)

#define RETURN_TO(new_state)                                    \
    do {                                                        \
        ejson->state = new_state;                               \
        goto next_input;                                       \
    } while (false)

enum ejson_state {
    ejson_init_state,
    ejson_finished_state,
    ejson_object_state,
    ejson_after_object_state,
    ejson_array_state,
    ejson_after_array_state,
    ejson_before_name_state,
    ejson_after_name_state,
    ejson_before_value_state,
    ejson_after_value_state,
    ejson_name_unquoted_state,
    ejson_name_single_quoted_state,
    ejson_name_double_quoted_state,
    ejson_value_single_quoted_state,
    ejson_value_double_quoted_state,
    ejson_after_value_double_quoted_state,
    ejson_value_two_double_quoted_state,
    ejson_value_three_double_quoted_state,
    ejson_keyword_state,
    ejson_after_keyword_state,
    ejson_byte_sequence_state,
    ejson_after_byte_sequence_state,
    ejson_hex_byte_sequence_state,
    ejson_binary_byte_sequence_state,
    ejson_base64_byte_sequence_state,
    ejson_value_number_state,
    ejson_after_value_number_state,
    ejson_value_number_integer_state,
    ejson_value_number_fraction_state,
    ejson_value_number_exponent_state,
    ejson_value_number_exponent_integer_state,
    ejson_value_number_suffix_integer_state,
    ejson_string_escape_state,
    ejson_string_escape_four_hexadecimal_digits_state
};

enum ejson_token_type {
    ejson_token_start_object,
    ejson_token_end_object,
    ejson_token_start_array,
    ejson_token_end_array,
    ejson_token_key,
    ejson_token_string,
    ejson_token_null,
    ejson_token_boolean,
    ejson_token_number,
    ejson_token_long_integer_number,
    ejson_token_unsigned_long_integer_number,
    ejson_token_long_double_number,
    ejson_token_colon,
    ejson_token_comma,
    ejson_token_text,
    ejson_token_byte_squence,
    ejson_token_eof,
};


struct pcejson_stack {
    uint8_t* buf;
    uint32_t capacity;
    int32_t last;
};

struct pcejson {
    enum ejson_state state;
    enum ejson_state return_state;
    int32_t depth;
    uint32_t flags;
    struct pcejson_stack* stack;
    purc_rwstream_t rws;
    purc_rwstream_t rws2;
};

struct pcejson_token {
    enum ejson_token_type type;
    char* buf;
};

struct pcvcm_tree;
typedef struct pcvcm_tree* pcvcm_tree_t;

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * Create a new pcejson stack.
 */
struct pcejson_stack* pcejson_stack_new(size_t sz_init);

/**
 * Check if the stack is empty.
 */
bool pcejson_stack_is_empty(struct pcejson_stack* stack);

/**
 * Push a character to the pcejson stack.
 */
void pcejson_stack_push(struct pcejson_stack* stack, uint8_t c);

/**
 * Pop a character from the pcejson stack.
 */
uint8_t pcejson_stack_pop(struct pcejson_stack* stack);

/**
 * Get the first character of the pcejson stack.
 */
uint8_t pcejson_stack_first(struct pcejson_stack* stack);

/**
 * Get the last character of the pcejson stack.
 */
uint8_t pcejson_stack_last(struct pcejson_stack* stack);

/**
 * Destory pcejson stack.
 */
void pcejson_stack_destroy(struct pcejson_stack* stack);

/**
 * Create ejson parser.
 */
struct pcejson* pcejson_create(int32_t depth, uint32_t flags);

/**
 * Destroy ejson parser.
 */
void pcejson_destroy(struct pcejson* parser);

/**
 * Reset ejson parser.
 */
void pcejson_reset(struct pcejson* parser, int32_t depth, uint32_t flags);

/**
 * Parse ejson.
 */
int pcejson_parse(pcvcm_tree_t vcm_tree, purc_rwstream_t rwstream);

/**
 * Create a new pcejson token.
 */
struct pcejson_token* pcejson_token_new(enum ejson_token_type type, char* buf);

/**
 * Destory pcejson token.
 */
void pcejson_token_destroy(struct pcejson_token* token);


/**
 * Get one pcejson token from rwstream.
 */
struct pcejson_token* pcejson_next_token(struct pcejson* ejson, purc_rwstream_t rws);

/**
 * Get ejson desc message
 */
const char* pcejson_ejson_state_desc(enum ejson_state state);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* not defined PURC_PRIVATE_EJSON_H */

