/**
 * @file purc-utils.h
 * @author Vincent Wei
 * @date 2021/07/05
 * @brief The API for utilities.
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

#ifndef PURC_PURC_UTILS_H
#define PURC_PURC_UTILS_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>  /* TODO: for ssize_t on MacOS */

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
 * purc_atom_to_string() or purc_atom_to_string_ex().
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

/** The atom bucket identifier reserved for user usage */
#define PURC_ATOM_BUCKET_USER   (PURC_ATOM_BUCKETS_NR - 1)

PCA_EXTERN_C_BEGIN

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
PCA_EXPORT purc_atom_t
purc_atom_from_string_ex(int bucket, const char* string);

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
    return purc_atom_from_string_ex(0, string);
}

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
PCA_EXPORT purc_atom_t
purc_atom_from_static_string_ex(int bucket, const char* string);

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
    return purc_atom_from_static_string_ex(0, string);
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
 * pcutils_get_next_fibonacci_number:
 *
 * Gets next fibonacci sequence number.
 *
 * @param n: current number
 *
 * @return the next fibonacci sequence number.
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

/** Check validation of Unicode characters in a UTF-8 encoded string. */
PCA_EXPORT bool
pcutils_string_check_utf8_len(const char* str, size_t max_len,
        size_t *nr_chars, const char **end);

/** Check validation of Unicode characters in a UTF-8 encoded string. */
PCA_EXPORT bool
pcutils_string_check_utf8(const char *str, ssize_t max_len,
        size_t *nr_chars, const char **end);

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
 * Assemble a broken-down URL.
 *
 * @param broken_down The pointer to a broken-down URL structure.
 *
 * Returns: The assembled URL string (not percent escaped) on success,
 *  otherwise @null.
 */
PCA_EXPORT char *
pcutils_url_assemble(const struct purc_broken_down_url *broken_down);

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

PCA_EXTERN_C_END

#endif /* not defined PURC_PURC_UTILS_H */

