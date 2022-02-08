/*
 * @file tokenizer.h
 * @author Xue Shuming
 * @date 2022/02/08
 * @brief The api of hvml tokenizer.
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
 *
 */

#ifndef PURC_HVML_TOKENIZER_H
#define PURC_HVML_TOKENIZER_H

#include "config.h"

#include "private/instance.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"

#ifdef HVML_DEBUG_PRINT
#define PRINT_STATE(state_name)                                             \
    fprintf(stderr, \
            "in %s|uc=%c|hex=0x%X|stack_is_empty=%d|stack_top=%c|vcm_node->type=%d\n",                              \
            pchvml_get_state_name(state_name), character, character,     \
            ejson_stack_is_empty(), (char)ejson_stack_top(),                \
            (parser->vcm_node != NULL ? (int)parser->vcm_node->type : -1));
#define SET_ERR(err)    do {                                                \
    fprintf(stderr, "error %s:%d %s\n", __FILE__, __LINE__,                 \
            pchvml_get_error_name(err));                                    \
    pcinst_set_error (err);                                                 \
} while (0)
#else
#define PRINT_STATE(state_name)
#define SET_ERR(err)    pcinst_set_error(err)
#endif

#define BEGIN_STATE(state_name)                                             \
    case state_name:                                                        \
    {                                                                       \
        enum pchvml_state curr_state = state_name;                          \
        UNUSED_PARAM(curr_state);                                           \
        PRINT_STATE(curr_state);

#define END_STATE()                                                         \
        break;                                                              \
    }

#define ADVANCE_TO(new_state)                                               \
    do {                                                                    \
        parser->state = new_state;                                          \
        goto next_input;                                                    \
    } while (false)

#define RECONSUME_IN(new_state)                                             \
    do {                                                                    \
        parser->state = new_state;                                          \
        goto next_state;                                                    \
    } while (false)

PCA_INLINE UNUSED_FUNCTION bool is_whitespace (uint32_t uc)
{
    return uc == ' ' || uc == '\x0A' || uc == '\x09' || uc == '\x0C';
}

PCA_INLINE UNUSED_FUNCTION uint32_t to_ascii_lower_unchecked (uint32_t uc)
{
    return uc | 0x20;
}

PCA_INLINE UNUSED_FUNCTION bool is_ascii (uint32_t uc)
{
    return !(uc & ~0x7F);
}

PCA_INLINE UNUSED_FUNCTION bool is_ascii_lower (uint32_t uc)
{
    return uc >= 'a' && uc <= 'z';
}

PCA_INLINE UNUSED_FUNCTION bool is_ascii_upper (uint32_t uc)
{
     return uc >= 'A' && uc <= 'Z';
}

PCA_INLINE UNUSED_FUNCTION bool is_ascii_space (uint32_t uc)
{
    return uc <= ' ' && (uc == ' ' || (uc <= 0xD && uc >= 0x9));
}

PCA_INLINE UNUSED_FUNCTION bool is_ascii_digit (uint32_t uc)
{
    return uc >= '0' && uc <= '9';
}

PCA_INLINE UNUSED_FUNCTION bool is_ascii_binary_digit (uint32_t uc)
{
     return uc == '0' || uc == '1';
}

PCA_INLINE UNUSED_FUNCTION bool is_ascii_hex_digit (uint32_t uc)
{
     return is_ascii_digit(uc) || (
             to_ascii_lower_unchecked(uc) >= 'a' &&
             to_ascii_lower_unchecked(uc) <= 'f'
             );
}

PCA_INLINE UNUSED_FUNCTION bool is_ascii_upper_hex_digit (uint32_t uc)
{
     return is_ascii_digit(uc) || (uc >= 'A' && uc <= 'F');
}

PCA_INLINE UNUSED_FUNCTION bool is_ascii_lower_hex_digit (uint32_t uc)
{
     return is_ascii_digit(uc) || (uc >= 'a' && uc <= 'f');
}

PCA_INLINE UNUSED_FUNCTION bool is_ascii_octal_digit (uint32_t uc)
{
     return uc >= '0' && uc <= '7';
}

PCA_INLINE UNUSED_FUNCTION bool is_ascii_alpha (uint32_t uc)
{
    return is_ascii_lower(to_ascii_lower_unchecked(uc));
}

PCA_INLINE UNUSED_FUNCTION bool is_ascii_alpha_numeric (uint32_t uc)
{
    return is_ascii_digit(uc) || is_ascii_alpha(uc);
}

#define PCHVML_END_OF_FILE       0
PCA_INLINE UNUSED_FUNCTION bool is_eof (uint32_t uc)
{
    return uc == PCHVML_END_OF_FILE;
}

PCA_INLINE UNUSED_FUNCTION bool is_separator(uint32_t c)
{
    switch (c) {
        case '{':
        case '}':
        case '[':
        case ']':
        case '<':
        case '>':
        case '(':
        case ')':
        case ',':
        case ':':
            return true;
    }
    return false;
}

#define PCHVML_NEXT_TOKEN_BEGIN                                         \
struct pchvml_token* pchvml_next_token(struct pchvml_parser* parser,    \
                                          purc_rwstream_t rws)          \
{                                                                       \
    struct pchvml_uc* hvml_uc = NULL;                                   \
    uint32_t character = 0;                                             \
    if (parser->token) {                                                \
        struct pchvml_token* token = parser->token;                     \
        parser->token = NULL;                                           \
        return token;                                                   \
    }                                                                   \
                                                                        \
    pchvml_rwswrap_set_rwstream (parser->rwswrap, rws);                 \
                                                                        \
next_input:                                                             \
    hvml_uc = pchvml_rwswrap_next_char (parser->rwswrap);               \
    if (!hvml_uc) {                                                     \
        return NULL;                                                    \
    }                                                                   \
                                                                        \
    character = hvml_uc->character;                                     \
    if (character == PCHVML_INVALID_CHARACTER) {                        \
        SET_ERR(PCHVML_ERROR_INVALID_UTF8_CHARACTER);                   \
        return NULL;                                                    \
    }                                                                   \
                                                                        \
    if (is_separator(character)) {                                      \
        if (parser->prev_separator == ',' && character == ',') {        \
            SET_ERR(PCHVML_ERROR_UNEXPECTED_COMMA);                     \
            return NULL;                                                \
        }                                                               \
        parser->prev_separator = character;                             \
    }                                                                   \
    else if (!is_whitespace(character)) {                               \
        parser->prev_separator = 0;                                     \
    }                                                                   \
                                                                        \
next_state:                                                             \
    switch (parser->state) {


#define PCHVML_NEXT_TOKEN_END                                           \
    default:                                                            \
        break;                                                          \
    }                                                                   \
    return NULL;                                                        \
}

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* PURC_HVML_TOKENIZER_H */
