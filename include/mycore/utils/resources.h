/*
** Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
** Copyright (C) 2015-2017 Alexander Borisov
**
** This file is a part of PurC (short for Purring Cat), an HVML parser
** and interpreter.
** 
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
**
** Author: VincentWei <https://github.com/VincentWei
** Author: lex.borisov@gmail.com (Alexander Borisov)
*/

#ifndef MyCORE_UTILS_RESOURCES_H
#define MyCORE_UTILS_RESOURCES_H

#pragma once

#include <stddef.h>

#define MyCORE_STRING_MAP_CHAR_OTHER        '\000'
#define MyCORE_STRING_MAP_CHAR_A_Z_a_z      '\001'
#define MyCORE_STRING_MAP_CHAR_WHITESPACE   '\002'

#ifdef __cplusplus
extern "C" {
#endif

extern const unsigned char mycore_string_chars_num_map[];
extern const unsigned char mycore_string_chars_hex_map[];
extern const unsigned char mycore_string_chars_lowercase_map[];
extern const unsigned char mycore_string_chars_uppercase_map[];
extern const size_t mycore_string_replacement_character[];
extern const unsigned char mycore_string_alphanumeric_character[];
extern const unsigned char mycore_string_alpha_character[];
extern const unsigned char mycore_string_tokenizer_chars_map[];
extern const unsigned char mycore_string_hex_to_char_map[];
extern const char * mycore_string_char_to_two_hex_value[];

#ifdef __cplusplus
}
#endif

#endif /* MyCORE_UTILS_RESOURCES_H */
