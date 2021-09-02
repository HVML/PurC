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

#define PCHVML_END_OF_FILE       0

#if HAVE(GLIB)
#define    PCHVML_ALLOC(sz)   g_slice_alloc0(sz)
#define    PCHVML_FREE(p)     g_slice_free1(sizeof(*p), (gpointer)p)
#else
#define    PCHVML_ALLOC(sz)   calloc(1, sz)
#define    PCHVML_FREE(p)     free(p)
#endif

#if 1
#define PRINT_STATE(state_name)
#else
#define PRINT_STATE(state_name)                                             \
    fprintf(stderr, "in %s|wc=%c|hex=%x\n",                                 \
            pchvml_pchvml_state_desc(state_name), hvml->wc, hvml->wc);
#endif

#if 1
#define PCHVML_SET_ERROR(err)    pcinst_set_error(err)
#else
#define PCHVML_SET_ERROR(err)    do {                                       \
    fprintf(stderr, "error %s:%d\n", __FILE__, __LINE__);                   \
    pcinst_set_error (err);                                                 \
} while (0)
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

#define SET_RETURN_STATE(new_state)                                         \
    do {                                                                    \
        hvml->return_state = new_state;                                     \
        goto next_state;                                                    \
    } while (false)

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
            pchvml_parser_save_appropriate_tag_name(hvml);                 \
            pchvml_token_done(hvml->current_token);                        \
            struct pchvml_token* token = hvml->current_token;              \
            return token;                                                  \
        }                                                                  \
        return NULL;                                                       \
    } while (false)

#define RETURN_AND_SWITCH_TO(next_state)                                   \
    do {                                                                   \
        hvml->state = next_state;                                          \
        pchvml_parser_save_appropriate_tag_name(hvml);                     \
        pchvml_token_done(hvml->current_token);                            \
        struct pchvml_token* token = hvml->current_token;                  \
        return token;                                                      \
    } while (false)

#define RETURN_AND_RECONSUME_IN(next_state)                                \
    do {                                                                   \
        hvml->state = next_state;                                          \
        pchvml_parser_save_appropriate_tag_name(hvml);                     \
        pchvml_token_done(hvml->current_token);                            \
        struct pchvml_token* token = hvml->current_token;                  \
        return token;                                                      \
    } while (false)

#define STATE_DESC(state_name)                                              \
    case state_name:                                                        \
        return ""#state_name;                                               \

#define APPEND_TO_CHARACTER(c, sz_c)                                           \
    do {                                                                    \
        pchvml_parser_append_to_character (hvml, c, sz_c);                  \
    } while (false)

#define APPEND_TEMP_BUFFER_TO_CHARACTER()                                   \
    do {                                                                    \
        pchvml_parser_append_to_character (hvml,                            \
            pchvml_temp_buffer_get_buffer(hvml->temp_buffer),               \
            pchvml_temp_buffer_get_size_in_bytes(hvml->temp_buffer));       \
        pchvml_temp_buffer_reset (hvml->temp_buffer);                       \
    } while (false)

#define APPEND_TO_TAG_NAME(c, sz_c)                                         \
    do {                                                                    \
        pchvml_parser_append_to_tag_name (hvml, c, sz_c);                   \
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
    /* PCHVML_ERROR_UNEXPECTED_NULL_CHARACTER */
    "pchvml error unexpected null character",
    /* PCHVML_ERROR_UNEXPECTED_QUESTION_MARK_INSTEAD_OF_TAG_NAME */
    "pchvml error unexpected question mark instead of tag name",
    /* PCHVML_ERROR_EOF_BEFORE_TAG_NAME */
    "pchvml error eof before tag name",
    /* PCHVML_ERROR_MISSING_END_TAG_NAME */
    "pchvml error missing end tag name",
    /* PCHVML_ERROR_INVALID_FIRST_CHARACTER_OF_TAG_NAME */
    "pchvml error invalid first character of tag name",
    /* PCHVML_ERROR_EOF_IN_TAG */
    "pchvml error eof in tag",
    /* PCHVML_ERROR_UNEXPECTED_EQUALS_SIGN_BEFORE_ATTRIBUTE_NAME */
    "pchvml error unexpected equals sign before attribute name",
    /* PCHVML_ERROR_UNEXPECTED_CHARACTER_IN_ATTRIBUTE_NAME */
    "pchvml error unexpected character in attribute name",
    /* PCHVML_ERROR_UNEXPECTED_CHARACTER_IN_UNQUOTED_ATTRIBUTE_VALUE */
    "pchvml error unexpected character in unquoted attribute value",
    /* PCHVML_ERROR_MISSING_WHITESPACE_BETWEEN_ATTRIBUTES */
    "pchvml error missing whitespace between attributes",
    /* PCHVML_ERROR_UNEXPECTED_SOLIDUS_IN_TAG */
    "pchvml error unexpected solidus in tag",
    /* PCHVML_ERROR_CDATA_IN_HTML_CONTENT */
    "pchvml error cdata in html content",
    /* PCHVML_ERROR_INCORRECTLY_OPENED_COMMENT */
    "pchvml error incorrectly opened comment",
    /* PCHVML_ERROR_ABRUPT_CLOSING_OF_EMPTY_COMMENT */
    "pchvml error abrupt closing of empty comment",
    /* PCHVML_ERROR_EOF_IN_COMMENT */
    "pchvml error eof in comment",
    /* PCHVML_ERROR_EOF_IN_DOCTYPE */
    "pchvml error eof in doctype",
    /* PCHVML_ERROR_MISSING_WHITESPACE_BEFORE_DOCTYPE_NAME */
    "pchvml error missing whitespace before doctype name",
    /* PCHVML_ERROR_MISSING_DOCTYPE_NAME */
    "pchvml error missing doctype name",
    /* PCHVML_ERROR_INVALID_CHARACTER_SEQUENCE_AFTER_DOCTYPE_NAME */
    "pchvml error invalid character sequence after doctype name",
    /* PCHVML_ERROR_MISSING_WHITESPACE_AFTER_DOCTYPE_PUBLIC_KEYWORD */
    "pchvml error missing whitespace after doctype public keyword",
    /* PCHVML_ERROR_MISSING_DOCTYPE_PUBLIC_IDENTIFIER */
    "pchvml error missing doctype public identifier",
    /* PCHVML_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_PUBLIC_IDENTIFIER */
    "pchvml error missing quote before doctype public identifier",
    /* PCHVML_ERROR_ABRUPT_DOCTYPE_PUBLIC_IDENTIFIER */
    "pchvml error abrupt doctype public identifier",
    /* PCHVML_ERROR_MISSING_WHITESPACE_BETWEEN_DOCTYPE_PUBLIC_AND_SYSTEM_IDENTIFIERS */
    "pchvml error missing whitespace between doctype public and system identifiers",
    /* PCHVML_ERROR_MISSING_WHITESPACE_AFTER_DOCTYPE_SYSTEM_KEYWORD */
    "pchvml error missing whitespace after doctype system keyword",
    /* PCHVML_ERROR_MISSING_DOCTYPE_SYSTEM_IDENTIFIER */
    "pchvml error missing doctype system identifier",
    /* PCHVML_ERROR_ABRUPT_DOCTYPE_SYSTEM_IDENTIFIER */
    "pchvml error abrupt doctype system identifier",
    /* PCHVML_ERROR_UNEXPECTED_CHARACTER_AFTER_DOCTYPE_SYSTEM_IDENTIFIER */
    "pchvml error unexpected character after doctype system identifier",
    /* PCHVML_ERROR_EOF_IN_CDATA */
    "pchvml error eof in cdata",
    /* PCHVML_ERROR_UNKNOWN_NAMED_CHARACTER_REFERENCE */
    "pchvml error unknown named character reference",
    /* PCHVML_ERROR_ABSENCE_OF_DIGITS_IN_NUMERIC_CHARACTER_REFERENCE */
    "pchvml error absence of digits in numeric character reference",
    /* PCHVML_ERROR_UNEXPECTED_CHARACTER */
    "pchvml error unexpected character",
    /* PCHVML_ERROR_UNEXPECTED_JSON_NUMBER_EXPONENT */
    "pchvml error unexpected json number exponent",
    /* PCHVML_ERROR_UNEXPECTED_JSON_NUMBER_FRACTION */
    "pchvml error unexpected json number fraction",
    /* PCHVML_ERROR_UNEXPECTED_JSON_NUMBER_INTEGER */
    "pchvml error unexpected json number integer",
    /* PCHVML_ERROR_UNEXPECTED_JSON_NUMBER */
    "pchvml error unexpected json number",
    /* PCHVML_ERROR_UNEXPECTED_RIGHT_BRACE */
    "pchvml error unexpected right brace",
    /* PCHVML_ERROR_UNEXPECTED_RIGHT_BRACKET */
    "pchvml error unexpected right bracket",
    /* PCHVML_ERROR_UNEXPECTED_JSON_KEY_NAME */
    "pchvml error unexpected json key name",
    /* PCHVML_ERROR_UNEXPECTED_COMMA */
    "pchvml error unexpected comma",
    /* PCHVML_ERROR_UNEXPECTED_JSON_KEYWORD */
    "pchvml error unexpected json keyword",
    /* PCHVML_ERROR_UNEXPECTED_BASE64 */
    "pchvml error unexpected base64",
    /* PCHVML_ERROR_BAD_JSON_NUMBER */
    "pchvml error bad json number",
    /* PCHVML_ERROR_BAD_JSON_STRING_ESCAPE_ENTITY */
    "pchvml error bad json string escape entity",
    /* PCHVML_ERROR_BAD_JSONEE */
    "pchvml error bad jsonee",
    /* PCHVML_ERROR_BAD_JSONEE_ESCAPE_ENTITY */
    "pchvml error bad jsonee escape entity",
    /* PCHVML_ERROR_BAD_JSONEE_VARIABLE_NAME */
    "pchvml error bad jsonee variable name",
    /* PCHVML_ERROR_EMPTY_JSONEE_NAME */
    "pchvml error empty jsonee name",
    /* PCHVML_ERROR_BAD_JSONEE_NAME */
    "pchvml error bad jsonee name",
    /* PCHVML_ERROR_BAD_JSONEE_KEYWORD */
    "pchvml error bad jsonee keyword",
    /* PCHVML_ERROR_EMPTY_JSONEE_KEYWORD */
    "pchvml error empty jsonee keyword",
    /* PCHVML_ERROR_BAD_JSONEE_UNEXPECTED_COMMA */
    "pchvml error bad jsonee unexpected comma",
    /* PCHVML_ERROR_BAD_JSONEE_UNEXPECTED_PARENTHESIS */
    "pchvml error bad jsonee unexpected parenthesis",
    /* PCHVML_ERROR_BAD_JSONEE_UNEXPECTED_LEFT_ANGLE_BRACKET */
    "pchvml error bad jsonee unexpected left angle bracket",
    /* PCHVML_ERROR_MISSING_MISSING_ATTRIBUTE_VALUE */
    "pchvml error missing missing attribute value",
    /* PCHVML_ERROR_NESTED_COMMENT */
    "pchvml error nested comment",
    /* PCHVML_ERROR_INCORRECTLY_CLOSED_COMMENT */
    "pchvml error incorrectly closed comment",
    /* PCHVML_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_SYSTEM_IDENTIFIER */
    "pchvml error missing quote before doctype system identifier"
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

static inline UNUSED_FUNCTION bool is_eof (wchar_t character)
{
    return character == PCHVML_END_OF_FILE;
}


void pchvml_init_once(void)
{
    pcinst_register_error_message_segment(&_hvml_err_msgs_seg);
}

struct pchvml_parser* pchvml_create(uint32_t flags, size_t queue_size)
{
    struct pchvml_parser* parser = (struct pchvml_parser*) PCHVML_ALLOC(
            sizeof(struct pchvml_parser));
    parser->state = PCHVML_DATA_STATE;
    parser->flags = flags;
    parser->queue_size = queue_size;
    parser->temp_buffer = pchvml_temp_buffer_new ();
    parser->appropriate_tag_name = pchvml_temp_buffer_new ();
    return parser;
}

void pchvml_reset(struct pchvml_parser* parser, uint32_t flags,
        size_t queue_size)
{
    parser->state = PCHVML_DATA_STATE;
    parser->flags = flags;
    parser->queue_size = queue_size;
    pchvml_temp_buffer_reset (parser->temp_buffer);
    pchvml_temp_buffer_reset (parser->appropriate_tag_name);
}

void pchvml_destroy(struct pchvml_parser* parser)
{
    if (parser) {
        pchvml_temp_buffer_destroy (parser->temp_buffer);
        PCHVML_FREE(parser);
    }
}

const char* pchvml_pchvml_state_desc (enum pchvml_state state)
{
    switch (state) {
        STATE_DESC(PCHVML_DATA_STATE)
        STATE_DESC(PCHVML_RCDATA_STATE)
        STATE_DESC(PCHVML_RAWTEXT_STATE)
        STATE_DESC(PCHVML_PLAINTEXT_STATE)
        STATE_DESC(PCHVML_TAG_OPEN_STATE)
        STATE_DESC(PCHVML_END_TAG_OPEN_STATE)
        STATE_DESC(PCHVML_TAG_NAME_STATE)
        STATE_DESC(PCHVML_RCDATA_LESS_THAN_SIGN_STATE)
        STATE_DESC(PCHVML_RCDATA_END_TAG_OPEN_STATE)
        STATE_DESC(PCHVML_RCDATA_END_TAG_NAME_STATE)
        STATE_DESC(PCHVML_RAWTEXT_LESS_THAN_SIGN_STATE)
        STATE_DESC(PCHVML_RAWTEXT_END_TAG_OPEN_STATE)
        STATE_DESC(PCHVML_RAWTEXT_END_TAG_NAME_STATE)
        STATE_DESC(PCHVML_BEFORE_ATTRIBUTE_NAME_STATE)
        STATE_DESC(PCHVML_ATTRIBUTE_NAME_STATE)
        STATE_DESC(PCHVML_AFTER_ATTRIBUTE_NAME_STATE)
        STATE_DESC(PCHVML_BEFORE_ATTRIBUTE_VALUE_STATE)
        STATE_DESC(PCHVML_ATTRIBUTE_VALUE_DOUBLE_QUOTED_STATE)
        STATE_DESC(PCHVML_ATTRIBUTE_VALUE_SINGLE_QUOTED_STATE)
        STATE_DESC(PCHVML_ATTRIBUTE_VALUE_UNQUOTED_STATE)
        STATE_DESC(PCHVML_AFTER_ATTRIBUTE_VALUE_QUOTED_STATE)
        STATE_DESC(PCHVML_SELF_CLOSING_START_TAG_STATE)
        STATE_DESC(PCHVML_BOGUS_COMMENT_STATE)
        STATE_DESC(PCHVML_MARKUP_DECLARATION_OPEN_STATE)
        STATE_DESC(PCHVML_COMMENT_START_STATE)
        STATE_DESC(PCHVML_COMMENT_START_DASH_STATE)
        STATE_DESC(PCHVML_COMMENT_STATE)
        STATE_DESC(PCHVML_COMMENT_LESS_THAN_SIGN_STATE)
        STATE_DESC(PCHVML_COMMENT_LESS_THAN_SIGN_BANG_STATE)
        STATE_DESC(PCHVML_COMMENT_LESS_THAN_SIGN_BANG_DASH_STATE)
        STATE_DESC(PCHVML_COMMENT_LESS_THAN_SIGN_BANG_DASH_DASH_STATE)
        STATE_DESC(PCHVML_COMMENT_END_DASH_STATE)
        STATE_DESC(PCHVML_COMMENT_END_STATE)
        STATE_DESC(PCHVML_COMMENT_END_BANG_STATE)
        STATE_DESC(PCHVML_DOCTYPE_STATE)
        STATE_DESC(PCHVML_BEFORE_DOCTYPE_NAME_STATE)
        STATE_DESC(PCHVML_DOCTYPE_NAME_STATE)
        STATE_DESC(PCHVML_AFTER_DOCTYPE_NAME_STATE)
        STATE_DESC(PCHVML_AFTER_DOCTYPE_PUBLIC_KEYWORD_STATE)
        STATE_DESC(PCHVML_BEFORE_DOCTYPE_PUBLIC_IDENTIFIER_STATE)
        STATE_DESC(PCHVML_DOCTYPE_PUBLIC_IDENTIFIER_DOUBLE_QUOTED_STATE)
        STATE_DESC(PCHVML_DOCTYPE_PUBLIC_IDENTIFIER_SINGLE_QUOTED_STATE)
        STATE_DESC(PCHVML_AFTER_DOCTYPE_PUBLIC_IDENTIFIER_STATE)
        STATE_DESC(PCHVML_BETWEEN_DOCTYPE_PUBLIC_IDENTIFIER_AND_SYSTEM_INFORMATION_STATE)
        STATE_DESC(PCHVML_AFTER_DOCTYPE_SYSTEM_KEYWORD_STATE)
        STATE_DESC(PCHVML_BEFORE_DOCTYPE_SYSTEM_INFORMATION_STATE)
        STATE_DESC(PCHVML_DOCTYPE_SYSTEM_INFORMATION_DOUBLE_QUOTED_STATE)
        STATE_DESC(PCHVML_DOCTYPE_SYSTEM_INFORMATION_SINGLE_QUOTED_STATE)
        STATE_DESC(PCHVML_AFTER_DOCTYPE_SYSTEM_INFORMATION_STATE)
        STATE_DESC(PCHVML_BOGUS_DOCTYPE_STATE)
        STATE_DESC(PCHVML_CDATA_SECTION_STATE)
        STATE_DESC(PCHVML_CDATA_SECTION_BRACKET_STATE)
        STATE_DESC(PCHVML_CDATA_SECTION_END_STATE)
        STATE_DESC(PCHVML_CHARACTER_REFERENCE_STATE)
        STATE_DESC(PCHVML_NAMED_CHARACTER_REFERENCE_STATE)
        STATE_DESC(PCHVML_AMBIGUOUS_AMPERSAND_STATE)
        STATE_DESC(PCHVML_NUMERIC_CHARACTER_REFERENCE_STATE)
        STATE_DESC(PCHVML_HEXADECIMAL_CHARACTER_REFERENCE_START_STATE)
        STATE_DESC(PCHVML_DECIMAL_CHARACTER_REFERENCE_START_STATE)
        STATE_DESC(PCHVML_HEXADECIMAL_CHARACTER_REFERENCE_STATE)
        STATE_DESC(PCHVML_DECIMAL_CHARACTER_REFERENCE_STATE)
        STATE_DESC(PCHVML_NUMERIC_CHARACTER_REFERENCE_END_STATE)
        STATE_DESC(PCHVML_SPECIAL_ATTRIBUTE_OPERATOR_IN_ATTRIBUTE_NAME_STATE)
        STATE_DESC(PCHVML_SPECIAL_ATTRIBUTE_OPERATOR_AFTER_ATTRIBUTE_NAME_STATE)
        STATE_DESC(PCHVML_EJSON_DATA_STATE)
        STATE_DESC(PCHVML_EJSON_FINISHED_STATE)
        STATE_DESC(PCHVML_EJSON_CONTROL_STATE)
        STATE_DESC(PCHVML_EJSON_LEFT_BRACE_STATE)
        STATE_DESC(PCHVML_EJSON_RIGHT_BRACE_STATE)
        STATE_DESC(PCHVML_EJSON_LEFT_BRACKET_STATE)
        STATE_DESC(PCHVML_EJSON_RIGHT_BRACKET_STATE)
        STATE_DESC(PCHVML_EJSON_LESS_THAN_SIGN_STATE)
        STATE_DESC(PCHVML_EJSON_GREATER_THAN_SIGN_STATE)
        STATE_DESC(PCHVML_EJSON_LEFT_PARENTHESIS_STATE)
        STATE_DESC(PCHVML_EJSON_RIGHT_PARENTHESIS_STATE)
        STATE_DESC(PCHVML_EJSON_DOLLAR_STATE)
        STATE_DESC(PCHVML_EJSON_AFTER_VALUE_STATE)
        STATE_DESC(PCHVML_EJSON_BEFORE_NAME_STATE)
        STATE_DESC(PCHVML_EJSON_AFTER_NAME_STATE)
        STATE_DESC(PCHVML_EJSON_NAME_UNQUOTED_STATE)
        STATE_DESC(PCHVML_EJSON_NAME_SINGLE_QUOTED_STATE)
        STATE_DESC(PCHVML_EJSON_NAME_DOUBLE_QUOTED_STATE)
        STATE_DESC(PCHVML_EJSON_VALUE_SINGLE_QUOTED_STATE)
        STATE_DESC(PCHVML_EJSON_VALUE_DOUBLE_QUOTED_STATE)
        STATE_DESC(PCHVML_EJSON_AFTER_VALUE_DOUBLE_QUOTED_STATE)
        STATE_DESC(PCHVML_EJSON_VALUE_TWO_DOUBLE_QUOTED_STATE)
        STATE_DESC(PCHVML_EJSON_VALUE_THREE_DOUBLE_QUOTED_STATE)
        STATE_DESC(PCHVML_EJSON_KEYWORD_STATE)
        STATE_DESC(PCHVML_EJSON_AFTER_KEYWORD_STATE)
        STATE_DESC(PCHVML_EJSON_BYTE_SEQUENCE_STATE)
        STATE_DESC(PCHVML_EJSON_AFTER_BYTE_SEQUENCE_STATE)
        STATE_DESC(PCHVML_EJSON_HEX_BYTE_SEQUENCE_STATE)
        STATE_DESC(PCHVML_EJSON_BINARY_BYTE_SEQUENCE_STATE)
        STATE_DESC(PCHVML_EJSON_BASE64_BYTE_SEQUENCE_STATE)
        STATE_DESC(PCHVML_EJSON_VALUE_NUMBER_STATE)
        STATE_DESC(PCHVML_EJSON_AFTER_VALUE_NUMBER_STATE)
        STATE_DESC(PCHVML_EJSON_VALUE_NUMBER_INTEGER_STATE)
        STATE_DESC(PCHVML_EJSON_VALUE_NUMBER_FRACTION_STATE)
        STATE_DESC(PCHVML_EJSON_VALUE_NUMBER_EXPONENT_STATE)
        STATE_DESC(PCHVML_EJSON_VALUE_NUMBER_EXPONENT_INTEGER_STATE)
        STATE_DESC(PCHVML_EJSON_VALUE_NUMBER_SUFFIX_INTEGER_STATE)
        STATE_DESC(PCHVML_EJSON_VALUE_NUMBER_INFINITY_STATE)
        STATE_DESC(PCHVML_EJSON_VALUE_NAN_STATE)
        STATE_DESC(PCHVML_EJSON_STRING_ESCAPE_STATE)
        STATE_DESC(PCHVML_EJSON_STRING_ESCAPE_FOUR_HEXADECIMAL_DIGITS_STATE)
        STATE_DESC(PCHVML_EJSON_JSONEE_VARIABLE_STATE)
        STATE_DESC(PCHVML_EJSON_JSONEE_FULL_STOP_SIGN_STATE)
        STATE_DESC(PCHVML_EJSON_JSONEE_KEYWORD_STATE)
        STATE_DESC(PCHVML_EJSON_JSONEE_STRING_STATE)
        STATE_DESC(PCHVML_EJSON_AFTER_JSONEE_STRING_STATE)
    }
    return NULL;
}

void pchvml_parser_append_to_tag_name (struct pchvml_parser* hvml,
        const char* bytes, size_t nr_bytes)
{
    if (hvml->current_token == NULL) {
        hvml->current_token = pchvml_token_new (PCHVML_TOKEN_CHARACTER);
    }
    pchvml_token_append_to_name(hvml->current_token, bytes, nr_bytes);
}

void pchvml_parser_append_to_character (struct pchvml_parser* hvml,
        const char* bytes, size_t nr_bytes)
{
    if (hvml->current_token == NULL) {
        hvml->current_token = pchvml_token_new (PCHVML_TOKEN_CHARACTER);
    }
    pchvml_token_append_to_character(hvml->current_token, bytes, nr_bytes);
}

void pchvml_parser_save_appropriate_tag_name (struct pchvml_parser* hvml)
{
    if (pchvml_token_is_type (hvml->current_token, PCHVML_TOKEN_START_TAG)) {
        const char* name = pchvml_token_get_name(hvml->current_token);
        pchvml_temp_buffer_append(hvml->appropriate_tag_name,
                name, strlen(name));
    }
}

void pchvml_parser_reset_appropriate_tag_name (struct pchvml_parser* hvml)
{
    pchvml_temp_buffer_reset(hvml->appropriate_tag_name);
}

bool pchvml_parser_is_appropriate_end_tag (struct pchvml_parser* hvml)
{
    const char* name = pchvml_token_get_name(hvml->current_token);
    return pchvml_temp_buffer_equal_to (hvml->appropriate_tag_name, name,
            strlen(name));
}

bool pchvml_parser_is_operation_tag_token (struct pchvml_token* token)
{
    UNUSED_PARAM(token);
    // TODO
    return true;
}

bool pchvml_parser_is_ordinary_attribute (struct pchvml_token_attribute* attr)
{
    UNUSED_PARAM(attr);
    // TODO
    return true;
}

bool pchvml_parser_is_preposition_attribute (struct pchvml_token_attribute* attr)
{
    UNUSED_PARAM(attr);
    // TODO
    return true;
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
        BEGIN_STATE(PCHVML_DATA_STATE)
            if (character == '&') {
                SET_RETURN_STATE(PCHVML_DATA_STATE);
                ADVANCE_TO(PCHVML_CHARACTER_REFERENCE_STATE);
            }
            if (character == '<') {
                if (pchvml_token_is_type(hvml->current_token,
                            PCHVML_TOKEN_CHARACTER)) {
                    RETURN_IN_CURRENT_STATE(true);
                }
                ADVANCE_TO(PCHVML_TAG_OPEN_STATE);
            }
            if (is_eof(character)) {
                return pchvml_token_new_eof();
            }
            APPEND_TO_CHARACTER(hvml->c, hvml->sz_c);
            ADVANCE_TO(PCHVML_DATA_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_RCDATA_STATE)
            if (character == '&') {
                SET_RETURN_STATE(PCHVML_RCDATA_STATE);
                ADVANCE_TO(PCHVML_CHARACTER_REFERENCE_STATE);
            }
            if (character == '<') {
                ADVANCE_TO(PCHVML_RCDATA_LESS_THAN_SIGN_STATE);
            }
            if (is_eof(character)) {
                RECONSUME_IN(PCHVML_DATA_STATE);
            }
            APPEND_TO_CHARACTER(hvml->c, hvml->sz_c);
            ADVANCE_TO(PCHVML_TOKEN_CHARACTER);
        END_STATE()

        BEGIN_STATE(PCHVML_RAWTEXT_STATE)
            if (character == '<') {
                ADVANCE_TO(PCHVML_RAWTEXT_LESS_THAN_SIGN_STATE);
            }
            if (is_eof(character)) {
                return pchvml_token_new_eof();
            }

            APPEND_TO_CHARACTER(hvml->c, hvml->sz_c);
            ADVANCE_TO(PCHVML_RAWTEXT_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_PLAINTEXT_STATE)
            if (is_eof(character)) {
                return pchvml_token_new_eof();
            }

            APPEND_TO_CHARACTER(hvml->c, hvml->sz_c);
            ADVANCE_TO(PCHVML_PLAINTEXT_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_TAG_OPEN_STATE)
            if (character == '!') {
                ADVANCE_TO(PCHVML_MARKUP_DECLARATION_OPEN_STATE);
            }
            if (character == '/') {
                ADVANCE_TO(PCHVML_END_TAG_OPEN_STATE);
            }
            if (is_ascii_alpha(character)) {
                hvml->current_token = pchvml_token_new_start_tag ();
                RECONSUME_IN(PCHVML_TAG_NAME_STATE);
            }
            if (character == '?') {
                PCHVML_SET_ERROR(
                    PCHVML_ERROR_UNEXPECTED_QUESTION_MARK_INSTEAD_OF_TAG_NAME);
                hvml->current_token = pchvml_token_new_comment();
                RECONSUME_IN(PCHVML_BOGUS_COMMENT_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_BEFORE_TAG_NAME);
                APPEND_TO_CHARACTER("<", 1);
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            PCHVML_SET_ERROR(PCHVML_ERROR_INVALID_FIRST_CHARACTER_OF_TAG_NAME);
            APPEND_TO_CHARACTER("<", 1);
            RECONSUME_IN(PCHVML_DATA_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_END_TAG_OPEN_STATE)
            if (is_ascii_alpha(character)) {
                hvml->current_token = pchvml_token_new_end_tag();
                pchvml_parser_reset_appropriate_tag_name(hvml);
                ADVANCE_TO(PCHVML_TAG_NAME_STATE);
            }
            if (character == '>') {
                PCHVML_SET_ERROR(PCHVML_ERROR_MISSING_END_TAG_NAME);
                ADVANCE_TO(PCHVML_DATA_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_BEFORE_TAG_NAME);
                APPEND_TO_CHARACTER("<", 1);
                APPEND_TO_CHARACTER("/", 1);
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            PCHVML_SET_ERROR(PCHVML_ERROR_INVALID_FIRST_CHARACTER_OF_TAG_NAME);
            hvml->current_token = pchvml_token_new_comment();
            RECONSUME_IN(PCHVML_BOGUS_COMMENT_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_TAG_NAME_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(PCHVML_BEFORE_ATTRIBUTE_NAME_STATE);
            }
            if (character == '/') {
                ADVANCE_TO(PCHVML_SELF_CLOSING_START_TAG_STATE);
            }
            if (character == '>') {
                RETURN_AND_SWITCH_TO(PCHVML_DATA_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_TAG);
                RECONSUME_IN(PCHVML_DATA_STATE);
            }
            APPEND_TO_TAG_NAME(hvml->c, hvml->sz_c);
            ADVANCE_TO(PCHVML_TAG_NAME_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_RCDATA_LESS_THAN_SIGN_STATE)
            if (character == '/') {
                RESET_TEMP_BUFFER();
                ADVANCE_TO(PCHVML_RCDATA_END_TAG_OPEN_STATE);
            }
            APPEND_TO_CHARACTER("<", 1);
            RECONSUME_IN(PCHVML_RCDATA_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_RCDATA_END_TAG_OPEN_STATE)
            if (is_ascii_alpha(character)) {
                hvml->current_token = pchvml_token_new_end_tag();
                RECONSUME_IN(PCHVML_RCDATA_END_TAG_NAME_STATE);
            }
            APPEND_TO_CHARACTER("<", 1);
            APPEND_TO_CHARACTER("/", 1);
            RECONSUME_IN(PCHVML_RCDATA_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_RCDATA_END_TAG_NAME_STATE)
            if (is_ascii_alpha(character)) {
                APPEND_TO_TAG_NAME(hvml->c, hvml->sz_c);
                APPEND_TEMP_BUFFER(hvml->c, hvml->sz_c);
                ADVANCE_TO(PCHVML_RCDATA_END_TAG_NAME_STATE);
            }
            if (is_whitespace(character)) {
                if (pchvml_parser_is_appropriate_end_tag(hvml)) {
                    SWITCH_TO(PCHVML_BEFORE_ATTRIBUTE_NAME_STATE);
                }
            }
            else if (character == '/') {
                if (pchvml_parser_is_appropriate_end_tag(hvml)) {
                    SWITCH_TO(PCHVML_SELF_CLOSING_START_TAG_STATE);
                }
            }
            else if (character == '>') {
                if (pchvml_parser_is_appropriate_end_tag(hvml)) {
                    RETURN_AND_SWITCH_TO(PCHVML_DATA_STATE);
                }
            }
            APPEND_TO_CHARACTER("<", 1);
            APPEND_TO_CHARACTER("/", 1);
            APPEND_TEMP_BUFFER_TO_CHARACTER();
            RECONSUME_IN(PCHVML_RCDATA_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_RAWTEXT_LESS_THAN_SIGN_STATE)
            if (character == '/') {
                RESET_TEMP_BUFFER();
                ADVANCE_TO(PCHVML_RAWTEXT_END_TAG_OPEN_STATE);
            }
            APPEND_TO_CHARACTER("<", 1);
            RECONSUME_IN(PCHVML_RAWTEXT_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_RAWTEXT_END_TAG_OPEN_STATE)
            if (is_ascii_alpha(character)) {
                hvml->current_token = pchvml_token_new_end_tag();
                RECONSUME_IN(PCHVML_RAWTEXT_END_TAG_NAME_STATE);
            }
            APPEND_TO_CHARACTER("<", 1);
            APPEND_TO_CHARACTER("/", 1);
            RECONSUME_IN(PCHVML_RAWTEXT_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_RAWTEXT_END_TAG_NAME_STATE)
            if (is_ascii_alpha(character)) {
                APPEND_TO_TAG_NAME(hvml->c, hvml->sz_c);
                APPEND_TEMP_BUFFER(hvml->c, hvml->sz_c);
                ADVANCE_TO(PCHVML_RAWTEXT_END_TAG_NAME_STATE);
            }
            if (is_whitespace(character)) {
                if (pchvml_parser_is_appropriate_end_tag(hvml)) {
                    SWITCH_TO(PCHVML_BEFORE_ATTRIBUTE_NAME_STATE);
                }
            }
            else if (character == '/') {
                if (pchvml_parser_is_appropriate_end_tag(hvml)) {
                    SWITCH_TO(PCHVML_SELF_CLOSING_START_TAG_STATE);
                }
            }
            else if (character == '>') {
                if (pchvml_parser_is_appropriate_end_tag(hvml)) {
                    RETURN_AND_SWITCH_TO(PCHVML_DATA_STATE);
                }
            }
            APPEND_TO_CHARACTER("<", 1);
            APPEND_TO_CHARACTER("/", 1);
            APPEND_TEMP_BUFFER_TO_CHARACTER();
            RECONSUME_IN(PCHVML_RAWTEXT_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_BEFORE_ATTRIBUTE_NAME_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(PCHVML_BEFORE_ATTRIBUTE_NAME_STATE);
            }
            if (character == '/' || character == '>'
                    || is_eof(character)) {
                RECONSUME_IN(PCHVML_AFTER_ATTRIBUTE_NAME_STATE);
            }
            if (character == '=') {
                PCHVML_SET_ERROR(
                    PCHVML_ERROR_UNEXPECTED_EQUALS_SIGN_BEFORE_ATTRIBUTE_NAME);
                pchvml_token_attribute_begin (hvml->current_token);
                pchvml_token_attribute_append_to_name (
                        hvml->current_token, hvml->c, hvml->sz_c);
                ADVANCE_TO(PCHVML_ATTRIBUTE_NAME_STATE);
            }
            pchvml_token_attribute_begin (hvml->current_token);
            RECONSUME_IN(PCHVML_ATTRIBUTE_NAME_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_ATTRIBUTE_NAME_STATE)
            if (is_whitespace(character) || character == '/'
                    || character == '>' || is_eof(character)) {
                RECONSUME_IN(PCHVML_AFTER_ATTRIBUTE_NAME_STATE);
            }
            if (character == '=') {
                ADVANCE_TO(PCHVML_BEFORE_ATTRIBUTE_VALUE_STATE);
            }
            if (character == '"' || character == '\'' || character == '<') {
                PCHVML_SET_ERROR(
                        PCHVML_ERROR_UNEXPECTED_CHARACTER_IN_ATTRIBUTE_NAME);
                pchvml_token_attribute_append_to_name (
                        hvml->current_token, hvml->c, hvml->sz_c);
            }
            if (character == '$' || character == '%' || character == '+'
                    || character == '-' || character == '^'
                    || character == '~') {
                if (pchvml_parser_is_operation_tag_token(hvml->current_token)
                        && pchvml_parser_is_ordinary_attribute(
                            hvml->current_token->curr_attr)) {
                    RESET_TEMP_BUFFER();
                    APPEND_TO_CHARACTER(hvml->c, hvml->sz_c);
                    SWITCH_TO(
                    PCHVML_SPECIAL_ATTRIBUTE_OPERATOR_IN_ATTRIBUTE_NAME_STATE);
                }
            }
            pchvml_token_attribute_append_to_name (
                    hvml->current_token, hvml->c, hvml->sz_c);
            ADVANCE_TO(PCHVML_ATTRIBUTE_NAME_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_AFTER_ATTRIBUTE_NAME_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(PCHVML_AFTER_ATTRIBUTE_NAME_STATE);
            }
            if (character == '/') {
                ADVANCE_TO(PCHVML_SELF_CLOSING_START_TAG_STATE);
            }
            if (character == '=') {
                ADVANCE_TO(PCHVML_BEFORE_ATTRIBUTE_VALUE_STATE);
            }
            if (character == '>') {
                RETURN_AND_SWITCH_TO(PCHVML_DATA_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_TAG);
                return pchvml_token_new_eof();
            }
            if (character == '$' || character == '%' || character == '+'
                    || character == '-' || character == '^'
                    || character == '~') {
                if (pchvml_parser_is_operation_tag_token(hvml->current_token)
                        && pchvml_parser_is_ordinary_attribute(
                            hvml->current_token->curr_attr)) {
                    RESET_TEMP_BUFFER();
                    APPEND_TO_CHARACTER(hvml->c, hvml->sz_c);
                    SWITCH_TO(
                    PCHVML_SPECIAL_ATTRIBUTE_OPERATOR_AFTER_ATTRIBUTE_NAME_STATE
                    );
                }
            }
            if (pchvml_parser_is_operation_tag_token(hvml->current_token)
                && pchvml_parser_is_preposition_attribute(
                        hvml->current_token->curr_attr)) {
                ADVANCE_TO(PCHVML_BEFORE_ATTRIBUTE_VALUE_STATE);
            }
            pchvml_token_attribute_begin(hvml->current_token);
            RECONSUME_IN(PCHVML_ATTRIBUTE_NAME_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_BEFORE_ATTRIBUTE_VALUE_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(PCHVML_BEFORE_ATTRIBUTE_VALUE_STATE);
            }
            if (character == '"') {
                ADVANCE_TO(PCHVML_ATTRIBUTE_VALUE_DOUBLE_QUOTED_STATE);
            }
            if (character == '&') {
                RECONSUME_IN(PCHVML_ATTRIBUTE_VALUE_UNQUOTED_STATE);
            }
            if (character == '\'') {
                ADVANCE_TO(PCHVML_ATTRIBUTE_VALUE_SINGLE_QUOTED_STATE);
            }
            if (character == '>') {
                PCHVML_SET_ERROR(PCHVML_ERROR_MISSING_MISSING_ATTRIBUTE_VALUE);
                RETURN_AND_SWITCH_TO(PCHVML_DATA_STATE);
            }
            if (character == '{' || character == '[' || character == '$') {
                RECONSUME_IN(PCHVML_EJSON_DATA_STATE);
            }
            if (is_eof(character)) {
                RECONSUME_IN(PCHVML_DATA_STATE);
            }
            RECONSUME_IN(PCHVML_ATTRIBUTE_VALUE_UNQUOTED_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_ATTRIBUTE_VALUE_DOUBLE_QUOTED_STATE)
            if (character == '"') {
                pchvml_token_attribute_end(hvml->current_token);
                ADVANCE_TO(PCHVML_AFTER_ATTRIBUTE_VALUE_QUOTED_STATE);
            }
            if (character == '&') {
                SET_RETURN_STATE(PCHVML_ATTRIBUTE_VALUE_DOUBLE_QUOTED_STATE);
                ADVANCE_TO(PCHVML_CHARACTER_REFERENCE_STATE);
            }
            if (character == '$') {
                // TODO : concat-string
            }
            if (is_eof(character)) {
                pchvml_token_attribute_end(hvml->current_token);
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_TAG);
                RECONSUME_IN(PCHVML_DATA_STATE);
            }
            pchvml_token_attribute_append_to_value(hvml->current_token,
                    hvml->c, hvml->sz_c);
            ADVANCE_TO(PCHVML_ATTRIBUTE_VALUE_DOUBLE_QUOTED_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_ATTRIBUTE_VALUE_SINGLE_QUOTED_STATE)
            if (character == '\'') {
                pchvml_token_attribute_end(hvml->current_token);
                ADVANCE_TO(PCHVML_AFTER_ATTRIBUTE_VALUE_QUOTED_STATE);
            }
            if (character == '&') {
                SET_RETURN_STATE(PCHVML_ATTRIBUTE_VALUE_SINGLE_QUOTED_STATE);
                ADVANCE_TO(PCHVML_CHARACTER_REFERENCE_STATE);
            }
            if (is_eof(character)) {
                pchvml_token_attribute_end(hvml->current_token);
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_TAG);
                RECONSUME_IN(PCHVML_DATA_STATE);
            }
            pchvml_token_attribute_append_to_value(hvml->current_token,
                    hvml->c, hvml->sz_c);
            ADVANCE_TO(PCHVML_ATTRIBUTE_VALUE_SINGLE_QUOTED_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_ATTRIBUTE_VALUE_UNQUOTED_STATE)
            if (is_whitespace(character)) {
                pchvml_token_attribute_end(hvml->current_token);
                ADVANCE_TO(PCHVML_BEFORE_ATTRIBUTE_NAME_STATE);
            }
            if (character == '&') {
                SET_RETURN_STATE(PCHVML_ATTRIBUTE_VALUE_UNQUOTED_STATE);
                ADVANCE_TO(PCHVML_CHARACTER_REFERENCE_STATE);
            }
            if (character == '>') {
                pchvml_token_attribute_end(hvml->current_token);
                RETURN_AND_SWITCH_TO(PCHVML_DATA_STATE);
            }
            if (is_eof(character)) {
                pchvml_token_attribute_end(hvml->current_token);
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_TAG);
                RECONSUME_IN(PCHVML_DATA_STATE);
            }
            if (character == '$') {
                // TODO concat-string and so on
                RECONSUME_IN(PCHVML_EJSON_DATA_STATE);
            }
            if (character == '"' || character == '\'' || character == '<'
                    || character == '=' || character == '`') {
                PCHVML_SET_ERROR(
                PCHVML_ERROR_UNEXPECTED_CHARACTER_IN_UNQUOTED_ATTRIBUTE_VALUE);
            }
            pchvml_token_attribute_append_to_value(hvml->current_token,
                    hvml->c, hvml->sz_c);
            ADVANCE_TO(PCHVML_ATTRIBUTE_VALUE_UNQUOTED_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_AFTER_ATTRIBUTE_VALUE_QUOTED_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(PCHVML_BEFORE_ATTRIBUTE_NAME_STATE);
            }
            if (character == '/') {
                ADVANCE_TO(PCHVML_SELF_CLOSING_START_TAG_STATE);
            }
            if (character == '>') {
                RETURN_AND_SWITCH_TO(PCHVML_DATA_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_TAG);
                RECONSUME_IN(PCHVML_DATA_STATE);
            }
            PCHVML_SET_ERROR(
                    PCHVML_ERROR_MISSING_WHITESPACE_BETWEEN_ATTRIBUTES);
            RECONSUME_IN(PCHVML_BEFORE_ATTRIBUTE_NAME_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_SELF_CLOSING_START_TAG_STATE)
            if (character == '>') {
                pchvml_token_set_self_closing(hvml->current_token, true);
                RETURN_IN_CURRENT_STATE(PCHVML_DATA_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_TAG);
                RECONSUME_IN(PCHVML_DATA_STATE);
            }
            PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_SOLIDUS_IN_TAG);
            RECONSUME_IN(PCHVML_BEFORE_ATTRIBUTE_NAME_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_BOGUS_COMMENT_STATE)
            if (character == '>') {
                RETURN_AND_SWITCH_TO(PCHVML_DATA_STATE);
            }
            if (is_eof(character)) {
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            APPEND_TO_CHARACTER(hvml->c, hvml->sz_c);
            ADVANCE_TO(PCHVML_BOGUS_COMMENT_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_MARKUP_DECLARATION_OPEN_STATE)
            //TODO
        END_STATE()

        BEGIN_STATE(PCHVML_COMMENT_START_STATE)
            if (character == '-') {
                ADVANCE_TO(PCHVML_COMMENT_START_DASH_STATE);
            }
            if (character == '>') {
                PCHVML_SET_ERROR(PCHVML_ERROR_ABRUPT_CLOSING_OF_EMPTY_COMMENT);
                RETURN_AND_SWITCH_TO(PCHVML_DATA_STATE);
            }
            RECONSUME_IN(PCHVML_COMMENT_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_COMMENT_START_DASH_STATE)
            if (character == '-') {
                ADVANCE_TO(PCHVML_COMMENT_END_STATE);
            }
            if (character == '>') {
                PCHVML_SET_ERROR(PCHVML_ERROR_ABRUPT_CLOSING_OF_EMPTY_COMMENT);
                RETURN_AND_SWITCH_TO(PCHVML_DATA_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_COMMENT);
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            APPEND_TO_CHARACTER("-", 1);
            RECONSUME_IN(PCHVML_COMMENT_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_COMMENT_STATE)
            if (character == '<') {
                APPEND_TO_CHARACTER(hvml->c, hvml->sz_c);
                SWITCH_TO(PCHVML_COMMENT_LESS_THAN_SIGN_STATE);
            }
            if (character == '-') {
                ADVANCE_TO(PCHVML_COMMENT_END_DASH_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_COMMENT);
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            APPEND_TO_CHARACTER(hvml->c, hvml->sz_c);
            ADVANCE_TO(PCHVML_COMMENT_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_COMMENT_LESS_THAN_SIGN_STATE)
            if (character == '!') {
                APPEND_TO_CHARACTER(hvml->c, hvml->sz_c);
                ADVANCE_TO(PCHVML_COMMENT_LESS_THAN_SIGN_BANG_STATE);
            }
            if (character == '<') {
                APPEND_TO_CHARACTER(hvml->c, hvml->sz_c);
                ADVANCE_TO(PCHVML_COMMENT_LESS_THAN_SIGN_STATE);
            }
            RECONSUME_IN(PCHVML_COMMENT_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_COMMENT_LESS_THAN_SIGN_BANG_STATE)
            if (character == '-') {
                SWITCH_TO(PCHVML_COMMENT_LESS_THAN_SIGN_BANG_DASH_STATE);
            }
            RECONSUME_IN(PCHVML_COMMENT_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_COMMENT_LESS_THAN_SIGN_BANG_DASH_STATE)
            if (character == '-') {
                SWITCH_TO(PCHVML_COMMENT_LESS_THAN_SIGN_BANG_DASH_DASH_STATE);
            }
            RECONSUME_IN(PCHVML_COMMENT_END_DASH_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_COMMENT_LESS_THAN_SIGN_BANG_DASH_DASH_STATE)
            if (character == '>' || is_eof(character)) {
                RECONSUME_IN(PCHVML_COMMENT_END_STATE);
            }
            PCHVML_SET_ERROR(PCHVML_ERROR_NESTED_COMMENT);
            RECONSUME_IN(PCHVML_COMMENT_END_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_COMMENT_END_DASH_STATE)
            if (character == '-') {
                ADVANCE_TO(PCHVML_COMMENT_END_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_COMMENT);
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            APPEND_TO_CHARACTER("-", 1);
            RECONSUME_IN(PCHVML_COMMENT_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_COMMENT_END_STATE)
            if (character == '>') {
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            if (character == '!') {
                ADVANCE_TO(PCHVML_COMMENT_END_BANG_STATE);
            }
            if (character == '-') {
                APPEND_TO_CHARACTER("-", 1);
                ADVANCE_TO(PCHVML_COMMENT_END_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_COMMENT);
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            APPEND_TO_CHARACTER("-", 1);
            APPEND_TO_CHARACTER("-", 1);
            RECONSUME_IN(PCHVML_COMMENT_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_COMMENT_END_BANG_STATE)
            if (character == '-') {
                APPEND_TO_CHARACTER("-", 1);
                APPEND_TO_CHARACTER("-", 1);
                APPEND_TO_CHARACTER("!", 1);
                ADVANCE_TO(PCHVML_COMMENT_END_DASH_STATE);
            }
            if (character == '>') {
                PCHVML_SET_ERROR(PCHVML_ERROR_INCORRECTLY_CLOSED_COMMENT);
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_COMMENT);
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            APPEND_TO_CHARACTER("-", 1);
            APPEND_TO_CHARACTER("-", 1);
            APPEND_TO_CHARACTER("!", 1);
            RECONSUME_IN(PCHVML_COMMENT_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_DOCTYPE_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(PCHVML_BEFORE_DOCTYPE_NAME_STATE);
            }
            if (character == '>') {
                RECONSUME_IN(PCHVML_BEFORE_DOCTYPE_NAME_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_DOCTYPE);
                hvml->current_token = pchvml_token_new_doctype();
                pchvml_token_set_force_quirks(hvml->current_token, true);
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            PCHVML_SET_ERROR(
                    PCHVML_ERROR_MISSING_WHITESPACE_BEFORE_DOCTYPE_NAME);
            RECONSUME_IN(PCHVML_BEFORE_DOCTYPE_NAME_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_BEFORE_DOCTYPE_NAME_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(PCHVML_BEFORE_DOCTYPE_NAME_STATE);
            }
            if (character == '>') {
                PCHVML_SET_ERROR(PCHVML_ERROR_MISSING_DOCTYPE_NAME);
                hvml->current_token = pchvml_token_new_doctype();
                pchvml_token_set_force_quirks(hvml->current_token, true);
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_DOCTYPE);
                hvml->current_token = pchvml_token_new_doctype();
                pchvml_token_set_force_quirks(hvml->current_token, true);
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            hvml->current_token = pchvml_token_new_doctype();
            APPEND_TO_TAG_NAME(hvml->c, hvml->sz_c);
            ADVANCE_TO(PCHVML_DOCTYPE_NAME_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_DOCTYPE_NAME_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(PCHVML_AFTER_DOCTYPE_NAME_STATE);
            }
            if (character == '>') {
                RETURN_AND_SWITCH_TO(PCHVML_DATA_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_DOCTYPE);
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            APPEND_TO_TAG_NAME(hvml->c, hvml->sz_c);
            ADVANCE_TO(PCHVML_DOCTYPE_NAME_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_AFTER_DOCTYPE_NAME_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(PCHVML_AFTER_DOCTYPE_NAME_STATE);
            }
            if (character == '>') {
                RETURN_AND_SWITCH_TO(PCHVML_DATA_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_DOCTYPE);
                pchvml_token_set_force_quirks(hvml->current_token, true);
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            // TODO : six characters starting from the current input character
        END_STATE()

        BEGIN_STATE(PCHVML_AFTER_DOCTYPE_PUBLIC_KEYWORD_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(PCHVML_BEFORE_DOCTYPE_PUBLIC_IDENTIFIER_STATE);
            }
            if (character == '"') {
                PCHVML_SET_ERROR(
                  PCHVML_ERROR_MISSING_WHITESPACE_AFTER_DOCTYPE_PUBLIC_KEYWORD);
                pchvml_temp_buffer_reset(hvml->current_token->public_identifier);
                ADVANCE_TO(PCHVML_DOCTYPE_PUBLIC_IDENTIFIER_DOUBLE_QUOTED_STATE);
            }
            if (character == '\'') {
                PCHVML_SET_ERROR(
                  PCHVML_ERROR_MISSING_WHITESPACE_AFTER_DOCTYPE_PUBLIC_KEYWORD);
                pchvml_temp_buffer_reset(
                        hvml->current_token->public_identifier);
                ADVANCE_TO(PCHVML_DOCTYPE_PUBLIC_IDENTIFIER_SINGLE_QUOTED_STATE);
            }
            if (character == '>') {
                PCHVML_SET_ERROR(
                  PCHVML_ERROR_MISSING_DOCTYPE_PUBLIC_IDENTIFIER);
                pchvml_token_set_force_quirks(hvml->current_token, true);
                RETURN_AND_SWITCH_TO(PCHVML_DATA_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_DOCTYPE);
                pchvml_token_set_force_quirks(hvml->current_token, true);
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            PCHVML_SET_ERROR(
                PCHVML_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_PUBLIC_IDENTIFIER);
            pchvml_token_set_force_quirks(hvml->current_token, true);
            ADVANCE_TO(PCHVML_BOGUS_DOCTYPE_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_BEFORE_DOCTYPE_PUBLIC_IDENTIFIER_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(PCHVML_BEFORE_DOCTYPE_PUBLIC_IDENTIFIER_STATE);
            }
            if (character == '"') {
                pchvml_temp_buffer_reset(hvml->current_token->public_identifier);
                ADVANCE_TO(PCHVML_DOCTYPE_PUBLIC_IDENTIFIER_DOUBLE_QUOTED_STATE);
            }
            if (character == '\'') {
                pchvml_temp_buffer_reset(hvml->current_token->public_identifier);
                ADVANCE_TO(PCHVML_DOCTYPE_PUBLIC_IDENTIFIER_SINGLE_QUOTED_STATE);
            }
            if (character == '>') {
                PCHVML_SET_ERROR(
                  PCHVML_ERROR_MISSING_DOCTYPE_PUBLIC_IDENTIFIER);
                pchvml_token_set_force_quirks(hvml->current_token, true);
                RETURN_AND_SWITCH_TO(PCHVML_DATA_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_DOCTYPE);
                pchvml_token_set_force_quirks(hvml->current_token, true);
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            PCHVML_SET_ERROR(
                PCHVML_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_PUBLIC_IDENTIFIER);
            pchvml_token_set_force_quirks(hvml->current_token, true);
            ADVANCE_TO(PCHVML_BOGUS_DOCTYPE_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_DOCTYPE_PUBLIC_IDENTIFIER_DOUBLE_QUOTED_STATE)
            if (character == '"') {
                ADVANCE_TO(PCHVML_AFTER_DOCTYPE_PUBLIC_IDENTIFIER_STATE);
            }
            if (character == '>') {
                PCHVML_SET_ERROR(PCHVML_ERROR_ABRUPT_DOCTYPE_PUBLIC_IDENTIFIER);
                pchvml_token_set_force_quirks(hvml->current_token, true);
                RETURN_AND_SWITCH_TO(PCHVML_DATA_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_DOCTYPE);
                pchvml_token_set_force_quirks(hvml->current_token, true);
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            pchvml_token_append_to_public_identifier(hvml->current_token,
                    hvml->c, hvml->sz_c);
            ADVANCE_TO(PCHVML_DOCTYPE_PUBLIC_IDENTIFIER_DOUBLE_QUOTED_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_DOCTYPE_PUBLIC_IDENTIFIER_SINGLE_QUOTED_STATE)
            if (character == '\'') {
                ADVANCE_TO(PCHVML_AFTER_DOCTYPE_PUBLIC_IDENTIFIER_STATE);
            }
            if (character == '>') {
                PCHVML_SET_ERROR(PCHVML_ERROR_ABRUPT_DOCTYPE_PUBLIC_IDENTIFIER);
                pchvml_token_set_force_quirks(hvml->current_token, true);
                RETURN_AND_SWITCH_TO(PCHVML_DATA_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_DOCTYPE);
                pchvml_token_set_force_quirks(hvml->current_token, true);
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            pchvml_token_append_to_public_identifier(hvml->current_token,
                    hvml->c, hvml->sz_c);
            ADVANCE_TO(PCHVML_DOCTYPE_PUBLIC_IDENTIFIER_SINGLE_QUOTED_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_AFTER_DOCTYPE_PUBLIC_IDENTIFIER_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(
                PCHVML_BETWEEN_DOCTYPE_PUBLIC_IDENTIFIER_AND_SYSTEM_INFORMATION_STATE);
            }
            if (character == '>') {
                RETURN_AND_SWITCH_TO(PCHVML_DATA_STATE);
            }
            if (character == '"') {
                PCHVML_SET_ERROR(
                  PCHVML_ERROR_MISSING_WHITESPACE_BETWEEN_DOCTYPE_PUBLIC_AND_SYSTEM_IDENTIFIERS
                  );
                pchvml_temp_buffer_reset(
                        hvml->current_token->system_identifier);
                ADVANCE_TO(PCHVML_DOCTYPE_SYSTEM_INFORMATION_DOUBLE_QUOTED_STATE);
            }
            if (character == '\'') {
                PCHVML_SET_ERROR(
                  PCHVML_ERROR_MISSING_WHITESPACE_BETWEEN_DOCTYPE_PUBLIC_AND_SYSTEM_IDENTIFIERS
                  );
                pchvml_temp_buffer_reset(
                        hvml->current_token->system_identifier);
                ADVANCE_TO(PCHVML_DOCTYPE_SYSTEM_INFORMATION_SINGLE_QUOTED_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_DOCTYPE);
                pchvml_token_set_force_quirks(hvml->current_token, true);
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            PCHVML_SET_ERROR(
                PCHVML_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_SYSTEM_IDENTIFIER);
            pchvml_token_set_force_quirks(hvml->current_token, true);
            ADVANCE_TO(PCHVML_BOGUS_DOCTYPE_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_BETWEEN_DOCTYPE_PUBLIC_IDENTIFIER_AND_SYSTEM_INFORMATION_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(PCHVML_BETWEEN_DOCTYPE_PUBLIC_IDENTIFIER_AND_SYSTEM_INFORMATION_STATE);
            }
            if (character == '>') {
                RETURN_AND_SWITCH_TO(PCHVML_DATA_STATE);
            }
            if (character == '"') {
                pchvml_temp_buffer_reset(
                        hvml->current_token->system_identifier);
                ADVANCE_TO(PCHVML_DOCTYPE_SYSTEM_INFORMATION_DOUBLE_QUOTED_STATE);
            }
            if (character == '\'') {
                pchvml_temp_buffer_reset(
                        hvml->current_token->system_identifier);
                ADVANCE_TO(PCHVML_DOCTYPE_SYSTEM_INFORMATION_SINGLE_QUOTED_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_DOCTYPE);
                pchvml_token_set_force_quirks(hvml->current_token, true);
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            PCHVML_SET_ERROR(
                PCHVML_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_SYSTEM_IDENTIFIER);
            pchvml_token_set_force_quirks(hvml->current_token, true);
            ADVANCE_TO(PCHVML_BOGUS_DOCTYPE_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_AFTER_DOCTYPE_SYSTEM_KEYWORD_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(PCHVML_BEFORE_DOCTYPE_SYSTEM_INFORMATION_STATE);
            }
            if (character == '"') {
                PCHVML_SET_ERROR(
                  PCHVML_ERROR_MISSING_WHITESPACE_AFTER_DOCTYPE_SYSTEM_KEYWORD);
                pchvml_temp_buffer_reset(
                        hvml->current_token->system_identifier);
                ADVANCE_TO(PCHVML_DOCTYPE_SYSTEM_INFORMATION_DOUBLE_QUOTED_STATE);
            }
            if (character == '\'') {
                PCHVML_SET_ERROR(
                  PCHVML_ERROR_MISSING_WHITESPACE_AFTER_DOCTYPE_SYSTEM_KEYWORD);
                pchvml_temp_buffer_reset(
                        hvml->current_token->system_identifier);
                ADVANCE_TO(PCHVML_DOCTYPE_SYSTEM_INFORMATION_SINGLE_QUOTED_STATE);
            }
            if (character == '>') {
                PCHVML_SET_ERROR(PCHVML_ERROR_MISSING_DOCTYPE_SYSTEM_IDENTIFIER);
                pchvml_token_set_force_quirks(hvml->current_token, true);
                RETURN_AND_SWITCH_TO(PCHVML_DATA_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_DOCTYPE);
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            PCHVML_SET_ERROR(
                PCHVML_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_SYSTEM_IDENTIFIER);
            pchvml_token_set_force_quirks(hvml->current_token, true);
            ADVANCE_TO(PCHVML_BOGUS_DOCTYPE_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_BEFORE_DOCTYPE_SYSTEM_INFORMATION_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(PCHVML_BEFORE_DOCTYPE_SYSTEM_INFORMATION_STATE);
            }
            if (character == '"') {
                pchvml_temp_buffer_reset(
                        hvml->current_token->system_identifier);
                ADVANCE_TO(PCHVML_DOCTYPE_SYSTEM_INFORMATION_DOUBLE_QUOTED_STATE);
            }
            if (character == '\'') {
                pchvml_temp_buffer_reset(
                        hvml->current_token->system_identifier);
                ADVANCE_TO(PCHVML_DOCTYPE_SYSTEM_INFORMATION_SINGLE_QUOTED_STATE);
            }
            if (character == '>') {
                PCHVML_SET_ERROR(
                        PCHVML_ERROR_MISSING_DOCTYPE_SYSTEM_IDENTIFIER);
                pchvml_token_set_force_quirks(hvml->current_token, true);
                RETURN_AND_SWITCH_TO(PCHVML_DATA_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_DOCTYPE);
                pchvml_token_set_force_quirks(hvml->current_token, true);
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            PCHVML_SET_ERROR(
                PCHVML_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_SYSTEM_IDENTIFIER);
            pchvml_token_set_force_quirks(hvml->current_token, true);
            ADVANCE_TO(PCHVML_BOGUS_DOCTYPE_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_DOCTYPE_SYSTEM_INFORMATION_DOUBLE_QUOTED_STATE)
            if (character == '"') {
                ADVANCE_TO(PCHVML_AFTER_DOCTYPE_SYSTEM_INFORMATION_STATE);
            }
            if (character == '>') {
                PCHVML_SET_ERROR(PCHVML_ERROR_ABRUPT_DOCTYPE_SYSTEM_IDENTIFIER);
                pchvml_token_set_force_quirks(hvml->current_token, true);
                RETURN_AND_SWITCH_TO(PCHVML_DATA_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_DOCTYPE);
                pchvml_token_set_force_quirks(hvml->current_token, true);
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            pchvml_token_append_to_system_identifier(hvml->current_token,
                    hvml->c, hvml->sz_c);
            ADVANCE_TO(PCHVML_DOCTYPE_SYSTEM_INFORMATION_DOUBLE_QUOTED_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_DOCTYPE_SYSTEM_INFORMATION_SINGLE_QUOTED_STATE)
            if (character == '\'') {
                ADVANCE_TO(PCHVML_AFTER_DOCTYPE_SYSTEM_INFORMATION_STATE);
            }
            if (character == '>') {
                PCHVML_SET_ERROR(PCHVML_ERROR_ABRUPT_DOCTYPE_SYSTEM_IDENTIFIER);
                pchvml_token_set_force_quirks(hvml->current_token, true);
                RETURN_AND_SWITCH_TO(PCHVML_DATA_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_DOCTYPE);
                pchvml_token_set_force_quirks(hvml->current_token, true);
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            pchvml_token_append_to_system_identifier(hvml->current_token,
                    hvml->c, hvml->sz_c);
            ADVANCE_TO(PCHVML_DOCTYPE_SYSTEM_INFORMATION_SINGLE_QUOTED_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_AFTER_DOCTYPE_SYSTEM_INFORMATION_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(PCHVML_AFTER_DOCTYPE_SYSTEM_INFORMATION_STATE);
            }
            if (character == '>') {
                RETURN_AND_SWITCH_TO(PCHVML_DATA_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_DOCTYPE);
                pchvml_token_set_force_quirks(hvml->current_token, true);
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            PCHVML_SET_ERROR(
             PCHVML_ERROR_UNEXPECTED_CHARACTER_AFTER_DOCTYPE_SYSTEM_IDENTIFIER);
            ADVANCE_TO(PCHVML_BOGUS_DOCTYPE_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_BOGUS_DOCTYPE_STATE)
            if (character == '>') {
                RETURN_AND_SWITCH_TO(PCHVML_DATA_STATE);
            }
            if (is_eof(character)) {
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            ADVANCE_TO(PCHVML_BOGUS_DOCTYPE_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_CDATA_SECTION_STATE)
            if (character == ']') {
                ADVANCE_TO(PCHVML_CDATA_SECTION_BRACKET_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_CDATA);
                RECONSUME_IN(PCHVML_DATA_STATE);
            }
            APPEND_TO_CHARACTER(hvml->c, hvml->sz_c);
            ADVANCE_TO(PCHVML_CDATA_SECTION_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_CDATA_SECTION_BRACKET_STATE)
            if (character == ']') {
                ADVANCE_TO(PCHVML_CDATA_SECTION_END_STATE);
            }
            APPEND_TO_CHARACTER("]", 1);
            RECONSUME_IN(PCHVML_CDATA_SECTION_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_CDATA_SECTION_END_STATE)
            if (character == ']') {
                APPEND_TO_CHARACTER("]", 1);
                ADVANCE_TO(PCHVML_CDATA_SECTION_END_STATE);
            }
            if (character == '>') {
                ADVANCE_TO(PCHVML_DATA_STATE);
            }
            APPEND_TO_CHARACTER("]", 1);
            APPEND_TO_CHARACTER("]", 1);
            RECONSUME_IN(PCHVML_CDATA_SECTION_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_CHARACTER_REFERENCE_STATE)
            if (is_ascii_alpha_numeric(character)) {
                RECONSUME_IN(PCHVML_NAMED_CHARACTER_REFERENCE_STATE);
            }
            if (character == '#') {
                APPEND_TEMP_BUFFER(hvml->c, hvml->sz_c);
                SWITCH_TO(PCHVML_NUMERIC_CHARACTER_REFERENCE_STATE);
            }
            RECONSUME_IN(hvml->return_state);
        END_STATE()

        BEGIN_STATE(PCHVML_NAMED_CHARACTER_REFERENCE_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_AMBIGUOUS_AMPERSAND_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_NUMERIC_CHARACTER_REFERENCE_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_HEXADECIMAL_CHARACTER_REFERENCE_START_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_DECIMAL_CHARACTER_REFERENCE_START_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_HEXADECIMAL_CHARACTER_REFERENCE_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_DECIMAL_CHARACTER_REFERENCE_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_NUMERIC_CHARACTER_REFERENCE_END_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_SPECIAL_ATTRIBUTE_OPERATOR_IN_ATTRIBUTE_NAME_STATE)
            if (character == '=') {
                if (pchvml_temp_buffer_is_empty(hvml->temp_buffer)) {
                    pchvml_token_attribute_set_assignment(
                            hvml->current_token,
                            PCHVML_ATTRIBUTE_ASSIGNMENT);
                }
                else {
                    wchar_t op = pchvml_temp_buffer_get_last_char(
                            hvml->temp_buffer);
                    switch (op) {
                        case '+':
                            pchvml_token_attribute_set_assignment(
                                    hvml->current_token,
                                    PCHVML_ATTRIBUTE_ADDITION_ASSIGNMENT);
                            break;
                        case '-':
                            pchvml_token_attribute_set_assignment(
                                    hvml->current_token,
                                    PCHVML_ATTRIBUTE_SUBTRACTION_ASSIGNMENT);
                            break;
                        case '%':
                            pchvml_token_attribute_set_assignment(
                                    hvml->current_token,
                                    PCHVML_ATTRIBUTE_REMAINDER_ASSIGNMENT);
                            break;
                        case '~':
                            pchvml_token_attribute_set_assignment(
                                    hvml->current_token,
                                    PCHVML_ATTRIBUTE_REPLACE_ASSIGNMENT);
                            break;
                        case '^':
                            pchvml_token_attribute_set_assignment(
                                    hvml->current_token,
                                    PCHVML_ATTRIBUTE_HEAD_ASSIGNMENT);
                            break;
                        case '$':
                            pchvml_token_attribute_set_assignment(
                                    hvml->current_token,
                                    PCHVML_ATTRIBUTE_TAIL_ASSIGNMENT);
                            break;
                        default:
                            pchvml_token_attribute_set_assignment(
                                    hvml->current_token,
                                    PCHVML_ATTRIBUTE_ASSIGNMENT);
                            break;
                    }
                }
                SWITCH_TO(PCHVML_BEFORE_ATTRIBUTE_VALUE_STATE);
            }
            // TODO
        END_STATE()

        BEGIN_STATE(PCHVML_SPECIAL_ATTRIBUTE_OPERATOR_AFTER_ATTRIBUTE_NAME_STATE)
            if (character == '=') {
                if (pchvml_temp_buffer_is_empty(hvml->temp_buffer)) {
                    pchvml_token_attribute_set_assignment(
                            hvml->current_token,
                            PCHVML_ATTRIBUTE_ASSIGNMENT);
                }
                else {
                    wchar_t op = pchvml_temp_buffer_get_last_char(
                            hvml->temp_buffer);
                    switch (op) {
                        case '+':
                            pchvml_token_attribute_set_assignment(
                                    hvml->current_token,
                                    PCHVML_ATTRIBUTE_ADDITION_ASSIGNMENT);
                            break;
                        case '-':
                            pchvml_token_attribute_set_assignment(
                                    hvml->current_token,
                                    PCHVML_ATTRIBUTE_SUBTRACTION_ASSIGNMENT);
                            break;
                        case '%':
                            pchvml_token_attribute_set_assignment(
                                    hvml->current_token,
                                    PCHVML_ATTRIBUTE_REMAINDER_ASSIGNMENT);
                            break;
                        case '~':
                            pchvml_token_attribute_set_assignment(
                                    hvml->current_token,
                                    PCHVML_ATTRIBUTE_REPLACE_ASSIGNMENT);
                            break;
                        case '^':
                            pchvml_token_attribute_set_assignment(
                                    hvml->current_token,
                                    PCHVML_ATTRIBUTE_HEAD_ASSIGNMENT);
                            break;
                        case '$':
                            pchvml_token_attribute_set_assignment(
                                    hvml->current_token,
                                    PCHVML_ATTRIBUTE_TAIL_ASSIGNMENT);
                            break;
                        default:
                            pchvml_token_attribute_set_assignment(
                                    hvml->current_token,
                                    PCHVML_ATTRIBUTE_ASSIGNMENT);
                            break;
                    }
                }
                SWITCH_TO(PCHVML_BEFORE_ATTRIBUTE_VALUE_STATE);
            }
            // TODO
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_DATA_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_FINISHED_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_CONTROL_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_LEFT_BRACE_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_RIGHT_BRACE_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_LEFT_BRACKET_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_RIGHT_BRACKET_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_LESS_THAN_SIGN_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_GREATER_THAN_SIGN_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_LEFT_PARENTHESIS_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_RIGHT_PARENTHESIS_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_DOLLAR_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_AFTER_VALUE_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_BEFORE_NAME_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_AFTER_NAME_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_NAME_UNQUOTED_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_NAME_SINGLE_QUOTED_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_NAME_DOUBLE_QUOTED_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_VALUE_SINGLE_QUOTED_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_VALUE_DOUBLE_QUOTED_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_AFTER_VALUE_DOUBLE_QUOTED_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_VALUE_TWO_DOUBLE_QUOTED_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_VALUE_THREE_DOUBLE_QUOTED_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_KEYWORD_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_AFTER_KEYWORD_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_BYTE_SEQUENCE_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_AFTER_BYTE_SEQUENCE_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_HEX_BYTE_SEQUENCE_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_BINARY_BYTE_SEQUENCE_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_BASE64_BYTE_SEQUENCE_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_VALUE_NUMBER_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_AFTER_VALUE_NUMBER_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_VALUE_NUMBER_INTEGER_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_VALUE_NUMBER_FRACTION_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_VALUE_NUMBER_EXPONENT_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_VALUE_NUMBER_EXPONENT_INTEGER_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_VALUE_NUMBER_SUFFIX_INTEGER_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_VALUE_NUMBER_INFINITY_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_VALUE_NAN_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_STRING_ESCAPE_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_STRING_ESCAPE_FOUR_HEXADECIMAL_DIGITS_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_JSONEE_VARIABLE_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_JSONEE_FULL_STOP_SIGN_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_JSONEE_KEYWORD_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_JSONEE_STRING_STATE)
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_AFTER_JSONEE_STRING_STATE)
        END_STATE()

        default:
            break;
    }
    return NULL;
}

