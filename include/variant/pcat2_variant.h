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
// 头文件说明改

#ifndef PURC_VARIANT_VARIANT_H
#define PURC_VARIANT_VARIANT_H

#pragma once

struct _PURC_VARIANT;

typedef struct _PURC_VARIANT PURC_VARIANT;
typedef struct _PURC_VARIANT* purc_variant_t;

#define PURC_VARIANT_UNDEFINED          ((purc_variant_t)(-1))
#define PURC_VARIANT_NULL               ((purc_variant_t)(0))

// TODO
#define PURC_VARIANT_TRUE               ((purc_variant_t)(1))
#define PURC_VARIANT_FALSE              ((purc_variant_t)(2))


/**
 * Creates a variant data of undefined type.
 *
 * Returns: A purc_variant_t on success.
 *
 * Since: 0.0.1
 */
 // 直接调用宏，不可能错误
purc_variant_t purc_variant_make_undefined (void);


/**
 * Creates a variant data of null type.
 *
 * Returns: A purc_variant_t.
 *
 * Since: 0.0.1
 */
 // 直接调用宏，不可能错误
purc_variant_t purc_variant_make_null (void);


/**
 * Creates a variant data of boolean type.
 *
 * @param b: the initial value of created data
 *
 * Returns: A purc_variant_t.
 *
 * Since: 0.0.1
 */
 // 直接调用宏，不可能错误
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
// make 错误，返回 undefined
purc_variant_t purc_variant_make_number (double d);


/**
 * Creates a variant data of long int type.
 *
 * @param u64: the initial value of created data
 *
 * @param sign: ture for positive, or flase for nagative
 *
 * Returns: A purc_variant_t on success, NULL on failure.
 *
 * Since: 0.0.1
 */
// 分成两个，带符号，不带符号
purc_variant_t purc_variant_make_longint (uint64_t u64, bool sign);


/**
 * Creates a variant data of string type.
 *
 * @param str_utf8: the pointer of a string which is in UTF-8 encoding 
 *
 * Returns: A purc_variant_t on success, NULL on failure.
 *
 * Since: 0.0.1
 */
purc_variant_t purc_variant_make_string (const char* str_utf8);


/**
 * Checks the format of input parameter, and creates a variant data of string type.
 *
 * @param str_utf8: the pointer of a string which is in UTF-8 encoding
 *
 * Returns: A purc_variant_t on success, NULL on failure.
 *
 * Since: 0.0.1
 */
// 没有出错信息，是否废弃
// 
purc_variant_t purc_variant_make_string_with_check (const char* str_utf8);


/**
 * Gets the pointer of string which is encapsulated in string type.
 *
 * @param value: the data of string type
 *
 * Returns: The pointer of char string, or NULL on failure.
 *
 * Since: 0.0.1
 */
const char* purc_variant_get_string_const (purc_variant_t value);

/**
 * Creates a variant data of char sequence.
 *
 * @param bytes: the pointer of a char sequence
 *
 * @param nr_bytes: the number of chars in sequence
 *
 * Returns: A purc_variant_t on success, NULL on failure.
 *
 * Since: 0.0.1
 */
purc_variant_t purc_variant_make_byte_sequence (const unsigned char* bytes, size_t nr_bytes);


/**
 * Gets the pointer of char array which is encapsulated in char sequence type.
 *
 * @param value: the data of char sequence type
 *
 * @param nr_bytes: the size of char sequence
 *
 * Returns: the pointer of char array on success, or NULL on failure.
 *
 * Since: 0.0.1
 */
const unsigned char* purc_variant_get_bytes_const (purc_variant_t value, size_t* nr_bytes);


typedef purc_variant_t (*PCB_DYNAMIC_VARIANT) (purc_variant_t root, int nr_args, purc_variant_t arg0, ...);


/**
 * Creates dynamic value by setter and getter functions
 *
 * @param getter: the getter funciton pointer
 *
 * @param setter: the setter function pointer
 *
 * Returns: A purc_variant_t on success, NULL on failure.
 *
 * Since: 0.0.1
 */
purc_variant_t purc_variant_make_dynamic_value (CB_DYNAMIC_VARIANT getter, CB_DYNAMIC_VARIANT setter);


/**
 * Creates a variant data of array type.
 *
 * @param sz: the size of array
 *
 * @param value0 ..... valuen: enumerates every elements in array 
 *
 * Returns: A purc_variant_t on success, NULL on failure.
 *
 * Since: 0.0.1
 */
purc_variant_t purc_variant_make_array (size_t sz, purc_variant_t value0, ...);


/**
 * Appends a variant data to the tail of an array.
 *
 * @param array: the variant data of array type
 *
 * @param value: the element to be appended
 *
 * Returns: the size of array after appending operation.
 *
 * Since: 0.0.1
 */
// 0 成功，-1 失败
int purc_variant_array_append (purc_variant_t array, purc_variant_t value);


/**
 * Gets an element from an array by index.
 *
 * @param array: the variant data of array type
 *
 * @param idx: the index of wanted element 
 *
 * Returns: A purc_variant_t on success, or NULL on failure.
 *
 * Since: 0.0.1
 */
purc_variant_t purc_variant_array_get (purc_variant_t array, int idx);


/**
 * Sets an element value in an array by index.
 *
 * @param array: the variant data of array type
 *
 * @param idx: the index of replaced element 
 *
 * @param value: the element to replace
 *
 * Returns: The replaced purc_variant_t on success, or NULL on failure.
 *
 * Since: 0.0.1
 */
// 1、扩大，2、返回错误
// 绑定时候，ref怎么处理
int purc_variant_array_set (purc_variant_t array, int idx, purc_variant_t value);


/**
 * Remove an element from an array by index.
 *
 * @param array: the variant data of array type
 *
 * @param idx: the index of element to be removed
 *
 * Returns: The removed purc_variant_t on success, NULL on failure.
 *
 * Since: 0.0.1
 */
int purc_variant_array_remove (purc_variant_t array, int idx);


/**
 * Inserts an element to an array, places it before an indicated element.
 *
 * @param array: the variant data of array type
 *
 * @param idx: the index of element before which the new element will be placed
 *
 * @param value: the inserted element 
 *
 * Returns: The inserted purc_variant_t on success, NULL on failure.
 *
 * Since: 0.0.1
 */
int purc_variant_array_insert_before (purc_variant_t array, int idx, purc_variant_t value);


/**
 * Inserts an element to an array, places it after an indicated element.
 *
 * @param array: the variant data of array type
 *
 * @param idx: the index of element after which the new element will be placed
 *
 * @param value: the inserted element 
 *
 * Returns: The inserted purc_variant_t on success, NULL on failure.
 *
 * Since: 0.0.1
 */
int purc_variant_array_insert_after (purc_variant_t array, int idx, purc_variant_t value);


/**
 * Creates a variant data of object type.
 *
 * @param nr_kv_pairs: the minimum of key-value pairs
 *
 * @param key0 ..... keyn: the keys of key-value pairs 
 *
 * @param value0 ..... valuen: the values of key-value pairs 
 *
 * Returns: A purc_variant_t on success, NULL on failure.
 *
 * Since: 0.0.1
 */
purc_variant_t purc_variant_make_object (size_t nr_kv_pairs, const char* key0, purc_variant_t value0, ...);


/**
 * Gets the value by key from an object.
 *
 * @param obj: the variant data of obj type
 *
 * @param key: the key of key-value pair 
 *
 * Returns: A purc_variant_t on success, NULL on failure.
 *
 * Since: 0.0.1
 */
purc_variant_t purc_variant_object_get (purc_variant_t obj, const char* key);


/**
 * Sets the value by key in an object.
 *
 * @param obj: the variant data of obj type
 *
 * @param key: the key of key-value pair 
 *
 * @param value: the value of key-value pair
 *
 * Returns: True on success, False on failure.
 *
 * Since: 0.0.1
 */
bool purc_variant_object_set (purc_variant_t obj, const char* key, purc_variant_t value);


/**
 * Remove a key-value pair from an object by key.
 *
 * @param obj: the variant data of obj type
 *
 * @param key: the key of key-value pair 
 *
 * Returns: True on success, False on failure.
 *
 * Since: 0.0.1
 */
bool purc_variant_object_remove (purc_variant_t obj, const char* key);


/**
 * Creates a variant data of set type.
 *
 * @param sz: the number of elements in a set
 *
 * @param unique_key0 ..... unique_keyn: the keys of unique key-value pairs 
 *
 * @param value0 ..... valuen: the values of unique key-value pairs 
 *
 * Returns: A purc_variant_t on success, NULL on failure.
 *
 * Since: 0.0.1
 */
// key 只有在 value 是 object 时候才有效
// https://gitlab.fmsoft.cn/hvml/hvml-docs/blob/master/zh/hvml-spec-v1.0-zh.md#2127-%E9%9B%86%E5%90%88
// avl 树。没有指定 key，序列化，是value值，加入树。如果有key，则取
purc_variant_t purc_variant_make_set (size_t sz, const char* unique_key, purc_variant_t value0, ...);


/**
 * Adds a unique key-value pair to a set.
 *
 * @param set: the set to be added
 *
 * @param key: the key of key-value pair
 *
 * @param value: the value of key-value pair
 *
 * Returns: True on success, False on failure
 *
 * Since: 0.0.1
 */
bool purc_variant_set_add (purc_variant_t set, purc_variant_t value);


/**
 * Remove a unique key-value pair from a set.
 *
 * @param set: the set to be operated 
 *
 * @param key: the key of key-value pair
 *
 * Returns: True on success, False on failure
 *
 * Since: 0.0.1
 */
bool purc_variant_set_remove (purc_variant_t set, purc_variant_t value);


/**
 * Gets the value by key from a set.
 *
 * @param set: the variant data of obj type
 *
 * @param match_key: the unique key of key-value pair 
 *
 * Returns: A purc_variant_t on success, NULL on failure.
 *
 * Since: 0.0.1
 */
purc_variant_t purc_variant_get_value_in_set (const purc_variant_t set, const char * match_key);


/**
 * Adds ref for a variant data
 *
 * @param value: variant data to be operated
 *
 * Returns: A purc_variant_t on success, NULL on failure.
 *
 * Since: 0.0.1
 */
int purc_variant_ref (purc_variant_t value);

// 反引用变体型数据。引用计数减 1；当引用计数为 0 时，释放资源
/**
 * substract ref for a variant data. When ref is zero, releases the resource occupied by the data
 *
 * @param value: variant data to be operated
 *
 * Since: 0.0.1
 */
int purc_variant_unref (purc_variant_t value);


/**
 * Creates a variant data from a string which contents Json data
 *
 * @param json: the pointer of string which contents json data
 *
 * @param sz: the size of string 
 *
 * Returns: A purc_variant_t on success, NULL on failure.
 *
 * Since: 0.0.1
 */
purc_variant_t purc_variant_make_from_json_string (const char* json, size_t sz);


/**
 * Creates a variant data from Json file 
 *
 * @param file: the Json file name
 *
 * Returns: A purc_variant_t on success, NULL on failure.
 *
 * Since: 0.0.1
 */
purc_variant_t purc_variant_load_from_json_file (const char* file);


/**
 * Creates a variant data from stream 
 *
 * @param stream: the stream of purc_rwstream_t type
 *
 * Returns: A purc_variant_t on success, NULL on failure.
 *
 * Since: 0.0.1
 */
purc_variant_t purc_variant_load_from_json_stream (purc_rwstream_t stream);


/**
 * Compares two variant data 
 *
 * @param v1: one of compared variant data
 *
 * @param v2: the other variant data to be compared
 *
 * Returns: return an integer less than, equal to, or greater than zero if.
.*
 * Since: 0.0.1
 */
int purc_variant_cmp (purc_variant_t v1, purc_variant v2);


/**
 * Serialize a variant data 
 *
 * @param value: the variant data to be serialized
 *
 * @param steam: the stream to which the serialized data write
 *
 * @param opts: the serialization options
 *
 * Returns: return the size of serialized data.
.*
 * Since: 0.0.1
 */
// opts 自己看着办
size_t purc_variant_serialize (purc_variant_t value, purc_rwstream_t stream, unsigned int opts);


/**
 * Loads a variant data from an indicated library
 *
 * @param so_name: the library name
 *
 * @param var_name: the variant data name
 *
 * Returns: A purc_variant_t on success, NULL on failure.
.*
 * Since: 0.0.1
 */
purc_variant_t purc_variant_dynamic_value_load_from_so (const char* so_name, const char* var_name);

#endif /* PURC_VARIANT_VARIANT_H */

