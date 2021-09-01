/*
 * @file hvml.c
 * @author Xu Xiaohong
 * @date 2021/08/23
 * @brief The implementation of public part for hvml parser.
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

#include "private/instance.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/edom.h"
#include "private/hvml.h"
#include "tempbuffer.h"
#include "hvml-token.h"
#include "config.h"

#if HAVE(GLIB)
#include <gmodule.h>
#else
#include <stdlib.h>
#endif

#define HVML_END_OF_FILE       0

#if HAVE(GLIB)
#define    HVML_ALLOC(sz)   g_slice_alloc0(sz)
#define    HVML_FREE(p)     g_slice_free1(sizeof(*p), (gpointer)p)
#else
#define    HVML_ALLOC(sz)   calloc(1, sz)
#define    HVML_FREE(p)     free(p)
#endif

#if 1
#define PRINT_STATE(state_name)
#else
#define PRINT_STATE(state_name)                                             \
    fprintf(stderr, "in %s|wc=%c|hex=%x\n",                                 \
            pchvml_pchvml_state_desc(state_name), hvml->wc, hvml->wc);
#endif

#define BEGIN_STATE(state_name)                                             \
    case state_name:                                                        \
    {                                                                       \
        enum pchvml_state current_state = state_name;                        \
        UNUSED_PARAM(current_state);                                        \
        PRINT_STATE(current_state);

#define END_STATE()                                                         \
        break;                                                              \
    }

#define RECONSUME_IN(new_state)                                             \
    do {                                                                    \
        hvml->state = new_state;                                           \
        goto next_state;                                                    \
    } while (false)

#define RECONSUME_IN_NEXT(new_state)                                        \
    do {                                                                    \
        hvml->state = new_state;                                           \
        hvml->need_reconsume = true;                                       \
    } while (false)

#define ADVANCE_TO(new_state)                                               \
    do {                                                                    \
        hvml->state = new_state;                                           \
        goto next_input;                                                    \
    } while (false)

#define SWITCH_TO(new_state)                                                \
    do {                                                                    \
        hvml->state = new_state;                                           \
    } while (false)

#define RETURN_IN_CURRENT_STATE(expression)                                \
    do {                                                                   \
        hvml->state = current_state;                                       \
        hvml->need_reconsume = true;                                       \
        if (expression) {                                                  \
            pchvml_token_done(hvml->current_token);                        \
            struct pchvml_token* token = hvml->current_token;              \
            return token;                                                  \
        }                                                                  \
        return NULL;                                                       \
    } while (false)

#define RETURN_AND_SWITCH_TO(next_state)                                   \
    do {                                                                   \
        hvml->state = next_state;                                          \
        pchvml_token_done(hvml->current_token);                            \
        struct pchvml_token* token = hvml->current_token;                  \
        return token;                                                      \
    } while (false)

#define RETURN_AND_RECONSUME_IN(next_state)                                \
    do {                                                                   \
        hvml->state = next_state;                                          \
        pchvml_token_done(hvml->current_token);                            \
        struct pchvml_token* token = hvml->current_token;                  \
        return token;                                                      \
    } while (false)

#define STATE_DESC(state_name)                                              \
    case state_name:                                                        \
        return ""#state_name;                                               \

#define BUFFER_CHARACTER(c, sz_c)                                           \
    do {                                                                    \
        pchvml_buffer_character (hvml, c, sz_c);                            \
    } while (false)

#define RESET_TEMP_BUFFER()                                                 \
    do {                                                                    \
        pchvml_temp_buffer_reset (hvml->temp_buffer);                       \
    } while (false)

#define APPEND_TEMP_BUFFER(c, sz_c)                                         \
    do {                                                                    \
        pchvml_temp_buffer_append (hvml->temp_buffer, c, sz_c);             \
    } while (false)

static const char* hvml_err_msgs[] = {
};

static struct err_msg_seg _hvml_err_msgs_seg = {
    { NULL, NULL },
    PURC_ERROR_FIRST_HVML,
    PURC_ERROR_FIRST_HVML + PCA_TABLESIZE(hvml_err_msgs) - 1,
    hvml_err_msgs
};

static inline bool is_whitespace (wchar_t character)
{
    return character == ' ' || character == '\x0A' ||
        character == '\x09' || character == '\x0C';
}

static inline wchar_t to_ascii_lower_unchecked (wchar_t character)
{
        return character | 0x20;
}

static inline UNUSED_FUNCTION bool is_ascii (wchar_t character)
{
    return !(character & ~0x7F);
}

static inline UNUSED_FUNCTION bool is_ascii_lower (wchar_t character)
{
    return character >= 'a' && character <= 'z';
}

static inline UNUSED_FUNCTION bool is_ascii_upper (wchar_t character)
{
     return character >= 'A' && character <= 'Z';
}

static inline UNUSED_FUNCTION bool is_ascii_space (wchar_t character)
{
    return character <= ' ' &&
        (character == ' ' || (character <= 0xD && character >= 0x9));
}

static inline UNUSED_FUNCTION bool is_ascii_digit (wchar_t character)
{
    return character >= '0' && character <= '9';
}

static inline UNUSED_FUNCTION bool is_ascii_binary_digit (wchar_t character)
{
     return character == '0' || character == '1';
}

static inline UNUSED_FUNCTION bool is_ascii_hex_digit (wchar_t character)
{
     return is_ascii_digit(character) ||
         (to_ascii_lower_unchecked(character) >= 'a' &&
          to_ascii_lower_unchecked(character) <= 'f');
}

static inline UNUSED_FUNCTION bool is_ascii_octal_digit (wchar_t character)
{
     return character >= '0' && character <= '7';
}

static inline UNUSED_FUNCTION bool is_ascii_alpha (wchar_t character)
{
    return is_ascii_lower(to_ascii_lower_unchecked(character));
}

static inline UNUSED_FUNCTION bool is_ascii_alpha_numeric (wchar_t character)
{
    return is_ascii_digit(character) || is_ascii_alpha(character);
}


void pchvml_init_once(void)
{
    pcinst_register_error_message_segment(&_hvml_err_msgs_seg);
}

struct pchvml_parser* pchvml_create(uint32_t flags, size_t queue_size)
{
    struct pchvml_parser* parser = (struct pchvml_parser*) HVML_ALLOC(
            sizeof(struct pchvml_parser));
    parser->state = HVML_DATA_STATE;
    parser->flags = flags;
    parser->queue_size = queue_size;
    parser->temp_buffer = pchvml_temp_buffer_new ();
    return parser;
}

void pchvml_reset(struct pchvml_parser* parser, uint32_t flags,
        size_t queue_size)
{
    parser->state = HVML_DATA_STATE;
    parser->flags = flags;
    parser->queue_size = queue_size;
    pchvml_temp_buffer_reset (parser->temp_buffer);
}

void pchvml_destroy(struct pchvml_parser* parser)
{
    if (parser) {
        pchvml_temp_buffer_destroy (parser->temp_buffer);
        HVML_FREE(parser);
    }
}

const char* pchvml_pchvml_state_desc (enum pchvml_state state)
{
    switch (state) {
        STATE_DESC(HVML_DATA_STATE)
        STATE_DESC(HVML_RCDATA_STATE)
        STATE_DESC(HVML_RAWTEXT_STATE)
        STATE_DESC(HVML_PLAINTEXT_STATE)
        STATE_DESC(HVML_TAG_OPEN_STATE)
        STATE_DESC(HVML_END_TAG_OPEN_STATE)
        STATE_DESC(HVML_TAG_NAME_STATE)
        STATE_DESC(HVML_RCDATA_LESS_THAN_SIGN_STATE)
        STATE_DESC(HVML_RCDATA_END_TAG_OPEN_STATE)
        STATE_DESC(HVML_RCDATA_END_TAG_NAME_STATE)
        STATE_DESC(HVML_RAWTEXT_LESS_THAN_SIGN_STATE)
        STATE_DESC(HVML_RAWTEXT_END_TAG_OPEN_STATE)
        STATE_DESC(HVML_RAWTEXT_END_TAG_NAME_STATE)
        STATE_DESC(HVML_BEFORE_ATTRIBUTE_NAME_STATE)
        STATE_DESC(HVML_ATTRIBUTE_NAME_STATE)
        STATE_DESC(HVML_AFTER_ATTRIBUTE_NAME_STATE)
        STATE_DESC(HVML_BEFORE_ATTRIBUTE_VALUE_STATE)
        STATE_DESC(HVML_ATTRIBUTE_VALUE_DOUBLE_QUOTED_STATE)
        STATE_DESC(HVML_ATTRIBUTE_VALUE_SINGLE_QUOTED_STATE)
        STATE_DESC(HVML_ATTRIBUTE_VALUE_UNQUOTED_STATE)
        STATE_DESC(HVML_AFTER_ATTRIBUTE_VALUE_QUOTED_STATE)
        STATE_DESC(HVML_SELF_CLOSING_START_TAG_STATE)
        STATE_DESC(HVML_BOGUS_COMMENT_STATE)
        STATE_DESC(HVML_MARKUP_DECLARATION_OPEN_STATE)
        STATE_DESC(HVML_COMMENT_START_STATE)
        STATE_DESC(HVML_COMMENT_START_DASH_STATE)
        STATE_DESC(HVML_COMMENT_STATE)
        STATE_DESC(HVML_COMMENT_LESS_THAN_SIGN_STATE)
        STATE_DESC(HVML_COMMENT_LESS_THAN_SIGN_BANG_STATE)
        STATE_DESC(HVML_COMMENT_LESS_THAN_SIGN_BANG_DASH_STATE)
        STATE_DESC(HVML_COMMENT_LESS_THAN_SIGN_BANG_DASH_DASH_STATE)
        STATE_DESC(HVML_COMMENT_END_DASH_STATE)
        STATE_DESC(HVML_COMMENT_END_STATE)
        STATE_DESC(HVML_COMMENT_END_BANG_STATE)
        STATE_DESC(HVML_DOCTYPE_STATE)
        STATE_DESC(HVML_BEFORE_DOCTYPE_NAME_STATE)
        STATE_DESC(HVML_DOCTYPE_NAME_STATE)
        STATE_DESC(HVML_AFTER_DOCTYPE_NAME_STATE)
        STATE_DESC(HVML_AFTER_DOCTYPE_PUBLIC_KEYWORD_STATE)
        STATE_DESC(HVML_BEFORE_DOCTYPE_PUBLIC_IDENTIFIER_STATE)
        STATE_DESC(HVML_DOCTYPE_PUBLIC_IDENTIFIER_DOUBLE_QUOTED_STATE)
        STATE_DESC(HVML_DOCTYPE_PUBLIC_IDENTIFIER_SINGLE_QUOTED_STATE)
        STATE_DESC(HVML_AFTER_DOCTYPE_PUBLIC_IDENTIFIER_STATE)
        STATE_DESC(HVML_BETWEEN_DOCTYPE_PUBLIC_IDENTIFIER_AND_SYSTEM_INFORMATION_STATE)
        STATE_DESC(HVML_AFTER_DOCTYPE_SYSTEM_KEYWORD_STATE)
        STATE_DESC(HVML_BEFORE_DOCTYPE_SYSTEM_INFORMATION_STATE)
        STATE_DESC(HVML_DOCTYPE_SYSTEM_INFORMATION_DOUBLE_QUOTED_STATE)
        STATE_DESC(HVML_DOCTYPE_SYSTEM_INFORMATION_SINGLE_QUOTED_STATE)
        STATE_DESC(HVML_AFTER_DOCTYPE_SYSTEM_INFORMATION_STATE)
        STATE_DESC(HVML_BOGUS_DOCTYPE_STATE)
        STATE_DESC(HVML_CDATA_SECTION_STATE)
        STATE_DESC(HVML_CDATA_SECTION_BRACKET_STATE)
        STATE_DESC(HVML_CDATA_SECTION_END_STATE)
        STATE_DESC(HVML_CHARACTER_REFERENCE_STATE)
        STATE_DESC(HVML_NAMED_CHARACTER_REFERENCE_STATE)
        STATE_DESC(HVML_AMBIGUOUS_AMPERSAND_STATE)
        STATE_DESC(HVML_NUMERIC_CHARACTER_REFERENCE_STATE)
        STATE_DESC(HVML_HEXADECIMAL_CHARACTER_REFERENCE_START_STATE)
        STATE_DESC(HVML_DECIMAL_CHARACTER_REFERENCE_START_STATE)
        STATE_DESC(HVML_HEXADECIMAL_CHARACTER_REFERENCE_STATE)
        STATE_DESC(HVML_DECIMAL_CHARACTER_REFERENCE_STATE)
        STATE_DESC(HVML_NUMERIC_CHARACTER_REFERENCE_END_STATE)
        STATE_DESC(HVML_SPECIAL_ATTRIBUTE_OPERATOR_IN_ATTRIBUTE_NAME_STATE)
        STATE_DESC(HVML_SPECIAL_ATTRIBUTE_OPERATOR_AFTER_ATTRIBUTE_NAME_STATE)
        STATE_DESC(HVML_EJSON_DATA_STATE)
        STATE_DESC(HVML_EJSON_FINISHED_STATE)
        STATE_DESC(HVML_EJSON_CONTROL_STATE)
        STATE_DESC(HVML_EJSON_LEFT_BRACE_STATE)
        STATE_DESC(HVML_EJSON_RIGHT_BRACE_STATE)
        STATE_DESC(HVML_EJSON_LEFT_BRACKET_STATE)
        STATE_DESC(HVML_EJSON_RIGHT_BRACKET_STATE)
        STATE_DESC(HVML_EJSON_LESS_THAN_SIGN_STATE)
        STATE_DESC(HVML_EJSON_GREATER_THAN_SIGN_STATE)
        STATE_DESC(HVML_EJSON_LEFT_PARENTHESIS_STATE)
        STATE_DESC(HVML_EJSON_RIGHT_PARENTHESIS_STATE)
        STATE_DESC(HVML_EJSON_DOLLAR_STATE)
        STATE_DESC(HVML_EJSON_AFTER_VALUE_STATE)
        STATE_DESC(HVML_EJSON_BEFORE_NAME_STATE)
        STATE_DESC(HVML_EJSON_AFTER_NAME_STATE)
        STATE_DESC(HVML_EJSON_NAME_UNQUOTED_STATE)
        STATE_DESC(HVML_EJSON_NAME_SINGLE_QUOTED_STATE)
        STATE_DESC(HVML_EJSON_NAME_DOUBLE_QUOTED_STATE)
        STATE_DESC(HVML_EJSON_VALUE_SINGLE_QUOTED_STATE)
        STATE_DESC(HVML_EJSON_VALUE_DOUBLE_QUOTED_STATE)
        STATE_DESC(HVML_EJSON_AFTER_VALUE_DOUBLE_QUOTED_STATE)
        STATE_DESC(HVML_EJSON_VALUE_TWO_DOUBLE_QUOTED_STATE)
        STATE_DESC(HVML_EJSON_VALUE_THREE_DOUBLE_QUOTED_STATE)
        STATE_DESC(HVML_EJSON_KEYWORD_STATE)
        STATE_DESC(HVML_EJSON_AFTER_KEYWORD_STATE)
        STATE_DESC(HVML_EJSON_BYTE_SEQUENCE_STATE)
        STATE_DESC(HVML_EJSON_AFTER_BYTE_SEQUENCE_STATE)
        STATE_DESC(HVML_EJSON_HEX_BYTE_SEQUENCE_STATE)
        STATE_DESC(HVML_EJSON_BINARY_BYTE_SEQUENCE_STATE)
        STATE_DESC(HVML_EJSON_BASE64_BYTE_SEQUENCE_STATE)
        STATE_DESC(HVML_EJSON_VALUE_NUMBER_STATE)
        STATE_DESC(HVML_EJSON_AFTER_VALUE_NUMBER_STATE)
        STATE_DESC(HVML_EJSON_VALUE_NUMBER_INTEGER_STATE)
        STATE_DESC(HVML_EJSON_VALUE_NUMBER_FRACTION_STATE)
        STATE_DESC(HVML_EJSON_VALUE_NUMBER_EXPONENT_STATE)
        STATE_DESC(HVML_EJSON_VALUE_NUMBER_EXPONENT_INTEGER_STATE)
        STATE_DESC(HVML_EJSON_VALUE_NUMBER_SUFFIX_INTEGER_STATE)
        STATE_DESC(HVML_EJSON_VALUE_NUMBER_INFINITY_STATE)
        STATE_DESC(HVML_EJSON_VALUE_NAN_STATE)
        STATE_DESC(HVML_EJSON_STRING_ESCAPE_STATE)
        STATE_DESC(HVML_EJSON_STRING_ESCAPE_FOUR_HEXADECIMAL_DIGITS_STATE)
        STATE_DESC(HVML_EJSON_JSONEE_VARIABLE_STATE)
        STATE_DESC(HVML_EJSON_JSONEE_FULL_STOP_SIGN_STATE)
        STATE_DESC(HVML_EJSON_JSONEE_KEYWORD_STATE)
        STATE_DESC(HVML_EJSON_JSONEE_STRING_STATE)
        STATE_DESC(HVML_EJSON_AFTER_JSONEE_STRING_STATE)
    }
    return NULL;
}

bool pchvml_have_buffered_character_token(struct pchvml_parser* hvml)
{
    return hvml->current_token && hvml->current_token->type == HVML_TOKEN_CHARACTER;
}

void pchvml_buffer_character(struct pchvml_parser* hvml, const char* bytes,
        size_t nr_bytes)
{
    if (hvml->current_token == NULL) {
        hvml->current_token = pchvml_token_new (HVML_TOKEN_CHARACTER);
    }
    pchvml_token_append(hvml->current_token, bytes, nr_bytes);
}

struct pchvml_token* pchvml_next_token (struct pchvml_parser* hvml,
                                          purc_rwstream_t rws)
{
next_input:
    if (!hvml->need_reconsume) {
        hvml->sz_c = purc_rwstream_read_utf8_char (rws,
                hvml->c, &hvml->wc);
        if (hvml->sz_c <= 0) {
            return NULL;
        }
    }
    hvml->need_reconsume = false;

    wchar_t character = character;

next_state:
    switch (hvml->state) {
        BEGIN_STATE(HVML_DATA_STATE)
            if (character == '&') {
                ADVANCE_TO(HVML_CHARACTER_REFERENCE_STATE);
            }
            if (character == '<') {
                if (pchvml_have_buffered_character_token(hvml)) {
                    RETURN_IN_CURRENT_STATE(true);
                }
                ADVANCE_TO(HVML_TAG_OPEN_STATE);
            }
            if (character == HVML_END_OF_FILE) {
                return pchvml_token_new(HVML_TOKEN_EOF);
            }
            BUFFER_CHARACTER(hvml->c, hvml->sz_c);
            ADVANCE_TO(HVML_DATA_STATE);
        END_STATE()

        BEGIN_STATE(HVML_RCDATA_STATE)
            if (character == '&') {
                hvml->return_state = HVML_RCDATA_STATE;
                ADVANCE_TO(HVML_CHARACTER_REFERENCE_STATE);
            }
            if (character == '<') {
                ADVANCE_TO(HVML_RCDATA_LESS_THAN_SIGN_STATE);
            }
            if (character == HVML_END_OF_FILE) {
                RECONSUME_IN(HVML_DATA_STATE);
            }
            BUFFER_CHARACTER(hvml->c, hvml->sz_c);
            ADVANCE_TO(HVML_TOKEN_CHARACTER);
        END_STATE()

        BEGIN_STATE(HVML_RAWTEXT_STATE)
            if (character == '<') {
                ADVANCE_TO(HVML_RAWTEXT_LESS_THAN_SIGN_STATE);
            }
            if (character == HVML_END_OF_FILE) {
                RECONSUME_IN (HVML_DATA_STATE);
            }

            BUFFER_CHARACTER(hvml->c, hvml->sz_c);
            ADVANCE_TO(HVML_RAWTEXT_STATE);
        END_STATE()

        BEGIN_STATE(HVML_PLAINTEXT_STATE)
            if (character == HVML_END_OF_FILE) {
                return pchvml_token_new(HVML_TOKEN_EOF);
            }

            BUFFER_CHARACTER(hvml->c, hvml->sz_c);
            ADVANCE_TO(HVML_PLAINTEXT_STATE);
        END_STATE()

        BEGIN_STATE(HVML_TAG_OPEN_STATE)
            if (character == '!') {
                ADVANCE_TO(HVML_MARKUP_DECLARATION_OPEN_STATE);
            }
            if (character == '/') {
                ADVANCE_TO(HVML_END_TAG_OPEN_STATE);
            }
            if (is_ascii_alpha(character)) {
                hvml->current_token = pchvml_token_new_start_tag ();
                ADVANCE_TO(HVML_TAG_NAME_STATE);
            }
            if (character == '?') {
                RECONSUME_IN(HVML_BOGUS_COMMENT_STATE);
            }
            BUFFER_CHARACTER("<", 1);
            RECONSUME_IN(HVML_DATA_STATE);
        END_STATE()

        BEGIN_STATE(HVML_END_TAG_OPEN_STATE)
            if (is_ascii_alpha(character)) {
                hvml->current_token = pchvml_token_new_end_tag();
                ADVANCE_TO(HVML_TAG_NAME_STATE);
            }
            if (character == '>') {
                ADVANCE_TO(HVML_DATA_STATE);
            }
            if (character == HVML_END_OF_FILE) {
                BUFFER_CHARACTER("<", 1);
                BUFFER_CHARACTER("/", 1);
                RECONSUME_IN(HVML_DATA_STATE);
            }
            RECONSUME_IN(HVML_BOGUS_COMMENT_STATE);
        END_STATE()

        BEGIN_STATE(HVML_TAG_NAME_STATE)
            if (is_whitespace(character))
                ADVANCE_TO(HVML_BEFORE_ATTRIBUTE_NAME_STATE);
            if (character == '/') {
                ADVANCE_TO(HVML_SELF_CLOSING_START_TAG_STATE);
            }
            if (character == '>') {
                RETURN_AND_SWITCH_TO(HVML_DATA_STATE);
            }
            if (character == '<') {
                RETURN_AND_RECONSUME_IN(HVML_DATA_STATE);
            }
            if (character == HVML_END_OF_FILE) {
                RECONSUME_IN(HVML_DATA_STATE);
            }
            BUFFER_CHARACTER(hvml->c, hvml->sz_c);
            ADVANCE_TO(HVML_TAG_NAME_STATE);
        END_STATE()

        BEGIN_STATE(HVML_RCDATA_LESS_THAN_SIGN_STATE)
            if (character == '/') {
                RESET_TEMP_BUFFER();
                ADVANCE_TO(HVML_RCDATA_END_TAG_OPEN_STATE);
            }
            BUFFER_CHARACTER("<", 1);
            RECONSUME_IN(HVML_RCDATA_STATE);
        END_STATE()

        BEGIN_STATE(HVML_RCDATA_END_TAG_OPEN_STATE)
            if (is_ascii_alpha(character)) {
                APPEND_TEMP_BUFFER(hvml->c, hvml->sz_c);
                // TODO : append to possible end tag
                ADVANCE_TO(HVML_RCDATA_END_TAG_NAME_STATE);
            }
            BUFFER_CHARACTER("<", 1);
            BUFFER_CHARACTER("/", 1);
            RECONSUME_IN(HVML_RCDATA_STATE);
        END_STATE()

        BEGIN_STATE(HVML_RCDATA_END_TAG_NAME_STATE)
        // TODO
        END_STATE()

        BEGIN_STATE(HVML_RAWTEXT_LESS_THAN_SIGN_STATE)
            if (character == '/') {
                RESET_TEMP_BUFFER();
                ADVANCE_TO(HVML_RAWTEXT_END_TAG_OPEN_STATE);
            }
            BUFFER_CHARACTER("<", 1);
            RECONSUME_IN(HVML_RAWTEXT_STATE);
        END_STATE()

        BEGIN_STATE(HVML_RAWTEXT_END_TAG_OPEN_STATE)
            if (is_ascii_alpha(character)) {
                APPEND_TEMP_BUFFER(hvml->c, hvml->sz_c);
                // TODO : append to possible end tag
                ADVANCE_TO(HVML_RAWTEXT_END_TAG_NAME_STATE);
            }
            BUFFER_CHARACTER("<", 1);
            BUFFER_CHARACTER("/", 1);
            RECONSUME_IN(HVML_RAWTEXT_STATE);
        END_STATE()

        BEGIN_STATE(HVML_RAWTEXT_END_TAG_NAME_STATE)
            // TODO
        END_STATE()

        BEGIN_STATE(HVML_BEFORE_ATTRIBUTE_NAME_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(HVML_BEFORE_ATTRIBUTE_NAME_STATE);
            }
            if (character == '/') {
                ADVANCE_TO(HVML_SELF_CLOSING_START_TAG_STATE);
            }
            if (character == '>') {
                SWITCH_TO(HVML_DATA_STATE);
                return hvml->current_token;
            }
            if (character == '<') {
                SWITCH_TO(HVML_DATA_STATE);
                return hvml->current_token;
            }
            if (character == HVML_END_OF_FILE) {
                RECONSUME_IN(HVML_DATA_STATE);
            }
            pchvml_token_attribute_begin (hvml->current_token);
            pchvml_token_attribute_append_to_name (
                    hvml->current_token, hvml->c, hvml->sz_c);
            ADVANCE_TO(HVML_ATTRIBUTE_NAME_STATE);
        END_STATE()

        BEGIN_STATE(HVML_ATTRIBUTE_NAME_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(HVML_AFTER_ATTRIBUTE_NAME_STATE);
            }
            if (character == '/') {
                ADVANCE_TO(HVML_SELF_CLOSING_START_TAG_STATE);
            }
            if (character == '$' || character == '%' || character == '+'
                    || character == '-' || character == '^'
                    || character == '~') {
                if (hvml->current_token->type == HVML_TOKEN_START_TAG) {
                    // TODO : check attribute is an ordinary attribute name
                    RESET_TEMP_BUFFER();
                    BUFFER_CHARACTER(hvml->c, hvml->sz_c);
                    SWITCH_TO(
                            HVML_SPECIAL_ATTRIBUTE_OPERATOR_IN_ATTRIBUTE_NAME_STATE);
                }
            }
            if (character == '=') {
                ADVANCE_TO(HVML_BEFORE_ATTRIBUTE_VALUE_STATE);
            }
            if (character == '>') {
                RETURN_AND_SWITCH_TO(HVML_DATA_STATE);
            }
            if (character == '<') {
                RETURN_AND_RECONSUME_IN(HVML_DATA_STATE);
            }
            if (character == HVML_END_OF_FILE) {
                RECONSUME_IN(HVML_DATA_STATE);
            }
            pchvml_token_attribute_append_to_name (
                    hvml->current_token, hvml->c, hvml->sz_c);
            ADVANCE_TO(HVML_ATTRIBUTE_NAME_STATE);
        END_STATE()

        BEGIN_STATE(HVML_AFTER_ATTRIBUTE_NAME_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(HVML_AFTER_ATTRIBUTE_NAME_STATE);
            }
            if (character == '/') {
                ADVANCE_TO(HVML_SELF_CLOSING_START_TAG_STATE);
            }
            if (character == '$' || character == '%' || character == '+'
                    || character == '-' || character == '^'
                    || character == '~') {
#if 0
                if () {
                    // TODO : check token is an operation tag 
                    // TODO : check attribute is an ordinary attribute name
                    RESET_TEMP_BUFFER();
                    BUFFER_CHARACTER(hvml->c, hvml->sz_c);
                    SWITCH_TO(
                    HVML_SPECIAL_ATTRIBUTE_OPERATOR_AFTER_ATTRIBUTE_NAME_STATE);
                }
#endif
            }
            if (character == '=') {
                ADVANCE_TO(HVML_BEFORE_ATTRIBUTE_VALUE_STATE);
            }
            if (character == '>') {
                RETURN_AND_SWITCH_TO(HVML_DATA_STATE);
            }
            if (character == '<') {
                RETURN_AND_RECONSUME_IN(HVML_DATA_STATE);
            }
            if (character == HVML_END_OF_FILE) {
                RECONSUME_IN(HVML_DATA_STATE);
            }
            pchvml_token_attribute_begin (hvml->current_token);
            pchvml_token_attribute_append_to_name (
                    hvml->current_token, hvml->c, hvml->sz_c);
            ADVANCE_TO(HVML_ATTRIBUTE_NAME_STATE);
        END_STATE()

        BEGIN_STATE(HVML_BEFORE_ATTRIBUTE_VALUE_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(HVML_BEFORE_ATTRIBUTE_VALUE_STATE);
            }
            if (character == '"') {
                ADVANCE_TO(HVML_ATTRIBUTE_VALUE_DOUBLE_QUOTED_STATE);
            }
            if (character == '&') {
                RECONSUME_IN(HVML_ATTRIBUTE_VALUE_UNQUOTED_STATE);
            }
            if (character == '\'') {
                ADVANCE_TO(HVML_ATTRIBUTE_VALUE_SINGLE_QUOTED_STATE);
            }
            if (character == '>') {
                SWITCH_TO(HVML_DATA_STATE);
                return hvml->current_token;
            }
            if (character == '$') {
                // TODO concat-string and so on
                RECONSUME_IN(HVML_EJSON_DATA_STATE);
            }
            if (character == HVML_END_OF_FILE) {
                RECONSUME_IN(HVML_DATA_STATE);
            }
            BUFFER_CHARACTER(hvml->c, hvml->sz_c);
            ADVANCE_TO(HVML_ATTRIBUTE_VALUE_UNQUOTED_STATE);
        END_STATE()

        BEGIN_STATE(HVML_ATTRIBUTE_VALUE_DOUBLE_QUOTED_STATE)
            if (character == '"') {
                pchvml_token_attribute_end(hvml->current_token);
                ADVANCE_TO(HVML_AFTER_ATTRIBUTE_VALUE_QUOTED_STATE);
            }
            if (character == '&') {
                ADVANCE_TO(HVML_CHARACTER_REFERENCE_STATE);
            }
            if (character == HVML_END_OF_FILE) {
                pchvml_token_attribute_end(hvml->current_token);
                RECONSUME_IN(HVML_DATA_STATE);
            }
            BUFFER_CHARACTER(hvml->c, hvml->sz_c);
            ADVANCE_TO(HVML_ATTRIBUTE_VALUE_UNQUOTED_STATE);
        END_STATE()

        BEGIN_STATE(HVML_ATTRIBUTE_VALUE_SINGLE_QUOTED_STATE)
            if (character == '\'') {
                pchvml_token_attribute_end(hvml->current_token);
                ADVANCE_TO(HVML_AFTER_ATTRIBUTE_VALUE_QUOTED_STATE);
            }
            if (character == '&') {
                hvml->return_state = HVML_ATTRIBUTE_VALUE_SINGLE_QUOTED_STATE;
                ADVANCE_TO(HVML_CHARACTER_REFERENCE_STATE);
            }
            if (character == HVML_END_OF_FILE) {
                pchvml_token_attribute_end(hvml->current_token);
                RECONSUME_IN(HVML_DATA_STATE);
            }
            BUFFER_CHARACTER(hvml->c, hvml->sz_c);
            ADVANCE_TO(HVML_ATTRIBUTE_VALUE_SINGLE_QUOTED_STATE);
        END_STATE()

        BEGIN_STATE(HVML_ATTRIBUTE_VALUE_UNQUOTED_STATE)
            if (is_whitespace(character)) {
                pchvml_token_attribute_end(hvml->current_token);
                ADVANCE_TO(HVML_BEFORE_ATTRIBUTE_NAME_STATE);
            }
            if (character == '&') {
                hvml->return_state = HVML_ATTRIBUTE_VALUE_UNQUOTED_STATE;
                ADVANCE_TO(HVML_CHARACTER_REFERENCE_STATE);
            }
            if (character == '>') {
                pchvml_token_attribute_end(hvml->current_token);
                SWITCH_TO(HVML_DATA_STATE);
                return hvml->current_token;
            }
            if (character == '$') {
                // TODO concat-string and so on
                RECONSUME_IN(HVML_EJSON_DATA_STATE);
            }
            if (character == HVML_END_OF_FILE) {
                pchvml_token_attribute_end(hvml->current_token);
                RECONSUME_IN(HVML_DATA_STATE);
            }
            BUFFER_CHARACTER(hvml->c, hvml->sz_c);
            ADVANCE_TO(HVML_ATTRIBUTE_VALUE_UNQUOTED_STATE);
        END_STATE()

        BEGIN_STATE(HVML_AFTER_ATTRIBUTE_VALUE_QUOTED_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(HVML_BEFORE_ATTRIBUTE_NAME_STATE);
            }
            if (character == '/') {
                ADVANCE_TO(HVML_SELF_CLOSING_START_TAG_STATE);
            }
            if (character == '>') {
                SWITCH_TO(HVML_DATA_STATE);
                return hvml->current_token;
            }
            if (character == '<') {
                SWITCH_TO(HVML_DATA_STATE);
                return hvml->current_token;
            }
            if (character == HVML_END_OF_FILE) {
                RECONSUME_IN(HVML_DATA_STATE);
            }
            RECONSUME_IN(HVML_BEFORE_ATTRIBUTE_NAME_STATE);
        END_STATE()

        BEGIN_STATE(HVML_SELF_CLOSING_START_TAG_STATE)
            if (character == '>') {
                // TODO : mark current token self close
                SWITCH_TO(HVML_DATA_STATE);
                return hvml->current_token;
            }
            if (character == HVML_END_OF_FILE) {
                RECONSUME_IN(HVML_DATA_STATE);
            }
            RECONSUME_IN(HVML_BEFORE_ATTRIBUTE_NAME_STATE);
        END_STATE()

        BEGIN_STATE(HVML_BOGUS_COMMENT_STATE)
            if (hvml->current_token->type != HVML_TOKEN_COMMENT) {
                hvml->current_token = pchvml_token_new(HVML_TOKEN_COMMENT);
            }
            if (character == '>') {
                SWITCH_TO(HVML_DATA_STATE);
                return hvml->current_token;
            }
            if (character == HVML_END_OF_FILE) {
                RECONSUME_IN_NEXT(HVML_DATA_STATE);
                return hvml->current_token;
            }
            BUFFER_CHARACTER(hvml->c, hvml->sz_c);
            ADVANCE_TO(HVML_BOGUS_COMMENT_STATE);
        END_STATE()

        BEGIN_STATE(HVML_MARKUP_DECLARATION_OPEN_STATE)
            //TODO
        END_STATE()

        BEGIN_STATE(HVML_COMMENT_START_STATE)
            if (character == '-') {
                ADVANCE_TO(HVML_COMMENT_START_DASH_STATE);
            }
            if (character == '>') {
                RECONSUME_IN_NEXT(HVML_DATA_STATE);
                return hvml->current_token;
            }
            if (character == HVML_END_OF_FILE) {
                RECONSUME_IN_NEXT(HVML_DATA_STATE);
                return hvml->current_token;
            }
            BUFFER_CHARACTER(hvml->c, hvml->sz_c);
            ADVANCE_TO(HVML_COMMENT_STATE);
        END_STATE()

        BEGIN_STATE(HVML_COMMENT_START_DASH_STATE)
            if (character == '-') {
                ADVANCE_TO(HVML_COMMENT_END_STATE);
            }
            if (character == '>') {
                RECONSUME_IN_NEXT(HVML_DATA_STATE);
                return hvml->current_token;
            }
            if (character == HVML_END_OF_FILE) {
                RECONSUME_IN_NEXT(HVML_DATA_STATE);
                return hvml->current_token;
            }
            BUFFER_CHARACTER("-", 1);
            BUFFER_CHARACTER(hvml->c, hvml->sz_c);
            ADVANCE_TO(HVML_COMMENT_STATE);
        END_STATE()

        BEGIN_STATE(HVML_COMMENT_STATE)
            // TODO : compare to doc
            if (character == '-') {
                ADVANCE_TO(HVML_COMMENT_END_DASH_STATE);
            }
            if (character == HVML_END_OF_FILE) {
                RECONSUME_IN_NEXT(HVML_DATA_STATE);
                return hvml->current_token;
            }
            BUFFER_CHARACTER(hvml->c, hvml->sz_c);
            ADVANCE_TO(HVML_COMMENT_STATE);
        END_STATE()

        BEGIN_STATE(HVML_COMMENT_LESS_THAN_SIGN_STATE)
            // TODO remove
        END_STATE()

        BEGIN_STATE(HVML_COMMENT_LESS_THAN_SIGN_BANG_STATE)
            // TODO remove
        END_STATE()

        BEGIN_STATE(HVML_COMMENT_LESS_THAN_SIGN_BANG_DASH_STATE)
            // TODO remove
        END_STATE()

        BEGIN_STATE(HVML_COMMENT_LESS_THAN_SIGN_BANG_DASH_DASH_STATE)
            // TODO remove
        END_STATE()

        BEGIN_STATE(HVML_COMMENT_END_DASH_STATE)
            if (character == '-') {
                ADVANCE_TO(HVML_COMMENT_END_STATE);
            }
            if (character == HVML_END_OF_FILE) {
                RECONSUME_IN_NEXT(HVML_DATA_STATE);
                return hvml->current_token;
            }
            BUFFER_CHARACTER("-", 1);
            ADVANCE_TO(HVML_COMMENT_STATE);
        END_STATE()

        BEGIN_STATE(HVML_COMMENT_END_STATE)
            if (character == '>') {
                RECONSUME_IN_NEXT(HVML_DATA_STATE);
                return hvml->current_token;
            }
            if (character == '!') {
                ADVANCE_TO(HVML_COMMENT_END_BANG_STATE);
            }
            if (character == '-') {
                BUFFER_CHARACTER("-", 1);
                ADVANCE_TO(HVML_COMMENT_END_STATE);
            }
            if (character == HVML_END_OF_FILE) {
                RECONSUME_IN_NEXT(HVML_DATA_STATE);
                return hvml->current_token;
            }
            BUFFER_CHARACTER("-", 1);
            BUFFER_CHARACTER("-", 1);
            ADVANCE_TO(HVML_COMMENT_STATE);
        END_STATE()

        BEGIN_STATE(HVML_COMMENT_END_BANG_STATE)
            if (character == '-') {
                BUFFER_CHARACTER("-", 1);
                BUFFER_CHARACTER("-", 1);
                BUFFER_CHARACTER("!", 1);
                ADVANCE_TO(HVML_COMMENT_END_DASH_STATE);
            }
            if (character == '>') {
                RECONSUME_IN_NEXT(HVML_DATA_STATE);
                return hvml->current_token;
            }
            if (character == HVML_END_OF_FILE) {
                RECONSUME_IN_NEXT(HVML_DATA_STATE);
                return hvml->current_token;
            }
            BUFFER_CHARACTER("-", 1);
            BUFFER_CHARACTER("-", 1);
            BUFFER_CHARACTER("!", 1);
            ADVANCE_TO(HVML_COMMENT_STATE);
        END_STATE()

        BEGIN_STATE(HVML_DOCTYPE_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(HVML_BEFORE_DOCTYPE_NAME_STATE);
            }
            if (character == HVML_END_OF_FILE) {
                hvml->current_token = pchvml_token_new(HVML_TOKEN_DOCTYPE);
                RETURN_AND_RECONSUME_IN(HVML_DATA_STATE);
            }
            RECONSUME_IN(HVML_BEFORE_DOCTYPE_NAME_STATE);
        END_STATE()

        BEGIN_STATE(HVML_BEFORE_DOCTYPE_NAME_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(HVML_BEFORE_DOCTYPE_NAME_STATE);
            }
            if (character == '>') {
                hvml->current_token = pchvml_token_new(HVML_TOKEN_DOCTYPE);
                RETURN_AND_SWITCH_TO(HVML_DATA_STATE);
            }
            if (character == HVML_END_OF_FILE) {
                hvml->current_token = pchvml_token_new(HVML_TOKEN_DOCTYPE);
                RETURN_AND_RECONSUME_IN(HVML_DATA_STATE);
            }
            hvml->current_token = pchvml_token_new(HVML_TOKEN_DOCTYPE);
            ADVANCE_TO(HVML_DOCTYPE_NAME_STATE);
        END_STATE()

        BEGIN_STATE(HVML_DOCTYPE_NAME_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(HVML_AFTER_DOCTYPE_NAME_STATE);
            }
            if (character == '>') {
                RETURN_AND_SWITCH_TO(HVML_DATA_STATE);
            }
            if (character == HVML_END_OF_FILE) {
                RETURN_AND_RECONSUME_IN(HVML_DATA_STATE);
            }
            BUFFER_CHARACTER(hvml->c, hvml->sz_c);
            ADVANCE_TO(HVML_DOCTYPE_NAME_STATE);
        END_STATE()

        BEGIN_STATE(HVML_AFTER_DOCTYPE_NAME_STATE)
        // TODO
        END_STATE()

        BEGIN_STATE(HVML_AFTER_DOCTYPE_PUBLIC_KEYWORD_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(HVML_BEFORE_DOCTYPE_PUBLIC_IDENTIFIER_STATE);
            }
            if (character == '"') {
                // setPublicIdentifierToEmptyString();
                ADVANCE_TO(HVML_DOCTYPE_PUBLIC_IDENTIFIER_DOUBLE_QUOTED_STATE);
            }
            if (character == '\'') {
                // setPublicIdentifierToEmptyString();
                ADVANCE_TO(HVML_DOCTYPE_PUBLIC_IDENTIFIER_SINGLE_QUOTED_STATE);
            }
            if (character == '>') {
                RETURN_AND_SWITCH_TO(HVML_DATA_STATE);
            }
            if (character == HVML_END_OF_FILE) {
                RETURN_AND_RECONSUME_IN(HVML_DATA_STATE);
            }
            ADVANCE_TO(HVML_BOGUS_DOCTYPE_STATE);
        END_STATE()

        BEGIN_STATE(HVML_BEFORE_DOCTYPE_PUBLIC_IDENTIFIER_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(HVML_BEFORE_DOCTYPE_PUBLIC_IDENTIFIER_STATE);
            }
            if (character == '"') {
                // setPublicIdentifierToEmptyString();
                ADVANCE_TO(HVML_DOCTYPE_PUBLIC_IDENTIFIER_DOUBLE_QUOTED_STATE);
            }
            if (character == '\'') {
                // setPublicIdentifierToEmptyString();
                ADVANCE_TO(HVML_DOCTYPE_PUBLIC_IDENTIFIER_SINGLE_QUOTED_STATE);
            }
            if (character == '>') {
                RETURN_AND_SWITCH_TO(HVML_DATA_STATE);
            }
            if (character == HVML_END_OF_FILE) {
                RETURN_AND_RECONSUME_IN(HVML_DATA_STATE);
            }
            ADVANCE_TO(HVML_BOGUS_DOCTYPE_STATE);
        END_STATE()

        BEGIN_STATE(HVML_DOCTYPE_PUBLIC_IDENTIFIER_DOUBLE_QUOTED_STATE)
            if (character == '"') {
                ADVANCE_TO(HVML_AFTER_DOCTYPE_PUBLIC_IDENTIFIER_STATE);
            }
            if (character == '>') {
                RETURN_AND_SWITCH_TO(HVML_DATA_STATE);
            }
            if (character == HVML_END_OF_FILE) {
                RETURN_AND_RECONSUME_IN(HVML_DATA_STATE);
            }
            // appendToPublicIdentifier(character);
            ADVANCE_TO(HVML_DOCTYPE_PUBLIC_IDENTIFIER_DOUBLE_QUOTED_STATE);
        END_STATE()

        BEGIN_STATE(HVML_DOCTYPE_PUBLIC_IDENTIFIER_SINGLE_QUOTED_STATE)
            if (character == '\'') {
                ADVANCE_TO(HVML_AFTER_DOCTYPE_PUBLIC_IDENTIFIER_STATE);
            }
            if (character == '>') {
                RETURN_AND_SWITCH_TO(HVML_DATA_STATE);
            }
            if (character == HVML_END_OF_FILE) {
                RETURN_AND_RECONSUME_IN(HVML_DATA_STATE);
            }
            // appendToPublicIdentifier(character);
            ADVANCE_TO(HVML_DOCTYPE_PUBLIC_IDENTIFIER_SINGLE_QUOTED_STATE);
        END_STATE()

        BEGIN_STATE(HVML_AFTER_DOCTYPE_PUBLIC_IDENTIFIER_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(HVML_BETWEEN_DOCTYPE_PUBLIC_IDENTIFIER_AND_SYSTEM_INFORMATION_STATE);
            }
            if (character == '>') {
                RETURN_AND_SWITCH_TO(HVML_DATA_STATE);
            }
            if (character == '"') {
                //TODO setSystemIdentifierToEmptyString();
                ADVANCE_TO(HVML_DOCTYPE_SYSTEM_INFORMATION_DOUBLE_QUOTED_STATE);
            }
            if (character == '\'') {
                //TODO setSystemIdentifierToEmptyString();
                ADVANCE_TO(HVML_DOCTYPE_SYSTEM_INFORMATION_SINGLE_QUOTED_STATE);
            }
            if (character == HVML_END_OF_FILE) {
                RETURN_AND_RECONSUME_IN(HVML_DATA_STATE);
            }
            ADVANCE_TO(HVML_BOGUS_DOCTYPE_STATE);
        END_STATE()

        BEGIN_STATE(HVML_BETWEEN_DOCTYPE_PUBLIC_IDENTIFIER_AND_SYSTEM_INFORMATION_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(HVML_BETWEEN_DOCTYPE_PUBLIC_IDENTIFIER_AND_SYSTEM_INFORMATION_STATE);
            }
            if (character == '>') {
                RETURN_AND_SWITCH_TO(HVML_DATA_STATE);
            }
            if (character == '"') {
                //TODO setSystemIdentifierToEmptyString();
                ADVANCE_TO(HVML_DOCTYPE_SYSTEM_INFORMATION_DOUBLE_QUOTED_STATE);
            }
            if (character == '\'') {
                //TODO setSystemIdentifierToEmptyString();
                ADVANCE_TO(HVML_DOCTYPE_SYSTEM_INFORMATION_SINGLE_QUOTED_STATE);
            }
            if (character == HVML_END_OF_FILE) {
                RETURN_AND_RECONSUME_IN(HVML_DATA_STATE);
            }
            ADVANCE_TO(HVML_BOGUS_DOCTYPE_STATE);
        END_STATE()

        BEGIN_STATE(HVML_AFTER_DOCTYPE_SYSTEM_KEYWORD_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(HVML_BEFORE_DOCTYPE_SYSTEM_INFORMATION_STATE);
            }
            if (character == '"') {
                // TODO setSystemIdentifierToEmptyString();
                ADVANCE_TO(HVML_DOCTYPE_SYSTEM_INFORMATION_DOUBLE_QUOTED_STATE);
            }
            if (character == '\'') {
                // TODO setSystemIdentifierToEmptyString();
                ADVANCE_TO(HVML_DOCTYPE_SYSTEM_INFORMATION_SINGLE_QUOTED_STATE);
            }
            if (character == '>') {
                RETURN_AND_SWITCH_TO(HVML_DATA_STATE);
            }
            if (character == HVML_END_OF_FILE) {
                RETURN_AND_RECONSUME_IN(HVML_DATA_STATE);
            }
            ADVANCE_TO(HVML_BOGUS_DOCTYPE_STATE);
        END_STATE()

        BEGIN_STATE(HVML_BEFORE_DOCTYPE_SYSTEM_INFORMATION_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(HVML_BEFORE_DOCTYPE_SYSTEM_INFORMATION_STATE);
            }
            if (character == '"') {
                // TODO setSystemIdentifierToEmptyString();
                ADVANCE_TO(HVML_DOCTYPE_SYSTEM_INFORMATION_DOUBLE_QUOTED_STATE);
            }
            if (character == '\'') {
                // TODO setSystemIdentifierToEmptyString();
                ADVANCE_TO(HVML_DOCTYPE_SYSTEM_INFORMATION_SINGLE_QUOTED_STATE);
            }
            if (character == '>') {
                RETURN_AND_SWITCH_TO(HVML_DATA_STATE);
            }
            if (character == HVML_END_OF_FILE) {
                RETURN_AND_RECONSUME_IN(HVML_DATA_STATE);
            }
            ADVANCE_TO(HVML_BOGUS_DOCTYPE_STATE);
        END_STATE()

        BEGIN_STATE(HVML_DOCTYPE_SYSTEM_INFORMATION_DOUBLE_QUOTED_STATE)
            if (character == '"') {
                ADVANCE_TO(HVML_AFTER_DOCTYPE_SYSTEM_INFORMATION_STATE);
            }
            if (character == '>') {
                RETURN_AND_SWITCH_TO(HVML_DATA_STATE);
            }
            if (character == HVML_END_OF_FILE) {
                RETURN_AND_RECONSUME_IN(HVML_DATA_STATE);
            }
            // TODO appendToSystemIdentifier
            ADVANCE_TO(HVML_DOCTYPE_SYSTEM_INFORMATION_DOUBLE_QUOTED_STATE);
        END_STATE()

        BEGIN_STATE(HVML_DOCTYPE_SYSTEM_INFORMATION_SINGLE_QUOTED_STATE)
            if (character == '\'') {
                ADVANCE_TO(HVML_AFTER_DOCTYPE_SYSTEM_INFORMATION_STATE);
            }
            if (character == '>') {
                RETURN_AND_SWITCH_TO(HVML_DATA_STATE);
            }
            if (character == HVML_END_OF_FILE) {
                RETURN_AND_RECONSUME_IN(HVML_DATA_STATE);
            }
            // TODO appendToSystemIdentifier(character);
            ADVANCE_TO(HVML_DOCTYPE_SYSTEM_INFORMATION_SINGLE_QUOTED_STATE);
        END_STATE()

        BEGIN_STATE(HVML_AFTER_DOCTYPE_SYSTEM_INFORMATION_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(HVML_AFTER_DOCTYPE_SYSTEM_INFORMATION_STATE);
            }
            if (character == '>') {
                RETURN_AND_SWITCH_TO(HVML_DATA_STATE);
            }
            if (character == HVML_END_OF_FILE) {
                RETURN_AND_RECONSUME_IN(HVML_DATA_STATE);
            }
            ADVANCE_TO(HVML_BOGUS_DOCTYPE_STATE);
        END_STATE()

        BEGIN_STATE(HVML_BOGUS_DOCTYPE_STATE)
            if (character == '>') {
                RETURN_AND_SWITCH_TO(HVML_DATA_STATE);
            }
            if (character == HVML_END_OF_FILE) {
                RETURN_AND_RECONSUME_IN(HVML_DATA_STATE);
            }
            ADVANCE_TO(HVML_BOGUS_DOCTYPE_STATE);
        END_STATE()

        BEGIN_STATE(HVML_CDATA_SECTION_STATE)
            if (character == ']') {
                ADVANCE_TO(HVML_CDATA_SECTION_BRACKET_STATE);
            }
            if (character == HVML_END_OF_FILE) {
                RECONSUME_IN(HVML_DATA_STATE);
            }
            BUFFER_CHARACTER(hvml->c, hvml->sz_c);
            ADVANCE_TO(HVML_CDATA_SECTION_STATE);
        END_STATE()

        BEGIN_STATE(HVML_CDATA_SECTION_BRACKET_STATE)
            if (character == ']') {
                ADVANCE_TO(HVML_CDATA_SECTION_END_STATE);
            }
            BUFFER_CHARACTER("]", 1);
            RECONSUME_IN(HVML_CDATA_SECTION_STATE);
        END_STATE()

        BEGIN_STATE(HVML_CDATA_SECTION_END_STATE)
            if (character == ']') {
                BUFFER_CHARACTER("]", 1);
                ADVANCE_TO(HVML_CDATA_SECTION_END_STATE);
            }
            if (character == '>') {
                ADVANCE_TO(HVML_DATA_STATE);
            }
            BUFFER_CHARACTER("]", 1);
            RECONSUME_IN(HVML_CDATA_SECTION_STATE);
        END_STATE()

        BEGIN_STATE(HVML_CHARACTER_REFERENCE_STATE)
            if (is_ascii_alpha_numeric(character)) {
                RECONSUME_IN(HVML_NAMED_CHARACTER_REFERENCE_STATE);
            }
            if (character == '#') {
                APPEND_TEMP_BUFFER(hvml->c, hvml->sz_c);
                SWITCH_TO(HVML_NUMERIC_CHARACTER_REFERENCE_STATE);
            }
            RECONSUME_IN(hvml->return_state);
        END_STATE()

        BEGIN_STATE(HVML_NAMED_CHARACTER_REFERENCE_STATE)
        END_STATE()

        BEGIN_STATE(HVML_AMBIGUOUS_AMPERSAND_STATE)
        END_STATE()

        BEGIN_STATE(HVML_NUMERIC_CHARACTER_REFERENCE_STATE)
        END_STATE()

        BEGIN_STATE(HVML_HEXADECIMAL_CHARACTER_REFERENCE_START_STATE)
        END_STATE()

        BEGIN_STATE(HVML_DECIMAL_CHARACTER_REFERENCE_START_STATE)
        END_STATE()

        BEGIN_STATE(HVML_HEXADECIMAL_CHARACTER_REFERENCE_STATE)
        END_STATE()

        BEGIN_STATE(HVML_DECIMAL_CHARACTER_REFERENCE_STATE)
        END_STATE()

        BEGIN_STATE(HVML_NUMERIC_CHARACTER_REFERENCE_END_STATE)
        END_STATE()

        BEGIN_STATE(HVML_SPECIAL_ATTRIBUTE_OPERATOR_IN_ATTRIBUTE_NAME_STATE)
            if (character == '=') {
                if (pchvml_temp_buffer_is_empty(hvml->temp_buffer)) {
                    pchvml_token_attribute_set_assignment(
                            hvml->current_token,
                            HVML_ATTRIBUTE_ASSIGNMENT);
                }
                else {
                    wchar_t op = pchvml_temp_buffer_get_last_char(
                            hvml->temp_buffer);
                    switch (op) {
                        case '+':
                            pchvml_token_attribute_set_assignment(
                                    hvml->current_token,
                                    HVML_ATTRIBUTE_ADDITION_ASSIGNMENT);
                            break;
                        case '-':
                            pchvml_token_attribute_set_assignment(
                                    hvml->current_token,
                                    HVML_ATTRIBUTE_SUBTRACTION_ASSIGNMENT);
                            break;
                        case '%':
                            pchvml_token_attribute_set_assignment(
                                    hvml->current_token,
                                    HVML_ATTRIBUTE_REMAINDER_ASSIGNMENT);
                            break;
                        case '~':
                            pchvml_token_attribute_set_assignment(
                                    hvml->current_token,
                                    HVML_ATTRIBUTE_REPLACE_ASSIGNMENT);
                            break;
                        case '^':
                            pchvml_token_attribute_set_assignment(
                                    hvml->current_token,
                                    HVML_ATTRIBUTE_HEAD_ASSIGNMENT);
                            break;
                        case '$':
                            pchvml_token_attribute_set_assignment(
                                    hvml->current_token,
                                    HVML_ATTRIBUTE_TAIL_ASSIGNMENT);
                            break;
                        default:
                            pchvml_token_attribute_set_assignment(
                                    hvml->current_token,
                                    HVML_ATTRIBUTE_ASSIGNMENT);
                            break;
                    }
                }
                SWITCH_TO(HVML_BEFORE_ATTRIBUTE_VALUE_STATE);
            }
            // TODO
        END_STATE()

        BEGIN_STATE(HVML_SPECIAL_ATTRIBUTE_OPERATOR_AFTER_ATTRIBUTE_NAME_STATE)
            if (character == '=') {
                if (pchvml_temp_buffer_is_empty(hvml->temp_buffer)) {
                    pchvml_token_attribute_set_assignment(
                            hvml->current_token,
                            HVML_ATTRIBUTE_ASSIGNMENT);
                }
                else {
                    wchar_t op = pchvml_temp_buffer_get_last_char(
                            hvml->temp_buffer);
                    switch (op) {
                        case '+':
                            pchvml_token_attribute_set_assignment(
                                    hvml->current_token,
                                    HVML_ATTRIBUTE_ADDITION_ASSIGNMENT);
                            break;
                        case '-':
                            pchvml_token_attribute_set_assignment(
                                    hvml->current_token,
                                    HVML_ATTRIBUTE_SUBTRACTION_ASSIGNMENT);
                            break;
                        case '%':
                            pchvml_token_attribute_set_assignment(
                                    hvml->current_token,
                                    HVML_ATTRIBUTE_REMAINDER_ASSIGNMENT);
                            break;
                        case '~':
                            pchvml_token_attribute_set_assignment(
                                    hvml->current_token,
                                    HVML_ATTRIBUTE_REPLACE_ASSIGNMENT);
                            break;
                        case '^':
                            pchvml_token_attribute_set_assignment(
                                    hvml->current_token,
                                    HVML_ATTRIBUTE_HEAD_ASSIGNMENT);
                            break;
                        case '$':
                            pchvml_token_attribute_set_assignment(
                                    hvml->current_token,
                                    HVML_ATTRIBUTE_TAIL_ASSIGNMENT);
                            break;
                        default:
                            pchvml_token_attribute_set_assignment(
                                    hvml->current_token,
                                    HVML_ATTRIBUTE_ASSIGNMENT);
                            break;
                    }
                }
                SWITCH_TO(HVML_BEFORE_ATTRIBUTE_VALUE_STATE);
            }
            // TODO
        END_STATE()

        BEGIN_STATE(HVML_EJSON_DATA_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_FINISHED_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_CONTROL_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_LEFT_BRACE_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_RIGHT_BRACE_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_LEFT_BRACKET_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_RIGHT_BRACKET_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_LESS_THAN_SIGN_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_GREATER_THAN_SIGN_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_LEFT_PARENTHESIS_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_RIGHT_PARENTHESIS_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_DOLLAR_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_AFTER_VALUE_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_BEFORE_NAME_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_AFTER_NAME_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_NAME_UNQUOTED_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_NAME_SINGLE_QUOTED_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_NAME_DOUBLE_QUOTED_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_VALUE_SINGLE_QUOTED_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_VALUE_DOUBLE_QUOTED_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_AFTER_VALUE_DOUBLE_QUOTED_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_VALUE_TWO_DOUBLE_QUOTED_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_VALUE_THREE_DOUBLE_QUOTED_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_KEYWORD_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_AFTER_KEYWORD_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_BYTE_SEQUENCE_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_AFTER_BYTE_SEQUENCE_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_HEX_BYTE_SEQUENCE_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_BINARY_BYTE_SEQUENCE_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_BASE64_BYTE_SEQUENCE_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_VALUE_NUMBER_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_AFTER_VALUE_NUMBER_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_VALUE_NUMBER_INTEGER_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_VALUE_NUMBER_FRACTION_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_VALUE_NUMBER_EXPONENT_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_VALUE_NUMBER_EXPONENT_INTEGER_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_VALUE_NUMBER_SUFFIX_INTEGER_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_VALUE_NUMBER_INFINITY_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_VALUE_NAN_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_STRING_ESCAPE_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_STRING_ESCAPE_FOUR_HEXADECIMAL_DIGITS_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_JSONEE_VARIABLE_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_JSONEE_FULL_STOP_SIGN_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_JSONEE_KEYWORD_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_JSONEE_STRING_STATE)
        END_STATE()

        BEGIN_STATE(HVML_EJSON_AFTER_JSONEE_STRING_STATE)
        END_STATE()

        default:
            break;
    }
    return NULL;
}

