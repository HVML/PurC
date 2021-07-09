/**
 * @file variant-object.c
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
 * object holds optionally key-value-pairs,
 * with key as c string, value as variant
 * once a purc_variant_t value is added into object,
 * no matter via `purc_variant_make_object` or `purc_variant_object_set/...`,
 * this value's ref + 1
 * once a purc_variant_t value in object get removed,
 * no matter via `_variant_object_release` or `purc_variant_object_remove`
 * or even implicitly being overwritten by `purc_variant_object_set/...`,
 * this value's ref - 1
 * note: value can be added into object for more than 1 times,
 *       but being noted, ref + 1 once it gets added
 *
 * thinking: shall we recursively check if there's ref-loop among object and it's
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

void pcvariant_object_release (purc_variant_t value)
{
    struct pchash_table *ht = (struct pchash_table*)value->sz_ptr[1];
    pchash_table_free(ht);
    value->sz_ptr[1] = (uintptr_t)NULL; // say no to double free
}

int pcvariant_object_compare (purc_variant_t lv, purc_variant_t rv)
{
    // only called via purc_variant_compare
    struct pchash_table *lht = (struct pchash_table*)lv->sz_ptr[1];
    struct pchash_table *rht = (struct pchash_table*)rv->sz_ptr[1];

    struct pchash_entry *lcurr = lht->head;
    struct pchash_entry *rcurr = rht->head;

    for (; lcurr && rcurr; lcurr=lcurr->next, rcurr=rcurr->next) {
        int r = pcvariant_object_compare(
                    (purc_variant_t)pchash_entry_v(lcurr),
                    (purc_variant_t)pchash_entry_v(rcurr));
        if (r)
            return r;
    }

    return lcurr ? 1 : -1;
}

purc_variant_t purc_variant_object_get_c (purc_variant_t obj, const char* key)
{
    PCVARIANT_CHECK_ARGS((obj && obj->type==PVT(_OBJECT) && obj->sz_ptr[1] && key),
        PURC_VARIANT_INVALID);

    struct pchash_table *ht = (struct pchash_table*)obj->sz_ptr[1];
    void *v = NULL;
    if (!pchash_table_lookup_ex(ht, key, &v)) {
        pcinst_set_error(PCVARIANT_ERROR_NOT_FOUND);
        return PURC_VARIANT_INVALID;
    }
    if (!v) {
        // seems like internal logic error
        // abort or fail-return?
        // fail-return for now
        pcinst_set_error(PCVARIANT_ERROR_NOT_FOUND);
        return PURC_VARIANT_INVALID;
    }

    return (purc_variant_t)v;
}

bool purc_variant_object_set_c (purc_variant_t obj, const char* key, purc_variant_t value)
{
    PCVARIANT_CHECK_ARGS((obj && obj->type==PVT(_OBJECT) && obj->sz_ptr[1] && key && *key && value),
        false);

    struct pchash_table *ht = (struct pchash_table*)obj->sz_ptr[1];

    // to avoid self-overwritten
    purc_variant_ref(value);
    int t = pchash_table_insert(ht, key, value);
    purc_variant_unref(value);

    if (t) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return false;
    }

    // add ref
    purc_variant_ref(value);

    return true;
}

bool purc_variant_object_remove_c (purc_variant_t obj, const char* key)
{
    PCVARIANT_CHECK_ARGS((obj && obj->type==PVT(_OBJECT) && obj->sz_ptr[1] && key && *key),
        false);

    struct pchash_table *ht = (struct pchash_table*)obj->sz_ptr[1];

    if (pchash_table_delete(ht, key)) {
        pcinst_set_error(PCVARIANT_ERROR_NOT_FOUND);
        return false;
    }

    return true;
}

size_t purc_variant_object_get_size (const purc_variant_t obj)
{
    PCVARIANT_CHECK_ARGS((obj && obj->type==PVT(_OBJECT) && obj->sz_ptr[1]),
        (size_t)-1);

    struct pchash_table *ht = (struct pchash_table*)obj->sz_ptr[1];

    int nr = pchash_table_length(ht);
    PURC_VARIANT_ASSERT(nr>=0);

    return (size_t)nr;
}

struct purc_variant_object_iterator {
    purc_variant_t           obj;

    struct pchash_entry     *curr;
};

struct purc_variant_object_iterator* purc_variant_object_make_iterator_begin (purc_variant_t object) {
    PCVARIANT_CHECK_ARGS((object && object->type==PVT(_OBJECT) && object->sz_ptr[1]),
        NULL);

    struct pchash_table *ht = (struct pchash_table*)object->sz_ptr[1];
    if (ht->head==NULL) {
        pcinst_set_error(PCVARIANT_ERROR_NOT_FOUND);
        return NULL;
    }

    struct purc_variant_object_iterator *it = (struct purc_variant_object_iterator*)malloc(sizeof(*it));
    if (!it) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    it->obj = object;
    purc_variant_ref(object);

    it->curr = ht->head;

    purc_variant_t v = (purc_variant_t)pchash_entry_v(it->curr);
    PURC_VARIANT_ASSERT(v);
    purc_variant_ref(v);

    return it;
}

struct purc_variant_object_iterator* purc_variant_object_make_iterator_end (purc_variant_t object) {
    PCVARIANT_CHECK_ARGS((object && object->type==PVT(_OBJECT) && object->sz_ptr[1]),
        NULL);

    struct pchash_table *ht = (struct pchash_table*)object->sz_ptr[1];
    if (ht->tail==NULL) {
        pcinst_set_error(PCVARIANT_ERROR_NOT_FOUND);
        return NULL;
    }

    struct purc_variant_object_iterator *it = (struct purc_variant_object_iterator*)malloc(sizeof(*it));
    if (!it) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    it->obj = object;
    purc_variant_ref(object);

    it->curr = ht->tail;

    purc_variant_t v = (purc_variant_t)pchash_entry_v(it->curr);
    PURC_VARIANT_ASSERT(v);
    purc_variant_ref(v);

    return it;
}

void purc_variant_object_release_iterator (struct purc_variant_object_iterator* it) {
    if (!it)
        return;
    PURC_VARIANT_ASSERT(it->obj);

    purc_variant_unref(it->obj);
    it->obj = NULL;

    if (it->curr) {
        purc_variant_t v = (purc_variant_t)pchash_entry_v(it->curr);
        purc_variant_unref(v);
        it->curr = NULL;
    }

    free(it);
}

bool purc_variant_object_iterator_next (struct purc_variant_object_iterator* it) {
    PCVARIANT_CHECK_ARGS(it, false);

    PURC_VARIANT_ASSERT(it->obj);

    if (it->curr) {
        purc_variant_t v = (purc_variant_t)pchash_entry_v(it->curr);
        purc_variant_unref(v);
        it->curr = it->curr->next;
    }

    if (it->curr) {
        purc_variant_t v = (purc_variant_t)pchash_entry_v(it->curr);
        purc_variant_ref(v);
        return true;
    }

    pcinst_set_error(PCVARIANT_ERROR_NOT_FOUND);
    return false;
}

bool purc_variant_object_iterator_prev (struct purc_variant_object_iterator* it) {
    PCVARIANT_CHECK_ARGS(it, false);

    PURC_VARIANT_ASSERT(it->obj);

    if (it->curr) {
        purc_variant_t v = (purc_variant_t)pchash_entry_v(it->curr);
        purc_variant_unref(v);
        it->curr = it->curr->prev;
    }

    if (it->curr) {
        purc_variant_t v = (purc_variant_t)pchash_entry_v(it->curr);
        purc_variant_ref(v);
        return true;
    }

    pcinst_set_error(PCVARIANT_ERROR_NOT_FOUND);
    return false;
}

const char *purc_variant_object_iterator_get_key (struct purc_variant_object_iterator* it) {
    PCVARIANT_CHECK_ARGS(it, NULL);

    PURC_VARIANT_ASSERT(it->obj);
    PURC_VARIANT_ASSERT(it->curr);

    return (const char*)pchash_entry_k(it->curr);
}

purc_variant_t purc_variant_object_iterator_get_value (struct purc_variant_object_iterator* it) {
    PCVARIANT_CHECK_ARGS(it, NULL);

    PURC_VARIANT_ASSERT(it->obj);
    PURC_VARIANT_ASSERT(it->curr);

    purc_variant_t  v = (purc_variant_t)pchash_entry_v(it->curr);
    // not add ref
    return v;
}

