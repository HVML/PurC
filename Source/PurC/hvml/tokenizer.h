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

#include "config.h"

#include "private/instance.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"

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

