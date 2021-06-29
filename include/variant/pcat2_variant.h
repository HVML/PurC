/*
** Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of Purring Cat 2, a HVML parser and interpreter.
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
*/

#ifndef PURC_VARIANT_VARIANT_H
#define PURC_VARIANT_VARIANT_H

#pragma once

struct _PURC_VARIANT;
typedef struct _PURC_VARIANT PURC_VARIANT;
typedef struct _PURC_VARIANT* purc_variant_t;

// 几个特殊变体数据
#define PURC_VARIANT_UNDEFINED          ((purc_variant_t)(-1))
#define PURC_VARIANT_NULL               ((purc_variant_t)(0))

// TODO
#define PURC_VARIANT_TRUE               ((purc_variant_t)(1))
#define PURC_VARIANT_FALSE              ((purc_variant_t)(2))


/**
 * Creates a variant data of undefined type.
 *
 * Returns: A purc_variant_t on success, NULL on failure.
 *
 * Since: 0.0.1
 */
purc_variant_t purc_variant_make_undefined (void);


/**
 * Creates a variant data of null type.
 *
 * Returns: A purc_variant_t on success, NULL on failure.
 *
 * Since: 0.0.1
 */
purc_variant_t purc_variant_make_null (void);


/**
 * Creates a variant data of boolean type.
 *
 * @param b: the initial value of created data
 *
 * Returns: A purc_variant_t on success, NULL on failure.
 *
 * Since: 0.0.1
 */
purc_variant_t purc_variant_make_boolean (bool b);


/**
 * Creates a variant data of number type.
 *
 * @param d: the initial value of created data
 *
 * Returns: A purc_variant_t on success, NULL on failure.
 *
 * Since: 0.0.1
 */
purc_variant_t purc_variant_make_number (double d);


/**
 * Creates a variant data of long int type.
 *
 * @param u64: the initial value of created data
 *
 * @param sign: ture for positive, and flase for nagative
 *
 * Returns: A purc_variant_t on success, NULL on failure.
 *
 * Since: 0.0.1
 */
purc_variant_t purc_variant_make_longint (uint64_t u64, bool sign);


/**
 * Creates a variant data of string type.
 *
 * @param str_utf8: the pointer of a string with UTF-8 encoding
 *
 * Returns: A purc_variant_t on success, NULL on failure.
 *
 * Since: 0.0.1
 */
purc_variant_t purc_variant_make_string (const char* str_utf8);


/**
 * Checks and Creates a variant data of string type.
 *
 * @param str_utf8: the pointer of a string with UTF-8 encoding
 *
 * Returns: A purc_variant_t on success, NULL on failure.
 *
 * Since: 0.0.1
 */
purc_variant_t purc_variant_make_string_with_check (const char* str_utf8);


// 获取字符串地址
/**
 * Gets the pointer of string which is encapsulated in string type.
 *
 * @param value: the data of string type
 *
 * Returns: A purc_variant_t on success, NULL on failure.
 *
 * Since: 0.0.1
 */
const char* purc_variant_get_string_const (purc_variant_t value);

// 构造字节序列
purc_variant_t purc_variant_make_byte_sequence (const unsigned char* bytes, size_t nr_bytes);
const unsigned char* purc_variant_get_bytes_const (purc_variant_t value, size_t* nr_bytes);

// 构造动态值
typedef purc_variant_t (*PCB_DYNAMIC_VARIANT) (purc_variant_t root, int nr_args, purc_variant_t arg0, ...);
purc_variant_t purc_variant_make_dynamic_value (CB_DYNAMIC_VARIANT getter, CB_DYNAMIC_VARIANT setter);


// https://gitlab.fmsoft.cn/hvml/docs-undisclosed/blob/master/design/purc-architecture-zh.md#4-%E6%A8%A1%E5%9D%97%E5%AE%9E%E7%8E%B0%E5%8F%8A%E5%AF%B9%E5%86%85%E6%8E%A5%E5%8F%A3

#endif /* PURC_VARIANT_VARIANT_H */

