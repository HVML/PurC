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

#define HT_EXTRA_SIZE(ht) sizeof(*ht) + \
    (ht->size) * sizeof(*ht->table)

static inline void
grown(purc_variant_t obj, purc_variant_t key, purc_variant_t val)
{
    if (!list_empty(&obj->listeners))
        return;
    purc_atom_t msg_type = purc_atom_from_string("grown");
    PC_ASSERT(msg_type);

    struct list_head *p;
    list_for_each(p, &obj->listeners) {
        struct pcvar_listener *l;
        l = container_of(p, struct pcvar_listener, list_node);
        PC_ASSERT(l->handler);
        if (l->name != msg_type)
            continue;

        purc_variant_t args[] = {
            key,
            val,
        };
        bool ok = l->handler(obj, msg_type, l->ctxt,
            PCA_TABLESIZE(args), args);
        PC_ASSERT(ok);
    }
}

static inline void
shrunk(purc_variant_t obj, purc_variant_t key, purc_variant_t val)
{
    if (!list_empty(&obj->listeners))
        return;
    purc_atom_t msg_type = purc_atom_from_string("shrunk");
    PC_ASSERT(msg_type);

    struct list_head *p;
    list_for_each(p, &obj->listeners) {
        struct pcvar_listener *l;
        l = container_of(p, struct pcvar_listener, list_node);
        PC_ASSERT(l->handler);
        if (l->name != msg_type)
            continue;

        purc_variant_t args[] = {
            key,
            val,
        };
        bool ok = l->handler(obj, msg_type, l->ctxt,
            PCA_TABLESIZE(args), args);
        PC_ASSERT(ok);
    }
}

static inline void
change(purc_variant_t obj,
        purc_variant_t ko, purc_variant_t vo,
        purc_variant_t kn, purc_variant_t vn)
{
    if (!list_empty(&obj->listeners))
        return;
    purc_atom_t msg_type = purc_atom_from_string("change");
    PC_ASSERT(msg_type);

    struct list_head *p;
    list_for_each(p, &obj->listeners) {
        struct pcvar_listener *l;
        l = container_of(p, struct pcvar_listener, list_node);
        PC_ASSERT(l->handler);
        if (l->name != msg_type)
            continue;

        purc_variant_t args[] = {
            kn,
            vn,
            ko,
            vo,
        };
        bool ok = l->handler(obj, msg_type, l->ctxt,
            PCA_TABLESIZE(args), args);
        PC_ASSERT(ok);
    }
}

static inline struct pchash_table*
v_object_get_ht(purc_variant_t obj)
{
    struct pchash_table *ht = (struct pchash_table*)obj->sz_ptr[1];
    return ht;
}

static unsigned long
key_hash(const void *k)
{
    const char *key;
    key = purc_variant_get_string_const((purc_variant_t)k);
    return pchash_default_char_hash(key);
}

static int
key_equal(const void *k1, const void *k2)
{
    if (k1==k2)
        return 1;

    purc_variant_t key1 = (purc_variant_t)k1;
    purc_variant_t key2 = (purc_variant_t)k2;
    return purc_variant_compare(key1, key2) ? 0 : 1;
}

static purc_variant_t v_object_new_with_capacity(size_t initial_size)
{
    purc_variant_t var = pcvariant_get(PVT(_OBJECT));
    if (!var) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    var->type          = PVT(_OBJECT);
    var->flags         = PCVARIANT_FLAG_EXTRA_SIZE;

    if (initial_size==0)
        initial_size = HASHTABLE_DEFAULT_SIZE;

    struct pchash_table *ht;
    ht = pchash_table_new(initial_size, NULL,
            key_hash, key_equal);

    if (!ht) {
        pcvariant_put(var);
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }
    var->sz_ptr[1]     = (uintptr_t)ht;
    var->refc          = 1;

    return var;
}

static int
v_object_set(purc_variant_t obj, purc_variant_t k, purc_variant_t val)
{
    if (!k || !val) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }
    struct pchash_table *ht = v_object_get_ht(obj);
    struct pchash_entry *e  = pchash_table_lookup_entry(ht, k);
    if (e) {
        purc_variant_t ko  = (purc_variant_t)pchash_entry_k(e);
        purc_variant_t old = (purc_variant_t)pchash_entry_v(e);
        if (old==val) {
            return 0;
        }
        change(obj, ko, old, k, val);
        if (ko!=k) {
            purc_variant_ref(k);
            purc_variant_unref(ko);
        }
        e->k = k;
        e->v = val;
        purc_variant_ref(val);
        purc_variant_unref(old);
        return 0;
    } else {
        if (pchash_table_resize(ht, ht->count + 1) ||
            pchash_table_insert(ht, k, val))
        {
            pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
            return -1;
        }
        grown(obj, k, val);
        purc_variant_ref(k);
        purc_variant_ref(val);

        return 0;
    }
}

static int
v_object_set_kvs_n(purc_variant_t obj, size_t nr_kv_pairs,
    int is_c, va_list ap)
{
    purc_variant_t k, v;

    size_t i = 0;
    while (i<nr_kv_pairs) {
        if (is_c) {
            const char *k_c = va_arg(ap, const char*);
            k = purc_variant_make_string(k_c, true);
            if (!k)
                break;
        } else {
            k = va_arg(ap, purc_variant_t);
            if (!k || k->type!=PVT(_STRING)) {
                pcinst_set_error(PURC_ERROR_INVALID_VALUE);
                break;
            }
        }
        v = va_arg(ap, purc_variant_t);

        int r = v_object_set(obj, k, v);
        if (is_c)
            purc_variant_unref(k);

        if (r)
            break;
        ++i;
    }
    return i<nr_kv_pairs ? -1 : 0;
}

static int v_object_remove(purc_variant_t obj, purc_variant_t key)
{
    struct pchash_table *ht = v_object_get_ht(obj);
    struct pchash_entry *e  = pchash_table_lookup_entry(ht, key);
    if (!e) {
        pcinst_set_error(PCVARIANT_ERROR_NOT_FOUND);
        return false;
    }

    purc_variant_t k = (purc_variant_t)pchash_entry_k(e);
    PC_ASSERT(k);
    purc_variant_t v = (purc_variant_t)pchash_entry_v(e);
    PC_ASSERT(v);

    if (pchash_table_delete_entry(ht, e)) {
        PC_ASSERT(0);
        return false;
    }

    shrunk(obj, k, v);
    purc_variant_unref(k);
    purc_variant_unref(v);

    return true;
}

static purc_variant_t
pv_make_object_by_static_ckey_n (size_t nr_kv_pairs,
    const char* key0, purc_variant_t value0, va_list ap)
{
    PCVARIANT_CHECK_FAIL_RET((nr_kv_pairs==0 && key0==NULL && value0==NULL) ||
                         (nr_kv_pairs>0 && key0 && value0),
        PURC_VARIANT_INVALID);

    purc_variant_t obj = v_object_new_with_capacity(nr_kv_pairs);
    if (!obj)
        return PURC_VARIANT_INVALID;

    struct pchash_table *ht = v_object_get_ht(obj);
    PC_ASSERT(ht);

    do {
        int r;
        if (nr_kv_pairs > 0) {
            purc_variant_t k = purc_variant_make_string(key0, true);
            purc_variant_t v = value0;
            r = v_object_set(obj, k, v);
            purc_variant_unref(k);
            if (r)
                break;
        }

        if (nr_kv_pairs > 1) {
            r = v_object_set_kvs_n(obj, nr_kv_pairs-1, 1, ap);
            if (r)
                break;
        }

        size_t extra = HT_EXTRA_SIZE(ht);
        pcvariant_stat_set_extra_size(obj, extra);

        return obj;
    } while (0);

    // cleanup
    purc_variant_unref(obj);

    return PURC_VARIANT_INVALID;
}

purc_variant_t
purc_variant_make_object_by_static_ckey (size_t nr_kv_pairs,
    const char* key0, purc_variant_t value0, ...)
{
    purc_variant_t v;
    va_list ap;
    va_start(ap, value0);
    v = pv_make_object_by_static_ckey_n(nr_kv_pairs, key0, value0, ap);
    va_end(ap);

    return v;
}

static purc_variant_t
pv_make_object_n(size_t nr_kv_pairs,
    purc_variant_t key0, purc_variant_t value0, va_list ap)
{
    PCVARIANT_CHECK_FAIL_RET((nr_kv_pairs==0 && key0==NULL && value0==NULL) ||
                         (nr_kv_pairs>0 && key0 && value0),
        PURC_VARIANT_INVALID);

    purc_variant_t obj = v_object_new_with_capacity(nr_kv_pairs);
    if (!obj)
        return PURC_VARIANT_INVALID;

    struct pchash_table *ht = v_object_get_ht(obj);
    PC_ASSERT(ht);

    do {
        if (nr_kv_pairs > 0) {
            purc_variant_t v = value0;
            if (v_object_set(obj, key0, v))
                break;
        }

        if (nr_kv_pairs > 1) {
            int r = v_object_set_kvs_n(obj, nr_kv_pairs-1, 0, ap);
            if (r)
                break;
        }

        struct pchash_table *ht = v_object_get_ht(obj);
        size_t extra = HT_EXTRA_SIZE(ht);
        pcvariant_stat_set_extra_size(obj, extra);

        return obj;
    } while (0);

    // cleanup
    purc_variant_unref(obj);

    return PURC_VARIANT_INVALID;
}

purc_variant_t
purc_variant_make_object (size_t nr_kv_pairs,
    purc_variant_t key0, purc_variant_t value0, ...)
{
    purc_variant_t v;
    va_list ap;
    va_start(ap, value0);
    v = pv_make_object_n(nr_kv_pairs, key0, value0, ap);
    va_end(ap);

    return v;
}

void pcvariant_object_release (purc_variant_t value)
{
    struct pchash_table *ht = v_object_get_ht(value);

    struct pchash_entry *e, *tmp;
    pchash_foreach_safe(ht, e, tmp) {
        purc_variant_t k = (purc_variant_t)pchash_entry_k(e);
        purc_variant_t v = (purc_variant_t)pchash_entry_v(e);
        int r = pchash_table_delete_entry(ht, e);
        PC_ASSERT(r==0);
        purc_variant_unref(k);
        purc_variant_unref(v);
    }
    pchash_table_free(ht);
    value->sz_ptr[1] = (uintptr_t)NULL; // say no to double free

    pcvariant_stat_set_extra_size(value, 0);
}

/* VWNOTE: unnecessary
int pcvariant_object_compare (purc_variant_t lv, purc_variant_t rv)
{
    // only called via purc_variant_compare
    struct pchash_table *lht = v_object_get_ht(lv);
    struct pchash_table *rht = v_object_get_ht(rv);

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
*/

purc_variant_t purc_variant_object_get(purc_variant_t obj, purc_variant_t key)
{
    PCVARIANT_CHECK_FAIL_RET((obj && obj->type==PVT(_OBJECT) &&
        obj->sz_ptr[1] && key),
        PURC_VARIANT_INVALID);

    struct pchash_table *ht = v_object_get_ht(obj);
    struct pchash_entry *e  = pchash_table_lookup_entry(ht, key);
    if (!e) {
        pcinst_set_error(PCVARIANT_ERROR_NOT_FOUND);
        return PURC_VARIANT_INVALID;
    }
    purc_variant_t v = (purc_variant_t)pchash_entry_v(e);
    PC_ASSERT(v);

    return v;
}

bool purc_variant_object_set (purc_variant_t obj,
    purc_variant_t key, purc_variant_t value)
{
    PCVARIANT_CHECK_FAIL_RET(obj && obj->type==PVT(_OBJECT) &&
        obj->sz_ptr[1] && key && value,
        false);

    int r = v_object_set(obj, key, value);

    return r ? false : true;
}

bool purc_variant_object_remove(purc_variant_t obj, purc_variant_t key)
{
    PCVARIANT_CHECK_FAIL_RET(obj && obj->type==PVT(_OBJECT) &&
        obj->sz_ptr[1] && key,
        false);

    if (v_object_remove(obj, key))
        return false;

    return true;
}

bool purc_variant_object_size (purc_variant_t obj, size_t *sz)
{
    PC_ASSERT(obj && sz);

    PCVARIANT_CHECK_FAIL_RET(obj->type == PVT(_OBJECT) && obj->sz_ptr[1],
        false);

    struct pchash_table *ht = v_object_get_ht(obj);
    int nr = pchash_table_length(ht);

    PC_ASSERT(nr >= 0);
    *sz = (size_t)nr;

    return true;
}

struct purc_variant_object_iterator {
    purc_variant_t           obj;

    struct pchash_entry     *curr;
    struct pchash_entry     *next, *prev;
};

#define refresh_iterator(it) do {            \
    if (it->curr) {                          \
        it->next = it->curr->next;           \
        it->prev = it->curr->prev;           \
    }                                        \
} while (0)

struct purc_variant_object_iterator*
purc_variant_object_make_iterator_begin (purc_variant_t object) {
    PCVARIANT_CHECK_FAIL_RET((object && object->type==PVT(_OBJECT) &&
        object->sz_ptr[1]),
        NULL);

    struct pchash_table *ht = v_object_get_ht(object);
    if (ht->count==0) {
        pcinst_set_error(PCVARIANT_ERROR_NOT_FOUND);
        return NULL;
    }

    struct purc_variant_object_iterator *it;
    it = (struct purc_variant_object_iterator*)malloc(sizeof(*it));
    if (!it) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    it->obj = object;
    it->curr = ht->head;
    refresh_iterator(it);

    return it;
}

struct purc_variant_object_iterator*
purc_variant_object_make_iterator_end (purc_variant_t object) {
    PCVARIANT_CHECK_FAIL_RET((object && object->type==PVT(_OBJECT) &&
        object->sz_ptr[1]),
        NULL);

    struct pchash_table *ht = v_object_get_ht(object);
    if (ht->count==0) {
        pcinst_set_error(PCVARIANT_ERROR_NOT_FOUND);
        return NULL;
    }

    struct purc_variant_object_iterator *it;
    it = (struct purc_variant_object_iterator*)malloc(sizeof(*it));
    if (!it) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    it->obj = object;
    it->curr = ht->tail;
    refresh_iterator(it);

    return it;
}

void
purc_variant_object_release_iterator (struct purc_variant_object_iterator* it)
{
    if (!it)
        return;
    PC_ASSERT(it->obj);

    it->obj = NULL;
    it->curr = NULL;
    it->next = NULL;
    it->prev = NULL;

    free(it);
}

bool
purc_variant_object_iterator_next (struct purc_variant_object_iterator* it)
{
    PCVARIANT_CHECK_FAIL_RET(it, false);

    PC_ASSERT(it->obj);

    if (it->curr) {
        it->curr = it->curr->next;
    }
    refresh_iterator(it);

    if (it->curr)
        return true;
    pcinst_set_error(PCVARIANT_ERROR_NOT_FOUND);
    return false;
}

bool
purc_variant_object_iterator_prev (struct purc_variant_object_iterator* it)
{
    PCVARIANT_CHECK_FAIL_RET(it, false);

    PC_ASSERT(it->obj);

    if (it->curr) {
        it->curr = it->curr->prev;
    }
    refresh_iterator(it);

    if (it->curr)
        return true;
    pcinst_set_error(PCVARIANT_ERROR_NOT_FOUND);
    return false;
}

purc_variant_t
purc_variant_object_iterator_get_key (struct purc_variant_object_iterator* it)
{
    PCVARIANT_CHECK_FAIL_RET(it, NULL);

    PC_ASSERT(it->obj);
    PC_ASSERT(it->curr);

    purc_variant_t k = (purc_variant_t)pchash_entry_k(it->curr);
    PC_ASSERT(purc_variant_is_string(k));
    return k;
}

purc_variant_t
purc_variant_object_iterator_get_value(struct purc_variant_object_iterator* it)
{
    PCVARIANT_CHECK_FAIL_RET(it, NULL);

    PC_ASSERT(it->obj);
    PC_ASSERT(it->curr);

    purc_variant_t  v = (purc_variant_t)pchash_entry_v(it->curr);
    return v;
}

