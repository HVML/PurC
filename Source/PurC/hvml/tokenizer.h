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

enum tokenizer_state {
    HVML_FIRST_STATE = 0,

    HVML_DATA_STATE = HVML_FIRST_STATE,
    HVML_TAG_OPEN_STATE,
    HVML_END_TAG_OPEN_STATE,
    HVML_TAG_CONTENT_STATE,
    HVML_TAG_NAME_STATE,
    HVML_BEFORE_ATTRIBUTE_NAME_STATE,
    HVML_ATTRIBUTE_NAME_STATE,
    HVML_AFTER_ATTRIBUTE_NAME_STATE,
    HVML_BEFORE_ATTRIBUTE_VALUE_STATE,
    HVML_AFTER_ATTRIBUTE_VALUE_STATE,
    HVML_SELF_CLOSING_START_TAG_STATE,
    HVML_MARKUP_DECLARATION_OPEN_STATE,
    HVML_COMMENT_START_STATE,
    HVML_COMMENT_START_DASH_STATE,
    HVML_COMMENT_STATE,
    HVML_COMMENT_LESS_THAN_SIGN_STATE,
    HVML_COMMENT_LESS_THAN_SIGN_BANG_STATE,
    HVML_COMMENT_LESS_THAN_SIGN_BANG_DASH_STATE,
    HVML_COMMENT_LESS_THAN_SIGN_BANG_DASH_DASH_STATE,
    HVML_COMMENT_END_DASH_STATE,
    HVML_COMMENT_END_STATE,
    HVML_COMMENT_END_BANG_STATE,
    HVML_DOCTYPE_STATE,
    HVML_BEFORE_DOCTYPE_NAME_STATE,
    HVML_DOCTYPE_NAME_STATE,
    HVML_AFTER_DOCTYPE_NAME_STATE,
    HVML_AFTER_DOCTYPE_PUBLIC_KEYWORD_STATE,
    HVML_BEFORE_DOCTYPE_PUBLIC_ID_STATE,
    HVML_DOCTYPE_PUBLIC_ID_DOUBLE_QUOTED_STATE,
    HVML_DOCTYPE_PUBLIC_ID_SINGLE_QUOTED_STATE,
    HVML_AFTER_DOCTYPE_PUBLIC_ID_STATE,
    HVML_BETWEEN_DOCTYPE_PUBLIC_ID_AND_SYSTEM_INFO_STATE,
    HVML_AFTER_DOCTYPE_SYSTEM_KEYWORD_STATE,
    HVML_BEFORE_DOCTYPE_SYSTEM_STATE,
    HVML_DOCTYPE_SYSTEM_DOUBLE_QUOTED_STATE,
    HVML_DOCTYPE_SYSTEM_SINGLE_QUOTED_STATE,
    HVML_AFTER_DOCTYPE_SYSTEM_STATE,
    HVML_BOGUS_DOCTYPE_STATE,
    HVML_CDATA_SECTION_STATE,
    HVML_CDATA_SECTION_BRACKET_STATE,
    HVML_CDATA_SECTION_END_STATE,
    HVML_CHARACTER_REFERENCE_STATE,
    HVML_NAMED_CHARACTER_REFERENCE_STATE,
    HVML_AMBIGUOUS_AMPERSAND_STATE,
    HVML_NUMERIC_CHARACTER_REFERENCE_STATE,
    HVML_HEXADECIMAL_CHARACTER_REFERENCE_START_STATE,
    HVML_DECIMAL_CHARACTER_REFERENCE_START_STATE,
    HVML_HEXADECIMAL_CHARACTER_REFERENCE_STATE,
    HVML_DECIMAL_CHARACTER_REFERENCE_STATE,
    HVML_NUMERIC_CHARACTER_REFERENCE_END_STATE,
    HVML_SPECIAL_ATTRIBUTE_OPERATOR_IN_ATTRIBUTE_NAME_STATE,
    HVML_SPECIAL_ATTRIBUTE_OPERATOR_AFTER_ATTRIBUTE_NAME_STATE,
    HVML_TEXT_CONTENT_STATE,
    HVML_JSONTEXT_CONTENT_STATE,
    HVML_JSONEE_ATTRIBUTE_VALUE_DOUBLE_QUOTED_STATE,
    HVML_JSONEE_ATTRIBUTE_VALUE_SINGLE_QUOTED_STATE,
    HVML_JSONEE_ATTRIBUTE_VALUE_UNQUOTED_STATE,
    HVML_EJSON_DATA_STATE,
    HVML_EJSON_FINISHED_STATE,
    HVML_EJSON_CONTROL_STATE,
    HVML_EJSON_LEFT_BRACE_STATE,
    HVML_EJSON_RIGHT_BRACE_STATE,
    HVML_EJSON_LEFT_BRACKET_STATE,
    HVML_EJSON_RIGHT_BRACKET_STATE,
    HVML_EJSON_LEFT_PARENTHESIS_STATE,
    HVML_EJSON_RIGHT_PARENTHESIS_STATE,
    HVML_EJSON_DOLLAR_STATE,
    HVML_EJSON_AFTER_VALUE_STATE,
    HVML_EJSON_BEFORE_NAME_STATE,
    HVML_EJSON_AFTER_NAME_STATE,
    HVML_EJSON_NAME_UNQUOTED_STATE,
    HVML_EJSON_NAME_SINGLE_QUOTED_STATE,
    HVML_EJSON_NAME_DOUBLE_QUOTED_STATE,
    HVML_EJSON_VALUE_SINGLE_QUOTED_STATE,
    HVML_EJSON_VALUE_DOUBLE_QUOTED_STATE,
    HVML_EJSON_AFTER_VALUE_DOUBLE_QUOTED_STATE,
    HVML_EJSON_VALUE_TWO_DOUBLE_QUOTED_STATE,
    HVML_EJSON_VALUE_THREE_DOUBLE_QUOTED_STATE,
    HVML_EJSON_KEYWORD_STATE,
    HVML_EJSON_AFTER_KEYWORD_STATE,
    HVML_EJSON_BYTE_SEQUENCE_STATE,
    HVML_EJSON_AFTER_BYTE_SEQUENCE_STATE,
    HVML_EJSON_HEX_BYTE_SEQUENCE_STATE,
    HVML_EJSON_BINARY_BYTE_SEQUENCE_STATE,
    HVML_EJSON_BASE64_BYTE_SEQUENCE_STATE,
    HVML_EJSON_VALUE_NUMBER_STATE,
    HVML_EJSON_AFTER_VALUE_NUMBER_STATE,
    HVML_EJSON_VALUE_NUMBER_INTEGER_STATE,
    HVML_EJSON_VALUE_NUMBER_FRACTION_STATE,
    HVML_EJSON_VALUE_NUMBER_EXPONENT_STATE,
    HVML_EJSON_VALUE_NUMBER_EXPONENT_INTEGER_STATE,
    HVML_EJSON_VALUE_NUMBER_SUFFIX_INTEGER_STATE,
    HVML_EJSON_VALUE_NUMBER_INFINITY_STATE,
    HVML_EJSON_VALUE_NAN_STATE,
    HVML_EJSON_STRING_ESCAPE_STATE,
    HVML_EJSON_STRING_ESCAPE_FOUR_HEXADECIMAL_DIGITS_STATE,
    HVML_EJSON_JSONEE_VARIABLE_STATE,
    HVML_EJSON_JSONEE_FULL_STOP_SIGN_STATE,
    HVML_EJSON_JSONEE_KEYWORD_STATE,
    HVML_EJSON_JSONEE_STRING_STATE,
    HVML_EJSON_AFTER_JSONEE_STRING_STATE,
    HVML_EJSON_TEMPLATE_DATA_STATE,
    HVML_EJSON_TEMPLATE_DATA_LESS_THAN_SIGN_STATE,
    HVML_EJSON_TEMPLATE_DATA_END_TAG_OPEN_STATE,
    HVML_EJSON_TEMPLATE_DATA_END_TAG_NAME_STATE,
    HVML_EJSON_TEMPLATE_FINISHED_STATE,

    HVML_LAST_STATE = HVML_EJSON_TEMPLATE_FINISHED_STATE,
};

#define HVML_STATE_NR \
        (HVML_LAST_STATE - HVML_FIRST_STATE + 1)

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

PCA_INLINE bool is_attribute_value_operator(uint32_t c)
{
    switch (c) {
        case '+':
        case '-':
        case '*':
        case '/':
        case '%':
        case '~':
        case '^':
        case '$':
            return true;
    }
    return false;
}

PCA_INLINE bool is_context_variable(uint32_t c)
{
    switch (c) {
        case '?':
        case '^':
        case '&':
        case '@':
        case '!':
        case ':':
        case '=':
        case '%':
            return true;
    }
    return false;
}

PCA_EXTERN_C_END

#endif /* PURC_HVML_TOKENIZER_H */
