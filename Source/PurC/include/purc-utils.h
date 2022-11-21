/**
 * @file purc-utils.h
 * @author Vincent Wei
 * @date 2021/07/05
 * @brief The API for utilities.
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

#ifndef PURC_PURC_UTILS_H
#define PURC_PURC_UTILS_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <locale.h>
#include <sys/types.h>  /* for ssize_t on macOS */

#include "purc-macros.h"

typedef struct {
    unsigned char * data;
    size_t          length;
} pcutils_str_t;

struct pcutils_mraw;
typedef struct pcutils_mraw pcutils_mraw_t;

PCA_EXTERN_C_BEGIN

pcutils_mraw_t *
pcutils_mraw_create(void);

unsigned int
pcutils_mraw_init(pcutils_mraw_t *mraw, size_t chunk_size);

void
pcutils_mraw_clean(pcutils_mraw_t *mraw);

pcutils_mraw_t *
pcutils_mraw_destroy(pcutils_mraw_t *mraw, bool destroy_self);


void *
pcutils_mraw_alloc(pcutils_mraw_t *mraw, size_t size);

void *
pcutils_mraw_calloc(pcutils_mraw_t *mraw, size_t size);

void *
pcutils_mraw_realloc(pcutils_mraw_t *mraw, void *data, size_t new_size);

void *
pcutils_mraw_free(pcutils_mraw_t *mraw, void *data);

PCA_EXTERN_C_END

#define PCUTILS_HASH_SHORT_SIZE     16

struct pcutils_hash;
typedef struct pcutils_hash pcutils_hash_t;

struct pcutils_hash_entry;
typedef struct pcutils_hash_entry pcutils_hash_entry_t;

struct pcutils_hash_entry {
    union {
        unsigned char *long_str;
        unsigned char short_str[PCUTILS_HASH_SHORT_SIZE + 1];
    } u;

    size_t              length;

    pcutils_hash_entry_t *next;
};

static inline unsigned char *
pcutils_hash_entry_str(const pcutils_hash_entry_t *entry)
{
    if (entry->length <= PCUTILS_HASH_SHORT_SIZE) {
        return (unsigned char *) entry->u.short_str;
    }

    return entry->u.long_str;
}

/**
 * SECTION: array
 * @title: array
 * @short_description: a simple array implementation
 */
typedef struct {
    void   **list;
    size_t size;
    size_t length;
} pcutils_array_t;

PCA_EXTERN_C_BEGIN

pcutils_array_t * pcutils_array_create(void);

unsigned int
pcutils_array_init(pcutils_array_t *array, size_t size);

void pcutils_array_clean(pcutils_array_t *array);

pcutils_array_t *
pcutils_array_destroy(pcutils_array_t *array, bool self_destroy);

void **
pcutils_array_expand(pcutils_array_t *array, size_t up_to);

unsigned int
pcutils_array_push(pcutils_array_t *array, void *value);

void * pcutils_array_pop(pcutils_array_t *array);

unsigned int
pcutils_array_insert(pcutils_array_t *array, size_t idx, void *value);

unsigned int
pcutils_array_set(pcutils_array_t *array, size_t idx, void *value);

void
pcutils_array_delete(pcutils_array_t *array, size_t begin, size_t length);

/* Inline functions */
static inline void * pcutils_array_get(pcutils_array_t *array, size_t idx)
{
    if (idx >= array->length) {
        return NULL;
    }

    return array->list[idx];
}

static inline size_t pcutils_array_length(pcutils_array_t *array)
{
    return array->length;
}

static inline size_t pcutils_array_size(pcutils_array_t *array)
{
    return array->size;
}

PCA_EXTERN_C_END

/**
 * SECTION: atom
 * @title: Atom String
 * @short_description: a 2-way association between a string and a
 *     unique integer identifier
 *
 * Atoms are associations between strings and integer identifiers.
 * Given either the string or the #purc_atom_t identifier it is possible to
 * retrieve the other.
 *
 * To create a new atom from a string, use purc_atom_from_string() or
 * purc_atom_from_static_string().
 *
 * To find the string corresponding to a given #purc_atom_t, use
 * purc_atom_to_string().
 *
 * To find the #purc_atom_t corresponding to a given string, use
 * purc_atom_try_string() or purc_atom_try_string_ex().
 */

/**
 * purc_atom_t:
 *
 * A purc_atom_t is a non-zero integer which uniquely identifies a
 * particular string. A purc_atom_t value of zero is associated to %NULL.
 */
typedef unsigned int purc_atom_t;

#define PURC_ATOM_BUCKET_BITS   4
#define PURC_ATOM_BUCKETS_NR    (1 << PURC_ATOM_BUCKET_BITS)

/** The atom bucket identifier for default */
#define PURC_ATOM_BUCKET_DEF    0

/** The atom bucket identifier for exception name */
#define PURC_ATOM_BUCKET_EXCEPT 1

/** The atom bucket identifier reserved for built-in renderer */
#define PURC_ATOM_BUCKET_RDR    (PURC_ATOM_BUCKETS_NR - 1)

/** The atom bucket identifier reserved for user usage */
#define PURC_ATOM_BUCKET_USER   (PURC_ATOM_BUCKET_RDR - 1)

PCA_EXTERN_C_BEGIN

/**
 * purc_atom_from_string_ex2:
 * @bucket: the identifier of the atom bucket.
 * @string: a string (nullable).
 * @newly_created: a boolean buffer to receive the state that the atom is
 *  newly created or it is an old one created by other (nullable).
 *
 * Gets the #purc_atom_t identifying the given string in the specified
 * atom bucket. If the string does not currently have an associated
 * #purc_atom_t a new #purc_atom_t is created, using a copy of the string.
 *
 * This function must not be used before library constructors have finished
 * running. In particular, this means it cannot be used to initialize global
 * variables in C++.
 *
 * Returns: the #purc_atom_t identifying the string, or 0 if @string is %NULL.
 */
PCA_EXPORT purc_atom_t
purc_atom_from_string_ex2(int bucket, const char* string, bool *newly_created);

/**
 * purc_atom_from_string_ex:
 * @bucket: the identifier of the atom bucket.
 * @string: (nullable), a string
 *
 * Gets the #purc_atom_t identifying the given string in the specified
 * atom bucket. If the string does not currently have an associated
 * #purc_atom_t, a new #purc_atom_t is created, using a copy of the string.
 *
 * This function must not be used before library constructors have finished
 * running. In particular, this means it cannot be used to initialize global
 * variables in C++.
 *
 * Returns: the #purc_atom_t identifying the string, or 0 if @string is %NULL
 */
static inline purc_atom_t
purc_atom_from_string_ex(int bucket, const char* string)
{
    return purc_atom_from_string_ex2(bucket, string, NULL);
}

/**
 * purc_atom_from_string:
 * @string: (nullable): a string
 *
 * Gets the #purc_atom_t identifying the given string in the default bucket.
 * If the string does not currently have an associated #purc_atom_t, a new
 * #purc_atom_t is created, using a copy of the string.
 *
 * This function must not be used before library constructors have finished
 * running. In particular, this means it cannot be used to initialize global
 * variables in C++.
 *
 * Returns: the #purc_atom_t identifying the string, or 0 if @string is %NULL
 */
static inline purc_atom_t purc_atom_from_string(const char* string) {
    return purc_atom_from_string_ex2(0, string, NULL);
}

/**
 * purc_atom_from_static_string_ex2:
 * @bucket: the identifier of the atom bucket.
 * @string: a string (nullable).
 * @newly_created: a boolean buffer to receive the state that the atom is
 *  newly created or it is an old one created by other (nullable).
 *
 * Gets the #purc_atom_t identifying the given (static) string in the specified
 * atom bucket. If the string does not currently have an associated
 * #purc_atom_t, a new #purc_atom_t is created, linked to the given string.
 *
 * Note that this function is identical to purc_atom_from_string_ex() except
 * that if a new #purc_atom_t is created the string itself is used rather
 * than a copy. This saves memory, but can only be used if the string
 * will continue to exist until the program terminates. It can be used
 * with statically allocated strings in the main program, but not with
 * statically allocated memory in dynamically loaded modules, if you
 * expect to ever unload the module again.
 *
 * This function must not be used before library constructors have finished
 * running. In particular, this means it cannot be used to initialize global
 * variables in C++.
 *
 * Returns: the #purc_atom_t identifying the string, or 0 if @string is %NULL
 */
PCA_EXPORT purc_atom_t
purc_atom_from_static_string_ex2(int bucket, const char* string,
        bool *newly_created);

/**
 * purc_atom_from_static_string_ex:
 * @bucket: the identifier of the atom bucket.
 * @string: (nullable): a string
 *
 * Gets the #purc_atom_t identifying the given (static) string in the specified
 * atom bucket. If the string does not currently have an associated
 * #purc_atom_t, a new #purc_atom_t is created, linked to the given string.
 *
 * Note that this function is identical to purc_atom_from_string_ex() except
 * that if a new #purc_atom_t is created the string itself is used rather
 * than a copy. This saves memory, but can only be used if the string
 * will continue to exist until the program terminates. It can be used
 * with statically allocated strings in the main program, but not with
 * statically allocated memory in dynamically loaded modules, if you
 * expect to ever unload the module again (e.g. do not use this
 * function in GTK+ theme engines).
 *
 * This function must not be used before library constructors have finished
 * running. In particular, this means it cannot be used to initialize global
 * variables in C++.
 *
 * Returns: the #purc_atom_t identifying the string, or 0 if @string is %NULL
 */
static inline purc_atom_t
purc_atom_from_static_string_ex(int bucket, const char* string) {
    return purc_atom_from_static_string_ex2(bucket, string, NULL);
}

/**
 * purc_atom_from_static_string:
 * @string: (nullable): a string
 *
 * Gets the #purc_atom_t identifying the given (static) string in the default
 * bucket. If the string does not currently have an associated #purc_atom_t,
 * a new #purc_atom_t is created, linked to the given string.
 *
 * Note that this function is identical to purc_atom_from_string() except
 * that if a new #purc_atom_t is created the string itself is used rather
 * than a copy. This saves memory, but can only be used if the string
 * will continue to exist until the program terminates. It can be used
 * with statically allocated strings in the main program, but not with
 * statically allocated memory in dynamically loaded modules, if you
 * expect to ever unload the module again (e.g. do not use this
 * function in GTK+ theme engines).
 *
 * This function must not be used before library constructors have finished
 * running. In particular, this means it cannot be used to initialize global
 * variables in C++.
 *
 * Returns: the #purc_atom_t identifying the string, or 0 if @string is %NULL
 */
static inline purc_atom_t purc_atom_from_static_string(const char* string) {
    return purc_atom_from_static_string_ex2(0, string, NULL);
}

/**
 * purc_atom_try_string_ex:
 * @bucket: the identifier of the atom bucket.
 * @string: (nullable): a string
 *
 * Gets the #purc_atom_t associated with the given string in the specified
 * bucket, or 0 if string is %NULL or it has no associated #purc_atom_t
 * in the specified bucket.
 *
 * If you want the purc_atom_t to be created if it doesn't already exist,
 * use purc_atom_from_string() or purc_atom_from_static_string().
 *
 * This function must not be used before library constructors have finished
 * running.
 *
 * Returns: the #purc_atom_t associated with the string, or 0 if @string is
 *     %NULL or there is no #purc_atom_t associated with it
 */
PCA_EXPORT purc_atom_t
purc_atom_try_string_ex(int bucket, const char* string);

/**
 * purc_atom_try_string:
 * @string: (nullable): a string
 *
 * Gets the #purc_atom_t associated with the given string, or 0 if string is
 * %NULL or it has no associated #purc_atom_t in the default bucket.
 *
 * If you want the purc_atom_t to be created if it doesn't already exist,
 * use purc_atom_from_string() or purc_atom_from_static_string().
 *
 * This function must not be used before library constructors have finished
 * running.
 *
 * Returns: the #purc_atom_t associated with the string, or 0 if @string is
 *     %NULL or there is no #purc_atom_t associated with it
 */
static inline purc_atom_t purc_atom_try_string(const char* string) {
    return purc_atom_try_string_ex(0, string);
}

/**
 * purc_atom_remove_string_ex:
 * @bucket: the identifier of the atom bucket.
 * @string: (nullable): a string
 *
 * Removes the given string in the specified bucket, so that the next call
 * to purc_atom_try_string_ex() will return 0 on the same string. If you
 * use purc_atom_from_string_ex() or purc_atom_from_static_string_ex() on the
 * same string in the same bucket after calling this function, you will get
 * another atom value. Note that the old atom value will be invalid, i.e.,
 * you cannot get the string by calling purc_atom_to_string() by using the
 * old atom value.
 *
 * This function must not be used before library constructors have finished
 * running.
 *
 * Returns: %TRUE if @string is removed from the specified bucekt;
 *      %FALSE there is no atom associated with the string.
 */
PCA_EXPORT bool
purc_atom_remove_string_ex(int bucket, const char* string);

/**
 * purc_atom_remove_string:
 * @string: (nullable): a string
 *
 * Removes the given string in the default bucket, so that the next call
 * to purc_atom_try_string() will return 0 on the same string. If you
 * use purc_atom_from_string() or purc_atom_from_static_string() on the
 * same string after calling this function, you will get
 * another atom value. Note that the old atom value will be invalid, i.e.,
 * you cannot get the string by calling purc_atom_to_string() by using the
 * old atom value.
 *
 * This function must not be used before library constructors have finished
 * running.
 *
 * Returns: %TRUE if @string is removed from the default bucekt;
 *      %FALSE there is no atom associated with the string.
 */
static inline bool purc_atom_remove_string(const char* string) {
    return purc_atom_remove_string_ex(0, string);
}

/**
 * purc_atom_to_string:
 * @quark: a #purc_atom_t.
 *
 * Gets the string associated with the given #purc_atom_t.
 *
 * Returns: the string associated with the #purc_atom_t
 */
PCA_EXPORT const char*
purc_atom_to_string(purc_atom_t atom);

/**
 * SECTION: misc_utils
 * @title: Misc. Utilities
 * @short_description: Some useful helpers and utilities.
 *
 */

/**
 * pcutils_get_random_seed:
 *
 * Gets a good and safe random seed.
 *
 * Returns: the random seed.
 */
PCA_EXPORT int
pcutils_get_random_seed(void);

/**
 * pcutils_get_prev_fibonacci_number:
 *
 * @n: a size_t number
 *
 * Gets previous fibonacci number before @n.
 *
 * @return The greatest fibonacci number less than @n.
 */
PCA_EXPORT size_t
pcutils_get_prev_fibonacci_number(size_t n);

/**
 * pcutils_get_next_fibonacci_number:
 *
 * @n: a size_t number
 *
 * Gets next fibonacci number after @n.
 *
 * @return The least fibonacci sequence greater than @n.
 */
PCA_EXPORT size_t
pcutils_get_next_fibonacci_number(size_t n);

/**
 * SECTION: arraylist
 * @title: Array List
 * @short_description: A basic Array implementation.
 *
 */

#define ARRAY_LIST_DEFAULT_SIZE 32

typedef void(array_list_free_fn)(void *data);

struct pcutils_arrlist {
    void **array;
    size_t length;
    size_t size;
    array_list_free_fn *free_fn;
};
typedef struct pcutils_arrlist pcutils_arrlist;

/**
 * Allocate a pcutils_arrlist of the desired size.
 *
 * If possible, the size should be chosen to closely match
 * the actual number of elements expected to be used.
 * If the exact size is unknown, there are tradeoffs to be made:
 * - too small - the pcutils_arrlist code will need to call realloc() more
 *   often (which might incur an additional memory copy).
 * - too large - will waste memory, but that can be mitigated
 *   by calling pcutils_arrlist_shrink() once the final size is known.
 *
 * VW: @free_fn is nullable.
 *
 * @see pcutils_arrlist_shrink
 */
PCA_EXPORT struct pcutils_arrlist *
pcutils_arrlist_new_ex(array_list_free_fn *free_fn, size_t initial_size);

/**
 * Allocate a pcutils_arrlist of the default size (32).
 */
static inline struct pcutils_arrlist *
pcutils_arrlist_new(array_list_free_fn *free_fn) {
    return pcutils_arrlist_new_ex(free_fn, ARRAY_LIST_DEFAULT_SIZE);
}

/** Free a pcutils_arrlist. */
PCA_EXPORT void
pcutils_arrlist_free(struct pcutils_arrlist *al);

/** Get data stored in the specified slot of a pcutils_arrlist. */
PCA_EXPORT void *
pcutils_arrlist_get_idx(struct pcutils_arrlist *al, size_t i);

/** Put data in the specified slot of a pcutils_arrlist. */
PCA_EXPORT int
pcutils_arrlist_put_idx(struct pcutils_arrlist *al, size_t i, void *data);

/** Swap data of two slots in a pcutils_arrlist. */
PCA_EXPORT bool
pcutils_arrlist_swap(struct pcutils_arrlist *al, size_t idx1, size_t idx2);

/** Append data to a pcutils_arrlist. */
PCA_EXPORT int
pcutils_arrlist_append(struct pcutils_arrlist *al, void *data);

/** Get the length (number of slots) of a pcutils_arrlist. */
PCA_EXPORT size_t
pcutils_arrlist_length(struct pcutils_arrlist *al);

/** Sort a pcutils_arrlist. */
PCA_EXPORT void
pcutils_arrlist_sort(struct pcutils_arrlist *arr,
        int (*compar)(const void *, const void *));

/** Perform a binary search in a sorted pcutils_arrlist. */
PCA_EXPORT void *
pcutils_arrlist_bsearch(const void **key,
        struct pcutils_arrlist *arr,
        int (*compar)(const void *, const void *));

/**
 * Remove data in a pcutils_arrlist.
 */
PCA_EXPORT int
pcutils_arrlist_del_idx(struct pcutils_arrlist *arr,
        size_t idx, size_t count);

/**
 * Shrink the array list to just enough to fit the number of elements in it,
 * plus empty_slots.
 */
PCA_EXPORT int
pcutils_arrlist_shrink(struct pcutils_arrlist *arr, size_t empty_slots);

/**
 * Get the data stored in the first slot of a pcutils_arrlist.
 */
PCA_EXPORT void*
pcutils_arrlist_get_first(struct pcutils_arrlist *arr);

/**
 * Get the data stored in the last slot of a pcutils_arrlist.
 */
PCA_EXPORT void*
pcutils_arrlist_get_last(struct pcutils_arrlist *arr);

/** Portable implementation of `snprintf` */
PCA_EXPORT char*
pcutils_snprintf(char *buf, size_t *sz_io, const char *fmt, ...)
    PCA_ATTRIBUTE_PRINTF(3, 4);

/** Portable implementation of `vsnprintf` */
PCA_EXPORT char*
pcutils_vsnprintf(char *buf, size_t *sz_io, const char *fmt, va_list ap);

/** Trim leading and trailling blank characters (whitespaces or tabs) */
PCA_EXPORT const char*
pcutils_trim_blanks(const char *str, size_t *sz_io);

/** Trim leading and trailling space characters (whitespace, form-feed ('\f'),
  * newline ('\n'), carriage return ('\r'), horizontal tab ('\t'),
  * and vertical tab ('\v')) */
PCA_EXPORT const char*
pcutils_trim_spaces(const char *str, size_t *sz_io);

/** Determine whether a string contains graphical characters (printable
  * characters except spaces. */
PCA_EXPORT bool
pcutils_contains_graph(const char *str);

/** Get the pointer of the next valid token and length in a string */
PCA_EXPORT const char *
pcutils_get_next_token(const char *str, const char *delims, size_t *length);

/** Get the pointer of the next valid token and length in a string */
PCA_EXPORT const char *
pcutils_get_next_token_len(const char *str, size_t str_len,
        const char *delims, size_t *length);

/** Escape a string for JSON */
PCA_EXPORT char*
pcutils_escape_string_for_json(const char* str);

/**
 * Counts the number of Unicode characters in a UTF-8 string until
 * reaching the ax length of bytes or encountering a null byte.
 * If len < 0, the string must be null-terminated.  */
PCA_EXPORT size_t
pcutils_string_utf8_chars(const char *p, ssize_t max);

/**
 * Counts the number of Unicode characters in a UTF-8 string which
 * can contains null characters. But if len < 0, the string must be
 * null-terminated.
 */
PCA_EXPORT size_t
pcutils_string_utf8_chars_with_nulls(const char *p, ssize_t len);

extern const char * const _pcutils_utf8_skip;

#define pcutils_utf8_next_char(p)   \
    (char *)((p) + _pcutils_utf8_skip[*(const unsigned char *)(p)])

/** Check validation of Unicode characters in a UTF-8 encoded string. */
PCA_EXPORT bool
pcutils_string_check_utf8_len(const char* str, size_t max_len,
        size_t *nr_chars, const char **end);

/** Check validation of Unicode characters in a UTF-8 encoded string. */
PCA_EXPORT bool
pcutils_string_check_utf8(const char *str, ssize_t max_len,
        size_t *nr_chars, const char **end);

PCA_EXPORT char *
pcutils_string_decode_utf16le(const unsigned char* bytes, size_t max_len,
        size_t *sz_space, size_t *consumed, bool silently);

PCA_EXPORT char *
pcutils_string_decode_utf32le(const unsigned char* bytes, size_t max_len,
        size_t *sz_space, size_t *consumed, bool silently);

PCA_EXPORT char *
pcutils_string_decode_utf16be(const unsigned char* bytes, size_t max_len,
        size_t *sz_space, size_t *consumed, bool silently);

PCA_EXPORT char *
pcutils_string_decode_utf32be(const unsigned char* bytes, size_t max_len,
        size_t *sz_space, size_t *consumed, bool silently);

PCA_EXPORT char *
pcutils_string_decode_utf16(const unsigned char* bytes, size_t max_len,
        size_t *sz_space, size_t *consumed, bool silently);

PCA_EXPORT char *
pcutils_string_decode_utf32(const unsigned char* bytes, size_t max_len,
        size_t *sz_space, size_t *consumed, bool silently);

PCA_EXPORT size_t
pcutils_string_decode_utf8(uint32_t *ucs, size_t max_chars,
        const char* str_utf8);

PCA_EXPORT uint32_t *
pcutils_string_decode_utf8_alloc(const char* str_utf8, ssize_t max_len,
        size_t *nr_chars);

PCA_EXPORT unsigned
pcutils_unichar_to_utf8(uint32_t uc, unsigned char* buff);

PCA_EXPORT uint32_t
pcutils_utf8_to_unichar(const unsigned char* mchar);

PCA_EXPORT char *
pcutils_string_encode_utf8(const uint32_t *ucs, size_t nr_chars,
        size_t *sz_space);

PCA_EXPORT size_t
pcutils_string_encode_utf16le(const char* utf8, size_t len, size_t nr_chars,
        unsigned char *bytes, size_t max_bytes);

PCA_EXPORT size_t
pcutils_string_encode_utf32le(const char* utf8, size_t len, size_t nr_chars,
        unsigned char *bytes, size_t max_bytes);

PCA_EXPORT size_t
pcutils_string_encode_utf16be(const char* utf8, size_t len, size_t nr_chars,
       unsigned char *bytes, size_t max_bytes);

PCA_EXPORT size_t
pcutils_string_encode_utf32be(const char* utf8, size_t len, size_t nr_chars,
        unsigned char *bytes, size_t max_bytes);

PCA_EXPORT size_t
pcutils_string_encode_utf16(const char* utf8, size_t len, size_t nr_chars,
        unsigned char *bytes, size_t max_bytes);

PCA_EXPORT size_t
pcutils_string_encode_utf32(const char* utf8, size_t len, size_t nr_chars,
        unsigned char *bytes, size_t max_bytes);

typedef union {
    int64_t     i64;
    uint64_t    u64;
    double      d;
    long double ld;
} purc_real_t;

PCA_EXPORT purc_real_t
purc_fetch_i8(const unsigned char *bytes);

PCA_EXPORT purc_real_t
purc_fetch_i16(const unsigned char *bytes);

PCA_EXPORT purc_real_t
purc_fetch_i32(const unsigned char *bytes);

PCA_EXPORT purc_real_t
purc_fetch_i64(const unsigned char *bytes);

PCA_EXPORT purc_real_t
purc_fetch_i16le(const unsigned char *bytes);

PCA_EXPORT purc_real_t
purc_fetch_i32le(const unsigned char *bytes);

PCA_EXPORT purc_real_t
purc_fetch_i64le(const unsigned char *bytes);

PCA_EXPORT purc_real_t
purc_fetch_i16be(const unsigned char *bytes);

PCA_EXPORT purc_real_t
purc_fetch_i32be(const unsigned char *bytes);

PCA_EXPORT purc_real_t
purc_fetch_i64be(const unsigned char *bytes);

PCA_EXPORT purc_real_t
purc_fetch_u8(const unsigned char *bytes);

PCA_EXPORT purc_real_t
purc_fetch_u16(const unsigned char *bytes);

PCA_EXPORT purc_real_t
purc_fetch_u32(const unsigned char *bytes);

PCA_EXPORT purc_real_t
purc_fetch_u64(const unsigned char *bytes);

PCA_EXPORT purc_real_t
purc_fetch_u16le(const unsigned char *bytes);

PCA_EXPORT purc_real_t
purc_fetch_u32le(const unsigned char *bytes);

PCA_EXPORT purc_real_t
purc_fetch_u64le(const unsigned char *bytes);

PCA_EXPORT purc_real_t
purc_fetch_u16be(const unsigned char *bytes);

PCA_EXPORT purc_real_t
purc_fetch_u32be(const unsigned char *bytes);

PCA_EXPORT purc_real_t
purc_fetch_u64be(const unsigned char *bytes);

PCA_EXPORT purc_real_t
purc_fetch_f16(const unsigned char *bytes);

PCA_EXPORT purc_real_t
purc_fetch_f32(const unsigned char *bytes);

PCA_EXPORT purc_real_t
purc_fetch_f64(const unsigned char *bytes);

PCA_EXPORT purc_real_t
purc_fetch_f96(const unsigned char *bytes);

PCA_EXPORT purc_real_t
purc_fetch_f128(const unsigned char *bytes);

PCA_EXPORT purc_real_t
purc_fetch_f16le(const unsigned char *bytes);

PCA_EXPORT purc_real_t
purc_fetch_f32le(const unsigned char *bytes);

PCA_EXPORT purc_real_t
purc_fetch_f64le(const unsigned char *bytes);

PCA_EXPORT purc_real_t
purc_fetch_f96le(const unsigned char *bytes);

PCA_EXPORT purc_real_t
purc_fetch_f128le(const unsigned char *bytes);

PCA_EXPORT purc_real_t
purc_fetch_f16be(const unsigned char *bytes);

PCA_EXPORT purc_real_t
purc_fetch_f32be(const unsigned char *bytes);

PCA_EXPORT purc_real_t
purc_fetch_f64be(const unsigned char *bytes);

PCA_EXPORT purc_real_t
purc_fetch_f96be(const unsigned char *bytes);

PCA_EXPORT purc_real_t
purc_fetch_f128be(const unsigned char *bytes);

PCA_EXPORT bool
purc_dump_i8(unsigned char *dst, purc_real_t real, bool force);

PCA_EXPORT bool
purc_dump_i16(unsigned char *dst, purc_real_t real, bool force);

PCA_EXPORT bool
purc_dump_i32(unsigned char *dst, purc_real_t real, bool force);

PCA_EXPORT bool
purc_dump_i64(unsigned char *dst, purc_real_t real, bool force);

PCA_EXPORT bool
purc_dump_i16le(unsigned char *dst, purc_real_t real, bool force);

PCA_EXPORT bool
purc_dump_i32le(unsigned char *dst, purc_real_t real, bool force);

PCA_EXPORT bool
purc_dump_i64le(unsigned char *dst, purc_real_t real, bool force);

PCA_EXPORT bool
purc_dump_i16be(unsigned char *dst, purc_real_t real, bool force);

PCA_EXPORT bool
purc_dump_i32be(unsigned char *dst, purc_real_t real, bool force);

PCA_EXPORT bool
purc_dump_i64be(unsigned char *dst, purc_real_t real, bool force);

PCA_EXPORT bool
purc_dump_u8(unsigned char *dst, purc_real_t real, bool force);

PCA_EXPORT bool
purc_dump_u16(unsigned char *dst, purc_real_t real, bool force);

PCA_EXPORT bool
purc_dump_u32(unsigned char *dst, purc_real_t real, bool force);

PCA_EXPORT bool
purc_dump_u64(unsigned char *dst, purc_real_t real, bool force);

PCA_EXPORT bool
purc_dump_u16le(unsigned char *dst, purc_real_t real, bool force);

PCA_EXPORT bool
purc_dump_u32le(unsigned char *dst, purc_real_t real, bool force);

PCA_EXPORT bool
purc_dump_u64le(unsigned char *dst, purc_real_t real, bool force);

PCA_EXPORT bool
purc_dump_u16be(unsigned char *dst, purc_real_t real, bool force);

PCA_EXPORT bool
purc_dump_u32be(unsigned char *dst, purc_real_t real, bool force);

PCA_EXPORT bool
purc_dump_u64be(unsigned char *dst, purc_real_t real, bool force);

PCA_EXPORT bool
purc_dump_f16(unsigned char *dst, purc_real_t real, bool force);

PCA_EXPORT bool
purc_dump_f32(unsigned char *dst, purc_real_t real, bool force);

PCA_EXPORT bool
purc_dump_f64(unsigned char *dst, purc_real_t real, bool force);

PCA_EXPORT bool
purc_dump_f96(unsigned char *dst, purc_real_t real, bool force);

PCA_EXPORT bool
purc_dump_f128(unsigned char *dst, purc_real_t real, bool force);

PCA_EXPORT bool
purc_dump_f16le(unsigned char *dst, purc_real_t real, bool force);

PCA_EXPORT bool
purc_dump_f32le(unsigned char *dst, purc_real_t real, bool force);

PCA_EXPORT bool
purc_dump_f64le(unsigned char *dst, purc_real_t real, bool force);

PCA_EXPORT bool
purc_dump_f96le(unsigned char *dst, purc_real_t real, bool force);

PCA_EXPORT bool
purc_dump_f128le(unsigned char *dst, purc_real_t real, bool force);

PCA_EXPORT bool
purc_dump_f16be(unsigned char *dst, purc_real_t real, bool force);

PCA_EXPORT bool
purc_dump_f32be(unsigned char *dst, purc_real_t real, bool force);

PCA_EXPORT bool
purc_dump_f64be(unsigned char *dst, purc_real_t real, bool force);

PCA_EXPORT bool
purc_dump_f96be(unsigned char *dst, purc_real_t real, bool force);

PCA_EXPORT bool
purc_dump_f128be(unsigned char *dst, purc_real_t real, bool force);

/** Returns the basename of fname. The result is a pointer into fname. */
PCA_EXPORT const char *
pcutils_basename(const char* fname);

/** The structure representing a broken-down URL. */
struct purc_broken_down_url {
    /** the schema component */
    char *schema;
    /** the user component */
    char *user;
    /** the password component */
    char *passwd;
    /** the host component */
    char *host;
    /** the path component */
    char *path;
    /** the query component */
    char *query;
    /** the fragment component */
    char *fragment;
    /** the port component */
    unsigned int port;
};

/**
 * Allocate an empty broken-down URL.
 *
 * Returns: The pointer to the newly allocated empty broken-down URL structure.
 */
static inline struct purc_broken_down_url *
pcutils_broken_down_url_new(void)
{
    return (struct purc_broken_down_url *)calloc(1,
            sizeof(struct purc_broken_down_url));
}

/**
 * Delete a broken-down URL.
 *
 * @param broken_down The pointer to a broken-down URL structure.
 */
PCA_EXPORT void
pcutils_broken_down_url_delete(struct purc_broken_down_url *broken_down);

/**
 * Clear a broken-down URL for reuse.
 */
PCA_EXPORT void
pcutils_broken_down_url_clear(struct purc_broken_down_url *broken_down);

/**
 * Assemble a broken-down URL.
 *
 * @param broken_down The pointer to a broken-down URL structure.
 * @param keep_percent_escaped Whether to keep percent escaped.
 *
 * Returns: The assembled URL string on success, otherwise @NULL.
 *  The caller will be the owner of the returned pointer; call
 *  free() to release the memory when done.
 */
PCA_EXPORT char *
pcutils_url_assemble(const struct purc_broken_down_url *broken_down,
        bool keep_percent_escaped);

/**
 * Break down a URL string.
 *
 * @param broken_down The pointer to a broken-down URL structure to store
 *  the components of a broken down URL.
 * @param url The null-terminated URL string.
 *
 * Returns: @true on success, @false for a bad URL string.
 */
PCA_EXPORT bool
pcutils_url_break_down(struct purc_broken_down_url *broken_down,
        const char *url);

/**
 * Checks whether the URL string is valid.
 *
 * @param url The null-terminated URL string.
 *
 * Returns: @true on success, @false for a bad URL string.
 */
PCA_EXPORT bool
pcutils_url_is_valid(const char *url);

/**
 * Copy the value of the specified key from a broken down URL to the specified
 * buffer if found.
 *
 * @param broken_down The broken down URL.
 * @param key The pointer to the key string.
 *
 * Returns: @true on success, @false not found.
 *
 * Note that the buffer should be large enough for the value.
 *
 * Since: 0.1.0
 */
PCA_EXPORT bool
pcutils_url_get_query_value(const struct purc_broken_down_url *broken_down,
        const char *key, char *value_buff);

/**
 * Copy the value of the specified key from a broken down URL to a newly
 * allocated buffer if found.
 *
 * @param broken_down The broken down URL.
 * @param key The pointer to the key string.
 * @param value_buff The pointer to a pointer to a string to receive
 *      the buffer allocated for the value. The caller will own the buffer,
 *      and should call free() to release the buffer after using it.
 *
 * Returns: @true on success, @false not found.
 *
 * Note that the buffer should be large enough for the value.
 *
 * Since: 0.1.0
 */
PCA_EXPORT bool
pcutils_url_get_query_value_alloc(
        const struct purc_broken_down_url *broken_down,
        const char *key, char **value_buff);

PCA_EXTERN_C_END

PCA_EXTERN_C_BEGIN
struct pcutils_wildcard;

struct pcutils_wildcard*
pcutils_wildcard_create(const char *pattern, size_t nr);

void
pcutils_wildcard_destroy(struct pcutils_wildcard *wildcard);

int
pcutils_wildcard_match(struct pcutils_wildcard *wildcard,
        const char *str, size_t nr, bool *matched);

/** the UTF-8 compliant version of strtoupper */
char *pcutils_strtoupper(const char *str, ssize_t len, size_t *new_len);

/** the UTF-8 compliant version of strtolower */
char *pcutils_strtolower(const char *str, ssize_t len, size_t *new_len);

/** the UTF-8 compliant version of strncasecmp */
int pcutils_strncasecmp(const char *s1, const char *s2, size_t n);

/** the UTF-8 compliant version of strcasestr */
char *pcutils_strcasestr(const char *haystack, const char *needle);

/** the UTF-8 compliant version of strreverse */
char *pcutils_strreverse(const char *str, ssize_t len, size_t nr_chars);

PCA_EXTERN_C_END

static inline int
pcutils_strcasecmp(const char *s1, const char *s2) {
    size_t n1 = strlen(s1);
    size_t n2 = strlen(s2);
    size_t n = n1 < n2 ? n1 : n2;
    int diff = pcutils_strncasecmp(s1, s2, n);
    if (diff)
        return diff;

    if (n1 == n2)
        return 0;

    return n1 < n2 ? -1 : 1;
}


/** Checks for a lowercase character. */
static inline int purc_islower(int c) {
    unsigned char uc = (unsigned char)c;
    return (uc >= 'a' && uc <= 'z');
}

/** Checks for a uppercase character. */
static inline int purc_isupper(int c) {
    unsigned char uc = (unsigned char)c;
    return (uc >= 'A' && uc <= 'Z');
}

/** Checks for an alphabetic character;
  * it is equivalent to (purc_isupper(c) || purc_islower(c)). */
static inline int purc_isalpha(int c) {
    return purc_islower(c) || purc_isupper(c);
}

/** Checks for a digit ('0' through '9'). */
static inline int purc_isdigit(int c) {
    unsigned char uc = (unsigned char)c;
    return (uc >= '0' && uc <= '9');
}

/** Checks for an alphanumeric character;
  * it is equivalent to (purc_isalpha(c) || purc_isdigit(c)). */
static inline int purc_isalnum(int c) {
    return purc_isalpha(c) || purc_isdigit(c);
}

/** Checks for a control character. */
static inline int purc_iscntrl(int c) {
    unsigned char uc = (unsigned char)c;
    return (uc < 0x20);
}

/** Checks for any printable character except space. */
static inline int purc_isgraph(int c) {
    unsigned char uc = (unsigned char)c;
    return (uc >= 0x21 && uc <= 0x7E);
}

/** Checks for any printable character including space. */
static inline int purc_isprint(int c) {
    unsigned char uc = (unsigned char)c;
    return ((uc >= 0x09 && uc <= 0x0D) || (uc >= 0x20 && uc <= 0x7E));
}

/** Checks for any printable character which is not a space or
  * an alphanumeric character. */
static inline int purc_ispunct(int c) {
    unsigned char uc = (unsigned char)c;
    return ((uc >= 0x21 && uc <= 0x2F) ||
            (uc >= 0x3A && uc <= 0x40) ||
            (uc >= 0x5B && uc <= 0x60) ||
            (uc >= 0x7B && uc <= 0x7E));
}

/** Checks for white-space characters: space, form-feed ('\f'), newline ('\n'),
  * carriage return ('\r'), horizontal tab ('\t'), and vertical tab ('\v'). */
static inline int purc_isspace(int c) {
    unsigned char uc = (unsigned char)c;
    return ((uc >= 0x09 && uc <= 0x0D) || (uc == 0x20));
}

/** Checks for hexadecimal digits, that is, one of
  * 0 1 2 3 4 5 6 7 8 9 a b c d e f A B C D E F. */
static inline int purc_isxdigit(int c) {
    unsigned char uc = (unsigned char)c;
    return ((uc >= '0' && uc <= '9') ||
            (uc >= 'a' && uc <= 'f') ||
            (uc >= 'A' && uc <= 'F'));
}

/** Checks whether c is a 7-bit unsigned char value that fits into
  * the ASCII character set. */
static inline int purc_isascii(int c) {
    unsigned char uc = (unsigned char)c;
    if (uc < 0x80)
        return 1;
    return 0;
}

/** Checks for a blank character; that is, a space or a tab. */
static inline int purc_isblank(int c) {
    unsigned char uc = (unsigned char)c;
    return (uc == ' ' || uc == '\t');
}

/** Returns the uppercase equivalent if the specified character is
  * an ASCII lowercase letter. Otherwise, it returns the character. */
static inline int purc_toupper(int c) {
    if (purc_islower(c)) {
        return c - 'a' + 'A';
    }
    return c;
}

/** Returns the lowercase equivalent if the specified character is
  * an ASCII uppercase letter. Otherwise, it returns the character. */
static inline int purc_tolower(int c) {
    if (purc_isupper(c)) {
        return c - 'A' + 'a';
    }
    return c;
}

#endif /* not defined PURC_PURC_UTILS_H */

