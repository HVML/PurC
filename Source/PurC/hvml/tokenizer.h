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

#define SET_RETURN_STATE(new_state)                                         \
    do {                                                                    \
        parser->return_state = new_state;                                   \
    } while (false)

#define CHECK_TEMPLATE_TAG_AND_SWITCH_STATE(token)                          \
    do {                                                                    \
        const char* name = pchvml_token_get_name(token);                    \
        if (pchvml_token_is_type(token, PCHVML_TOKEN_START_TAG) &&          \
                pchvml_parser_is_template_tag(name)) {                      \
            parser->state = PCHVML_EJSON_DATA_STATE;                        \
        }                                                                   \
    } while (false)

#define RETURN_AND_SWITCH_TO(next_state)                                    \
    do {                                                                    \
        parser->state = next_state;                                         \
        pchvml_parser_save_tag_name(parser);                                \
        pchvml_token_done(parser->token);                                   \
        struct pchvml_token* token = parser->token;                         \
        parser->token = NULL;                                               \
        CHECK_TEMPLATE_TAG_AND_SWITCH_STATE(token);                         \
        return token;                                                       \
    } while (false)

#define RETURN_NEW_EOF_TOKEN()                                              \
    do {                                                                    \
        if (parser->token) {                                                \
            struct pchvml_token* token = parser->token;                     \
            parser->token = pchvml_token_new_eof();                         \
            return token;                                                   \
        }                                                                   \
        return pchvml_token_new_eof();                                      \
    } while (false)

#define RETURN_AND_STOP_PARSE()                                             \
    do {                                                                    \
        return NULL;                                                        \
    } while (false)

#define RESET_TEMP_BUFFER()                                                 \
    do {                                                                    \
        pchvml_buffer_reset(parser->temp_buffer);                           \
    } while (false)

PCA_EXTERN_C_BEGIN

PCA_INLINE bool is_whitespace(uint32_t uc)
{
    return uc == ' ' || uc == '\x0A' || uc == '\x09' || uc == '\x0C';
}

PCA_INLINE uint32_t to_ascii_lower_unchecked(uint32_t uc)
{
    return uc | 0x20;
}

PCA_INLINE bool is_ascii(uint32_t uc)
{
    return !(uc & ~0x7F);
}

PCA_INLINE bool is_ascii_lower(uint32_t uc)
{
    return uc >= 'a' && uc <= 'z';
}

PCA_INLINE bool is_ascii_upper(uint32_t uc)
{
     return uc >= 'A' && uc <= 'Z';
}

PCA_INLINE bool is_ascii_space(uint32_t uc)
{
    return uc <= ' ' && (uc == ' ' || (uc <= 0xD && uc >= 0x9));
}

PCA_INLINE bool is_ascii_digit(uint32_t uc)
{
    return uc >= '0' && uc <= '9';
}

PCA_INLINE bool is_ascii_binary_digit(uint32_t uc)
{
     return uc == '0' || uc == '1';
}

PCA_INLINE bool is_ascii_hex_digit(uint32_t uc)
{
     return is_ascii_digit(uc) || (
             to_ascii_lower_unchecked(uc) >= 'a' &&
             to_ascii_lower_unchecked(uc) <= 'f'
             );
}

PCA_INLINE bool is_ascii_upper_hex_digit(uint32_t uc)
{
     return is_ascii_digit(uc) || (uc >= 'A' && uc <= 'F');
}

PCA_INLINE bool is_ascii_lower_hex_digit(uint32_t uc)
{
     return is_ascii_digit(uc) || (uc >= 'a' && uc <= 'f');
}

PCA_INLINE bool is_ascii_octal_digit(uint32_t uc)
{
     return uc >= '0' && uc <= '7';
}

PCA_INLINE bool is_ascii_alpha(uint32_t uc)
{
    return is_ascii_lower(to_ascii_lower_unchecked(uc));
}

PCA_INLINE bool is_ascii_alpha_numeric(uint32_t uc)
{
    return is_ascii_digit(uc) || is_ascii_alpha(uc);
}

#define PCHVML_END_OF_FILE       0
PCA_INLINE bool is_eof(uint32_t uc)
{
    return uc == PCHVML_END_OF_FILE;
}

PCA_INLINE bool is_separator(uint32_t c)
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

PCA_EXTERN_C_END

#endif /* PURC_HVML_TOKENIZER_H */
