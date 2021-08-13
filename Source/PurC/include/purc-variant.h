/**
 * @file purc-variant.h
 * @author 
 * @date 2021/07/02
 * @brief The API for variant.
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
 */

#ifndef PURC_PURC_VARIANT_H
#define PURC_PURC_VARIANT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "purc-macros.h"
#include "purc-rwstream.h"

struct purc_variant;
typedef struct purc_variant purc_variant;
typedef struct purc_variant* purc_variant_t;

#define PURC_VARIANT_INVALID            ((purc_variant_t)(0))
#define PURC_VARIANT_UNDEFINED          ((purc_variant_t)(-1))
#define PURC_VARIANT_NULL               ((purc_variant_t)(-2))
#define PURC_VARIANT_TRUE               ((purc_variant_t)(1))
#define PURC_VARIANT_FALSE              ((purc_variant_t)(2))


PCA_EXTERN_C_BEGIN

/**
 * Creates a variant value of undefined type.
 *
 * Returns: A purc_variant_t with undefined type.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t purc_variant_make_undefined(void);


/**
 * Creates a variant value of null type.
 *
 * Returns: A purc_variant_t with null type.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t purc_variant_make_null(void);


/**
 * Creates a variant value of boolean type.
 *
 * @param b: the initial value of created data
 *
 * Returns: A purc_variant_t with boolean type.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t purc_variant_make_boolean(bool b);


/**
 * Creates a variant value of number type.
 *
 * @param d: the initial value of created data
 *
 * Returns: A purc_variant_t with number type,
 *      or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t purc_variant_make_number(double d);


/**
 * get the value from number type.
 *
 * @param number: the variant value of number type
 *
 * Returns: the number of variant 
 *
 * Since: 0.0.1
 */
PCA_EXPORT double purc_variant_get_number (const purc_variant_t number);


/**
 * Creates a variant value of long int type.
 *
 * @param u64: the initial value of unsigned long int type
 *
 * Returns: A purc_variant_t with long integer type,
 *      or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t purc_variant_make_ulongint(uint64_t u64);


/**
 * Creates a variant value of long int type.
 *
 * @param u64: the initial value of signed long int type
 *
 * Returns: A purc_variant_t with long int type,
 *      or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t purc_variant_make_longint(int64_t u64);


/**
 * Creates a variant value of long double type.
 *
 * @param d: the initial value of created data
 *
 * Returns: A purc_variant_t with long double type,
 *      or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t purc_variant_make_longdouble(long double lf);


/**
 * Creates a variant value of string type.
 *
 * @param str_utf8: the pointer of a string which is in UTF-8 encoding
 * @param check_encoding: whether check str_utf8 in UTF-8 encoding
 *
 * Returns: A purc_variant_t with string type,
 *      or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_make_string(const char* str_utf8, bool check_encoding);


/**
 * Gets the pointer of string which is encapsulated in string type.
 *
 * @param value: the data of string type
 *
 * Returns: The pointer of char string, or NULL if value is not string type.
 *
 * Since: 0.0.1
 */
PCA_EXPORT const char* purc_variant_get_string_const(purc_variant_t value);


/**
 * Get the number of characters in an string variant value.
 *
 * @param value: the variant value of string type
 *
 * Returns: The number of characters in an string variant value.
 *
 * Since: 0.0.1
 */
PCA_EXPORT size_t purc_variant_string_length(const purc_variant_t value);


/**
 * append a string to the tail of a string variant.
 *
 * @param string: the variant value of string type
 * @param str_utf8: the pointer of a string which is in UTF-8 encoding
 * @param check_encoding: whether check str_utf8 in UTF-8 encoding
 *
 * Returns: string variant value.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_string_append (const purc_variant_t string, 
                                const char* str_utf8, bool check_encoding);



/**
 * clear a string variant.
 *
 * @param string: the variant value of string type
 *
 * Returns: string variant value.
 *
 * Since: 0.0.1
 */
purc_variant_t purc_variant_string_clear (const purc_variant_t string);


/**
 * Whether the string variant is empty.
 *
 * @param string: the variant value of string type
 *
 * Returns: true for empty, otherwise false.
 *
 * Since: 0.0.1
 */
bool purc_variant_string_is_empty (const purc_variant_t string);


/**
 * Creates a variant value of atom string type.
 *
 * @param str_utf8: the pointer of a string which is in UTF-8 encoding
 * @param check_encoding: whether check str_utf8 in UTF-8 encoding
 *
 * Returns: A purc_variant_t with atom string type,
 *      or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_make_atom_string(const char* str_utf8, bool check_encoding);


/**
 * Creates a variant value of atom string type.
 *
 * @param str_utf8: the pointer of a string which is in UTF-8 encoding
 * @param check_encoding: whether check str_utf8 in UTF-8 encoding
 *
 * Returns: A purc_variant_t with atom string type,
 *      or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_make_atom_string_static(const char* str_utf8,
        bool check_encoding);


/**
 * Gets the pointer of string which is encapsulated in atom string type.
 *
 * @param value: the data of atom string type
 *
 * Returns: The pointer of const char string,
 *      or NULL if value is not string type.
 *
 * Since: 0.0.1
 */
PCA_EXPORT const char*
purc_variant_get_atom_string_const(purc_variant_t value);


/**
 * Creates a variant value of byte sequence type.
 *
 * @param bytes: the pointer of a byte sequence
 * @param nr_bytes: the number of bytes in sequence
 *
 * Returns: A purc_variant_t with byte sequence type,
 *      or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_make_byte_sequence(const void* bytes, size_t nr_bytes);


/**
 * Gets the pointer of byte array which is encapsulated in byte sequence type.
 *
 * @param value: the data of byte sequence type
 * @param nr_bytes: the size of byte sequence
 *
 * Returns: the pointer of byte array on success, or NULL on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT const unsigned char*
purc_variant_get_bytes_const(purc_variant_t value, size_t* nr_bytes);


/**
 * Get the number of bytes in an sequence variant value.
 *
 * @param sequence: the variant value of sequence type
 *
 * Returns: The number of bytes in an sequence variant value.
 *
 * Since: 0.0.1
 */
PCA_EXPORT size_t purc_variant_sequence_length(const purc_variant_t sequence);

typedef purc_variant_t (*purc_dvariant_method) (purc_variant_t root,
        int nr_args, purc_variant_t * argv);

/**
 * Creates dynamic value by setter and getter functions
 *
 * @param getter: the getter funciton pointer
 * @param setter: the setter function pointer
 *
 * Returns: A purc_variant_t with dynamic value,
 *      or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_make_dynamic(purc_dvariant_method getter,
        purc_dvariant_method setter);


/**
 * Get the getter function from a dynamic value
 *
 * @param dynamic: the variant value of dynamic type
 *
 * Returns: A purc_dvariant_method funciton pointer 
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_dvariant_method
purc_variant_dynamic_get_getter(const purc_variant_t dynamic);


/**
 * Get the setter function from a dynamic value
 *
 * @param dynamic: the variant value of dynamic type
 *
 * Returns: A purc_dvariant_method funciton pointer 
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_dvariant_method
purc_variant_dynamic_get_setter(const purc_variant_t dynamic);


typedef bool (*purc_navtive_releaser) (void* entity);

/**
 * Creates a variant value of native type.
 *
 * @param entity: the pointer to the native entity.
 * @param releaser: the pointer to a purc_navtive_releaser function.
 *
 * Returns: A purc_variant_t with native value,
 *      or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_make_native(void *entity, purc_navtive_releaser releaser);


/**
 * Creates a variant value of array type.
 *
 * @param sz: the size of array
 * @param value0 ..... valuen: enumerates every elements in array
 *
 * Returns: A purc_variant_t with array type,
 *      or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_make_array(size_t sz, purc_variant_t value0, ...);


/**
 * Appends a variant value to the tail of an array.
 *
 * @param array: the variant value of array type
 * @param value: the element to be appended
 *
 * Returns: True on success, otherwise False.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_array_append(purc_variant_t array, purc_variant_t value);


/**
 * Insert a variant value to the head of an array.
 *
 * @param array: the variant value of array type
 * @param value: the element to be insert
 *
 * Returns: True on success, otherwise False.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_array_prepend(purc_variant_t array, purc_variant_t value);


/**
 * Gets an element from an array by index.
 *
 * @param array: the variant value of array type
 * @param idx: the index of wanted element
 *
 * Returns: A purc_variant_t on success, or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_array_get(purc_variant_t array, int idx);


/**
 * Sets an element value in an array by index.
 *
 * @param array: the variant value of array type
 * @param idx: the index of replaced element
 * @param value: the element to replace
 *
 * Returns: True on success, otherwise False.
 *
 * Note: If idx is greater than max index of array, return -1.
 *       Whether free the replaced element, depends on its ref.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_array_set(purc_variant_t array, int idx, purc_variant_t value);


/**
 * Remove an element from an array by index.
 *
 * @param array: the variant value of array type
 * @param idx: the index of element to be removed
 *
 * Returns: True on success, otherwise False.
 *
 * Note: If idx is greater than max index of array, return -1.
 *       Whether free the removed element, depends on its ref.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool purc_variant_array_remove(purc_variant_t array, int idx);


/**
 * Inserts an element to an array, places it before an indicated element.
 *
 * @param array: the variant value of array type
 * @param idx: the index of element before which the new element will be placed
 *
 * @param value: the inserted element
 *
 * Returns: True on success, otherwise False.
 *
 * Note: If idx is greater than max index of array, return -1.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_array_insert_before(purc_variant_t array,
        int idx, purc_variant_t value);


/**
 * Inserts an element to an array, places it after an indicated element.
 *
 * @param array: the variant value of array type
 * @param idx: the index of element after which the new element will be placed
 * @param value: the inserted element
 *
 * Returns: True on success, otherwise False.
 *
 * Note: If idx is greater than sum of one plus max index of array, return -1.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_array_insert_after(purc_variant_t array,
        int idx, purc_variant_t value);


/**
 * Get the number of elements in an array variant value.
 *
 * @param array: the variant value of array type
 *
 * Returns: The number of elements in the array; -1 on failure:
 *      - the variant value is not an array.
 *
 * VWNOTE: the prototype of this function should be changed to:
 *
 *      ssize_t purc_variant_array_get_size(const purc_variant_t set);
 *
 * Since: 0.0.1
 */
PCA_EXPORT size_t purc_variant_array_get_size(const purc_variant_t array);


/**
 * Creates a variant value of object type with key as c string
 *
 * @param nr_kv_pairs: the minimum of key-value pairs
 * @param key0 ..... keyn: the keys of key-value pairs
 * @param value0 ..... valuen: the values of key-value pairs
 *
 * Returns: A purc_variant_t with object type,
 *      or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_make_object_c(size_t nr_kv_pairs,
        const char* key0, purc_variant_t value0, ...);

/**
 * Creates a variant value of object type with key as another variant
 *
 * @param nr_kv_pairs: the minimum of key-value pairs
 * @param key0 ..... keyn: the keys of key-value pairs
 * @param value0 ..... valuen: the values of key-value pairs
 *
 * Returns: A purc_variant_t with object type,
 *      or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_make_object(size_t nr_kv_pairs,
        purc_variant_t key0, purc_variant_t value0, ...);

/**
 * Gets the value by key from an object with key as c string
 *
 * @param obj: the variant value of obj type
 * @param key: the key of key-value pair
 *
 * Returns: A purc_variant_t on success, or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_object_get_c(purc_variant_t obj, const char* key);

/**
 * Gets the value by key from an object with key as another variant
 *
 * @param obj: the variant value of obj type
 * @param key: the key of key-value pair
 *
 * Returns: A purc_variant_t on success, or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
static inline purc_variant_t
purc_variant_object_get(purc_variant_t obj, purc_variant_t key)
{
    return purc_variant_object_get_c(obj, purc_variant_get_string_const(key));
}


/**
 * Sets the value by key in an object with key as c string
 *
 * @param obj: the variant value of obj type
 * @param key: the key of key-value pair
 * @param value: the value of key-value pair
 *
 * Returns: True on success, otherwise False.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_object_set_c(purc_variant_t obj, const char* key,
        purc_variant_t value);

/**
 * Sets the value by key in an object with key as another variant
 *
 * @param obj: the variant value of obj type
 * @param key: the key of key-value pair
 * @param value: the value of key-value pair
 *
 * Returns: True on success, otherwise False.
 *
 * Since: 0.0.1
 */
static inline bool
purc_variant_object_set(purc_variant_t obj,
        purc_variant_t key, purc_variant_t value)
{
    return purc_variant_object_set_c(obj,
            purc_variant_get_string_const(key), value);
}

/**
 * Remove a key-value pair from an object by key with key as c string
 *
 * @param obj: the variant value of obj type
 * @param key: the key of key-value pair
 *
 * Returns: True on success, otherwise False.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_object_remove_c(purc_variant_t obj, const char* key);

/**
 * Remove a key-value pair from an object by key with key as another variant
 *
 * @param obj: the variant value of obj type
 * @param key: the key of key-value pair
 *
 * Returns: True on success, otherwise False.
 *
 * Since: 0.0.1
 */
static inline bool
purc_variant_object_remove(purc_variant_t obj, purc_variant_t key)
{
    return purc_variant_object_remove_c(obj, purc_variant_get_string_const(key));
}

/**
 * Get the number of key-value pairs in an object variant value.
 *
 * @param obj: the variant value of object type
 *
 * Returns: The number of key-value pairs in the object; -1 on failure:
 *      - the variant value is not an object.
 *
 * VWNOTE: the prototype of this function should be changed to:
 *
 *      ssize_t purc_variant_object_get_size(const purc_variant_t set);
 *
 * Since: 0.0.1
 */
PCA_EXPORT size_t
purc_variant_object_get_size(const purc_variant_t obj);

/**
 * object iterator usage example:
 *
 * purc_variant_t obj;
 * ...
 * purc_variant_object_iterator* it = purc_variant_object_make_iterator_begin(obj);
 * while (it) {
 *     const char     *key = purc_variant_object_iterator_get_key(it);
 *     purc_variant_t  val = purc_variant_object_iterator_get_value(it);
 *     ...
 *     bool having = purc_variant_object_iterator_next(it);
 *     // behavior of accessing `val`/`key` is un-defined
 *     if (!having) {
 *         purc_variant_object_release_iterator(it);
 *         // behavior of accessing `it` is un-defined
 *         break;
 *     }
 * }
 */

struct purc_variant_object_iterator;

/**
 * Get the begin-iterator of the object,
 * which points to the head key-val-pair of the object
 *
 * @param object: the variant value of object type
 *
 * Returns: the begin-iterator of the object.
 *          NULL if no key-val-pair in the object
 *          returned iterator will inc object's ref for iterator's lifetime
 *          returned iterator shall also inc the pointed key-val-pair's ref
 *
 * Since: 0.0.1
 */
PCA_EXPORT struct purc_variant_object_iterator*
purc_variant_object_make_iterator_begin(purc_variant_t object);

/**
 * Get the end-iterator of the object,
 * which points to the tail key-val-pair of the object
 *
 * @param object: the variant value of object type
 *
 * Returns: the end-iterator of the object
 *          NULL if no key-val-pair in the object
 *          returned iterator will hold object's ref for iterator's lifetime
 *          returned iterator shall also hold the pointed key-val-pair's ref
 *
 * Since: 0.0.1
 */
PCA_EXPORT struct purc_variant_object_iterator*
purc_variant_object_make_iterator_end(purc_variant_t object);

/**
 * Release the object's iterator
 *
 * @param it: iterator of itself
 *
 * Returns: void
 *          both object's ref and the pointed key-val-pair's ref shall be dec`d
 *
 * Since: 0.0.1
 */
PCA_EXPORT void
purc_variant_object_release_iterator(struct purc_variant_object_iterator* it);

/**
 * Make the iterator point to it's successor,
 * or the next key-val-pair of the bounded object value
 *
 * @param it: iterator of itself
 *
 * Returns: True if iterator `it` has no following key-val-pair, False otherwise
 *          dec original key-val-pair's ref
 *          inc current key-val-pair's ref
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_object_iterator_next(struct purc_variant_object_iterator* it);

/**
 * Make the iterator point to it's predecessor,
 * or the previous key-val-pair of the bounded object value
 *
 * @param it: iterator of itself
 *
 * Returns: True if iterator `it` has no leading key-val-pair, False otherwise
 *          dec original key-val-pair's ref
 *          inc current key-val-pair's ref
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_object_iterator_prev(struct purc_variant_object_iterator* it);

/**
 * Get the key of key-val-pair that the iterator points to
 *
 * @param it: iterator of itself
 *
 * Returns: the key of key-val-pair, not duplicated
 *
 * Since: 0.0.1
 */
PCA_EXPORT const char *
purc_variant_object_iterator_get_key(struct purc_variant_object_iterator* it);

/**
 * Get the value of key-val-pair that the iterator points to
 *
 * @param it: iterator of itself
 *
 * Returns: the value of key-val-pair, not duplicated
 *          the returned value's ref remains unchanged
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_object_iterator_get_value(struct purc_variant_object_iterator* it);

/**
 * Creates a variant value of set type.
 *
 * @param sz: the initial number of elements in a set.
 * @param unique_key: the unique keys specified in a C string (nullable).
 *      If the unique keyis NULL, the set is a generic one.
 *
 * @param value0 ..... valuen: the values.
 *
 * Returns: A purc_variant_t on success, or PURC_VARIANT_INVALID on failure.
 *
 * Note: The key is legal, only when the value is object type.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_make_set_c(size_t sz, const char* unique_key,
        purc_variant_t value0, ...);

/**
 * Creates a variant value of set type.
 *
 * @param sz: the initial number of elements in a set.
 * @param unique_key: the unique keys specified in a variant. If the unique key
 *      is PURC_VARIANT_INVALID, the set is a generic one.
 * @param value0 ... valuen: the values will be add to the set.
 *
 * Returns: A purc_variant_t on success, or PURC_VARIANT_INVALID on failure.
 *
 * Note: The key is legal, only when the value is object type.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_make_set(size_t sz, purc_variant_t unique_key,
        purc_variant_t value0, ...);

/**
 * Adds a variant value to a set.
 *
 * @param set: the variant value of the set type.
 * @param value: the value to be added.
 *
 * Returns: @true on success, @false if:
 *      - there is already such a value in the set.
 *      - the value is not an object if the set is managed by unique keys.
 *
 * VWNOTE: We should change the prototype of this function:
 *
 * bool purc_variant_set_add(purc_variant_t obj, purc_variant_t value,
 *      bool override)
 *
 * @param override: If the set is managed by unique keys and @override is true,
 *  the function will override the old value which is equal to the new value
 *  under the unique keys, and return true. otherwise, it returns false.
 *
 * VWNOTE: If the new value has not a property (a key-value pair) under
 *  a specific unique key, the value of the key should be treated
 *  as `undefined`.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_set_add(purc_variant_t obj, purc_variant_t value, bool override);

/**
 * Remove a variant value from a set.
 *
 * @param set: the set to be operated
 * @param value: the value to be removed
 *
 * Returns: @true on success, @false if:
 *      - no any matching member in the set.
 *
 * VWNOTE: See notes below.
 *
 * Notes: This function works if the set is not managed by unique keys, or
 *  there is only one unique key. If there are multiple unique keys,
 *  use @purc_variant_set_remove_member_by_key_values() instead.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_set_remove(purc_variant_t obj, purc_variant_t value);

/**
 * Gets the member by the values of unique keys from a set.
 *
 * @param set: the variant value of the set type.
 * @param v1...vN: the values for matching. The caller should pass one value
 *      for each unique key. The number of the matching values must match
 *      the number of the unique keys.
 *
 * Returns: The memeber matched on success, or PURC_VARIANT_INVALID if:
 *      - the set does not managed by the unique keys, or
 *      - no any matching member.
 *
 * VWNOTE: new API (replacement of old purc_variant_set_get_value).
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_set_get_member_by_key_values(purc_variant_t set,
        purc_variant_t v1, ...);

/**
 * Removes the member by the values of unique keys from a set.
 *
 * @param set: the variant value of the set type. The set should be managed
 *      by unique keys.
 * @param v1...vN: the values for matching. The caller should pass one value
 *      for each unique key. The number of the matching values must match
 *      the number of the unique keys.
 *
 * Returns: @true on success, or @false if:
 *      - the set does not managed by unique keys, or
 *      - no any matching member.
 *
 * VWNOTE: new API.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_set_remove_member_by_key_values(purc_variant_t set,
        purc_variant_t v1, ...);

/**
 * Get the number of elements in a set variant value.
 *
 * @param set: the variant value of set type
 *
 * Returns: The number of elements in a set variant value; -1 on failure:
 *      - the variant value is not a set.
 *
 * VWNOTE: the prototype of this function should be changed to:
 *
 *      ssize_t purc_variant_set_get_size(const purc_variant_t set);
 *
 * Since: 0.0.1
 */
PCA_EXPORT size_t purc_variant_set_get_size(const purc_variant_t set);

/**
 * set iterator usage example:
 *
 * purc_variant_t obj;
 * ...
 * purc_variant_set_iterator* it = purc_variant_set_make_iterator_begin(obj);
 * while (it) {
 *     purc_variant_t  val = purc_variant_set_iterator_get_value(it);
 *     ...
 *     bool having = purc_variant_set_iterator_next(it);
 *     // behavior of accessing `val`/`key` is un-defined
 *     if (!having) {
 *         purc_variant_set_release_iterator(it);
 *         // behavior of accessing `it` is un-defined
 *         break;
 *     }
 * }
 */

struct purc_variant_set_iterator;

/**
 * Get the begin-iterator of the set,
 * which points to the head element of the set
 *
 * @param set: the variant value of set type
 * 
 * Returns: the begin-iterator of the set.
 *          NULL if no element in the set
 *          returned iterator will inc set's ref for iterator's lifetime
 *          returned iterator shall also inc the pointed element's ref
 *
 * Since: 0.0.1
 */
PCA_EXPORT struct purc_variant_set_iterator*
purc_variant_set_make_iterator_begin(purc_variant_t set);

/**
 * Get the end-iterator of the set,
 * which points to the head element of the set
 *
 * @param set: the variant value of set type
 *
 * Returns: the end-iterator of the set.
 *          NULL if no element in the set
 *          returned iterator will inc set's ref for iterator's lifetime
 *          returned iterator shall also inc the pointed element's ref
 *
 * Since: 0.0.1
 */
PCA_EXPORT struct purc_variant_set_iterator*
purc_variant_set_make_iterator_end(purc_variant_t set);

/**
 * Release the set's iterator
 *
 * @param it: iterator of itself
 *
 * Returns: void
 *          both set's ref and the pointed element's ref shall be dec`d
 *
 * Since: 0.0.1
 */
PCA_EXPORT void
purc_variant_set_release_iterator(struct purc_variant_set_iterator* it);

/**
 * Make the set's iterator point to it's successor,
 * or the next element of the bounded set
 *
 * @param it: iterator of itself
 *
 * Returns: True if iterator `it` has no following element, False otherwise
 *          dec original element's ref
 *          inc current element's ref
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_set_iterator_next(struct purc_variant_set_iterator* it);

/**
 * Make the set's iterator point to it's predecessor,
 * or the prev element of the bounded set
 *
 * @param it: iterator of itself
 *
 * Returns: True if iterator `it` has no leading element, False otherwise
 *          dec original element's ref
 *          inc current element's ref
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_set_iterator_prev(struct purc_variant_set_iterator* it);

/**
 * Get the value of the element that the iterator points to
 *
 * @param it: iterator of itself
 *
 * Returns: the value of the element
 *          the returned value's ref remains unchanged
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_set_iterator_get_value(struct purc_variant_set_iterator* it);

/**
 * Adds ref for a variant value
 *
 * @param value: variant value to be operated
 *
 * Returns: A purc_variant_t on success, NULL on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT unsigned int purc_variant_ref(purc_variant_t value);

/**
 * substract ref for a variant value. When ref is zero, releases the resource
 * occupied by the data
 *
 * @param value: variant value to be operated
 *
 * Note: When the reference count reaches zero, the system will release
 *      all memory used by value.
 *
 * Since: 0.0.1
 */
PCA_EXPORT unsigned int purc_variant_unref(purc_variant_t value);


/**
 * Creates a variant value from a string which contents Json data
 *
 * @param json: the pointer of string which contents json data
 *
 * @param sz: the size of string
 *
 * Returns: A purc_variant_t on success, or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t purc_variant_make_from_json_string
(const char* json, size_t sz);


/**
 * Creates a variant value from Json file
 *
 * @param file: the Json file name
 *
 * Returns: A purc_variant_t on success, or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_load_from_json_file(const char* file);


/**
 * Creates a variant value from stream
 *
 * @param stream: the stream of purc_rwstream_t type
 *
 * Returns: A purc_variant_t on success, or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_load_from_json_stream(purc_rwstream_t stream);

/**
 * Trys to cast a variant value to a long integer.
 *
 * @param v: the variant value.
 * @param i64: the buffer to receive the casted long integer if success.
 * @param parse_str: a boolean indicates whether to parse a string for
 *      casting.
 *
 * Returns: @TRUE on success, or @FALSE on failure (the variant value can not
 *      be casted to a long integer).
 *
 * Note: A null, a boolean, a number, a long integer,
 *      an unsigned long integer, or a long double can always be casted to
 *      a long integer. If 
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_cast_to_longint(purc_variant_t v, int64_t *i64, bool parse_str);

/**
 * Trys to cast a variant value to a unsigned long integer.
 *
 * @param v: the variant value.
 * @param u64: the buffer to receive the casted unsigned long integer
 *      if success.
 * @param parse_str: a boolean indicates whether to parse a string for
 *      casting.
 *
 * Returns: @TRUE on success, or @FALSE on failure (the variant value can not
 *      be casted to an unsigned long integer).
 *
 * Note: A null, a boolean, a number, a long integer,
 *      an unsigned long integer, or a long double can always be casted to
 *      an unsigned long integer.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_cast_to_ulongint(purc_variant_t v, uint64_t *u64, bool parse_str);

/**
 * Trys to cast a variant value to a nubmer.
 *
 * @param v: the variant value.
 * @param d: the buffer to receive the casted number if success.
 * @param parse_str: a boolean indicates whether to parse a string for
 *      casting.
 *
 * Returns: @TRUE on success, or @FALSE on failure (the variant value can not
 *      be casted to a number).
 *
 * Note: A null, a boolean, a number, a long integer,
 *      an unsigned long integer, or a long double can always be casted to
 *      a number (double float number).
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_cast_to_number(purc_variant_t v, double *d, bool parse_str);

/**
 * Trys to cast a variant value to a long double float number.
 *
 * @param v: the variant value.
 * @param ld: the buffer to receive the casted long double if success.
 * @param parse_str: a boolean indicates whether to parse a string for
 *      casting.
 *
 * Returns: @TRUE on success, or @FALSE on failure (the variant value can not
 *      be casted to a long double).
 *
 * Note: A null, a boolean, a number, a long integer,
 *      an unsigned long integer, or a long double can always be casted to
 *      a long double float number.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_cast_to_long_double(purc_variant_t v, long double *ld,
        bool parse_str);

/**
 * Trys to cast a variant value to a byte sequence.
 *
 * @param v: the variant value.
 * @param bytes: the buffer to receive the pointer to the byte sequence.
 * @param sz: the buffer to receive the size of the byte sequence in bytes.
 *
 * Returns: @TRUE on success, or @FALSE on failure (the variant value can not
 *      be casted to a byte sequence).
 *
 * Note: Only a string, an atom string, or a byte sequence can be casted to
 *      a byte sequence.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_cast_to_byte_sequence(purc_variant_t v,
        const void **bytes, size_t *sz);

/**
 * Compares two variant value
 *
 * @param v1: one of compared variant value
 * @param v2: the other variant value to be compared
 *
 * Returns: The function returns an integer less than, equal to, or greater
 *      than zero if v1 is found, respectively, to be less than, to match,
 *      or be greater than v2.
 *
 * Since: 0.0.1
 */
PCA_EXPORT int
purc_variant_compare(purc_variant_t v1, purc_variant_t v2);

/**
 * A flag for the purc_variant_serialize() function which causes the output
 * to have no extra whitespace or formatting applied.
 */
#define PCVARIANT_SERIALIZE_OPT_PLAIN           0x0000

/**
 * A flag for the purc_variant_serialize() function which causes the output to
 * have minimal whitespace inserted to make things slightly more readable.
 */
#define PCVARIANT_SERIALIZE_OPT_SPACED          0x0001

/**
 * A flag for the purc_variant_serialize() function which causes
 * the output to be formatted.
 *
 * See the "Two Space Tab" option at http://jsonformatter.curiousconcept.com/
 * for an example of the format.
 */
#define PCVARIANT_SERIALIZE_OPT_PRETTY          0x0002

/**
 * A flag for the purc_variant_serialize() function which causes
 * the output to drop trailing zero for float values.
 */
#define PCVARIANT_SERIALIZE_OPT_NOZERO          0x0004

/**
 * A flag for the purc_variant_serialize() function which causes
 * the output to be formatted.
 *
 * Instead of a "Two Space Tab" this gives a single tab character.
 */
#define PCVARIANT_SERIALIZE_OPT_PRETTY_TAB      0x0010

/**
 * A flag for the purc_variant_serialize() function which causes
 * the output to not escape the forward slashes.
 */
#define PCVARIANT_SERIALIZE_OPT_NOSLASHESCAPE   0x0020

#define PCVARIANT_SERIALIZE_OPT_BSEQUECE_MASK   0x0F00
/**
 * A flag for the purc_variant_serialize() function which causes
 * the output to use hexadecimal characters for byte sequence.
 */
#define PCVARIANT_SERIALIZE_OPT_BSEQUECE_HEX    0x0100

/**
 * A flag for the purc_variant_serialize() function which causes
 * the output to use binary characters for byte sequence.
 */
#define PCVARIANT_SERIALIZE_OPT_BSEQUECE_BIN    0x0200

/**
 * A flag for the purc_variant_serialize() function which causes
 * the output to use BASE64 encoding for byte sequence.
 */
#define PCVARIANT_SERIALIZE_OPT_BSEQUECE_BASE64 0x0300

/**
 * A flag for the purc_variant_serialize() function which causes
 * the output to have dot for binary sequence.
 */
#define PCVARIANT_SERIALIZE_OPT_BSEQUENCE_BIN_DOT       0x0040

/**
 * A flag for the purc_variant_serialize() function which causes
 * the function ignores the output errors.
 */
#define PCVARIANT_SERIALIZE_OPT_IGNORE_ERRORS           0x0080

/**
 * Serialize a variant value
 *
 * @param value: the variant value to be serialized.
 * @param stream: the stream to which the serialized data write.
 * @param indent_level: the initial indent level. 0 for most cases.
 * @param flags: the serialization flags.
 * @param len_expected: The buffer to receive the expected length of
 *      the serialized data (nullable). The value in the buffer should be
 *      set to 0 initially.
 *
 * Returns:
 * The size of the serialized data written to the stream;
 * On error, -1 is returned, and error code is set to indicate
 * the cause of the error.
 *
 * If the function is called with the flag
 * PCVARIANT_SERIALIZE_OPT_IGNORE_ERRORS set, this function always
 * returned the number of bytes written to the stream actually.
 * Meanwhile, if @len_expected is not null, the expected length of
 * the serialized data will be returned through this buffer.
 *
 * Therefore, you can prepare a small memory stream with the flag
 * PCVARIANT_SERIALIZE_OPT_IGNORE_ERRORS set to count the
 * expected length of the serialized data.
 *
 * Since: 0.0.1
 */
PCA_EXPORT ssize_t
purc_variant_serialize(purc_variant_t value, purc_rwstream_t stream,
        int indent_level, unsigned int flags, size_t *len_expected);


/**
 * Loads a variant value from an indicated library
 *
 * @param so_name: the library name
 *
 * @param var_name: the variant value name
 *
 * Returns: A purc_variant_t on success, or PURC_VARIANT_INVALID on failure.
.*
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_dynamic_value_load_from_so(const char* so_name,
        const char* var_name);

typedef enum purc_variant_type
{
    PURC_VARIANT_TYPE_NULL,
    PURC_VARIANT_TYPE_UNDEFINED,
    PURC_VARIANT_TYPE_BOOLEAN,
    PURC_VARIANT_TYPE_NUMBER,
    PURC_VARIANT_TYPE_LONGINT,
    PURC_VARIANT_TYPE_ULONGINT,
    PURC_VARIANT_TYPE_LONGDOUBLE,
    PURC_VARIANT_TYPE_ATOMSTRING,
    PURC_VARIANT_TYPE_STRING,
    PURC_VARIANT_TYPE_BSEQUENCE,
    PURC_VARIANT_TYPE_DYNAMIC,
    PURC_VARIANT_TYPE_NATIVE,
    PURC_VARIANT_TYPE_OBJECT,
    PURC_VARIANT_TYPE_ARRAY,
    PURC_VARIANT_TYPE_SET,
    /* critical: this MUST be the last enum */
    PURC_VARIANT_TYPE_MAX,
} purc_variant_type;

/**
 * Whether the vairant is indicated type.
 *
 * @param value: the variant value
 * @param type: wanted type
 *
 * Returns: True on success, otherwise False.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool purc_variant_is_type(const purc_variant_t value,
        enum purc_variant_type type);


/**
 * Get the type of a vairant value.
 *
 * @param value: the variant value
 *
 * Returns: The type of input variant value
 *
 * Since: 0.0.1
 */
PCA_EXPORT enum purc_variant_type
purc_variant_get_type(const purc_variant_t value);


/**
 * Whether the value is of indicated type.
 *
 * @param v: the variant value
 *
 * Returns: True if Yes, otherwise False.
 *
 * Since: 0.0.1
 */
PCA_EXPORT inline bool purc_variant_is_boolean(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_BOOLEAN);
}

PCA_EXPORT inline bool purc_variant_is_number(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_NUMBER);
}

PCA_EXPORT inline bool purc_variant_is_longint(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_LONGINT);
}

PCA_EXPORT inline bool purc_variant_is_ulongint(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_ULONGINT);
}

PCA_EXPORT inline bool purc_variant_is_longdouble(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_LONGDOUBLE);
}

PCA_EXPORT inline bool purc_variant_is_atomstring(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_ATOMSTRING);
}

PCA_EXPORT inline bool purc_variant_is_string(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_STRING);
}

PCA_EXPORT inline bool purc_variant_is_sequence(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_BSEQUENCE);
}

PCA_EXPORT inline bool purc_variant_is_dynamic(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_DYNAMIC);
}

PCA_EXPORT inline bool purc_variant_is_native(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_NATIVE);
}

PCA_EXPORT inline bool purc_variant_is_object(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_OBJECT);
}

PCA_EXPORT inline bool purc_variant_is_array(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_ARRAY);
}

PCA_EXPORT inline bool purc_variant_is_set(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_SET);
}

struct purc_variant_stat {
    size_t nr_values[PURC_VARIANT_TYPE_MAX];
    size_t sz_mem[PURC_VARIANT_TYPE_MAX];
    size_t nr_total_values;
    size_t sz_total_mem;
    size_t nr_reserved;
    size_t nr_max_reserved;
};

/**
 * Statistic of variant status.
 *
 * Returns: The pointer to struct purc_variant_stat on success, otherwise NULL.
 *
 * Since: 0.0.1
 */
PCA_EXPORT struct purc_variant_stat*  purc_variant_usage_stat(void);

PCA_EXTERN_C_END

#endif /* not defined PURC_PURC_VARIANT_H */
