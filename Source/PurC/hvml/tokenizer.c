/*
 * @file tokenizer.c
 * @author Xue Shuming
 * @date 2022/02/08
 * @brief The implementation of hvml tokenizer.
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

#include "config.h"

#include "tokenizer.h"

#include "private/instance.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/dom.h"
#include "private/hvml.h"

#include "hvml-buffer.h"
#include "hvml-rwswrap.h"
#include "hvml-token.h"
#include "hvml-sbst.h"
#include "hvml-attr.h"
#include "hvml-tag.h"

#include <math.h>

#if HAVE(GLIB)
#include <gmodule.h>
#else
#include <stdlib.h>
#endif

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


#define IS_CHAR(c)                      (character == c)
#define IS_EOF()                        is_eof(character)
#define IS_ASCII_ALPHA()                is_ascii_alpha(character)


static UNUSED_FUNCTION
bool pchvml_parser_is_operation_tag(const char* name)
{
    if (!name) {
        return NULL;
    }
    const struct pchvml_tag_entry* entry = pchvml_tag_static_search(name,
            strlen(name));
    return (entry &&
            (entry->cats & (PCHVML_TAGCAT_TEMPLATE | PCHVML_TAGCAT_VERB)));
}

static UNUSED_FUNCTION
void pchvml_parser_save_tag_name(struct pchvml_parser* parser)
{
    if (pchvml_token_is_type (parser->token, PCHVML_TOKEN_START_TAG)) {
        const char* name = pchvml_token_get_name(parser->token);
        parser->tag_is_operation = pchvml_parser_is_operation_tag(name);
        pchvml_buffer_reset(parser->tag_name);
        pchvml_buffer_append_bytes(parser->tag_name,
                name, strlen(name));
    }
    else {
        pchvml_buffer_reset(parser->tag_name);
        parser->tag_is_operation = false;
    }
}

static UNUSED_FUNCTION
bool pchvml_parser_is_appropriate_end_tag(struct pchvml_parser* parser)
{
    const char* name = pchvml_token_get_name(parser->token);
    return pchvml_buffer_equal_to (parser->tag_name, name,
            strlen(name));
}

static UNUSED_FUNCTION
bool pchvml_parser_is_appropriate_tag_name(struct pchvml_parser* parser,
        const char* name)
{
    return pchvml_buffer_equal_to (parser->tag_name, name,
            strlen(name));
}

static UNUSED_FUNCTION
bool pchvml_parser_is_operation_tag_token (struct pchvml_token* token)
{
    const char* name = pchvml_token_get_name(token);
    return pchvml_parser_is_operation_tag(name);
}

static UNUSED_FUNCTION
bool pchvml_parser_is_ordinary_attribute (struct pchvml_token_attr* attr)
{
    const char* name = pchvml_token_attr_get_name(attr);
    const struct pchvml_attr_entry* entry =pchvml_attr_static_search(name,
            strlen(name));
    return (entry && entry->type == PCHVML_ATTR_TYPE_ORDINARY);
}

static UNUSED_FUNCTION
bool pchvml_parser_is_preposition_attribute (
        struct pchvml_token_attr* attr)
{
    const char* name = pchvml_token_attr_get_name(attr);
    const struct pchvml_attr_entry* entry =pchvml_attr_static_search(name,
            strlen(name));
    return (entry && entry->type == PCHVML_ATTR_TYPE_PREP);
}

static UNUSED_FUNCTION
bool pchvml_parser_is_template_tag (const char* name)
{
    const struct pchvml_tag_entry* entry = pchvml_tag_static_search(name,
            strlen(name));
    bool ret = (entry && (entry->id == PCHVML_TAG_ARCHETYPE
                || entry->id == PCHVML_TAG_ERROR
                || entry->id == PCHVML_TAG_EXCEPT));
    return ret;
}

static UNUSED_FUNCTION
bool pchvml_parser_is_in_template (struct pchvml_parser* parser)
{
    const char* name = pchvml_buffer_get_buffer(parser->tag_name);
    return pchvml_parser_is_template_tag(name);
}

static UNUSED_FUNCTION
bool pchvml_parser_is_handle_as_jsonee(struct pchvml_token* token, uint32_t uc)
{
    if (!(uc == '[' || uc == '{' || uc == '$')) {
        return false;
    }

    struct pchvml_token_attr* attr = pchvml_token_get_curr_attr(token);
    const char* name = pchvml_token_attr_get_name(attr);
    if (pchvml_parser_is_operation_tag_token(token)
            && (strcmp(name, "on") == 0 || strcmp(name, "with") == 0)) {
        return true;
    }
    const char* token_name = pchvml_token_get_name(token);
    if (strcmp(name, "via") == 0 && (
                strcmp(token_name, "choose") == 0 ||
                strcmp(token_name, "iterate") == 0 ||
                strcmp(token_name, "reduce") == 0 ||
                strcmp(token_name, "update") == 0)) {
        return true;
    }
    return false;
}

#ifdef USE_NEW_TOKENIZER
PCHVML_NEXT_TOKEN_BEGIN


BEGIN_STATE(PCHVML_DATA_STATE)
    if (IS_CHAR('&')) {
        SET_RETURN_STATE(PCHVML_DATA_STATE);
        ADVANCE_TO(PCHVML_CHARACTER_REFERENCE_STATE);
    }
    if (IS_CHAR('<')) {
        if (parser->token) {
            RETURN_AND_SWITCH_TO(PCHVML_TAG_OPEN_STATE);
        }
        ADVANCE_TO(PCHVML_TAG_OPEN_STATE);
    }
    if (IS_EOF()) {
        RETURN_NEW_EOF_TOKEN();
    }
    RESET_TEMP_BUFFER();
    RECONSUME_IN(PCHVML_TAG_CONTENT_STATE);
END_STATE()

BEGIN_STATE(PCHVML_TAG_OPEN_STATE)
    if (IS_CHAR('!')) {
        ADVANCE_TO(PCHVML_MARKUP_DECLARATION_OPEN_STATE);
    }
    if (IS_CHAR('/')) {
        ADVANCE_TO(PCHVML_END_TAG_OPEN_STATE);
    }
    if (IS_ASCII_ALPHA()) {
        parser->token = pchvml_token_new_start_tag ();
        RECONSUME_IN(PCHVML_TAG_NAME_STATE);
    }
    if (IS_CHAR('?')) {
        SET_ERR(PCHVML_ERROR_UNEXPECTED_QUESTION_MARK_INSTEAD_OF_TAG_NAME);
        RETURN_AND_STOP_PARSE();
    }
    if (IS_EOF()) {
        SET_ERR(PCHVML_ERROR_EOF_BEFORE_TAG_NAME);
        RETURN_AND_STOP_PARSE();
    }
    SET_ERR(PCHVML_ERROR_INVALID_FIRST_CHARACTER_OF_TAG_NAME);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(PCHVML_END_TAG_OPEN_STATE)
    if (IS_ASCII_ALPHA()) {
        parser->token = pchvml_token_new_end_tag();
        RECONSUME_IN(PCHVML_TAG_NAME_STATE);
    }
    if (IS_CHAR('>')) {
        SET_ERR(PCHVML_ERROR_MISSING_END_TAG_NAME);
        RETURN_AND_STOP_PARSE();
    }
    if (IS_EOF()) {
        SET_ERR(PCHVML_ERROR_EOF_BEFORE_TAG_NAME);
        RETURN_AND_STOP_PARSE();
    }
    SET_ERR(PCHVML_ERROR_INVALID_FIRST_CHARACTER_OF_TAG_NAME);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(PCHVML_TAG_NAME_STATE)
END_STATE()

BEGIN_STATE(PCHVML_BEFORE_ATTRIBUTE_NAME_STATE)
END_STATE()

BEGIN_STATE(PCHVML_ATTRIBUTE_NAME_STATE)
END_STATE()

BEGIN_STATE(PCHVML_AFTER_ATTRIBUTE_NAME_STATE)
END_STATE()

BEGIN_STATE(PCHVML_BEFORE_ATTRIBUTE_VALUE_STATE)
END_STATE()

BEGIN_STATE(PCHVML_ATTRIBUTE_VALUE_DOUBLE_QUOTED_STATE)
END_STATE()

BEGIN_STATE(PCHVML_ATTRIBUTE_VALUE_SINGLE_QUOTED_STATE)
END_STATE()

BEGIN_STATE(PCHVML_ATTRIBUTE_VALUE_UNQUOTED_STATE)
END_STATE()

BEGIN_STATE(PCHVML_AFTER_ATTRIBUTE_VALUE_QUOTED_STATE)
END_STATE()

BEGIN_STATE(PCHVML_SELF_CLOSING_START_TAG_STATE)
END_STATE()

BEGIN_STATE(PCHVML_BOGUS_COMMENT_STATE)
END_STATE()

BEGIN_STATE(PCHVML_MARKUP_DECLARATION_OPEN_STATE)
END_STATE()

BEGIN_STATE(PCHVML_COMMENT_START_STATE)
END_STATE()

BEGIN_STATE(PCHVML_COMMENT_START_DASH_STATE)
        ADVANCE_TO(PCHVML_COMMENT_END_STATE);
END_STATE()

BEGIN_STATE(PCHVML_COMMENT_STATE)
END_STATE()

BEGIN_STATE(PCHVML_COMMENT_LESS_THAN_SIGN_STATE)
END_STATE()

BEGIN_STATE(PCHVML_COMMENT_LESS_THAN_SIGN_BANG_STATE)
END_STATE()

BEGIN_STATE(PCHVML_COMMENT_LESS_THAN_SIGN_BANG_DASH_STATE)
END_STATE()

BEGIN_STATE(PCHVML_COMMENT_LESS_THAN_SIGN_BANG_DASH_DASH_STATE)
        RECONSUME_IN(PCHVML_COMMENT_END_STATE);
    RECONSUME_IN(PCHVML_COMMENT_END_STATE);
END_STATE()

BEGIN_STATE(PCHVML_COMMENT_END_DASH_STATE)
        ADVANCE_TO(PCHVML_COMMENT_END_STATE);
END_STATE()

BEGIN_STATE(PCHVML_COMMENT_END_STATE)
        ADVANCE_TO(PCHVML_COMMENT_END_STATE);
END_STATE()

BEGIN_STATE(PCHVML_COMMENT_END_BANG_STATE)
END_STATE()

BEGIN_STATE(PCHVML_DOCTYPE_STATE)
END_STATE()

BEGIN_STATE(PCHVML_BEFORE_DOCTYPE_NAME_STATE)
END_STATE()

BEGIN_STATE(PCHVML_DOCTYPE_NAME_STATE)
END_STATE()

BEGIN_STATE(PCHVML_AFTER_DOCTYPE_NAME_STATE)
END_STATE()

BEGIN_STATE(PCHVML_AFTER_DOCTYPE_PUBLIC_KEYWORD_STATE)
END_STATE()

BEGIN_STATE(PCHVML_BEFORE_DOCTYPE_PUBLIC_ID_STATE)
END_STATE()

BEGIN_STATE(PCHVML_DOCTYPE_PUBLIC_ID_DOUBLE_QUOTED_STATE)
END_STATE()

BEGIN_STATE(PCHVML_DOCTYPE_PUBLIC_ID_SINGLE_QUOTED_STATE)
END_STATE()

BEGIN_STATE(PCHVML_AFTER_DOCTYPE_PUBLIC_ID_STATE)
END_STATE()

BEGIN_STATE(PCHVML_BETWEEN_DOCTYPE_PUBLIC_ID_AND_SYSTEM_INFO_STATE)
END_STATE()

BEGIN_STATE(PCHVML_AFTER_DOCTYPE_SYSTEM_KEYWORD_STATE)
END_STATE()

BEGIN_STATE(PCHVML_BEFORE_DOCTYPE_SYSTEM_STATE)
END_STATE()

BEGIN_STATE(PCHVML_DOCTYPE_SYSTEM_DOUBLE_QUOTED_STATE)
END_STATE()

BEGIN_STATE(PCHVML_DOCTYPE_SYSTEM_SINGLE_QUOTED_STATE)
END_STATE()

BEGIN_STATE(PCHVML_AFTER_DOCTYPE_SYSTEM_STATE)
END_STATE()

BEGIN_STATE(PCHVML_BOGUS_DOCTYPE_STATE)
END_STATE()

BEGIN_STATE(PCHVML_CDATA_SECTION_STATE)
END_STATE()

BEGIN_STATE(PCHVML_CDATA_SECTION_BRACKET_STATE)
        ADVANCE_TO(PCHVML_CDATA_SECTION_END_STATE);
END_STATE()

BEGIN_STATE(PCHVML_CDATA_SECTION_END_STATE)
        ADVANCE_TO(PCHVML_CDATA_SECTION_END_STATE);
END_STATE()

BEGIN_STATE(PCHVML_CHARACTER_REFERENCE_STATE)
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
        ADVANCE_TO(PCHVML_NUMERIC_CHARACTER_REFERENCE_END_STATE);
END_STATE()

BEGIN_STATE(PCHVML_DECIMAL_CHARACTER_REFERENCE_STATE)
        ADVANCE_TO(PCHVML_NUMERIC_CHARACTER_REFERENCE_END_STATE);
END_STATE()

BEGIN_STATE(PCHVML_NUMERIC_CHARACTER_REFERENCE_END_STATE)
END_STATE()

BEGIN_STATE(PCHVML_SPECIAL_ATTRIBUTE_OPERATOR_IN_ATTRIBUTE_NAME_STATE)
END_STATE()

BEGIN_STATE(PCHVML_SPECIAL_ATTRIBUTE_OPERATOR_AFTER_ATTRIBUTE_NAME_STATE)
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

BEGIN_STATE(PCHVML_EJSON_TEMPLATE_DATA_STATE)
END_STATE()

BEGIN_STATE(PCHVML_EJSON_TEMPLATE_DATA_LESS_THAN_SIGN_STATE)
END_STATE()

BEGIN_STATE(PCHVML_EJSON_TEMPLATE_DATA_END_TAG_OPEN_STATE)
END_STATE()

BEGIN_STATE(PCHVML_EJSON_TEMPLATE_DATA_END_TAG_NAME_STATE)
END_STATE()

BEGIN_STATE(PCHVML_EJSON_TEMPLATE_FINISHED_STATE)
END_STATE()

PCHVML_NEXT_TOKEN_END

#endif


