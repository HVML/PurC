/**
 * @file variant.c
 * @author Xu Xiaohong (freemine)
 * @date 2021/07/08
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


#include "config.h"
#include "private/variant.h"
#include "private/hashtable.h"
#include "private/errors.h"
#include "purc-errors.h"
#include "variant-internals.h"


#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>


/*
 * array holds purc_variant_t values
 * once a purc_variant_t value is added into array,
 * no matter via `purc_variant_make_array` or `purc_variant_array_set/append/...`,
 * this value's ref + 1
 * once a purc_variant_t value in array get removed,
 * no matter via `_variant_array_release` or `purc_variant_array_remove`
 * or even implicitly being overwritten by `purc_variant_array_set/append/...`,
 * this value's ref - 1
 * note: value can be added into array for more than 1 times,
 *       but being noted, ref + 1 once it gets added
 *
 * thinking: shall we recursively check if there's ref-loop among array and it's
 *           children element?
 */
static void _object_kv_free(struct pchash_entry *e)
{
    char           *k = (char*)pchash_entry_k(e);
    purc_variant_t  v = (purc_variant_t)pchash_entry_v(e);
    if (!e->k_is_constant) {
        free(k);
    }
    purc_variant_unref(v);
    // no need to free e
    // hashtable will take care
}

purc_variant_t purc_variant_make_object_c (size_t nr_kv_pairs, const char* key0, purc_variant_t value0, ...)
{
    PCVARIANT_CHECK_ARGS((nr_kv_pairs==0 && key0==NULL && value0==NULL) ||
                         (nr_kv_pairs>0 && key0 && *key0),
        PURC_VARIANT_INVALID);

    // later, we'll use MACRO rather than malloc directly
    purc_variant_t var = (purc_variant_t)malloc(sizeof(*var));
    if (!var) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    do {
        var->type          = PVT(_OBJECT);
        var->refc          = 1;

        size_t initial_size = HASHTABLE_DEFAULT_SIZE;
        if (nr_kv_pairs)
            initial_size = nr_kv_pairs;

        struct pchash_table *ht = pchash_kchar_table_new(initial_size, _object_kv_free);

        if (!ht)
            break;
        var->sz_ptr[1]     = (uintptr_t)ht;

        if (nr_kv_pairs==0) {
            // object with no kv
            return var;
        }

        va_list ap;
        va_start(ap, value0);

        const char     *k = key0;
        purc_variant_t  v = value0;
        if (pchash_table_insert(ht, k, v)) {
            pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
            break;
        }
        // add ref
        purc_variant_ref(v);

        size_t i = 1;
        while (i<nr_kv_pairs) {
            k = va_arg(ap, const char*);
            v = va_arg(ap, purc_variant_t);
            if (!k || !*k || !v) {
                pcinst_set_error(PURC_ERROR_INVALID_VALUE);
                break;
            }

            if (pchash_table_insert(ht, k, v)) {
                pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
                break;
            }
            // add ref
            purc_variant_ref(v);
        }
        va_end(ap);

        if (i<nr_kv_pairs)
            break;

        return var;
    } while (0);

    // cleanup
    struct pchash_table *ht = (struct pchash_table*)var->sz_ptr[1];
    pchash_table_free(ht);
    var->sz_ptr[1] = (uintptr_t)NULL; // say no to double free
    // todo: use macro instead
    free(var);

    return PURC_VARIANT_INVALID;
}

purc_variant_t purc_variant_make_object (size_t nr_kv_pairs, purc_variant_t key0, purc_variant_t value0, ...)
{
    PCVARIANT_CHECK_ARGS((nr_kv_pairs==0 && key0==NULL && value0==NULL) ||
                         (nr_kv_pairs>0 && key0 && value0),
        PURC_VARIANT_INVALID);

    // later, we'll use MACRO rather than malloc directly
    purc_variant_t var = (purc_variant_t)malloc(sizeof(*var));
    if (!var) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    do {
        var->type          = PVT(_OBJECT);
        var->refc          = 1;

        size_t initial_size = HASHTABLE_DEFAULT_SIZE;
        if (nr_kv_pairs>initial_size)
            initial_size = nr_kv_pairs;

        struct pchash_table *ht = pchash_kchar_table_new(initial_size, _object_kv_free);

        if (!ht)
            break;
        var->sz_ptr[1]     = (uintptr_t)ht;

        if (nr_kv_pairs==0) {
            // object with no kv
            return var;
        }

        va_list ap;
        va_start(ap, value0);

        purc_variant_t k = key0;
        purc_variant_t v = value0;
        const char *key = purc_variant_get_string_const (k);
        if (!key) {
            pcinst_set_error(PURC_ERROR_INVALID_VALUE);
            break;
        }
        if (pchash_table_insert(ht, key, v)) {
            pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
            break;
        }
        // add ref
        purc_variant_ref(v);

        size_t i = 1;
        while (i<nr_kv_pairs) {
            k = va_arg(ap, purc_variant_t);
            v = va_arg(ap, purc_variant_t);
            if (!k || !v) {
                pcinst_set_error(PURC_ERROR_INVALID_VALUE);
                break;
            }
            key = purc_variant_get_string_const (k);
            if (!key) {
                pcinst_set_error(PURC_ERROR_INVALID_VALUE);
                break;
            }

            if (pchash_table_insert(ht, key, v)) {
                pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
                break;
            }
            // add ref
            purc_variant_ref(v);
        }
        va_end(ap);

        if (i<nr_kv_pairs)
            break;

        return var;
    } while (0);

    // cleanup
    struct pchash_table *ht = (struct pchash_table*)var->sz_ptr[1];
    pchash_table_free(ht);
    var->sz_ptr[1] = (uintptr_t)NULL; // say no to double free
    // todo: use macro instead
    free(var);

    return PURC_VARIANT_INVALID;
}

#if 0

/**
 * Gets the value by key from an object with key as c string
 *
 * @param obj: the variant data of obj type
 * @param key: the key of key-value pair
 *
 * Returns: A purc_variant_t on success, or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
bool purc_variant_object_get_c (purc_variant_t obj, const char* key);

/**
 * Gets the value by key from an object with key as another variant
 *
 * @param obj: the variant data of obj type
 * @param key: the key of key-value pair
 *
 * Returns: A purc_variant_t on success, or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.0.1
 */
bool purc_variant_object_get (purc_variant_t obj, purc_variant_t key);

/**
 * Sets the value by key in an object with key as c string
 *
 * @param obj: the variant data of obj type
 * @param key: the key of key-value pair
 * @param value: the value of key-value pair
 *
 * Returns: True on success, otherwise False.
 *
 * Since: 0.0.1
 */
bool purc_variant_object_set_c (purc_variant_t obj, const char* key, purc_variant_t value);

/**
 * Sets the value by key in an object with key as another variant
 *
 * @param obj: the variant data of obj type
 * @param key: the key of key-value pair
 * @param value: the value of key-value pair
 *
 * Returns: True on success, otherwise False.
 *
 * Since: 0.0.1
 */
bool purc_variant_object_set (purc_variant_t obj, purc_variant_t key, purc_variant_t value);

/**
 * Remove a key-value pair from an object by key with key as c string
 *
 * @param obj: the variant data of obj type
 * @param key: the key of key-value pair
 *
 * Returns: True on success, otherwise False.
 *
 * Since: 0.0.1
 */
bool purc_variant_object_remove_c (purc_variant_t obj, const char* key);

/**
 * Remove a key-value pair from an object by key with key as another variant
 *
 * @param obj: the variant data of obj type
 * @param key: the key of key-value pair
 *
 * Returns: True on success, otherwise False.
 *
 * Since: 0.0.1
 */
bool purc_variant_object_remove (purc_variant_t obj, purc_variant_t key);

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

#endif // 0

