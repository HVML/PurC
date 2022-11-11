/**
 * @file purc-variant.h
 * @author Vincent Wei
 * @date 2021/07/02
 * @brief The API for variant.
 *
 * Copyright (C) 2021, 2022 FMSoft <https://www.fmsoft.cn>
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
#include "purc-utils.h"

struct purc_variant;
typedef struct purc_variant purc_variant;
typedef struct purc_variant* purc_variant_t;

#define PURC_VARIANT_INVALID            ((purc_variant_t)(0))

#define PURC_VARIANT_BADSIZE            ((ssize_t)(-1))

PCA_EXTERN_C_BEGIN

/**
 * Gets the size of a variant wrapper.
 *
 * Returns: The size of a variant wrapper.
 *
 * Since: 0.1.1
 */
PCA_EXPORT size_t
purc_variant_wrapper_size(void);

/**
 * Gets the reference count of a variant.
 *
 * @param value: The variant value.
 *
 * Returns: The reference count of the variant.
 *
 * Since: 0.1.0
 */
PCA_EXPORT unsigned int
purc_variant_ref_count(purc_variant_t value);

/**
 * Increments the reference count of a variant by one.
 *
 * @param value: The variant value.
 *
 * Returns: The passed in variant value on success,
 *  %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_ref(purc_variant_t value);

/**
 * Decrements the reference count of a variant by one.
 * If the reference count drops to 0, the variant will be released.
 *
 * @param value: The variant value to un-reference.
 *
 * Since: 0.0.1
 */
PCA_EXPORT unsigned int
purc_variant_unref(purc_variant_t value);

/**
 * Creates a variant which represents an undefined value.
 *
 * Returns: A variant having value of `undefined`,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_make_undefined(void);

/**
 * Creates a variant which represents an exception.
 *
 * @param except_atom: The atom value of the exception.
 *
 * Returns: A variant having value of the specified exception atom,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.2
 */
PCA_EXPORT purc_variant_t
purc_variant_make_exception(purc_atom_t except_atom);

/**
 * Gets the exception name of an exception variant.
 *
 * @param value: An exception variant created by purc_variant_make_exception().
 *
 * Returns: The pointer to the exception name which is a null-terminated string,
 *      or %NULL if the variant is not an exception.
 *
 * Since: 0.0.2
 */
PCA_EXPORT const char *
purc_variant_get_exception_string_const(purc_variant_t value);

/**
 * Creates a variant which represents a null value.
 *
 * Returns: A variant having value of `null`.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_make_null(void);

/**
 * Creates a variant which represents a boolean value.
 *
 * @param b: A #bool value.
 *
 * Returns: A variant having the specified boolean value,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_make_boolean(bool b);

/**
 * Creates a variant which represents a number value.
 *
 * @param d: A double value which specifying the number.
 *
 * Returns: A variant having the specified number value,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_make_number(double d);

/**
 * Creates a variant which represents an unsigned long integer value.
 *
 * @param u64: A uint64_t value which specifying the unsigned long integer.
 *
 * Returns: A variant having the specified unsigned long integer value,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_make_ulongint(uint64_t u64);

/**
 * Creates a variant which represents a long integer value.
 *
 * @param u64: A int64_t value which specifying the long integer.
 *
 * Returns: A variant having the specified long integer value,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_make_longint(int64_t i64);

/**
 * Creates a variant which represents a high precision float number.
 *
 * @param lf: A long double value which specifying
 *      the high precision float number.
 *
 * Returns: A variant having the specified high precision float number,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_make_longdouble(long double lf);

/**
 * Creates a variant which represents a null-terminated string
 * in UTF-8 encoding. Note that the new variant will hold a copy of
 * the specified string if success.
 *
 * @param str_utf8: The pointer to a null-terminated string
 *      which is encoded in UTF-8.
 * @param check_encoding: Whether to check the encoding.
 *
 * Returns: A variant contains the specified text string in UTF-8 encoding,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_make_string(const char* str_utf8, bool check_encoding);

/**
 * Creates a variant which represents a null-terminted static string
 * in UTF-8 encoding. Note that the new variant will only hold the pointer
 * to the static string, not a copy of the string.
 *
 * @param str_utf8: The pointer to a null-terminated string
 *      which is in UTF-8 encoding.
 * @param check_encoding: Whether to check the encoding.
 *
 * Returns: A variant contains the pointer to the static string,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.2
 */
PCA_EXPORT purc_variant_t
purc_variant_make_string_static(const char* str_utf8, bool check_encoding);

/**
 * Creates a variant which represents a null-terminated static string in
 * UTF-8 encoding. Note that the new variant will take the ownership of
 * the buffer which containing the string. The buffer will be released by
 * calling free() when the variant is destroyed ultimately.
 *
 * @param str_utf8: The pointer to a null-terminated string which is encoded
 *      in UTF-8.
 * @param sz_buff: The size of the buffer (not the length of the string).
 * @param check_encoding: Whether to check the encoding.
 *
 * Returns: A variant which contains the specified string in UTF-8 encoding
 *      and takes the ownership of the string buffer,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.2
 */
PCA_EXPORT purc_variant_t
purc_variant_make_string_reuse_buff(char* str_utf8, size_t sz_buff,
        bool check_encoding);

/**
 * Creates a variant which represents a null-terminated string in UTF-8
 * encoding by copying a string, but at most the specified length copied.
 *
 * @param str_utf8: The pointer to a string which is encoded in UTF-8.
 * @param len: The length of string (in bytes) to be copied at most.
 * @param check_encoding: Whether to check the encoding.
 *
 * Returns: A variant which copies the string of specified length in
 *  UTF-8 encoding, or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.5
 */
PCA_EXPORT purc_variant_t
purc_variant_make_string_ex(const char* str_utf8, size_t len,
        bool check_encoding);

/**
 * Gets the pointer of the string contained in the specified variant
 * if the variant represents a string, an atom, or an exception variant.
 *
 * Note that you should not change the contents in the buffer
 * pointed to by the return value.
 *
 * @param value: A string, an atom, or an exception variant.
 * @param length (nullable): The pointer to a size_t buffer to receive
 *      the length of the string (not including the terminating null byte).
 *
 * Returns: The pointer to the string in UTF-8 encoding, or %NULL if
 *      the variant does not respresent a string, an atom, or an exception.
 *
 * Since: 0.1.0
 */
PCA_EXPORT const char*
purc_variant_get_string_const_ex(purc_variant_t value, size_t *length);


/**
 * Gets the pointer of the string contained in the specified variant
 * if the variant represents a string, an atom, or an exception variant.
 *
 * Note that you should not change the contents in the buffer
 * pointed to by the return value.
 *
 * @param value: A string, an atom, or an exception variant.
 *
 * Returns: The pointer to the string in UTF-8 encoding, or %NULL if
 *      the variant does not respresent a string, an atom, or an exception.
 *
 * Since: 0.0.1
 */
static inline const char*
purc_variant_get_string_const(purc_variant_t value)
{
    return purc_variant_get_string_const_ex(value, NULL);
}

/**
 * Gets the length of the string contained in the specified variant
 * if the variant represents a string, an atom, or an exception variant.
 *
 * @param value: A string, an atom, or an exception variant.
 * @param length (nullable): The pointer to a size_t buffer to receive
 *      the length of the string (not including the terminating null byte).
 *
 * Returns: %true on success, otherwise %false.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_string_bytes(purc_variant_t value, size_t *length);

/**
 * Gets the length of the string contained in the specified variant
 * if the variant represents a string, an atom, or an exception variant.
 *
 * @param value: A string, an atom, or an exception variant.
 *
 * Returns: The length in bytes (not including the terminating null byte)
 *  of the string on success; %PURC_VARIANT_BADSIZE (-1) if the variant
 *  is not a string, an atom, or an exception variant.
 *
 * Since: 0.0.1
 */
static inline ssize_t
purc_variant_string_size(purc_variant_t value)
{
    size_t len;
    if (!purc_variant_string_bytes(value, &len))
        return PURC_VARIANT_BADSIZE;
    return len;
}

/**
 * Get the number of valid characters in a string variant value.
 *
 * @param value: the variant value in string, atomstring, or exception type.
 * @param nr_chars: the buffer to receive the number of valid characters
 *      in the string.
 *
 * Returns: @true on success, and @false when the value is not a string.
 *
 * Since: 0.1.0
 */
PCA_EXPORT bool
purc_variant_string_chars(purc_variant_t value, size_t *nr_chars);

/**
 * Creates a variant value of atom string type.
 *
 * @param str_utf8: the pointer of a string which is in UTF-8 encoding
 * @param check_encoding: whether check str_utf8 in UTF-8 encoding
 *
 * Returns: A purc_variant_t with atom string type,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_make_atom_string(const char* str_utf8,
        bool check_encoding);

/**
 * Creates a variant value of atom string type.
 *
 * @param str_utf8: the pointer of a string which is in UTF-8 encoding
 * @param check_encoding: whether check str_utf8 in UTF-8 encoding
 *
 * Returns: A purc_variant_t with atom string type,
 *      or %PURC_VARIANT_INVALID on failure.
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
 * Creates a variant value of byte sequence type by coping the content.
 *
 * @param bytes: the pointer of a byte sequence.
 * @param nr_bytes: the number of bytes in sequence.
 *
 * Returns: A purc_variant_t with byte sequence type,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_make_byte_sequence(const void* bytes, size_t nr_bytes);

/**
 * Creates a variant value of byte sequence type from a static C byte array.
 *
 * @param bytes: the pointer of a byte sequence.
 * @param nr_bytes: the number of bytes in sequence.
 *
 * Returns: A purc_variant_t with string type,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.2
 */
PCA_EXPORT purc_variant_t
purc_variant_make_byte_sequence_static(const void* bytes, size_t nr_bytes);

/**
 * Creates a variant value of byte sequence type by reusing the bytes buffer.
 * The buffer will be released by calling free() when the variant is destroyed.
 *
 * @param bytes: the pointer of a byte sequence.
 * @param nr_bytes: the number of bytes in sequence.
 * @param sz_buff: the size of the bytes buffer.
 *
 * Returns: A purc_variant_t with string type,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.2
 */
PCA_EXPORT purc_variant_t
purc_variant_make_byte_sequence_reuse_buff(void* bytes, size_t nr_bytes,
        size_t sz_buff);

/**
 * Creates an empty byte sequence variant.
 *
 * Returns: A purc_variant_t with byte sequence type,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.2
 */
PCA_EXPORT purc_variant_t
purc_variant_make_byte_sequence_empty(void);

/**
 * Gets the pointer of byte array which is encapsulated in byte sequence type.
 *
 * @param value: the data of byte sequence type
 * @param nr_bytes: the buffer to receive the size of byte sequence
 *
 * Returns: the pointer of byte array on success, or NULL on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT const unsigned char*
purc_variant_get_bytes_const(purc_variant_t value, size_t* nr_bytes);

/**
 * Get the number of bytes in a byte sequence variant value.
 *
 * @param sequence: the variant value of sequence type
 * @param length: the buffer receiving the length in bytes of the byte
 *  sequence.
 *
 * Returns: @true on success, otherwise @false.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_bsequence_bytes(purc_variant_t sequence, size_t *length);

/**
 * Get the number of bytes in a byte sequence variant value.
 *
 * @param sequence: the variant value of sequence type
 *
 * Returns: The number of bytes in an sequence variant value;
 *  \PURC_VARIANT_BADSIZE (-1) if the variant is not a byte sequence.
 *
 * Since: 0.0.1
 */
static inline ssize_t
purc_variant_bsequence_length(purc_variant_t sequence)
{
    size_t len;
    if (!purc_variant_bsequence_bytes(sequence, &len))
        return PURC_VARIANT_BADSIZE;
    return len;
}

#define PCVRT_CALL_FLAG_NONE            0x0000
#define PCVRT_CALL_FLAG_SILENTLY        0x0001
#define PCVRT_CALL_FLAG_AGAIN           0x0002
#define PCVRT_CALL_FLAG_TIMEOUT         0x0004

typedef purc_variant_t (*purc_dvariant_method) (purc_variant_t root,
        size_t nr_args, purc_variant_t * argv, unsigned call_flags);

/**
 * Creates dynamic value by setter and getter functions
 *
 * @param getter: the getter funciton pointer
 * @param setter: the setter function pointer
 *
 * Returns: A purc_variant_t with dynamic value,
 *      or %PURC_VARIANT_INVALID on failure.
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
purc_variant_dynamic_get_getter(purc_variant_t dynamic);


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
purc_variant_dynamic_get_setter(purc_variant_t dynamic);


typedef purc_variant_t (*purc_nvariant_method) (void* native_entity,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags);

/** the operation set for native entity */
struct purc_native_ops {
    /** query the getter for a specific property. */
    purc_nvariant_method (*property_getter)(void* native_entity,
            const char* propert_name);

    /** query the setter for a specific property. */
    purc_nvariant_method (*property_setter)(void* native_entity,
            const char* property_name);

    /** query the cleaner for a specific property. */
    purc_nvariant_method (*property_cleaner)(void* native_entity,
            const char* property_name);

    /** query the eraser for a specific property. */
    purc_nvariant_method (*property_eraser)(void* native_entity,
            const char* property_name);

    /** the updater to update the content represented by
      * the native entity (nullable). */
    purc_variant_t (*updater)(void* native_entity,
            purc_variant_t new_value, unsigned call_flags);

    /** the cleaner to clear the content represented by
      * the native entity (nullable). */
    purc_variant_t (*cleaner)(void* native_entity, unsigned call_flags);

    /** the eraser to erase the content represented by
      * the native entity (nullable). */
    purc_variant_t (*eraser)(void* native_entity, unsigned call_flags);

    /** verify if the val is match the observer */
    bool (*match_observe)(void* native_entity, purc_variant_t val);

    /** the callback when the variant was observed (nullable). */
    bool (*on_observe)(void* native_entity,
            const char *event_name, const char *event_subname);

    /** the callback when the variant was revoke observed (nullable). */
    bool (*on_forget)(void* native_entity,
            const char *event_name, const char *event_subname);

    /** the callback when the variant was released (nullable). */
    void (*on_release)(void* native_entity);
};

/**
 * Creates a variant value of native type.
 *
 * @param entity: the pointer to the native entity.
 * @param ops: the pointer to the ops for the native entity.
 *
 * Returns: A purc_variant_t with native value,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.2
 */
purc_variant_t purc_variant_make_native(void *native_entity,
    const struct purc_native_ops *ops);



/**
 * Get the native pointer of native variant value
 *
 * @param native: the variant value of native type
 *
 * Returns: the native pointer
 *
 * Since: 0.0.1
 */
PCA_EXPORT void *
purc_variant_native_get_entity(purc_variant_t native);

/**
 * Get the ops pointer of native variant value
 *
 * @param native: the variant value of native type
 *
 * Returns: the ops pointer
 *
 * Since: 0.0.1
 */
PCA_EXPORT struct purc_native_ops *
purc_variant_native_get_ops(purc_variant_t native);


/**
 * Creates a variant value of array type.
 *
 * @param sz: the size of array
 * @param value0 ..... valuen: enumerates every elements in array
 *
 * Returns: A purc_variant_t with array type,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_make_array(size_t sz, purc_variant_t value0, ...);

/**
 * Creates an empty array variant.
 *
 * Returns: An empty array variant or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.2.0
 */
static inline purc_variant_t
purc_variant_make_array_0(void)
{
    return purc_variant_make_array(0, PURC_VARIANT_INVALID);
}

/**
 * Appends a variant value to the tail of an array.
 *
 * @param array: the variant value of array type
 * @param value: the element to be appended
 *
 * Returns: @true on success, otherwise @false.
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
 * Returns: @true on success, otherwise @false.
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
 * Returns: A purc_variant_t on success, or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_array_get(purc_variant_t array, size_t idx);


/**
 * Sets an element value in an array by index.
 *
 * @param array: the variant value of array type
 * @param idx: the index of replaced element
 * @param value: the element to replace
 *
 * Returns: @true on success, otherwise @false.
 *
 * FIXME: returns -1?
 * Note: If idx is greater than max index of array, return -1.
 *       Whether free the replaced element, depends on its ref.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_array_set(purc_variant_t array, size_t idx, purc_variant_t value);


/**
 * Remove an element from an array by index.
 *
 * @param array: the variant value of array type
 * @param idx: the index of element to be removed
 *
 * Returns: @true on success, otherwise @false.
 *
 * FIXME: returns -1?
 * Note: If idx is greater than max index of array, return -1.
 *       Whether free the removed element, depends on its ref.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_array_remove(purc_variant_t array, int idx);


/**
 * Inserts an element to an array, places it before an indicated element.
 *
 * @param array: the variant value of array type
 * @param idx: the index of element before which the new element will be placed
 *
 * @param value: the inserted element
 *
 * Returns: @true on success, otherwise @false.
 *
 * FIXME: returns -1?
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
 * Returns: @true on success, otherwise @false.
 *
 * FIXME: returns -1?
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
 * @param sz: the buffer receiving the number of elements of the array.
 *
 * Returns: @true on success, otherwise @false.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_array_size(purc_variant_t array, size_t *sz);

/**
 * Get the number of elements in an array variant value.
 *
 * @param array: the variant value of array type
 *
 * Returns: The number of elements in the array;
 *  \PURC_VARIANT_BADSIZE (-1) if the variant is not an array.
 *
 * Note: This function is deprecated, use \purc_variant_array_size instead.
 *
 * Since: 0.0.1
 */
static inline ssize_t purc_variant_array_get_size(purc_variant_t array)
{
    size_t sz;
    if (!purc_variant_array_size(array, &sz))
        return PURC_VARIANT_BADSIZE;
    return sz;
}

/**
 * Creates a variant value of object type with key as c string
 *
 * @param nr_kv_pairs: the minimum of key-value pairs
 * @param key0 ..... keyn: the keys of key-value pairs
 * @param value0 ..... valuen: the values of key-value pairs
 *
 * Returns: A purc_variant_t with object type,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_make_object_by_static_ckey(size_t nr_kv_pairs,
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
 * Creates an empty object variant
 *
 * Returns: An empty object variant or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.2.0
 */
static inline purc_variant_t
purc_variant_make_object_0(void)
{
    return purc_variant_make_object(0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
}

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
purc_variant_object_get_by_ckey(purc_variant_t obj, const char* key);

/**
 * Sets the value by key in an object with key as another variant
 *
 * @param obj: the variant value of obj type
 * @param key: the key of key-value pair
 * @param value: the value of key-value pair
 *
 * Returns: @true on success, otherwise @false.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_object_set(purc_variant_t obj,
        purc_variant_t key, purc_variant_t value);

/**
 * Sets the value by key in an object with key as c string
 *
 * @param obj: the variant value of obj type
 * @param key: the key of key-value pair
 * @param value: the value of key-value pair
 *
 * Returns: @true on success, otherwise @false.
 *
 * Since: 0.0.1
 */
static inline bool
purc_variant_object_set_by_static_ckey(purc_variant_t obj, const char* key,
        purc_variant_t value)
{
    purc_variant_t k = purc_variant_make_string_static(key, true);
    if (k==PURC_VARIANT_INVALID) {
        return false;
    }
    bool b = purc_variant_object_set(obj, k, value);
    purc_variant_unref(k);
    return b;
}

/**
 * Remove a key-value pair from an object by key with key as c string
 *
 * @param obj: the variant value of obj type
 * @param key: the key of key-value pair
 * @param silently: @true means ignoring the following errors:
 *      - PCVARIANT_ERROR_NOT_FOUND (return @true)
 *
 * Returns: @true on success, otherwise @false.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_object_remove_by_static_ckey(purc_variant_t obj, const char* key,
        bool silently);

/**
 * Get the number of key-value pairs in an object variant value.
 *
 * @param obj: the variant value of object type
 * @param sz: the buffer receiving the number of the key-value pairs
 *  in the object
 *
 * Returns: @true on success, otherwise @false if the variant is not an object.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_object_size(purc_variant_t obj, size_t *sz);

/**
 * Get the number of key-value pairs in an object variant value.
 *
 * @param obj: the variant value of object type
 *
 * Returns: The number of the key-value pairs in the object;
 *  \PURC_VARIANT_BADSIZE (-1) if the variant is not an object.
 *
 * Note: This function is deprecated, use \purc_variant_object_size instead.
 *
 * Since: 0.0.1
 */
static inline ssize_t purc_variant_object_get_size(purc_variant_t obj)
{
    size_t sz;
    if (!purc_variant_object_size(obj, &sz))
        return PURC_VARIANT_BADSIZE;
    return sz;
}

/**
 * object iterator usage example:
 *
 * purc_variant_t obj;
 * ...
 * purc_variant_object_iterator* it;
 * it = purc_variant_object_make_iterator_begin(obj);
 * while (it) {
 *     const char     *key = purc_variant_object_iterator_get_key(it);
 *     purc_variant_t  val = purc_variant_object_iterator_get_value(it);
 *     ...
 *     bool having = purc_variant_object_iterator_next(it);
 *     // behavior of accessing `val`/`key` is un-defined
 *     if (!having) {
 *         // behavior of accessing `it` is un-defined
 *         break;
 *     }
 * }
 * if (it)
 *     purc_variant_object_release_iterator(it);
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
 * Returns: @true if iterator `it` has no following key-val-pair,
 *          @false otherwise
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
 * Returns: @true if iterator `it` has no leading key-val-pair, @false otherwise
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
 * Returns: the key of key-val-pair
 *
 * Since: 0.0.2
 */
PCA_EXPORT purc_variant_t
purc_variant_object_iterator_get_key(struct purc_variant_object_iterator* it);

/**
 * Get the key of key-val-pair that the iterator points to
 *
 * @param it: iterator of itself
 *
 * Returns: the key of key-val-pair, not duplicated
 *
 * Since: 0.0.2
 */
static inline const char*
purc_variant_object_iterator_get_ckey(struct purc_variant_object_iterator* it)
{
    purc_variant_t k = purc_variant_object_iterator_get_key(it);
    return purc_variant_get_string_const(k);
}

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
purc_variant_object_iterator_get_value(
        struct purc_variant_object_iterator* it);

/**
 * Creates a variant value of set type.
 *
 * @param sz: the initial number of elements in a set.
 * @param unique_key: the unique keys specified in a C string (nullable).
 *      If the unique keyis NULL, the set is a generic one.
 * @param caseless: if compare caselessly or not
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
purc_variant_make_set_by_ckey_ex(size_t sz, const char* unique_key,
        bool caseless, purc_variant_t value0, ...);

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
#define purc_variant_make_set_by_ckey(sz, unique_key, v0, ...)     \
    purc_variant_make_set_by_ckey_ex(sz,                           \
            unique_key, false,                                     \
            v0, ##__VA_ARGS__)

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
#define purc_variant_make_set(sz, unique_key, v0, ...)             \
    purc_variant_make_set_by_ckey_ex(sz,                           \
            purc_variant_get_string_const(unique_key), false,      \
            v0, ##__VA_ARGS__)

/**
 * Creates an empty set variant.
 *
 * @param unique_key: the unique keys specified in a variant. If the unique key
 *      is PURC_VARIANT_INVALID, the set is a generic one.
 *
 * Returns: An empty set variant on success, or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.2.0
 */
#define purc_variant_make_set_0(unique_key)                        \
    purc_variant_make_set_by_ckey_ex(0,                            \
            purc_variant_get_string_const(unique_key), false,      \
            PURC_VARIANT_INVALID)

/**
 * Adds a variant value to a set.
 *
 * @param set: the variant value of the set type.
 * @param value: the value to be added.
 * Returns: @true on success, @false if:
 *      - there is already such a value in the set.
 *      - the value is not an object if the set is managed by unique keys.
 * @param override: If the set is managed by unique keys and @overwrite is
 *  true, the function will override the old value which is equal to
 *  the new value under the unique keys, and return true. otherwise,
 *  it returns false.
 *
 * @note If the new value has not a property (a key-value pair) under
 *  a specific unique key, the value of the key should be treated
 *  as `undefined`.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_set_add(purc_variant_t obj, purc_variant_t value, bool overwrite);

/**
 * Remove a variant value from a set.
 *
 * @param set: the set to be operated
 * @param value: the value to be removed
 * @param silently: @true means ignoring the following errors:
 *      - PCVARIANT_ERROR_NOT_FOUND (return @true)
 *
 * Returns: @true on success, @false if:
 *      - silently is @false And no any matching member in the set.
 *
 * @note This function works if the set is not managed by unique keys, or
 *  there is only one unique key. If there are multiple unique keys,
 *  use @purc_variant_set_remove_member_by_key_values() instead.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_set_remove(purc_variant_t obj, purc_variant_t value, bool silently);

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
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_set_remove_member_by_key_values(purc_variant_t set,
        purc_variant_t v1, ...);

/**
 * Get the number of elements in a set variant value.
 *
 * @param set: the variant value of set type
 * @param sz: the variant value of set type
 *
 * Returns: @true on success, otherwise @false if the variant is not a set.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool purc_variant_set_size(purc_variant_t set, size_t *sz);

/**
 * Get the number of elements in a set variant value.
 *
 * @param set: the variant value of set type
 *
 * Returns: The number of elements in a set variant value;
 *  \PURC_VARIANT_BADSIZE (-1) if the variant is not a set.
 *
 * Note: This function is deprecated, use \purc_variant_set_size instead.
 *
 * Since: 0.0.1
 */
static inline ssize_t purc_variant_set_get_size(purc_variant_t set)
{
    size_t sz;
    if (!purc_variant_set_size(set, &sz))
        return PURC_VARIANT_BADSIZE;
    return sz;
}

/**
 * Get an element from set by index.
 *
 * @param array: the variant value of set type
 * @param idx: the index of wanted element
 *
 * Returns: A purc_variant_t on success, or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_set_get_by_index(purc_variant_t set, size_t idx);

/**
 * Remove the element in set by index and return
 *
 * @param array: the variant value of set type
 * @param idx: the index of the element to be removed
 *
 * Returns: the variant removed at the index or PURC_VARIANT_INVALID if failed
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_set_remove_by_index(purc_variant_t set, size_t idx);

/**
 * Set an element in set by index.
 *
 * @param array: the variant value of set type
 * @param idx: the index of the element to be replaced
 * @val: the val that's to be set in the set
 *
 * Returns: A boolean that indicates if it succeeds or not
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_set_set_by_index(purc_variant_t set,
        size_t idx, purc_variant_t val);

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
 *         // behavior of accessing `it` is un-defined
 *         break;
 *     }
 * }
 * if (it)
 *     purc_variant_set_release_iterator(it);
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
 * Returns: @true if iterator `it` has no following element, @false otherwise
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
 * Returns: @true if iterator `it` has no leading element, @false otherwise
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
 * Creates a tuple variant from variants.
 *
 * @param sz: the size of the tuple, i.e., the number of members in the tuple.
 * @param members: a C array of the members to put into the tuple.
 *
 * The function will setting the left members as null variants after
 * it encountered an invalud variant (%PURC_VARIANT_INVALID) in \members.
 * You can call purc_variant_tuple_set() to set the left members with other
 * variants.
 *
 * Note that if \members is %NULL, all members in the tuple will be
 * null variants initially.
 *
 * Returns: A purc_variant_t on success, or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.1.0
 */
PCA_EXPORT purc_variant_t
purc_variant_make_tuple(size_t sz, purc_variant_t *members);

/**
 * Creates an empty tuple variant.
 *
 * Returns: A purc_variant_t on success, or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.2.0
 */
static inline purc_variant_t
purc_variant_make_tuple_0(void)
{
    return purc_variant_make_tuple(0, NULL);
}

/**
 * Gets the size of a tuple variant.
 *
 * @param tuple: the tuple variant.
 * @param sz: the buffer to receive the size of the tuple.
 *
 * Returns: @true when success, or @false on failure.
 *
 * Since: 0.1.0
 */
PCA_EXPORT bool
purc_variant_tuple_size(purc_variant_t tuple, size_t *sz);

/**
 * Gets the size of a tuple variant.
 *
 * @param tuple: the tuple variant.
 *
 * Returns: The number of elements in the tuple;
 *  \PURC_VARIANT_BADSIZE (-1) if the variant is not a tuple.
 *
 * Since: 0.1.0
 */
static inline ssize_t
purc_variant_tuple_get_size(purc_variant_t tuple)
{
    size_t sz;
    if (!purc_variant_tuple_size(tuple, &sz))
        return PURC_VARIANT_BADSIZE;
    return sz;
}

/**
 * Gets a member from a tuple by index.
 *
 * @param dobule: the tuple variant.
 * @param idx: the index of wanted member.
 *
 * Returns: A purc_variant_t on success, or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.1.0
 */
PCA_EXPORT purc_variant_t
purc_variant_tuple_get(purc_variant_t tuple, size_t idx);

/**
 * Sets a member in a tuple by index.
 *
 * @param double: the tuple variant.
 * @param idx: the index of the member to replace.
 * @param value: the new value of the member.
 *
 * Returns: @true on success, otherwise @false.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_tuple_set(purc_variant_t tuple, size_t idx, purc_variant_t value);

#define PCVARIANT_SAFLAG_ASC            0x0000
#define PCVARIANT_SAFLAG_DESC           0x0001
#define PCVARIANT_SAFLAG_DEFAULT        0x0000

typedef int  (*pcvariant_compare_method) (purc_variant_t v1, purc_variant_t v2);

PCA_EXPORT purc_variant_t
purc_variant_make_sorted_array(unsigned int flags, size_t sz_init,
        pcvariant_compare_method cmp);

PCA_EXPORT int
purc_variant_sorted_array_add(purc_variant_t array, purc_variant_t value);

PCA_EXPORT bool
purc_variant_sorted_array_remove(purc_variant_t array, purc_variant_t value);

PCA_EXPORT bool
purc_variant_sorted_array_delete(purc_variant_t array, size_t idx);

PCA_EXPORT bool
purc_variant_sorted_array_find(purc_variant_t array, purc_variant_t value);

PCA_EXPORT purc_variant_t
purc_variant_sorted_array_get(purc_variant_t array, size_t idx);

PCA_EXPORT bool
purc_variant_sorted_array_size(purc_variant_t array, size_t *sz);

static inline ssize_t purc_variant_sorted_array_get_size(purc_variant_t array)
{
    size_t sz;
    if (!purc_variant_sorted_array_size(array, &sz))
        return PURC_VARIANT_BADSIZE;
    return sz;
}


/**
 * Gets the size of a linear container variant.
 *
 * @param container: the linear container variant, must be one of array,
 *      set, or tuple.
 * @param sz: the buffer receiving the number of members in the container.
 *
 * Returns: @true on success, otherwise @false.
 *
 * Since: 0.1.0
 */
PCA_EXPORT bool
purc_variant_linear_container_size(purc_variant_t container, size_t *sz);

/**
 * Get the number of elements in a linear container variant.
 *
 * @param container: the linear container variant, must be one of array,
 *      set, or tuple.
 *
 * Returns: The number of elements in the container;
 *  \PURC_VARIANT_BADSIZE (-1) if the variant is not a linear container.
 *
 * Note: This function is deprecated, use
 *  \purc_variant_linear_container_size() instead.
 *
 * Since: 0.0.1
 */
static inline ssize_t
purc_variant_linear_container_get_size(purc_variant_t container)
{
    size_t sz;
    if (!purc_variant_linear_container_size(container, &sz))
        return PURC_VARIANT_BADSIZE;
    return sz;
}

/**
 * Gets a member from a linear container by index.
 *
 * @param container: the linear container variant, must be one of array,
 *      set, or tuple.
 * @param idx: the index of wanted member.
 *
 * Returns: A purc_variant_t on success, or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.1.0
 */
PCA_EXPORT purc_variant_t
purc_variant_linear_container_get(purc_variant_t container, size_t idx);

/**
 * Sets a member in a linear container by index.
 *
 * @param container: the linear container variant, must be one of array,
 *      set, or tuple.
 * @param idx: the index of wanted member.
 * @param value: the new value.
 *
 * Returns: @true on success, otherwise @false.
 *
 * Since: 0.1.0
 */
PCA_EXPORT bool
purc_variant_linear_container_set(purc_variant_t container,
        size_t idx, purc_variant_t value);

/**
 * Creates a variant value from a string which contains JSON data.
 *
 * @param json: the pointer of string which contains JSON data.
 * @param sz: the size of string.
 *
 * Returns: A purc_variant_t on success, or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_make_from_json_string(const char* json, size_t sz);

/**
 * Creates a variant value from a file which contains JSON data
 *
 * @param file: the file name
 *
 * Returns: A purc_variant_t on success, or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_load_from_json_file(const char* file);


/**
 * Creates a variant value from a stream which contains JSON data.
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
 * Trys to cast a variant value to a 32-bit integer.
 *
 * @param v: the variant value.
 * @param i32: the buffer to receive the casted integer if success.
 * @param force: a boolean indicates whether to force casting, e.g.,
 *  parsing a string or returning a integer value for null
 *  and boolean type.
 *
 * Returns: @TRUE on success, or @FALSE on failure (the variant value can not
 *      be casted to a 32-bit integer).
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_cast_to_int32(purc_variant_t v, int32_t *i32, bool force);

/**
 * Trys to cast a variant value to a unsigned 32-bit integer.
 *
 * @param v: the variant value.
 * @param u32: the buffer to receive the casted integer if success.
 * @param force: a boolean indicates whether to force casting, e.g.,
 *  parsing a string or returning a integer value for null
 *  and boolean type.
 *
 * Returns: @TRUE on success, or @FALSE on failure (the variant value can not
 *      be casted to a unsigned 32-bit integer).
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_cast_to_uint32(purc_variant_t v, uint32_t *u32, bool force);

/**
 * Trys to cast a variant value to a long integer.
 *
 * @param v: the variant value.
 * @param i64: the buffer to receive the casted long integer if success.
 * @param force: a boolean indicates whether to force casting, e.g.,
 *  parsing a string or returning a long integer value for null
 *  and boolean type.
 *
 * Returns: @TRUE on success, or @FALSE on failure (the variant value can not
 *      be casted to a long integer).
 *
 * Note: A number, a long integer, an unsigned long integer, or a long double
 *      can always be casted to a long integer.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_cast_to_longint(purc_variant_t v, int64_t *i64, bool force);

/**
 * Trys to cast a variant value to a unsigned long integer.
 *
 * @param v: the variant value.
 * @param u64: the buffer to receive the casted unsigned long integer
 *      if success.
 * @param force: a boolean indicates whether to force casting, e.g.,
 *  parsing a string or returning a long integer value for null
 *  and boolean type.
 *
 * Returns: @TRUE on success, or @FALSE on failure (the variant value can not
 *      be casted to an unsigned long integer).
 *
 * Note: A negative number, long integer, or long double will be casted
 *      to 0 when @force is @true.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_cast_to_ulongint(purc_variant_t v, uint64_t *u64, bool force);

/**
 * Trys to cast a variant value to a nubmer.
 *
 * @param v: the variant value.
 * @param d: the buffer to receive the casted number if success.
 * @param force: a boolean indicates whether to force casting, e.g.,
 *  parsing a string or returning a long integer value for null
 *  and boolean type.
 *
 * Note: A number, a long integer, an unsigned long integer, or a long double
 *      can always be casted to a number (double float number).
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_cast_to_number(purc_variant_t v, double *d, bool force);

/**
 * Trys to cast a variant value to a long double float number.
 *
 * @param v: the variant value.
 * @param ld: the buffer to receive the casted long double if success.
 * @param force: a boolean indicates whether to force casting, e.g.,
 *  parsing a string or returning a long integer value for null
 *  and boolean type.
 *
 * Returns: @TRUE on success, or @FALSE on failure (the variant value can not
 *      be casted to a long double).
 *
 * Note: A number, a long integer, an unsigned long integer, or a long double
 *      can always be casted to a long double float number.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_cast_to_longdouble(purc_variant_t v, long double *ld,
        bool force);

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
 * Check one variant is equal to another exactly or not.
 *
 * @param v1: one variant value
 * @param v2: another variant value
 *
 * Returns: The function returns a boolean indicating whether v1 is equal
 *  to v2 exactly.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_is_equal_to(purc_variant_t v1, purc_variant_t v2);

typedef enum purc_variant_compare_opt
{
    PCVARIANT_COMPARE_OPT_AUTO,
    PCVARIANT_COMPARE_OPT_NUMBER,
    PCVARIANT_COMPARE_OPT_CASE,
    PCVARIANT_COMPARE_OPT_CASELESS,
} purc_vrtcmp_opt_t;

/**
 * Compares two variant value
 *
 * @param v1: one of compared variant value
 * @param v2: the other variant value to be compared
 * @param flags: comparison flags
 *
 * Returns: The function returns an integer less than, equal to, or greater
 *      than zero if v1 is found, respectively, to be less than, to match,
 *      or be greater than v2.
 *
 * Since: 0.0.1
 */
PCA_EXPORT int
purc_variant_compare_ex(purc_variant_t v1, purc_variant_t v2,
        purc_vrtcmp_opt_t opt);

/**
 * A flag for the purc_variant_serialize() function which serializes
 * all real numbers as JSON numbers.
 */
#define PCVARIANT_SERIALIZE_OPT_REAL_JSON               0x00000000

/**
 * A flag for the purc_variant_serialize() function which serializes
 * a real numbers by using EJSON notation.
 */
#define PCVARIANT_SERIALIZE_OPT_REAL_EJSON              0x00000001

/**
 * A flag for the purc_variant_serialize() function which serializes
 * all runtime types (undefined, dynamic, and native) as JSON null.
 */
#define PCVARIANT_SERIALIZE_OPT_RUNTIME_NULL            0x00000000

/**
 * A flag for the purc_variant_serialize() function which serializes
 * all runtime types (undefined, dynamic, and native) as placeholders
 * in JSON strings.
 */
#define PCVARIANT_SERIALIZE_OPT_RUNTIME_STRING          0x00000002

/**
 * A flag for the purc_variant_serialize() function which causes
 * the output to drop trailing zero for float values.
 */
#define PCVARIANT_SERIALIZE_OPT_NOZERO                  0x00000004

/**
 * A flag for the purc_variant_serialize() function which causes
 * the output to not escape the forward slashes ('/').
 */
#define PCVARIANT_SERIALIZE_OPT_NOSLASHESCAPE           0x00000008

/**
 * A flag for the purc_variant_serialize() function which
 * causes the output to have no extra whitespace or formatting applied.
 */
#define PCVARIANT_SERIALIZE_OPT_PLAIN                   0x00000000

/**
 * A flag for the purc_variant_serialize() function which causes
 * the output to have minimal whitespace inserted to make things slightly
 * more readable.
 */
#define PCVARIANT_SERIALIZE_OPT_SPACED                  0x00000010

/**
 * A flag for the purc_variant_serialize() function which causes
 * the output to be formatted by using "Two Space Tab".
 *
 * See the "Two Space Tab" option at <http://jsonformatter.curiousconcept.com>
 * for an example of the format.
 */
#define PCVARIANT_SERIALIZE_OPT_PRETTY                  0x00000020

/**
 * A flag for the purc_variant_serialize() function which causes
 * the output to be formatted by using a single tab character instead of
 * "Two Space Tab".
 */
#define PCVARIANT_SERIALIZE_OPT_PRETTY_TAB              0x00000040

#define PCVARIANT_SERIALIZE_OPT_BSEQUENCE_MASK          0x00000F00

/**
 * A flag for the purc_variant_serialize() function which causes
 * the function serializes byte sequences as a hexadecimal string.
 */
#define PCVARIANT_SERIALIZE_OPT_BSEQUENCE_HEX_STRING    0x00000000

/**
 * A flag for the purc_variant_serialize() function which causes
 * the output to use hexadecimal characters for byte sequence.
 */
#define PCVARIANT_SERIALIZE_OPT_BSEQUENCE_HEX           0x00000100

/**
 * A flag for the purc_variant_serialize() function which causes
 * the output to use binary characters for byte sequence.
 */
#define PCVARIANT_SERIALIZE_OPT_BSEQUENCE_BIN           0x00000200

/**
 * A flag for the purc_variant_serialize() function which causes
 * the output to use BASE64 encoding for byte sequence.
 */
#define PCVARIANT_SERIALIZE_OPT_BSEQUENCE_BASE64        0x00000300

/**
 * A flag for the purc_variant_serialize() function which causes
 * the output to have dot for binary sequence.
 */
#define PCVARIANT_SERIALIZE_OPT_BSEQUENCE_BIN_DOT       0x00000400

/**
 * A flag for the purc_variant_serialize() function which causes
 * the output to print unique keys of a set.
 */
#define PCVARIANT_SERIALIZE_OPT_UNIQKEYS                0x00001000

/**
 * A flag for the purc_variant_serialize() function which serializes
 * a tuple by using EJSON notation.
 */
#define PCVARIANT_SERIALIZE_OPT_TUPLE_EJSON             0x00002000

/**
 * A flag for the purc_variant_serialize() function which causes
 * the function ignores the output errors.
 */
#define PCVARIANT_SERIALIZE_OPT_IGNORE_ERRORS           0x10000000

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


#define PURC_ENVV_DVOBJS_PATH   "PURC_DVOBJS_PATH"

/**
 * Loads a variant value from an indicated library
 *
 * @param so_name: the library name
 *
 * @param var_name: the variant value name
 *
 * @param ver_code: version number
 *
 * Returns: A purc_variant_t on success, or PURC_VARIANT_INVALID on failure.
.*
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_load_dvobj_from_so (const char *so_name,
        const char *dvobj_name);

/**
 * Unloads a dynamic library
 *
 * @value: dynamic object
 *
 * Returns: @true for success, false on failure.
.*
 * Since: 0.0.1
 */
PCA_EXPORT bool purc_variant_unload_dvobj (purc_variant_t dvobj);

typedef enum purc_variant_type
{
    PURC_VARIANT_TYPE_FIRST = 0,

    /* XXX: keep consistency with type names */
#define PURC_VARIANT_TYPE_NAME_UNDEFINED    "undefined"
    PURC_VARIANT_TYPE_UNDEFINED = PURC_VARIANT_TYPE_FIRST,
#define PURC_VARIANT_TYPE_NAME_NULL         "null"
    PURC_VARIANT_TYPE_NULL,
#define PURC_VARIANT_TYPE_NAME_BOOLEAN      "boolean"
    PURC_VARIANT_TYPE_BOOLEAN,
#define PURC_VARIANT_TYPE_NAME_EXCEPTION    "exception"
    PURC_VARIANT_TYPE_EXCEPTION,
#define PURC_VARIANT_TYPE_NAME_NUMBER       "number"
    PURC_VARIANT_TYPE_NUMBER,
#define PURC_VARIANT_TYPE_NAME_LONGINT      "longint"
    PURC_VARIANT_TYPE_LONGINT,
#define PURC_VARIANT_TYPE_NAME_ULONGINT     "ulongint"
    PURC_VARIANT_TYPE_ULONGINT,
#define PURC_VARIANT_TYPE_NAME_LONGDOUBLE   "longdouble"
    PURC_VARIANT_TYPE_LONGDOUBLE,
#define PURC_VARIANT_TYPE_NAME_ATOMSTRING   "atomstring"
    PURC_VARIANT_TYPE_ATOMSTRING,
#define PURC_VARIANT_TYPE_NAME_STRING       "string"
    PURC_VARIANT_TYPE_STRING,
#define PURC_VARIANT_TYPE_NAME_BYTESEQUENCE "bsequence"
    PURC_VARIANT_TYPE_BSEQUENCE,
#define PURC_VARIANT_TYPE_NAME_DYNAMIC      "dynamic"
    PURC_VARIANT_TYPE_DYNAMIC,
#define PURC_VARIANT_TYPE_NAME_NATIVE       "native"
    PURC_VARIANT_TYPE_NATIVE,
#define PURC_VARIANT_TYPE_NAME_OBJECT       "object"
    PURC_VARIANT_TYPE_OBJECT,
#define PURC_VARIANT_TYPE_NAME_ARRAY        "array"
    PURC_VARIANT_TYPE_ARRAY,
#define PURC_VARIANT_TYPE_NAME_SET          "set"
    PURC_VARIANT_TYPE_SET,
#define PURC_VARIANT_TYPE_NAME_TUPLE        "tuple"
    PURC_VARIANT_TYPE_TUPLE,

    /* XXX: change this if you append a new type. */
    PURC_VARIANT_TYPE_LAST = PURC_VARIANT_TYPE_TUPLE,
} purc_variant_type;

#define PURC_VARIANT_TYPE_NR \
    (PURC_VARIANT_TYPE_LAST - PURC_VARIANT_TYPE_FIRST + 1)

/**
 * Whether the vairant is indicated type.
 *
 * @param value: the variant value
 * @param type: wanted type
 *
 * Returns: @true on success, otherwise @false.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool purc_variant_is_type(purc_variant_t value,
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
purc_variant_get_type(purc_variant_t value);

/**
 * Get the type name (static string) for a variant type.
 *
 * @param type: the variant type
 *
 * Returns: The type name (a pointer to a static string).
 *
 * Since: 0.1.0
 */
PCA_EXPORT const char*
purc_variant_typename(enum purc_variant_type type);

/** Check whether the value is a undefined. */
static inline bool purc_variant_is_undefined(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_UNDEFINED);
}

/** Check whether the value is a null. */
static inline bool purc_variant_is_null(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_NULL);
}

/** Check whether the value is a boolean. */
static inline bool purc_variant_is_boolean(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_BOOLEAN);
}

/** Check whether the value is an exception. */
static inline bool purc_variant_is_exception(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_EXCEPTION);
}

/** Check whether the value is a number. */
static inline bool purc_variant_is_number(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_NUMBER);
}

/** Check whether the value is a longint. */
static inline bool purc_variant_is_longint(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_LONGINT);
}

/** Check whether the value is a ulongint. */
static inline bool purc_variant_is_ulongint(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_ULONGINT);
}

/** Check whether the value is a longdouble. */
static inline bool purc_variant_is_longdouble(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_LONGDOUBLE);
}

/** Check whether the value is an atomstring. */
static inline bool purc_variant_is_atomstring(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_ATOMSTRING);
}

/** Check whether the value is a string. */
static inline bool purc_variant_is_string(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_STRING);
}

/** Check whether the value is a byte sequence. */
static inline bool purc_variant_is_bsequence(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_BSEQUENCE);
}

/** Check whether the value is a dynamic variant. */
static inline bool purc_variant_is_dynamic(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_DYNAMIC);
}

/** Check whether the value is a native entity. */
static inline bool purc_variant_is_native(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_NATIVE);
}

/** Check whether the value is an object. */
static inline bool purc_variant_is_object(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_OBJECT);
}

/** Check whether the value is an array. */
static inline bool purc_variant_is_array(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_ARRAY);
}

/** Check whether the value is a set. */
static inline bool purc_variant_is_set(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_SET);
}

static inline bool purc_variant_is_tuple(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_TUPLE);
}

/** Check whether the value is a boolean and having value of true. */
PCA_EXPORT bool
purc_variant_is_true(purc_variant_t v);

/** Check whether the value is a boolean and having value of false. */
PCA_EXPORT bool
purc_variant_is_false(purc_variant_t v);

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
PCA_INLINE purc_variant_t
purc_variant_object_get(purc_variant_t obj, purc_variant_t key)
{
    const char *sk = NULL;
    if (key && purc_variant_is_string(key))
        sk = purc_variant_get_string_const(key);

    return purc_variant_object_get_by_ckey(obj, sk);
}

/**
 * Remove a key-value pair from an object by key with key as another variant
 *
 * @param obj: the variant value of obj type
 * @param key: the key of key-value pair
 * @param silently: @true means ignoring the following errors:
 *      - PCVARIANT_ERROR_NOT_FOUND (return @true)
 *
 * Returns: @true on success, otherwise @false.
 *
 * Since: 0.0.1
 */
PCA_INLINE bool
purc_variant_object_remove(purc_variant_t obj, purc_variant_t key,
        bool silently)
{
    const char *sk = NULL;
    if (key && purc_variant_is_string(key))
        sk = purc_variant_get_string_const(key);

    return purc_variant_object_remove_by_static_ckey(obj, sk, silently);
}


struct purc_variant_stat {
    size_t nr_values[PURC_VARIANT_TYPE_NR];
    size_t sz_mem[PURC_VARIANT_TYPE_NR];
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
PCA_EXPORT const struct purc_variant_stat *
purc_variant_usage_stat(void);

/**
 * Numberify a variant value to double
 *
 * @param value: variant value to be operated
 *
 * Returns: a double number that is numberified from the variant value
 *
 * Since: 0.0.3
 */
PCA_EXPORT double
purc_variant_numerify(purc_variant_t value);

/**
 * Booleanize a variant value to boolean
 *
 * @param value: variant value to be operated
 *
 * Returns: a boolean value that is booleanized from the variant value
 *
 * Since: 0.0.3
 */
PCA_EXPORT bool
purc_variant_booleanize(purc_variant_t value);

/**
 * Stringify a variant value to a pre-allocated buffer.
 *
 * @param buff: the pointer to the buffer to store result content.
 * @param sz_buff: the size of the pre-allocated buffer.
 * @param value: the variant value to be stringified.
 *
 * Returns: totol # of result content that has been succesfully written
 *          or shall be written if buffer is large enough
 *          or -1 in case of other failure
 *
 * Note: API is similar to `snprintf`
 *
 * Since: 0.0.3
 */
PCA_EXPORT ssize_t
purc_variant_stringify_buff(char *buff, size_t sz_buff, purc_variant_t value);

/**
 * Stringify a variant value in the similar way as `asprintf` does.
 *
 * @param strp: the buffer to receive the pointer to the allocated space.
 * @param value: variant value to be operated.
 *
 * Returns: totol # of result content that has been succesfully written
 *          or shall be written if buffer is large enough
 *          or -1 in case of other failure, such of OOM
 *
 * Note: API is similar to `asprintf`
 *
 * Since: 0.0.3
 */
PCA_EXPORT ssize_t
purc_variant_stringify_alloc(char **strp, purc_variant_t value);

/**
 * A flag for the purc_variant_stringify() function which causes
 * the function ignores the output errors.
 */
#define PCVARIANT_STRINGIFY_OPT_IGNORE_ERRORS           0x10000000

/**
 * A flag for the purc_variant_stringify() function which causes
 * the function stringifies byte sequences as bare bytes.
 */
#define PCVARIANT_STRINGIFY_OPT_BSEQUENCE_BAREBYTES     0x00000100

/**
 * A flag for the purc_variant_stringify() function which causes
 * the function stringifies real numers as bare bytes.
 */
#define PCVARIANT_STRINGIFY_OPT_REAL_BAREBYTES          0x00000200

/**
 * Stringify a variant value to a writable stream.
 *
 * @param stream: the stream to which the stringified data write.
 * @param value: the variant value to be stringified.
 * @param flags: the stringifing flags.
 * @param len_expected: The buffer to receive the expected length of
 *      the stringified data (nullable). The value in the buffer should be
 *      set to 0 initially.
 *
 * Returns:
 * The size of the stringified data written to the stream;
 * On error, -1 is returned, and error code is set to indicate
 * the cause of the error.
 *
 * If the function is called with the flag
 * PCVARIANT_STRINGIFY_OPT_IGNORE_ERRORS set, this function always
 * returned the number of bytes written to the stream actually.
 * Meanwhile, if @len_expected is not null, the expected length of
 * the stringified data will be returned through this buffer.
 *
 * Therefore, you can prepare a small memory stream with the flag
 * PCVARIANT_STRINGIFY_OPT_IGNORE_ERRORS set to count the
 * expected length of the stringified data.
 *
 * Since: 0.1.1
 */
PCA_EXPORT ssize_t
purc_variant_stringify(purc_rwstream_t stream, purc_variant_t value,
        unsigned int flags, size_t *len_expected);

struct pcvar_listener;
typedef struct pcvar_listener pcvar_listener;
typedef struct pcvar_listener *pcvar_listener_t;

typedef enum {
    PCVAR_OPERATION_GROW         = (0x01 << 0),
    PCVAR_OPERATION_SHRINK       = (0x01 << 1),
    PCVAR_OPERATION_CHANGE       = (0x01 << 2),
    PCVAR_OPERATION_REFASCHILD   = (0x01 << 3),
    PCVAR_OPERATION_ALL          = ((0x01 << 4) - 1),
} pcvar_op_t;

typedef bool (*pcvar_op_handler) (
        purc_variant_t src,  // the source variant.
        pcvar_op_t op,       // the operation identifier.
        void *ctxt,          // the context stored when registering the handler.
        size_t nr_args,      // the number of the relevant child variants.
        purc_variant_t *argv // the array of all relevant child variants.
        );

/**
 * Register a pre-operation listener
 *
 * @param v: the variant that is to be observed
 *
 * @param op: the atom of the operation, such as `grow`,  `shrink`, or `change`
 *
 * @param handler: the callback that is to be called upon when the observed
 *                 event is fired
 * @param ctxt: the context belongs to the callback
 *
 * Returns: the registered-listener
 *
 * Since: 0.0.5
 */
PCA_EXPORT struct pcvar_listener*
purc_variant_register_pre_listener(purc_variant_t v,
        pcvar_op_t op, pcvar_op_handler handler, void *ctxt);

/**
 * Register a post-operation listener
 *
 * @param v: the variant that is to be observed
 *
 * @param op: the atom of the operation, such as `grow`,  `shrink`, or `change`
 *
 * @param handler: the callback that is to be called upon when the observed
 *                 event is fired
 * @param ctxt: the context belongs to the callback
 *
 * Returns: the registered-listener
 *
 * Since: 0.0.5
 */
PCA_EXPORT struct pcvar_listener*
purc_variant_register_post_listener(purc_variant_t v,
        pcvar_op_t op, pcvar_op_handler handler, void *ctxt);

/**
 * Revoke a variant listener
 *
 * @param v: the variant whose listener is to be revoked
 *
 * @param listener: the listener that is to be revoked
 *
 * Returns: boolean that designates if the operation succeeds or not
 *
 * Since: 0.0.4
 */
PCA_EXPORT bool
purc_variant_revoke_listener(purc_variant_t v,
        struct pcvar_listener *listener);

/**
 * Displace the values of the container.
 *
 * @param dst: the dst variant (object, array, set)
 * @param value: the variant to replace (object, array, set)
 * @param silently: @true means ignoring the following errors:
 *      - PURC_ERROR_INVALID_VALUE
 *      - PURC_ERROR_WRONG_DATA_TYPE
 *
 * Returns: @true on success, otherwise @false.
 *
 * Since: 0.0.5
 */
PCA_EXPORT bool
purc_variant_container_displace(purc_variant_t dst,
        purc_variant_t src, bool silently);

/**
 * Remove the values from the container.
 *
 * @param dst: the dst variant (object, array, set)
 * @param value: the variant to remove from container (object, array, set)
 * @param silently: @true means ignoring the following errors:
 *      - PURC_ERROR_INVALID_VALUE
 *      - PURC_ERROR_WRONG_DATA_TYPE
 *      - PCVARIANT_ERROR_NOT_FOUND
 *
 * Returns: @true on success, otherwise @false.
 *
 * Since: 0.0.5
 */
PCA_EXPORT bool
purc_variant_container_remove(purc_variant_t dst,
        purc_variant_t src, bool silently);

/**
 * Appends all the members of the array to the tail of the target array.
 *
 * @param array: the dst array variant
 * @param value: the value to be appended (array)
 * @param silently: @true means ignoring the following errors:
 *      - PURC_ERROR_INVALID_VALUE
 *      - PURC_ERROR_WRONG_DATA_TYPE
 *
 * Returns: @true on success, otherwise @false.
 *
 * Since: 0.0.5
 */
PCA_EXPORT bool
purc_variant_array_append_another(purc_variant_t array,
        purc_variant_t another, bool silently);

/**
 * Insert all the members of the array to the head of the target array.
 *
 * @param array: the dst array variant
 * @param value: the value to be insert (array)
 * @param silently: @true means ignoring the following errors:
 *      - PURC_ERROR_INVALID_VALUE
 *      - PURC_ERROR_WRONG_DATA_TYPE
 *
 * Returns: @true on success, otherwise @false.
 *
 * Since: 0.0.5
 */
PCA_EXPORT bool
purc_variant_array_prepend_another(purc_variant_t array,
        purc_variant_t another, bool silently);

/**
 * Merge value to the object
 *
 * @param object: the dst object variant
 * @param value: the value to be merge (object)
 * @param silently: @true means ignoring the following errors:
 *      - PURC_ERROR_INVALID_VALUE
 *      - PURC_ERROR_WRONG_DATA_TYPE
 *
 * Returns: @true on success, otherwise @false.
 *
 * Since: 0.0.5
 */
PCA_EXPORT bool
purc_variant_object_merge_another(purc_variant_t object,
        purc_variant_t another, bool silently);

/**
 * Insert all the members of the array into the target array and place it
 * after the indicated element.
 *
 * @param array: the dst array variant
 * @param idx: the index of element before which the new value will be placed
 * @param value: the inserted value (array)
 * @param silently: @true means ignoring the following errors:
 *      - PURC_ERROR_INVALID_VALUE
 *      - PURC_ERROR_WRONG_DATA_TYPE
 *
 * Returns: @true on success, otherwise @false.
 *
 * Since: 0.0.5
 */
PCA_EXPORT bool
purc_variant_array_insert_another_before(purc_variant_t array,
        int idx, purc_variant_t another, bool silently);

/**
 * Insert all the members of the array into the target array and place it
 * after the specified element
 *
 * @param array: the dst array variant
 * @param idx: the index of element after which the new value will be placed
 * @param value: the inserted value (array)
 * @param silently: @true means ignoring the following errors:
 *      - PURC_ERROR_INVALID_VALUE
 *      - PURC_ERROR_WRONG_DATA_TYPE
 *
 * Returns: @true on success, otherwise @false.
 *
 * Since: 0.0.5
 */
PCA_EXPORT bool
purc_variant_array_insert_another_after(purc_variant_t array,
        int idx, purc_variant_t another, bool silently);

/**
 * Unite operation on the set
 *
 * @param set: the dst set variant
 * @param value: the value to be unite (array, set)
 * @param silently: @true means ignoring the following errors:
 *      - PURC_ERROR_INVALID_VALUE
 *      - PURC_ERROR_WRONG_DATA_TYPE
 *
 * Returns: @true on success, otherwise @false.
 *
 * Since: 0.0.5
 */
PCA_EXPORT bool
purc_variant_set_unite(purc_variant_t set,
        purc_variant_t src, bool silently);

/**
 * Intersection operation on the set
 *
 * @param set: the dst set variant
 * @param value: the value to intersect (array, set)
 * @param silently: @true means ignoring the following errors:
 *      - PURC_ERROR_INVALID_VALUE
 *      - PURC_ERROR_WRONG_DATA_TYPE
 *
 * Returns: @true on success, otherwise @false.
 *
 * Since: 0.0.5
 */
PCA_EXPORT bool
purc_variant_set_intersect(purc_variant_t set,
        purc_variant_t src, bool silently);

/**
 * Subtraction operation on the set
 *
 * @param set: the dst set variant
 * @param value: the value to substract (array, set)
 * @param silently: @true means ignoring the following errors:
 *      - PURC_ERROR_INVALID_VALUE
 *      - PURC_ERROR_WRONG_DATA_TYPE
 *
 * Returns: @true on success, otherwise @false.
 *
 * Since: 0.0.5
 */
PCA_EXPORT bool
purc_variant_set_subtract(purc_variant_t set,
        purc_variant_t src, bool silently);

/**
 * Xor operation on the set
 *
 * @param set: the dst set variant
 * @param value: the value to xor (array, set)
 * @param silently: @true means ignoring the following errors:
 *      - PURC_ERROR_INVALID_VALUE
 *      - PURC_ERROR_WRONG_DATA_TYPE
 *
 * Returns: @true on success, otherwise @false.
 *
 * Since: 0.0.5
 */
PCA_EXPORT bool
purc_variant_set_xor(purc_variant_t set,
        purc_variant_t src, bool silently);

/**
 * Overwrite operation on the set
 *
 * @param set: the dst set variant
 * @param value: the value to overwrite (object, array, set)
 * @param silently: @true means ignoring the following errors:
 *      - PURC_ERROR_INVALID_VALUE
 *      - PURC_ERROR_WRONG_DATA_TYPE
 *      - PCVARIANT_ERROR_NOT_FOUND
 *
 * Returns: @true on success, otherwise @false.
 *
 * Since: 0.0.5
 */
PCA_EXPORT bool
purc_variant_set_overwrite(purc_variant_t set,
        purc_variant_t src, bool silently);


/**
 * Check if the variant is the mutable one, array/object/set
 *
 * @param var: the variant to check
 * @param is_mutable: the pointer where the result is to store
 *
 * Return: denote if the function succeeds or not
 *         0:  Success
 *         -1: Failed
 *
 * Since: 0.1.1
 */
PCA_EXPORT int
purc_variant_is_mutable(purc_variant_t var, bool *is_mutable);

/**
 * Clone a container
 *
 * @param ctnr: the source container variant
 *
 * Return: the cloned container variant
 *
 * Since: 0.1.1
 */
PCA_EXPORT purc_variant_t
purc_variant_container_clone(purc_variant_t ctnr);

/**
 * Recursively clone a container (deep clone).
 *
 * @param ctnr: the source container variant
 *
 * Return: the deep-cloned container variant
 *
 * Since: 0.1.1
 */
PCA_EXPORT purc_variant_t
purc_variant_container_clone_recursively(purc_variant_t ctnr);

struct purc_ejson_parse_tree;

/**
 * Parse an EJSON in the string and return the EJSON parse tree.
 *
 * @param str: the pointer to the string
 * @param sz: the size of the string in bytes.
 *
 * Return: the pointer to an EJSON parse tree on success, otherwise NULL.
 *
 * Since: 0.1.1
 */
PCA_EXPORT struct purc_ejson_parse_tree *
purc_variant_ejson_parse_string(const char *ejson, size_t sz);

/**
 * Parse an EJSON in the file and return the EJSON parse tree.
 *
 * @param fname: the file name.
 *
 * Return: the pointer to an EJSON parse tree on success, otherwise NULL.
 *
 * Since: 0.1.1
 */
PCA_EXPORT struct purc_ejson_parse_tree *
purc_variant_ejson_parse_file(const char *fname);

/**
 * Parse an EJSON stream and return the EJSON parse tree.
 *
 * @param rws: the stream of purc_rwstream_t type.
 *
 * Return: the pointer to an EJSON parse tree on success, otherwise NULL.
 *
 * Since: 0.1.1
 */
PCA_EXPORT struct purc_ejson_parse_tree *
purc_variant_ejson_parse_stream(purc_rwstream_t rws);

typedef purc_variant_t (*purc_cb_get_var)(void* ctxt, const char* name);

/**
 * Evaluate an EJSON parse tree with customized variables.
 *
 * @param parse_tree: the parse tree will be evaluated.
 * @param fn_get_var: the callback function returns the variant
 *      for a variable name.
 * @param ctxt: the context will be passed to the callback when evaluting
 *      a variable.
 * @param silently: @true means ignoring the errors.
 *
 * Since: 0.1.1
 */
PCA_EXPORT purc_variant_t
purc_variant_ejson_parse_tree_evalute(struct purc_ejson_parse_tree *parse_tree,
        purc_cb_get_var fn_get_var, void *ctxt, bool silently);

/**
 * Destroy an EJSON parse tree.
 *
 * @param parse_tree: the parse tree will be destroyed.
 *
 * Since: 0.1.1
 */
PCA_EXPORT void
purc_variant_ejson_parse_tree_destroy(struct purc_ejson_parse_tree *parse_tree);

PCA_EXTERN_C_END

#define PURC_VARIANT_SAFE_CLEAR(_v)             \
do {                                            \
    if (_v != PURC_VARIANT_INVALID) {           \
        purc_variant_unref(_v);                 \
        _v = PURC_VARIANT_INVALID;              \
    }                                           \
} while (0)

#endif /* not defined PURC_PURC_VARIANT_H */
