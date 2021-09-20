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

#include "hvml-buffer.h"
#include "hvml-rwswrap.h"
#include "hvml-token.h"
#include "hvml-sbst.h"
#include "config.h"

#include <math.h>

#if HAVE(GLIB)
#include <gmodule.h>
#else
#include <stdlib.h>
#endif

//#define HVML_DEBUG_PRINT

#define PCHVML_END_OF_FILE       0

#if HAVE(GLIB)
#define    PCHVML_ALLOC(sz)   g_slice_alloc0(sz)
#define    PCHVML_FREE(p)     g_slice_free1(sizeof(*p), (gpointer)p)
#else
#define    PCHVML_ALLOC(sz)   calloc(1, sz)
#define    PCHVML_FREE(p)     free(p)
#endif

#ifdef HVML_DEBUG_PRINT
#define PRINT_STATE(state_name)                                             \
    fprintf(stderr, "in %s|wc=%c|hex=0x%X\n",                               \
            pchvml_pchvml_state_desc(state_name), character, character);
#define PCHVML_SET_ERROR(err)    do {                                       \
    fprintf(stderr, "error %s:%d %s\n", __FILE__, __LINE__,                 \
            pchvml_error_desc(err));                                        \
    pcinst_set_error (err);                                                 \
} while (0)
#else
#define PRINT_STATE(state_name)
#define PCHVML_SET_ERROR(err)    pcinst_set_error(err)
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

#define SET_RETURN_STATE(new_state)                                         \
    do {                                                                    \
        hvml->return_state = new_state;                                     \
    } while (false)

#define RECONSUME_IN(new_state)                                             \
    do {                                                                    \
        hvml->state = new_state;                                            \
        goto next_state;                                                    \
    } while (false)

#define RECONSUME_IN_NEXT(new_state)                                        \
    do {                                                                    \
        hvml->state = new_state;                                            \
        pchvml_rwswrap_buffer_chars(hvml->rwswrap, &character, 1);          \
    } while (false)

#define ADVANCE_TO(new_state)                                               \
    do {                                                                    \
        hvml->state = new_state;                                            \
        goto next_input;                                                    \
    } while (false)

#define SWITCH_TO(new_state)                                                \
    do {                                                                    \
        hvml->state = new_state;                                            \
    } while (false)

#define RETURN_IN_CURRENT_STATE(expression)                                 \
    do {                                                                    \
        hvml->state = curr_state;                                           \
        pchvml_rwswrap_buffer_chars(hvml->rwswrap, &character, 1);          \
        if (expression) {                                                   \
            pchvml_parser_save_appropriate_tag_name(hvml);                  \
            pchvml_token_done(hvml->token);                                 \
            struct pchvml_token* token = hvml->token;                       \
            hvml->token = NULL;                                             \
            return token;                                                   \
        }                                                                   \
        return NULL;                                                        \
    } while (false)

#define RETURN_AND_SWITCH_TO(next_state)                                    \
    do {                                                                    \
        hvml->state = next_state;                                           \
        pchvml_parser_save_appropriate_tag_name(hvml);                      \
        pchvml_token_done(hvml->token);                                     \
        struct pchvml_token* token = hvml->token;                           \
        hvml->token = NULL;                                                 \
        return token;                                                       \
    } while (false)

#define RETURN_AND_RECONSUME_IN(next_state)                                 \
    do {                                                                    \
        hvml->state = next_state;                                           \
        pchvml_parser_save_appropriate_tag_name(hvml);                      \
        pchvml_token_done(hvml->token);                                     \
        struct pchvml_token* token = hvml->token;                           \
        hvml->token = NULL;                                                 \
        return token;                                                       \
    } while (false)

#define RETURN_AND_STOP_PARSE()                                             \
    do {                                                                    \
        return NULL;                                                        \
    } while (false)

#define RETURN_NEW_EOF_TOKEN()                                              \
    do {                                                                    \
        if (hvml->token) {                                                  \
            struct pchvml_token* token = hvml->token;                       \
            hvml->token = pchvml_token_new_eof();                           \
            return token;                                                   \
        }                                                                   \
        return pchvml_token_new_eof();                                      \
    } while (false)

#define RETURN_CACHED_TOKEN()                                               \
    do {                                                                    \
        if (hvml->token) {                                                  \
            struct pchvml_token* token = hvml->token;                       \
            hvml->token = NULL;                                             \
            return token;                                                   \
        }                                                                   \
    } while (false)

#define STATE_DESC(state_name)                                              \
    case state_name:                                                        \
        return ""#state_name;                                               \

#define APPEND_TO_TOKEN_NAME(uc)                                            \
    do {                                                                    \
        pchvml_token_append_to_name(hvml->token, uc);                       \
    } while (false)

#define APPEND_TO_TOKEN_TEXT(uc)                                            \
    do {                                                                    \
        if (hvml->token == NULL) {                                          \
            hvml->token = pchvml_token_new (PCHVML_TOKEN_CHARACTER);        \
        }                                                                   \
        pchvml_token_append_to_text(hvml->token, uc);                       \
    } while (false)

#define APPEND_BYTES_TO_TOKEN_TEXT(c, nr_c)                                 \
    do {                                                                    \
        pchvml_token_append_bytes_to_text(hvml->token, c, nr_c);            \
    } while (false)

#define APPEND_TEMP_BUFFER_TO_TOKEN_TEXT()                                  \
    do {                                                                    \
        const char* c = pchvml_buffer_get_buffer(hvml->temp_buffer);   \
        size_t nr_c = pchvml_buffer_get_size_in_bytes(                 \
                hvml->temp_buffer);                                         \
        pchvml_token_append_bytes_to_text(hvml->token, c, nr_c);            \
        pchvml_buffer_reset(hvml->temp_buffer);                        \
    } while (false)

#define APPEND_TO_TOKEN_PUBLIC_IDENTIFIER(uc)                               \
    do {                                                                    \
        pchvml_token_append_to_public_identifier(hvml->token, uc);          \
    } while (false)

#define RESET_TOKEN_PUBLIC_IDENTIFIER()                                     \
    do {                                                                    \
        pchvml_token_reset_public_identifier(hvml->token);                  \
    } while (false)

#define APPEND_TO_TOKEN_SYSTEM_INFORMATION(uc)                              \
    do {                                                                    \
        pchvml_token_append_to_system_information(hvml->token, uc);         \
    } while (false)

#define RESET_TOKEN_SYSTEM_INFORMATION()                                    \
    do {                                                                    \
        pchvml_token_reset_system_information(hvml->token);                 \
    } while (false)

#define APPEND_TO_TOKEN_ATTR_NAME(uc)                                       \
    do {                                                                    \
        pchvml_token_append_to_attr_name(hvml->token, uc);                  \
    } while (false)

#define APPEND_TEMP_BUFFER_TO_TOKEN_ATTR_NAME()                             \
    do {                                                                    \
        const char* c = pchvml_buffer_get_buffer(hvml->temp_buffer);   \
        size_t nr_c = pchvml_buffer_get_size_in_bytes(                 \
                hvml->temp_buffer);                                         \
        pchvml_token_append_bytes_to_attr_name(hvml->token, c, nr_c);       \
        pchvml_buffer_reset(hvml->temp_buffer);                        \
    } while (false)

#define BEGIN_TOKEN_ATTR()                                                  \
    do {                                                                    \
        pchvml_token_begin_attr(hvml->token);                               \
    } while (false)

#define END_TOKEN_ATTR()                                                    \
    do {                                                                    \
        pchvml_token_end_attr(hvml->token);                                 \
    } while (false)

#define APPEND_TO_TOKEN_ATTR_VALUE(uc)                                      \
    do {                                                                    \
        pchvml_token_append_to_attr_value(hvml->token, uc);                 \
    } while (false)

#define APPEND_BYTES_TO_TOKEN_ATTR_VALUE(c, nr_c)                           \
    do {                                                                    \
        pchvml_token_append_bytes_to_attr_value(hvml->token, c, nr_c);      \
    } while (false)

#define APPEND_BUFFER_TO_TOKEN_ATTR_VALUE(buffer)                           \
    do {                                                                    \
        const char* c = pchvml_buffer_get_buffer(buffer);                   \
        size_t nr_c = pchvml_buffer_get_size_in_bytes(buffer);              \
        pchvml_token_append_bytes_to_attr_value(hvml->token, c, nr_c);      \
    } while (false)

#define RESET_TEMP_BUFFER()                                                 \
    do {                                                                    \
        pchvml_buffer_reset(hvml->temp_buffer);                             \
    } while (false)

#define APPEND_TO_TEMP_BUFFER(uc)                                           \
    do {                                                                    \
        pchvml_buffer_append(hvml->temp_buffer, uc);                        \
    } while (false)

#define APPEND_BYTES_TO_TEMP_BUFFER(bytes, nr_bytes)                        \
    do {                                                                    \
        pchvml_buffer_append_bytes(hvml->temp_buffer, bytes, nr_bytes);     \
    } while (false)

#define APPEND_BUFFER_TO_TEMP_BUFFER(buffer)                                \
    do {                                                                    \
        pchvml_buffer_append_temp_buffer(hvml->temp_buffer, buffer);        \
    } while (false)

#define APPEND_TO_ESCAPE_BUFFER(uc)                                         \
    do {                                                                    \
        pchvml_buffer_append(hvml->escape_buffer, uc);                      \
    } while (false)

#define RESET_STRING_BUFFER()                                               \
    do {                                                                    \
        pchvml_buffer_reset(hvml->string_buffer);                           \
    } while (false)

#define APPEND_TO_STRING_BUFFER(uc)                                         \
    do {                                                                    \
        pchvml_buffer_append(hvml->string_buffer, uc);                      \
    } while (false)

#define SET_VCM_NODE(node)                                                  \
    do {                                                                    \
        hvml->vcm_node = node;                                              \
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
    /* PCHVML_ERROR_MISSING_WHITESPACE_BETWEEN_DOCTYPE_PUBLIC_AND_SYSTEM\
     _INFORMATIONS */
    "pchvml error missing whitespace between doctype public and system\
        informations",
    /* PCHVML_ERROR_MISSING_WHITESPACE_AFTER_DOCTYPE_SYSTEM_KEYWORD */
    "pchvml error missing whitespace after doctype system keyword",
    /* PCHVML_ERROR_MISSING_DOCTYPE_SYSTEM_INFORMATION */
    "pchvml error missing doctype system information",
    /* PCHVML_ERROR_ABRUPT_DOCTYPE_SYSTEM_INFORMATION */
    "pchvml error abrupt doctype system information",
    /* PCHVML_ERROR_UNEXPECTED_CHARACTER_AFTER_DOCTYPE_SYSTEM_INFORMATION */
    "pchvml error unexpected character after doctype system information",
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
    /* PCHVML_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_SYSTEM_INFORMATION */
    "pchvml error missing quote before doctype system information",
    /* PCHVML_ERROR_MISSING_SEMICOLON_AFTER_CHARACTER_REFERENCE */
    "pchvml error missing semicolon after character reference",
    /* PCHVML_ERROR_CHARACTER_REFERENCE_OUTSIDE_UNICODE_RANGE */
    "pchvml error character reference outside unicode range",
    /* PCHVML_ERROR_SURROGATE_CHARACTER_REFERENCE */
    "pchvml error surrogate character reference",
    /* PCHVML_ERROR_NONCHARACTER_CHARACTER_REFERENCE */
    "pchvml error noncharacter character reference",
    /* PCHVML_ERROR_NULL_CHARACTER_REFERENCE */
    "pchvml error null character reference",
    /* PCHVML_ERROR_CONTROL_CHARACTER_REFERENCE*/
    "pchvml error control character reference",
    /* PCHVML_ERROR_INVALID_UTF8_CHARACTER */
    "pchvml error invalid utf8 character",
};

static struct err_msg_seg _hvml_err_msgs_seg = {
    { NULL, NULL },
    PURC_ERROR_FIRST_HVML,
    PURC_ERROR_FIRST_HVML + PCA_TABLESIZE(hvml_err_msgs) - 1,
    hvml_err_msgs
};

static const uint32_t numeric_char_ref_extension_array[32] = {
    0x20AC, 0x0081, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, // 80-87
    0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008D, 0x017D, 0x008F, // 88-8F
    0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, // 90-97
    0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x009D, 0x017E, 0x0178, // 98-9F
};

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

PCA_INLINE UNUSED_FUNCTION bool is_eof (uint32_t uc)
{
    return uc == PCHVML_END_OF_FILE;
}

void pchvml_init_once(void)
{
    pcinst_register_error_message_segment(&_hvml_err_msgs_seg);
}

struct pchvml_parser* pchvml_create(uint32_t flags, size_t queue_size)
{
    UNUSED_PARAM(flags);
    UNUSED_PARAM(queue_size);

    struct pchvml_parser* parser = (struct pchvml_parser*) PCHVML_ALLOC(
            sizeof(struct pchvml_parser));
    parser->state = PCHVML_DATA_STATE;
    parser->rwswrap = pchvml_rwswrap_new ();
    parser->temp_buffer = pchvml_buffer_new ();
    parser->appropriate_tag_name = pchvml_buffer_new ();
    parser->escape_buffer = pchvml_buffer_new ();
    parser->string_buffer = pchvml_buffer_new ();
    parser->vcm_stack = pcvcm_stack_new();
    parser->ejson_stack = pcutils_stack_new(0);
    return parser;
}

void pchvml_reset(struct pchvml_parser* parser, uint32_t flags,
        size_t queue_size)
{
    UNUSED_PARAM(flags);
    UNUSED_PARAM(queue_size);

    parser->state = PCHVML_DATA_STATE;
    pchvml_rwswrap_destroy (parser->rwswrap);
    parser->rwswrap = pchvml_rwswrap_new ();
    pchvml_buffer_reset (parser->temp_buffer);
    pchvml_buffer_reset (parser->appropriate_tag_name);
    pchvml_buffer_reset (parser->escape_buffer);
    pchvml_buffer_reset (parser->string_buffer);

    struct pcvcm_node* n = parser->vcm_node;
    parser->vcm_node = NULL;
    while (!pcvcm_stack_is_empty(parser->vcm_stack)) {
        struct pcvcm_node* node = pcvcm_stack_pop(parser->vcm_stack);
        if (n) {
            pctree_node_append_child((struct pctree_node*)node,
                    (struct pctree_node*)n);
        }
        n = node;
    }
    pcvcm_node_destroy(n);
    pcvcm_stack_destroy(parser->vcm_stack);
    parser->vcm_stack = pcvcm_stack_new();
    pcutils_stack_destroy(parser->ejson_stack);
    parser->ejson_stack = pcutils_stack_new(0);
    if (parser->token) {
        pchvml_token_destroy(parser->token);
        parser->token = NULL;
    }
}

void pchvml_destroy(struct pchvml_parser* parser)
{
    if (parser) {
        pchvml_rwswrap_destroy (parser->rwswrap);
        pchvml_buffer_destroy (parser->temp_buffer);
        pchvml_buffer_destroy (parser->appropriate_tag_name);
        pchvml_buffer_destroy (parser->escape_buffer);
        pchvml_buffer_destroy (parser->string_buffer);
        if (parser->sbst) {
            pchvml_sbst_destroy(parser->sbst);
        }
        struct pcvcm_node* n = parser->vcm_node;
        parser->vcm_node = NULL;
        while (!pcvcm_stack_is_empty(parser->vcm_stack)) {
            struct pcvcm_node* node = pcvcm_stack_pop(parser->vcm_stack);
            if (n) {
                pctree_node_append_child((struct pctree_node*)node,
                        (struct pctree_node*)n);
            }
            n = node;
        }
        pcvcm_node_destroy(n);
        pcvcm_stack_destroy(parser->vcm_stack);
        pcutils_stack_destroy(parser->ejson_stack);
        if (parser->token) {
            pchvml_token_destroy(parser->token);
        }
        PCHVML_FREE(parser);
    }
}

const char* pchvml_error_desc (enum pchvml_error err)
{
    switch (err) {
    STATE_DESC(PCHVML_SUCCESS)
    STATE_DESC(PCHVML_ERROR_UNEXPECTED_NULL_CHARACTER)
    STATE_DESC(PCHVML_ERROR_UNEXPECTED_QUESTION_MARK_INSTEAD_OF_TAG_NAME)
    STATE_DESC(PCHVML_ERROR_EOF_BEFORE_TAG_NAME)
    STATE_DESC(PCHVML_ERROR_MISSING_END_TAG_NAME)
    STATE_DESC(PCHVML_ERROR_INVALID_FIRST_CHARACTER_OF_TAG_NAME)
    STATE_DESC(PCHVML_ERROR_EOF_IN_TAG)
    STATE_DESC(PCHVML_ERROR_UNEXPECTED_EQUALS_SIGN_BEFORE_ATTRIBUTE_NAME)
    STATE_DESC(PCHVML_ERROR_UNEXPECTED_CHARACTER_IN_ATTRIBUTE_NAME)
    STATE_DESC(PCHVML_ERROR_UNEXPECTED_CHARACTER_IN_UNQUOTED_ATTRIBUTE_VALUE)
    STATE_DESC(PCHVML_ERROR_MISSING_WHITESPACE_BETWEEN_ATTRIBUTES)
    STATE_DESC(PCHVML_ERROR_UNEXPECTED_SOLIDUS_IN_TAG)
    STATE_DESC(PCHVML_ERROR_CDATA_IN_HTML_CONTENT)
    STATE_DESC(PCHVML_ERROR_INCORRECTLY_OPENED_COMMENT)
    STATE_DESC(PCHVML_ERROR_ABRUPT_CLOSING_OF_EMPTY_COMMENT)
    STATE_DESC(PCHVML_ERROR_EOF_IN_COMMENT)
    STATE_DESC(PCHVML_ERROR_EOF_IN_DOCTYPE)
    STATE_DESC(PCHVML_ERROR_MISSING_WHITESPACE_BEFORE_DOCTYPE_NAME)
    STATE_DESC(PCHVML_ERROR_MISSING_DOCTYPE_NAME)
    STATE_DESC(PCHVML_ERROR_INVALID_CHARACTER_SEQUENCE_AFTER_DOCTYPE_NAME)
    STATE_DESC(PCHVML_ERROR_MISSING_WHITESPACE_AFTER_DOCTYPE_PUBLIC_KEYWORD)
    STATE_DESC(PCHVML_ERROR_MISSING_DOCTYPE_PUBLIC_IDENTIFIER)
    STATE_DESC(PCHVML_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_PUBLIC_IDENTIFIER)
    STATE_DESC(PCHVML_ERROR_ABRUPT_DOCTYPE_PUBLIC_IDENTIFIER)
    STATE_DESC(PCHVML_ERROR_MISSING_WHITESPACE_BETWEEN_DOCTYPE_PUBLIC_AND_SYSTEM_INFORMATIONS)
    STATE_DESC(PCHVML_ERROR_MISSING_WHITESPACE_AFTER_DOCTYPE_SYSTEM_KEYWORD)
    STATE_DESC(PCHVML_ERROR_MISSING_DOCTYPE_SYSTEM_INFORMATION)
    STATE_DESC(PCHVML_ERROR_ABRUPT_DOCTYPE_SYSTEM_INFORMATION)
    STATE_DESC(PCHVML_ERROR_UNEXPECTED_CHARACTER_AFTER_DOCTYPE_SYSTEM_INFORMATION)
    STATE_DESC(PCHVML_ERROR_EOF_IN_CDATA)
    STATE_DESC(PCHVML_ERROR_UNKNOWN_NAMED_CHARACTER_REFERENCE)
    STATE_DESC(PCHVML_ERROR_ABSENCE_OF_DIGITS_IN_NUMERIC_CHARACTER_REFERENCE)
    STATE_DESC(PCHVML_ERROR_UNEXPECTED_CHARACTER)
    STATE_DESC(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER_EXPONENT)
    STATE_DESC(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER_FRACTION)
    STATE_DESC(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER_INTEGER)
    STATE_DESC(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER)
    STATE_DESC(PCHVML_ERROR_UNEXPECTED_RIGHT_BRACE)
    STATE_DESC(PCHVML_ERROR_UNEXPECTED_RIGHT_BRACKET)
    STATE_DESC(PCHVML_ERROR_UNEXPECTED_JSON_KEY_NAME)
    STATE_DESC(PCHVML_ERROR_UNEXPECTED_COMMA)
    STATE_DESC(PCHVML_ERROR_UNEXPECTED_JSON_KEYWORD)
    STATE_DESC(PCHVML_ERROR_UNEXPECTED_BASE64)
    STATE_DESC(PCHVML_ERROR_BAD_JSON_NUMBER)
    STATE_DESC(PCHVML_ERROR_BAD_JSON_STRING_ESCAPE_ENTITY)
    STATE_DESC(PCHVML_ERROR_BAD_JSONEE)
    STATE_DESC(PCHVML_ERROR_BAD_JSONEE_ESCAPE_ENTITY)
    STATE_DESC(PCHVML_ERROR_BAD_JSONEE_VARIABLE_NAME)
    STATE_DESC(PCHVML_ERROR_EMPTY_JSONEE_NAME)
    STATE_DESC(PCHVML_ERROR_BAD_JSONEE_NAME)
    STATE_DESC(PCHVML_ERROR_BAD_JSONEE_KEYWORD)
    STATE_DESC(PCHVML_ERROR_EMPTY_JSONEE_KEYWORD)
    STATE_DESC(PCHVML_ERROR_BAD_JSONEE_UNEXPECTED_COMMA)
    STATE_DESC(PCHVML_ERROR_BAD_JSONEE_UNEXPECTED_PARENTHESIS)
    STATE_DESC(PCHVML_ERROR_BAD_JSONEE_UNEXPECTED_LEFT_ANGLE_BRACKET)
    STATE_DESC(PCHVML_ERROR_MISSING_MISSING_ATTRIBUTE_VALUE)
    STATE_DESC(PCHVML_ERROR_NESTED_COMMENT)
    STATE_DESC(PCHVML_ERROR_INCORRECTLY_CLOSED_COMMENT)
    STATE_DESC(PCHVML_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_SYSTEM_INFORMATION)
    STATE_DESC(PCHVML_ERROR_MISSING_SEMICOLON_AFTER_CHARACTER_REFERENCE)
    STATE_DESC(PCHVML_ERROR_CHARACTER_REFERENCE_OUTSIDE_UNICODE_RANGE)
    STATE_DESC(PCHVML_ERROR_SURROGATE_CHARACTER_REFERENCE)
    STATE_DESC(PCHVML_ERROR_NONCHARACTER_CHARACTER_REFERENCE)
    STATE_DESC(PCHVML_ERROR_NULL_CHARACTER_REFERENCE)
    STATE_DESC(PCHVML_ERROR_CONTROL_CHARACTER_REFERENCE)
    STATE_DESC(PCHVML_ERROR_INVALID_UTF8_CHARACTER)
    }
    return NULL;
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
        STATE_DESC(
          PCHVML_BETWEEN_DOCTYPE_PUBLIC_IDENTIFIER_AND_SYSTEM_INFORMATION_STATE
        )
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

void pchvml_parser_save_appropriate_tag_name (struct pchvml_parser* hvml)
{
    if (pchvml_token_is_type (hvml->token, PCHVML_TOKEN_START_TAG)) {
        const char* name = pchvml_token_get_name(hvml->token);
        pchvml_buffer_append_bytes(hvml->appropriate_tag_name,
                name, strlen(name));
    }
}

void pchvml_parser_reset_appropriate_tag_name (struct pchvml_parser* hvml)
{
    pchvml_buffer_reset(hvml->appropriate_tag_name);
}

bool pchvml_parser_is_appropriate_end_tag (struct pchvml_parser* hvml)
{
    const char* name = pchvml_token_get_name(hvml->token);
    return pchvml_buffer_equal_to (hvml->appropriate_tag_name, name,
            strlen(name));
}

bool pchvml_parser_is_operation_tag_token (struct pchvml_token* token)
{
    UNUSED_PARAM(token);
    // TODO
    return true;
}

bool pchvml_parser_is_ordinary_attribute (struct pchvml_token_attr* attr)
{
    UNUSED_PARAM(attr);
    // TODO
    return true;
}

bool pchvml_parser_is_preposition_attribute (
        struct pchvml_token_attr* attr)
{
    UNUSED_PARAM(attr);
    // TODO
    return true;
}

bool pchvml_parse_is_adjusted_current_node (struct pchvml_parser* hvml)
{
    UNUSED_PARAM(hvml);
    // TODO
    return false;
}

bool pchvml_parse_is_not_in_hvml_namespace (struct pchvml_parser* hvml)
{
    UNUSED_PARAM(hvml);
    // TODO
    return false;
}

struct pcvcm_node* pchvml_parser_new_byte_sequence (struct pchvml_parser* hvml,
    struct pchvml_buffer* buffer)
{
    UNUSED_PARAM(hvml);
    UNUSED_PARAM(buffer);
    size_t nr_bytes = pchvml_buffer_get_size_in_bytes(buffer);
    const char* bytes = pchvml_buffer_get_buffer(buffer);
    if (bytes[1] == 'x') {
        return pcvcm_node_new_byte_sequence_from_bx(bytes + 2, nr_bytes - 2);
    }
    else if (bytes[1] == 'b') {
        return pcvcm_node_new_byte_sequence_from_bb(bytes + 2, nr_bytes - 2);
    }
    else if (bytes[1] == '6') {
        return pcvcm_node_new_byte_sequence_from_b64(bytes + 3, nr_bytes - 3);
    }
    return NULL;
}

bool pchvml_parser_is_in_attribute (struct pchvml_parser* parser)
{
    return parser->token && pchvml_token_is_in_attr(parser->token);
}

struct pchvml_token* pchvml_next_token (struct pchvml_parser* hvml,
                                          purc_rwstream_t rws)
{
    uint32_t character = 0;
    RETURN_CACHED_TOKEN();

    pchvml_rwswrap_set_rwstream (hvml->rwswrap, rws);

next_input:
    character = pchvml_rwswrap_next_char (hvml->rwswrap);
    if (character == 0xFFFFFFFF) {
        PCHVML_SET_ERROR(PCHVML_ERROR_INVALID_UTF8_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }

next_state:
    switch (hvml->state) {
        BEGIN_STATE(PCHVML_DATA_STATE)
            if (character == '&') {
                SET_RETURN_STATE(PCHVML_DATA_STATE);
                ADVANCE_TO(PCHVML_CHARACTER_REFERENCE_STATE);
            }
            if (character == '<') {
                if (pchvml_token_is_type(hvml->token,
                            PCHVML_TOKEN_CHARACTER)) {
                    RETURN_IN_CURRENT_STATE(true);
                }
                ADVANCE_TO(PCHVML_TAG_OPEN_STATE);
            }
            if (is_eof(character)) {
                RETURN_NEW_EOF_TOKEN();
            }
            APPEND_TO_TOKEN_TEXT(character);
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
            APPEND_TO_TOKEN_TEXT(character);
            ADVANCE_TO(PCHVML_TOKEN_CHARACTER);
        END_STATE()

        BEGIN_STATE(PCHVML_RAWTEXT_STATE)
            if (character == '<') {
                ADVANCE_TO(PCHVML_RAWTEXT_LESS_THAN_SIGN_STATE);
            }
            if (is_eof(character)) {
                RETURN_NEW_EOF_TOKEN();
            }

            APPEND_TO_TOKEN_TEXT(character);
            ADVANCE_TO(PCHVML_RAWTEXT_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_PLAINTEXT_STATE)
            if (is_eof(character)) {
                RETURN_NEW_EOF_TOKEN();
            }

            APPEND_TO_TOKEN_TEXT(character);
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
                hvml->token = pchvml_token_new_start_tag ();
                RECONSUME_IN(PCHVML_TAG_NAME_STATE);
            }
            if (character == '?') {
                PCHVML_SET_ERROR(
                    PCHVML_ERROR_UNEXPECTED_QUESTION_MARK_INSTEAD_OF_TAG_NAME);
                hvml->token = pchvml_token_new_comment();
                RECONSUME_IN(PCHVML_BOGUS_COMMENT_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_BEFORE_TAG_NAME);
                APPEND_TO_TOKEN_TEXT('<');
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            PCHVML_SET_ERROR(PCHVML_ERROR_INVALID_FIRST_CHARACTER_OF_TAG_NAME);
            APPEND_TO_TOKEN_TEXT('<');
            RECONSUME_IN(PCHVML_DATA_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_END_TAG_OPEN_STATE)
            if (is_ascii_alpha(character)) {
                hvml->token = pchvml_token_new_end_tag();
                pchvml_parser_reset_appropriate_tag_name(hvml);
                RECONSUME_IN(PCHVML_TAG_NAME_STATE);
            }
            if (character == '>') {
                PCHVML_SET_ERROR(PCHVML_ERROR_MISSING_END_TAG_NAME);
                ADVANCE_TO(PCHVML_DATA_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_BEFORE_TAG_NAME);
                APPEND_TO_TOKEN_TEXT('<');
                APPEND_TO_TOKEN_TEXT('/');
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            PCHVML_SET_ERROR(PCHVML_ERROR_INVALID_FIRST_CHARACTER_OF_TAG_NAME);
            hvml->token = pchvml_token_new_comment();
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
            APPEND_TO_TOKEN_NAME(character);
            ADVANCE_TO(PCHVML_TAG_NAME_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_RCDATA_LESS_THAN_SIGN_STATE)
            if (character == '/') {
                RESET_TEMP_BUFFER();
                ADVANCE_TO(PCHVML_RCDATA_END_TAG_OPEN_STATE);
            }
            APPEND_TO_TOKEN_TEXT('<');
            RECONSUME_IN(PCHVML_RCDATA_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_RCDATA_END_TAG_OPEN_STATE)
            if (is_ascii_alpha(character)) {
                hvml->token = pchvml_token_new_end_tag();
                RECONSUME_IN(PCHVML_RCDATA_END_TAG_NAME_STATE);
            }
            APPEND_TO_TOKEN_TEXT('<');
            APPEND_TO_TOKEN_TEXT('/');
            RECONSUME_IN(PCHVML_RCDATA_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_RCDATA_END_TAG_NAME_STATE)
            if (is_ascii_alpha(character)) {
                APPEND_TO_TOKEN_NAME(character);
                APPEND_TO_TEMP_BUFFER(character);
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
            APPEND_TO_TOKEN_TEXT('<');
            APPEND_TO_TOKEN_TEXT('/');
            APPEND_TEMP_BUFFER_TO_TOKEN_TEXT();
            RECONSUME_IN(PCHVML_RCDATA_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_RAWTEXT_LESS_THAN_SIGN_STATE)
            if (character == '/') {
                RESET_TEMP_BUFFER();
                ADVANCE_TO(PCHVML_RAWTEXT_END_TAG_OPEN_STATE);
            }
            APPEND_TO_TOKEN_TEXT('<');
            RECONSUME_IN(PCHVML_RAWTEXT_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_RAWTEXT_END_TAG_OPEN_STATE)
            if (is_ascii_alpha(character)) {
                hvml->token = pchvml_token_new_end_tag();
                RECONSUME_IN(PCHVML_RAWTEXT_END_TAG_NAME_STATE);
            }
            APPEND_TO_TOKEN_TEXT('<');
            APPEND_TO_TOKEN_TEXT('/');
            RECONSUME_IN(PCHVML_RAWTEXT_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_RAWTEXT_END_TAG_NAME_STATE)
            if (is_ascii_alpha(character)) {
                APPEND_TO_TOKEN_NAME(character);
                APPEND_TO_TEMP_BUFFER(character);
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
            APPEND_TO_TOKEN_TEXT('<');
            APPEND_TO_TOKEN_TEXT('/');
            APPEND_TEMP_BUFFER_TO_TOKEN_TEXT();
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
                BEGIN_TOKEN_ATTR();
                APPEND_TO_TOKEN_ATTR_NAME(character);
                ADVANCE_TO(PCHVML_ATTRIBUTE_NAME_STATE);
            }
            BEGIN_TOKEN_ATTR();
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
                APPEND_TO_TOKEN_ATTR_NAME(character);
            }
            if (character == '$' || character == '%' || character == '+'
                    || character == '-' || character == '^'
                    || character == '~') {
                if (pchvml_parser_is_operation_tag_token(hvml->token)
                        && pchvml_parser_is_ordinary_attribute(
                            pchvml_token_get_curr_attr(hvml->token))) {
                    RESET_TEMP_BUFFER();
                    APPEND_TO_TOKEN_TEXT(character);
                    SWITCH_TO(
                    PCHVML_SPECIAL_ATTRIBUTE_OPERATOR_IN_ATTRIBUTE_NAME_STATE);
                }
            }
            APPEND_TO_TOKEN_ATTR_NAME(character);
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
                END_TOKEN_ATTR();
                RETURN_AND_SWITCH_TO(PCHVML_DATA_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_TAG);
                RETURN_NEW_EOF_TOKEN();
            }
            if (character == '$' || character == '%' || character == '+'
                    || character == '-' || character == '^'
                    || character == '~') {
                if (pchvml_parser_is_operation_tag_token(hvml->token)
                        && pchvml_parser_is_ordinary_attribute(
                            pchvml_token_get_curr_attr(hvml->token))) {
                    RESET_TEMP_BUFFER();
                    APPEND_TO_TOKEN_TEXT(character);
                    SWITCH_TO(
                    PCHVML_SPECIAL_ATTRIBUTE_OPERATOR_AFTER_ATTRIBUTE_NAME_STATE
                    );
                }
            }
            if (pchvml_parser_is_operation_tag_token(hvml->token)
                && pchvml_parser_is_preposition_attribute(
                        pchvml_token_get_curr_attr(hvml->token))) {
                ADVANCE_TO(PCHVML_BEFORE_ATTRIBUTE_VALUE_STATE);
            }
            BEGIN_TOKEN_ATTR();
            RECONSUME_IN(PCHVML_ATTRIBUTE_NAME_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_BEFORE_ATTRIBUTE_VALUE_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(PCHVML_BEFORE_ATTRIBUTE_VALUE_STATE);
            }
            if (character == '"') {
                RESET_STRING_BUFFER();
                ADVANCE_TO(PCHVML_ATTRIBUTE_VALUE_DOUBLE_QUOTED_STATE);
            }
            if (character == '&') {
                RESET_STRING_BUFFER();
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
            RESET_STRING_BUFFER();
            RECONSUME_IN(PCHVML_ATTRIBUTE_VALUE_UNQUOTED_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_ATTRIBUTE_VALUE_DOUBLE_QUOTED_STATE)
            if (character == '"') {
                if (!pchvml_buffer_is_empty(hvml->string_buffer)) {
                    APPEND_BUFFER_TO_TOKEN_ATTR_VALUE(hvml->string_buffer);
                    RESET_STRING_BUFFER();
                }
                END_TOKEN_ATTR();
                ADVANCE_TO(PCHVML_AFTER_ATTRIBUTE_VALUE_QUOTED_STATE);
            }
            if (character == '&') {
                SET_RETURN_STATE(PCHVML_ATTRIBUTE_VALUE_DOUBLE_QUOTED_STATE);
                ADVANCE_TO(PCHVML_CHARACTER_REFERENCE_STATE);
            }
            if (character == '$') {
                SET_VCM_NODE(pcvcm_node_new_concat_string(0, NULL));
                pcutils_stack_push(hvml->ejson_stack, '"');
                if (!pchvml_buffer_is_empty(hvml->string_buffer)) {
                    struct pcvcm_node* node = pcvcm_node_new_string(
                            pchvml_buffer_get_buffer(hvml->string_buffer)
                            );
                    pctree_node_append_child(
                            (struct pctree_node*)hvml->vcm_node,
                            (struct pctree_node*)node);
                    RESET_STRING_BUFFER();
                }
                RECONSUME_IN(PCHVML_EJSON_DATA_STATE);
            }
            if (is_eof(character)) {
                if (!pchvml_buffer_is_empty(hvml->string_buffer)) {
                    APPEND_BUFFER_TO_TOKEN_ATTR_VALUE(hvml->string_buffer);
                    RESET_STRING_BUFFER();
                }
                END_TOKEN_ATTR();
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_TAG);
                RECONSUME_IN(PCHVML_DATA_STATE);
            }
            APPEND_TO_STRING_BUFFER(character);
            ADVANCE_TO(PCHVML_ATTRIBUTE_VALUE_DOUBLE_QUOTED_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_ATTRIBUTE_VALUE_SINGLE_QUOTED_STATE)
            if (character == '\'') {
                END_TOKEN_ATTR();
                ADVANCE_TO(PCHVML_AFTER_ATTRIBUTE_VALUE_QUOTED_STATE);
            }
            if (character == '&') {
                SET_RETURN_STATE(PCHVML_ATTRIBUTE_VALUE_SINGLE_QUOTED_STATE);
                ADVANCE_TO(PCHVML_CHARACTER_REFERENCE_STATE);
            }
            if (is_eof(character)) {
                END_TOKEN_ATTR();
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_TAG);
                RECONSUME_IN(PCHVML_DATA_STATE);
            }
            APPEND_TO_TOKEN_ATTR_VALUE(character);
            ADVANCE_TO(PCHVML_ATTRIBUTE_VALUE_SINGLE_QUOTED_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_ATTRIBUTE_VALUE_UNQUOTED_STATE)
            if (is_whitespace(character)) {
                if (!pchvml_buffer_is_empty(hvml->string_buffer)) {
                    APPEND_BUFFER_TO_TOKEN_ATTR_VALUE(hvml->string_buffer);
                    RESET_STRING_BUFFER();
                }
                END_TOKEN_ATTR();
                ADVANCE_TO(PCHVML_BEFORE_ATTRIBUTE_NAME_STATE);
            }
            if (character == '&') {
                SET_RETURN_STATE(PCHVML_ATTRIBUTE_VALUE_UNQUOTED_STATE);
                ADVANCE_TO(PCHVML_CHARACTER_REFERENCE_STATE);
            }
            if (character == '>') {
                if (!pchvml_buffer_is_empty(hvml->string_buffer)) {
                    APPEND_BUFFER_TO_TOKEN_ATTR_VALUE(hvml->string_buffer);
                    RESET_STRING_BUFFER();
                }
                END_TOKEN_ATTR();
                RETURN_AND_SWITCH_TO(PCHVML_DATA_STATE);
            }
            if (is_eof(character)) {
                if (!pchvml_buffer_is_empty(hvml->string_buffer)) {
                    APPEND_BUFFER_TO_TOKEN_ATTR_VALUE(hvml->string_buffer);
                    RESET_STRING_BUFFER();
                }
                END_TOKEN_ATTR();
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_TAG);
                RECONSUME_IN(PCHVML_DATA_STATE);
            }
            if (character == '$') {
                SET_VCM_NODE(pcvcm_node_new_concat_string(0, NULL));
                pcutils_stack_push(hvml->ejson_stack, 'U');
                if (!pchvml_buffer_is_empty(hvml->string_buffer)) {
                    struct pcvcm_node* node = pcvcm_node_new_string(
                            pchvml_buffer_get_buffer(hvml->string_buffer)
                            );
                    pctree_node_append_child(
                            (struct pctree_node*)hvml->vcm_node,
                            (struct pctree_node*)node);
                    RESET_STRING_BUFFER();
                }
                RECONSUME_IN(PCHVML_EJSON_DATA_STATE);
            }
            if (character == '"' || character == '\'' || character == '<'
                    || character == '=' || character == '`') {
                PCHVML_SET_ERROR(
                PCHVML_ERROR_UNEXPECTED_CHARACTER_IN_UNQUOTED_ATTRIBUTE_VALUE);
            }
            APPEND_TO_STRING_BUFFER(character);
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
                pchvml_token_set_self_closing(hvml->token, true);
                RETURN_AND_SWITCH_TO(PCHVML_DATA_STATE);
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
            APPEND_TO_TOKEN_TEXT(character);
            ADVANCE_TO(PCHVML_BOGUS_COMMENT_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_MARKUP_DECLARATION_OPEN_STATE)
            if (hvml->sbst == NULL) {
                hvml->sbst = pchvml_sbst_new_markup_declaration_open_state();
            }
            bool ret = pchvml_sbst_advance_ex(hvml->sbst, character, true);
            if (!ret) {
                PCHVML_SET_ERROR(PCHVML_ERROR_INCORRECTLY_OPENED_COMMENT);
                pchvml_rwswrap_buffer_arrlist(hvml->rwswrap,
                        pchvml_sbst_get_buffered_ucs(hvml->sbst));
                hvml->token = pchvml_token_new_comment();
                pchvml_sbst_destroy(hvml->sbst);
                hvml->sbst = NULL;
                ADVANCE_TO(PCHVML_BOGUS_COMMENT_STATE);
            }

            const char* value = pchvml_sbst_get_match(hvml->sbst);
            if (value == NULL) {
                ADVANCE_TO(PCHVML_MARKUP_DECLARATION_OPEN_STATE);
            }

            if (strcmp(value, "--") == 0) {
                hvml->token = pchvml_token_new_comment();
                pchvml_sbst_destroy(hvml->sbst);
                hvml->sbst = NULL;
                ADVANCE_TO(PCHVML_COMMENT_START_STATE);
            }
            if (strcmp(value, "DOCTYPE") == 0) {
                pchvml_sbst_destroy(hvml->sbst);
                hvml->sbst = NULL;
                ADVANCE_TO(PCHVML_DOCTYPE_STATE);
            }
            if (strcmp(value, "[CDATA[") == 0) {
                pchvml_sbst_destroy(hvml->sbst);
                hvml->sbst = NULL;
                if (pchvml_parse_is_adjusted_current_node(hvml)
                       && pchvml_parse_is_not_in_hvml_namespace(hvml) ) {
                    ADVANCE_TO(PCHVML_CDATA_SECTION_STATE);
                }
                else {
                    PCHVML_SET_ERROR(PCHVML_ERROR_CDATA_IN_HTML_CONTENT);
                    hvml->token = pchvml_token_new_comment();
                    APPEND_BYTES_TO_TOKEN_TEXT("[CDATA[", 7);
                    ADVANCE_TO(PCHVML_BOGUS_COMMENT_STATE);
                }
            }
            PCHVML_SET_ERROR(PCHVML_ERROR_INCORRECTLY_OPENED_COMMENT);
            pchvml_rwswrap_buffer_arrlist(hvml->rwswrap,
                    pchvml_sbst_get_buffered_ucs(hvml->sbst));
            hvml->token = pchvml_token_new_comment();
            pchvml_sbst_destroy(hvml->sbst);
            hvml->sbst = NULL;
            ADVANCE_TO(PCHVML_BOGUS_COMMENT_STATE);
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
            APPEND_TO_TOKEN_TEXT('-');
            RECONSUME_IN(PCHVML_COMMENT_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_COMMENT_STATE)
            if (character == '<') {
                APPEND_TO_TOKEN_TEXT(character);
                SWITCH_TO(PCHVML_COMMENT_LESS_THAN_SIGN_STATE);
            }
            if (character == '-') {
                ADVANCE_TO(PCHVML_COMMENT_END_DASH_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_COMMENT);
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            APPEND_TO_TOKEN_TEXT(character);
            ADVANCE_TO(PCHVML_COMMENT_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_COMMENT_LESS_THAN_SIGN_STATE)
            if (character == '!') {
                APPEND_TO_TOKEN_TEXT(character);
                ADVANCE_TO(PCHVML_COMMENT_LESS_THAN_SIGN_BANG_STATE);
            }
            if (character == '<') {
                APPEND_TO_TOKEN_TEXT(character);
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
            APPEND_TO_TOKEN_TEXT('-');
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
                APPEND_TO_TOKEN_TEXT('-');
                ADVANCE_TO(PCHVML_COMMENT_END_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_COMMENT);
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            APPEND_TO_TOKEN_TEXT('-');
            APPEND_TO_TOKEN_TEXT('-');
            RECONSUME_IN(PCHVML_COMMENT_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_COMMENT_END_BANG_STATE)
            if (character == '-') {
                APPEND_TO_TOKEN_TEXT('-');
                APPEND_TO_TOKEN_TEXT('-');
                APPEND_TO_TOKEN_TEXT('!');
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
            APPEND_TO_TOKEN_TEXT('-');
            APPEND_TO_TOKEN_TEXT('-');
            APPEND_TO_TOKEN_TEXT('!');
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
                hvml->token = pchvml_token_new_doctype();
                pchvml_token_set_force_quirks(hvml->token, true);
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
                hvml->token = pchvml_token_new_doctype();
                pchvml_token_set_force_quirks(hvml->token, true);
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_DOCTYPE);
                hvml->token = pchvml_token_new_doctype();
                pchvml_token_set_force_quirks(hvml->token, true);
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            hvml->token = pchvml_token_new_doctype();
            APPEND_TO_TOKEN_NAME(character);
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
            APPEND_TO_TOKEN_NAME(character);
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
                pchvml_token_set_force_quirks(hvml->token, true);
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            if (hvml->sbst == NULL) {
                hvml->sbst = pchvml_sbst_new_after_doctype_name_state();
            }
            bool ret = pchvml_sbst_advance_ex(hvml->sbst, character, true);
            if (!ret) {
                PCHVML_SET_ERROR(
                    PCHVML_ERROR_INVALID_CHARACTER_SEQUENCE_AFTER_DOCTYPE_NAME);
                pchvml_rwswrap_buffer_arrlist(hvml->rwswrap,
                        pchvml_sbst_get_buffered_ucs(hvml->sbst));
                pchvml_sbst_destroy(hvml->sbst);
                hvml->sbst = NULL;
                pchvml_token_set_force_quirks(hvml->token, true);
                ADVANCE_TO(PCHVML_BOGUS_COMMENT_STATE);
            }

            const char* value = pchvml_sbst_get_match(hvml->sbst);
            if (value == NULL) {
                ADVANCE_TO(PCHVML_MARKUP_DECLARATION_OPEN_STATE);
            }

            if (strcmp(value, "PUBLIC") == 0) {
                pchvml_sbst_destroy(hvml->sbst);
                hvml->sbst = NULL;
                ADVANCE_TO(PCHVML_AFTER_DOCTYPE_PUBLIC_KEYWORD_STATE);
            }
            if (strcmp(value, "SYSTEM") == 0) {
                pchvml_sbst_destroy(hvml->sbst);
                hvml->sbst = NULL;
                ADVANCE_TO(PCHVML_AFTER_DOCTYPE_SYSTEM_KEYWORD_STATE);
            }
            PCHVML_SET_ERROR(
                PCHVML_ERROR_INVALID_CHARACTER_SEQUENCE_AFTER_DOCTYPE_NAME);
            pchvml_rwswrap_buffer_arrlist(hvml->rwswrap,
                    pchvml_sbst_get_buffered_ucs(hvml->sbst));
            pchvml_sbst_destroy(hvml->sbst);
            hvml->sbst = NULL;
            pchvml_token_set_force_quirks(hvml->token, true);
            ADVANCE_TO(PCHVML_BOGUS_COMMENT_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_AFTER_DOCTYPE_PUBLIC_KEYWORD_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(PCHVML_BEFORE_DOCTYPE_PUBLIC_IDENTIFIER_STATE);
            }
            if (character == '"') {
                PCHVML_SET_ERROR(
                  PCHVML_ERROR_MISSING_WHITESPACE_AFTER_DOCTYPE_PUBLIC_KEYWORD);
                RESET_TOKEN_PUBLIC_IDENTIFIER();
                ADVANCE_TO(PCHVML_DOCTYPE_PUBLIC_IDENTIFIER_DOUBLE_QUOTED_STATE);
            }
            if (character == '\'') {
                PCHVML_SET_ERROR(
                  PCHVML_ERROR_MISSING_WHITESPACE_AFTER_DOCTYPE_PUBLIC_KEYWORD);
                RESET_TOKEN_PUBLIC_IDENTIFIER();
                ADVANCE_TO(PCHVML_DOCTYPE_PUBLIC_IDENTIFIER_SINGLE_QUOTED_STATE);
            }
            if (character == '>') {
                PCHVML_SET_ERROR(
                  PCHVML_ERROR_MISSING_DOCTYPE_PUBLIC_IDENTIFIER);
                pchvml_token_set_force_quirks(hvml->token, true);
                RETURN_AND_SWITCH_TO(PCHVML_DATA_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_DOCTYPE);
                pchvml_token_set_force_quirks(hvml->token, true);
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            PCHVML_SET_ERROR(
                PCHVML_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_PUBLIC_IDENTIFIER);
            pchvml_token_set_force_quirks(hvml->token, true);
            ADVANCE_TO(PCHVML_BOGUS_DOCTYPE_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_BEFORE_DOCTYPE_PUBLIC_IDENTIFIER_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(PCHVML_BEFORE_DOCTYPE_PUBLIC_IDENTIFIER_STATE);
            }
            if (character == '"') {
                RESET_TOKEN_PUBLIC_IDENTIFIER();
                ADVANCE_TO(PCHVML_DOCTYPE_PUBLIC_IDENTIFIER_DOUBLE_QUOTED_STATE);
            }
            if (character == '\'') {
                RESET_TOKEN_PUBLIC_IDENTIFIER();
                ADVANCE_TO(PCHVML_DOCTYPE_PUBLIC_IDENTIFIER_SINGLE_QUOTED_STATE);
            }
            if (character == '>') {
                PCHVML_SET_ERROR(
                  PCHVML_ERROR_MISSING_DOCTYPE_PUBLIC_IDENTIFIER);
                pchvml_token_set_force_quirks(hvml->token, true);
                RETURN_AND_SWITCH_TO(PCHVML_DATA_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_DOCTYPE);
                pchvml_token_set_force_quirks(hvml->token, true);
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            PCHVML_SET_ERROR(
                PCHVML_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_PUBLIC_IDENTIFIER);
            pchvml_token_set_force_quirks(hvml->token, true);
            ADVANCE_TO(PCHVML_BOGUS_DOCTYPE_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_DOCTYPE_PUBLIC_IDENTIFIER_DOUBLE_QUOTED_STATE)
            if (character == '"') {
                ADVANCE_TO(PCHVML_AFTER_DOCTYPE_PUBLIC_IDENTIFIER_STATE);
            }
            if (character == '>') {
                PCHVML_SET_ERROR(PCHVML_ERROR_ABRUPT_DOCTYPE_PUBLIC_IDENTIFIER);
                pchvml_token_set_force_quirks(hvml->token, true);
                RETURN_AND_SWITCH_TO(PCHVML_DATA_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_DOCTYPE);
                pchvml_token_set_force_quirks(hvml->token, true);
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            APPEND_TO_TOKEN_PUBLIC_IDENTIFIER(character);
            ADVANCE_TO(PCHVML_DOCTYPE_PUBLIC_IDENTIFIER_DOUBLE_QUOTED_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_DOCTYPE_PUBLIC_IDENTIFIER_SINGLE_QUOTED_STATE)
            if (character == '\'') {
                ADVANCE_TO(PCHVML_AFTER_DOCTYPE_PUBLIC_IDENTIFIER_STATE);
            }
            if (character == '>') {
                PCHVML_SET_ERROR(PCHVML_ERROR_ABRUPT_DOCTYPE_PUBLIC_IDENTIFIER);
                pchvml_token_set_force_quirks(hvml->token, true);
                RETURN_AND_SWITCH_TO(PCHVML_DATA_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_DOCTYPE);
                pchvml_token_set_force_quirks(hvml->token, true);
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            APPEND_TO_TOKEN_PUBLIC_IDENTIFIER(character);
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
                  PCHVML_ERROR_MISSING_WHITESPACE_BETWEEN_DOCTYPE_PUBLIC_AND_SYSTEM_INFORMATIONS
                  );
                RESET_TOKEN_SYSTEM_INFORMATION();
                ADVANCE_TO(PCHVML_DOCTYPE_SYSTEM_INFORMATION_DOUBLE_QUOTED_STATE);
            }
            if (character == '\'') {
                PCHVML_SET_ERROR(
                  PCHVML_ERROR_MISSING_WHITESPACE_BETWEEN_DOCTYPE_PUBLIC_AND_SYSTEM_INFORMATIONS
                  );
                RESET_TOKEN_SYSTEM_INFORMATION();
                ADVANCE_TO(PCHVML_DOCTYPE_SYSTEM_INFORMATION_SINGLE_QUOTED_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_DOCTYPE);
                pchvml_token_set_force_quirks(hvml->token, true);
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            PCHVML_SET_ERROR(
                PCHVML_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_SYSTEM_INFORMATION);
            pchvml_token_set_force_quirks(hvml->token, true);
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
                RESET_TOKEN_SYSTEM_INFORMATION();
                ADVANCE_TO(PCHVML_DOCTYPE_SYSTEM_INFORMATION_DOUBLE_QUOTED_STATE);
            }
            if (character == '\'') {
                RESET_TOKEN_SYSTEM_INFORMATION();
                ADVANCE_TO(PCHVML_DOCTYPE_SYSTEM_INFORMATION_SINGLE_QUOTED_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_DOCTYPE);
                pchvml_token_set_force_quirks(hvml->token, true);
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            PCHVML_SET_ERROR(
                PCHVML_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_SYSTEM_INFORMATION);
            pchvml_token_set_force_quirks(hvml->token, true);
            ADVANCE_TO(PCHVML_BOGUS_DOCTYPE_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_AFTER_DOCTYPE_SYSTEM_KEYWORD_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(PCHVML_BEFORE_DOCTYPE_SYSTEM_INFORMATION_STATE);
            }
            if (character == '"') {
                PCHVML_SET_ERROR(
                  PCHVML_ERROR_MISSING_WHITESPACE_AFTER_DOCTYPE_SYSTEM_KEYWORD);
                RESET_TOKEN_SYSTEM_INFORMATION();
                ADVANCE_TO(PCHVML_DOCTYPE_SYSTEM_INFORMATION_DOUBLE_QUOTED_STATE);
            }
            if (character == '\'') {
                PCHVML_SET_ERROR(
                  PCHVML_ERROR_MISSING_WHITESPACE_AFTER_DOCTYPE_SYSTEM_KEYWORD);
                RESET_TOKEN_SYSTEM_INFORMATION();
                ADVANCE_TO(PCHVML_DOCTYPE_SYSTEM_INFORMATION_SINGLE_QUOTED_STATE);
            }
            if (character == '>') {
                PCHVML_SET_ERROR(PCHVML_ERROR_MISSING_DOCTYPE_SYSTEM_INFORMATION);
                pchvml_token_set_force_quirks(hvml->token, true);
                RETURN_AND_SWITCH_TO(PCHVML_DATA_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_DOCTYPE);
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            PCHVML_SET_ERROR(
                PCHVML_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_SYSTEM_INFORMATION);
            pchvml_token_set_force_quirks(hvml->token, true);
            ADVANCE_TO(PCHVML_BOGUS_DOCTYPE_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_BEFORE_DOCTYPE_SYSTEM_INFORMATION_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(PCHVML_BEFORE_DOCTYPE_SYSTEM_INFORMATION_STATE);
            }
            if (character == '"') {
                RESET_TOKEN_SYSTEM_INFORMATION();
                ADVANCE_TO(PCHVML_DOCTYPE_SYSTEM_INFORMATION_DOUBLE_QUOTED_STATE);
            }
            if (character == '\'') {
                RESET_TOKEN_SYSTEM_INFORMATION();
                ADVANCE_TO(PCHVML_DOCTYPE_SYSTEM_INFORMATION_SINGLE_QUOTED_STATE);
            }
            if (character == '>') {
                PCHVML_SET_ERROR(
                        PCHVML_ERROR_MISSING_DOCTYPE_SYSTEM_INFORMATION);
                pchvml_token_set_force_quirks(hvml->token, true);
                RETURN_AND_SWITCH_TO(PCHVML_DATA_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_DOCTYPE);
                pchvml_token_set_force_quirks(hvml->token, true);
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            PCHVML_SET_ERROR(
                PCHVML_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_SYSTEM_INFORMATION);
            pchvml_token_set_force_quirks(hvml->token, true);
            ADVANCE_TO(PCHVML_BOGUS_DOCTYPE_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_DOCTYPE_SYSTEM_INFORMATION_DOUBLE_QUOTED_STATE)
            if (character == '"') {
                ADVANCE_TO(PCHVML_AFTER_DOCTYPE_SYSTEM_INFORMATION_STATE);
            }
            if (character == '>') {
                PCHVML_SET_ERROR(PCHVML_ERROR_ABRUPT_DOCTYPE_SYSTEM_INFORMATION);
                pchvml_token_set_force_quirks(hvml->token, true);
                RETURN_AND_SWITCH_TO(PCHVML_DATA_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_DOCTYPE);
                pchvml_token_set_force_quirks(hvml->token, true);
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            APPEND_TO_TOKEN_SYSTEM_INFORMATION(character);
            ADVANCE_TO(PCHVML_DOCTYPE_SYSTEM_INFORMATION_DOUBLE_QUOTED_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_DOCTYPE_SYSTEM_INFORMATION_SINGLE_QUOTED_STATE)
            if (character == '\'') {
                ADVANCE_TO(PCHVML_AFTER_DOCTYPE_SYSTEM_INFORMATION_STATE);
            }
            if (character == '>') {
                PCHVML_SET_ERROR(PCHVML_ERROR_ABRUPT_DOCTYPE_SYSTEM_INFORMATION);
                pchvml_token_set_force_quirks(hvml->token, true);
                RETURN_AND_SWITCH_TO(PCHVML_DATA_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_DOCTYPE);
                pchvml_token_set_force_quirks(hvml->token, true);
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            APPEND_TO_TOKEN_SYSTEM_INFORMATION(character);
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
                pchvml_token_set_force_quirks(hvml->token, true);
                RETURN_AND_RECONSUME_IN(PCHVML_DATA_STATE);
            }
            PCHVML_SET_ERROR(
             PCHVML_ERROR_UNEXPECTED_CHARACTER_AFTER_DOCTYPE_SYSTEM_INFORMATION);
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
            APPEND_TO_TOKEN_TEXT(character);
            ADVANCE_TO(PCHVML_CDATA_SECTION_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_CDATA_SECTION_BRACKET_STATE)
            if (character == ']') {
                ADVANCE_TO(PCHVML_CDATA_SECTION_END_STATE);
            }
            APPEND_TO_TOKEN_TEXT(']');
            RECONSUME_IN(PCHVML_CDATA_SECTION_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_CDATA_SECTION_END_STATE)
            if (character == ']') {
                APPEND_TO_TOKEN_TEXT(']');
                ADVANCE_TO(PCHVML_CDATA_SECTION_END_STATE);
            }
            if (character == '>') {
                ADVANCE_TO(PCHVML_DATA_STATE);
            }
            APPEND_TO_TOKEN_TEXT(']');
            APPEND_TO_TOKEN_TEXT(']');
            RECONSUME_IN(PCHVML_CDATA_SECTION_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_CHARACTER_REFERENCE_STATE)
            RESET_TEMP_BUFFER();
            APPEND_TO_TEMP_BUFFER('&');
            if (is_ascii_alpha_numeric(character)) {
                RECONSUME_IN(PCHVML_NAMED_CHARACTER_REFERENCE_STATE);
            }
            if (character == '#') {
                APPEND_TO_TEMP_BUFFER(character);
                ADVANCE_TO(PCHVML_NUMERIC_CHARACTER_REFERENCE_STATE);
            }
            // FIXME: character reference in attribute value
            APPEND_TEMP_BUFFER_TO_TOKEN_TEXT();
            RESET_TEMP_BUFFER();
            RECONSUME_IN(hvml->return_state);
        END_STATE()

        BEGIN_STATE(PCHVML_NAMED_CHARACTER_REFERENCE_STATE)
            if (hvml->sbst == NULL) {
                hvml->sbst = pchvml_sbst_new_char_ref();
            }
            bool ret = pchvml_sbst_advance(hvml->sbst, character);
            if (!ret) {
                struct pcutils_arrlist* ucs = pchvml_sbst_get_buffered_ucs(
                        hvml->sbst);
                size_t length = pcutils_arrlist_length(ucs);
                for (size_t i = 0; i < length; i++) {
                    uint32_t uc = (uint32_t)(uintptr_t) pcutils_arrlist_get_idx(
                            ucs, i);
                    APPEND_TO_TEMP_BUFFER(uc);
                }
                pchvml_sbst_destroy(hvml->sbst);
                hvml->sbst = NULL;
                APPEND_TEMP_BUFFER_TO_TOKEN_TEXT();
                RESET_TEMP_BUFFER();
                ADVANCE_TO(PCHVML_AMBIGUOUS_AMPERSAND_STATE);
            }

            const char* value = pchvml_sbst_get_match(hvml->sbst);
            if (value == NULL) {
                ADVANCE_TO(PCHVML_NAMED_CHARACTER_REFERENCE_STATE);
            }
            if (character != ';') {
                ADVANCE_TO(PCHVML_NAMED_CHARACTER_REFERENCE_STATE);
            }
            RESET_TEMP_BUFFER();
            APPEND_BYTES_TO_TOKEN_TEXT(value, strlen(value));

            pchvml_sbst_destroy(hvml->sbst);
            hvml->sbst = NULL;
            ADVANCE_TO(hvml->return_state);
        END_STATE()

        BEGIN_STATE(PCHVML_AMBIGUOUS_AMPERSAND_STATE)
            if (is_ascii_alpha_numeric(character)) {
                if (pchvml_parser_is_in_attribute(hvml)) {
                    APPEND_TO_TOKEN_ATTR_VALUE(character);
                    ADVANCE_TO(PCHVML_AMBIGUOUS_AMPERSAND_STATE);
                }
                else {
                    RECONSUME_IN(hvml->return_state);
                }
            }
            if (character == ';') {
                PCHVML_SET_ERROR(PCHVML_ERROR_UNKNOWN_NAMED_CHARACTER_REFERENCE);
                RECONSUME_IN(hvml->return_state);
            }
            RECONSUME_IN(hvml->return_state);
        END_STATE()

        BEGIN_STATE(PCHVML_NUMERIC_CHARACTER_REFERENCE_STATE)
            hvml->char_ref_code = 0;
            if (character == 'x' || character == 'X') {
                APPEND_TO_TEMP_BUFFER(character);
                ADVANCE_TO(PCHVML_HEXADECIMAL_CHARACTER_REFERENCE_START_STATE);
            }
            RECONSUME_IN(PCHVML_DECIMAL_CHARACTER_REFERENCE_START_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_HEXADECIMAL_CHARACTER_REFERENCE_START_STATE)
            if (is_ascii_hex_digit(character)) {
                RECONSUME_IN(PCHVML_HEXADECIMAL_CHARACTER_REFERENCE_STATE);
            }
            PCHVML_SET_ERROR(
                PCHVML_ERROR_ABSENCE_OF_DIGITS_IN_NUMERIC_CHARACTER_REFERENCE);
            APPEND_TEMP_BUFFER_TO_TOKEN_TEXT();
            RECONSUME_IN(hvml->return_state);
        END_STATE()

        BEGIN_STATE(PCHVML_DECIMAL_CHARACTER_REFERENCE_START_STATE)
            if (is_ascii_digit(character)) {
                RECONSUME_IN(PCHVML_DECIMAL_CHARACTER_REFERENCE_STATE);
            }
            PCHVML_SET_ERROR(
                PCHVML_ERROR_ABSENCE_OF_DIGITS_IN_NUMERIC_CHARACTER_REFERENCE);
            APPEND_TEMP_BUFFER_TO_TOKEN_TEXT();
            RECONSUME_IN(hvml->return_state);
        END_STATE()

        BEGIN_STATE(PCHVML_HEXADECIMAL_CHARACTER_REFERENCE_STATE)
            if (is_ascii_digit(character)) {
                hvml->char_ref_code *= 16;
                hvml->char_ref_code += character - 0x30;
            }
            if (is_ascii_upper_hex_digit(character)) {
                hvml->char_ref_code *= 16;
                hvml->char_ref_code += character - 0x37;
            }
            if (is_ascii_lower_hex_digit(character)) {
                hvml->char_ref_code *= 16;
                hvml->char_ref_code += character - 0x57;
            }
            if (character == ';') {
                ADVANCE_TO(PCHVML_NUMERIC_CHARACTER_REFERENCE_END_STATE);
            }
            PCHVML_SET_ERROR(
                    PCHVML_ERROR_MISSING_SEMICOLON_AFTER_CHARACTER_REFERENCE);
            RECONSUME_IN(PCHVML_NUMERIC_CHARACTER_REFERENCE_END_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_DECIMAL_CHARACTER_REFERENCE_STATE)
            if (is_ascii_digit(character)) {
                hvml->char_ref_code *= 10;
                hvml->char_ref_code += character - 0x30;
                ADVANCE_TO(PCHVML_DECIMAL_CHARACTER_REFERENCE_STATE);
            }
            if (character == ';') {
                ADVANCE_TO(PCHVML_NUMERIC_CHARACTER_REFERENCE_END_STATE);
            }
            PCHVML_SET_ERROR(
                    PCHVML_ERROR_MISSING_SEMICOLON_AFTER_CHARACTER_REFERENCE);
            RECONSUME_IN(PCHVML_NUMERIC_CHARACTER_REFERENCE_END_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_NUMERIC_CHARACTER_REFERENCE_END_STATE)
            uint32_t uc = hvml->char_ref_code;
            if (uc == 0x00) {
                PCHVML_SET_ERROR(PCHVML_ERROR_NULL_CHARACTER_REFERENCE);
                hvml->char_ref_code = 0xFFFD;
            }
            if (uc > 0x10FFFF) {
                PCHVML_SET_ERROR(
                    PCHVML_ERROR_CHARACTER_REFERENCE_OUTSIDE_UNICODE_RANGE);
                hvml->char_ref_code = 0xFFFD;
            }
            if ((uc & 0xFFFFF800) == 0xD800) {
                PCHVML_SET_ERROR(
                        PCHVML_ERROR_SURROGATE_CHARACTER_REFERENCE);
            }
            if (uc >= 0xFDD0 && (uc <= 0xFDEF || (uc&0xFFFE) == 0xFFFE) &&
                    uc <= 0x10FFFF) {
                PCHVML_SET_ERROR(
                        PCHVML_ERROR_NONCHARACTER_CHARACTER_REFERENCE);
            }
            if (uc <= 0x1F &&
                    !(uc == 0x09 || uc == 0x0A || uc == 0x0C)){
                PCHVML_SET_ERROR(
                    PCHVML_ERROR_CONTROL_CHARACTER_REFERENCE);
            }
            if (uc >= 0x7F && uc <= 0x9F) {
                PCHVML_SET_ERROR(
                    PCHVML_ERROR_CONTROL_CHARACTER_REFERENCE);
                if (uc >= 0x80) {
                    hvml->char_ref_code =
                        numeric_char_ref_extension_array[uc - 0x80];
                }
            }
            RESET_TEMP_BUFFER();
            uc = hvml->char_ref_code;
            APPEND_TO_TOKEN_TEXT(uc);
            RECONSUME_IN(hvml->return_state);
        END_STATE()

        BEGIN_STATE(PCHVML_SPECIAL_ATTRIBUTE_OPERATOR_IN_ATTRIBUTE_NAME_STATE)
            if (character == '=') {
                if (pchvml_buffer_is_empty(hvml->temp_buffer)) {
                    pchvml_token_set_assignment_to_attr(
                            hvml->token,
                            PCHVML_ATTRIBUTE_ASSIGNMENT);
                }
                else {
                    uint32_t op = pchvml_buffer_get_last_char(
                            hvml->temp_buffer);
                    switch (op) {
                        case '+':
                            pchvml_token_set_assignment_to_attr(
                                    hvml->token,
                                    PCHVML_ATTRIBUTE_ADDITION_ASSIGNMENT);
                            break;
                        case '-':
                            pchvml_token_set_assignment_to_attr(
                                    hvml->token,
                                    PCHVML_ATTRIBUTE_SUBTRACTION_ASSIGNMENT);
                            break;
                        case '%':
                            pchvml_token_set_assignment_to_attr(
                                    hvml->token,
                                    PCHVML_ATTRIBUTE_REMAINDER_ASSIGNMENT);
                            break;
                        case '~':
                            pchvml_token_set_assignment_to_attr(
                                    hvml->token,
                                    PCHVML_ATTRIBUTE_REPLACE_ASSIGNMENT);
                            break;
                        case '^':
                            pchvml_token_set_assignment_to_attr(
                                    hvml->token,
                                    PCHVML_ATTRIBUTE_HEAD_ASSIGNMENT);
                            break;
                        case '$':
                            pchvml_token_set_assignment_to_attr(
                                    hvml->token,
                                    PCHVML_ATTRIBUTE_TAIL_ASSIGNMENT);
                            break;
                        default:
                            pchvml_token_set_assignment_to_attr(
                                    hvml->token,
                                    PCHVML_ATTRIBUTE_ASSIGNMENT);
                            break;
                    }
                }
                SWITCH_TO(PCHVML_BEFORE_ATTRIBUTE_VALUE_STATE);
            }
            APPEND_TEMP_BUFFER_TO_TOKEN_ATTR_NAME();
            RECONSUME_IN(PCHVML_ATTRIBUTE_NAME_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_SPECIAL_ATTRIBUTE_OPERATOR_AFTER_ATTRIBUTE_NAME_STATE)
            if (character == '=') {
                if (pchvml_buffer_is_empty(hvml->temp_buffer)) {
                    pchvml_token_set_assignment_to_attr(
                            hvml->token,
                            PCHVML_ATTRIBUTE_ASSIGNMENT);
                }
                else {
                    uint32_t op = pchvml_buffer_get_last_char(
                            hvml->temp_buffer);
                    switch (op) {
                        case '+':
                            pchvml_token_set_assignment_to_attr(
                                    hvml->token,
                                    PCHVML_ATTRIBUTE_ADDITION_ASSIGNMENT);
                            break;
                        case '-':
                            pchvml_token_set_assignment_to_attr(
                                    hvml->token,
                                    PCHVML_ATTRIBUTE_SUBTRACTION_ASSIGNMENT);
                            break;
                        case '%':
                            pchvml_token_set_assignment_to_attr(
                                    hvml->token,
                                    PCHVML_ATTRIBUTE_REMAINDER_ASSIGNMENT);
                            break;
                        case '~':
                            pchvml_token_set_assignment_to_attr(
                                    hvml->token,
                                    PCHVML_ATTRIBUTE_REPLACE_ASSIGNMENT);
                            break;
                        case '^':
                            pchvml_token_set_assignment_to_attr(
                                    hvml->token,
                                    PCHVML_ATTRIBUTE_HEAD_ASSIGNMENT);
                            break;
                        case '$':
                            pchvml_token_set_assignment_to_attr(
                                    hvml->token,
                                    PCHVML_ATTRIBUTE_TAIL_ASSIGNMENT);
                            break;
                        default:
                            pchvml_token_set_assignment_to_attr(
                                    hvml->token,
                                    PCHVML_ATTRIBUTE_ASSIGNMENT);
                            break;
                    }
                }
                SWITCH_TO(PCHVML_BEFORE_ATTRIBUTE_VALUE_STATE);
            }
            BEGIN_TOKEN_ATTR();
            APPEND_TEMP_BUFFER_TO_TOKEN_ATTR_NAME();
            RECONSUME_IN(PCHVML_ATTRIBUTE_NAME_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_DATA_STATE)
            if (is_whitespace (character) || character == 0xFEFF) {
                ADVANCE_TO(PCHVML_EJSON_DATA_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_TAG);
                RETURN_NEW_EOF_TOKEN();
            }
            hvml->vcm_tree = NULL;
            RECONSUME_IN(PCHVML_EJSON_CONTROL_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_FINISHED_STATE)
            if (is_whitespace(character) || character == '}' ||
                    character == '"' || character == '>') {
                while (!pcvcm_stack_is_empty(hvml->vcm_stack)) {
                    pcutils_stack_pop (hvml->ejson_stack);
                    struct pcvcm_node* node = pcvcm_stack_pop(
                            hvml->vcm_stack);

                    if (hvml->vcm_node) {
                        pctree_node_append_child((struct pctree_node*)node,
                                (struct pctree_node*)hvml->vcm_node);
                    }
                    SET_VCM_NODE(node);
                }

                if (hvml->vcm_node) {
                    if (hvml->vcm_tree) {
                        pctree_node_append_child(
                                (struct pctree_node*)hvml->vcm_tree,
                                (struct pctree_node*)hvml->vcm_node);
                    }
                    else {
                        hvml->vcm_tree = hvml->vcm_node;
                    }
                }
                if (pchvml_token_is_type(hvml->token, PCHVML_TOKEN_START_TAG)
                        ) {
                    pchvml_token_append_vcm_to_attr(hvml->token,
                            hvml->vcm_tree);
                    END_TOKEN_ATTR();
                    SET_VCM_NODE(NULL);
                    RECONSUME_IN(PCHVML_AFTER_ATTRIBUTE_VALUE_QUOTED_STATE);
                }
                hvml->token = pchvml_token_new_vcm(hvml->vcm_tree);
                hvml->vcm_tree = NULL;
                SET_VCM_NODE(NULL);
                RETURN_AND_SWITCH_TO(PCHVML_DATA_STATE);
            }
            if (character == '<') {
                while (!pcvcm_stack_is_empty(hvml->vcm_stack)) {
                    pcutils_stack_pop (hvml->ejson_stack);
                    struct pcvcm_node* node = pcvcm_stack_pop(
                            hvml->vcm_stack);

                    if (hvml->vcm_node) {
                        pctree_node_append_child((struct pctree_node*)node,
                                (struct pctree_node*)hvml->vcm_node);
                    }
                    SET_VCM_NODE(node);
                }
                if (hvml->vcm_node) {
                    if (hvml->vcm_tree) {
                        pctree_node_append_child(
                                (struct pctree_node*)hvml->vcm_tree,
                                (struct pctree_node*)hvml->vcm_node);
                    }
                    else {
                        hvml->vcm_tree = hvml->vcm_node;
                    }
                }
                hvml->token = pchvml_token_new_vcm(hvml->vcm_tree);
                hvml->vcm_tree = NULL;
                SET_VCM_NODE(NULL);
                RETURN_AND_SWITCH_TO(PCHVML_TAG_OPEN_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_TAG);
                RETURN_NEW_EOF_TOKEN();
            }
            PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
            RETURN_AND_STOP_PARSE();
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_CONTROL_STATE)
            uint32_t uc = pcutils_stack_top (hvml->ejson_stack);
            if (is_whitespace(character)) {
                if (hvml->vcm_node->type ==
                        PCVCM_NODE_TYPE_FUNC_CONCAT_STRING
                        && (uc == '"' || uc == '\'' || uc == 'U')) {
                    RECONSUME_IN(PCHVML_EJSON_AFTER_JSONEE_STRING_STATE);
                }
                ADVANCE_TO(PCHVML_EJSON_CONTROL_STATE);
            }
            if (character == '{') {
                RECONSUME_IN(PCHVML_EJSON_LEFT_BRACE_STATE);
            }
            if (character == '}') {
                if (hvml->vcm_node->type ==
                        PCVCM_NODE_TYPE_FUNC_CONCAT_STRING
                        && (uc == '"' || uc == '\'' || uc == 'U')) {
                    RECONSUME_IN(PCHVML_EJSON_AFTER_JSONEE_STRING_STATE);
                }
                RECONSUME_IN(PCHVML_EJSON_RIGHT_BRACE_STATE);
            }
            if (character == '[') {
                RECONSUME_IN(PCHVML_EJSON_LEFT_BRACKET_STATE);
            }
            if (character == ']') {
                if (hvml->vcm_node->type ==
                        PCVCM_NODE_TYPE_FUNC_CONCAT_STRING
                        && (uc == '"' || uc == '\'' || uc == 'U')) {
                    RECONSUME_IN(PCHVML_EJSON_AFTER_JSONEE_STRING_STATE);
                }
                RECONSUME_IN(PCHVML_EJSON_RIGHT_BRACKET_STATE);
            }
            if (character == '<' || character == '>') {
                RECONSUME_IN(PCHVML_EJSON_FINISHED_STATE);
            }
            if (character == '(') {
                ADVANCE_TO(PCHVML_EJSON_LEFT_PARENTHESIS_STATE);
            }
            if (character == ')') {
                if (uc == '"' || uc == '\'' || uc == 'U') {
                    RECONSUME_IN(PCHVML_EJSON_AFTER_JSONEE_STRING_STATE);
                }
                RECONSUME_IN(PCHVML_EJSON_RIGHT_PARENTHESIS_STATE);
            }
            if (character == '$') {
                RECONSUME_IN(PCHVML_EJSON_DOLLAR_STATE);
            }
            if (character == '"') {
                if (uc == '"') {
                    RECONSUME_IN(PCHVML_EJSON_AFTER_JSONEE_STRING_STATE);
                }
                else {
                    RESET_TEMP_BUFFER();
                    RECONSUME_IN(PCHVML_EJSON_VALUE_DOUBLE_QUOTED_STATE);
                }
            }
            if (character == '\'') {
                RESET_TEMP_BUFFER();
                RECONSUME_IN(PCHVML_EJSON_VALUE_SINGLE_QUOTED_STATE);
            }
            if (character == 'b') {
                RESET_TEMP_BUFFER();
                RECONSUME_IN(PCHVML_EJSON_BYTE_SEQUENCE_STATE);
            }
            if (character == 't' || character == 'f' || character == 'n') {
                RESET_TEMP_BUFFER();
                RECONSUME_IN(PCHVML_EJSON_KEYWORD_STATE);
            }
            if (character == 'I') {
                RESET_TEMP_BUFFER();
                RECONSUME_IN(PCHVML_EJSON_VALUE_NUMBER_INFINITY_STATE);
            }
            if (character == 'N') {
                RESET_TEMP_BUFFER();
                RECONSUME_IN(PCHVML_EJSON_VALUE_NAN_STATE);
            }
            if (is_ascii_digit(character) || character == '-') {
                RESET_TEMP_BUFFER();
                RECONSUME_IN(PCHVML_EJSON_VALUE_NUMBER_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_TAG);
                RETURN_NEW_EOF_TOKEN();
            }
            if (character == ',') {
                if (uc == '{') {
                    pcutils_stack_pop(hvml->ejson_stack);
                    ADVANCE_TO(PCHVML_EJSON_BEFORE_NAME_STATE);
                }
                if (uc == '[') {
                    ADVANCE_TO(PCHVML_EJSON_CONTROL_STATE);
                }
                if (uc == '(') {
                    ADVANCE_TO(PCHVML_EJSON_CONTROL_STATE);
                }
                if (uc == '<') {
                    ADVANCE_TO(PCHVML_EJSON_CONTROL_STATE);
                }
                if (uc == ':') {
                    pcutils_stack_pop(hvml->ejson_stack);
                    if (!pchvml_buffer_is_empty(hvml->temp_buffer)) {
                        struct pcvcm_node* node = pcvcm_node_new_string(
                        pchvml_buffer_get_buffer(hvml->temp_buffer));
                        pctree_node_append_child(
                                (struct pctree_node*)hvml->vcm_node,
                                (struct pctree_node*)node);
                        RESET_TEMP_BUFFER();
                    }
                    if (hvml->vcm_node &&
                            hvml->vcm_node->type != PCVCM_NODE_TYPE_OBJECT) {
                        struct pcvcm_node* node = pcvcm_stack_pop(
                                hvml->vcm_stack);
                        pctree_node_append_child(
                                (struct pctree_node*)node,
                                (struct pctree_node*)hvml->vcm_node);
                        SET_VCM_NODE(node);
                    }
                    ADVANCE_TO(PCHVML_EJSON_BEFORE_NAME_STATE);
                }
                PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
                RETURN_AND_STOP_PARSE();
            }
            if (character == '.') {
                RECONSUME_IN(PCHVML_EJSON_JSONEE_FULL_STOP_SIGN_STATE);
            }
            if (uc == '[') {
                PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
                RETURN_AND_STOP_PARSE();
            }
            RECONSUME_IN(PCHVML_EJSON_JSONEE_STRING_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_LEFT_BRACE_STATE)
            if (character == '{') {
                pcutils_stack_push(hvml->ejson_stack, 'P');
                ADVANCE_TO(PCHVML_EJSON_LEFT_BRACE_STATE);
            }
            if (character == '$') {
                RECONSUME_IN(PCHVML_EJSON_DOLLAR_STATE);
            }
            uint32_t uc = pcutils_stack_top (hvml->ejson_stack);
            if (uc == 'P') {
                pcutils_stack_pop(hvml->ejson_stack);
                pcutils_stack_push(hvml->ejson_stack, '{');
                if (hvml->vcm_node) {
                    pcvcm_stack_push(hvml->vcm_stack, hvml->vcm_node);
                }
                struct pcvcm_node* node = pcvcm_node_new_object(0, NULL);
                SET_VCM_NODE(node);
                RECONSUME_IN(PCHVML_EJSON_BEFORE_NAME_STATE);
            }
            PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
            RETURN_AND_STOP_PARSE();
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_RIGHT_BRACE_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(PCHVML_EJSON_RIGHT_BRACE_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_TAG);
                RETURN_NEW_EOF_TOKEN();
            }
            uint32_t uc = pcutils_stack_top (hvml->ejson_stack);
            if (character == '}') {
                if (uc == ':') {
                    pcutils_stack_pop(hvml->ejson_stack);
                    uc = pcutils_stack_top(hvml->ejson_stack);
                }
                if (uc == '{') {
                    pcutils_stack_pop(hvml->ejson_stack);
                    struct pcvcm_node* node = pcvcm_stack_pop(
                            hvml->vcm_stack);
                    if (node && hvml->vcm_node) {
                        pctree_node_append_child(
                                (struct pctree_node*)node,
                                (struct pctree_node*)hvml->vcm_node);
                        SET_VCM_NODE(node);
                    }
                    if (pcutils_stack_is_empty(hvml->ejson_stack)) {
                        ADVANCE_TO(PCHVML_EJSON_FINISHED_STATE);
                    }
                    ADVANCE_TO(PCHVML_EJSON_AFTER_VALUE_STATE);
                }
                else if (uc == 'P') {
                    pcutils_stack_pop(hvml->ejson_stack);
                    if (hvml->vcm_node->extra & EXTRA_PROTECT_FLAG) {
                        hvml->vcm_node->extra &= EXTRA_SUGAR_FLAG;
                    }
                    else {
                        hvml->vcm_node->extra &= EXTRA_PROTECT_FLAG;
                    }
                    ADVANCE_TO(PCHVML_EJSON_RIGHT_BRACE_STATE);
                }
                else if (uc == '(' || uc == '<') {
                    ADVANCE_TO(PCHVML_EJSON_CONTROL_STATE);
                }
                PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_RIGHT_BRACE);
                RETURN_AND_STOP_PARSE();
            }
            if (character == ':') {
                if (uc == '{') {
                    struct pcvcm_node* node = pcvcm_stack_pop(
                            hvml->vcm_stack);
                    if (hvml->vcm_node) {
                        pctree_node_append_child(
                                (struct pctree_node*)node,
                                (struct pctree_node*)hvml->vcm_node);
                    }
                    pcvcm_stack_push(hvml->vcm_stack, node);
                    SET_VCM_NODE(NULL);
                    ADVANCE_TO(PCHVML_EJSON_CONTROL_STATE);
                }
                if (uc == 'P') {
                    pcutils_stack_pop(hvml->ejson_stack);
                    pcutils_stack_push(hvml->ejson_stack, '{');
                    struct pcvcm_node* node = pcvcm_node_new_object(0, NULL);
                    if (hvml->vcm_node) {
                        pctree_node_append_child(
                                (struct pctree_node*)node,
                                (struct pctree_node*)hvml->vcm_node);
                    }
                    pcvcm_stack_push(hvml->vcm_stack, node);
                    SET_VCM_NODE(NULL);
                    ADVANCE_TO(PCHVML_EJSON_CONTROL_STATE);
                }
                PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
                RETURN_AND_STOP_PARSE();
            }
            if (character == '.' && uc == '$') {
                pcutils_stack_pop(hvml->ejson_stack);
                struct pcvcm_node* node = pcvcm_stack_pop(hvml->vcm_stack);
                if (hvml->vcm_node) {
                    pctree_node_append_child(
                            (struct pctree_node*)node,
                            (struct pctree_node*)hvml->vcm_node);
                }
                SET_VCM_NODE(node);
            }
            RECONSUME_IN(PCHVML_EJSON_CONTROL_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_LEFT_BRACKET_STATE)
            if (character == '[') {
                if (pcutils_stack_is_empty(hvml->ejson_stack)) {
                    pcutils_stack_push(hvml->ejson_stack, '[');
                    struct pcvcm_node* node = pcvcm_node_new_get_element(NULL,
                            NULL);
                    if (hvml->vcm_node) {
                        pctree_node_append_child(
                                (struct pctree_node*)node,
                                (struct pctree_node*)hvml->vcm_node);
                    }
                    SET_VCM_NODE(node);
                    ADVANCE_TO(PCHVML_EJSON_CONTROL_STATE);
                }
                if (hvml->vcm_node->type ==
                        PCVCM_NODE_TYPE_FUNC_GET_VARIABLE ||
                        hvml->vcm_node->type ==
                        PCVCM_NODE_TYPE_FUNC_GET_ELEMENT) {
                    pcutils_stack_push(hvml->ejson_stack, '.');
                    struct pcvcm_node* node = pcvcm_node_new_get_element(NULL,
                            NULL);
                    if (hvml->vcm_node) {
                        pctree_node_append_child(
                                (struct pctree_node*)node,
                                (struct pctree_node*)hvml->vcm_node);
                    }
                    SET_VCM_NODE(node);
                    ADVANCE_TO(PCHVML_EJSON_JSONEE_VARIABLE_STATE);
                }
                uint32_t uc = pcutils_stack_top (hvml->ejson_stack);
                if (uc == '(' || uc == '<' || uc == '[' || uc == ':') {
                    pcutils_stack_push(hvml->ejson_stack, '[');
                    if (hvml->vcm_node) {
                        pcvcm_stack_push(hvml->vcm_stack, hvml->vcm_node);
                    }
                    struct pcvcm_node* node = pcvcm_node_new_array(0, NULL);
                    SET_VCM_NODE(node);
                    ADVANCE_TO(PCHVML_EJSON_CONTROL_STATE);
                }
                PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
                RETURN_AND_STOP_PARSE();
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_TAG);
                RETURN_NEW_EOF_TOKEN();
            }
            PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
            RETURN_AND_STOP_PARSE();
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_RIGHT_BRACKET_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(PCHVML_EJSON_RIGHT_BRACE_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_TAG);
                RETURN_NEW_EOF_TOKEN();
            }
            uint32_t uc = pcutils_stack_top(hvml->ejson_stack);
            if (character == ']') {
                if (uc == '.') {
                    pcutils_stack_pop(hvml->ejson_stack);
                    struct pcvcm_node* node = pcvcm_stack_pop(
                            hvml->vcm_stack);

                    if (node && hvml->vcm_node) {
                        pctree_node_append_child((struct pctree_node*)node,
                                (struct pctree_node*)hvml->vcm_node);
                        SET_VCM_NODE(node);
                    }
                    ADVANCE_TO(PCHVML_EJSON_AFTER_VALUE_STATE);
                }
                if (uc == '[') {
                    pcutils_stack_pop(hvml->ejson_stack);
                    struct pcvcm_node* node = pcvcm_stack_pop(
                            hvml->vcm_stack);
                    if (node && hvml->vcm_node) {
                        pctree_node_append_child((struct pctree_node*)node,
                                (struct pctree_node*)hvml->vcm_node);
                        SET_VCM_NODE(node);
                    }
                    struct pcvcm_node* parent = (struct pcvcm_node*)
                        pctree_node_parent((struct pctree_node*)hvml->vcm_node);
                    if (parent) {
                        SET_VCM_NODE(parent);
                    }
                    if (pcutils_stack_is_empty(hvml->ejson_stack)) {
                        ADVANCE_TO(PCHVML_EJSON_FINISHED_STATE);
                    }
                    ADVANCE_TO(PCHVML_EJSON_AFTER_VALUE_STATE);
                }
                PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_RIGHT_BRACKET);
                RETURN_AND_STOP_PARSE();
            }
            if (pcutils_stack_is_empty(hvml->ejson_stack)
                    || uc == '(' || uc == '<') {
                RECONSUME_IN(PCHVML_EJSON_CONTROL_STATE);
            }
            ADVANCE_TO(PCHVML_EJSON_CONTROL_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_LESS_THAN_SIGN_STATE)
        // TODO: remove
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_GREATER_THAN_SIGN_STATE)
        // TODO: remove
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_LEFT_PARENTHESIS_STATE)
            if (character == '!') {
                if (hvml->vcm_node->type ==
                        PCVCM_NODE_TYPE_FUNC_GET_VARIABLE ||
                        hvml->vcm_node->type ==
                        PCVCM_NODE_TYPE_FUNC_GET_ELEMENT) {
                    struct pcvcm_node* node = pcvcm_node_new_call_setter(NULL,
                            0, NULL);
                    if (hvml->vcm_node) {
                        pctree_node_append_child(
                                (struct pctree_node*)node,
                                (struct pctree_node*)hvml->vcm_node);
                    }
                    SET_VCM_NODE(node);
                    pcutils_stack_push(hvml->ejson_stack, '<');
                    ADVANCE_TO(PCHVML_EJSON_CONTROL_STATE);
                }
                PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
                RETURN_AND_STOP_PARSE();
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_TAG);
                RETURN_NEW_EOF_TOKEN();
            }
            if (hvml->vcm_node->type ==
                    PCVCM_NODE_TYPE_FUNC_GET_VARIABLE ||
                    hvml->vcm_node->type ==
                    PCVCM_NODE_TYPE_FUNC_GET_ELEMENT) {
                struct pcvcm_node* node = pcvcm_node_new_call_getter(NULL,
                        0, NULL);
                if (hvml->vcm_node) {
                    pctree_node_append_child(
                            (struct pctree_node*)node,
                            (struct pctree_node*)hvml->vcm_node);
                }
                SET_VCM_NODE(node);
                pcutils_stack_push(hvml->ejson_stack, '(');
                RECONSUME_IN(PCHVML_EJSON_CONTROL_STATE);
            }
            if (pcutils_stack_is_empty(hvml->ejson_stack)) {
                RECONSUME_IN(PCHVML_EJSON_FINISHED_STATE);
            }
            PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
            RETURN_AND_STOP_PARSE();
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_RIGHT_PARENTHESIS_STATE)
            if (character == ')') {
                uint32_t uc = pcutils_stack_top(hvml->ejson_stack);
                if (uc == '(' || uc == '<') {
                    pcutils_stack_pop(hvml->ejson_stack);
                    if (!pcvcm_stack_is_empty(hvml->vcm_stack)) {
                        struct pcvcm_node* node = pcvcm_stack_pop(
                                hvml->vcm_stack);
                        if (hvml->vcm_node) {
                            pctree_node_append_child((struct pctree_node*)node,
                                    (struct pctree_node*)hvml->vcm_node);
                        }
                        SET_VCM_NODE(node);
                    }
                    ADVANCE_TO(PCHVML_EJSON_CONTROL_STATE);
                }
                if (pcutils_stack_is_empty(hvml->ejson_stack)) {
                    PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
                    RETURN_AND_STOP_PARSE();
                }
                ADVANCE_TO(PCHVML_EJSON_CONTROL_STATE);
            }
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_DOLLAR_STATE)
            if (is_whitespace(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
                RETURN_AND_STOP_PARSE();
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_TAG);
                RETURN_NEW_EOF_TOKEN();
            }
            if (character == '$') {
                if (hvml->vcm_node) {
                    pcvcm_stack_push(hvml->vcm_stack, hvml->vcm_node);
                }
                pcutils_stack_push(hvml->ejson_stack, '$');
                SET_VCM_NODE(pcvcm_node_new_get_variable(NULL));
                ADVANCE_TO(PCHVML_EJSON_DOLLAR_STATE);
            }
            if (character == '{') {
                pcutils_stack_push(hvml->ejson_stack, 'P');
                RESET_TEMP_BUFFER();
                ADVANCE_TO(PCHVML_EJSON_JSONEE_VARIABLE_STATE);
            }
            RESET_TEMP_BUFFER();
            RECONSUME_IN(PCHVML_EJSON_JSONEE_VARIABLE_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_AFTER_VALUE_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(PCHVML_EJSON_AFTER_VALUE_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_TAG);
                RETURN_NEW_EOF_TOKEN();
            }
            if (character == '"' || character == '\'') {
                struct pcvcm_node* node = pcvcm_node_new_string(
                        pchvml_buffer_get_buffer(hvml->temp_buffer));
                pctree_node_append_child((struct pctree_node*)hvml->vcm_node,
                        (struct pctree_node*)node);
                RESET_TEMP_BUFFER();
                ADVANCE_TO(PCHVML_EJSON_AFTER_VALUE_STATE);
            }
            if (character == '}') {
                RECONSUME_IN(PCHVML_EJSON_RIGHT_BRACE_STATE);
            }
            if (character == ']') {
                RECONSUME_IN(PCHVML_EJSON_RIGHT_BRACKET_STATE);
            }
            if (character == ')') {
                RECONSUME_IN(PCHVML_EJSON_RIGHT_PARENTHESIS_STATE);
            }
            if (character == ',') {
                uint32_t uc = pcutils_stack_top(hvml->ejson_stack);
                if (uc == '{') {
                    pcutils_stack_pop(hvml->ejson_stack);
                    ADVANCE_TO(PCHVML_EJSON_BEFORE_NAME_STATE);
                }
                if (uc == '[') {
                    if (!pchvml_buffer_is_empty(hvml->temp_buffer)) {
                        struct pcvcm_node* node = pcvcm_node_new_string(
                        pchvml_buffer_get_buffer(hvml->temp_buffer));
                        pctree_node_append_child(
                                (struct pctree_node*)hvml->vcm_node,
                                (struct pctree_node*)node);
                        RESET_TEMP_BUFFER();
                    }
                    if (hvml->vcm_node &&
                            hvml->vcm_node->type != PCVCM_NODE_TYPE_ARRAY) {
                        struct pcvcm_node* node = pcvcm_stack_pop(
                                hvml->vcm_stack);
                        pctree_node_append_child(
                                (struct pctree_node*)node,
                                (struct pctree_node*)hvml->vcm_node);
                        SET_VCM_NODE(node);
                    }
                    ADVANCE_TO(PCHVML_EJSON_CONTROL_STATE);
                }
                if (uc == '(') {
                    ADVANCE_TO(PCHVML_EJSON_CONTROL_STATE);
                }
                if (uc == '<') {
                    ADVANCE_TO(PCHVML_EJSON_CONTROL_STATE);
                }
                if (uc == ':') {
                    pcutils_stack_pop(hvml->ejson_stack);
                    if (!pchvml_buffer_is_empty(hvml->temp_buffer)) {
                        struct pcvcm_node* node = pcvcm_node_new_string(
                        pchvml_buffer_get_buffer(hvml->temp_buffer));
                        pctree_node_append_child(
                                (struct pctree_node*)hvml->vcm_node,
                                (struct pctree_node*)node);
                        RESET_TEMP_BUFFER();
                    }
                    if (hvml->vcm_node &&
                            hvml->vcm_node->type != PCVCM_NODE_TYPE_OBJECT) {
                        struct pcvcm_node* node = pcvcm_stack_pop(
                                hvml->vcm_stack);
                        pctree_node_append_child(
                                (struct pctree_node*)node,
                                (struct pctree_node*)hvml->vcm_node);
                        SET_VCM_NODE(node);
                    }
                    ADVANCE_TO(PCHVML_EJSON_BEFORE_NAME_STATE);
                }
                PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
                RETURN_AND_STOP_PARSE();
            }
            PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
            RETURN_AND_STOP_PARSE();
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_BEFORE_NAME_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(PCHVML_EJSON_BEFORE_NAME_STATE);
            }
            uint32_t uc = pcutils_stack_top(hvml->ejson_stack);
            if (character == '"') {
                RESET_TEMP_BUFFER();
                if (uc == '{') {
                    pcutils_stack_push(hvml->ejson_stack, ':');
                }
                RECONSUME_IN(PCHVML_EJSON_NAME_DOUBLE_QUOTED_STATE);
            }
            if (character == '\'') {
                RESET_TEMP_BUFFER();
                if (uc == '{') {
                    pcutils_stack_push(hvml->ejson_stack, ':');
                }
                RECONSUME_IN(PCHVML_EJSON_NAME_SINGLE_QUOTED_STATE);
            }
            if (character == '}') {
                RECONSUME_IN(PCHVML_EJSON_RIGHT_BRACE_STATE);
            }
            if (character == '$') {
                RECONSUME_IN(PCHVML_EJSON_CONTROL_STATE);
            }
            if (is_ascii_alpha(character)) {
                RESET_TEMP_BUFFER();
                if (uc == '{') {
                    pcutils_stack_push(hvml->ejson_stack, ':');
                }
                RECONSUME_IN(PCHVML_EJSON_NAME_UNQUOTED_STATE);
            }
            PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
            RETURN_AND_STOP_PARSE();
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_AFTER_NAME_STATE)
            if (is_whitespace(character)) {
                ADVANCE_TO(PCHVML_EJSON_AFTER_NAME_STATE);
            }
            if (character == ':') {
                if (!pchvml_buffer_is_empty(hvml->temp_buffer)) {
                    struct pcvcm_node* node = pcvcm_node_new_string(
                        pchvml_buffer_get_buffer(hvml->temp_buffer));
                    pctree_node_append_child((struct pctree_node*)hvml->vcm_node,
                            (struct pctree_node*)node);
                }
                ADVANCE_TO(PCHVML_EJSON_CONTROL_STATE);
            }
            PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
            RETURN_AND_STOP_PARSE();
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_NAME_UNQUOTED_STATE)
            if (is_whitespace(character) || character == ':') {
                RECONSUME_IN(PCHVML_EJSON_AFTER_NAME_STATE);
            }
            if (is_ascii_alpha(character) || is_ascii_digit(character)
                    || character == '-' || character == '_') {
                APPEND_TO_TEMP_BUFFER(character);
                ADVANCE_TO(PCHVML_EJSON_NAME_UNQUOTED_STATE);
            }
            if (character == '$') {
                if (hvml->vcm_node) {
                    pcvcm_stack_push(hvml->vcm_stack, hvml->vcm_node);
                }
                SET_VCM_NODE(pcvcm_node_new_concat_string(0, NULL));
                pcutils_stack_push(hvml->ejson_stack, 'U');
                if (!pchvml_buffer_is_empty(hvml->temp_buffer)) {
                    struct pcvcm_node* node = pcvcm_node_new_string(
                            pchvml_buffer_get_buffer(hvml->temp_buffer)
                            );
                    pctree_node_append_child(
                            (struct pctree_node*)hvml->vcm_node,
                            (struct pctree_node*)node);
                    RESET_TEMP_BUFFER();
                }
                RECONSUME_IN(PCHVML_EJSON_CONTROL_STATE);
            }
            PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
            RETURN_AND_STOP_PARSE();
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_NAME_SINGLE_QUOTED_STATE)
            if (character == '\'') {
                size_t nr_buf_chars = pchvml_buffer_get_size_in_chars(
                        hvml->temp_buffer);
                if (nr_buf_chars >= 1) {
                    ADVANCE_TO(PCHVML_EJSON_AFTER_NAME_STATE);
                }
                else {
                    ADVANCE_TO(PCHVML_EJSON_NAME_SINGLE_QUOTED_STATE);
                }
            }
            if (character == '\\') {
                SET_RETURN_STATE(PCHVML_EJSON_NAME_SINGLE_QUOTED_STATE);
                ADVANCE_TO(PCHVML_EJSON_STRING_ESCAPE_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_TAG);
                RETURN_NEW_EOF_TOKEN();
            }
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(PCHVML_EJSON_NAME_SINGLE_QUOTED_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_NAME_DOUBLE_QUOTED_STATE)
            if (character == '"') {
                size_t nr_buf_chars = pchvml_buffer_get_size_in_chars(
                        hvml->temp_buffer);
                if (nr_buf_chars > 1) {
                    pchvml_buffer_delete_head_chars (hvml->temp_buffer, 1);
                    ADVANCE_TO(PCHVML_EJSON_AFTER_NAME_STATE);
                }
                else if (nr_buf_chars == 1) {
                    RESET_TEMP_BUFFER();
                    struct pcvcm_node* node = pcvcm_node_new_string ("");
                    if (!hvml->vcm_node) {
                        hvml->vcm_node = pcvcm_stack_pop(
                                hvml->vcm_stack);
                    }
                    pctree_node_append_child(
                            (struct pctree_node*)hvml->vcm_node,
                            (struct pctree_node*)node);
                    ADVANCE_TO(PCHVML_EJSON_AFTER_NAME_STATE);
                }
                APPEND_TO_TEMP_BUFFER(character);
                ADVANCE_TO(PCHVML_EJSON_NAME_DOUBLE_QUOTED_STATE);
            }
            if (character == '\\') {
                SET_RETURN_STATE(curr_state);
                ADVANCE_TO(PCHVML_EJSON_STRING_ESCAPE_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_TAG);
                RETURN_NEW_EOF_TOKEN();
            }
            if (character == '$') {
                if (hvml->vcm_node) {
                    pcvcm_stack_push(hvml->vcm_stack, hvml->vcm_node);
                }
                SET_VCM_NODE(pcvcm_node_new_concat_string(0, NULL));
                pcutils_stack_push(hvml->ejson_stack, '"');
                if (!pchvml_buffer_is_empty(hvml->temp_buffer)) {
                    struct pcvcm_node* node = pcvcm_node_new_string(
                            pchvml_buffer_get_buffer(hvml->temp_buffer)
                            );
                    pctree_node_append_child(
                            (struct pctree_node*)hvml->vcm_node,
                            (struct pctree_node*)node);
                    RESET_TEMP_BUFFER();
                }
                RECONSUME_IN(PCHVML_EJSON_CONTROL_STATE);
            }
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(PCHVML_EJSON_NAME_DOUBLE_QUOTED_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_VALUE_SINGLE_QUOTED_STATE)
            if (character == '\'') {
                size_t nr_buf_chars = pchvml_buffer_get_size_in_chars(
                        hvml->temp_buffer);
                if (nr_buf_chars >= 1) {
                    RECONSUME_IN(PCHVML_EJSON_AFTER_VALUE_STATE);
                }
                else {
                    ADVANCE_TO(PCHVML_EJSON_VALUE_SINGLE_QUOTED_STATE);
                }
            }
            if (character == '\\') {
                SET_RETURN_STATE(curr_state);
                ADVANCE_TO(PCHVML_EJSON_STRING_ESCAPE_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_TAG);
                RETURN_NEW_EOF_TOKEN();
            }
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(PCHVML_EJSON_VALUE_SINGLE_QUOTED_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_VALUE_DOUBLE_QUOTED_STATE)
            if (character == '"') {
                if (pchvml_buffer_is_empty(hvml->temp_buffer)) {
                    APPEND_TO_TEMP_BUFFER(character);
                    ADVANCE_TO(PCHVML_EJSON_VALUE_DOUBLE_QUOTED_STATE);
                }
                else if (pchvml_buffer_equal_to(hvml->temp_buffer, "\"",
                            1)) {
                    RECONSUME_IN(PCHVML_EJSON_VALUE_TWO_DOUBLE_QUOTED_STATE);
                }
                RECONSUME_IN(PCHVML_EJSON_AFTER_VALUE_DOUBLE_QUOTED_STATE);
            }
            if (character == '\\') {
                SET_RETURN_STATE(curr_state);
                ADVANCE_TO(PCHVML_EJSON_STRING_ESCAPE_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_TAG);
                RETURN_NEW_EOF_TOKEN();
            }
            if (character == '$') {
                if (hvml->vcm_node) {
                    pcvcm_stack_push(hvml->vcm_stack, hvml->vcm_node);
                }
                SET_VCM_NODE(pcvcm_node_new_concat_string(0, NULL));
                pcutils_stack_push(hvml->ejson_stack, '"');
                if (!pchvml_buffer_is_empty(hvml->temp_buffer)) {
                    struct pcvcm_node* node = pcvcm_node_new_string(
                            pchvml_buffer_get_buffer(hvml->temp_buffer)
                            );
                    pctree_node_append_child(
                            (struct pctree_node*)hvml->vcm_node,
                            (struct pctree_node*)node);
                    RESET_TEMP_BUFFER();
                }
                RECONSUME_IN(PCHVML_EJSON_CONTROL_STATE);
            }
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(PCHVML_EJSON_VALUE_DOUBLE_QUOTED_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_AFTER_VALUE_DOUBLE_QUOTED_STATE)
            if (character == '\"') {
                pchvml_buffer_delete_head_chars(hvml->temp_buffer, 1);
                RECONSUME_IN(PCHVML_EJSON_AFTER_VALUE_STATE);
            }
            PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
            RETURN_AND_STOP_PARSE();
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_VALUE_TWO_DOUBLE_QUOTED_STATE)
            if (character == '"') {
                if (pchvml_buffer_equal_to(hvml->temp_buffer, "\"", 1)) {
                    APPEND_TO_TEMP_BUFFER(character);
                    ADVANCE_TO(PCHVML_EJSON_VALUE_TWO_DOUBLE_QUOTED_STATE);
                }
                if (pchvml_buffer_equal_to(hvml->temp_buffer, "\"\"", 2)) {
                    RECONSUME_IN(PCHVML_EJSON_VALUE_THREE_DOUBLE_QUOTED_STATE);
                }
            }
            pchvml_buffer_delete_head_chars(hvml->temp_buffer, 1);
            pchvml_buffer_delete_tail_chars(hvml->temp_buffer, 1);
            struct pcvcm_node* node = pcvcm_node_new_string(
                    pchvml_buffer_get_buffer(hvml->temp_buffer)
                    );
            if (!hvml->vcm_node) {
                SET_VCM_NODE(pcvcm_stack_pop(hvml->vcm_stack));
            }
            pctree_node_append_child(
                    (struct pctree_node*)hvml->vcm_node,
                    (struct pctree_node*)node);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(PCHVML_EJSON_AFTER_VALUE_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_VALUE_THREE_DOUBLE_QUOTED_STATE)
            if (character == '\"') {
                APPEND_TO_TEMP_BUFFER(character);
                size_t buf_len = pchvml_buffer_get_size_in_chars(
                        hvml->temp_buffer);
                if (buf_len >= 6
                        && pchvml_buffer_end_with(hvml->temp_buffer,
                            "\"\"\"", 3)) {
                    pchvml_buffer_delete_head_chars(hvml->temp_buffer, 3);
                    pchvml_buffer_delete_tail_chars(hvml->temp_buffer, 3);
                    struct pcvcm_node* node = pcvcm_node_new_string(
                            pchvml_buffer_get_buffer(hvml->temp_buffer)
                            );
                    if (!hvml->vcm_node) {
                        hvml->vcm_node = pcvcm_stack_pop(
                                hvml->vcm_stack);
                    }
                    pctree_node_append_child(
                            (struct pctree_node*)hvml->vcm_node,
                            (struct pctree_node*)node);
                    RESET_TEMP_BUFFER();
                    ADVANCE_TO(PCHVML_EJSON_AFTER_VALUE_STATE);
                }
                ADVANCE_TO(PCHVML_EJSON_VALUE_THREE_DOUBLE_QUOTED_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_TAG);
                RETURN_NEW_EOF_TOKEN();
            }
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(PCHVML_EJSON_VALUE_THREE_DOUBLE_QUOTED_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_KEYWORD_STATE)
            if (is_whitespace(character) || character == '}'
                    || character == ']' || character == ','
                    || character == ')') {
                RECONSUME_IN(PCHVML_EJSON_AFTER_KEYWORD_STATE);
            }
            if (character == '$') {
                if (hvml->vcm_node) {
                    pcvcm_stack_push(hvml->vcm_stack, hvml->vcm_node);
                }
                SET_VCM_NODE(pcvcm_node_new_concat_string(0, NULL));
                pcutils_stack_push(hvml->ejson_stack, 'U');
                if (!pchvml_buffer_is_empty(hvml->temp_buffer)) {
                    struct pcvcm_node* node = pcvcm_node_new_string(
                            pchvml_buffer_get_buffer(hvml->temp_buffer)
                            );
                    pctree_node_append_child(
                            (struct pctree_node*)hvml->vcm_node,
                            (struct pctree_node*)node);
                    RESET_TEMP_BUFFER();
                }
                RECONSUME_IN(PCHVML_EJSON_CONTROL_STATE);
            }
            if (character == 't' || character == 'f' || character == 'n') {
                if (pchvml_buffer_is_empty(hvml->temp_buffer)) {
                    APPEND_TO_TEMP_BUFFER(character);
                    ADVANCE_TO(PCHVML_EJSON_KEYWORD_STATE);
                }
                PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_JSON_KEYWORD);
                RETURN_AND_STOP_PARSE();
            }
            if (character == 'r') {
                if (pchvml_buffer_equal_to(hvml->temp_buffer, "t", 1)) {
                    APPEND_TO_TEMP_BUFFER(character);
                    ADVANCE_TO(PCHVML_EJSON_KEYWORD_STATE);
                }
                PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_JSON_KEYWORD);
                RETURN_AND_STOP_PARSE();
            }
            if (character == 'u') {
                if (pchvml_buffer_equal_to(hvml->temp_buffer, "tr", 2)
                   || pchvml_buffer_equal_to(hvml->temp_buffer, "n", 1)) {
                    APPEND_TO_TEMP_BUFFER(character);
                    ADVANCE_TO(PCHVML_EJSON_KEYWORD_STATE);
                }
                PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_JSON_KEYWORD);
                RETURN_AND_STOP_PARSE();
            }
            if (character == 'e') {
                if (pchvml_buffer_equal_to(hvml->temp_buffer, "tru", 3)
                   || pchvml_buffer_equal_to(hvml->temp_buffer, "fals", 4)
                   ) {
                    APPEND_TO_TEMP_BUFFER(character);
                    ADVANCE_TO(PCHVML_EJSON_KEYWORD_STATE);
                }
                PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_JSON_KEYWORD);
                RETURN_AND_STOP_PARSE();
            }
            if (character == 'a') {
                if (pchvml_buffer_equal_to(hvml->temp_buffer, "f", 1)) {
                    APPEND_TO_TEMP_BUFFER(character);
                    ADVANCE_TO(PCHVML_EJSON_KEYWORD_STATE);
                }
                PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_JSON_KEYWORD);
                RETURN_AND_STOP_PARSE();
            }
            if (character == 'l') {
                if (pchvml_buffer_equal_to(hvml->temp_buffer, "nu", 2)
                 || pchvml_buffer_equal_to(hvml->temp_buffer, "nul", 3)
                 || pchvml_buffer_equal_to(hvml->temp_buffer, "fa", 2)) {
                    APPEND_TO_TEMP_BUFFER(character);
                    ADVANCE_TO(PCHVML_EJSON_KEYWORD_STATE);
                }
                PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_JSON_KEYWORD);
                RETURN_AND_STOP_PARSE();
            }
            if (character == 's') {
                if (pchvml_buffer_equal_to(hvml->temp_buffer, "fal", 3)) {
                    APPEND_TO_TEMP_BUFFER(character);
                    ADVANCE_TO(PCHVML_EJSON_KEYWORD_STATE);
                }
                PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_JSON_KEYWORD);
                RETURN_AND_STOP_PARSE();
            }
            PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
            RETURN_AND_STOP_PARSE();
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_AFTER_KEYWORD_STATE)
            if (is_whitespace(character) || character == '}'
                    || character == ']' || character == ','
                    || character == ')') {
                if (pchvml_buffer_equal_to(hvml->temp_buffer, "true", 4)) {
                    if (!hvml->vcm_node) {
                        hvml->vcm_node = pcvcm_stack_pop(
                                hvml->vcm_stack);
                    }
                    struct pcvcm_node* node = pcvcm_node_new_boolean(true);
                    pctree_node_append_child(
                            (struct pctree_node*)hvml->vcm_node,
                            (struct pctree_node*)node);
                    RESET_TEMP_BUFFER();
                    RECONSUME_IN(PCHVML_EJSON_AFTER_VALUE_STATE);
                }
                if (pchvml_buffer_equal_to(hvml->temp_buffer, "false",
                            5)) {
                    if (!hvml->vcm_node) {
                        hvml->vcm_node = pcvcm_stack_pop(
                                hvml->vcm_stack);
                    }
                    struct pcvcm_node* node = pcvcm_node_new_boolean(false);
                    pctree_node_append_child(
                            (struct pctree_node*)hvml->vcm_node,
                            (struct pctree_node*)node);
                    RESET_TEMP_BUFFER();
                    RECONSUME_IN(PCHVML_EJSON_AFTER_VALUE_STATE);
                }
                if (pchvml_buffer_equal_to(hvml->temp_buffer, "null", 4)) {
                    struct pcvcm_node* node = pcvcm_node_new_null();
                    pctree_node_append_child(
                            (struct pctree_node*)hvml->vcm_node,
                            (struct pctree_node*)node);
                    RESET_TEMP_BUFFER();
                    RECONSUME_IN(PCHVML_EJSON_AFTER_VALUE_STATE);
                }
                RESET_TEMP_BUFFER();
                PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
                RETURN_AND_STOP_PARSE();
            }
            RESET_TEMP_BUFFER();
            PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
            RETURN_AND_STOP_PARSE();
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_BYTE_SEQUENCE_STATE)
            if (character == 'b') {
                if (pchvml_buffer_is_empty(hvml->temp_buffer)) {
                    APPEND_TO_TEMP_BUFFER(character);
                    ADVANCE_TO(PCHVML_EJSON_BYTE_SEQUENCE_STATE);
                }
                APPEND_TO_TEMP_BUFFER(character);
                ADVANCE_TO(PCHVML_EJSON_BINARY_BYTE_SEQUENCE_STATE);
            }
            if (character == 'x') {
                APPEND_TO_TEMP_BUFFER(character);
                ADVANCE_TO(PCHVML_EJSON_HEX_BYTE_SEQUENCE_STATE);
            }
            if (character == '6') {
                APPEND_TO_TEMP_BUFFER(character);
                ADVANCE_TO(PCHVML_EJSON_BASE64_BYTE_SEQUENCE_STATE);
            }
            if (character == '$') {
                if (hvml->vcm_node) {
                    pcvcm_stack_push(hvml->vcm_stack, hvml->vcm_node);
                }
                SET_VCM_NODE(pcvcm_node_new_concat_string(0, NULL));
                pcutils_stack_push(hvml->ejson_stack, 'U');
                if (!pchvml_buffer_is_empty(hvml->temp_buffer)) {
                    struct pcvcm_node* node = pcvcm_node_new_string(
                            pchvml_buffer_get_buffer(hvml->temp_buffer)
                            );
                    pctree_node_append_child(
                            (struct pctree_node*)hvml->vcm_node,
                            (struct pctree_node*)node);
                    RESET_TEMP_BUFFER();
                }
                RECONSUME_IN(PCHVML_EJSON_CONTROL_STATE);
            }
            PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
            RETURN_AND_STOP_PARSE();
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_AFTER_BYTE_SEQUENCE_STATE)
            if (is_whitespace(character) || character == '}'
                    || character == ']' || character == ',' ) {
                struct pcvcm_node* node = pchvml_parser_new_byte_sequence(hvml,
                        hvml->temp_buffer);
                if (node == NULL) {
                    PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
                    RETURN_AND_STOP_PARSE();
                }
                if (!hvml->vcm_node) {
                    SET_VCM_NODE(pcvcm_stack_pop(hvml->vcm_stack));
                }
                pctree_node_append_child(
                        (struct pctree_node*)hvml->vcm_node,
                        (struct pctree_node*)node);
                RESET_TEMP_BUFFER();
                RECONSUME_IN(PCHVML_EJSON_AFTER_VALUE_STATE);
            }
            PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
            RETURN_AND_STOP_PARSE();
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_HEX_BYTE_SEQUENCE_STATE)
            if (is_whitespace(character) || character == '}'
                    || character == ']' || character == ',' ) {
                RECONSUME_IN(PCHVML_EJSON_AFTER_BYTE_SEQUENCE_STATE);
            }
            else if (is_ascii_digit(character)
                    || is_ascii_hex_digit(character)) {
                APPEND_TO_TEMP_BUFFER(character);
                ADVANCE_TO(PCHVML_EJSON_HEX_BYTE_SEQUENCE_STATE);
            }
            PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
            RETURN_AND_STOP_PARSE();
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_BINARY_BYTE_SEQUENCE_STATE)
            if (is_whitespace(character) || character == '}'
                    || character == ']' || character == ',' ) {
                RECONSUME_IN(PCHVML_EJSON_AFTER_BYTE_SEQUENCE_STATE);
            }
            else if (is_ascii_binary_digit(character)) {
                APPEND_TO_TEMP_BUFFER(character);
                ADVANCE_TO(PCHVML_EJSON_BINARY_BYTE_SEQUENCE_STATE);
            }
            if (character == '.') {
                ADVANCE_TO(PCHVML_EJSON_BINARY_BYTE_SEQUENCE_STATE);
            }
            PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
            RETURN_AND_STOP_PARSE();
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_BASE64_BYTE_SEQUENCE_STATE)
            if (is_whitespace(character) || character == '}'
                    || character == ']' || character == ',' ) {
                RECONSUME_IN(PCHVML_EJSON_AFTER_BYTE_SEQUENCE_STATE);
            }
            if (character == '=') {
                APPEND_TO_TEMP_BUFFER(character);
                ADVANCE_TO(PCHVML_EJSON_BASE64_BYTE_SEQUENCE_STATE);
            }
            if (is_ascii_digit(character) || is_ascii_alpha(character)
                    || character == '+' || character == '-') {
                if (!pchvml_buffer_end_with(hvml->temp_buffer, "=", 1)) {
                    APPEND_TO_TEMP_BUFFER(character);
                    ADVANCE_TO(PCHVML_EJSON_BASE64_BYTE_SEQUENCE_STATE);
                }
                PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_BASE64);
                RETURN_AND_STOP_PARSE();
            }
            PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
            RETURN_AND_STOP_PARSE();
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_VALUE_NUMBER_STATE)
            if (is_whitespace(character) || character == '}'
                    || character == ']' || character == ',' ) {
                RECONSUME_IN(PCHVML_EJSON_AFTER_VALUE_NUMBER_STATE);
            }
            if (is_ascii_digit(character)) {
                RECONSUME_IN(PCHVML_EJSON_VALUE_NUMBER_INTEGER_STATE);
            }
            if (character == '-') {
                APPEND_TO_TEMP_BUFFER(character);
                ADVANCE_TO(PCHVML_EJSON_VALUE_NUMBER_INTEGER_STATE);
            }
            if (character == '$') {
                if (hvml->vcm_node) {
                    pcvcm_stack_push(hvml->vcm_stack, hvml->vcm_node);
                }
                SET_VCM_NODE(pcvcm_node_new_concat_string(0, NULL));
                pcutils_stack_push(hvml->ejson_stack, 'U');
                if (!pchvml_buffer_is_empty(hvml->temp_buffer)) {
                    struct pcvcm_node* node = pcvcm_node_new_string(
                            pchvml_buffer_get_buffer(hvml->temp_buffer)
                            );
                    pctree_node_append_child(
                            (struct pctree_node*)hvml->vcm_node,
                            (struct pctree_node*)node);
                    RESET_TEMP_BUFFER();
                }
                RECONSUME_IN(PCHVML_EJSON_CONTROL_STATE);
            }
            PCHVML_SET_ERROR(PCHVML_ERROR_BAD_JSON_NUMBER);
            RETURN_AND_STOP_PARSE();
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_AFTER_VALUE_NUMBER_STATE)
            if (is_whitespace(character) || character == '}'
                    || character == ']' || character == ','
                    || character == ')') {
                if (pchvml_buffer_end_with(hvml->temp_buffer, "-", 1)
                    || pchvml_buffer_end_with(hvml->temp_buffer, "E", 1)
                    || pchvml_buffer_end_with(hvml->temp_buffer, "e", 1)) {
                    PCHVML_SET_ERROR(PCHVML_ERROR_BAD_JSON_NUMBER);
                    RETURN_AND_STOP_PARSE();
                }
                double d = strtod(
                        pchvml_buffer_get_buffer(hvml->temp_buffer), NULL);
                struct pcvcm_node* node = pcvcm_node_new_number(d);
                if (!hvml->vcm_node) {
                    SET_VCM_NODE(pcvcm_stack_pop(hvml->vcm_stack));
                }
                pctree_node_append_child(
                        (struct pctree_node*)hvml->vcm_node,
                        (struct pctree_node*)node);
                RESET_TEMP_BUFFER();
                RECONSUME_IN(PCHVML_EJSON_AFTER_VALUE_STATE);
            }
            PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
            RETURN_AND_STOP_PARSE();
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_VALUE_NUMBER_INTEGER_STATE)
            if (is_whitespace(character) || character == '}'
                    || character == ']' || character == ','
                    || character == ')') {
                RECONSUME_IN(PCHVML_EJSON_AFTER_VALUE_NUMBER_STATE);
            }
            if (is_ascii_digit(character)) {
                APPEND_TO_TEMP_BUFFER(character);
                ADVANCE_TO(PCHVML_EJSON_VALUE_NUMBER_INTEGER_STATE);
            }
            if (character == 'E' || character == 'e') {
                APPEND_TO_TEMP_BUFFER('e');
                ADVANCE_TO(PCHVML_EJSON_VALUE_NUMBER_EXPONENT_STATE);
            }
            if (character == '.' || character == 'F') {
                APPEND_TO_TEMP_BUFFER(character);
                ADVANCE_TO(PCHVML_EJSON_VALUE_NUMBER_FRACTION_STATE);
            }
            if (character == 'U' || character == 'L') {
                RECONSUME_IN(PCHVML_EJSON_VALUE_NUMBER_SUFFIX_INTEGER_STATE);
            }
            if (character == 'I' && (
                        pchvml_buffer_is_empty(hvml->temp_buffer) ||
                        pchvml_buffer_equal_to(hvml->temp_buffer, "-", 1)
                        )) {
                RECONSUME_IN(PCHVML_EJSON_VALUE_NUMBER_INFINITY_STATE);
            }
            PCHVML_SET_ERROR(
                    PCHVML_ERROR_UNEXPECTED_JSON_NUMBER_INTEGER);
            RETURN_AND_STOP_PARSE();
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_VALUE_NUMBER_FRACTION_STATE)
            if (is_whitespace(character) || character == '}'
                    || character == ']' || character == ',' ) {
                RECONSUME_IN(PCHVML_EJSON_AFTER_VALUE_NUMBER_STATE);
            }

            if (is_ascii_digit(character)) {
                if (pchvml_buffer_end_with(hvml->temp_buffer, "F", 1)) {
                    PCHVML_SET_ERROR(PCHVML_ERROR_BAD_JSON_NUMBER);
                    RETURN_AND_STOP_PARSE();
                }
                APPEND_TO_TEMP_BUFFER(character);
                ADVANCE_TO(PCHVML_EJSON_VALUE_NUMBER_FRACTION_STATE);
            }
            if (character == 'F') {
                APPEND_TO_TEMP_BUFFER(character);
                ADVANCE_TO(PCHVML_EJSON_VALUE_NUMBER_FRACTION_STATE);
            }
            if (character == 'L') {
                if (pchvml_buffer_end_with(hvml->temp_buffer, "F", 1)) {
                    APPEND_TO_TEMP_BUFFER(character);
                    long double ld = strtold (
                            pchvml_buffer_get_buffer(hvml->temp_buffer), NULL);
                    struct pcvcm_node* node = pcvcm_node_new_longdouble(ld);
                    if (!hvml->vcm_node) {
                        SET_VCM_NODE(pcvcm_stack_pop(hvml->vcm_stack));
                    }
                    pctree_node_append_child(
                            (struct pctree_node*)hvml->vcm_node,
                            (struct pctree_node*)node);
                    RESET_TEMP_BUFFER();
                    ADVANCE_TO(PCHVML_EJSON_AFTER_VALUE_STATE);
                }
            }
            if (character == 'E' || character == 'e') {
                if (pchvml_buffer_end_with(hvml->temp_buffer, ".", 1)) {
                    PCHVML_SET_ERROR(
                            PCHVML_ERROR_UNEXPECTED_JSON_NUMBER_FRACTION);
                    RETURN_AND_STOP_PARSE();
                }
                APPEND_TO_TEMP_BUFFER('e');
                ADVANCE_TO(PCHVML_EJSON_VALUE_NUMBER_EXPONENT_STATE);
            }
            PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER_FRACTION);
            RETURN_AND_STOP_PARSE();
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_VALUE_NUMBER_EXPONENT_STATE)
            if (is_whitespace(character) || character == '}'
                    || character == ']' || character == ',' ) {
                RECONSUME_IN(PCHVML_EJSON_AFTER_VALUE_NUMBER_STATE);
            }
            if (is_ascii_digit(character)) {
                RECONSUME_IN(PCHVML_EJSON_VALUE_NUMBER_EXPONENT_INTEGER_STATE);
            }
            if (character == '+' || character == '-') {
                APPEND_TO_TEMP_BUFFER(character);
                ADVANCE_TO(PCHVML_EJSON_VALUE_NUMBER_EXPONENT_INTEGER_STATE);
            }
            PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER_EXPONENT);
            RETURN_AND_STOP_PARSE();
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_VALUE_NUMBER_EXPONENT_INTEGER_STATE)
            if (is_whitespace(character) || character == '}'
                    || character == ']' || character == ',' ) {
                RECONSUME_IN(PCHVML_EJSON_AFTER_VALUE_NUMBER_STATE);
            }
            if (is_ascii_digit(character)) {
                if (pchvml_buffer_end_with(hvml->temp_buffer, "F", 1)) {
                    PCHVML_SET_ERROR(PCHVML_ERROR_BAD_JSON_NUMBER);
                    RETURN_AND_STOP_PARSE();
                }
                APPEND_TO_TEMP_BUFFER(character);
                ADVANCE_TO(PCHVML_EJSON_VALUE_NUMBER_EXPONENT_INTEGER_STATE);
            }
            if (character == 'F') {
                APPEND_TO_TEMP_BUFFER(character);
                ADVANCE_TO(PCHVML_EJSON_VALUE_NUMBER_EXPONENT_INTEGER_STATE);
            }
            if (character == 'L') {
                if (pchvml_buffer_end_with(hvml->temp_buffer, "F", 1)) {
                    APPEND_TO_TEMP_BUFFER(character);
                    long double ld = strtold (
                            pchvml_buffer_get_buffer(hvml->temp_buffer), NULL);
                    struct pcvcm_node* node = pcvcm_node_new_longdouble(ld);
                    if (!hvml->vcm_node) {
                        SET_VCM_NODE(pcvcm_stack_pop(hvml->vcm_stack));
                    }
                    pctree_node_append_child(
                            (struct pctree_node*)hvml->vcm_node,
                            (struct pctree_node*)node);
                    RESET_TEMP_BUFFER();
                    ADVANCE_TO(PCHVML_EJSON_AFTER_VALUE_NUMBER_STATE);
                }
            }
            PCHVML_SET_ERROR(
                    PCHVML_ERROR_UNEXPECTED_JSON_NUMBER_EXPONENT);
            RETURN_AND_STOP_PARSE();
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_VALUE_NUMBER_SUFFIX_INTEGER_STATE)
            uint32_t last_c = pchvml_buffer_get_last_char(
                    hvml->temp_buffer);
            if (is_whitespace(character) || character == '}'
                    || character == ']' || character == ',' ) {
                RECONSUME_IN(PCHVML_EJSON_AFTER_VALUE_NUMBER_STATE);
            }
            if (character == 'U') {
                if (is_ascii_digit(last_c)) {
                    APPEND_TO_TEMP_BUFFER(character);
                    ADVANCE_TO(PCHVML_EJSON_VALUE_NUMBER_SUFFIX_INTEGER_STATE);
                }
            }
            if (character == 'L') {
                if (is_ascii_digit(last_c) || last_c == 'U') {
                    APPEND_TO_TEMP_BUFFER(character);
                    if (pchvml_buffer_end_with(hvml->temp_buffer, "UL", 2)
                            ) {
                        uint64_t u64 = strtoull (
                            pchvml_buffer_get_buffer(hvml->temp_buffer),
                            NULL, 10);
                        struct pcvcm_node* node = pcvcm_node_new_ulongint(u64);
                        if (!hvml->vcm_node) {
                            hvml->vcm_node = pcvcm_stack_pop(
                                    hvml->vcm_stack);
                        }
                        pctree_node_append_child(
                                (struct pctree_node*)hvml->vcm_node,
                                (struct pctree_node*)node);
                        RESET_TEMP_BUFFER();
                        ADVANCE_TO(PCHVML_EJSON_AFTER_VALUE_STATE);
                    }
                    else if (pchvml_buffer_end_with(hvml->temp_buffer,
                                "L", 1)) {
                        int64_t i64 = strtoll (
                            pchvml_buffer_get_buffer(hvml->temp_buffer),
                            NULL, 10);
                        struct pcvcm_node* node = pcvcm_node_new_longint(i64);
                        if (!hvml->vcm_node) {
                            hvml->vcm_node = pcvcm_stack_pop(
                                    hvml->vcm_stack);
                        }
                        pctree_node_append_child(
                                (struct pctree_node*)hvml->vcm_node,
                                (struct pctree_node*)node);
                        RESET_TEMP_BUFFER();
                        ADVANCE_TO(PCHVML_EJSON_AFTER_VALUE_STATE);
                    }
                }
            }
            PCHVML_SET_ERROR(
                    PCHVML_ERROR_UNEXPECTED_JSON_NUMBER_INTEGER);
            RETURN_AND_STOP_PARSE();
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_VALUE_NUMBER_INFINITY_STATE)
            if (is_whitespace(character) || character == '}'
                    || character == ']' || character == ',' ) {
                if (pchvml_buffer_equal_to(hvml->temp_buffer,
                            "-Infinity", 9)) {
                    double d = -INFINITY;
                    struct pcvcm_node* node = pcvcm_node_new_number(d);
                    if (!hvml->vcm_node) {
                        SET_VCM_NODE(pcvcm_stack_pop(hvml->vcm_stack));
                    }
                    pctree_node_append_child(
                            (struct pctree_node*)hvml->vcm_node,
                            (struct pctree_node*)node);
                    RESET_TEMP_BUFFER();
                    RECONSUME_IN(PCHVML_EJSON_AFTER_VALUE_STATE);
                }
                if (pchvml_buffer_equal_to(hvml->temp_buffer,
                        "Infinity", 8)) {
                    double d = INFINITY;
                    struct pcvcm_node* node = pcvcm_node_new_number(d);
                    if (!hvml->vcm_node) {
                        SET_VCM_NODE(pcvcm_stack_pop(hvml->vcm_stack));
                    }
                    pctree_node_append_child(
                            (struct pctree_node*)hvml->vcm_node,
                            (struct pctree_node*)node);
                    RESET_TEMP_BUFFER();
                    RECONSUME_IN(PCHVML_EJSON_AFTER_VALUE_STATE);
                }
                PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
                RETURN_AND_STOP_PARSE();
            }
            if (character == 'I') {
                if (pchvml_buffer_is_empty(hvml->temp_buffer)
                    || pchvml_buffer_equal_to(hvml->temp_buffer, "-", 1)) {
                    APPEND_TO_TEMP_BUFFER(character);
                    ADVANCE_TO(PCHVML_EJSON_VALUE_NUMBER_INFINITY_STATE);
                }
                PCHVML_SET_ERROR(
                        PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
                RETURN_AND_STOP_PARSE();
            }

            if (character == 'n') {
                if (pchvml_buffer_equal_to(hvml->temp_buffer, "I", 1)
                  || pchvml_buffer_equal_to(hvml->temp_buffer, "-I", 2)
                  || pchvml_buffer_equal_to(hvml->temp_buffer, "Infi", 4)
                  || pchvml_buffer_equal_to(hvml->temp_buffer, "-Infi", 5)
                    ) {
                    APPEND_TO_TEMP_BUFFER(character);
                    ADVANCE_TO(PCHVML_EJSON_VALUE_NUMBER_INFINITY_STATE);
                }
                PCHVML_SET_ERROR(
                        PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
                RETURN_AND_STOP_PARSE();
            }

            if (character == 'f') {
                if (pchvml_buffer_equal_to(hvml->temp_buffer, "In", 2)
                    || pchvml_buffer_equal_to (hvml->temp_buffer, "-In", 3)
                        ) {
                    APPEND_TO_TEMP_BUFFER(character);
                    ADVANCE_TO(PCHVML_EJSON_VALUE_NUMBER_INFINITY_STATE);
                }
                PCHVML_SET_ERROR(
                        PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
                RETURN_AND_STOP_PARSE();
            }

            if (character == 'i') {
                if (pchvml_buffer_equal_to(hvml->temp_buffer, "Inf", 3)
                 || pchvml_buffer_equal_to(hvml->temp_buffer, "-Inf", 4)
                 || pchvml_buffer_equal_to(hvml->temp_buffer, "Infin", 5)
                 || pchvml_buffer_equal_to(hvml->temp_buffer, "-Infin", 6)
                 ) {
                    APPEND_TO_TEMP_BUFFER(character);
                    ADVANCE_TO(PCHVML_EJSON_VALUE_NUMBER_INFINITY_STATE);
                }
                PCHVML_SET_ERROR(
                        PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
                RETURN_AND_STOP_PARSE();
            }

            if (character == 't') {
                if (pchvml_buffer_equal_to(hvml->temp_buffer, "Infini", 6)
                    || pchvml_buffer_equal_to (hvml->temp_buffer,
                        "-Infini", 7)
                        ) {
                    APPEND_TO_TEMP_BUFFER(character);
                    ADVANCE_TO(PCHVML_EJSON_VALUE_NUMBER_INFINITY_STATE);
                }
                PCHVML_SET_ERROR(
                        PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
                RETURN_AND_STOP_PARSE();
            }

            if (character == 'y') {
                if (pchvml_buffer_equal_to(hvml->temp_buffer, "Infinit", 7)
                   || pchvml_buffer_equal_to (hvml->temp_buffer,
                       "-Infinit", 8)
                        ) {
                    APPEND_TO_TEMP_BUFFER(character);
                    ADVANCE_TO(PCHVML_EJSON_VALUE_NUMBER_INFINITY_STATE);
                }
                PCHVML_SET_ERROR(
                        PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
                RETURN_AND_STOP_PARSE();
            }

            PCHVML_SET_ERROR(
                    PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
            RETURN_AND_STOP_PARSE();
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_VALUE_NAN_STATE)
            if (is_whitespace(character) || character == '}'
                    || character == ']' || character == ',' ) {
                if (pchvml_buffer_equal_to(hvml->temp_buffer, "NaN", 3)) {
                    double d = NAN;
                    struct pcvcm_node* node = pcvcm_node_new_number(d);
                    if (!hvml->vcm_node) {
                        SET_VCM_NODE(pcvcm_stack_pop(hvml->vcm_stack));
                    }
                    pctree_node_append_child(
                            (struct pctree_node*)hvml->vcm_node,
                            (struct pctree_node*)node);
                    RESET_TEMP_BUFFER();
                    RECONSUME_IN(PCHVML_EJSON_AFTER_VALUE_STATE);
                }
                PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
                RETURN_AND_STOP_PARSE();
            }
            if (character == 'N') {
                if (pchvml_buffer_is_empty(hvml->temp_buffer)
                  || pchvml_buffer_equal_to(hvml->temp_buffer, "Na", 2)) {
                    APPEND_TO_TEMP_BUFFER(character);
                    ADVANCE_TO(PCHVML_EJSON_VALUE_NAN_STATE);
                }
                PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
                RETURN_AND_STOP_PARSE();
            }

            if (character == 'a') {
                if (pchvml_buffer_equal_to(hvml->temp_buffer, "N", 1)) {
                    APPEND_TO_TEMP_BUFFER(character);
                    ADVANCE_TO(PCHVML_EJSON_VALUE_NAN_STATE);
                }
                PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
                RETURN_AND_STOP_PARSE();
            }

            PCHVML_SET_ERROR(
                    PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
            RETURN_AND_STOP_PARSE();
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_STRING_ESCAPE_STATE)
            switch (character)
            {
                case 'b':
                case 'f':
                case 'n':
                case 'r':
                case 't':
                    APPEND_TO_TEMP_BUFFER('\\');
                    APPEND_TO_TEMP_BUFFER(character);
                    ADVANCE_TO(hvml->return_state);
                    break;
                case '$':
                case '{':
                case '}':
                case '<':
                case '>':
                case '/':
                case '\\':
                case '"':
                    APPEND_TO_TEMP_BUFFER(character);
                    ADVANCE_TO(hvml->return_state);
                    break;
                case 'u':
                    pchvml_buffer_reset(hvml->escape_buffer);
                    ADVANCE_TO(
                      PCHVML_EJSON_STRING_ESCAPE_FOUR_HEXADECIMAL_DIGITS_STATE);
                    break;
                default:
                    PCHVML_SET_ERROR(
                         PCHVML_ERROR_BAD_JSON_STRING_ESCAPE_ENTITY);
                    RETURN_AND_STOP_PARSE();
            }
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_STRING_ESCAPE_FOUR_HEXADECIMAL_DIGITS_STATE)
            if (is_ascii_hex_digit(character)) {
                APPEND_TO_ESCAPE_BUFFER(character);
                size_t nr_chars = pchvml_buffer_get_size_in_chars(
                        hvml->escape_buffer);
                if (nr_chars == 4) {
                    APPEND_BYTES_TO_TEMP_BUFFER("\\u", 2);
                    APPEND_BUFFER_TO_TEMP_BUFFER(hvml->escape_buffer);
                    pchvml_buffer_reset(hvml->escape_buffer);
                    ADVANCE_TO(hvml->return_state);
                }
                ADVANCE_TO(
                    PCHVML_EJSON_STRING_ESCAPE_FOUR_HEXADECIMAL_DIGITS_STATE);
            }
            PCHVML_SET_ERROR(
                    PCHVML_ERROR_BAD_JSON_STRING_ESCAPE_ENTITY);
            RETURN_AND_STOP_PARSE();
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_JSONEE_VARIABLE_STATE)
            if (character == '$') {
                RECONSUME_IN(PCHVML_EJSON_CONTROL_STATE);
            }
            if (character == '#' || character == '%'
                    || character == '?' || character == '@'
                    || character == '&') {
                if (pchvml_buffer_is_empty(hvml->temp_buffer)
                    || pchvml_buffer_is_int(hvml->temp_buffer)) {
                    APPEND_TO_TEMP_BUFFER(character);
                    ADVANCE_TO(PCHVML_EJSON_JSONEE_VARIABLE_STATE);
                }
                PCHVML_SET_ERROR(PCHVML_ERROR_BAD_JSONEE_VARIABLE_NAME);
                RETURN_AND_STOP_PARSE();
            }
            if (character == '_' || is_ascii_digit(character)) {
                APPEND_TO_TEMP_BUFFER(character);
                ADVANCE_TO(PCHVML_EJSON_JSONEE_VARIABLE_STATE);
            }
            if (is_ascii_alpha(character)) {
                APPEND_TO_TEMP_BUFFER(character);
                ADVANCE_TO(PCHVML_EJSON_JSONEE_VARIABLE_STATE);
            }
            if (is_whitespace(character) || character == '}'
                    || character == '"' || character == '$'
                    || character == ']' || character == ')') {
                if (pchvml_buffer_is_empty(hvml->temp_buffer)) {
                    PCHVML_SET_ERROR(PCHVML_ERROR_BAD_JSONEE_VARIABLE_NAME);
                    RETURN_AND_STOP_PARSE();
                }
                if (hvml->vcm_node) {
                    pcvcm_stack_push(hvml->vcm_stack, hvml->vcm_node);
                }
                hvml->vcm_node = pcvcm_node_new_string(
                           pchvml_buffer_get_buffer(hvml->temp_buffer));
                RESET_TEMP_BUFFER();
                uint32_t uc = pcutils_stack_top (hvml->ejson_stack);
                while (uc == '$') {
                    pcutils_stack_pop (hvml->ejson_stack);
                    struct pcvcm_node* node = pcvcm_stack_pop(
                            hvml->vcm_stack);
                    pctree_node_append_child((struct pctree_node*)node,
                                (struct pctree_node*)hvml->vcm_node);
                    SET_VCM_NODE(node);
                    uc = pcutils_stack_top (hvml->ejson_stack);
                }
                if (uc == '(' || uc == '<' || uc == '.') {
                    struct pcvcm_node* node = pcvcm_stack_pop(
                            hvml->vcm_stack);
                    pctree_node_append_child((struct pctree_node*)node,
                                (struct pctree_node*)hvml->vcm_node);
                    SET_VCM_NODE(node);
                }
                RECONSUME_IN(PCHVML_EJSON_CONTROL_STATE);
            }
            if (character == ',') {
                if (pchvml_buffer_is_empty(hvml->temp_buffer)) {
                    PCHVML_SET_ERROR(PCHVML_ERROR_BAD_JSONEE_VARIABLE_NAME);
                    RETURN_AND_STOP_PARSE();
                }
                if (hvml->vcm_node) {
                    pcvcm_stack_push(hvml->vcm_stack, hvml->vcm_node);
                }
                hvml->vcm_node = pcvcm_node_new_string(
                           pchvml_buffer_get_buffer(hvml->temp_buffer));
                RESET_TEMP_BUFFER();
                uint32_t uc = pcutils_stack_top (hvml->ejson_stack);
                while (uc == '$') {
                    pcutils_stack_pop (hvml->ejson_stack);
                    struct pcvcm_node* node = pcvcm_stack_pop(
                            hvml->vcm_stack);
                    pctree_node_append_child((struct pctree_node*)node,
                                (struct pctree_node*)hvml->vcm_node);
                    SET_VCM_NODE(node);
                    uc = pcutils_stack_top (hvml->ejson_stack);
                }
                if (uc == '(') {
                    struct pcvcm_node* node = pcvcm_stack_pop(
                            hvml->vcm_stack);
                    pctree_node_append_child((struct pctree_node*)node,
                                (struct pctree_node*)hvml->vcm_node);
                    SET_VCM_NODE(node);
                }
                if (uc == '<') {
                    struct pcvcm_node* node = pcvcm_stack_pop(
                            hvml->vcm_stack);
                    pctree_node_append_child((struct pctree_node*)node,
                                (struct pctree_node*)hvml->vcm_node);
                    SET_VCM_NODE(node);
                }
                RECONSUME_IN(PCHVML_EJSON_AFTER_VALUE_STATE);
            }
            if (character == ':') {
                if (pchvml_buffer_is_empty(hvml->temp_buffer)) {
                    PCHVML_SET_ERROR(PCHVML_ERROR_BAD_JSONEE_VARIABLE_NAME);
                    RETURN_AND_STOP_PARSE();
                }
                if (hvml->vcm_node) {
                    pcvcm_stack_push(hvml->vcm_stack, hvml->vcm_node);
                }
                hvml->vcm_node = pcvcm_node_new_string(
                           pchvml_buffer_get_buffer(hvml->temp_buffer));
                RESET_TEMP_BUFFER();
                uint32_t uc = pcutils_stack_top (hvml->ejson_stack);
                while (uc == '$') {
                    pcutils_stack_pop (hvml->ejson_stack);
                    struct pcvcm_node* node = pcvcm_stack_pop(
                            hvml->vcm_stack);
                    pctree_node_append_child((struct pctree_node*)node,
                                (struct pctree_node*)hvml->vcm_node);
                    SET_VCM_NODE(node);
                    uc = pcutils_stack_top(hvml->ejson_stack);
                }
                if (uc == '(') {
                    struct pcvcm_node* node = pcvcm_stack_pop(
                            hvml->vcm_stack);
                    pctree_node_append_child((struct pctree_node*)node,
                                (struct pctree_node*)hvml->vcm_node);
                    SET_VCM_NODE(node);
                }
                if (uc == '<') {
                    struct pcvcm_node* node = pcvcm_stack_pop(
                            hvml->vcm_stack);
                    pctree_node_append_child((struct pctree_node*)node,
                                (struct pctree_node*)hvml->vcm_node);
                    SET_VCM_NODE(node);
                }
                if (uc == 'P') {
                    pcutils_stack_pop(hvml->ejson_stack);
                    pcutils_stack_push(hvml->ejson_stack, '{');
                    pcutils_stack_push(hvml->ejson_stack, ':');
                    struct pcvcm_node* node = pcvcm_node_new_object(0, NULL);
                    pctree_node_append_child((struct pctree_node*)node,
                                (struct pctree_node*)hvml->vcm_node);
                    SET_VCM_NODE(node);
                }
                if (uc == '{') {
                    struct pcvcm_node* node = pcvcm_stack_pop(
                            hvml->vcm_stack);
                    pctree_node_append_child((struct pctree_node*)node,
                                (struct pctree_node*)hvml->vcm_node);
                    SET_VCM_NODE(node);
                }
                ADVANCE_TO(PCHVML_EJSON_CONTROL_STATE);
            }
            if (character == '[' || character == '(') {
                if (pchvml_buffer_is_empty(hvml->temp_buffer)) {
                    PCHVML_SET_ERROR(PCHVML_ERROR_BAD_JSONEE_VARIABLE_NAME);
                    RETURN_AND_STOP_PARSE();
                }
                if (hvml->vcm_node) {
                    pcvcm_stack_push(hvml->vcm_stack, hvml->vcm_node);
                }
                hvml->vcm_node = pcvcm_node_new_string(
                           pchvml_buffer_get_buffer(hvml->temp_buffer));
                RESET_TEMP_BUFFER();
                uint32_t uc = pcutils_stack_top (hvml->ejson_stack);
                if (uc == '$') {
                    pcutils_stack_pop (hvml->ejson_stack);
                    struct pcvcm_node* node = pcvcm_stack_pop(
                            hvml->vcm_stack);
                    pctree_node_append_child((struct pctree_node*)node,
                                (struct pctree_node*)hvml->vcm_node);
                    SET_VCM_NODE(node);
                }
                RECONSUME_IN(PCHVML_EJSON_CONTROL_STATE);
            }
            if (character == '<' || character == '>') {
                // FIXME
                if (pchvml_buffer_is_empty(hvml->temp_buffer)) {
                    APPEND_TO_TEMP_BUFFER(character);
                }
                if (hvml->vcm_node) {
                    pcvcm_stack_push(hvml->vcm_stack, hvml->vcm_node);
                }
                hvml->vcm_node = pcvcm_node_new_string(
                           pchvml_buffer_get_buffer(hvml->temp_buffer));
                RESET_TEMP_BUFFER();
                uint32_t uc = pcutils_stack_top (hvml->ejson_stack);
                if (uc == '$') {
                    pcutils_stack_pop (hvml->ejson_stack);
                    struct pcvcm_node* node = pcvcm_stack_pop(
                            hvml->vcm_stack);
                    pctree_node_append_child((struct pctree_node*)node,
                                (struct pctree_node*)hvml->vcm_node);
                    SET_VCM_NODE(node);
                }
                RECONSUME_IN(PCHVML_EJSON_CONTROL_STATE);
            }
            if (character == '.') {
                if (pchvml_buffer_is_empty(hvml->temp_buffer)) {
                    PCHVML_SET_ERROR(PCHVML_ERROR_BAD_JSONEE_VARIABLE_NAME);
                    RETURN_AND_STOP_PARSE();
                }
                if (hvml->vcm_node) {
                    pcvcm_stack_push(hvml->vcm_stack, hvml->vcm_node);
                }
                hvml->vcm_node = pcvcm_node_new_string(
                           pchvml_buffer_get_buffer(hvml->temp_buffer));
                RESET_TEMP_BUFFER();
                uint32_t uc = pcutils_stack_top (hvml->ejson_stack);
                if (uc == '$') {
                    pcutils_stack_pop (hvml->ejson_stack);
                    struct pcvcm_node* node = pcvcm_stack_pop(
                            hvml->vcm_stack);
                    pctree_node_append_child((struct pctree_node*)node,
                                (struct pctree_node*)hvml->vcm_node);
                    SET_VCM_NODE(node);
                }
                RECONSUME_IN(PCHVML_EJSON_JSONEE_FULL_STOP_SIGN_STATE);
            }
            PCHVML_SET_ERROR(PCHVML_ERROR_BAD_JSONEE_VARIABLE_NAME);
            RETURN_AND_STOP_PARSE();
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_JSONEE_FULL_STOP_SIGN_STATE)
            if (character == '.') {
                pcutils_stack_push(hvml->ejson_stack, '.');
                struct pcvcm_node* node = pcvcm_node_new_get_element(NULL,
                        NULL);
                if (hvml->vcm_node) {
                    pctree_node_append_child(
                            (struct pctree_node*)node,
                            (struct pctree_node*)hvml->vcm_node);
                }
                SET_VCM_NODE(node);
                ADVANCE_TO(PCHVML_EJSON_JSONEE_KEYWORD_STATE);
            }
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_JSONEE_KEYWORD_STATE)
            if (is_ascii_digit(character)) {
                if (pchvml_buffer_is_empty(hvml->temp_buffer)) {
                    PCHVML_SET_ERROR(PCHVML_ERROR_BAD_JSONEE_KEYWORD);
                    RETURN_AND_STOP_PARSE();
                }
                APPEND_TO_TEMP_BUFFER(character);
                ADVANCE_TO(PCHVML_EJSON_JSONEE_KEYWORD_STATE);
            }
            if (is_ascii_alpha(character) || character == '_') {
                APPEND_TO_TEMP_BUFFER(character);
                ADVANCE_TO(PCHVML_EJSON_JSONEE_KEYWORD_STATE);
            }
            if (is_whitespace(character) || character == '[' ||
                    character == '(' || character == '<' || character == '}' ||
                    character == '$' || character == '>' || character == ']'
                    || character == ')') {
                if (pchvml_buffer_is_empty(hvml->temp_buffer)) {
                    PCHVML_SET_ERROR(PCHVML_ERROR_BAD_JSONEE_KEYWORD);
                    RETURN_AND_STOP_PARSE();
                }
                if (hvml->vcm_node) {
                    pcvcm_stack_push(hvml->vcm_stack, hvml->vcm_node);
                }
                hvml->vcm_node = pcvcm_node_new_string(
                           pchvml_buffer_get_buffer(hvml->temp_buffer));
                RESET_TEMP_BUFFER();
                pcutils_stack_pop(hvml->ejson_stack);
                struct pcvcm_node* node = pcvcm_stack_pop(
                        hvml->vcm_stack);
                pctree_node_append_child((struct pctree_node*)node,
                        (struct pctree_node*)hvml->vcm_node);
                SET_VCM_NODE(node);
                RECONSUME_IN(PCHVML_EJSON_CONTROL_STATE);
            }
            if (character == ',') {
                if (pchvml_buffer_is_empty(hvml->temp_buffer)) {
                    PCHVML_SET_ERROR(PCHVML_ERROR_BAD_JSONEE_KEYWORD);
                    RETURN_AND_STOP_PARSE();
                }
                if (hvml->vcm_node) {
                    pcvcm_stack_push(hvml->vcm_stack, hvml->vcm_node);
                }
                hvml->vcm_node = pcvcm_node_new_string(
                           pchvml_buffer_get_buffer(hvml->temp_buffer));
                RESET_TEMP_BUFFER();
                pcutils_stack_pop(hvml->ejson_stack);
                struct pcvcm_node* node = pcvcm_stack_pop(
                        hvml->vcm_stack);
                pctree_node_append_child((struct pctree_node*)node,
                        (struct pctree_node*)hvml->vcm_node);
                SET_VCM_NODE(node);
                uint32_t uc = pcutils_stack_top (hvml->ejson_stack);
                if (uc == '(' || uc == '<') {
                    struct pcvcm_node* node = pcvcm_stack_pop(
                            hvml->vcm_stack);
                    pctree_node_append_child((struct pctree_node*)node,
                                (struct pctree_node*)hvml->vcm_node);
                    SET_VCM_NODE(node);
                }
                RECONSUME_IN(PCHVML_EJSON_AFTER_VALUE_STATE);
            }
            if (character == '.') {
                if (pchvml_buffer_is_empty(hvml->temp_buffer)) {
                    PCHVML_SET_ERROR(PCHVML_ERROR_BAD_JSONEE_KEYWORD);
                    RETURN_AND_STOP_PARSE();
                }
                if (hvml->vcm_node) {
                    pcvcm_stack_push(hvml->vcm_stack, hvml->vcm_node);
                }
                hvml->vcm_node = pcvcm_node_new_string(
                           pchvml_buffer_get_buffer(hvml->temp_buffer));
                RESET_TEMP_BUFFER();
                pcutils_stack_pop(hvml->ejson_stack);
                struct pcvcm_node* node = pcvcm_stack_pop(
                        hvml->vcm_stack);
                pctree_node_append_child((struct pctree_node*)node,
                        (struct pctree_node*)hvml->vcm_node);
                SET_VCM_NODE(node);
                RECONSUME_IN(PCHVML_EJSON_JSONEE_FULL_STOP_SIGN_STATE);
            }
            PCHVML_SET_ERROR(PCHVML_ERROR_BAD_JSONEE_KEYWORD);
            RETURN_AND_STOP_PARSE();
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_JSONEE_STRING_STATE)
            uint32_t uc = pcutils_stack_top (hvml->ejson_stack);
            if (is_whitespace(character)) {
                if (uc == 'U') {
                    RECONSUME_IN(PCHVML_EJSON_AFTER_JSONEE_STRING_STATE);
                }
                APPEND_TO_TEMP_BUFFER(character);
                ADVANCE_TO(PCHVML_EJSON_JSONEE_STRING_STATE);
            }
            if (character == '$') {
                if (uc != 'U' && uc != '"') {
                    SET_VCM_NODE(pcvcm_node_new_concat_string(0, NULL));
                    pcutils_stack_push(hvml->ejson_stack, '"');
                    if (!pchvml_buffer_is_empty(hvml->temp_buffer)) {
                        struct pcvcm_node* node = pcvcm_node_new_string(
                           pchvml_buffer_get_buffer(hvml->temp_buffer));
                        pctree_node_append_child(
                                (struct pctree_node*)hvml->vcm_node,
                                (struct pctree_node*)node);
                        RESET_TEMP_BUFFER();
                        ADVANCE_TO(PCHVML_EJSON_JSONEE_STRING_STATE);
                    }
                }
                RECONSUME_IN(PCHVML_EJSON_CONTROL_STATE);
            }
            if (character == '\\') {
                SET_RETURN_STATE(curr_state);
                ADVANCE_TO(PCHVML_EJSON_STRING_ESCAPE_STATE);
            }
            if (character == '"') {
                hvml->vcm_node = pcvcm_node_new_string(
                        pchvml_buffer_get_buffer(hvml->temp_buffer));
                RESET_TEMP_BUFFER();
                RECONSUME_IN(PCHVML_EJSON_AFTER_JSONEE_STRING_STATE);
            }
            if (is_eof(character)) {
                PCHVML_SET_ERROR(PCHVML_ERROR_EOF_IN_TAG);
                RETURN_NEW_EOF_TOKEN();
            }
            if (character == ':' && uc == ':') {
                PCHVML_SET_ERROR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
                RESET_TEMP_BUFFER();
                RETURN_AND_STOP_PARSE();
            }
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(PCHVML_EJSON_JSONEE_STRING_STATE);
        END_STATE()

        BEGIN_STATE(PCHVML_EJSON_AFTER_JSONEE_STRING_STATE)
            uint32_t uc = pcutils_stack_top (hvml->ejson_stack);
            if (is_whitespace(character)) {
                struct pcvcm_node* node = pcvcm_stack_pop(hvml->vcm_stack);

                if (hvml->vcm_node) {
                    pctree_node_append_child((struct pctree_node*)node,
                            (struct pctree_node*)hvml->vcm_node);
                }
                SET_VCM_NODE(node);
                if (uc == 'U') {
                    pcutils_stack_pop(hvml->ejson_stack);
                    if (!pcutils_stack_is_empty(hvml->ejson_stack)) {
                        struct pcvcm_node* node = pcvcm_stack_pop(
                                hvml->vcm_stack);
                        pctree_node_append_child((struct pctree_node*)node,
                            (struct pctree_node*)hvml->vcm_node);
                        SET_VCM_NODE(node);
                    }
                    ADVANCE_TO(PCHVML_EJSON_CONTROL_STATE);
                }
                RECONSUME_IN(PCHVML_EJSON_JSONEE_STRING_STATE);
            }
            if (character == '"') {
                if (uc == 'U') {
                    PCHVML_SET_ERROR(PCHVML_ERROR_BAD_JSONEE_NAME);
                    RETURN_AND_STOP_PARSE();
                }
                struct pcvcm_node* node = pcvcm_stack_pop(hvml->vcm_stack);
                if (hvml->vcm_node) {
                    pctree_node_append_child((struct pctree_node*)node,
                            (struct pctree_node*)hvml->vcm_node);
                }
                SET_VCM_NODE(node);
                pcutils_stack_pop(hvml->ejson_stack);
                if (!pcutils_stack_is_empty(hvml->ejson_stack)) {
                    struct pcvcm_node* node = pcvcm_stack_pop(
                            hvml->vcm_stack);
                    pctree_node_append_child((struct pctree_node*)node,
                            (struct pctree_node*)hvml->vcm_node);
                    SET_VCM_NODE(node);
                }
                ADVANCE_TO(PCHVML_EJSON_CONTROL_STATE);
            }
            if (character == '}' || character == ']' || character == ')') {
                struct pcvcm_node* node = pcvcm_stack_pop(hvml->vcm_stack);
                if (hvml->vcm_node) {
                    pctree_node_append_child((struct pctree_node*)node,
                            (struct pctree_node*)hvml->vcm_node);
                }
                SET_VCM_NODE(node);
                pcutils_stack_pop(hvml->ejson_stack);
                if (!pcutils_stack_is_empty(hvml->ejson_stack)) {
                    struct pcvcm_node* node = pcvcm_stack_pop(
                            hvml->vcm_stack);
                    pctree_node_append_child((struct pctree_node*)node,
                            (struct pctree_node*)hvml->vcm_node);
                    SET_VCM_NODE(node);
                }
                ADVANCE_TO(PCHVML_EJSON_CONTROL_STATE);
            }
            PCHVML_SET_ERROR(PCHVML_ERROR_BAD_JSONEE_NAME);
            RETURN_AND_STOP_PARSE();
        END_STATE()

        default:
            break;
    }
    return NULL;
}

