/*
 * @file tkz-helper.h
 * @author Xue Shuming
 * @date 2022/04/22
 * @brief The helper function for ejson/hvml tokenizer.
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

#ifndef PURC_TKZ_HELPER_H
#define PURC_TKZ_HELPER_H

#include "config.h"

#include "private/instance.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"

#define TKZ_END_OF_FILE       0

PCA_EXTERN_C_BEGIN

PCA_INLINE bool
is_eof(uint32_t c)
{
    return c == TKZ_END_OF_FILE;
}

PCA_INLINE bool
is_whitespace(uint32_t c)
{
    switch (c) {
    case ' ':
    case 0x0A:
    case 0x09:
    case 0x0C:
        return true;
    }
    return false;
}

PCA_INLINE uint32_t
to_ascii_lower_unchecked(uint32_t c)
{
    return c | 0x20;
}

PCA_INLINE bool
is_ascii(uint32_t c)
{
    return !(c & ~0x7F);
}

PCA_INLINE bool
is_ascii_lower(uint32_t c)
{
    return c >= 'a' && c <= 'z';
}

PCA_INLINE bool
is_ascii_upper(uint32_t c)
{
    return c >= 'A' && c <= 'Z';
}

PCA_INLINE bool
is_ascii_space(uint32_t c)
{
    return c <= ' ' && (c == ' ' || (c <= 0xD && c >= 0x9));
}

PCA_INLINE bool
is_ascii_digit(uint32_t c)
{
    return c >= '0' && c <= '9';
}

PCA_INLINE bool
is_ascii_binary_digit(uint32_t c)
{
    return c == '0' || c == '1';
}

PCA_INLINE bool
is_ascii_hex_digit(uint32_t c)
{
    return is_ascii_digit(c) || (
            to_ascii_lower_unchecked(c) >= 'a' &&
            to_ascii_lower_unchecked(c) <= 'f'
            );
}

PCA_INLINE bool
is_ascii_upper_hex_digit(uint32_t c)
{
    return is_ascii_digit(c) || (c >= 'A' && c <= 'F');
}

PCA_INLINE bool
is_ascii_lower_hex_digit(uint32_t c)
{
    return is_ascii_digit(c) || (c >= 'a' && c <= 'f');
}

PCA_INLINE bool
is_ascii_octal_digit(uint32_t c)
{
    return c >= '0' && c <= '7';
}

PCA_INLINE bool
is_ascii_alpha(uint32_t c)
{
    return is_ascii_lower(to_ascii_lower_unchecked(c));
}

PCA_INLINE bool
is_ascii_alpha_numeric(uint32_t c)
{
    return is_ascii_digit(c) || is_ascii_alpha(c);
}

PCA_INLINE bool
is_separator(uint32_t c)
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

PCA_INLINE bool
is_delimiter(uint32_t c)
{
    switch (c) {
    case TKZ_END_OF_FILE:
    case ' ':
    case 0x0A:
    case 0x09:
    case 0x0C:
    case '{':
    case '}':
    case '[':
    case ']':
    case '(':
    case ')':
    case '<':
    case '>':
    case '$':
    case ':':
    case ';':
    case '&':
    case '|':
        return true;
    }
    return false;
}

PCA_INLINE bool
is_attribute_value_operator(uint32_t c)
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

PCA_INLINE bool
is_context_variable(uint32_t c)
{
    switch (c) {
    case '?':
    case '<':
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

#endif /* PURC_TKZ_HELPER_H */
