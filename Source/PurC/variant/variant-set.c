/**
 * @file variant-set.c
 * @author Xu Xiaohong (freemine)
 * @date 2021/07/09
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
#include "private/list.h"
#include "private/hashtable.h"
#include "private/errors.h"
#include "purc-errors.h"
#include "variant-internals.h"


#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

static inline variant_set_t _pcv_set_get_data(purc_variant_t set)
{
    return (variant_set_t)set->sz_ptr[1];
}

static inline size_t _variant_set_get_extra_size(variant_set_t set)
{
    size_t extra = 0;
    if (set->unique_key) {
        extra += strlen(set->unique_key) + 1;
        extra += sizeof(*set->keynames) * set->nr_keynames;
    }
    size_t sz_record = sizeof(struct obj_node) +
        sizeof(purc_variant_t) * set->nr_keynames;
    size_t count = pcutils_arrlist_length(set->arr);
    extra += sz_record * count;

    return extra;
}

static inline void _pcv_set_set_data(purc_variant_t set, variant_set_t data)
{
    set->sz_ptr[1]     = (uintptr_t)data;
}

static int _variant_set_keyvals_cmp (const void *k1, const void *k2, void *ptr)
{
    purc_variant_t *kvs1 = (purc_variant_t*)k1;
    purc_variant_t *kvs2 = (purc_variant_t*)k2;
    variant_set_t   set  = (variant_set_t)ptr;

    int diff = 0;
    for (size_t i=0; i<set->nr_keynames; ++i) {
        purc_variant_t kv1 = kvs1[i];
        purc_variant_t kv2 = kvs2[i];
        PC_ASSERT(kv1 != PURC_VARIANT_INVALID);
        PC_ASSERT(kv2 != PURC_VARIANT_INVALID);
        if (0) {
            diff = purc_variant_compare(kv1, kv2);
        } else {
            int r1=0, r2=0;
            char *s1 = NULL, *s2 = NULL;

            r1 = purc_variant_stringify_alloc(&s1, kv1);
            PC_ASSERT(r1 != -1); // FIXME: oom exception
            r2 = purc_variant_stringify_alloc(&s2, kv2);
            PC_ASSERT(r2 != -1); // FIXME: oom exception

            diff = strcmp(s1, s2);

            free(s1);
            free(s2);
        }
        if (diff)
            break;
    }

    return diff;
}

static int _variant_set_init(variant_set_t set, const char *unique_key)
{
    PC_ASSERT(unique_key);

    set->objs = RB_ROOT;

    size_t initial_size = ARRAY_LIST_DEFAULT_SIZE;
    set->arr = pcutils_arrlist_new_ex(NULL, initial_size);
    if (!set->arr) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }

    if (!*unique_key) {
        // empty key
        set->nr_keynames = 1;
        return 0;
    }

    set->unique_key = strdup(unique_key);
    if (!set->unique_key) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }

    size_t n = strlen(set->unique_key);
    set->keynames = (char**)calloc(n, sizeof(*set->keynames));
    if (!set->keynames) {
        free(set->unique_key);
        set->unique_key = NULL;
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }

    strcpy(set->unique_key, unique_key);
    char *ctx = set->unique_key;
    char *tok = strtok_r(ctx, " ", &ctx);
    size_t idx = 0;
    while (tok) {
        set->keynames[idx++] = tok;
        tok = strtok_r(ctx, " ", &ctx);
    }

    if (idx==0) {
        // no content in key
        free(set->unique_key);
        set->unique_key = NULL;
        set->nr_keynames = 1;
        return 0;
    }

    PC_ASSERT(idx>0);
    set->nr_keynames = idx;

    return 0;
}

static int
_variant_set_cache_obj_keyval(variant_set_t set,
    purc_variant_t value, purc_variant_t *kvs)
{
    PC_ASSERT(value != PURC_VARIANT_INVALID);
    PC_ASSERT(set->nr_keynames);

    if (set->unique_key) {
        for (size_t i=0; i<set->nr_keynames; ++i) {
            purc_variant_t v;
            v = purc_variant_object_get_by_ckey(value, set->keynames[i]);
            if (v == PURC_VARIANT_INVALID) {
                v = purc_variant_make_undefined();
            }
            if (v == PURC_VARIANT_INVALID) {
                return -1;
            }
            kvs[i] = v;
        }
    } else {
        PC_ASSERT(set->nr_keynames==1);
        kvs[0] = value;
    }
    return 0;
}

static purc_variant_t _pcv_set_new(void)
{
    purc_variant_t set = pcvariant_get(PVT(_SET));
    if (!set) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    set->type          = PVT(_SET);
    set->flags         = PCVARIANT_FLAG_EXTRA_SIZE;

    variant_set_t ptr  = (variant_set_t)calloc(1, sizeof(*ptr));
    _pcv_set_set_data(set, ptr);

    if (!ptr) {
        pcvariant_put(set);
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    set->refc          = 1;

    // a valid empty set
    return set;
}

void pcvariant_set_release_obj(struct obj_node *obj)
{
    if (obj->obj) {
        purc_variant_unref(obj->obj);
        obj->obj = PURC_VARIANT_INVALID;
    }
    if (obj->kvs) {
        free(obj->kvs);
        obj->kvs = NULL;
    }
}

static inline void
_refresh_arr(struct pcutils_arrlist *arr, size_t idx)
{
    if (idx == (size_t)-1)
        return;

    size_t count = pcutils_arrlist_length(arr);
    for (; idx < count; ++idx) {
        struct obj_node *p;
        p = (struct obj_node*)pcutils_arrlist_get_idx(arr, idx);
        p->idx = idx;
    }
}

static void _variant_set_release_objs(variant_set_t set)
{
    struct rb_node *node, *next;
    for (node=pcutils_rbtree_first(&set->objs);
         ({next = node ? pcutils_rbtree_next(node) : NULL; node;});
         node = next)
    {
        struct obj_node *p;
        p = container_of(node, struct obj_node, node);
        pcutils_rbtree_erase(node, &set->objs);
        // NOTE: for the sake of performance
        // int r = pcutils_arrlist_del_idx(set->arr, p->idx, 1);
        // PC_ASSERT(r==0);
        // _refresh_arr(set->arr, p->idx);
        pcvariant_set_release_obj(p);
        free(p);
    }

    pcutils_arrlist_free(set->arr);
    set->arr = NULL;
}

static inline void _variant_set_release(variant_set_t set)
{
    _variant_set_release_objs(set);

    free(set->keynames);
    set->keynames = NULL;
    set->nr_keynames = 0;
    free(set->unique_key);
    set->unique_key = NULL;
}

static inline purc_variant_t*
_variant_set_create_empty_kvs (variant_set_t set)
{
    purc_variant_t *kvs;
    kvs = (purc_variant_t*)calloc(set->nr_keynames, sizeof(*kvs));
    if (!kvs) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    return kvs;
}

static inline purc_variant_t*
_variant_set_create_kvs (variant_set_t set, purc_variant_t val)
{
    purc_variant_t *kvs;
    kvs = _variant_set_create_empty_kvs(set);
    if (!kvs)
        return NULL;

    if (_variant_set_cache_obj_keyval(set, val, kvs)) {
        free(kvs);
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    return kvs;
}

static inline purc_variant_t*
_variant_set_create_kvs_n (variant_set_t set, purc_variant_t v1, va_list ap)
{
    PC_ASSERT(v1 != PURC_VARIANT_INVALID);

    purc_variant_t *kvs;
    kvs = _variant_set_create_empty_kvs(set);
    if (!kvs)
        return NULL;

    size_t i = 0;
    kvs[i] = v1;
    for (i=1; i<set->nr_keynames; ++i) {
        purc_variant_t v;
        v = va_arg(ap, purc_variant_t);
        if (!v) {
            free(kvs);
            pcinst_set_error(PURC_ERROR_INVALID_VALUE);
            return NULL;
        }
        kvs[i] = v;
    }

    return kvs;
}

static struct obj_node*
_variant_set_create_obj_node (variant_set_t set, purc_variant_t val)
{
    struct obj_node *_new = (struct obj_node*)calloc(1, sizeof(*_new));
    if (!_new) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    _new->kvs = _variant_set_create_kvs(set, val);
    if (!_new->kvs) {
        free(_new);
        return NULL;
    }

    _new->obj = val;
    purc_variant_ref(val);

    return _new;
}

static inline struct obj_node*
_find_element(variant_set_t set, void *key)
{
    struct rb_node **pnode = &set->objs.rb_node;
    struct rb_node *parent = NULL;
    struct rb_node *entry = NULL;
    while (*pnode) {
        struct obj_node *on;
        on = container_of(*pnode, struct obj_node, node);
        int ret = _variant_set_keyvals_cmp(key, on->kvs, set);

        parent = *pnode;

        if (ret < 0)
            pnode = &parent->rb_left;
        else if (ret > 0)
            pnode = &parent->rb_right;
        else{
            entry = *pnode;
            break;
        }
    }

    if (!entry)
        return NULL;

    return container_of(entry, struct obj_node, node);
}

static inline int
_insert_or_replace(variant_set_t set, struct obj_node *node, bool override)
{
    struct rb_node **pnode = &set->objs.rb_node;
    struct rb_node *parent = NULL;
    struct rb_node *entry = NULL;
    while (*pnode) {
        struct obj_node *on;
        on = container_of(*pnode, struct obj_node, node);
        int ret = _variant_set_keyvals_cmp(node->kvs, on->kvs, set);

        parent = *pnode;

        if (ret < 0)
            pnode = &parent->rb_left;
        else if (ret > 0)
            pnode = &parent->rb_right;
        else{
            entry = *pnode;
            break;
        }
    }

    if (!entry) {
        int r = pcutils_arrlist_add(set->arr, node);
        if (r)
            return -1;
        size_t count = pcutils_arrlist_length(set->arr);
        node->idx = count - 1;

        entry = &node->node;

        pcutils_rbtree_link_node(entry, parent, pnode);
        pcutils_rbtree_insert_color(entry, &set->objs);
        return 0;
    }

    if (!override) {
        return -1;
    }

    struct obj_node *curr;
    curr = container_of(entry, struct obj_node, node);
    PC_ASSERT(curr != node);
    PC_ASSERT(curr->kvs != node->kvs);
    PC_ASSERT(curr->obj != node->obj);
    curr->kvs = node->kvs;
    node->kvs = NULL;
    curr->obj = node->obj;
    purc_variant_ref(curr->obj);
    pcvariant_set_release_obj(node);
    free(node);

    return 0;
}

static inline int
_remove(variant_set_t set, void *key)
{
    struct obj_node *p;
    p = _find_element(set, key);
    if (!p)
        return -1;
    pcutils_rbtree_erase(&p->node, &set->objs);
    int r = pcutils_arrlist_del_idx(set->arr, p->idx, 1);
    PC_ASSERT(r==0);
    _refresh_arr(set->arr, p->idx);
    p->idx = -1;
    pcvariant_set_release_obj(p);
    free(p);
    return 0;
}

static int
_variant_set_add_val(variant_set_t set, purc_variant_t val, bool override)
{
    if (!val) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }
    struct obj_node *_new;
    _new = _variant_set_create_obj_node(set, val);

    if (!_new) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }

    if (_insert_or_replace(set, _new, override)) {
        pcvariant_set_release_obj(_new);
        free(_new);
        return -1;
    }

    return 0;
}

static int
_variant_set_add_valsn(variant_set_t set, bool override,
    size_t sz, va_list ap)
{
    size_t i = 0;
    while (i<sz) {
        purc_variant_t v = va_arg(ap, purc_variant_t);
        if (!v) {
            pcinst_set_error(PURC_ERROR_INVALID_VALUE);
            break;
        }

        if (_variant_set_add_val(set, v, override)) {
            break;
        }

        ++i;
    }
    return i<sz ? -1 : 0;
}

static inline purc_variant_t
_make_set_c(size_t sz, const char *unique_key,
    purc_variant_t value0, va_list ap)
{
    purc_variant_t set = _pcv_set_new();
    if (set==PURC_VARIANT_INVALID) {
        return PURC_VARIANT_INVALID;
    }

    do {
        variant_set_t data = _pcv_set_get_data(set);
        if (_variant_set_init(data, unique_key))
            break;

        if (sz>0) {
            purc_variant_t  v = value0;
            if (_variant_set_add_val(data, v, true))
                break;

            int r = _variant_set_add_valsn(data, true, sz-1, ap);
            if (r)
                break;
        }

        size_t extra = _variant_set_get_extra_size(data);
        pcvariant_stat_set_extra_size(set, extra);
        return set;
    } while (0);

    // cleanup
    purc_variant_unref(set);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
pv_make_set_by_ckey_n (size_t sz, const char* unique_key,
    purc_variant_t value0, va_list ap)
{
    PCVARIANT_CHECK_FAIL_RET((sz==0 && value0==NULL) || (sz>0 && value0),
        PURC_VARIANT_INVALID);

    if (!unique_key)
        unique_key = "";

    purc_variant_t v = _make_set_c(sz, unique_key, value0, ap);

    return v;
}

purc_variant_t
purc_variant_make_set_by_ckey (size_t sz, const char* unique_key,
    purc_variant_t value0, ...)
{
    purc_variant_t v;
    va_list ap;
    va_start(ap, value0);
    v = pv_make_set_by_ckey_n(sz, unique_key, value0, ap);
    va_end(ap);

    return v;
}

static purc_variant_t
pv_make_set_n (size_t sz, purc_variant_t unique_key,
    purc_variant_t value0, va_list ap)
{
    PCVARIANT_CHECK_FAIL_RET((sz==0 && value0==NULL) ||
        (sz>0 && value0),
        PURC_VARIANT_INVALID);

    PCVARIANT_CHECK_FAIL_RET(!unique_key || unique_key->type==PVT(_STRING),
        PURC_VARIANT_INVALID);

    if (!unique_key) {
        unique_key = purc_variant_make_string("", false);
        if (!unique_key) {
            pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
            return PURC_VARIANT_INVALID;
        }
    } else {
        purc_variant_ref(unique_key);
    }

    const char *uk = purc_variant_get_string_const(unique_key);
    PC_ASSERT(uk);

    purc_variant_t v = _make_set_c(sz, uk, value0, ap);

    purc_variant_unref(unique_key);

    return v;
}

purc_variant_t
purc_variant_make_set (size_t sz, purc_variant_t unique_key,
    purc_variant_t value0, ...)
{
    purc_variant_t v;
    va_list ap;
    va_start(ap, value0);
    v = pv_make_set_n(sz, unique_key, value0, ap);
    va_end(ap);

    return v;
}

bool
purc_variant_set_add (purc_variant_t set, purc_variant_t value, bool override)
{
    PCVARIANT_CHECK_FAIL_RET(set && set->type==PVT(_SET) && value,
        PURC_VARIANT_INVALID);

    variant_set_t data = _pcv_set_get_data(set);
    PC_ASSERT(data);

    if (_variant_set_add_val(data, value, override))
        return false;

    size_t extra = _variant_set_get_extra_size(data);
    pcvariant_stat_set_extra_size(set, extra);
    return true;
}

bool purc_variant_set_remove (purc_variant_t set, purc_variant_t value)
{
    PCVARIANT_CHECK_FAIL_RET(set && set->type==PVT(_SET) && value,
        PURC_VARIANT_INVALID);

    variant_set_t data = _pcv_set_get_data(set);
    PC_ASSERT(data);
    PC_ASSERT(data->nr_keynames);

    purc_variant_t *kvs = _variant_set_create_kvs(data, value);
    if (!kvs)
        return false;

    int r = _remove(data, kvs);
    free(kvs);
    return r ? false : true;
}

purc_variant_t
purc_variant_set_get_member_by_key_values(purc_variant_t set,
        purc_variant_t v1, ...)
{
    PCVARIANT_CHECK_FAIL_RET(set && set->type==PVT(_SET) && v1,
        PURC_VARIANT_INVALID);

    variant_set_t data = _pcv_set_get_data(set);
    if (!data || !data->unique_key || data->nr_keynames==0) {
        pcinst_set_error(PURC_ERROR_NOT_SUPPORTED);
        return PURC_VARIANT_INVALID;
    }

    va_list ap;
    va_start(ap, v1);
    purc_variant_t *kvs = _variant_set_create_kvs_n(data, v1, ap);
    va_end(ap);
    if (!kvs)
        return false;

    struct obj_node *p;
    p = _find_element(data, kvs);
    free(kvs);
    return p ? p->obj : PURC_VARIANT_INVALID;
}

purc_variant_t
purc_variant_set_remove_member_by_key_values(purc_variant_t set,
        purc_variant_t v1, ...)
{
    PCVARIANT_CHECK_FAIL_RET(set && set->type==PVT(_SET) && v1,
        PURC_VARIANT_INVALID);

    variant_set_t data = _pcv_set_get_data(set);
    if (!data || !data->unique_key || data->nr_keynames==0) {
        pcinst_set_error(PURC_ERROR_NOT_SUPPORTED);
        return PURC_VARIANT_INVALID;
    }

    va_list ap;
    va_start(ap, v1);
    purc_variant_t *kvs = _variant_set_create_kvs_n(data, v1, ap);
    va_end(ap);
    if (!kvs)
        return false;

    struct obj_node *p;
    p = _find_element(data, kvs);
    free(kvs);

    if (!p) {
        pcinst_set_error(PCVARIANT_ERROR_NOT_FOUND);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t v = p->obj;
    purc_variant_ref(v);

    pcutils_rbtree_erase(&p->node, &data->objs);
    int r = pcutils_arrlist_del_idx(data->arr, p->idx, 1);
    PC_ASSERT(r==0);
    _refresh_arr(data->arr, p->idx);
    p->idx = -1;
    pcvariant_set_release_obj(p);
    free(p);

    size_t extra = _variant_set_get_extra_size(data);
    pcvariant_stat_set_extra_size(set, extra);

    return v;
}

bool purc_variant_set_size(purc_variant_t set, size_t *sz)
{
    PC_ASSERT(set && sz);

    PCVARIANT_CHECK_FAIL_RET(set->type == PVT(_SET), false);

    variant_set_t data = _pcv_set_get_data(set);

    PC_ASSERT(data);
    size_t count = pcutils_arrlist_length(data->arr);
    *sz = count;

    return true;
}

purc_variant_t
purc_variant_set_get(purc_variant_t set, int idx)
{
    PC_ASSERT(set);

    variant_set_t data = _pcv_set_get_data(set);
    size_t count = pcutils_arrlist_length(data->arr);

    if (idx < 0 || (size_t)idx >= count)
        return PURC_VARIANT_INVALID;

    struct obj_node *node;
    node = (struct obj_node*)pcutils_arrlist_get_idx(data->arr, idx);
    PC_ASSERT(node);
    PC_ASSERT(node->idx == (size_t)idx);
    PC_ASSERT(node->obj != PURC_VARIANT_INVALID);

    return node->obj;
}

struct purc_variant_set_iterator {
    purc_variant_t      set;
    struct rb_node     *curr;
    struct rb_node     *prev, *next;
};

static inline void
_iterator_refresh(struct purc_variant_set_iterator *it)
{
    if (it->curr == NULL) {
        it->next = NULL;
        it->prev = NULL;
        return;
    }
    variant_set_t data = _pcv_set_get_data(it->set);
    size_t count = pcutils_arrlist_length(data->arr);
    if (count==0) {
        it->next = NULL;
        it->prev = NULL;
        return;
    }
    struct rb_node *first, *last;
    first = pcutils_rbtree_first(&data->objs);
    last  = pcutils_rbtree_last(&data->objs);
    if (it->curr == first) {
        it->prev = NULL;
    } else {
        it->prev = pcutils_rbtree_prev(it->curr);
    }
    if (it->curr == last) {
        it->next = NULL;
    } else {
        it->next = pcutils_rbtree_next(it->curr);
    }
}

struct purc_variant_set_iterator*
purc_variant_set_make_iterator_begin (purc_variant_t set)
{
    PCVARIANT_CHECK_FAIL_RET(set && set->type==PVT(_SET),
        NULL);

    variant_set_t data = _pcv_set_get_data(set);
    PC_ASSERT(data);

    size_t count = pcutils_arrlist_length(data->arr);
    if (count == 0) {
        pcinst_set_error(PCVARIANT_ERROR_NOT_FOUND);
        return NULL;
    }

    struct purc_variant_set_iterator *it;
    it = (struct purc_variant_set_iterator*)calloc(1, sizeof(*it));
    if (!it) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    it->set = set;

    struct rb_node *p;
    p = pcutils_rbtree_first(&data->objs);
    PC_ASSERT(p);

    it->curr = p;
    _iterator_refresh(it);

    return it;
}

struct purc_variant_set_iterator*
purc_variant_set_make_iterator_end (purc_variant_t set)
{
    PCVARIANT_CHECK_FAIL_RET(set && set->type==PVT(_SET),
        NULL);

    variant_set_t data = _pcv_set_get_data(set);
    PC_ASSERT(data);

    size_t count = pcutils_arrlist_length(data->arr);
    if (count == 0) {
        pcinst_set_error(PCVARIANT_ERROR_NOT_FOUND);
        return NULL;
    }

    struct purc_variant_set_iterator *it;
    it = (struct purc_variant_set_iterator*)calloc(1, sizeof(*it));
    if (!it) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    it->set = set;

    struct rb_node *p;
    p = pcutils_rbtree_last(&data->objs);
    PC_ASSERT(p);

    it->curr = p;
    _iterator_refresh(it);

    return it;
}

void purc_variant_set_release_iterator (struct purc_variant_set_iterator* it)
{
    if (!it)
        return;
    free(it);
}

bool purc_variant_set_iterator_next (struct purc_variant_set_iterator* it)
{
    PCVARIANT_CHECK_FAIL_RET(it && it->set &&
        it->set->type==PVT(_SET) && it->curr,
        false);

    variant_set_t data = _pcv_set_get_data(it->set);
    PC_ASSERT(data);

    it->curr = it->next;
    _iterator_refresh(it);

    return it->curr ? true : false;
}

bool purc_variant_set_iterator_prev (struct purc_variant_set_iterator* it)
{
    PCVARIANT_CHECK_FAIL_RET(it && it->set &&
        it->set->type==PVT(_SET) && it->curr,
        false);

    variant_set_t data = _pcv_set_get_data(it->set);
    PC_ASSERT(data);

    it->curr = it->prev;
    _iterator_refresh(it);

    return it->curr ? true : false;
}

purc_variant_t
purc_variant_set_iterator_get_value (struct purc_variant_set_iterator* it)
{
    PCVARIANT_CHECK_FAIL_RET(it && it->set &&
        it->set->type==PVT(_SET) && it->curr,
        PURC_VARIANT_INVALID);

    struct obj_node *p;
    p = container_of(it->curr, struct obj_node, node);
    return p->obj;
}

void pcvariant_set_release (purc_variant_t value)
{
    variant_set_t data = _pcv_set_get_data(value);
    PC_ASSERT(data);

    _variant_set_release(data);
    free(data);
    _pcv_set_set_data(value, NULL);
    pcvariant_stat_set_extra_size(value, 0);
}

/* VWNOTE: unnecessary
int pcvariant_set_compare (purc_variant_t lv, purc_variant_t rv)
{
    variant_set_t ldata = _pcv_set_get_data(lv);
    variant_set_t rdata = _pcv_set_get_data(rv);
    PC_ASSERT(ldata && rdata);

    struct obj_node *ln, *rn;
    ln = avl_first_element(&ldata->objs, ln, avl);
    rn = avl_first_element(&rdata->objs, rn, avl);
    for (; ln && rn;
        ln = avl_next_element(ln, avl),
        rn = avl_next_element(rn, avl))
    {
        int t = purc_variant_compare(ln->obj, rn->obj);
        if (t)
            return t;
    }

    return ln ? 1 : -1;
}
*/

