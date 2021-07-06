/*
** Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
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
** Author: Vincent Wei <https://github.com/VincentWei>
*/

#ifndef MyHVML_MYSTRING_H
#define MyHVML_MYSTRING_H

#pragma once

#include "myosi.h"

#include "myencoding/encoding.h"
#include "myencoding/mystring.h"

#include "mycore/mystring.h"
#include "mycore/utils/mchar_async.h"

#define myhvml_string_get(str, attr) str->attr
#define myhvml_string_set(str, attr) myhvml_string_get(str, attr)
#define myhvml_string_len(str) myhvml_string_get(str, length)

#ifdef __cplusplus
extern "C" {
#endif
    
/* append with convert encoding with preprocessing */
size_t myhvml_string_append_with_convert_encoding_with_preprocessing(mycore_string_t* str, const char* buff, size_t length, myencoding_t encoding, bool emit_null_chars);
size_t myhvml_string_append_chunk_with_convert_encoding_with_preprocessing(mycore_string_t* str, myencoding_result_t* res, const char* buff, size_t length, myencoding_t encoding, bool emit_null_chars);

/* append with convert encoding lowercase with preprocessing */
size_t myhvml_string_append_lowercase_with_convert_encoding_with_preprocessing(mycore_string_t* str, const char* buff, size_t length, myencoding_t encoding, bool emit_null_chars);
size_t myhvml_string_append_lowercase_chunk_with_convert_encoding_with_preprocessing(mycore_string_t* str, myencoding_result_t* res, const char* buff, size_t length, myencoding_t encoding, bool emit_null_chars);

/* append with preprocessing */
size_t myhvml_string_before_append_any_preprocessing(mycore_string_t* str, const char* buff, size_t length, size_t last_position);
size_t myhvml_string_append_with_preprocessing(mycore_string_t* str, const char* buff, size_t length, bool emit_null_chars);
size_t myhvml_string_append_lowercase_with_preprocessing(mycore_string_t* str, const char* buff, size_t length, bool emit_null_chars);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MyHVML_MYSTRING_H */
