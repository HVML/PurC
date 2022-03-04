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

#define _GNU_SOURCE       // qsort_r

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

#define TO_DEBUG 1

static bool
grow(purc_variant_t set, purc_variant_t value)
{
    purc_variant_t vals[] = { value };

    return pcvariant_on_pre_fired(set, pcvariant_atom_grow,
            PCA_TABLESIZE(vals), vals);
}

static bool
shrink(purc_variant_t set, purc_variant_t value)
{
    purc_variant_t vals[] = { value };

    return pcvariant_on_pre_fired(set, pcvariant_atom_shrink,
            PCA_TABLESIZE(vals), vals);
}

static bool
change(purc_variant_t set,
        purc_variant_t o, purc_variant_t n)
{
    purc_variant_t vals[] = { o, n };

    return pcvariant_on_pre_fired(set, pcvariant_atom_change,
            PCA_TABLESIZE(vals), vals);
}

static void
grown(purc_variant_t set, purc_variant_t value)
{
    purc_variant_t vals[] = { value };

    pcvariant_on_post_fired(set, pcvariant_atom_grow,
            PCA_TABLESIZE(vals), vals);
}

static void
shrunk(purc_variant_t set, purc_variant_t value)
{
    purc_variant_t vals[] = { value };

    pcvariant_on_post_fired(set, pcvariant_atom_shrink,
            PCA_TABLESIZE(vals), vals);
}

static void
changed(purc_variant_t set,
        purc_variant_t o, purc_variant_t n)
{
    purc_variant_t vals[] = { o, n };

    pcvariant_on_post_fired(set, pcvariant_atom_change,
            PCA_TABLESIZE(vals), vals);
}

static variant_set_t
pcv_set_get_data(purc_variant_t set)
{
    return (variant_set_t)set->sz_ptr[1];
}

static size_t
variant_set_get_extra_size(variant_set_t set)
{
    size_t extra = 0;
    if (set->unique_key) {
        extra += strlen(set->unique_key) + 1;
        extra += sizeof(*set->keynames) * set->nr_keynames;
    }
    size_t sz_record = sizeof(struct elem_node) +
        sizeof(purc_variant_t) * set->nr_keynames;
    size_t count = pcutils_arrlist_length(set->arr);
    extra += sz_record * count;
    extra += sizeof(*set->arr);
    extra += sizeof(struct elem_node*)*(set->arr->size);

    return extra;
}

static void
pcv_set_set_data(purc_variant_t set, variant_set_t data)
{
    set->sz_ptr[1]     = (uintptr_t)data;
}

static int
variant_cmp(purc_variant_t v1, purc_variant_t v2)
{
    if (v1 == v2)
        return 0;
    if (v1 == PURC_VARIANT_INVALID)
        return -1;
    if (v2 == PURC_VARIANT_INVALID)
        return 1;

    return purc_variant_compare_ex(v1, v2, PCVARIANT_COMPARE_OPT_AUTO);
}

static int
variant_set_keyvals_cmp (const void *k1, const void *k2, void *ptr)
{
    purc_variant_t *kvs1 = (purc_variant_t*)k1;
    purc_variant_t *kvs2 = (purc_variant_t*)k2;
    variant_set_t   set  = (variant_set_t)ptr;

    int diff = 0;
    for (size_t i=0; i<set->nr_keynames; ++i) {
        purc_variant_t kv1 = kvs1[i];
        purc_variant_t kv2 = kvs2[i];
        diff = variant_cmp(kv1, kv2);
        if (diff)
            break;
    }

    return diff;
}

static int
variant_set_init(variant_set_t set, const char *unique_key)
{
    set->elems = RB_ROOT;

    size_t initial_size = ARRAY_LIST_DEFAULT_SIZE;
    set->arr = pcutils_arrlist_new_ex(NULL, initial_size);
    if (!set->arr) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }

    if (!unique_key || !*unique_key) {
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
    set->keynames = (const char**)calloc(n, sizeof(*set->keynames));
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
variant_set_cache_obj_keyval(variant_set_t set,
    purc_variant_t value, purc_variant_t *kvs)
{
    PC_ASSERT(value != PURC_VARIANT_INVALID);
    PC_ASSERT(set->nr_keynames);

    if (set->unique_key) {
        for (size_t i=0; i<set->nr_keynames; ++i) {
            purc_variant_t v;
            v = purc_variant_object_get_by_ckey(value, set->keynames[i], false);
            kvs[i] = v;
        }
    } else {
        PC_ASSERT(set->nr_keynames==1);
        kvs[0] = value;
    }
    return 0;
}

static purc_variant_t
pcv_set_new(void)
{
    purc_variant_t set = pcvariant_get(PVT(_SET));
    if (!set) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    set->type          = PVT(_SET);
    set->flags         = PCVARIANT_FLAG_EXTRA_SIZE;

    variant_set_t ptr  = (variant_set_t)calloc(1, sizeof(*ptr));
    pcv_set_set_data(set, ptr);

    if (!ptr) {
        pcvariant_put(set);
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    set->refc          = 1;

    // a valid empty set
    return set;
}

static void
elem_node_revoke_constraints(struct elem_node *elem)
{
    elem->set = PURC_VARIANT_INVALID;
}

static bool
elem_node_setup_constraints(purc_variant_t set, struct elem_node *elem)
{
    PC_ASSERT(elem->set == PURC_VARIANT_INVALID);
    elem->set = set;
    purc_variant_t child = elem->elem;
    PC_ASSERT(child != PURC_VARIANT_INVALID);
    PC_ASSERT(purc_variant_is_object(child));

    variant_set_t data = pcv_set_get_data(set);

    for (size_t i=0; data->keynames && i<data->nr_keynames; ++i) {
        const char *sk = data->keynames[i];
        purc_variant_t v;
        const bool silently = true;
        v = purc_variant_object_get_by_ckey(child, sk, silently);
        if (v == PURC_VARIANT_INVALID)
            continue;
    }

    return true;
}

static void
elem_node_release(struct elem_node *elem)
{
    if (elem->elem != PURC_VARIANT_INVALID) {
        elem_node_revoke_constraints(elem);
        purc_variant_unref(elem->elem);
        elem->elem = PURC_VARIANT_INVALID;
    }
    if (elem->kvs) {
        free(elem->kvs);
        elem->kvs = NULL;
    }
    elem->set = PURC_VARIANT_INVALID;
}

static void
refresh_arr(struct pcutils_arrlist *arr, size_t idx)
{
    if (idx == (size_t)-1)
        return;

    size_t count = pcutils_arrlist_length(arr);
    for (; idx < count; ++idx) {
        struct elem_node *p;
        p = (struct elem_node*)pcutils_arrlist_get_idx(arr, idx);
        p->idx = idx;
    }
}

static void
variant_set_release_elems(variant_set_t set)
{
    struct rb_node *node, *next;
    for (node=pcutils_rbtree_first(&set->elems);
         ({next = node ? pcutils_rbtree_next(node) : NULL; node;});
         node = next)
    {
        struct elem_node *p;
        p = container_of(node, struct elem_node, node);
        pcutils_rbtree_erase(node, &set->elems);
        // NOTE: for the sake of performance
        // int r = pcutils_arrlist_del_idx(set->arr, p->idx, 1);
        // PC_ASSERT(r==0);
        // refresh_arr(set->arr, p->idx);
        elem_node_release(p);
        free(p);
    }

    pcutils_arrlist_free(set->arr);
    set->arr = NULL;
}

static void
variant_set_release(variant_set_t data)
{
    variant_set_release_elems(data);

    free(data->keynames);
    data->keynames = NULL;
    data->nr_keynames = 0;
    free(data->unique_key);
    data->unique_key = NULL;
}

static purc_variant_t*
variant_set_create_empty_kvs (variant_set_t set)
{
    purc_variant_t *kvs;
    kvs = (purc_variant_t*)calloc(set->nr_keynames, sizeof(*kvs));
    if (!kvs) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    return kvs;
}

static purc_variant_t*
variant_set_create_kvs (variant_set_t set, purc_variant_t val)
{
    purc_variant_t *kvs;
    kvs = variant_set_create_empty_kvs(set);
    if (!kvs)
        return NULL;

    if (variant_set_cache_obj_keyval(set, val, kvs)) {
        free(kvs);
        return NULL;
    }

    return kvs;
}

static purc_variant_t*
variant_set_create_kvs_n (variant_set_t set, purc_variant_t v1, va_list ap)
{
    PC_ASSERT(v1 != PURC_VARIANT_INVALID);

    purc_variant_t *kvs;
    kvs = variant_set_create_empty_kvs(set);
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

static struct elem_node*
variant_set_create_elem_node (variant_set_t set, purc_variant_t val)
{
    struct elem_node *_new = (struct elem_node*)calloc(1, sizeof(*_new));
    if (!_new) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    _new->kvs = variant_set_create_kvs(set, val);
    if (!_new->kvs) {
        free(_new);
        return NULL;
    }

    _new->elem = val;
    purc_variant_ref(val);

    return _new;
}

static struct elem_node*
find_element(variant_set_t set, void *key)
{
    struct rb_node **pnode = &set->elems.rb_node;
    struct rb_node *parent = NULL;
    struct rb_node *entry = NULL;
    while (*pnode) {
        struct elem_node *on;
        on = container_of(*pnode, struct elem_node, node);
        int ret = variant_set_keyvals_cmp(key, on->kvs, set);

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

    return container_of(entry, struct elem_node, node);
}

static int
insert_or_replace(purc_variant_t set,
        variant_set_t data, struct elem_node *node, bool override)
{
    struct rb_node **pnode = &data->elems.rb_node;
    struct rb_node *parent = NULL;
    struct rb_node *entry = NULL;
    while (*pnode) {
        struct elem_node *on;
        on = container_of(*pnode, struct elem_node, node);
        int ret = variant_set_keyvals_cmp(node->kvs, on->kvs, data);

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
        int r = pcutils_arrlist_add(data->arr, node);
        if (r)
            return -1;
        size_t count = pcutils_arrlist_length(data->arr);

        if (!grow(set, node->elem)) {
            bool ok = pcutils_arrlist_del_idx(data->arr, count-1, 1);
            PC_ASSERT(ok);
            return -1;
        }

        if (!elem_node_setup_constraints(set, node)) {
            bool ok = pcutils_arrlist_del_idx(data->arr, count-1, 1);
            PC_ASSERT(ok);
            return -1;
        }

        node->idx = count - 1;

        entry = &node->node;

        pcutils_rbtree_link_node(entry, parent, pnode);
        pcutils_rbtree_insert_color(entry, &data->elems);

        grown(set, node->elem);

        return 0;
    }

    if (!override) {
        return -1;
    }

    struct elem_node *curr;
    curr = container_of(entry, struct elem_node, node);
    PC_ASSERT(curr != node);
    PC_ASSERT(curr->kvs != node->kvs);

    if (curr->elem == node->elem) {
        elem_node_release(node);
        free(node);
        return 0;
    }

    PC_ASSERT(curr->elem != node->elem);
    if (data->keynames == NULL) {
        // totally equal, nothing changed
        elem_node_release(node);
        free(node);
        return 0;
    }

    purc_variant_t tmp = purc_variant_make_object(0,
        PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    if (tmp == PURC_VARIANT_INVALID) {
        return -1;
    }

    // TODO: performance, performance, performance!!!
    bool ok = true;
    // step1: copy current into tmp
    purc_variant_t k, v;
    foreach_key_value_in_variant_object(curr->elem, k, v)
        ok = purc_variant_object_set(tmp, k, v);
        if (!ok)
            break;
    end_foreach;
    if (!ok) {
        purc_variant_unref(tmp);
        return -1;
    }
    // step2: update tmp with src
    foreach_key_value_in_variant_object(node->elem, k, v)
        const char *sk = purc_variant_get_string_const(k);
        // bypass key-fields
        bool is_key = false;
        for (size_t i=0; i<data->nr_keynames; ++i) {
            if (strcmp(data->keynames[i], sk)==0) {
                is_key = true;
                break;
            }
        }
        if (is_key)
            continue;
        if (purc_variant_is_type(v, PURC_VARIANT_TYPE_UNDEFINED)) {
            // remove the specified key-field
            purc_variant_object_remove(tmp, k, true);
        }
        else {
            // add kv pair
            ok = purc_variant_object_set(tmp, k, v);
            if (!ok)
                break;
        }
    end_foreach;
    if (!ok) {
        purc_variant_unref(tmp);
        return -1;
    }

    if (!change(set, curr->elem, tmp)) {
        purc_variant_unref(tmp);
        return -1;
    }

    changed(set, curr->elem, tmp);

    // replace with tmp, performance, performance, performance!!!
    foreach_key_value_in_variant_object(tmp, k, v)
        const char *sk = purc_variant_get_string_const(k);
        // bypass key-fields
        bool is_key = false;
        for (size_t i=0; i<data->nr_keynames; ++i) {
            if (strcmp(data->keynames[i], sk)==0) {
                is_key = true;
                break;
            }
        }
        if (is_key)
            continue;
        if (purc_variant_is_type(v, PURC_VARIANT_TYPE_UNDEFINED)) {
            // remove the specified key-field
            purc_variant_object_remove(curr->elem, k, true);
        }
        else {
            // add kv pair
            ok = purc_variant_object_set(curr->elem, k, v);
            PC_ASSERT(ok); // TODO: rollback???
            if (!ok)
                break;
        }
    end_foreach;
    PURC_VARIANT_SAFE_CLEAR(tmp);

    elem_node_release(node);
    free(node);

    return 0;
}

static int
set_remove(purc_variant_t set, variant_set_t data, struct elem_node *node)
{
    if (!shrink(set, node->elem)) {
        return -1;
    }

    pcutils_rbtree_erase(&node->node, &data->elems);
    int r = pcutils_arrlist_del_idx(data->arr, node->idx, 1);
    PC_ASSERT(r==0);

    shrunk(set, node->elem);

    refresh_arr(data->arr, node->idx);
    node->idx = -1;
    elem_node_release(node);
    free(node);

    return 0;
}

static int
variant_set_add_val(purc_variant_t set,
        variant_set_t data, purc_variant_t val, bool override)
{
    if (!val) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }
    if (purc_variant_is_object(val) == false) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    struct elem_node *_new;
    _new = variant_set_create_elem_node(data, val);

    if (!_new)
        return -1;

    if (insert_or_replace(set, data, _new, override)) {
        elem_node_release(_new);
        free(_new);
        return -1;
    }

    return 0;
}

static int
variant_set_add_valsn(purc_variant_t set, variant_set_t data, bool override,
    size_t sz, va_list ap)
{
    size_t i = 0;
    while (i<sz) {
        purc_variant_t v = va_arg(ap, purc_variant_t);
        if (!v) {
            pcinst_set_error(PURC_ERROR_INVALID_VALUE);
            break;
        }

        if (variant_set_add_val(set, data, v, override)) {
            break;
        }

        ++i;
    }
    return i<sz ? -1 : 0;
}

static purc_variant_t
make_set_c(size_t sz, const char *unique_key,
    purc_variant_t value0, va_list ap)
{
    purc_variant_t set = pcv_set_new();
    if (set==PURC_VARIANT_INVALID) {
        return PURC_VARIANT_INVALID;
    }

    do {
        variant_set_t data = pcv_set_get_data(set);
        if (variant_set_init(data, unique_key))
            break;

        if (sz>0) {
            purc_variant_t  v = value0;
            if (variant_set_add_val(set, data, v, true))
                break;

            int r = variant_set_add_valsn(set, data, true, sz-1, ap);
            if (r)
                break;
        }

        size_t extra = variant_set_get_extra_size(data);
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

    purc_variant_t v = make_set_c(sz, unique_key, value0, ap);

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

    const char *uk = NULL;
    if (unique_key) {
        uk = purc_variant_get_string_const(unique_key);
        PC_ASSERT(uk);
    }

    purc_variant_t v = make_set_c(sz, uk, value0, ap);

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
    PCVARIANT_CHECK_FAIL_RET(set && set->type==PVT(_SET) &&
        value && value->type==PVT(_OBJECT),
        PURC_VARIANT_INVALID);

    variant_set_t data = pcv_set_get_data(set);
    PC_ASSERT(data);

    if (variant_set_add_val(set, data, value, override))
        return false;

    size_t extra = variant_set_get_extra_size(data);
    pcvariant_stat_set_extra_size(set, extra);
    return true;
}

bool
purc_variant_set_remove (purc_variant_t set, purc_variant_t value,
        bool silently)
{
    PCVARIANT_CHECK_FAIL_RET(set && set->type==PVT(_SET) && value,
        PURC_VARIANT_INVALID);

    variant_set_t data = pcv_set_get_data(set);
    PC_ASSERT(data);
    PC_ASSERT(data->nr_keynames);

    purc_variant_t *kvs = variant_set_create_kvs(data, value);
    if (!kvs)
        return false;

    int r = 0;
    struct elem_node *p;
    p = find_element(data, kvs);
    if (p) {
        r = set_remove(set, data, p);
    }
    free(kvs);

    if (r)
        return false;

    return p ? true : (silently ? true : false);
}

purc_variant_t
purc_variant_set_get_member_by_key_values(purc_variant_t set,
        purc_variant_t v1, ...)
{
    PCVARIANT_CHECK_FAIL_RET(set && set->type==PVT(_SET) && v1,
        PURC_VARIANT_INVALID);

    variant_set_t data = pcv_set_get_data(set);
    if (!data || !data->unique_key || data->nr_keynames==0) {
        pcinst_set_error(PURC_ERROR_NOT_SUPPORTED);
        return PURC_VARIANT_INVALID;
    }

    va_list ap;
    va_start(ap, v1);
    purc_variant_t *kvs = variant_set_create_kvs_n(data, v1, ap);
    va_end(ap);
    if (!kvs)
        return false;

    struct elem_node *p;
    p = find_element(data, kvs);
    free(kvs);
    return p ? p->elem : PURC_VARIANT_INVALID;
}

purc_variant_t
purc_variant_set_remove_member_by_key_values(purc_variant_t set,
        purc_variant_t v1, ...)
{
    PCVARIANT_CHECK_FAIL_RET(set && set->type==PVT(_SET) && v1,
        PURC_VARIANT_INVALID);

    variant_set_t data = pcv_set_get_data(set);
    if (!data || !data->unique_key || data->nr_keynames==0) {
        pcinst_set_error(PURC_ERROR_NOT_SUPPORTED);
        return PURC_VARIANT_INVALID;
    }

    va_list ap;
    va_start(ap, v1);
    purc_variant_t *kvs = variant_set_create_kvs_n(data, v1, ap);
    va_end(ap);
    if (!kvs)
        return PURC_VARIANT_INVALID;

    struct elem_node *p;
    p = find_element(data, kvs);
    free(kvs);

    if (!p) {
        pcinst_set_error(PCVARIANT_ERROR_NOT_FOUND);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t v = p->elem;
    purc_variant_ref(v);

    int r = set_remove(set, data, p);
    if (r) {
        purc_variant_unref(v);
        return PURC_VARIANT_INVALID;
    }

    size_t extra = variant_set_get_extra_size(data);
    pcvariant_stat_set_extra_size(set, extra);

    return v;
}

bool
purc_variant_set_size(purc_variant_t set, size_t *sz)
{
    PC_ASSERT(set && sz);

    PCVARIANT_CHECK_FAIL_RET(set->type == PVT(_SET), false);

    variant_set_t data = pcv_set_get_data(set);

    PC_ASSERT(data);
    size_t count = pcutils_arrlist_length(data->arr);
    *sz = count;

    return true;
}

purc_variant_t
purc_variant_set_get_by_index(purc_variant_t set, int idx)
{
    PC_ASSERT(set);

    variant_set_t data = pcv_set_get_data(set);
    size_t count = pcutils_arrlist_length(data->arr);

    if (idx < 0 || (size_t)idx >= count)
        return PURC_VARIANT_INVALID;

    struct elem_node *node;
    node = (struct elem_node*)pcutils_arrlist_get_idx(data->arr, idx);
    PC_ASSERT(node);
    PC_ASSERT(node->idx == (size_t)idx);
    PC_ASSERT(node->elem != PURC_VARIANT_INVALID);

    return node->elem;
}

PCA_EXPORT purc_variant_t
purc_variant_set_remove_by_index(purc_variant_t set, int idx)
{
    PC_ASSERT(set);

    variant_set_t data = pcv_set_get_data(set);
    size_t count = pcutils_arrlist_length(data->arr);

    if (idx < 0 || (size_t)idx >= count) {
        pcinst_set_error(PCVARIANT_ERROR_OUT_OF_BOUNDS);
        return PURC_VARIANT_INVALID;
    }

    struct elem_node *elem;
    elem = (struct elem_node*)pcutils_arrlist_get_idx(data->arr, idx);
    PC_ASSERT(elem);
    PC_ASSERT(elem->idx == (size_t)idx);

    purc_variant_t v = elem->elem;
    purc_variant_ref(v);

    int r = set_remove(set, data, elem);
    if (r) {
        purc_variant_unref(v);
        return PURC_VARIANT_INVALID;
    }

    size_t extra = variant_set_get_extra_size(data);
    pcvariant_stat_set_extra_size(set, extra);

    return v;
}

PCA_EXPORT bool
purc_variant_set_set_by_index(purc_variant_t set, int idx, purc_variant_t val)
{
    PC_ASSERT(set);

    variant_set_t data = pcv_set_get_data(set);
    size_t count = pcutils_arrlist_length(data->arr);

    if (idx < 0 || (size_t)idx >= count) {
        pcinst_set_error(PCVARIANT_ERROR_OUT_OF_BOUNDS);
        return false;
    }

    struct elem_node *elem;
    elem = (struct elem_node*)pcutils_arrlist_get_idx(data->arr, idx);
    if (elem->elem == val)
        return true;

    purc_variant_t v = purc_variant_set_remove_by_index(set, idx);
    PC_ASSERT(v != PURC_VARIANT_INVALID);
    bool ok = purc_variant_set_add(set, val, true);
    PC_ASSERT(ok);
    purc_variant_unref(v);
    return ok;
}

struct purc_variant_set_iterator {
    purc_variant_t      set;
    struct rb_node     *curr;
    struct rb_node     *prev, *next;
};

static void
iterator_refresh(struct purc_variant_set_iterator *it)
{
    if (it->curr == NULL) {
        it->next = NULL;
        it->prev = NULL;
        return;
    }
    variant_set_t data = pcv_set_get_data(it->set);
    size_t count = pcutils_arrlist_length(data->arr);
    if (count==0) {
        it->next = NULL;
        it->prev = NULL;
        return;
    }
    struct rb_node *first, *last;
    first = pcutils_rbtree_first(&data->elems);
    last  = pcutils_rbtree_last(&data->elems);
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

    variant_set_t data = pcv_set_get_data(set);
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
    p = pcutils_rbtree_first(&data->elems);
    PC_ASSERT(p);

    it->curr = p;
    iterator_refresh(it);

    return it;
}

struct purc_variant_set_iterator*
purc_variant_set_make_iterator_end (purc_variant_t set)
{
    PCVARIANT_CHECK_FAIL_RET(set && set->type==PVT(_SET),
        NULL);

    variant_set_t data = pcv_set_get_data(set);
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
    p = pcutils_rbtree_last(&data->elems);
    PC_ASSERT(p);

    it->curr = p;
    iterator_refresh(it);

    return it;
}

void
purc_variant_set_release_iterator (struct purc_variant_set_iterator* it)
{
    if (!it)
        return;
    free(it);
}

bool
purc_variant_set_iterator_next (struct purc_variant_set_iterator* it)
{
    PCVARIANT_CHECK_FAIL_RET(it && it->set &&
        it->set->type==PVT(_SET) && it->curr,
        false);

    variant_set_t data = pcv_set_get_data(it->set);
    PC_ASSERT(data);

    it->curr = it->next;
    iterator_refresh(it);

    return it->curr ? true : false;
}

bool
purc_variant_set_iterator_prev (struct purc_variant_set_iterator* it)
{
    PCVARIANT_CHECK_FAIL_RET(it && it->set &&
        it->set->type==PVT(_SET) && it->curr,
        false);

    variant_set_t data = pcv_set_get_data(it->set);
    PC_ASSERT(data);

    it->curr = it->prev;
    iterator_refresh(it);

    return it->curr ? true : false;
}

purc_variant_t
purc_variant_set_iterator_get_value (struct purc_variant_set_iterator* it)
{
    PCVARIANT_CHECK_FAIL_RET(it && it->set &&
        it->set->type==PVT(_SET) && it->curr,
        PURC_VARIANT_INVALID);

    struct elem_node *p;
    p = container_of(it->curr, struct elem_node, node);
    return p->elem;
}

void
pcvariant_set_release (purc_variant_t value)
{
    variant_set_t data = pcv_set_get_data(value);
    PC_ASSERT(data);

    variant_set_release(data);
    free(data);
    pcv_set_set_data(value, NULL);
    pcvariant_stat_set_extra_size(value, 0);
}

/* VWNOTE: unnecessary
int pcvariant_set_compare (purc_variant_t lv, purc_variant_t rv)
{
    variant_set_t ldata = _pcv_set_get_data(lv);
    variant_set_t rdata = _pcv_set_get_data(rv);
    PC_ASSERT(ldata && rdata);

    struct elem_node *ln, *rn;
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

struct set_user_data {
    int (*cmp)(size_t nr_keynames,
            purc_variant_t l[], purc_variant_t r[], void *ud);
    void *ud;
    size_t               nr_keynames;
};

#if OS(HURD) || OS(LINUX)
static int cmp_variant(const void *l, const void *r, void *ud)
{
    struct elem_node *nl = *(struct elem_node**)l;
    struct elem_node *nr = *(struct elem_node**)r;
    purc_variant_t *vl = nl->kvs;
    purc_variant_t *vr = nr->kvs;
    struct set_user_data *d = (struct set_user_data*)ud;
    return d->cmp(d->nr_keynames, vl, vr, d->ud);
}
#elif OS(DARWIN) || OS(FREEBSD) || OS(NETBSD) || OS(OPENBSD) || OS(WINDOWS)
static int cmp_variant(void *ud, const void *l, const void *r)
{
    struct elem_node *nl = *(struct elem_node**)l;
    struct elem_node *nr = *(struct elem_node**)r;
    purc_variant_t *vl = nl->kvs;
    purc_variant_t *vr = nr->kvs;
    struct set_user_data *d = (struct set_user_data*)ud;
    return d->cmp(d->nr_keynames, vl, vr, d->ud);
}
#else
#error Unsupported operating system.
#endif

int pcvariant_set_sort(purc_variant_t value, void *ud,
        int (*cmp)(size_t nr_keynames,
            purc_variant_t l[], purc_variant_t r[], void *ud))
{
    if (!value || value->type != PURC_VARIANT_TYPE_SET)
        return -1;

    variant_set_t data = pcv_set_get_data(value);
    struct pcutils_arrlist *al = data->arr;
    if (!al)
        return -1;
    void *arr = al->array;

    struct set_user_data d = {
        .cmp         = cmp,
        .ud          = ud,
        .nr_keynames = data->nr_keynames,
    };

#if OS(HURD) || OS(LINUX)
    qsort_r(arr, al->length, sizeof(struct elem_node*), cmp_variant, &d);
#elif OS(DARWIN) || OS(FREEBSD) || OS(NETBSD) || OS(OPENBSD)
    qsort_r(arr, al->length, sizeof(struct elem_node*), &d, cmp_variant);
#elif OS(WINDOWS)
    qsort_s(arr, al->length, sizeof(struct elem_node*), cmp_variant, &d);
#endif

    refresh_arr(al, 0);

    return 0;
}

purc_variant_t
pcvariant_set_find (purc_variant_t set, purc_variant_t value)
{
    PCVARIANT_CHECK_FAIL_RET(set && set->type==PVT(_SET) && value,
        PURC_VARIANT_INVALID);

    variant_set_t data = pcv_set_get_data(set);
    PC_ASSERT(data);
    PC_ASSERT(data->nr_keynames);

    purc_variant_t *kvs = variant_set_create_kvs(data, value);
    if (!kvs)
        return false;

    struct elem_node *p;
    p = find_element(data, kvs);
    free(kvs);

    return p ? p->elem : PURC_VARIANT_INVALID;
}

int pcvariant_set_get_uniqkeys(purc_variant_t set, size_t *nr_keynames,
        const char ***keynames)
{
    PCVARIANT_CHECK_FAIL_RET(set && set->type==PVT(_SET) &&
            nr_keynames && keynames, -1);

    variant_set_t data = pcv_set_get_data(set);
    PC_ASSERT(data);
    PC_ASSERT(data->nr_keynames);

    *nr_keynames = data->nr_keynames;
    *keynames = data->keynames;

    return 0;
}


