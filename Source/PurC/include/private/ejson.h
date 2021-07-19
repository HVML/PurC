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

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

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
    ejson_after_name_unquoted_state,
    ejson_name_single_quoted_state,
    ejson_after_name_single_quoted_state,
    ejson_name_double_quoted_state,
    ejson_after_name_double_quoted_state,
    ejson_value_single_quoted_state,
    ejson_after_value_single_quoted_state,
    ejson_value_double_quoted_state,
    ejson_after_value_double_quoted_state,
    ejson_value_two_double_quoted_state,
    ejson_after_value_two_double_quoted_state,
    ejson_value_three_double_quoted_state,
    ejson_after_value_three_double_quoted_state,
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
    ejson_after_value_number_fraction_state,
    ejson_value_number_exponent_state,
    ejson_value_number_exponent_integer_state,
    ejson_after_value_number_exponent_integer_state,
    ejson_value_number_suffix_integer_state,
    ejson_after_value_number_suffix_integer_state,
    ejson_string_escape_state,
    ejson_string_escape_four_hexadecimal_digits_state
};

#define BEGIN_STATE(state_name)                                  \
    case state_name:                                             \
    state_name: {                                                \
        enum ejson_state current_state = state_name;             \
        UNUSED_PARAM(current_state);

#define END_STATE()                                             \
        break;                                                  \
    }

#define RETURN_IN_CURRENT_STATE(expression)                     \
    do {                                                        \
        tokenizer->state = current_state;                       \
        return expression;                                      \
    } while (false)

#define RECONSUME_IN(new_state)                                 \
    do {                                                        \
        goto new_state;                                         \
    } while (false)


struct pcejson {
    enum ejson_state state;
    int32_t depth;
    uint32_t flags;
};

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

struct pcejson* pcejson_create(int32_t depth, uint32_t flags);
void pcejson_destroy(struct pcejson* parser);
void pcejson_reset(struct pcejson* parser, int32_t depth, uint32_t flags);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* not defined PURC_PRIVATE_EJSON_H */

