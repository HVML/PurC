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

#include "purc-macros.h"


struct purc_variant;
typedef struct purc_variant purc_variant;
typedef struct purc_variant* purc_variant_t;

#define PURC_VARIANT_INVALID            ((purc_variant_t)(0))

enum pcvariant_error
{
    PCVARIANT_SUCCESS = PURC_ERROR_OK,
    PCVARIANT_BAD_ENCODING = PURC_ERROR_FIRST_VARIANT,
};

PCA_EXTERN_C_BEGIN

/**
 * Creates a variant value of undefined type.
 *
 * Returns: A purc_variant_t with undefined type.
 *
 * Since: 0.0.1
 */
purc_variant_t purc_variant_make_undefined (void);


/**
 * Creates a variant value of null type.
 *
 * Returns: A purc_variant_t with null type.
 *
 * Since: 0.0.1
 */
purc_variant_t purc_variant_make_null (void);


/**
 * Creates a variant value of boolean type.
 *
 * @param b: the initial value of created data
 *
 * Returns: A purc_variant_t with boolean type.
 *
 * Since: 0.0.1
 */
purc_variant_t purc_variant_make_boolean (bool b);


/**
 * Creates a variant value of number type.
 *
 * @param d: the initial value of created data
 *
 * Returns: A purc_variant_t with number type, or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
purc_variant_t purc_variant_make_number (double d);


/**
 * Creates a variant value of long int type.
 *
 * @param u64: the initial value of unsigned long int type
 *
 * Returns: A purc_variant_t with long int type, or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
purc_variant_t purc_variant_make_longuint (uint64_t u64);


/**
 * Creates a variant value of long int type.
 *
 * @param u64: the initial value of signed long int type
 *
 * Returns: A purc_variant_t with long int type, or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
purc_variant_t purc_variant_make_longint (int64_t u64);


/**
 * Creates a variant value of long double type.
 *
 * @param d: the initial value of created data
 *
 * Returns: A purc_variant_t with long double type, or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
purc_variant_t purc_variant_make_longdouble (long double lf);


/**
 * Creates a variant value of string type.
 *
 * @param str_utf8: the pointer of a string which is in UTF-8 encoding
 *
 * Returns: A purc_variant_t with string type, or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
purc_variant_t purc_variant_make_string (const char* str_utf8);


/**
 * Checks the encoding format of input parameter, and creates a variant
 * value of string type.
 *
 * @param str_utf8: the pointer of a string which is in UTF-8 encoding
 *
 * Returns: A purc_variant_t with string type, or PURC_VARIANT_INVALID on failure.
 *
 * Note: If str_utf8 is not in UTF-8 encoding, return PURC_VARIANT_INVALID.
 *
 * Since: 0.0.1
 */
purc_variant_t purc_variant_make_string_with_check (const char* str_utf8);


/**
 * Gets the pointer of string which is encapsulated in string type.
 *
 * @param value: the data of string type
 *
 * Returns: The pointer of char string, or NULL if value is not string type.
 *
 * Since: 0.0.1
 */
const char* purc_variant_get_string_const (purc_variant_t value);


/**
 * Get the number of characters in an string variant value.
 *
 * @param value: the variant value of string type
 *
 * Returns: The number of characters in an string variant value.
 *
 * Since: 0.0.1
 */
size_t purc_variant_string_length(const purc_variant_t value);


/**
 * Creates a variant value of byte sequence type.
 *
 * @param bytes: the pointer of a byte sequence
 * @param nr_bytes: the number of bytes in sequence
 *
 * Returns: A purc_variant_t with byte sequence type, or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
purc_variant_t purc_variant_make_byte_sequence (const unsigned char* bytes, size_t nr_bytes);


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
const unsigned char* purc_variant_get_bytes_const (purc_variant_t value, size_t* nr_bytes);


/**
 * Get the number of bytes in an sequence variant value.
 *
 * @param sequence: the variant value of sequence type
 *
 * Returns: The number of bytes in an sequence variant value.
 *
 * Since: 0.0.1
 */
size_t purc_variant_sequence_length(const purc_variant_t sequence);


typedef purc_variant_t (*PCB_DYNAMIC_VARIANT) (purc_variant_t root, int nr_args, purc_variant_t arg0, ...);


/**
 * Creates dynamic value by setter and getter functions
 *
 * @param getter: the getter funciton pointer
 * @param setter: the setter function pointer
 *
 * Returns: A purc_variant_t with dynamic value, or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
purc_variant_t purc_variant_make_dynamic_value (CB_DYNAMIC_VARIANT getter, CB_DYNAMIC_VARIANT setter);


typedef bool (*purc_nvariant_releaser) (void* native_obj);


/**
 * Creates a variant value of native type.
 *
 * @param native_obj: the pointer of native ojbect
 * @param releaser: the purc_nvariant_releaser function pointer
 *
 * Returns: A purc_variant_t with native value, or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
purc_variant_t purc_variant_make_native (void *native_obj, purc_nvariant_releaser releaser);


/**
 * Creates a variant value of array type.
 *
 * @param sz: the size of array
 * @param value0 ..... valuen: enumerates every elements in array
 *
 * Returns: A purc_variant_t with array type, or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
purc_variant_t purc_variant_make_array (size_t sz, purc_variant_t value0, ...);


/**
 * Appends a variant data to the tail of an array.
 *
 * @param array: the variant data of array type
 * @param value: the element to be appended
 *
 * Returns: True on success, otherwise False.
 *
 * Since: 0.0.1
 */
bool purc_variant_array_append (purc_variant_t array, purc_variant_t value);


/**
 * Insert a variant data to the head of an array.
 *
 * @param array: the variant data of array type
 * @param value: the element to be insert
 *
 * Returns: True on success, otherwise False.
 *
 * Since: 0.0.1
 */
bool purc_variant_array_prepend (purc_variant_t array, purc_variant_t value);


/**
 * Gets an element from an array by index.
 *
 * @param array: the variant data of array type
 * @param idx: the index of wanted element
 *
 * Returns: A purc_variant_t on success, or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
purc_variant_t purc_variant_array_get (purc_variant_t array, int idx);


/**
 * Sets an element value in an array by index.
 *
 * @param array: the variant data of array type
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
bool purc_variant_array_set (purc_variant_t array, int idx, purc_variant_t value);


/**
 * Remove an element from an array by index.
 *
 * @param array: the variant data of array type
 * @param idx: the index of element to be removed
 *
 * Returns: True on success, otherwise False.
 *
 * Note: If idx is greater than max index of array, return -1.
 *       Whether free the removed element, depends on its ref.
 *
 * Since: 0.0.1
 */
bool purc_variant_array_remove (purc_variant_t array, int idx);


/**
 * Inserts an element to an array, places it before an indicated element.
 *
 * @param array: the variant data of array type
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
bool purc_variant_array_insert_before (purc_variant_t array, int idx, purc_variant_t value);


/**
 * Inserts an element to an array, places it after an indicated element.
 *
 * @param array: the variant data of array type
 * @param idx: the index of element after which the new element will be placed
 * @param value: the inserted element
 *
 * Returns: True on success, otherwise False.
 *
 * Note: If idx is greater than sum of one plus max index of array, return -1.
 *
 * Since: 0.0.1
 */
bool purc_variant_array_insert_after (purc_variant_t array, int idx, purc_variant_t value);


/**
 * Get the number of elements in an array variant value.
 *
 * @param array: the variant value of array type
 *
 * Returns: The number of elements in an array variant value.
 *
 * Since: 0.0.1
 */
size_t purc_variant_array_get_size(const purc_variant_t array);


/**
 * Creates a variant value of object type.
 *
 * @param nr_kv_pairs: the minimum of key-value pairs
 * @param key0 ..... keyn: the keys of key-value pairs
 * @param value0 ..... valuen: the values of key-value pairs
 *
 * Returns: A purc_variant_t with object type, or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
purc_variant_t purc_variant_make_object (size_t nr_kv_pairs, const char* key0, purc_variant_t value0, ...);


/**
 * Gets the value by key from an object.
 *
 * @param obj: the variant data of obj type
 * @param key: the key of key-value pair
 *
 * Returns: A purc_variant_t on success, or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
purc_variant_t purc_variant_object_get (purc_variant_t obj, const char* key);


/**
 * Sets the value by key in an object.
 *
 * @param obj: the variant data of obj type
 * @param key: the key of key-value pair
 * @param value: the value of key-value pair
 *
 * Returns: True on success, otherwise False.
 *
 * Since: 0.0.1
 */
bool purc_variant_object_set (purc_variant_t obj, const char* key, purc_variant_t value);


/**
 * Remove a key-value pair from an object by key.
 *
 * @param obj: the variant data of obj type
 * @param key: the key of key-value pair
 *
 * Returns: True on success, otherwise False.
 *
 * Since: 0.0.1
 */
bool purc_variant_object_remove (purc_variant_t obj, const char* key);


/**
 * Get the number of key-value pairs in an object variant value.
 *
 * @param obj: the variant value of object type
 *
 * Returns: The number of key-value pairs in an object variant value.
 *
 * Since: 0.0.1
 */
size_t purc_variant_object_get_size (const purc_variant_t obj);




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
struct purc_variant_object_iterator* purc_variant_object_make_iterator_begin (purc_variant_t object);

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
struct purc_variant_object_iterator* purc_variant_object_make_iterator_end (purc_variant_t object);

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
void purc_variant_object_release_iterator (struct purc_variant_object_iterator* it);

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
bool purc_variant_object_iterator_next (struct purc_variant_object_iterator* it);

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
bool purc_variant_object_iterator_prev (struct purc_variant_object_iterator* it);

/**
 * Get the key of key-val-pair that the iterator points to
 *
 * @param it: iterator of itself
 *
 * Returns: the key of key-val-pair, not duplicated
 *
 * Since: 0.0.1
 */
const char *purc_variant_object_iterator_get_key (struct purc_variant_object_iterator* it);

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
purc_variant_t purc_variant_object_iterator_get_value (struct purc_variant_object_iterator* it);

/**
 * Creates a variant data of set type.
 *
 * @param sz: the number of elements in a set
 * @param unique_key0 ..... unique_keyn: the keys of unique value
 * @param value0 ..... valuen: the values related to the key.
 *
 * Returns: A purc_variant_t on success, or PURC_VARIANT_INVALID on failure.
 *
 * Note: The key is legal, only when the value is object type.
 *
 * Since: 0.0.1
 */
purc_variant_t purc_variant_make_set (size_t sz, const char* unique_key, purc_variant_t value0, ...);


/**
 * Adds a unique key-value pair to a set.
 *
 * @param set: the set to be added
 * @param value: the value to be added
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
 * @param value: the value to be removed
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
 * @param match_key: the unique key related to the value
 *
 * Returns: A purc_variant_t on success, or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
purc_variant_t purc_variant_set_get_value (const purc_variant_t set, const char * match_key);


/**
 * Get the number of elements in a set variant value.
 *
 * @param set: the variant value of set type
 *
 * Returns: The number of elements in a set variant value.
 *
 * Since: 0.0.1
 */
size_t purc_variant_set_get_size(const purc_variant_t set);



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
struct purc_variant_set_iterator* purc_variant_set_make_iterator_begin (purc_variant_t set);

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
struct purc_variant_set_iterator* purc_variant_set_make_iterator_end (purc_variant_t set);

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
void purc_variant_set_release_iterator (struct purc_variant_set_iterator* it);

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
bool purc_variant_set_iterator_next (struct purc_variant_set_iterator* it);

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
bool purc_variant_set_iterator_prev (struct purc_variant_set_iterator* it);

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
purc_variant_t purc_variant_set_iterator_get_value (struct purc_variant_set_iterator* it);



/**
 * Adds ref for a variant data
 *
 * @param value: variant data to be operated
 *
 * Returns: A purc_variant_t on success, NULL on failure.
 *
 * Since: 0.0.1
 */
unsigned int purc_variant_ref (purc_variant_t value);


/**
 * substract ref for a variant data. When ref is zero, releases the resource occupied by the data
 *
 * @param value: variant data to be operated
 *
 * Note: When the ref is zero, the system will release all resource ocupied by value.
 *
 * Since: 0.0.1
 */
unsigned int purc_variant_unref (purc_variant_t value);


/**
 * Creates a variant data from a string which contents Json data
 *
 * @param json: the pointer of string which contents json data
 *
 * @param sz: the size of string
 *
 * Returns: A purc_variant_t on success, or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
purc_variant_t purc_variant_make_from_json_string (const char* json, size_t sz);


/**
 * Creates a variant data from Json file
 *
 * @param file: the Json file name
 *
 * Returns: A purc_variant_t on success, or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
purc_variant_t purc_variant_load_from_json_file (const char* file);


/**
 * Creates a variant data from stream
 *
 * @param stream: the stream of purc_rwstream_t type
 *
 * Returns: A purc_variant_t on success, or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
purc_variant_t purc_variant_load_from_json_stream (purc_rwstream_t stream);


/**
 * Compares two variant data
 *
 * @param v1: one of compared variant data
 * @param v2: the other variant data to be compared
 *
 * Returns: return zero for identical, otherwise -1.
.*
 * Since: 0.0.1
 */
int purc_variant_compare (purc_variant_t v1, purc_variant v2);


/**
 * Serialize a variant data
 *
 * @param value: the variant data to be serialized
 *
 * @param steam: the stream to which the serialized data write
 * @param opts: the serialization options       // To be defined
 *
 * Returns: return the size of serialized data.
.*
 * Since: 0.0.1
 */
size_t purc_variant_serialize (purc_variant_t value, purc_rwstream_t stream, unsigned int opts);


/**
 * Loads a variant data from an indicated library
 *
 * @param so_name: the library name
 *
 * @param var_name: the variant data name
 *
 * Returns: A purc_variant_t on success, or PURC_VARIANT_INVALID on failure.
.*
 * Since: 0.0.1
 */
purc_variant_t purc_variant_dynamic_value_load_from_so (const char* so_name, const char* var_name);



typedef enum variant_type
{
    variant_type_null,
    variant_type_undefined,
    variant_type_boolean,
    variant_type_number,
    variant_type_longint,
    variant_type_longdouble,
    variant_type_string,
    variant_type_sequence,
    variant_type_dynamic,
    variant_type_native,
    variant_type_object,
    variant_type_array,
    variant_type_set,
} variant_type;


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
bool purc_variant_is_type(const purc_variant_t value, enum variant_type type);


/**
 * Get the type of a vairant value.
 *
 * @param value: the variant value
 *
 * Returns: The type of input variant value
 *
 * Since: 0.0.1
 */
enum variant_type purc_variant_get_type(const purc_variant_t value);


struct purc_variant_stat {
    // to be defined
};


/**
 * Statistic of variant status.
 *
 * @param stat: the pointer of purc_variant_stat
 *
 * Returns: True on success, otherwise False.
 *
 * Since: 0.0.1
 */
bool purc_variant_usage_stat (struct purc_variant_stat* stat);


#define foreach_value_in_variant_array(array, value)                \
    do {                                                            \
        purc_variant_object_iterator *__oite = NULL;                \
        purc_variant_set_iterator *__site    = NULL;                \
        int array_size = purc_variant_array_get_size (array)        \
        for (int i = 0; i < array_size,                             \
                        value = purc_variant_array_get (array, i);  \
             i++) {                                                 \
     /* } */                                                        \
 /* } while (0) */

#define foreach_value_in_variant_object(obj, value)                               \
    do {                                                                          \
        purc_variant_object_iterator *__oite = NULL;                              \
        purc_variant_set_iterator *__site    = NULL;                              \
        bool __having = true;                                                     \
        for (__oite = purc_variant_object_make_iterator_begin(obj);               \
             __oite && __having;                                                  \
             __having = purc_variant_object_iterator_next(__oite) )               \
        {                                                                         \
            value = purc_variant_object_iterator_get_value(__oite);               \
     /* } */                                                                      \
 /* } while (0) */
        

#define foreach_key_value_in_variant_object(obj, key, value)                      \
    do {                                                                          \
        purc_variant_object_iterator *__oite = NULL;                              \
        purc_variant_set_iterator *__site    = NULL;                              \
        bool __having = true;                                                     \
        for (__oite = purc_variant_object_make_iterator_begin(obj);               \
             __oite && __having;                                                  \
             __having = purc_variant_object_iterator_next(__oite) )               \
        {                                                                         \
            key   = purc_variant_object_iterator_get_key(__oite);                 \
            value = purc_variant_object_iterator_get_value(__oite);               \
     /* } */                                                                      \
 /* } while (0) */
        

#define foreach_value_in_variant_set(set, value)                                  \
    do {                                                                          \
        purc_variant_object_iterator *__oite = NULL;                              \
        purc_variant_set_iterator *__site    = NULL;                              \
        bool __having = true;                                                     \
        for (__site = purc_variant_object_make_iterator_begin(obj);               \
             __site && __having;                                                  \
             __having = purc_variant_object_iterator_next(__site) )               \
        {                                                                         \
            value = purc_variant_object_iterator_get_value(__site);               \
     /* } */                                                                      \
  /* } while (0) */


#define end_foreach                                                     \
 /* do { */                                                             \
     /* for (...) { */                                                  \
        }                                                               \
        if (__oite) purc_variant_object_release_iterator(__oite);       \
        if (__site) purc_variant_set_release_iterator(__site);          \
    } while (0)

PCA_EXTERN_C_END


#endif /* not defined PURC_PURC_VARIANT_H */
