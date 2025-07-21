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
#include "purc/purc-features.h"
#include "purc/purc-macros.h"

/**
 * SECTION: purc_variant
 * @title: Variant
 * @short_description: Variant is an abstract representation of data for HVML.
 */

struct purc_variant;
typedef struct purc_variant purc_variant;
typedef struct purc_variant* purc_variant_t;

#define PURC_VARIANT_INVALID            ((purc_variant_t)(0))
#define PURC_VARIANT_BADSIZE            ((ssize_t)(-1))

PCA_EXTERN_C_BEGIN

/**
 * purc_variant_wrapper_size_ex:
 *
 * @scalar: Indicate the category of a variant (scalar or not).
 *
 * Gets the size of the wrapper of a scalar or vector variant.
 *
 * Returns: The size of a variant wrapper.
 *
 * Since: 0.1.1
 */
PCA_EXPORT size_t
purc_variant_wrapper_size_ex(bool scalar);

/**
 * purc_variant_wrapper_size:
 *
 * Gets the size of the wrapper of a vector variant.
 *
 * Returns: The size of a variant wrapper.
 *
 * Since: 0.1.1
 */
static inline size_t
purc_variant_wrapper_size(void) {
    return purc_variant_wrapper_size_ex(false);
}

/**
 * purc_variant_ref_count:
 *
 * @value: A variant value.
 *
 * Gets the reference count of @value.
 *
 * Returns: The reference count of the variant.
 *
 * Since: 0.1.0
 */
PCA_EXPORT unsigned int
purc_variant_ref_count(purc_variant_t value);

/**
 * purc_variant_ref:
 *
 * @value: The variant value to reference.
 *
 * Increments the reference count of @value by one.
 *
 * Returns: The passed in @value on success, %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_ref(purc_variant_t value);

/**
 * purc_variant_unref:
 *
 * @value: A variant value to un-reference.
 *
 * Decrements the reference count of @value by one.
 * If the reference count drops to 0, the variant will be released.
 *
 * Since: 0.0.1
 */
PCA_EXPORT unsigned int
purc_variant_unref(purc_variant_t value);

/**
 * purc_variant_get_memory_size:
 *
 * @value: A variant value to calculate.
 *
 * Get the memory size occupied by the specified variant.
 *
 * Since: 0.9.22
 */
PCA_EXPORT size_t
purc_variant_get_memory_size(purc_variant_t value);

/**
 * purc_variant_make_undefined:
 *
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
 * purc_variant_make_exception:
 *
 * @except_atom: The atom value of an exception.
 *
 * Creates a variant which represents the exception specified by @except_atom.
 *
 * Returns: A variant having value of the specified exception atom,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.2
 */
PCA_EXPORT purc_variant_t
purc_variant_make_exception(purc_atom_t except_atom);

/**
 * purc_variant_get_exception_string_const:
 *
 * @value: An exception variant created by purc_variant_make_exception().
 *
 * Gets the exception name string of the exception variant @value.
 *
 * Returns: The pointer to the exception name which is a null-terminated string,
 *      or %NULL if the variant is not an exception.
 *
 * Since: 0.0.2
 */
PCA_EXPORT const char *
purc_variant_get_exception_string_const(purc_variant_t value);

/**
 * purc_variant_make_null:
 *
 * Creates a variant which represents a null value.
 *
 * Returns: A variant having value of `null`.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_make_null(void);

/**
 * purc_variant_make_boolean:
 *
 * @b: A C bool value.
 *
 * Creates a variant which represents the boolean value @b.
 *
 * Returns: A variant having the specified boolean value,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_make_boolean(bool b);

/**
 * purc_variant_make_number:
 *
 * @d: A C double value.
 *
 * Creates a variant which represents the number value @d.
 *
 * Returns: A variant having the specified number value,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_make_number(double d);

/**
 * purc_variant_make_ulongint:
 *
 * @u64: A C uint64_t value which specifying an unsigned long integer.
 *
 * Creates a variant which represents an unsigned long integer value @u64.
 *
 * Returns: A variant having the specified unsigned long integer value,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_make_ulongint(uint64_t u64);

/**
 * purc_variant_make_longint:
 *
 * @i64: A C int64_t value which specifying an long integer.
 *
 * Creates a variant which represents a long integer value @i64.
 *
 * Returns: A variant having the specified long integer value,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_make_longint(int64_t i64);

/**
 * purc_variant_make_longdouble:
 *
 * @lf: A long double value which specifying a high precision float number.
 *
 * Creates a long double variant which represents a high precision float number
 * by using @lf.
 *
 * Returns: A long double variant having the specified high precision float number,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_make_longdouble(long double lf);

/**
 * purc_variant_make_bigint_from_i64:
 *
 * @i64: A signed 64-bit integer.
 *
 * Creates a bigint variant which represents an arbitrary precision integer
 * from @i64.
 *
 * Returns: A bigint variant having the specified initial value,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.9.26
 */
PCA_EXPORT purc_variant_t
purc_variant_make_bigint_from_i64(int64_t i64);

/**
 * purc_variant_make_bigint_from_u64:
 *
 * @u64: A unsigned 64-bit integer.
 *
 * Creates a bigint variant which represents an arbitrary precision integer
 * from @u64.
 *
 * Returns: A bigint variant having the specified initial value,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.9.26
 */
PCA_EXPORT purc_variant_t
purc_variant_make_bigint_from_u64(uint64_t u64);

/**
 * purc_variant_make_bigint_from_double:
 *
 * @d: A double (64-bit) floating-point number.
 * @force: A boolean indicates whether to discard the fraction part of
 *      the floating number.
 *
 * Creates a bigint variant which represents an arbitrary precision integer
 * from @d.
 *
 * Returns: A bigint variant having the specified initial value,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.9.26
 */
PCA_EXPORT purc_variant_t
purc_variant_make_bigint_from_double(double d, bool force);

/**
 * purc_variant_make_bigint_from_longdouble:
 *
 * @ld: A long double floating-point number.
 * @force: A boolean indicates whether to discard the fraction part of
 *      the floating number.
 *
 * Creates a bigint variant which represents an arbitrary precision integer
 * from @ld.
 *
 * Returns: A bigint variant having the specified initial value,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.9.26
 */
PCA_EXPORT purc_variant_t
purc_variant_make_bigint_from_longdouble(long double ld, bool force);

/**
 * purc_variant_make_bigint_from_string:
 *
 * @str: A number string in the specified base.
 * @end (nullable): A pointer to a buffer to store the address
 *      of the first invalid character.
 * @base: The base of the number string; it must be between 2 and 36 inclusive,
 *      or be the special value 0.
 *
 * Create a bigint variant which represents an arbitrary precision integer
 * from a number string in the given base.
 *
 * This function stores the address of the first invalid character in *end.
 *
 * Returns: A bigint variant having the specified initial value,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.9.26
 */
PCA_EXPORT purc_variant_t
purc_variant_make_bigint_from_string(const char *str, char **end, int base);

/**
 * purc_variant_make_string:
 *
 * @str_utf8: The pointer to a null-terminated string
 *      which is encoded in UTF-8.
 * @check_encoding: Whether to check the encoding.
 *
 * Creates a variant which represents a null-terminated string
 * in UTF-8 encoding. Note that the new variant will hold a copy of
 * the specified string if success.
 *
 * Returns: A variant contains the specified text string in UTF-8 encoding,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_make_string(const char* str_utf8, bool check_encoding);

/**
 * purc_variant_make_string_static:
 *
 * @str_utf8: The pointer to a null-terminated string
 *      which is in UTF-8 encoding.
 * @check_encoding: Whether to check the encoding.
 *
 * Creates a variant which represents a null-terminted static string
 * in UTF-8 encoding. Note that the new variant will only hold the pointer
 * to the static string, not a copy of the string.
 *
 * Returns: A variant contains the pointer to the static string,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.2
 */
PCA_EXPORT purc_variant_t
purc_variant_make_string_static(const char* str_utf8, bool check_encoding);

/**
 * purc_variant_make_string_reuse_buff:
 *
 * @str_utf8: The pointer to a null-terminated string which is encoded
 *      in UTF-8.
 * @sz_buff: The size of the buffer (not the length of the string).
 * @check_encoding: Whether to check the encoding.
 *
 * Create a variant which represents a null-terminated string in
 * UTF-8 encoding. Note that the new variant will take the ownership of
 * the buffer which containing the string. The buffer will be released by
 * calling free() when the variant is destroyed ultimately.
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
 * purc_variant_make_string_ex:
 *
 * @str_utf8: The pointer to a string which is encoded in UTF-8.
 * @len: The length of string (in bytes) to be copied at most.
 * @check_encoding: Whether to check the encoding.
 *
 * Creates a variant which represents a null-terminated string in UTF-8
 * encoding by copying a string, but at most the specified length copied.
 *
 * Returns: A variant which copies the string of specified length in
 *      UTF-8 encoding, or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.5
 */
PCA_EXPORT purc_variant_t
purc_variant_make_string_ex(const char* str_utf8, size_t len,
        bool check_encoding);

/**
 * purc_variant_get_string_const_ex:
 *
 * @value: A string, an atom, or an exception variant.
 * @length (nullable): The pointer to a size_t buffer to receive
 *      the length of the string (not including the terminating null byte).
 *
 * Gets the pointer of the string contained in the specified variant
 * if the variant represents a string, an atom, or an exception variant.
 *
 * Note that you should not change the contents in the buffer
 * pointed to by the return value.
 *
 * Returns: The pointer to the string in UTF-8 encoding, or %NULL if
 *      the variant does not respresent a string, an atom, or an exception.
 *
 * Since: 0.1.0
 */
PCA_EXPORT const char*
purc_variant_get_string_const_ex(purc_variant_t value, size_t *length);

/**
 * purc_variant_get_string_const:
 *
 * @value: A string, an atom, or an exception variant.
 *
 * Gets the pointer of the string contained in the specified variant
 * if the variant represents a string, an atom, or an exception variant.
 *
 * Note that you should not change the contents in the buffer
 * pointed to by the return value.
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
 * purc_variant_string_bytes:
 *
 * @value: A string, an atom, or an exception variant.
 * @length (nullable): The pointer to a size_t buffer to receive
 *      the length of the string (not including the terminating null byte).
 *
 * Gets the length of the string contained in the specified variant
 * if the variant represents a string, an atom, or an exception variant.
 *
 * Returns: %true on success, otherwise %false.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_string_bytes(purc_variant_t value, size_t *length);

/**
 * purc_variant_string_size:
 *
 * @value: A string, an atom, or an exception variant.
 *
 * Get the length of the string contained in the specified variant
 * if the variant represents a string, an atom, or an exception variant.
 *
 * Returns: The length in bytes (including the terminating null byte)
 *  of the string on success; %PURC_VARIANT_BADSIZE (-1) if the value
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
 * purc_variant_string_chars:
 *
 * @value: A string, an atom, or an exception variant.
 * @nr_chars: The pointer to a size_t buffer to receive the number of
 *  valid characters in the string.
 *
 * Gets the number of valid characters of the string contained in
 * the specified variant, if the variant represents a string, an atom,
 * or an exception variant.
 *
 * Returns: %true on success, and %false if the value is not a string,
 *      an atom, or an exception variant.
 *
 * Since: 0.1.0
 */
PCA_EXPORT bool
purc_variant_string_chars(purc_variant_t value, size_t *nr_chars);

/**
 * purc_variant_make_atom:
 *
 * @atom: An atom value returned by purc_atom_from_string_ex() or
 *      purc_atom_from_string_static_ex().
 *
 * Creates a variant which represents an atom.
 *
 * Returns: A variant contains the given atom,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.9.2
 */
PCA_EXPORT purc_variant_t
purc_variant_make_atom(purc_atom_t atom);

/**
 * purc_variant_make_atom_string:
 *
 * @str_utf8: The pointer to a null-terminated string
 *      which is encoded in UTF-8.
 * @check_encoding: Whether to check the encoding.
 *
 * Creates a variant which represents the atom corresponding to the given
 * string @str_utf8.
 *
 * Returns: A variant contains the atom corresponding to the given string,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_make_atom_string(const char* str_utf8,
        bool check_encoding);

/**
 * purc_variant_make_atom_string_static:
 *
 * @str_utf8: The pointer to a null-terminated static string
 *      which is encoded in UTF-8.
 * @check_encoding: Whether to check the encoding.
 *
 * Creates a variant which represents the atom corresponding to the given
 * static string.
 *
 * Returns: A variant contains the atom corresponding to the given string,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_make_atom_string_static(const char* str_utf8,
        bool check_encoding);

/**
 * purc_variant_get_atom_string_const:
 *
 * @value: An atom variant.
 *
 * Gets the pointer to the string which is associated to the atom contained in
 * the given atom variant.
 *
 * Returns: The pointer to the string which is associated to the atom,
 *      or %NULL if the variant does not respresent an atom.
 *
 * Since: 0.0.1
 */
PCA_EXPORT const char*
purc_variant_get_atom_string_const(purc_variant_t value);

/**
 * purc_variant_make_byte_sequence:
 *
 * @bytes: The pointer to a byte sequence.
 * @nr_bytes: The number of bytes in the sequence.
 *
 * Creates a variant which represents a byte sequence (`bsequence` for short).
 * Note that the new variant will hold a copy of the specified byte sequence
 * if success.
 *
 * Returns: A bsequence variant if success,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_make_byte_sequence(const void* bytes, size_t nr_bytes);

/**
 * purc_variant_make_byte_sequence_static:
 *
 * @bytes: The pointer to a byte sequence.
 * @nr_bytes: The number of bytes in the sequence.
 *
 * Creates a variant which represents a byte sequence (`bsequence` for short).
 * Note that the new variant will only hold the pointer to the byte sequence,
 * not a copy of the byte sequence.
 *
 * Returns: A bsequence variant if success,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.2
 */
PCA_EXPORT purc_variant_t
purc_variant_make_byte_sequence_static(const void* bytes, size_t nr_bytes);

/**
 * purc_variant_make_byte_sequence_reuse_buff:
 *
 * @bytes: The pointer to a byte sequence.
 * @nr_bytes: The number of bytes in the sequence.
 * @sz_buff: The size of the buffer storing the bytes.
 *
 * Creates a variant which represents a byte sequence (`bsequence` for short)
 * by reusing the buffer storing the bytes. Note that the buffer will be
 * released by calling free() when the variant is destroyed.
 *
 * Returns: A bsequence variant if success,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.2
 */
PCA_EXPORT purc_variant_t
purc_variant_make_byte_sequence_reuse_buff(void* bytes, size_t nr_bytes,
        size_t sz_buff);

/**
 * purc_variant_make_byte_sequence_empty:
 *
 * Creates a variant which represents an empty byte sequence.
 *
 * Returns: An empty bsequence variant if success,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.2
 */
PCA_EXPORT purc_variant_t
purc_variant_make_byte_sequence_empty(void);

/**
 * purc_variant_make_byte_sequence_empty_ex:
 *
 * @sz_buf: The size of the buffer desired in bytes.
 *
 * Creates a variant which represents an empty byte sequence but has
 * the specified buffer length.
 *
 * Returns: An empty bsequence variant if success,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.9.22
 */
PCA_EXPORT purc_variant_t
purc_variant_make_byte_sequence_empty_ex(size_t sz_buf);

/**
 * purc_variant_bsequence_buffer:
 *
 * @sequence: The bsequence variant.
 * @nr_bytes: The pointer to a size_t buffer to receive the length in bytes
 *      of the byte sequence.
 * @sz_buf: The pointer to a size_t buffer to receive the size in bytes
 *      of the buffer.
 *
 * Gets the pointer to the buffer of the sequence. If it is a byte sequence
 * created from a static buffer, the size of the buffer will be zero.
 *
 * Returns: The pointer to the bytes array on success, or %NULL on failure.
 *
 * Since: 0.9.22
 */
PCA_EXPORT unsigned char *
purc_variant_bsequence_buffer(purc_variant_t sequence, size_t *nr_bytes,
        size_t *sz_buf);

/**
 * purc_variant_bsequence_set_bytes:
 *
 * @sequence: The bsequence variant.
 * @nr_bytes: The new bytes valid in the buffer.
 *
 * Set the new number of valid bytes in the buffer.
 *
 * Returns: %true for success, or %false for failure.
 *
 * Since: 0.9.22
 */
PCA_EXPORT bool
purc_variant_bsequence_set_bytes(purc_variant_t sequence, size_t nr_bytes);

/**
 * purc_variant_bsequence_append:
 *
 * @sequence: The bsequence variant.
 * @bytes: The pointer to the new byte array to be appended to the sequence.
 * @nr_bytes: The length in bytes of the new byte array.
 *
 * Append a new byte array to the byte sequence which has an enough long buffer.
 *
 * Returns: %true for success, or %false for failure.
 *
 * Since: 0.9.22
 */
PCA_EXPORT bool
purc_variant_bsequence_append(purc_variant_t sequence,
        const unsigned char *bytes, size_t nr_bytes);

/**
 * purc_variant_bsequence_roll:
 *
 * @sequence: The bsequence variant.
 * @offset: The offset starting roll the byte sequence.
 *
 * Roll a byte sequence from the specified position. The left bytes starting
 * from @offset will be copied to the head of the buffer. It will empty
 * the byte sequence if @offset is less than 0.
 *
 * Returns: The number of bytes rolled actually, or -1 for failure.
 *
 * Since: 0.9.22
 */
PCA_EXPORT ssize_t
purc_variant_bsequence_roll(purc_variant_t sequence, ssize_t offset);

/**
 * purc_variant_bsequence_bytes:
 *
 * @bsequence: The bsequence variant.
 * @length: The pointer to a size_t buffer receiving the length in bytes of
 *      the byte sequence.
 *
 * Gets the number of bytes contained in a bsequence variant.
 *
 * Returns: %true on success, otherwise %false.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_bsequence_bytes(purc_variant_t bsequence, size_t *length);

/**
 * purc_variant_bsequence_length:
 *
 * @bseqence: The bsequence variant.
 *
 * Gets the number of bytes contained in a bsequence variant.
 *
 * Returns: The number of bytes in @bsequence variant;
 *  %PURC_VARIANT_BADSIZE (-1) if the variant is not a bsequence.
 *
 * Since: 0.0.1
 */
static inline ssize_t
purc_variant_bsequence_length(purc_variant_t bsequence)
{
    size_t len;
    if (!purc_variant_bsequence_bytes(bsequence, &len))
        return PURC_VARIANT_BADSIZE;
    return len;
}

/**
 * purc_variant_get_bytes_const:
 *
 * @value: The bsequence or string variant.
 * @nr_bytes: The pointer to a size_t buffer to receive the length in bytes
 *      of the byte sequence.
 *
 * Gets the pointer to the bytes array contained in a bsequence or
 * a string variant.
 *
 * Returns: The pointer to the bytes array on success, or %NULL on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT const unsigned char *
purc_variant_get_bytes_const(purc_variant_t value, size_t *nr_bytes);

#define PCVRT_CALL_FLAG_NONE            0x0000
#define PCVRT_CALL_FLAG_SILENTLY        0x0001
#define PCVRT_CALL_FLAG_AGAIN           0x0002
#define PCVRT_CALL_FLAG_TIMEOUT         0x0004

typedef purc_variant_t (*purc_dvariant_method) (purc_variant_t root,
        size_t nr_args, purc_variant_t * argv, unsigned call_flags);

/**
 * purc_variant_make_dynamic:
 *
 * @getter: The pointer to the getter function.
 * @setter: The pointer to the setter function.
 *
 * Creates a dynamic variant by using the given setter and getter functions.
 *
 * Returns: A dynamic variant, or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_make_dynamic(purc_dvariant_method getter,
        purc_dvariant_method setter);

/**
 * purc_variant_dynamic_get_getter:
 *
 * @dynamic: The dynamic variant.
 *
 * Gets the getter function of a dynamic variant.
 *
 * Returns: The pointer to the getter function of @dynamic.
 *      Note that the getter function of a dynamic variant might be %NULL.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_dvariant_method
purc_variant_dynamic_get_getter(purc_variant_t dynamic);

/**
 * purc_variant_dynamic_get_setter:
 *
 * @dynamic: The dynamic variant.
 *
 * Gets the setter function a dynamic variant.
 *
 * Returns: The pointer to the setter function of @dynamic.
 *      Note that the setter function of a dynamic variant might be %NULL.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_dvariant_method
purc_variant_dynamic_get_setter(purc_variant_t dynamic);

typedef purc_variant_t (*purc_nvariant_method)(
        void* native_entity, const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags);

/**
 * purc_native_ops:
 *
 * The operation set for a native entity variant.
 */
struct purc_native_ops {
    /** This operation gets the value represented by
      * the native entity itself (nullable).
    purc_nvariant_method getter;

    ** This operation sets the value represented by
      * the native entity itself (nullable).
    purc_nvariant_method setter;
    */

    /** This operation returns the getter for a specific property.
      * If @property_name is NULL, it returns the getter for
      * the native entity itself. */
    purc_nvariant_method (*property_getter)(void* native_entity,
            const char* property_name);

    /** This operation returns the setter for a specific property.
      * If @property_name is NULL, it returns the setter for
      * the native entity itself. */
    purc_nvariant_method (*property_setter)(void* native_entity,
            const char* property_name);

    /** This operations returns the cleaner for a specific property. */
    purc_nvariant_method (*property_cleaner)(void* native_entity,
            const char* property_name);

    /** This operation returns the eraser for a specific property. */
    purc_nvariant_method (*property_eraser)(void* native_entity,
            const char* property_name);

    /** This operation updates the content represented by
      * the native entity (nullable). */
    purc_variant_t (*updater)(void* native_entity,
            purc_variant_t new_value, unsigned call_flags);

    /** This operation cleans the content represented by
      * the native entity (nullable). */
    purc_variant_t (*cleaner)(void* native_entity, unsigned call_flags);

    /** This operation erases to erase the content represented by
      * the native entity (nullable). */
    purc_variant_t (*eraser)(void* native_entity, unsigned call_flags);

    /** This operation checks if the destination specified by @val matches */
    bool (*did_matched)(void* native_entity, purc_variant_t val);

    /**
     * This operation will be called when the variant was observed (nullable).
     */
    bool (*on_observe)(void* native_entity,
            const char *event_name, const char *event_subname);

    /**
     * This operation will be called when the observer on this entity was
     * revoked (nullable).
     */
    bool (*on_forget)(void* native_entity,
            const char *event_name, const char *event_subname);

    /**
     * This operation will be called when the variant was released (nullable).
     */
    void (*on_release)(void* native_entity);

    const void *priv_ops;
};

/**
 * purc_variant_make_native_entity:
 *
 * @entity (nullable): The pointer to a native entity.
 * @ops (nullable): The pointer to the operation set structure
 *      (#purc_native_ops) for the native entity.
 * @name (nullable): The pointer to a static null-terminated string which
 *      indicates the name of the native entity. If it is %NULL, the native
 *      entity will have the default name `anonymous`.
 *
 * Create a variant which represents a native entity.
 *
 * Returns: A desired native entity variant,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.9.8
 */
PCA_EXPORT purc_variant_t
purc_variant_make_native_entity(void *native_entity,
    struct purc_native_ops *ops, const char *name);

/**
 * purc_variant_make_native:
 *
 * @entity (nullable): The pointer to a native entity.
 * @ops (nullable): The pointer to the operation set structure
 *      (#purc_native_ops) for the native entity.
 *
 * Create a variant which represents the native entity.
 *
 * Returns: A desired native entity variant,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.2
 */
static inline purc_variant_t
purc_variant_make_native(void *native_entity, struct purc_native_ops *ops)
{
    return purc_variant_make_native_entity(native_entity, ops, NULL);
}

/**
 * purc_variant_native_get_entity:
 *
 * @native: A native entity variant.
 *
 * Gets the pointer to the entity of the native entity variant @native.
 *
 * Returns: The pointer to the native pointer. On failure, it returns %NULL
 *      and sets error code %PCVRNT_ERROR_INVALID_TYPE.
 *      Note that, the pointer to the entity can be %NULL for a valid native
 *      entity variant.
 *
 * Since: 0.0.1
 */
PCA_EXPORT void *
purc_variant_native_get_entity(purc_variant_t native);

/**
 * purc_variant_native_get_ops:
 *
 * @native: A native entity variant.
 *
 * Get the pointer to the operation set of the native entity variant @native.
 *
 * Returns: The pointer to the native pointer. On failure, it returns %NULL
 *      and sets error code %PCVRNT_ERROR_INVALID_TYPE.
 *      Note that, the pointer to the entity can be %NULL for a valid native
 *      entity variant.
 *
 * Since: 0.0.1
 */
PCA_EXPORT struct purc_native_ops *
purc_variant_native_get_ops(purc_variant_t native);

/**
 * purc_variant_native_get_name:
 *
 * @native: A native entity variant.
 *
 * Get the pointer to the name of the native entity variant @native.
 *
 * Returns: The pointer to the native name string. On failure, it returns %NULL
 *      and sets error code %PCVRNT_ERROR_INVALID_TYPE. Note that, the pointer
 *      to the entity name can be %NULL for a valid native entity variant.
 *
 * Since: 0.9.8
 */
PCA_EXPORT const char *
purc_variant_native_get_name(purc_variant_t native);

/**
 * purc_variant_native_set_ops:
 *
 * @native: A native entity variant.
 * @ops (nullable): The pointer to the new operation set structure
 *      (#purc_native_ops) for the native entity.
 *
 * Set the operation set of the given native entity variant @native.
 *
 * Returns: The pointer to the old native operation set. On failure, it returns
 *      %NULL and sets error code %PCVRNT_ERROR_INVALID_TYPE. Note that,
 *      the pointer to the operation set can be %NULL for a valid native
 *      entity variant.
 *
 * Since: 0.9.22
 */
PCA_EXPORT struct purc_native_ops *
purc_variant_native_set_ops(purc_variant_t native,
        struct purc_native_ops *ops);

/**
 * purc_variant_make_array:
 *
 * @sz: The size of the new array.
 * @value0: The first member of the new array.
 * @...: The subsequent members of the new array.
 *
 * Creates an array variant with the specified initial size and members.
 *
 * Returns: An array variant contains the specified initial members,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_make_array(size_t sz, purc_variant_t value0, ...);

/**
 * purc_variant_make_array_0:
 *
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
 * purc_variant_array_append:
 *
 * @array: An array variant.
 * @value: The variant to be appended to the array.
 *
 * Appends a variant at the tail of @array.
 *
 * Returns: %true on success, otherwise %false.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_array_append(purc_variant_t array, purc_variant_t value);

/**
 * purc_variant_array_prepend:
 *
 * @array: An array variant.
 * @value: The variant to be prepended to the array.
 *
 * Prepends a variant at the head of @array.
 *
 * Returns: %true on success, otherwise %false.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_array_prepend(purc_variant_t array, purc_variant_t value);

/**
 * purc_variant_array_get:
 *
 * @array: An array variant.
 * @idx: The index of the member.
 *
 * Gets the member from the array variant @array by index (@idx).
 *
 * Returns: A variant on success, or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_array_get(purc_variant_t array, size_t idx);

/**
 * purc_variant_array_set:
 *
 * @array: An array variant.
 * @idx: The index of the member to be set.
 * @value: The variant.
 *
 * Sets the member in the array (@array) by index (@idx) with @value.
 * Note that the reference count of @value will be incremented,
 * and the reference count of the old value of the member will be decremented.
 *
 * Returns: %true on success, otherwise %false.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_array_set(purc_variant_t array, size_t idx, purc_variant_t value);

/**
 * purc_variant_array_remove:
 *
 * @array: An array variant.
 * @idx: The index of the member to be removed.
 *
 * Removes the member in the array (@array) by index (@idx).
 * Note that the size of the array will decrement, and the reference count
 * of the member removed will decrement as well.
 *
 * Returns: %true on success, otherwise %false.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_array_remove(purc_variant_t array, int idx);

/**
 * purc_variant_array_insert_before:
 *
 * @array: An array variant.
 * @idx: The index used to specify the insertion position.
 * @value: The variant will be inserted to the array.
 *
 * Inserts a variant to @array, before the member indicated by the index (@idx).
 * Note that the size of the array will increment, and the reference count
 * of @value will increment as well.
 *
 * Returns: %true on success, otherwise %false.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_array_insert_before(purc_variant_t array,
        int idx, purc_variant_t value);

/**
 * purc_variant_array_insert_after:
 *
 * @array: An array variant.
 * @idx: The index used to specify the insertion position.
 * @value: The variant will be inserted to the array.
 *
 * Inserts a variant to @array, after the member indicated by the index (@idx).
 * Note that the size of the array will increment, and the reference count
 * of @value will increment as well.
 *
 * Returns: %true on success, otherwise %false.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_array_insert_after(purc_variant_t array,
        int idx, purc_variant_t value);

/**
 * purc_variant_array_size:
 *
 * @array: An array variant.
 * @sz: The pointer to a size_t buffer to receive the size of the array.
 *
 * Gets the size (the number of all members) in the array variant @array,
 * and returns it through @sz.
 *
 * Returns: %true on success, otherwise %false.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_array_size(purc_variant_t array, size_t *sz);

/**
 * purc_variant_array_get_size:
 *
 * @array: An array variant.
 *
 * Gets the number of members in the array variant, i.e., the size of @array.
 *
 * Returns: The number of elements in the array;
 *  \PURC_VARIANT_BADSIZE (-1) if the variant is not an array.
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
 * purc_variant_make_object_by_static_ckey:
 *
 * @nr_kv_pairs: The number of the initial key/value pairs.
 * @key0: The key of the first property given by null-terminated strings.
 * @value0: The value of the first property.
 * @...: The subsequent key/value pairs of the new object.
 *
 * Creates an object variant with the given key/value pairs.
 * The result object will have @nr_kv_pairs properties.
 *
 * Returns: An object variant, or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_make_object_by_static_ckey(size_t nr_kv_pairs,
        const char* key0, purc_variant_t value0, ...);

/**
 * purc_variant_make_object:
 *
 * @nr_kv_pairs: The number of the initial key/value pairs.
 * @key0: The key of the first property.
 * @value0: The value of the first property.
 * @...: The subsequent key/value pairs of the new object.
 *
 * Creates an object variant with the given key/value pairs.
 * The result object will have @nr_kv_pairs properties.
 *
 * Returns: An object variant, or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_make_object(size_t nr_kv_pairs,
        purc_variant_t key0, purc_variant_t value0, ...);

/**
 * purc_variant_make_object_0:
 *
 * Creates an empty object variant.
 *
 * Returns: An empty object variant or %PURC_VARIANT_INVALID on failure.
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
 * purc_variant_object_get_by_ckey_ex:
 *
 * @obj: An object variant.
 * @key: The key of the property to find.
 * @silently: Indicate whether to report the following error(s):
 *  - PURC_ERROR_NO_SUCH_KEY
 *
 * Gets the property value in @obj by the key value specified with
 * a null-terminated string @key.
 *
 * Returns: The property value on success, or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.9.22
 */
PCA_EXPORT purc_variant_t
purc_variant_object_get_by_ckey_ex(purc_variant_t obj, const char* key,
        bool silently);

/**
 * purc_variant_object_get_by_ckey:
 *
 * @obj: An object variant.
 * @key: The key of the property to find.
 *
 * Gets the property value in @obj by the key value specified with
 * a null-terminated string @key.
 *
 * Returns: The property value on success, or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_INLINE purc_variant_t
purc_variant_object_get_by_ckey(purc_variant_t obj, const char* key) {
    return purc_variant_object_get_by_ckey_ex(obj, key, true);
}

/**
 * purc_variant_object_get_ex:
 *
 * @obj: An object variant.
 * @key: The key of the property to find.
 * @silently: Indicate whether to report the following error(s):
 *  - PURC_ERROR_NO_SUCH_KEY
 *
 * Gets the property value in @obj by the key value specified by
 * a string, an atom, or an exception variant.
 *
 * Returns: The property value on success, or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_INLINE purc_variant_t
purc_variant_object_get_ex(purc_variant_t obj, purc_variant_t key,
        bool silently)
{
    const char *sk = purc_variant_get_string_const(key);
    if (sk) {
        return purc_variant_object_get_by_ckey_ex(obj, sk, silently);
    }

    return PURC_VARIANT_INVALID;
}

/**
 * purc_variant_object_get:
 *
 * @obj: An object variant.
 * @key: The key of the property to find.
 *
 * Gets the property value in @obj by the key value specified by
 * a string, an atom, or an exception variant.
 *
 * Returns: The property value on success, or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_INLINE purc_variant_t
purc_variant_object_get(purc_variant_t obj, purc_variant_t key)
{
    const char *sk = purc_variant_get_string_const(key);
    if (sk) {
        return purc_variant_object_get_by_ckey_ex(obj, sk, true);
    }

    return PURC_VARIANT_INVALID;
}

/**
 * purc_variant_object_set:
 *
 * Sets the value of the property given by @key to @value, in the object
 * variant @obj.
 *
 * If there is no property in @obj specified by @key, this function will
 * create a new property with @key and @value.
 *
 * Note that the reference count of @value will be incremented,
 * and the reference count of the replaced value (if have) will be decremented.
 *
 * Returns: %true on success, otherwise %false.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_object_set(purc_variant_t obj,
        purc_variant_t key, purc_variant_t value);

/**
 * purc_variant_object_set_by_static_ckey:
 *
 * @obj: An object variant.
 * @key: The key of the property to set.
 * @value: The new property value.
 *
 * Sets the value of the property given by a static null-terminated
 * string @key to @value, in the object variant @obj.
 *
 * If there is no property in @obj specified by @key, this function will
 * create a new property with @key and @value.
 *
 * Returns: %true on success, otherwise %false.
 *
 * Since: 0.0.1
 */
static inline bool
purc_variant_object_set_by_static_ckey(purc_variant_t obj, const char* key,
        purc_variant_t value)
{
    purc_variant_t k = purc_variant_make_string_static(key, true);
    if (k == PURC_VARIANT_INVALID) {
        return false;
    }
    bool b = purc_variant_object_set(obj, k, value);
    purc_variant_unref(k);
    return b;
}

/**
 * purc_variant_object_set_by_ckey:
 *
 * @obj: An object variant.
 * @key: The key of the property to set.
 * @value: The new property value.
 *
 * Set the value of the property given by a static null-terminated
 * string @key to @value, in the object variant @obj.
 *
 * If there is no property in @obj specified by @key, this function will
 * create a new property with @key and @value.
 *
 * Returns: %true on success, otherwise %false.
 *
 * Since: 0.9.8
 */
static inline bool
purc_variant_object_set_by_ckey(purc_variant_t obj, const char* key,
        purc_variant_t value)
{
    purc_variant_t k = purc_variant_make_string(key, true);
    if (k == PURC_VARIANT_INVALID) {
        return false;
    }
    bool b = purc_variant_object_set(obj, k, value);
    purc_variant_unref(k);
    return b;
}

/**
 * purc_variant_object_remove_by_ckey:
 *
 * @obj: An object variant.
 * @key: The key of an property, specified by a static null-terminated string.
 * @silently: Whether to ignore the following errors.
 *      - PCVRNT_ERROR_NOT_FOUND
 *
 * Removes the property given by a static null-terminated string @key
 * in the object variant @obj.
 *
 * If @silently is %true, this function will return %true even if the property
 * specified by @key does not exist.
 *
 * Returns: %true on success, otherwise %false.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_object_remove_by_ckey(purc_variant_t obj, const char* key,
        bool silently);

/**
 * purc_variant_object_remove:
 *
 * @obj: An object variant.
 * @key: The key of the property to find.
 * @silently: Whether to ignore the following errors:
 *      - PCVRNT_ERROR_NOT_FOUND
 *
 * Removes a property from the object by the key value specified by
 * a string, an atom, or an exception variant.
 *
 * Returns: %true on success, otherwise %false.
 *
 * Since: 0.0.1
 */
PCA_INLINE bool
purc_variant_object_remove(purc_variant_t obj, purc_variant_t key,
        bool silently)
{
    const char *sk = purc_variant_get_string_const(key);
    if (sk) {
        return purc_variant_object_remove_by_ckey(obj, sk, silently);
    }

    return false;
}

/**
 * purc_variant_object_size:
 *
 * @obj: An object variant.
 * @sz: The pointer to a size_t buffer to receive the size of the object.
 *
 * Gets the size (the number of all properties) in the object variant @obj,
 * and returns it through @sz.
 *
 * Returns: %true on success, otherwise %false if the variant is not an object.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_object_size(purc_variant_t obj, size_t *sz);

/**
 * purc_variant_object_get_size:
 *
 * @obj: An object variant.
 *
 * Gets the size (the number of all properties) in the object variant @obj.
 *
 * Returns: The size of @obj on success,
 *      or %PURC_VARIANT_BADSIZE (-1) if the variant is not an object.
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

typedef enum pcvrnt_conflict_resolution_method {
    PCVRNT_CR_METHOD_IGNORE,
    PCVRNT_CR_METHOD_OVERWRITE,
    PCVRNT_CR_METHOD_COMPLAIN,
} pcvrnt_cr_method_k;

typedef enum pcvrnt_notfound_resolution_method {
    PCVRNT_NR_METHOD_IGNORE,
    PCVRNT_NR_METHOD_COMPLAIN,
} pcvrnt_nr_method_k;

/**
 * purc_variant_object_unite:
 *
 * @dst: The destination object variant.
 * @src: The source object variant.
 * @cr_method: The method to resolve the conflict, can be one of the following
 *  values:
 *      - PCVRNT_CR_METHOD_IGNORE:
 *        Ignore the source value and keep the destination property not changed.
 *      - PCVRNT_CR_METHOD_OVERWRITE:
 *        Overwrite the value of the property in the destination object.
 *      - PCVRNT_CR_METHOD_COMPLAIN:
 *        Report %PURC_ERROR_DUPLICATED error.
 *
 * Unites properties in an object (@src) to the destination object (@dst).
 *
 * Returns: The number of properties in the destination object changed or added,
 *      -1 for error.
 *
 * Since: 0.0.5
 */
PCA_EXPORT ssize_t
purc_variant_object_unite(purc_variant_t dst,
        purc_variant_t src, pcvrnt_cr_method_k cr_method);

/**
 * purc_variant_object_intersect:
 *
 * @dst: The destination object variant.
 * @src: The source object variant to intersect.
 *
 * Intersect @src with the given destination object, that is, only keeps the
 * properties which are also existing in @src,
 * whatever the property values are.
 *
 * Returns: The number of properties of the destination object
 * after intersecting, -1 on error.
 *
 * Since: 0.9.4
 */
PCA_EXPORT ssize_t
purc_variant_object_intersect(purc_variant_t dst, purc_variant_t src);

/**
 * purc_variant_object_subtract:
 *
 * @dst: The destination object variant.
 * @src: The source object variant to subtract.
 *
 * Subtracts @src from the destination object @dst.
 * It will remove any properties in @dst which is also existing in @src,
 * whatever the property values are.
 *
 * Returns: The number of properties of the destination object
 * after subtracting, -1 on error.
 *
 * Since: 0.9.4
 */
PCA_EXPORT ssize_t
purc_variant_object_subtract(purc_variant_t dst, purc_variant_t src);

/**
 * purc_variant_object_xor:
 *
 * @dst: The destination object variant.
 * @src: The source object variant to XOR.
 *
 * Does XOR operation on the destination object.
 *
 * Returns: The number of properties of the object after the operation,
 *  -1 on error.
 *
 * Since: 0.9.4
 */
PCA_EXPORT ssize_t
purc_variant_object_xor(purc_variant_t dst, purc_variant_t src);

/**
 * purc_variant_object_overwrite:
 *
 * @dst: The destination object variant to overwrite.
 * @src: The source object variant.
 * @nr_method: The method to resolve the not-found error if there is no member
 *  matched the value in the object.
 *      - PCVRNT_NR_METHOD_IGNORE:
 *        Ignore and go on.
 *      - PCVRNT_NR_METHOD_COMPLAIN:
 *        Report %PCVRNT_ERROR_NOT_FOUND error.
 *
 * Overwrites the properties in the given object with properties in @src.
 *
 * Returns: The number of changed properties of the object, -1 on error.
 *
 * Since: 0.9.4
 */
PCA_EXPORT ssize_t
purc_variant_object_overwrite(purc_variant_t dst, purc_variant_t src,
        pcvrnt_nr_method_k nr_method);

/**
 * pcvrnt_object_iterator:
 *
 * purc_variant_t obj;
 * ...
 * struct pcvrnt_object_iterator *it;
 * it = pcvrnt_object_iterator_create_begin(obj);
 * while (it) {
 *     const char     *key = pcvrnt_object_iterator_get_ckey(it);
 *     purc_variant_t  val = pcvrnt_object_iterator_get_value(it);
 *     ...
 *     bool having = pcvrnt_object_iterator_next(it);
 *     // behavior of accessing `val`/`key` is un-defined
 *     if (!having) {
 *         // behavior of accessing `it` is un-defined
 *         break;
 *     }
 * }
 * if (it)
 *     pcvrnt_object_iterator_release(it);
 */
struct pcvrnt_object_iterator;

/**
 * pcvrnt_object_iterator_create_begin:
 *
 * @object: An object variant.
 *
 * Creates a new beginning iterator for the object variant @object.
 * The returned iterator will point to the first property in the object.
 *
 * Note that a new iterator will hold a reference of the object, until it is
 * released by calling pcvrnt_object_iterator_release(). It will also
 * hold a reference of the property it points to, until it was moved to
 * another one.
 *
 * Returns: The iterator for the object; %NULL if there is no property
 *      in the object.
 *
 * Since: 0.0.1
 */
PCA_EXPORT struct pcvrnt_object_iterator*
pcvrnt_object_iterator_create_begin(purc_variant_t object);

/**
 * pcvrnt_object_iterator_create_end:
 *
 * @object: An object variant.
 *
 * Creates a new end iterator for the object variant @object.
 * The returned iterator will point to the last property in the object.
 *
 * Note that a new iterator will hold a reference of the object, until it is
 * released by calling pcvrnt_object_iterator_release(). It will also
 * hold a reference of the property it points to, until it was moved to
 * another one.
 *
 * Returns: The iterator for the object; %NULL if there is no property
 *      in the object.
 *
 * Since: 0.0.1
 */
PCA_EXPORT struct pcvrnt_object_iterator*
pcvrnt_object_iterator_create_end(purc_variant_t object);

/**
 * pcvrnt_object_iterator_release:
 *
 * @it: The iterator of an object variant.
 *
 * Releases the object iterator (@it). The reference count of the object
 * and the property (if any) pointed to by @it will be decremented.
 *
 * Returns: None.
 *
 * Since: 0.0.1
 */
PCA_EXPORT void
pcvrnt_object_iterator_release(struct pcvrnt_object_iterator* it);

/**
 * pcvrnt_object_iterator_next:
 *
 * @it: The iterator of an object variant.
 *
 * Forwards the iterator to point to the next property in the object.
 * Note that the iterator will release the reference of the former property,
 * and hold a new reference to the next property (if any).
 *
 * Returns: %true if success, @false if there is no subsequent property.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
pcvrnt_object_iterator_next(struct pcvrnt_object_iterator* it);

/**
 * pcvrnt_object_iterator_prev:
 *
 * @it: The iterator of an object variant.
 *
 * Backwards the iterator to point to the previous property in the object.
 * Note that the iterator will release the reference of the former property
 * it pointed to, and hold a new reference to the previous property (if any).
 *
 * Returns: %true if success, @false if there is no preceding property.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
pcvrnt_object_iterator_prev(struct pcvrnt_object_iterator* it);

/**
 * pcvrnt_object_iterator_get_key:
 *
 * @it: The iterator of an object variant.
 *
 * Gets the key of the property to which the iterator @it points.
 *
 * Returns: The variant of the current property key.
 *
 * Since: 0.0.2
 */
PCA_EXPORT purc_variant_t
pcvrnt_object_iterator_get_key(struct pcvrnt_object_iterator* it);

/**
 * pcvrnt_object_iterator_get_ckey:
 *
 * @it: The iterator of an object variant.
 *
 * Gets the key as a read-only null-terminated string of the property to which
 * the iterator @it points.
 *
 * Returns: The pointer to a read-only null-terminated string which is
 *      holden by the current property's key.
 *
 * Since: 0.0.2
 */
static inline const char*
pcvrnt_object_iterator_get_ckey(struct pcvrnt_object_iterator* it)
{
    purc_variant_t k = pcvrnt_object_iterator_get_key(it);
    return purc_variant_get_string_const(k);
}

/**
 * pcvrnt_object_iterator_get_value:
 *
 * @it: The iterator of an object variant.
 *
 * Gets the value of the property to which the iterator @it points.
 *
 * Returns: The variant of the current property value.
 *
 * Since: 0.0.2
 */
PCA_EXPORT purc_variant_t
pcvrnt_object_iterator_get_value(
        struct pcvrnt_object_iterator* it);

/**
 * purc_variant_make_set_by_ckey_ex:
 *
 * @sz: The initial size (the number of members) of the new set.
 * @unique_key (nullable): The unique keys specified in a null-terminated
 *      string. If there are multiple keys, use whitespaces to separate them.
 *      If it is %NULL, this function will create a generic set.
 * @caseless: Compare values in case-insensitively or not.
 * @value0: The first variant will be added to the set as the ininitial member.
 * @...: The subsequent variants as the ininitial members for the new set.
 *
 * Creates a set variant having the space for @sz members by using the
 * specified values @value0 as the initial members and the unique keys
 * specified by @unique_key.
 *
 * To determine whether a value will be added to the set is unique for the set,
 * the value of an variant or the values specified by the unique keys
 * in an object variant will be stringified and concatenated (if multiple
 * unique keys specified) to a string first, then compare with members which
 * are already existing in the set case-insensitively (@caseless is %true) or
 * case-sensitively (@caseless is %false). If there is no such existing member
 * matched, the value will become a new member of the set. Otherwise, this
 * function will overwrite the existing member with the new value.
 *
 * Note that if @unique_key is not %NULL, but the value to be added to the set
 * is not an object, or it is an object but there is no property attached to
 * the key in the object variant, the value will be treated as %undefined.
 *
 * Returns: A set variant on success, or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_make_set_by_ckey_ex(size_t sz, const char* unique_key,
        bool caseless, purc_variant_t value0, ...);

/**
 * purc_variant_make_set_by_ckey:
 *
 * @sz: The initial size (the number of members) of the new set.
 * @unique_key (nullable): The unique keys specified in a null-terminated
 *      string. If there are multiple keys, use whitespaces to separate them.
 *      If it is %NULL, this function will create a generic set.
 * @value0: The first variant will be added to the set as the ininitial member.
 * @...: The subsequent variants as the ininitial members for the new set.
 *
 * Creates a set variant having the space for @sz members by using the
 * specified values @value0 as the initial members and the unique keys
 * specified by @unique_key.
 *
 * To determine whether a value will be added to the set is unique for the set,
 * the value of an variant or the values specified by the unique keys
 * in an object variant will be stringified and concatenated (if multiple
 * unique keys specified) to a string first, then compare with members which
 * are already existing in the set case-sensitively.
 * If there is no such existing member matched, the value will become
 * a new member of the set. Otherwise, this
 * function will overwrite the existing member with the new value.
 *
 * Note that if @unique_key is not %NULL, but the value to be added to the set
 * is not an object, or it is an object but there is no property attached to
 * the key in the object variant, the value will be treated as %undefined.
 *
 * Returns: A set variant on success, or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
#define purc_variant_make_set_by_ckey(sz, unique_key, v0, ...)     \
    purc_variant_make_set_by_ckey_ex(sz,                           \
            unique_key, false,                                     \
            v0, ##__VA_ARGS__)

/**
 * purc_variant_make_set:
 *
 * @sz: The initial size (the number of members) of the new set.
 * @unique_key: The unique keys specified by a variant.
 *      Use %PURC_VARIANT_INVALID for a generic set.
 * @value0: The first variant will be added to the set as the ininitial member.
 * @...: The subsequent variants as the ininitial members for the new set.
 *
 * Creates a set variant having the space for @sz members by using the
 * specified values @value0 as the initial members and the unique keys
 * specified by @unique_key variant.
 *
 * Returns: A set variant on success, or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
#define purc_variant_make_set(sz, unique_key, v0, ...)                      \
    purc_variant_make_set_by_ckey_ex(sz,                                    \
            unique_key ? purc_variant_get_string_const(unique_key) : NULL,  \
            false, v0, ##__VA_ARGS__)

/**
 * purc_variant_make_set_0:
 *
 * @unique_key: The unique keys specified by a variant.
 *      Use %PURC_VARIANT_INVALID for a generic set.
 *
 * Creates a empty set variant by using the unique keys specified by
 * @unique_key variant.
 *
 * Returns: A empty set variant on success, or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.2.0
 */
#define purc_variant_make_set_0(unique_key)                                 \
    purc_variant_make_set_by_ckey_ex(0,                                     \
            unique_key ? purc_variant_get_string_const(unique_key) : NULL,  \
            false, PURC_VARIANT_INVALID)

/**
 * purc_variant_set_add:
 *
 * @set: An set variant.
 * @value: The value to be added to the set variant.
 * @cr_method: The method to resolve the conflict if @value having
 *  the same value under the unique keys of the set. It can be one of
 *  the following values:
 *      - PCVRNT_CR_METHOD_IGNORE:
 *        Ignore the source value and keep the destination set not changed.
 *      - PCVRNT_CR_METHOD_OVERWRITE:
 *        Overwrite the member in the destination set.
 *      - PCVRNT_CR_METHOD_COMPLAIN:
 *        Report %PURC_ERROR_DUPLICATED error.
 *
 * Adds a new value to the set.
 *
 * If the set is managed by unique keys and @cr_method is
 * %PCVRNT_CR_METHOD_OVERWRITE, the function will overwrite the existing member
 * which is equal to the new value under the unique keys.
 *
 * Note that if the new value has not a property under a specific unique key,
 * the value of the key will be treated as `undefined`.
 *
 * Returns: The number of new members or changed members (1 or 0) in the set,
 * -1 for error.
 *
 * Since: 0.0.1
 */
PCA_EXPORT ssize_t
purc_variant_set_add(purc_variant_t set, purc_variant_t value,
        pcvrnt_cr_method_k cr_method);

/**
 * purc_variant_set_remove:
 *
 * @set: An set variant.
 * @value: The value to be removed.
 * @nr_method: The method to resolve the not-found error if there is no member
 *  matched the value in the set.
 *      - PCVRNT_NR_METHOD_IGNORE:
 *        Ignore the conflict and keep the destination set not changed.
 *      - PCVRNT_NR_METHOD_COMPLAIN:
 *        Report %PCVRNT_ERROR_NOT_FOUND error.
 *
 * Removes a variant from a given set variant (@set).
 *
 * Note that this function works if the set is not managed by unique keys,
 * or there is only one unique key. If there are multiple unique keys,
 * use @purc_variant_set_remove_member_by_key_values() instead.
 *
 * Returns: The number of members removed (1 or 0) in the destination set,
 *  -1 for error.
 *
 * Since: 0.0.1
 */
PCA_EXPORT ssize_t
purc_variant_set_remove(purc_variant_t obj, purc_variant_t value,
        pcvrnt_nr_method_k nr_method);

/**
 * purc_variant_set_get_member_by_key_values:
 *
 * @set: An set variant. The set should be managed by unique keys.
 * @v1: The first value for the first unique key.
 * @...: The values for the other unique keys.
 *
 * Gets the member by the values of unique keys from a set.
 * The caller should pass one value for each unique key.
 * The number of the matching values must match the number of the unique keys.
 *
 * Returns: The memeber matched on success, or %PURC_VARIANT_INVALID
 * if the set does not managed by the unique keys, or there is no any matched
 * member.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_set_get_member_by_key_values(purc_variant_t set,
        purc_variant_t v1, ...);

/**
 * purc_variant_set_remove_member_by_key_values:
 *
 * @set: A set variant. The set should be managed by unique keys.
 * @v1: The first value for the first unique key.
 * @...: The values for the other unique keys.
 *
 * Removes the member by the values of unique keys from a set.
 * The caller should pass one value for each unique key.
 * The number of the matching values must match the number of the unique keys.
 *
 * Returns: %true on success, or %false if the set does not managed by
 * unique keys, or there is no any matched member.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_set_remove_member_by_key_values(purc_variant_t set,
        purc_variant_t v1, ...);

/**
 * purc_variant_set_unique_keys:
 *
 * @set: A set variant.
 * @sz: The pointer to a string pointer buffer to receive the pointer to
 *  the unique keys of this set.
 *
 * Gets the unique keys of a set variant.
 *
 * Returns: %true on success, otherwise %false (if the variant is not a set).
 *
 * Since: 0.9.8
 */
PCA_EXPORT bool
purc_variant_set_unique_keys(purc_variant_t set, const char **unique_keys);

/**
 * purc_variant_set_size:
 *
 * @set: A set variant.
 * @sz: The pointer to a size_t buffer to receive the size of the set variant.
 *
 * Gets the size (the number of members) of a set variant.
 *
 * Returns: %true on success, otherwise %false (if the variant is not a set).
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_set_size(purc_variant_t set, size_t *sz);

/**
 * purc_variant_set_get_size:
 *
 * @set: An set variant.
 *
 * Gets the size (the number of members) of a set variant.
 *
 * Returns: The size (the number of members) of the set variant.
 *  %PURC_VARIANT_BADSIZE (-1) if the variant is not a set.
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
 * purc_variant_set_get_by_index:
 *
 * @set: A set variant.
 * @idx: The index of the desired member.
 *
 * Gets a member of a set by index.
 *
 * Returns: A variant on success, or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_set_get_by_index(purc_variant_t set, size_t idx);

/**
 * purc_variant_set_remove_by_index:
 *
 * @set: A set variant.
 * @idx: The index of the member to be removed.
 *
 * Removes a member of the given set by index.
 *
 * Returns: The variant removed at the index or %PURC_VARIANT_INVALID
 *  if failed. Note that you need to un-reference the returned variant
 *  in order to avoid memory leak.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_set_remove_by_index(purc_variant_t set, size_t idx);

/**
 * purc_variant_set_set_by_index:
 *
 * @set: A set variant.
 * @idx: The index of the member to be replaced.
 * @val: The new variant which will replace the old one.
 *
 * Sets a member of the given set by index.
 *
 * Returns: A boolean that indicates if it succeeds or not.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_set_set_by_index(purc_variant_t set,
        size_t idx, purc_variant_t val);

/**
 * purc_variant_set_unite:
 *
 * @set: The destination set variant.
 * @value: The value to be united.
 * @cr_method: The method to resolve the conflict, can be one of the following
 *  values:
 *      - PCVRNT_CR_METHOD_IGNORE:
 *        Ignore the conflict and continue.
 *      - PCVRNT_CR_METHOD_OVERWRITE:
 *        Overwrite the member in the destination set.
 *      - PCVRNT_CR_METHOD_COMPLAIN:
 *        Report %PURC_ERROR_DUPLICATED error.
 *
 * Unites a variant or members in a linear container to the destination set.
 *
 * Returns: The number of members in the destination set after uniting,
 *  -1 for error.
 *
 * Since: 0.0.5
 */
PCA_EXPORT ssize_t
purc_variant_set_unite(purc_variant_t set, purc_variant_t value,
            pcvrnt_cr_method_k cr_method);

/**
 * purc_variant_set_intersect:
 *
 * @set: The destination set variant.
 * @value: The variant to intersect.
 *
 * Intersects @value with the given set, that is, only keeps the members which
 * match @value or any member in @value if @value is a linear container.
 *
 * Returns: The number of members of the set after intersecting,
 *  -1 on error.
 *
 * Since: 0.0.5
 */
PCA_EXPORT ssize_t
purc_variant_set_intersect(purc_variant_t set, purc_variant_t value);

/**
 * purc_variant_set_subtract:
 *
 * @set: The destination set variant.
 * @value: The variant to substract.
 *
 * Subtracts @value from the given set.
 * If @value is a linear container, it will try to find each member of
 * @value in the set. If there is any member in the set matched
 * the variant to find, the member will be removed from the set.
 *
 * Returns: The number of members of the set after subtracting,
 *  -1 on error.
 *
 * Since: 0.0.5
 */
PCA_EXPORT ssize_t
purc_variant_set_subtract(purc_variant_t set, purc_variant_t value);

/**
 * purc_variant_set_xor:
 *
 * @set: The destination set variant.
 * @value: The value to XOR.
 *
 * Does XOR operation on the set.
 *
 * Returns: The number of members of the set after the operation,
 *  -1 on error.
 *
 * Since: 0.0.5
 */
PCA_EXPORT ssize_t
purc_variant_set_xor(purc_variant_t set, purc_variant_t value);

/**
 * purc_variant_set_overwrite:
 *
 * @set: The destination set variant to overwrite.
 * @value: The source variant.
 * @nr_method: The method to resolve the not-found error if there is no member
 *  matched the value in the set.
 *      - PCVRNT_NR_METHOD_IGNORE:
 *        Ignore the conflict and go on.
 *      - PCVRNT_NR_METHOD_COMPLAIN:
 *        Report %PCVRNT_ERROR_NOT_FOUND error.
 *
 * Overwrites the members in the given set with @value or members in
 * @value if @value is a linear container.
 *
 * Returns: The number of changed members of the set, -1 on error.
 *
 * Since: 0.0.5
 */
PCA_EXPORT ssize_t
purc_variant_set_overwrite(purc_variant_t set, purc_variant_t value,
        pcvrnt_nr_method_k nr_method);

/**
 * struct pcvrnt_set_iterator:
 *
 * The iterator for set variant; Usage example:
 *
 * purc_variant_t obj;
 * ...
 * pcvrnt_set_iterator* it = pcvrnt_set_iterator_create_begin(obj);
 * while (it) {
 *     purc_variant_t  val = pcvrnt_set_iterator_get_value(it);
 *     ...
 *     bool having = pcvrnt_set_iterator_next(it);
 *     // behavior of accessing `val`/`key` is un-defined
 *     if (!having) {
 *         // behavior of accessing `it` is un-defined
 *         break;
 *     }
 * }
 * if (it)
 *     pcvrnt_set_iterator_release(it);
 */

struct pcvrnt_set_iterator;

/**
 * pcvrnt_set_iterator_create_begin:
 *
 * @set: A set variant.
 *
 * Creates a new beginning iterator for the set variant @set.
 * The returned iterator will point to the first member in the set.
 *
 * Note that a new iterator will hold a reference of the set, until it is
 * released by calling pcvrnt_set_iterator_release(). It will also
 * hold a reference of the member it points to, until it was moved to
 * another one.
 *
 * Returns: The iterator for the set; %NULL if there is no member in the set.
 *
 * Since: 0.0.1
 */
PCA_EXPORT struct pcvrnt_set_iterator *
pcvrnt_set_iterator_create_begin(purc_variant_t set);

/**
 * pcvrnt_set_iterator_create_end:
 *
 * Creates a new end iterator for the set variant @set.
 * The returned iterator will point to the last member in the set.
 *
 * Note that a new iterator will hold a reference of the set, until it is
 * released by calling pcvrnt_set_iterator_release(). It will also
 * hold a reference of the member it points to, until it was moved to
 * another one.
 *
 * Returns: The iterator for the set; %NULL if there is no member in the set.
 *
 * Since: 0.0.1
 */
PCA_EXPORT struct pcvrnt_set_iterator *
pcvrnt_set_iterator_create_end(purc_variant_t set);

/**
 * pcvrnt_set_iterator_release:
 *
 * @it: The iterator of a set variant.
 *
 * Releases the set iterator (@it). The reference count of the set
 * and the member (if any) pointed to by @it will be decremented.
 *
 * Returns: None.
 *
 * Since: 0.0.1
 */
PCA_EXPORT void
pcvrnt_set_iterator_release(struct pcvrnt_set_iterator* it);

/**
 * pcvrnt_set_iterator_next:
 *
 * @it: The iterator of a set variant.
 *
 * Forwards the iterator to point to the next member in the set.
 * Note that the iterator will release the reference of the former member,
 * and hold a new reference to the next member (if any).
 *
 * Returns: %true if success, @false if there is no subsequent member.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
pcvrnt_set_iterator_next(struct pcvrnt_set_iterator* it);

/**
 * pcvrnt_set_iterator_prev:
 *
 * @it: The iterator of a set variant.
 *
 * Backwards the iterator to point to the previous member in the set.
 * Note that the iterator will release the reference of the former member
 * it pointed to, and hold a new reference to the prevoius member (if any).
 *
 * Returns: %true if success, @false if there is no preceding member.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
pcvrnt_set_iterator_prev(struct pcvrnt_set_iterator* it);

/**
 * pcvrnt_set_iterator_get_value:
 *
 * @it: The iterator of a set variant.
 *
 * Gets the value of the member to which the iterator points.
 *
 * Returns: The variant of the current member.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
pcvrnt_set_iterator_get_value(struct pcvrnt_set_iterator* it);

/**
 * purc_variant_make_tuple:
 *
 * @sz: The size of the tuple, i.e., the number of members in the tuple.
 * @members: A C array of the members to put into the tuple.
 *
 * Creates a tuple variant from variants.
 *
 * The function will set the left members as %null variants after
 * it encountered an invalud variant (%PURC_VARIANT_INVALID) in @members.
 * You can call purc_variant_tuple_set() to set the left members with other
 * variants.
 *
 * Note that if @members is %NULL, all members in the tuple will be
 * %null variants initially.
 *
 * Returns: A tuple variant on success, or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.1.0
 */
PCA_EXPORT purc_variant_t
purc_variant_make_tuple(size_t sz, purc_variant_t *members);

/**
 * purc_variant_make_tuple_0:
 *
 * Creates an empty tuple variant.
 *
 * Returns: A tuple variant on success, or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.2.0
 */
static inline purc_variant_t
purc_variant_make_tuple_0(void)
{
    return purc_variant_make_tuple(0, NULL);
}

/**
 * purc_variant_tuple_size:
 *
 * @tuple: A tuple variant.
 * @sz: The pointer to a size_t buffer to receive the size of the tuple.
 *
 * Gets the size (the number of the members) of a tuple variant.
 *
 * Returns: %true when success, or %false on failure. When it returns %false,
 *  the value contained in the buffer pointed to by @sz is undefined.
 *
 * Since: 0.1.0
 */
PCA_EXPORT bool
purc_variant_tuple_size(purc_variant_t tuple, size_t *sz);

/**
 * purc_variant_tuple_get_size:
 *
 * @tuple: A tuple variant.
 *
 * Gets the size (the number of the members) of a tuple variant.
 *
 * Returns: The size (the number of members) of the tuple;
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
 * purc_variant_tuple_get:
 *
 * @tuple: A tuple variant.
 * @idx: The index of wanted member.
 *
 * Gets a member from a tuple by index.
 *
 * Returns: A variant on success, or % PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.1.0
 */
PCA_EXPORT purc_variant_t
purc_variant_tuple_get(purc_variant_t tuple, size_t idx);

/**
 * purc_variant_tuple_set:
 *
 * @tuple: A tuple variant.
 * @idx: The index of the member to replace.
 * @value: The new variant of the member.
 *
 * Sets (replaces) a member in a tuple by index.
 * This function will decrement the reference count of the old variant,
 * and increment the reference count of the new variant.
 *
 * Returns: %true on success, otherwise %false.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_tuple_set(purc_variant_t tuple, size_t idx, purc_variant_t value);

#define PCVRNT_SAFLAG_ASC            0x0000
#define PCVRNT_SAFLAG_DESC           0x0001
#define PCVRNT_SAFLAG_DEFAULT        0x0000

typedef int (*pcvrnt_compare_cb)(purc_variant_t v1, purc_variant_t v2);

/**
 * purc_variant_make_sorted_array:
 *
 * @flags: The flags indicating the order and other options.
 *      Use %PCVRNT_SAFLAG_ASC for ascending order and %PCVRNT_SAFLAG_DESC
 *      for descending order.
 * @sz_init: The initial size allocated for the sorted array.
 * @cmp: The callback function to compare two variants.
 *
 * Creates an empty sorted array variant.
 * Note that, currently, the sorted array variant is implementd as a native
 * entity, not an inherent variant type.
 *
 * Returns: A sorted array variant on success,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.9.2
 */
PCA_EXPORT purc_variant_t
purc_variant_make_sorted_array(unsigned int flags, size_t sz_init,
        pcvrnt_compare_cb cmp);

/**
 * purc_variant_sorted_array_add:
 *
 * @array: A sorted array variant returned by purc_variant_make_sorted_array().
 * @value: The variant will be added to the array.
 *
 * Adds a new variant to the given sorted array. Note that, currently,
 * duplicate values are not allowed.
 *
 * Returns: A ssize_t indicating the index of the new member, -1 for failure.
 *
 * Since: 0.9.2
 */
PCA_EXPORT ssize_t
purc_variant_sorted_array_add(purc_variant_t array, purc_variant_t value);

/**
 * purc_variant_sorted_array_remove:
 *
 * @array: A sorted array variant returned by purc_variant_make_sorted_array().
 * @value: The variant will be used to match a member in the sorted array.
 *
 * Removes a member which is equal to the given variant in value from
 * the specified sorted array.
 *
 * Returns: %true on success, %false on failure.
 *
 * Since: 0.9.2
 */
PCA_EXPORT bool
purc_variant_sorted_array_remove(purc_variant_t array, purc_variant_t value);

/**
 * purc_variant_sorted_array_delete:
 *
 * @array: A sorted array variant returned by purc_variant_make_sorted_array().
 * @idx: The index of the member will be deleted.
 *
 * Deletes a member by index from the specified sorted array.
 *
 * Returns: %true on success, %false on failure.
 *
 * Since: 0.9.2
 */
PCA_EXPORT bool
purc_variant_sorted_array_delete(purc_variant_t array, size_t idx);

/**
 * purc_variant_sorted_array_find:
 *
 * @array: A sorted array variant returned by purc_variant_make_sorted_array().
 * @value: The variant will be used to match a member in the sorted array.
 *
 * Checks whether there is a member which is equal to the given variant
 * in value in the specified sorted array and returns the index the member.
 *
 * Returns: A ssize_t indicating the index of the found member, -1 for not found.
 *
 * Since: 0.9.2
 */
PCA_EXPORT ssize_t
purc_variant_sorted_array_find(purc_variant_t array, purc_variant_t value);

/**
 * purc_variant_sorted_array_get:
 *
 * @array: A sorted array variant returned by purc_variant_make_sorted_array().
 * @idx: The index of the desired member.
 *
 * Gets the member in the given sorted array by index.
 *
 * Returns: A variant on success, or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.9.2
 */
PCA_EXPORT purc_variant_t
purc_variant_sorted_array_get(purc_variant_t array, size_t idx);

/**
 * purc_variant_sorted_array_size:
 *
 * @array: A sorted array variant returned by purc_variant_make_sorted_array().
 * @sz: The pointer to a size_t buffer to receive the size of the sorted array.
 *
 * Gets the size (the number of members) of the given sorted array.
 *
 * Returns: %true when success, or %false on failure. When it returns %false,
 *  the value contained in the buffer pointed to by @sz is undefined.
 *
 * Since: 0.9.2
 */
PCA_EXPORT bool
purc_variant_sorted_array_size(purc_variant_t array, size_t *sz);

/**
 * purc_variant_sorted_array_get_size:
 *
 * @array: A sorted array variant returned by purc_variant_make_sorted_array().
 *
 * Gets the size (the number of members) of the given sorted array.
 *
 * Returns: The size (the number of members) of the sorted array;
 *  \PURC_VARIANT_BADSIZE (-1) if the variant is not a softed array.
 *
 * Since: 0.9.2
 */
static inline ssize_t purc_variant_sorted_array_get_size(purc_variant_t array)
{
    size_t sz;
    if (!purc_variant_sorted_array_size(array, &sz))
        return PURC_VARIANT_BADSIZE;
    return sz;
}

/**
 * purc_variant_linear_container_size:
 *
 * @container: A valid linear container variant, must be an array,
 *      a set, or a tuple variant.
 * @sz: The pointer to a size_t buffer to receive the size of the container.
 *
 * Gets the size (the number of members) of a linear container variant.
 *
 * Returns: %true on success, otherwise %false.
 *
 * Since: 0.1.0
 */
PCA_EXPORT bool
purc_variant_linear_container_size(purc_variant_t container, size_t *sz);

/**
 * purc_variant_linear_container_get_size:
 *
 * @container: A valid linear container variant, must be an array,
 *      a set, or a tuple variant.
 *
 * Gets the size (the number of members) of a linear container variant,
 * and returns the size directly.
 *
 * Returns: The size of the container on success;
 *  \PURC_VARIANT_BADSIZE (-1) if the variant is not a linear container.
 *
 * Since: 0.1.0
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
 * purc_variant_linear_container_get:
 *
 * @container: A valid linear container variant, must be an array,
 *      a set, or a tuple variant.
 * @idx: The index of wanted member.
 *
 * Gets a member from a linear container by index.
 *
 * Returns: A variant on success, or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.1.0
 */
PCA_EXPORT purc_variant_t
purc_variant_linear_container_get(purc_variant_t container, size_t idx);

/**
 * purc_variant_linear_container_set:
 *
 * @container: A valid linear container variant, must be an array,
 *      a set, or a tuple variant.
 * @idx: The index of the member to set.
 * @value: The new variant.
 *
 * Sets the member in the linear container (@container) by index (@idx)
 * with @value.
 * Note that the reference count of @value will be incremented, and
 * the reference count of the old value of the member will be decremented.
 *
 * Returns: %true on success, otherwise %false.
 *
 * Since: 0.1.0
 */
PCA_EXPORT bool
purc_variant_linear_container_set(purc_variant_t container,
        size_t idx, purc_variant_t value);

/**
 * purc_variant_make_from_json_string:
 *
 * @json: The pointer to a string which contains a valid eJSON data.
 * @sz: The size of string.
 *
 * Creates a variant from a string which contains a valid eJSON data.
 *
 * Returns: A variant on success, or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_make_from_json_string(const char* json, size_t sz);

/**
 * purc_variant_load_from_json_file:
 *
 * @file: The pointer to a null-terminated string which specify the file name.
 *
 * Creates a variant from a file which contains valid JSON data.
 *
 * Returns: A variant on success, or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_load_from_json_file(const char* file);

/**
 * purc_variant_load_from_json_stream:
 *
 * @stream: A purc_rwstream_t stream.
 *
 * Creates a variant from a stream which contains valid JSON data.
 *
 * Returns: A variant on success, or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_load_from_json_stream(purc_rwstream_t stream);

/**
 * purc_variant_cast_to_int32:
 *
 * @v: A variant value.
 * @i32: A pointer to an int32_t buffer to receive the casted integer.
 * @force: A boolean indicates whether to force casting, e.g.,
 *      parsing a string or returning an integer value for a null variant
 *      or a boolean variant.
 *
 * Tries to cast a variant value to a signed 32-bit integer.
 *
 * Returns: %true on success, or %false on failure (the variant value can not
 *      be casted to a 32-bit integer).
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_cast_to_int32(purc_variant_t v, int32_t *i32, bool force);

/**
 * purc_variant_cast_to_uint32:
 *
 * @v: A variant value.
 * @u32: A pointer to a uint32_t buffer to receive the casted integer.
 * @force: A boolean indicates whether to force casting, e.g.,
 *      parsing a string or returning a integer value for a null variant
 *      or a boolean variant.
 *
 * Tries to cast a variant value to an unsigned 32-bit integer.
 *
 * Returns: %true on success, or %false on failure (the variant value can not
 *      be casted to an unsigned 32-bit integer).
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_cast_to_uint32(purc_variant_t v, uint32_t *u32, bool force);

/**
 * purc_variant_cast_to_longint:
 *
 * @v: A variant value.
 * @i64: The pointer to an int64_t buffer to receive the casted long integer.
 * @force: A boolean indicates whether to force casting, e.g.,
 *      parsing a string or returning a long integer value for a null variant
 *      or a boolean variant.
 *
 * Tries to cast a variant value to a long integer.
 *
 * Returns: %true on success, or %false on failure (the variant value can not
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
 * purc_variant_cast_to_ulongint:
 *
 * @v: A variant value.
 * @u64: The pointer to a uint64_t buffer to receive the casted unsigned
 *      long integer.
 * @force: A boolean indicates whether to force casting, e.g.,
 *      parsing a string or returning a long integer value for a null variant
 *      or a boolean variant.
 *
 * Tries to cast a variant value to an unsigned long integer.
 *
 * Returns: %true on success, or %false on failure (the variant value can not
 *      be casted to an unsigned long integer).
 *
 * Note: A negative number, long integer, or long double will be casted
 *      to 0 when @force is %true.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_cast_to_ulongint(purc_variant_t v, uint64_t *u64, bool force);

/**
 * purc_variant_cast_to_number:
 *
 * @v: A variant value.
 * @d: The pointer to a double buffer to receive the casted number.
 * @force: A boolean indicates whether to force casting, e.g.,
 *      parsing a string or returning a double value for a null variant
 *      or a boolean variant.
 *
 * Tries to cast a variant value to a nubmer.
 *
 * Note: A number, a long integer, an unsigned long integer, or a long double
 *      can always be casted to a number (a double value).
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_cast_to_number(purc_variant_t v, double *d, bool force);

/**
 * purc_variant_cast_to_longdouble:
 *
 * @v: A variant value.
 * @ld: The pointer to a long double buffer to receive the casted long double
 *      value.
 * @force: A boolean indicates whether to force casting, e.g.,
 *      parsing a string or returning a long integer value for a null variant
 *      or a boolean variant.
 *
 * Tries to cast a variant value to a long double float number.
 *
 * Returns: %true on success, or %false on failure (the variant value can not
 *      be casted to a long double value).
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
 * purc_variant_cast_to_byte_sequence:
 *
 * @v: A variant value.
 * @bytes: A pointer to a pointer buffer to receive the pointer to
 *      the byte sequence.
 * @sz: A pointer to a size_t buffer to receive the size of the byte sequence
 *      in bytes.
 *
 * Tries to cast a variant value to a byte sequence.
 *
 * Returns: %true on success, or %false on failure (the variant value can not
 *      be casted to a byte sequence).
 *
 * Note: Only a string, an atom string, an exception, or a byte sequence can
 *      be casted to a byte sequence.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_cast_to_byte_sequence(purc_variant_t v,
        const void **bytes, size_t *sz);

/**
 * purc_variant_is_equal_to:
 *
 * @v1: one variant.
 * @v2: another variant.
 *
 * Checks one variant is equal to another one exactly or not.
 *
 * Returns: The function returns a boolean indicating whether @v1 is equal
 *  to @v2 exactly.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_is_equal_to(purc_variant_t v1, purc_variant_t v2);

typedef enum pcvrnt_compare_method {
    PCVRNT_COMPARE_METHOD_AUTO,
    PCVRNT_COMPARE_METHOD_NUMBER,
    PCVRNT_COMPARE_METHOD_CASE,
    PCVRNT_COMPARE_METHOD_CASELESS,
} pcvrnt_compare_method_k;

/**
 * purc_variant_compare_ex:
 *
 * @v1: One variant.
 * @v2: Another variant.
 * @method: The comparison method. It can be one of the following values:
 *      - PCVRNT_COMPARE_METHOD_NUMBER:
 *          Compares two variants as they are numbers. The variants may be
 *          numerified first before comparing.
 *      - PCVRNT_COMPARE_METHOD_CASE:
 *          Compares two variants as they are strings and case-sensitively.
 *          The variants may be stringified first before comparing.
 *      - PCVRNT_COMPARE_METHOD_CASELESS:
 *          Compares two variants as they are strings and case-insensitively.
 *          The variants may be stringified first before comparing.
 *      - PCVRNT_COMPARE_METHOD_AUTO:
 *          Compares two variants automatially. The exact method used to
 *          compare them is determined by the type of the first variant.
 *          If the first variant is a number, this function will numerify
 *          the second variant and compare them as they are numbers.
 *          Otherwise, this function compares them as they are strings
 *          and case-sensitively.
 *
 * Compares two variants in the specified method.
 *
 * Returns: The function returns an integer less than, equal to, or greater
 *      than zero if @v1 is found, respectively, to be less than, to match,
 *      or be greater than @v2.
 *
 * Since: 0.0.1
 */
PCA_EXPORT int
purc_variant_compare_ex(purc_variant_t v1, purc_variant_t v2,
        pcvrnt_compare_method_k method);

/**
 * A flag for the purc_variant_serialize() function which serializes
 * all real numbers as JSON numbers.
 */
#define PCVRNT_SERIALIZE_OPT_REAL_JSON               0x00000000

/**
 * A flag for the purc_variant_serialize() function which serializes
 * a real numbers by using eJSON notation.
 */
#define PCVRNT_SERIALIZE_OPT_REAL_EJSON              0x00000001

/**
 * A flag for the purc_variant_serialize() function which serializes
 * all runtime types (undefined, dynamic, and native) as JSON null.
 */
#define PCVRNT_SERIALIZE_OPT_RUNTIME_NULL            0x00000000

/**
 * A flag for the purc_variant_serialize() function which serializes
 * all runtime types (undefined, dynamic, and native) as placeholders
 * in JSON strings.
 */
#define PCVRNT_SERIALIZE_OPT_RUNTIME_STRING          0x00000002

/**
 * A flag for the purc_variant_serialize() function which causes
 * the output to drop trailing zero for float values.
 */
#define PCVRNT_SERIALIZE_OPT_NOZERO                  0x00000004

/**
 * A flag for the purc_variant_serialize() function which causes
 * the output to not escape the forward slashes ('/').
 */
#define PCVRNT_SERIALIZE_OPT_NOSLASHESCAPE           0x00000008

/**
 * A flag for the purc_variant_serialize() function which
 * causes the output to have no extra whitespace or formatting applied.
 */
#define PCVRNT_SERIALIZE_OPT_PLAIN                   0x00000000

/**
 * A flag for the purc_variant_serialize() function which causes
 * the output to have minimal whitespace inserted to make things slightly
 * more readable.
 */
#define PCVRNT_SERIALIZE_OPT_SPACED                  0x00000010

/**
 * A flag for the purc_variant_serialize() function which causes
 * the output to be formatted by using "Two Space Tab".
 *
 * See the "Two Space Tab" option at <http://jsonformatter.curiousconcept.com>
 * for an example of the format.
 */
#define PCVRNT_SERIALIZE_OPT_PRETTY                  0x00000020

/**
 * A flag for the purc_variant_serialize() function which causes
 * the output to be formatted by using a single tab character instead of
 * "Two Space Tab".
 */
#define PCVRNT_SERIALIZE_OPT_PRETTY_TAB              0x00000040

#define PCVRNT_SERIALIZE_OPT_BSEQUENCE_MASK          0x00000F00

/**
 * A flag for the purc_variant_serialize() function which causes
 * the function serializes byte sequences as a hexadecimal string.
 */
#define PCVRNT_SERIALIZE_OPT_BSEQUENCE_HEX_STRING    0x00000000

/**
 * A flag for the purc_variant_serialize() function which causes
 * the output to use hexadecimal characters for byte sequence.
 */
#define PCVRNT_SERIALIZE_OPT_BSEQUENCE_HEX           0x00000100

/**
 * A flag for the purc_variant_serialize() function which causes
 * the output to use binary characters for byte sequence.
 */
#define PCVRNT_SERIALIZE_OPT_BSEQUENCE_BIN           0x00000200

/**
 * A flag for the purc_variant_serialize() function which causes
 * the output to use BASE64 encoding for byte sequence.
 */
#define PCVRNT_SERIALIZE_OPT_BSEQUENCE_BASE64        0x00000300

/**
 * A flag for the purc_variant_serialize() function which causes
 * the output to have dot for binary sequence.
 */
#define PCVRNT_SERIALIZE_OPT_BSEQUENCE_BIN_DOT       0x00000400

/**
 * A flag for the purc_variant_serialize() function which causes
 * the output to print unique keys of a set.
 */
#define PCVRNT_SERIALIZE_OPT_UNIQKEYS                0x00001000

/**
 * A flag for the purc_variant_serialize() function which serializes
 * a tuple by using eJSON notation.
 */
#define PCVRNT_SERIALIZE_OPT_TUPLE_EJSON             0x00002000

/**
 * A flag for the purc_variant_serialize() function which causes
 * the output to use hexadecimal digits for bigint.
 */
#define PCVRNT_SERIALIZE_OPT_BIGINT_HEX              0x00004000

/**
 * A flag for the purc_variant_serialize() function which causes
 * the function ignores the output errors.
 */
#define PCVRNT_SERIALIZE_OPT_IGNORE_ERRORS           0x10000000

/**
 * purc_variant_serialize:
 *
 * @value: A variant value to be serialized.
 * @stream: A stream to which the serialized data write.
 * @indent_level: The initial indent level. 0 for most cases.
 * @flags: The serialization flags.
 * @len_expected: The pointer to a size_t buffer to receive the expected
 *      length of the whole serialized data (nullable). The value in the buffer
 *      should be set to 0 initially.
 *
 * Serializes a variant value to a purc_rwstream_t object in the given flags.
 *
 * Returns: The size of the serialized data written to the stream;
 * On error, -1 is returned, and error code is set to indicate
 * the cause of the error.
 *
 * If the function is called with the flag
 * %PCVRNT_SERIALIZE_OPT_IGNORE_ERRORS set, this function always
 * returned the number of bytes written to the stream actually.
 * Meanwhile, if @len_expected is not null, the expected length of
 * the serialized data will be returned through this buffer.
 *
 * Therefore, you can prepare a small memory stream with the flag
 * %PCVRNT_SERIALIZE_OPT_IGNORE_ERRORS set to count the
 * expected length of the whole serialized data.
 *
 * Since: 0.0.1
 */
PCA_EXPORT ssize_t
purc_variant_serialize(purc_variant_t value, purc_rwstream_t stream,
        int indent_level, unsigned int flags, size_t *len_expected);

/**
 * purc_variant_serialize_alloc:
 *
 * @value: A variant value to be serialized.
 * @indent_level: The initial indent level. 0 for most cases.
 * @flags: The serialization flags.
 * @sz_content (nullable): A pointer to a buffer to receive the length
 *  of serialized content.
 * @sz_buffer (nullable): A pointer to a buffer to receive the size
 *  of the new buffer.
 *
 * Serializes a variant value to a newly allocated buffer in the given flags.
 *
 * Returns: The pointer to the buffer or NULL on error. The caller is
 *  responsible to free the buffer by calling free(2).
 *
 * Since: 0.9.26
 */
PCA_EXPORT char *
purc_variant_serialize_alloc(purc_variant_t value, int indent_level,
        unsigned flags, size_t *sz_content, size_t *sz_buffer);

#define PURC_ENVV_DVOBJS_PATH   "PURC_DVOBJS_PATH"

/**
 * purc_variant_load_dvobj_from_so:
 *
 * @so_name: The name of the shared library.
 * @var_name: The name of the dynamic variant to load.
 *
 * Loads a dynamic variant from the given shared library.
 *
 * Returns: A dynamic variant on success, or %PURC_VARIANT_INVALID on failure.
.*
 * Since: 0.0.1
 */
PCA_EXPORT purc_variant_t
purc_variant_load_dvobj_from_so(const char *so_name, const char *dvobj_name);

/**
 * purc_variant_unload_dvobj:
 *
 * @value: A dynamic variant returned by purc_variant_load_dvobj_from_so().
 *
 * Unloads a dynamic variant.
 *
 * Returns: %true for success, %false on failure.
.*
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_unload_dvobj(purc_variant_t dvobj);

typedef enum purc_variant_type {
    PURC_VARIANT_TYPE_FIRST = 0,

    /* XXX: keep consistency with type names */
#define PURC_VARIANT_TYPE_NAME_UNDEFINED    "undefined"
    PURC_VARIANT_TYPE_UNDEFINED = PURC_VARIANT_TYPE_FIRST,
#define PURC_VARIANT_TYPE_NAME_NULL         "null"
    PURC_VARIANT_TYPE_NULL,
#define PURC_VARIANT_TYPE_NAME_BOOLEAN      "boolean"
    PURC_VARIANT_TYPE_BOOLEAN,
#define PURC_VARIANT_TYPE_NAME_NUMBER       "number"
    PURC_VARIANT_TYPE_NUMBER,
#define PURC_VARIANT_TYPE_NAME_LONGINT      "longint"
    PURC_VARIANT_TYPE_LONGINT,
#define PURC_VARIANT_TYPE_NAME_ULONGINT     "ulongint"
    PURC_VARIANT_TYPE_ULONGINT,
#define PURC_VARIANT_TYPE_NAME_EXCEPTION    "exception"
    PURC_VARIANT_TYPE_EXCEPTION,
#define PURC_VARIANT_TYPE_NAME_ATOMSTRING   "atomstring"
    PURC_VARIANT_TYPE_ATOMSTRING,
#define PURC_VARIANT_TYPE_NAME_LONGDOUBLE   "longdouble"
    PURC_VARIANT_TYPE_LONGDOUBLE,
#define PURC_VARIANT_TYPE_NAME_BIGINT       "bigint"
    PURC_VARIANT_TYPE_BIGINT,

    /* the above types are considered as scalar variants:
       bit-width is LE 64, no extra size, and without change events. */
    PURC_VARIANT_TYPE_LAST_SCALAR = PURC_VARIANT_TYPE_BIGINT,

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

#define PURC_VARIANT_TYPE_DOUBLE            PURC_VARIANT_TYPE_NUMBER
#define PURC_VARIANT_TYPE_NR \
    (PURC_VARIANT_TYPE_LAST - PURC_VARIANT_TYPE_FIRST + 1)

/**
 * purc_variant_is_type:
 *
 * @value: A valid variant value.
 * @type: Desired type
 *
 * Checks whether the given variant belongs to the specified type.
 *
 * Returns: %true on success, otherwise %false.
 *
 * Since: 0.0.1
 */
PCA_EXPORT bool
purc_variant_is_type(purc_variant_t value, enum purc_variant_type type);

/**
 * purc_variant_get_type:
 *
 * @value: A valid variant value.
 *
 * Gets the type of a variant value.
 *
 * Returns: The type of the given variant.
 *
 * Since: 0.0.1
 */
PCA_EXPORT enum purc_variant_type
purc_variant_get_type(purc_variant_t value);

/**
 * purc_variant_typename:
 *
 * @type: A enumeration value which represents a variant type.
 *
 * Gets the type name (a static null-terminated string) of
 * the given variant type.
 *
 * Returns: The type name (a pointer to a static null-terminated string).
 *
 * Since: 0.1.0
 */
PCA_EXPORT const char*
purc_variant_typename(enum purc_variant_type type);

/**
 * purc_variant_is_undefined:
 *
 * @v: A valid variant.
 *
 * Checks whether the given variant is %undefined.
 *
 * Returns: %true if the given variant is %undefined, otherwise %false.
 *
 * Since: 0.1.0
 */
static inline bool purc_variant_is_undefined(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_UNDEFINED);
}

/**
 * purc_variant_is_null:
 *
 * @v: A valid variant.
 *
 * Checks whether the given variant is %null.
 *
 * Returns: %true if the given variant is %null, otherwise %false.
 *
 * Since: 0.1.0
 */
static inline bool purc_variant_is_null(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_NULL);
}

/**
 * purc_variant_is_boolean:
 *
 * @v: A valid variant.
 *
 * Checks whether the given variant is a boolean.
 *
 * Returns: %true if the given variant is a boolean, otherwise %false.
 *
 * Since: 0.1.0
 */
static inline bool purc_variant_is_boolean(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_BOOLEAN);
}

/**
 * purc_variant_is_exception:
 *
 * @v: A valid variant.
 *
 * Checks whether the given variant is an exception.
 *
 * Returns: %true if the given variant is an exception, otherwise %false.
 *
 * Since: 0.1.0
 */
static inline bool purc_variant_is_exception(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_EXCEPTION);
}

/**
 * purc_variant_is_number:
 *
 * @v: A valid variant.
 *
 * Checks whether the given variant is a number.
 *
 * Returns: %true if the given variant is a number, otherwise %false.
 *
 * Since: 0.1.0
 */
static inline bool purc_variant_is_number(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_NUMBER);
}

/**
 * purc_variant_is_longint:
 *
 * @v: A valid variant.
 *
 * Checks whether the given variant is a longint.
 *
 * Returns: %true if the given variant is a longint, otherwise %false.
 *
 * Since: 0.1.0
 */
static inline bool purc_variant_is_longint(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_LONGINT);
}

/**
 * purc_variant_is_ulongint:
 *
 * @v: A valid variant.
 *
 * Checks whether the given variant is a ulongint.
 *
 * Returns: %true if the given variant is a ulongint, otherwise %false.
 *
 * Since: 0.1.0
 */
static inline bool purc_variant_is_ulongint(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_ULONGINT);
}

/**
 * purc_variant_is_atomstring:
 *
 * @v: A valid variant.
 *
 * Checks whether the given variant is an atom.
 *
 * Returns: %true if the given variant is an atom, otherwise %false.
 *
 * Since: 0.1.0
 */
static inline bool purc_variant_is_atomstring(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_ATOMSTRING);
}

/**
 * purc_variant_is_longdouble:
 *
 * @v: A valid variant.
 *
 * Checks whether the given variant is a longdouble.
 *
 * Returns: %true if the given variant is a longdouble otherwise %false.
 *
 * Since: 0.1.0
 */
static inline bool purc_variant_is_longdouble(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_LONGDOUBLE);
}

/**
 * purc_variant_is_bigint:
 *
 * @v: A valid variant.
 *
 * Checks whether the given variant is a bigint.
 *
 * Returns: %true if the given variant is a bigint otherwise %false.
 *
 * Since: 0.9.26
 */
static inline bool purc_variant_is_bigint(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_BIGINT);
}

/**
 * purc_variant_is_string:
 *
 * @v: A valid variant.
 *
 * Checks whether the given variant is a string.
 *
 * Returns: %true if the given variant is a string, otherwise %false.
 *
 * Since: 0.1.0
 */
static inline bool purc_variant_is_string(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_STRING);
}

/**
 * purc_variant_is_bsequence:
 *
 * @v: A valid variant.
 *
 * Checks whether the given variant is a byte sequence.
 *
 * Returns: %true if the given variant is a byte sequence, otherwise %false.
 *
 * Since: 0.1.0
 */
static inline bool purc_variant_is_bsequence(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_BSEQUENCE);
}

/**
 * purc_variant_is_dynamic:
 *
 * @v: A valid variant.
 *
 * Checks whether the given variant is a dynamic property.
 *
 * Returns: %true if the given variant is a dynamic property, otherwise %false.
 *
 * Since: 0.1.0
 */
static inline bool purc_variant_is_dynamic(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_DYNAMIC);
}

/**
 * purc_variant_is_native:
 *
 * @v: A valid variant.
 *
 * Checks whether the given variant is a native entity.
 *
 * Returns: %true if the given variant is a native entity, otherwise %false.
 *
 * Since: 0.1.0
 */
static inline bool purc_variant_is_native(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_NATIVE);
}

/**
 * purc_variant_is_object:
 *
 * @v: A valid variant.
 *
 * Checks whether the given variant is an object.
 *
 * Returns: %true if the given variant is an object, otherwise %false.
 *
 * Since: 0.1.0
 */
static inline bool purc_variant_is_object(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_OBJECT);
}

/**
 * purc_variant_is_array:
 *
 * @v: A valid variant.
 *
 * Checks whether the given variant is an array.
 *
 * Returns: %true if the given variant is an array, otherwise %false.
 *
 * Since: 0.1.0
 */
static inline bool purc_variant_is_array(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_ARRAY);
}

/**
 * purc_variant_is_set:
 *
 * @v: A valid variant.
 *
 * Checks whether the given variant is a set.
 *
 * Returns: %true if the given variant is a set, otherwise %false.
 *
 * Since: 0.1.0
 */
static inline bool purc_variant_is_set(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_SET);
}

/**
 * purc_variant_is_tuple:
 *
 * @v: A valid variant.
 *
 * Checks whether the given variant is a tuple.
 *
 * Returns: %true if the given variant is a tuple, otherwise %false.
 *
 * Since: 0.1.0
 */
static inline bool purc_variant_is_tuple(purc_variant_t v)
{
    return purc_variant_is_type(v, PURC_VARIANT_TYPE_TUPLE);
}

/**
 * purc_variant_is_true:
 *
 * @v: A valid variant.
 *
 * Checks whether the given variant is a boolean with the value of %true.
 *
 * Returns: %true if the variant is a boolean with the value of %true,
 *  otherwise %false.
 *
 * Since: 0.1.0
 */
PCA_EXPORT bool
purc_variant_is_true(purc_variant_t v);

/**
 * purc_variant_is_false:
 *
 * @v: A valid variant.
 *
 * Checks whether the given variant is a boolean with the value of %false.
 *
 * Returns: %true if the variant is a boolean with the value of %false,
 *  otherwise %false.
 *
 * Since: 0.1.0
 */
PCA_EXPORT bool
purc_variant_is_false(purc_variant_t v);

/**
 * purc_variant_is_container:
 *
 * @v: A valid variant.
 *
 * Checks whether the given variant is a container, i.e.,
 * an array, an object, a set, or a tuple.
 *
 * Returns: A boolean value indicating if the variant is a container;
 *      %true for a container, %false for other.
 *
 * Since: 0.1.1
 */
PCA_EXPORT bool
purc_variant_is_container(purc_variant_t v);

struct purc_variant_stat {
    size_t nr_values[PURC_VARIANT_TYPE_NR];
    size_t sz_mem[PURC_VARIANT_TYPE_NR];
    size_t nr_total_values;
    size_t sz_total_mem;
    size_t nr_reserved_scalar, nr_reserved_vector;            // Since 0.9.26
    size_t nr_max_reserved_scalar, nr_max_reserved_vector;    // Since 0.9.26
};

/**
 * purc_variant_usage_stat:
 *
 * Gets statistic of the memory usage of variants.
 *
 * Returns: The read-only pointer to struct purc_variant_stat on success,
 *      otherwise %NULL.
 *
 * Since: 0.0.1
 */
PCA_EXPORT const struct purc_variant_stat *
purc_variant_usage_stat(void);

/**
 * purc_variant_numerify:
 *
 * @value: A variant.
 *
 * Numerifies a variant to double.
 *
 * Returns: A double number that is numerified from the given variant.
 *
 * Since: 0.0.3
 */
PCA_EXPORT double
purc_variant_numerify(purc_variant_t value);

/**
 * purc_variant_numerify_long:
 *
 * @value: A variant.
 *
 * Numerifies a variant to long double.
 *
 * Returns: A long double number that is numerified from the given variant.
 *
 * Since: 0.9.26
 */
PCA_EXPORT long double
purc_variant_numerify_long(purc_variant_t value);

/**
 * purc_variant_booleanize:
 *
 * @value: variant value to be operated
 *
 * Booleanizes a variant to boolean.
 *
 * Returns: A boolean value that is booleanized from the given variant.
 *
 * Since: 0.0.3
 */
PCA_EXPORT bool
purc_variant_booleanize(purc_variant_t value);

/**
 * purc_variant_stringify_buff:
 *
 * @buff: The pointer to a string buffer to store the result.
 * @sz_buff: The size of the pre-allocated buffer.
 * @value: The variant value to be stringified.
 *
 * Stringifies a variant value to a pre-allocated buffer.
 * This function is similar to snprintf().
 *
 * Returns: Totol number of result content in bytes that has been succesfully
 *      written or shall be written if the buffer is large enough,
 *      or -1 in case of other failure.
 *
 * Since: 0.0.3
 */
PCA_EXPORT ssize_t
purc_variant_stringify_buff(char *buff, size_t sz_buff, purc_variant_t value);

/**
 * purc_variant_stringify_alloc_ex:
 *
 * @strp: The pointer to a char * buffer to receive the pointer to
 *      the allocated space.
 * @value: The variant value to be stringified.
 * @sz_buff_p (nullable): The pointer to a size_t buffer to received the size of
 *  the buffer.
 *
 * Stringifies a variant value in the similar way as `asprintf` does.
 *
 * Returns: Totol number of result content in bytes that has been succesfully
 *      written or shall be written if the buffer is large enough,
 *      or -1 in case of other failure.
 *
 * Since: 0.9.22
 */
PCA_EXPORT ssize_t
purc_variant_stringify_alloc_ex(char **strp, purc_variant_t value,
        size_t *sz_buff);

/**
 * purc_variant_stringify_alloc:
 *
 * @strp: The pointer to a char * buffer to receive the pointer to
 *      the allocated space.
 * @value: The variant value to be stringified.
 *
 * Stringifies a variant value in the similar way as `asprintf` does.
 *
 * Returns: Totol number of result content in bytes that has been succesfully
 *      written or shall be written if the buffer is large enough,
 *      or -1 in case of other failure.
 *
 * Since: 0.0.3
 */
static inline ssize_t
purc_variant_stringify_alloc(char **strp, purc_variant_t value) {
    return purc_variant_stringify_alloc_ex(strp, value, NULL);
}

/**
 * A flag for the purc_variant_stringify() function which causes
 * the function ignores the output errors.
 */
#define PCVRNT_STRINGIFY_OPT_IGNORE_ERRORS           0x10000000

/**
 * A flag for the purc_variant_stringify() function which causes
 * the function stringifies byte sequences as bare bytes.
 */
#define PCVRNT_STRINGIFY_OPT_BSEQUENCE_BAREBYTES     0x00000100

/**
 * A flag for the purc_variant_stringify() function which causes
 * the function stringifies real numers as bare bytes.
 */
#define PCVRNT_STRINGIFY_OPT_REAL_BAREBYTES          0x00000200

/**
 * purc_variant_stringify:
 *
 * @stream: The stream to which the stringified data write.
 * @value: The variant value to be stringified.
 * @flags: The stringifing flags.
 * @len_expected: The buffer to receive the expected length of
 *      the stringified data (nullable). The value in the buffer should be
 *      set to 0 initially.
 *
 * Stringifies a variant value to a writable stream.
 *
 * If the function is called with the flag
 * %PCVRNT_STRINGIFY_OPT_IGNORE_ERRORS set, this function always
 * returned the number of bytes written to the stream actually.
 * Meanwhile, if @len_expected is not null, the expected length of
 * the stringified data will be returned through this buffer.
 *
 * Therefore, you can prepare a small memory stream with the flag
 * %PCVRNT_STRINGIFY_OPT_IGNORE_ERRORS set to count the
 * expected length of the stringified data.
 *
 * Returns: The size of the stringified data written to the stream;
 * On error, -1 is returned, and error code is set to indicate
 * the cause of the error.
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
    PCVAR_OPERATION_INFLATED     = (0x01 << 0),
    PCVAR_OPERATION_DEFLATED     = (0x01 << 1),
    PCVAR_OPERATION_MODIFIED     = (0x01 << 2),
    PCVAR_OPERATION_REFASCHILD   = (0x01 << 3),
    PCVAR_OPERATION_RELEASING    = (0x01 << 4),
    PCVAR_OPERATION_ALL          = ((0x01 << 5) - 1),
} pcvar_op_t;

typedef bool (*pcvar_op_handler) (
        purc_variant_t src,  // the source variant.
        pcvar_op_t op,       // the operation identifier.
        void *ctxt,          // the context stored when registering the handler.
        size_t nr_args,      // the number of the relevant child variants.
        purc_variant_t *argv // the array of all relevant child variants.
        );

/**
 * purc_variant_register_pre_listener:
 *
 * @v: The variant that is to be listened.
 * @op: The operations to be listened, can OR'd by the following values:
 *      - PCVAR_OPERATION_INFLATED:
 *        A new member will be added to the container.
 *      - PCVAR_OPERATION_DEFLATED:
 *        A member will be removed from the container.
 *      - PCVAR_OPERATION_MODIFIED:
 *        The contents of the container will change.
 *      - PCVAR_OPERATION_REFASCHILD:
 *        The variant will be referenced as a child of another container.
 * @handler: The callback that will be called upon the listened event is fired.
 * @ctxt: The context will be passed to the callback.
 *
 * Registers a pre-operation listener to a container.
 *
 * Returns: the registered listener; %NULL on error.
 *
 * Since: 0.0.5
 */
PCA_EXPORT struct pcvar_listener *
purc_variant_register_pre_listener(purc_variant_t v,
        pcvar_op_t op, pcvar_op_handler handler, void *ctxt);

/**
 * purc_variant_register_post_listener:
 *
 * @v: The variant that is to be listened, it must be a container.
 * @op: The operations to be listened, can OR'd by the following values:
 *      - PCVAR_OPERATION_INFLATED:
 *        A new member has been added to the container.
 *      - PCVAR_OPERATION_DEFLATED:
 *        A member has be removed from the container.
 *      - PCVAR_OPERATION_MODIFIED:
 *        The contents of the container have changed.
 *      - PCVAR_OPERATION_REFASCHILD:
 *        The variant was referenced as a child of another container.
 *      - PCVAR_OPERATION_RELEASING:
 *        The variant is being released.
 * @handler: The callback that will be called upon the listened event is fired.
 * @ctxt: The context will be passed to the callback.
 *
 * Registers a post-operation listener to a container.
 *
 * Returns: the registered listener; %NULL on error.
 *
 * Since: 0.0.5
 */
PCA_EXPORT struct pcvar_listener *
purc_variant_register_post_listener(purc_variant_t v,
        pcvar_op_t op, pcvar_op_handler handler, void *ctxt);

/**
 * purc_variant_revoke_listener:
 *
 * @v: The variant whose listener is to be revoked.
 * @listener: The listener that is to be revoked.
 *
 * Revokes a registered listener on the given container.
 *
 * Returns: a boolean that indicates if the operation succeeds or not.
 *
 * Since: 0.0.4
 */
PCA_EXPORT bool
purc_variant_revoke_listener(purc_variant_t v,
        struct pcvar_listener *listener);

/**
 * purc_variant_container_clone:
 *
 * @ctnr: The source container variant.
 *
 * Clones a container. However, this function does not clone the members
 * contained in this container; the cloned container will only hold a new
 * reference of the members.
 *
 * Returns: The cloned container variant on success,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.1.1
 */
PCA_EXPORT purc_variant_t
purc_variant_container_clone(purc_variant_t ctnr);

/**
 * purc_variant_container_clone_recursively:
 *
 * @ctnr: The source container variant.
 *
 * Recursively clones a container (deep clone).
 *
 * Returns: The deep-cloned container variant on success,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.1.1
 */
PCA_EXPORT purc_variant_t
purc_variant_container_clone_recursively(purc_variant_t ctnr);

struct purc_ejson_parsing_tree;

/**
 * purc_variant_ejson_parse_string:
 *
 * @str: The pointer to a string in UTF-8 enconding.
 * @sz: The maximal length in bytes to parse.
 *
 * Parses an eJSON in the string and returns an eJSON parsing tree.
 *
 * Returns: the pointer to an eJSON parsing tree on success, otherwise NULL.
 *
 * Since: 0.1.1
 */
PCA_EXPORT struct purc_ejson_parsing_tree *
purc_variant_ejson_parse_string(const char *ejson, size_t sz);

/**
 * purc_variant_ejson_parse_file:
 *
 * @fname: A null-terminated string which specifies the file name.
 *
 * Parses an eJSON in contained in the given file (@fname) and returns
 * the eJSON parsing tree.
 *
 * Note that the characters in the file should be encoded in UTF-8.
 *
 * Returns: The pointer to the eJSON parsing tree on success, otherwise NULL.
 *
 * Since: 0.1.1
 */
PCA_EXPORT struct purc_ejson_parsing_tree *
purc_variant_ejson_parse_file(const char *fname);

/**
 * purc_variant_ejson_parse_stream:
 *
 * @rws: A purc_rwstream_t stream.
 *
 * Parses an eJSON stream and returns an eJSON parsing tree.
 *
 * Returns: The pointer to an eJSON parsing tree on success, otherwise NULL.
 *
 * Since: 0.1.1
 */
PCA_EXPORT struct purc_ejson_parsing_tree *
purc_variant_ejson_parse_stream(purc_rwstream_t rws);

typedef purc_variant_t (*purc_cb_get_var)(void* ctxt, const char* name);

/**
 * purc_ejson_parsing_tree_evalute:
 *
 * @parse_tree: An eJSON parsing tree which will be evaluated.
 * @fn_get_var: A callback function which returns a variant
 *      for a given variable name.
 * @ctxt: The context will be passed to the callback when resolving a variable.
 * @silently: %true means ignoring errors which are not fatal.
 *
 * Evaluates an eJSON parsing tree within variables.
 *
 * Returns: A variant evaluated on success,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.1.1
 */
PCA_EXPORT purc_variant_t
purc_ejson_parsing_tree_evalute(struct purc_ejson_parsing_tree *parse_tree,
        purc_cb_get_var fn_get_var, void *ctxt, bool silently);

/**
 * purc_ejson_parsing_tree_destroy:
 *
 * @parse_tree: An eJSON parsing tree which will be destroyed.
 *
 * Destroies an eJSON parsing tree.
 *
 * Returns: A variant evaluated on success,
 *
 * Since: 0.1.1
 */
PCA_EXPORT void
purc_ejson_parsing_tree_destroy(struct purc_ejson_parsing_tree *parse_tree);

/**
 * purc_variant_operator_lt:
 *
 * @v1: The first variant.
 * @v2: The second variant.
 *
 * Perform the less-than comparison (@v1 < @v2) and
 * return the truth value of the result.
 *
 * Returns: @true if @v1 < @v2, otherwise @false.
 */
PCA_EXPORT bool
purc_variant_operator_lt(purc_variant_t v1, purc_variant_t v2);

/**
 * purc_variant_operator_le:
 *
 * @v1: The first variant.
 * @v2: The second variant.
 *
 * Perform the less-than or equal to comparison (@v1 <= @v2) and
 * return the truth value of the result.
 *
 * Returns: @true if @v1 <= @v2, otherwise @false.
 */
PCA_EXPORT bool
purc_variant_operator_le(purc_variant_t v1, purc_variant_t v2);

 /**
 * purc_variant_operator_eq:
 *
 * @v1: The first variant.
 * @v2: The second variant.
 *
 * Perform the equal to comparison (@v1 == @v2) and
 * return the truth value of the result.
 *
 * Returns: @true if @v1 == @v2, otherwise @false.
 */
PCA_EXPORT bool
purc_variant_operator_eq(purc_variant_t v1, purc_variant_t v2);

/**
 * purc_variant_operator_ne:
 *
 * @v1: The first variant.
 * @v2: The second variant.
 *
 * Perform the not equal to comparison and
 * return the truth value of the result.
 *
 * Returns: @true if @v1 != @v2, otherwise @false.
 */
PCA_EXPORT bool
purc_variant_operator_ne(purc_variant_t v1, purc_variant_t v2);

/**
 * purc_variant_operator_gt:
 *
 * @v1: The first variant.
 * @v2: The second variant.
 *
 * Perform the greater-than comparison (@v1 > @v2) and
 * return the truth value of the result.
 *
 * Returns: @true if @v1 > @v2, otherwise @false.
 */
PCA_EXPORT bool
purc_variant_operator_gt(purc_variant_t v1, purc_variant_t v2);

/**
 * purc_variant_operator_ge:
 *
 * @v1: The first variant.
 * @v2: The second variant.
 *
 * Perform the greater-than or equal to comparison (@v1 >= @v2) and
 * return the truth value of the result.
 *
 * Returns: @true if @v1 >= @v2, otherwise @false.
 */
PCA_EXPORT bool
purc_variant_operator_ge(purc_variant_t v1, purc_variant_t v2);

/**
 * purc_variant_operator_not:
 *
 * @v: The variant.
 *
 * Perform the logical negation (not @v) and return the negation of @v.
 *
 * Returns: @true if @v is not true, otherwise @false.
 */
PCA_EXPORT bool
purc_variant_operator_not(purc_variant_t v);

/**
 * purc_variant_operator_truth:
 *
 * @v: The variant.
 *
 * Perform the truth test (bool(@v)) and return the truth value of @v.
 *
 * Returns: @true if @v is true, otherwise @false.
 */
PCA_EXPORT bool
purc_variant_operator_truth(purc_variant_t v);

/**
 * purc_variant_operator_is:
 *
 * @v1: The first variant.
 * @v2: The second variant.
 *
 * Perform the object identity test (@v1 is @v2) and return the truth value
 * of the result.
 *
 * Returns: @true if @v1 is @v2, otherwise @false.
 */
PCA_EXPORT bool
purc_variant_operator_is(purc_variant_t v1, purc_variant_t v2);

/**
 * purc_variant_operator_is_not:
 *
 * @v1: The first variant.
 * @v2: The second variant.
 *
 * Perform the object identity test (@v1 is not @v2) and return the negation
 * of the result.
 *
 * Returns: @true if @v1 is not @v2, otherwise @false.
 */
PCA_EXPORT bool
purc_variant_operator_is_not(purc_variant_t v1, purc_variant_t v2);

/**
 * purc_variant_operator_abs:
 *
 * @v: The variant.
 *
 * Perform the absolute value operation (abs(@v)) and return
 * the absolute value of @v.
 *
 * Returns: A variant evaluated on success,
 *      or %PURC_VARIANT_INVALID on failure.
 */
PCA_EXPORT purc_variant_t
purc_variant_operator_abs(purc_variant_t v);

/**
 * purc_variant_operator_neg:
 *
 * @v: The variant.
 *
 * Perform the negation operation (-@v) and return the negation of @v.
 *
 * Returns: A variant evaluated on success,
 *      or %PURC_VARIANT_INVALID on failure.
 */
PCA_EXPORT purc_variant_t
purc_variant_operator_neg(purc_variant_t v);

/**
 * purc_variant_operator_pos:
 *
 * @v: The variant.
 *
 * Perform the positive operation (+@v) and return the positive of @v.
 *
 * Returns: A variant evaluated on success,
 *      or %PURC_VARIANT_INVALID on failure.
 */
PCA_EXPORT purc_variant_t
purc_variant_operator_pos(purc_variant_t v);

/**
 * purc_variant_operator_add:
 *
 * @v1: The first variant.
 * @v2: The second variant.
 *
 * Perform the addition operation (@v1 + @v2) and return the summary of
 * @v1 and @v2.
 *
 * Returns: A variant evaluated on success,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.9.26
 */
PCA_EXPORT purc_variant_t
purc_variant_operator_add(purc_variant_t v1, purc_variant_t v2);

/**
 * purc_variant_operator_sub:
 *
 * @v1: The first variant.
 * @v2: The second variant.
 *
 * Perform the subtraction operation (@v1 - @v2) and return the difference
 * of @v1 and @v2.
 *
 * Returns: A variant evaluated on success,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.9.26
 */
PCA_EXPORT purc_variant_t
purc_variant_operator_sub(purc_variant_t v1, purc_variant_t v2);

/**
 * purc_variant_operator_mul:
 *
 * @v1: The first variant.
 * @v2: The second variant.
 *
 * Perform the multiplication operation (@v1 * @v2) and return the product
 * of @v1 and @v2.
 *
 * Returns: A variant evaluated on success,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.9.26
 */
PCA_EXPORT purc_variant_t
purc_variant_operator_mul(purc_variant_t v1, purc_variant_t v2);

/**
 * purc_variant_operator_truediv:
 *
 * @v1: The first variant.
 * @v2: The second variant.
 *
 * Perform the true division operation (@v1 / @v2) and return the true division
 * of @v1 and @v2.
 *
 * Returns: A variant evaluated on success,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.9.26
 */
PCA_EXPORT purc_variant_t
purc_variant_operator_truediv(purc_variant_t v1, purc_variant_t v2);

/**
 * purc_variant_operator_floordiv:
 *
 * @v1: The first variant.
 * @v2: The second variant.
 *
 * Perform the floor division operation and return the floor division
 * of @v1 and @v2.
 *
 * Returns: A variant evaluated on success,
 *      or %PURC_VARIANT_INVALID on failure.
 */
PCA_EXPORT purc_variant_t
purc_variant_operator_floordiv(purc_variant_t v1, purc_variant_t v2);

/**
 * purc_variant_operator_mod:
 *
 * @v1: The first variant.
 * @v2: The second variant.
 *
 * Perform the modulo operation (@v1 % @v2) and return the remainder of
 * @v1 and @v2.
 *
 * Returns: A variant evaluated on success,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.9.26
 */
PCA_EXPORT purc_variant_t
purc_variant_operator_mod(purc_variant_t v1, purc_variant_t v2);

/**
 * purc_variant_operator_pow:
 *
 * @v1: The first variant.
 * @v2: The second variant.
 *
 * Perform the power operation (@v1 ** @v2) and return the result
 * of @v1 raised to power @v2.
 *
 * Returns: A variant evaluated on success,
 *      or %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.9.26
 */
PCA_EXPORT purc_variant_t
purc_variant_operator_pow(purc_variant_t v1, purc_variant_t v2);

/**
 * purc_variant_operator_invert:
 *
 * @v: The variant.
 *
 * Perform the bitwise inverse of the number represented by @v.
 *
 * Returns: A variant evaluated on success,
 *      or %PURC_VARIANT_INVALID on failure.
 */
PCA_EXPORT purc_variant_t
purc_variant_operator_invert(purc_variant_t v);

/**
 * purc_variant_operator_and:
 *
 * @v1: The first variant.
 * @v2: The second variant.
 *
 * Perform the bitwise and operation (@v1 & @v2) and
 * return the bitwise and of @v1 and @v2.
 *
 * Returns: A variant evaluated on success,
 *      or %PURC_VARIANT_INVALID on failure.
 */
PCA_EXPORT purc_variant_t
purc_variant_operator_and(purc_variant_t v1, purc_variant_t v2);

/**
 * purc_variant_operator_or:
 *
 * @v1: The first variant.
 * @v2: The second variant.
 *
 * Perform the bitwise or operation (@v1 | @v2) and
 * return the bitwise or of @v1 and @v2.
 *
 * Returns: A variant evaluated on success,
 *      or %PURC_VARIANT_INVALID on failure.
 */
PCA_EXPORT purc_variant_t
purc_variant_operator_or(purc_variant_t v1, purc_variant_t v2);

/**
 * purc_variant_operator_xor:
 *
 * @v1: The first variant.
 * @v2: The second variant.
 *
 * Perform the bitwise xor operation (@v1 ^ @v2) and
 * return the bitwise xor of @v1 and @v2.
 *
 * Returns: A variant evaluated on success,
 *      or %PURC_VARIANT_INVALID on failure.
 */
PCA_EXPORT purc_variant_t
purc_variant_operator_xor(purc_variant_t v1, purc_variant_t v2);

/**
 * purc_variant_operator_lshift:
 *
 * @v: The variant.
 * @c: The shift count.
 *
 * Return @v shifted left by @c.
 *
 * Returns: A variant evaluated on success,
 *      or %PURC_VARIANT_INVALID on failure.
 */
PCA_EXPORT purc_variant_t
purc_variant_operator_lshift(purc_variant_t v, purc_variant_t c);

 /**
 * purc_variant_operator_rshift:
 *
 * @v: The variant.
 * @c: The shift count.
 *
 * Return @v shifted right by @c.
 *
 * Returns: A variant evaluated on success,
 *      or %PURC_VARIANT_INVALID on failure.
 */
PCA_EXPORT purc_variant_t
purc_variant_operator_rshift(purc_variant_t v, purc_variant_t c);

/**
 * purc_variant_operator_concat:
 *
 * @a: The first variant.
 * @b: The second variant.
 *
 * Perform the concatenation operation (@a + @b) for two sequences,
 * arrays, or tuples, and then return the concatenation of @a and @b.
 *
 * Returns: A variant evaluated on success,
 *      or %PURC_VARIANT_INVALID on failure.
 */
PCA_EXPORT purc_variant_t
purc_variant_operator_concat(purc_variant_t a, purc_variant_t b);

/**
 * purc_variant_operator_contains:
 *
 * @a: The first variant.
 * @b: The second variant.
 *
 * Perform the contains operation (@b in @a) for sequences and
 * return a boolean result.
 *
 * Returns: A boolean variant evaluated on success,
 *      or %PURC_VARIANT_INVALID on failure.
 */
PCA_EXPORT purc_variant_t
purc_variant_operator_contains(purc_variant_t a, purc_variant_t b);

/**
 * purc_variant_operator_iadd:
 *
 * @v1: The first variant.
 * @v2: The second variant.
 *
 * Perform the in-place addition operation (@v1 += @v2) and return @v1.
 *
 * Returns: 0 on success, or -1 on failure.
 *
 * Since: 0.9.26
 */
PCA_EXPORT int
purc_variant_operator_iadd(purc_variant_t v1, purc_variant_t v2);

/**
 * purc_variant_operator_isub:
 *
 * @v1: The first variant.
 * @v2: The second variant.
 *
 * Perform the in-place subtraction operation (@v1 -= @v2) and return @v1.
 *
 * Returns: 0 on success, or -1 on failure.
 *
 * Since: 0.9.26
 */
PCA_EXPORT int
purc_variant_operator_isub(purc_variant_t v1, purc_variant_t v2);

/**
 * purc_variant_operator_imul:
 *
 * @v1: The first variant.
 * @v2: The second variant.
 *
 * Perform the in-place multiplication operation (@v1 *= @v2) and return @v1.
 *
 * Returns: 0 on success, or -1 on failure.
 *
 * Since: 0.9.26
 */
PCA_EXPORT int
purc_variant_operator_imul(purc_variant_t v1, purc_variant_t v2); 

/**
 * purc_variant_operator_itruediv:
 *
 * @v1: The first variant.
 * @v2: The second variant.
 *
 * Perform the in-place true division operation (@v1 /= @v2) and return @v1.
 *
 * Returns: 0 on success, or -1 on failure.
 *
 * Since: 0.9.26
 */
PCA_EXPORT int
purc_variant_operator_itruediv(purc_variant_t v1, purc_variant_t v2);

/**
 * purc_variant_operator_ifloordiv:
 *
 * @v1: The first variant.
 * @v2: The second variant.
 *
 * Perform the in-place floor division operation (@v1 //= @v2) and return @v1.
 *
 * Returns: 0 on success, or -1 on failure.
 */
PCA_EXPORT int
purc_variant_operator_ifloordiv(purc_variant_t v1, purc_variant_t v2);

/**
 * purc_variant_operator_imod:
 *
 * @v1: The first variant.
 * @v2: The second variant.
 *
 * Perform the in-place modulo operation (@v1 %= @v2) and return @v1.
 *
 * Returns: 0 on success, or -1 on failure.
 *
 * Since: 0.9.26
 */
PCA_EXPORT int
purc_variant_operator_imod(purc_variant_t v1, purc_variant_t v2); 

/**
 * purc_variant_operator_ipow:
 *
 * @v1: The first variant.
 * @v2: The second variant.
 *
 * Perform the in-place power operation (@v1 **= @v2) and return @v1.
 *
 * Returns: 0 on success, or -1 on failure.
 *
 * Since: 0.9.26
 */
PCA_EXPORT int
purc_variant_operator_ipow(purc_variant_t v1, purc_variant_t v2);

/**
 * purc_variant_operator_iand:
 *
 * @v1: The first variant.
 * @v2: The second variant.
 *
 * Perform the in-place bitwise and operation (@v1 &= @v2) and return @v1.
 *
 * Returns: 0 on success, or -1 on failure.
 */
PCA_EXPORT int
purc_variant_operator_iand(purc_variant_t v1, purc_variant_t v2);

/**
 * purc_variant_operator_ior:
 *
 * @v1: The first variant.
 * @v2: The second variant.
 *
 * Perform the in-place bitwise or operation (@v1 |= @v2) and return @v1.
 *
 * Returns: 0 on success, or -1 on failure.
 */
PCA_EXPORT int
purc_variant_operator_ior(purc_variant_t v1, purc_variant_t v2);

/**
 * purc_variant_operator_ixor:
 *
 * @v1: The first variant.
 * @v2: The second variant.
 *
 * Perform the in-place bitwise xor operation (@v1 ^= @v2) and return @v1.
 *
 * Returns: 0 on success, or -1 on failure.
 */
PCA_EXPORT int
purc_variant_operator_ixor(purc_variant_t v1, purc_variant_t v2);

/**
 * purc_variant_operator_ilshift:
 *
 * @v: The variant.
 * @c: The shift count.
 *
 * Perform the in-place left shift operation (@v <<= @c) and return @v.
 *
 * Returns: 0 on success, or -1 on failure.
 *
 * Since: 0.9.26
 */
PCA_EXPORT int
purc_variant_operator_ilshift(purc_variant_t v, purc_variant_t c);

/**
 * purc_variant_operator_irshift:
 *
 * @v: The variant.
 * @c: The shift count.
 *
 * Perform the in-place right shift operation (@v >>= @c) and return @v.
 *
 * Returns: 0 on success, or -1 on failure.
 *
 * Since: 0.9.26
 */
PCA_EXPORT int
purc_variant_operator_irshift(purc_variant_t v, purc_variant_t c);

/**
 * purc_variant_operator_iconcat:
 *
 * @a: The first variant.
 * @b: The second variant.
 *
 * Perform the in-place concatenation operation (@a += @b) for two sequences
 * or two arrays.
 *
 * Returns: 0 on success, or -1 on failure.
 *
 * Since: 0.9.26
 */
PCA_EXPORT int
purc_variant_operator_iconcat(purc_variant_t a, purc_variant_t b);

PCA_EXTERN_C_END

#endif /* not defined PURC_PURC_VARIANT_H */

