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
 * no matter via `pcvariant_object_release` or `purc_variant_object_remove`
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
    free(k);
    purc_variant_unref(v);
    // no need to free e
    // hashtable will take care
}

static purc_variant_t _variant_object_new_with_capacity(size_t initial_size)
{
    purc_variant_t var = pcvariant_get(PVT(_OBJECT));
    if (!var) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    var->type          = PVT(_OBJECT);

    if (initial_size==0)
        initial_size = HASHTABLE_DEFAULT_SIZE;

    struct pchash_table *ht = pchash_kchar_table_new(initial_size, _object_kv_free);
    if (!ht) {
        pcvariant_put(var);
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }
    var->sz_ptr[1]     = (uintptr_t)ht;
    var->refc          = 1;
    // an empty variant is born

    return var;
}

static int _variant_object_set(purc_variant_t obj, const char *k, purc_variant_t val)
{
    int r;
    struct pchash_table *ht = (struct pchash_table*)obj->sz_ptr[1];
    struct pchash_entry *e  = pchash_table_lookup_entry(ht, k);
    if (e) {
        if (pchash_entry_v(e)==val)
            return 0;
        r = pchash_table_delete_entry(ht, e);
        PC_ASSERT(r==0);
    }
    if (pchash_table_resize(ht, pchash_table_length(ht)+1) ||
        pchash_table_insert(ht, k, val))
    {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }

    return 0;
}

static int _variant_object_set_kvs_n(purc_variant_t obj, size_t nr_kv_pairs, int _c, va_list ap)
{
    const char *k_c;
    purc_variant_t k, v;

    size_t i = 1;
    while (i<nr_kv_pairs) {
        if (_c) {
            k_c = va_arg(ap, const char*);
        } else {
            k = va_arg(ap, purc_variant_t);
            if (!k) {
                pcinst_set_error(PURC_ERROR_INVALID_VALUE);
                break;
            }
            k_c = purc_variant_get_string_const(k);
            PC_ASSERT(k_c);
        }
        v = va_arg(ap, purc_variant_t);
        if (!k_c || !*k_c || !v) {
            pcinst_set_error(PURC_ERROR_INVALID_VALUE);
            break;
        }

        if (_variant_object_set(obj, k_c, v)) {
            // errcode already by _variant_object_set
            break;
        }
        // add ref
        purc_variant_ref(v);
    }
    return i<nr_kv_pairs ? -1 : 0;
}

static int _variant_object_remove(purc_variant_t obj, const char *key)
{
    struct pchash_table *ht = (struct pchash_table*)obj->sz_ptr[1];

    if (pchash_table_delete(ht, key)) {
        pcinst_set_error(PCVARIANT_ERROR_NOT_FOUND);
        return false;
    }

    return true;
}

purc_variant_t purc_variant_make_object_c (size_t nr_kv_pairs, const char* key0, purc_variant_t value0, ...)
{
    PCVARIANT_CHECK_FAIL_RET((nr_kv_pairs==0 && key0==NULL && value0==NULL) ||
                         (nr_kv_pairs>0 && key0 && *key0),
        PURC_VARIANT_INVALID);

    purc_variant_t obj = _variant_object_new_with_capacity(nr_kv_pairs);
    if (!obj) {
        // errcode already by _variant_object_set
        return PURC_VARIANT_INVALID;
    }
    if (nr_kv_pairs==0) {
        // object with no kv
        return obj;
    }

    do {
        const char     *k = key0;
        purc_variant_t  v = value0;
        if (_variant_object_set(obj, k, v)) {
            // errcode already by _variant_object_set
            break;
        }
        // add ref
        purc_variant_ref(v);

        if (nr_kv_pairs>1) {
            va_list ap;
            va_start(ap, value0);
            int r = _variant_object_set_kvs_n(obj, nr_kv_pairs-1, 1, ap);
            va_end(ap);
            if (r) {
                // errcode already by _variant_object_set
                break;
            }
        }

        return obj;
    } while (0);

    // cleanup
    --obj->refc;
    pcvariant_object_release(obj);
    // todo: use macro instead
    free(obj);

    return PURC_VARIANT_INVALID;
}

purc_variant_t purc_variant_make_object (size_t nr_kv_pairs, purc_variant_t key0, purc_variant_t value0, ...)
{
    PCVARIANT_CHECK_FAIL_RET((nr_kv_pairs==0 && key0==NULL && value0==NULL) ||
                         (nr_kv_pairs>0 && key0 && value0),
        PURC_VARIANT_INVALID);

    purc_variant_t obj = _variant_object_new_with_capacity(nr_kv_pairs);
    if (!obj) {
        // errcode already by _variant_object_set
        return PURC_VARIANT_INVALID;
    }
    if (nr_kv_pairs==0) {
        // object with no kv
        return obj;
    }

    do {
        const char *k = purc_variant_get_string_const(key0);
        purc_variant_t v = value0;
        if (!k) {
            pcinst_set_error(PURC_ERROR_INVALID_VALUE);
            break;
        }
        if (_variant_object_set(obj, k, v)) {
            // errcode already by _variant_object_set
            break;
        }
        // add ref
        purc_variant_ref(v);

        if (nr_kv_pairs>1) {
            va_list ap;
            va_start(ap, value0);
            int r = _variant_object_set_kvs_n(obj, nr_kv_pairs-1, 0, ap);
            va_end(ap);
            if (r) {
                // errcode already by _variant_object_set
                break;
            }
        }

        return obj;
    } while (0);

    // cleanup
    --obj->refc;
    pcvariant_object_release(obj);
    // todo: use macro instead
    free(obj);

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
    PCVARIANT_CHECK_FAIL_RET((obj && obj->type==PVT(_OBJECT) && obj->sz_ptr[1] && key),
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
    PCVARIANT_CHECK_FAIL_RET((obj && obj->type==PVT(_OBJECT) && obj->sz_ptr[1] && key && *key && value),
        false);

    if (_variant_object_set(obj, key, value)) {
        // errcode already by _variant_object_set
        return false;
    }
    // add ref
    purc_variant_ref(value);

    return true;
}

bool purc_variant_object_remove_c (purc_variant_t obj, const char* key)
{
    PCVARIANT_CHECK_FAIL_RET((obj && obj->type==PVT(_OBJECT) && obj->sz_ptr[1] && key && *key),
        false);

    if (_variant_object_remove(obj, key)) {
        // errcode already by _variant_object_remove
        return false;
    }

    return true;
}

size_t purc_variant_object_get_size (const purc_variant_t obj)
{
    PCVARIANT_CHECK_FAIL_RET((obj && obj->type==PVT(_OBJECT) && obj->sz_ptr[1]),
        (size_t)-1);

    struct pchash_table *ht = (struct pchash_table*)obj->sz_ptr[1];

    int nr = pchash_table_length(ht);
    PC_ASSERT(nr>=0);

    return (size_t)nr;
}

struct purc_variant_object_iterator {
    purc_variant_t           obj;

    struct pchash_entry     *curr;
};

struct purc_variant_object_iterator* purc_variant_object_make_iterator_begin (purc_variant_t object) {
    PCVARIANT_CHECK_FAIL_RET((object && object->type==PVT(_OBJECT) && object->sz_ptr[1]),
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
    PC_ASSERT(v);
    purc_variant_ref(v);

    return it;
}

struct purc_variant_object_iterator* purc_variant_object_make_iterator_end (purc_variant_t object) {
    PCVARIANT_CHECK_FAIL_RET((object && object->type==PVT(_OBJECT) && object->sz_ptr[1]),
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
    PC_ASSERT(v);
    purc_variant_ref(v);

    return it;
}

void purc_variant_object_release_iterator (struct purc_variant_object_iterator* it) {
    if (!it)
        return;
    PC_ASSERT(it->obj);

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
    PCVARIANT_CHECK_FAIL_RET(it, false);

    PC_ASSERT(it->obj);

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
    PCVARIANT_CHECK_FAIL_RET(it, false);

    PC_ASSERT(it->obj);

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
    PCVARIANT_CHECK_FAIL_RET(it, NULL);

    PC_ASSERT(it->obj);
    PC_ASSERT(it->curr);

    return (const char*)pchash_entry_k(it->curr);
}

purc_variant_t purc_variant_object_iterator_get_value (struct purc_variant_object_iterator* it) {
    PCVARIANT_CHECK_FAIL_RET(it, NULL);

    PC_ASSERT(it->obj);
    PC_ASSERT(it->curr);

    purc_variant_t  v = (purc_variant_t)pchash_entry_v(it->curr);
    // not add ref
    return v;
}

