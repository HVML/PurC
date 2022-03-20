/**
 * @file variant-array.c
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

#define _GNU_SOURCE

#include "config.h"
#include "private/variant.h"
#include "private/errors.h"
#include "variant-internals.h"
#include "purc-errors.h"
#include "purc-utils.h"

#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

static size_t
variant_arr_length(variant_arr_t data)
{
    struct pcutils_array_list *al = &data->al;
    return pcutils_array_list_length(al);
}

static inline bool
grow(purc_variant_t arr, purc_variant_t pos, purc_variant_t value,
        bool check)
{
    if (!check)
        return true;

    purc_variant_t vals[] = { pos, value };

    return pcvariant_on_pre_fired(arr, PCVAR_OPERATION_GROW,
            PCA_TABLESIZE(vals), vals);
}

static inline bool
shrink(purc_variant_t arr, purc_variant_t pos, purc_variant_t value,
        bool check)
{
    if (!check)
        return true;

    purc_variant_t vals[] = { pos, value };

    return pcvariant_on_pre_fired(arr, PCVAR_OPERATION_SHRINK,
            PCA_TABLESIZE(vals), vals);
}

static inline bool
change(purc_variant_t arr, purc_variant_t pos,
        purc_variant_t o, purc_variant_t n,
        bool check)
{
    if (!check)
        return true;

    purc_variant_t vals[] = { pos, o, n };

    return pcvariant_on_pre_fired(arr, PCVAR_OPERATION_CHANGE,
            PCA_TABLESIZE(vals), vals);
}

static inline void
grown(purc_variant_t arr, purc_variant_t pos, purc_variant_t value,
        bool check)
{
    if (!check)
        return;

    purc_variant_t vals[] = { pos, value };

    pcvariant_on_post_fired(arr, PCVAR_OPERATION_GROW,
            PCA_TABLESIZE(vals), vals);
}

static inline void
shrunk(purc_variant_t arr, purc_variant_t pos, purc_variant_t value,
        bool check)
{
    if (!check)
        return;

    purc_variant_t vals[] = { pos, value };

    pcvariant_on_post_fired(arr, PCVAR_OPERATION_SHRINK,
            PCA_TABLESIZE(vals), vals);
}

static inline void
changed(purc_variant_t arr, purc_variant_t pos,
        purc_variant_t o, purc_variant_t n,
        bool check)
{
    if (!check)
        return;

    purc_variant_t vals[] = { pos, o, n };

    pcvariant_on_post_fired(arr, PCVAR_OPERATION_CHANGE,
            PCA_TABLESIZE(vals), vals);
}

variant_arr_t
pcvar_arr_get_data(purc_variant_t arr)
{
    return (variant_arr_t)arr->sz_ptr[1];
}

static void
break_rev_update_chain(purc_variant_t arr, struct arr_node *node)
{
    struct pcvar_rev_update_edge edge = {
        .parent        = arr,
        .arr_me        = node,
    };

    pcvar_break_edge_to_parent(node->val, &edge);
    pcvar_break_rue_downward(node->val);
}

static void
arr_node_release(purc_variant_t arr, struct arr_node *node)
{
    if (!node)
        return;

    break_rev_update_chain(arr, node);

    if (node->node.idx != (size_t)-1) {
        variant_arr_t data = pcvar_arr_get_data(arr);
        PC_ASSERT(data);

        struct pcutils_array_list *al = &data->al;
        PC_ASSERT(al);

        struct pcutils_array_list_node *p;
        int r = pcutils_array_list_remove(al, node->node.idx, &p);
        PC_ASSERT(r == 0);
        PC_ASSERT(&node->node == p);
        PC_ASSERT(node->node.idx == (size_t)-1);
    }

    PURC_VARIANT_SAFE_CLEAR(node->val);
}

static void
arr_node_destroy(purc_variant_t arr, struct arr_node *node)
{
    if (!node)
        return;

    arr_node_release(arr, node);
    free(node);
}

static purc_variant_t
variant_arr_make_pos(variant_arr_t data, size_t idx)
{
    size_t len = variant_arr_length(data);
    if (idx > len)
        idx = len;

    return purc_variant_make_longint(idx);
}

static struct arr_node*
arr_node_create(purc_variant_t val)
{
    struct arr_node *node;
    node = (struct arr_node*)calloc(1, sizeof(*node));
    if (!node) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    node->node.idx = (size_t)-1;

    node->val = val;
    purc_variant_ref(val);

    return node;
}

static int
build_rev_update_chain(purc_variant_t arr, struct arr_node *node)
{
#if PURC_SET_CONSTRAINT_WITH_CLONE == 1
    if (!pcvar_container_belongs_to_set(arr))
        return 0;
#endif

    int r;

    struct pcvar_rev_update_edge edge = {
        .parent        = arr,
        .arr_me        = node,
    };

    r = pcvar_build_edge_to_parent(node->val, &edge);
    if (r == 0) {
        r = pcvar_build_rue_downward(node->val);
    }

    return r ? -1 : 0;
}

static int
variant_arr_insert_before(purc_variant_t arr, size_t idx, purc_variant_t val,
        bool check)
{
    if (purc_variant_is_undefined(val)) {
        // FIXME: `undefined` not allowed in arr???
        return 0;
    }

    variant_arr_t data = pcvar_arr_get_data(arr);
    PC_ASSERT(data);

    struct pcutils_array_list *al = &data->al;
    PC_ASSERT(al);

    size_t nr = pcutils_array_list_length(al);
    if (idx > nr)
        idx = nr;

#if PURC_SET_CONSTRAINT_WITH_CLONE == 1
    if (pcvar_container_belongs_to_set(val)) {
        purc_set_error_with_info(PURC_ERROR_NOT_SUPPORTED,
                "add subchildren of set's uniqkey-val to other container");
        return -1;
    }
#endif

    purc_variant_t pos = variant_arr_make_pos(data, idx);
    if (pos == PURC_VARIANT_INVALID)
        return -1;

    struct arr_node *node = NULL;

    do {
        if (!grow(arr, pos, val, check))
            break;

        node = arr_node_create(val);
        if (!node)
            break;

        int r;
        PC_ASSERT(node->node.idx == (size_t)-1);
        r = pcutils_array_list_insert_before(al, idx, &node->node);
        if (r) {
            PC_ASSERT(node->node.idx == (size_t)-1);
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            break;
        }
        PC_ASSERT(node->node.idx != (size_t)-1);

        if (build_rev_update_chain(arr, node))
            break;

        pcvar_adjust_set_by_descendant(arr);

        grown(arr, pos, val, check);
        purc_variant_unref(pos);

        return 0;
    } while (0);

    arr_node_destroy(arr, node);
    purc_variant_unref(pos);

    return -1;
}

static int
variant_arr_append(purc_variant_t arr, purc_variant_t val,
        bool check)
{
    variant_arr_t data = pcvar_arr_get_data(arr);
    struct pcutils_array_list *al = &data->al;
    size_t nr = pcutils_array_list_length(al);
    return variant_arr_insert_before(arr, nr, val, check);
}

static int
variant_arr_prepend(purc_variant_t arr, purc_variant_t val,
        bool check)
{
    return variant_arr_insert_before(arr, 0, val, check);
}

static purc_variant_t
variant_arr_get(variant_arr_t data, size_t idx)
{
    struct pcutils_array_list *al = &data->al;
    struct pcutils_array_list_node *p;
    p = pcutils_array_list_get(al, idx);
    if (p == NULL)
        return PURC_VARIANT_INVALID;

    struct arr_node *node;
    node = (struct arr_node*)container_of(p, struct arr_node, node);
    return node->val;
}

static int
variant_arr_set(purc_variant_t arr, size_t idx, purc_variant_t val,
        bool check)
{
    variant_arr_t data = pcvar_arr_get_data(arr);
    PC_ASSERT(data);

    struct pcutils_array_list *al = &data->al;
    PC_ASSERT(al);

    size_t nr = pcutils_array_list_length(al);
    if (idx >= nr) {
        purc_set_error(PURC_ERROR_OVERFLOW);
        return -1;
    }

    struct pcutils_array_list_node *p;
    p = pcutils_array_list_get(al, idx);
    PC_ASSERT(p);

    struct arr_node *old_node;
    old_node = (struct arr_node*)container_of(p, struct arr_node, node);
    PC_ASSERT(old_node->val != PURC_VARIANT_INVALID);
    if (old_node->val == val) {
        // NOTE: keep refc intact
        return 0;
    }

    purc_variant_t pos = variant_arr_make_pos(data, idx);
    if (pos == PURC_VARIANT_INVALID)
        return -1;

    do {
        purc_variant_t old = old_node->val;

        if (!change(arr, pos, old, val, check))
            break;

        old_node->val = val;
        if (build_rev_update_chain(arr, old_node)) {
            break_rev_update_chain(arr, old_node);
            old_node->val = old;
            break;
        }

        old_node->val = old;
        break_rev_update_chain(arr, old_node);

        old_node->val = purc_variant_ref(val);

        pcvar_adjust_set_by_descendant(arr);

        changed(arr, pos, old, val, check);

        purc_variant_unref(old);
        purc_variant_unref(pos);

        return 0;
    } while (0);

    purc_variant_unref(pos);

    return -1;
}

static int
variant_arr_remove(purc_variant_t arr, size_t idx,
        bool check)
{
    variant_arr_t data = pcvar_arr_get_data(arr);
    PC_ASSERT(data);

    struct pcutils_array_list *al = &data->al;
    PC_ASSERT(al);

    size_t nr = pcutils_array_list_length(al);
    if (idx >= nr) {
        // FIXME: failure or success???
        return 0;
    }

    purc_variant_t pos = variant_arr_make_pos(data, idx);
    if (pos == PURC_VARIANT_INVALID)
        return -1;

    struct pcutils_array_list_node *p, *n;
    p = pcutils_array_list_get(al, idx);
    PC_ASSERT(p);

    struct arr_node *node;
    node = (struct arr_node*)container_of(p, struct arr_node, node);
    PC_ASSERT(node->val);

    do {
        if (!shrink(arr, pos, node->val, check))
            break;

        break_rev_update_chain(arr, node);

        PC_ASSERT(node->node.idx != (size_t)-1);
        int r = pcutils_array_list_remove(al, idx, &n);
        PC_ASSERT(r == 0);
        PC_ASSERT(&node->node == n);
        PC_ASSERT(node->node.idx == (size_t)-1);

        pcvar_adjust_set_by_descendant(arr);

        shrunk(arr, pos, node->val, check);

        arr_node_destroy(arr, node);
        purc_variant_unref(pos);

        return 0;
    } while (0);

    purc_variant_unref(pos);

    return -1;
}

static inline void
array_release (purc_variant_t arr)
{
    variant_arr_t data = pcvar_arr_get_data(arr);
    if (!data)
        return;

    struct pcutils_array_list *al = &data->al;
    struct arr_node *p, *n;
    array_list_for_each_entry_reverse_safe(al, p, n, node) {
        arr_node_destroy(arr, p);
    };

    pcutils_array_list_reset(al);
    free(data);
    arr->sz_ptr[1] = (uintptr_t)NULL;

    pcvariant_stat_set_extra_size(arr, 0);
}

static void
refresh_extra(purc_variant_t arr)
{
    size_t extra = 0;
    variant_arr_t data = pcvar_arr_get_data(arr);
    if (data) {
        extra += sizeof(*data);
        struct pcutils_array_list *al = &data->al;
        extra += al->sz * sizeof(*al->nodes);
        extra += al->nr * sizeof(struct arr_node);
    }
    pcvariant_stat_set_extra_size(arr, extra);
}

static purc_variant_t
pv_make_array_n (bool check, size_t sz, purc_variant_t value0, va_list ap)
{
    PCVARIANT_CHECK_FAIL_RET((sz==0 && value0==NULL) || (sz > 0 && value0),
        PURC_VARIANT_INVALID);

    purc_variant_t var = pcvariant_get(PVT(_ARRAY));
    if (!var) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    do {
        var->type          = PVT(_ARRAY);
        var->flags         = PCVARIANT_FLAG_EXTRA_SIZE;
        var->refc          = 1;

        size_t initial_size = ARRAY_LIST_DEFAULT_SIZE;
        if (sz>initial_size)
            initial_size = sz;

        variant_arr_t data = (variant_arr_t)calloc(1, sizeof(*data));
        if (!data) {
            pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
            break;
        }

        struct pcutils_array_list *al;
        al = &data->al;
        if (pcutils_array_list_init(al) ||
            pcutils_array_list_expand(al, initial_size))
        {
            free(data);
            pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
            break;
        }

        var->sz_ptr[1]     = (uintptr_t)data;

        if (sz > 0) {
            purc_variant_t v = value0;
            // question: shall we track mem for al->arr?
            if (variant_arr_append(var, v, check)) {
                pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
                break;
            }

            size_t i = 1;
            while (i < sz) {
                v = va_arg(ap, purc_variant_t);
                if (!v) {
                    pcinst_set_error(PURC_ERROR_INVALID_VALUE);
                    break;
                }

                if (variant_arr_append(var, v, check)) {
                    pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
                    break;
                }

                i++;
            }

            if (i < sz)
                break;
        }

        refresh_extra(var);

        return var;

    } while (0);

    array_release(var);
    pcvariant_put(var);

    return PURC_VARIANT_INVALID;
}

purc_variant_t purc_variant_make_array (size_t sz, purc_variant_t value0, ...)
{
    bool check = true;
    purc_variant_t v;
    va_list ap;
    va_start(ap, value0);
    v = pv_make_array_n(check, sz, value0, ap);
    va_end(ap);

    return v;
}

void pcvariant_array_release (purc_variant_t value)
{
    array_release(value);
}

/* VWNOTE: unnecessary
int pcvariant_array_compare (purc_variant_t lv, purc_variant_t rv)
{
    // only called via purc_variant_compare
    struct pcutils_arrlist *lal = (struct pcutils_arrlist*)lv->sz_ptr[1];
    struct pcutils_arrlist *ral = (struct pcutils_arrlist*)rv->sz_ptr[1];
    size_t                  lnr = pcutils_arrlist_length(lal);
    size_t                  rnr = pcutils_arrlist_length(ral);

    size_t i = 0;
    for (; i<lnr && i<rnr; ++i) {
        purc_variant_t l = (purc_variant_t)lal->arr[i];
        purc_variant_t r = (purc_variant_t)ral->arr[i];
        int t = pcvariant_array_compare(l, r);
        if (t)
            return t;
    }

    return i<lnr ? 1 : -1;
}
*/

bool purc_variant_array_append (purc_variant_t arr, purc_variant_t value)
{
    PCVARIANT_CHECK_FAIL_RET(arr && arr->type==PVT(_ARRAY) && value,
        PURC_VARIANT_INVALID);

    bool check = true;
    int r = variant_arr_append(arr, value, check);
    refresh_extra(arr);
    return r ? false : true;
}

bool purc_variant_array_prepend (purc_variant_t arr, purc_variant_t value)
{
    PCVARIANT_CHECK_FAIL_RET(arr && arr->type==PVT(_ARRAY) && value,
        PURC_VARIANT_INVALID);

    bool check = true;
    int r = variant_arr_prepend(arr, value, check);
    refresh_extra(arr);
    return r ? false : true;
}

purc_variant_t purc_variant_array_get (purc_variant_t arr, int idx)
{
    PCVARIANT_CHECK_FAIL_RET(arr && arr->type==PVT(_ARRAY) && idx>=0,
        PURC_VARIANT_INVALID);

    variant_arr_t data = pcvar_arr_get_data(arr);

    return variant_arr_get(data, idx);
}

bool purc_variant_array_size(purc_variant_t arr, size_t *sz)
{
    PC_ASSERT(arr && sz);

    PCVARIANT_CHECK_FAIL_RET(arr->type==PVT(_ARRAY), false);

    variant_arr_t data = pcvar_arr_get_data(arr);
    *sz = variant_arr_length(data);
    return true;
}

bool purc_variant_array_set (purc_variant_t arr, int idx,
        purc_variant_t value)
{
    PCVARIANT_CHECK_FAIL_RET(arr && arr->type==PVT(_ARRAY) &&
        idx>=0 && value && arr != value,
        PURC_VARIANT_INVALID);

    bool check = true;
    int r = variant_arr_set(arr, idx, value, check);
    refresh_extra(arr);
    return r ? false : true;
}

bool purc_variant_array_remove (purc_variant_t arr, int idx)
{
    PCVARIANT_CHECK_FAIL_RET(arr && arr->type==PVT(_ARRAY) && idx>=0,
        PURC_VARIANT_INVALID);

    bool check = true;
    int r = variant_arr_remove(arr, idx, check);
    refresh_extra(arr);
    return r ? false : true;
}

bool purc_variant_array_insert_before (purc_variant_t arr, int idx,
        purc_variant_t value)
{
    PCVARIANT_CHECK_FAIL_RET(arr && arr->type==PVT(_ARRAY) &&
        idx>=0 && value && arr != value,
        PURC_VARIANT_INVALID);

    bool check = true;
    int r = variant_arr_insert_before(arr, idx, value, check);
    refresh_extra(arr);
    return r ? false : true;
}

bool purc_variant_array_insert_after (purc_variant_t arr, int idx,
        purc_variant_t value)
{
    return purc_variant_array_insert_before(arr, idx+1, value);
}

struct arr_user_data {
    int (*cmp)(purc_variant_t l, purc_variant_t r, void *ud);
    void *ud;
};

static int
sort_cmp(struct pcutils_array_list_node *l, struct pcutils_array_list_node *r,
        void *ud)
{
    struct arr_node *l_n, *r_n;
    l_n = container_of(l, struct arr_node, node);
    r_n = container_of(r, struct arr_node, node);

    struct arr_user_data *d = (struct arr_user_data*)ud;
    return d->cmp(l_n->val, r_n->val, d->ud);
}

int pcvariant_array_sort(purc_variant_t arr, void *ud,
        int (*cmp)(purc_variant_t l, purc_variant_t r, void *ud))
{
    if (!arr || arr->type != PURC_VARIANT_TYPE_ARRAY)
        return -1;

    variant_arr_t data = pcvar_arr_get_data(arr);

    struct arr_user_data d = {
        .cmp = cmp,
        .ud  = ud,
    };

    int r;
    r = pcutils_array_list_sort(&data->al, &d, sort_cmp);

    return r ? -1 : 0;
}

purc_variant_t
pcvariant_array_clone(purc_variant_t arr, bool recursively)
{
    purc_variant_t var;
    var = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    if (var == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    purc_variant_t v;
    size_t idx;
    foreach_value_in_variant_array(arr, v, idx) {
        UNUSED_PARAM(idx);
        purc_variant_t val;
        if (recursively) {
            val = pcvariant_container_clone(v, recursively);
        }
        else {
            val = purc_variant_ref(v);
        }
        if (val == PURC_VARIANT_INVALID) {
            purc_variant_unref(var);
            return PURC_VARIANT_INVALID;
        }
        bool ok;
        ok = purc_variant_array_append(var, val);
        purc_variant_unref(val);
        if (!ok) {
            purc_variant_unref(var);
            return PURC_VARIANT_INVALID;
        }
    } end_foreach;

    PC_ASSERT(var != arr);
    return var;
}

void
pcvar_array_break_rue_downward(purc_variant_t arr)
{
    PC_ASSERT(purc_variant_is_array(arr));

    variant_arr_t data = pcvar_arr_get_data(arr);
    if (!data)
        return;

    struct arr_node *p;
    foreach_in_variant_array(arr, p) {
        struct pcvar_rev_update_edge edge = {
            .parent         = arr,
            .arr_me         = p,
        };
        pcvar_break_edge_to_parent(p->val, &edge);
        pcvar_break_rue_downward(p->val);
    }
}

void
pcvar_array_break_edge_to_parent(purc_variant_t arr,
        struct pcvar_rev_update_edge *edge)
{
    PC_ASSERT(purc_variant_is_array(arr));
    variant_arr_t data = pcvar_arr_get_data(arr);
    if (!data)
        return;

    pcvar_break_edge(arr, &data->rev_update_chain, edge);
}

int
pcvar_array_build_rue_downward(purc_variant_t arr)
{
    PC_ASSERT(purc_variant_is_array(arr));

    variant_arr_t data = pcvar_arr_get_data(arr);
    if (!data)
        return 0;

    struct arr_node *p;
    foreach_in_variant_array(arr, p) {
        struct pcvar_rev_update_edge edge = {
            .parent         = arr,
            .arr_me         = p,
        };
        int r = pcvar_build_edge_to_parent(p->val, &edge);
        if (r)
            return -1;
        r = pcvar_build_rue_downward(p->val);
        if (r)
            return -1;
    }

    return 0;
}

int
pcvar_array_build_edge_to_parent(purc_variant_t arr,
        struct pcvar_rev_update_edge *edge)
{
    PC_ASSERT(purc_variant_is_array(arr));
    variant_arr_t data = pcvar_arr_get_data(arr);
    if (!data)
        return 0;

    return pcvar_build_edge(arr, &data->rev_update_chain, edge);
}

static struct pcutils_array_list_node*
next_node(struct pcutils_array_list *al, struct pcutils_array_list_node *node)
{
    if (!node)
        return NULL;
    size_t count = pcutils_array_list_length(al);
    size_t idx = node->idx + 1;
    if (idx >= count)
        return NULL;

    return pcutils_array_list_get(al, idx);
}

static struct pcutils_array_list_node*
prev_node(struct pcutils_array_list *al, struct pcutils_array_list_node *node)
{
    if (!node)
        return NULL;
    if (node->idx == 0)
        return NULL;

    size_t count = pcutils_array_list_length(al);
    size_t idx = node->idx - 1;
    if (idx >= count)
        return NULL;

    return pcutils_array_list_get(al, idx);
}

static void
it_refresh(struct arr_iterator *it,
        struct pcutils_array_list *al,
        struct pcutils_array_list_node *curr)
{
    struct pcutils_array_list_node *next  = NULL;
    struct pcutils_array_list_node *prev  = NULL;
    if (curr) {
        next  = next_node(al, curr);
        prev  = prev_node(al, curr);
    }

    if (curr) {
        it->curr = container_of(curr, struct arr_node, node);
    }
    else {
        it->curr = NULL;
    }

    if (next) {
        it->next = container_of(next, struct arr_node, node);
    }
    else {
        it->next = NULL;
    }

    if (prev) {
        it->prev = container_of(prev, struct arr_node, node);
    }
    else {
        it->prev = NULL;
    }
}

struct arr_iterator
pcvar_arr_it_first(purc_variant_t arr)
{
    struct arr_iterator it = {
        .arr         = arr,
    };
    if (arr == PURC_VARIANT_INVALID)
        return it;

    variant_arr_t data = pcvar_arr_get_data(arr);
    size_t count = variant_arr_length(data);
    if (count == 0)
        return it;

    struct pcutils_array_list *al = &data->al;

    struct pcutils_array_list_node *first;
    first = pcutils_array_list_get_first(al);

    it_refresh(&it, al, first);

    return it;
}

struct arr_iterator
pcvar_arr_it_last(purc_variant_t arr)
{
    struct arr_iterator it = {
        .arr         = arr,
    };
    if (arr == PURC_VARIANT_INVALID)
        return it;

    variant_arr_t data = pcvar_arr_get_data(arr);
    size_t count = variant_arr_length(data);
    if (count == 0)
        return it;

    struct pcutils_array_list *al = &data->al;

    struct pcutils_array_list_node *last;
    last = pcutils_array_list_get_last(al);

    it_refresh(&it, al, last);

    return it;
}

void
pcvar_arr_it_next(struct arr_iterator *it)
{
    if (it->curr == NULL)
        return;

    if (it->next) {
        variant_arr_t data = pcvar_arr_get_data(it->arr);
        struct pcutils_array_list *al = &data->al;

        struct pcutils_array_list_node *next;
        next = &it->next->node;

        it_refresh(it, al, next);
    }
    else {
        it->curr = NULL;
        it->next = NULL;
        it->prev = NULL;
    }
}

void
pcvar_arr_it_prev(struct arr_iterator *it)
{
    if (it->curr == NULL)
        return;

    if (it->prev) {
        variant_arr_t data = pcvar_arr_get_data(it->arr);
        struct pcutils_array_list *al = &data->al;

        struct pcutils_array_list_node *prev;
        prev = &it->prev->node;

        it_refresh(it, al, prev);
    }
    else {
        it->curr = NULL;
        it->next = NULL;
        it->prev = NULL;
    }
}


