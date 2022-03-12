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
grow(purc_variant_t array, purc_variant_t pos, purc_variant_t value,
        bool check)
{
    if (!check)
        return true;

    purc_variant_t vals[] = { pos, value };

    return pcvariant_on_pre_fired(array, PCVAR_OPERATION_GROW,
            PCA_TABLESIZE(vals), vals);
}

static inline bool
shrink(purc_variant_t array, purc_variant_t pos, purc_variant_t value,
        bool check)
{
    if (!check)
        return true;

    purc_variant_t vals[] = { pos, value };

    return pcvariant_on_pre_fired(array, PCVAR_OPERATION_SHRINK,
            PCA_TABLESIZE(vals), vals);
}

static inline bool
change(purc_variant_t array, purc_variant_t pos,
        purc_variant_t o, purc_variant_t n,
        bool check)
{
    if (!check)
        return true;

    purc_variant_t vals[] = { pos, o, n };

    return pcvariant_on_pre_fired(array, PCVAR_OPERATION_CHANGE,
            PCA_TABLESIZE(vals), vals);
}

static inline void
grown(purc_variant_t array, purc_variant_t pos, purc_variant_t value,
        bool check)
{
    if (!check)
        return;

    purc_variant_t vals[] = { pos, value };

    pcvariant_on_post_fired(array, PCVAR_OPERATION_GROW,
            PCA_TABLESIZE(vals), vals);
}

static inline void
shrunk(purc_variant_t array, purc_variant_t pos, purc_variant_t value,
        bool check)
{
    if (!check)
        return;

    purc_variant_t vals[] = { pos, value };

    pcvariant_on_post_fired(array, PCVAR_OPERATION_SHRINK,
            PCA_TABLESIZE(vals), vals);
}

static inline void
changed(purc_variant_t array, purc_variant_t pos,
        purc_variant_t o, purc_variant_t n,
        bool check)
{
    if (!check)
        return;

    purc_variant_t vals[] = { pos, o, n };

    pcvariant_on_post_fired(array, PCVAR_OPERATION_CHANGE,
            PCA_TABLESIZE(vals), vals);
}

static void
arr_node_destroy(struct arr_node *node)
{
    if (node) {
        PURC_VARIANT_SAFE_CLEAR(node->val);
        free(node);
    }
}

static purc_variant_t
variant_arr_make_pos(variant_arr_t data, size_t idx)
{
    size_t len = variant_arr_length(data);
    if (idx > len)
        idx = len;

    return purc_variant_make_longint(idx);
}

static int
variant_arr_insert_before(purc_variant_t array, size_t idx, purc_variant_t val,
        bool check)
{
    if (purc_variant_is_undefined(val)) {
        // FIXME: `undefined` not allowed in array???
        return 0;
    }

    variant_arr_t data = (variant_arr_t)array->sz_ptr[1];
    purc_variant_t pos = variant_arr_make_pos(data, idx);
    if (pos == PURC_VARIANT_INVALID)
        return -1;

    if (!grow(array, pos, val, check)) {
        purc_variant_unref(pos);
        return -1;
    }

    struct arr_node *node;
    node = (struct arr_node*)calloc(1, sizeof(*node));
    if (!node) {
        purc_variant_unref(pos);
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }
    node->val = val;
    purc_variant_ref(val);

    struct pcutils_array_list *al = &data->al;
    int r = pcutils_array_list_insert_before(al, idx, &node->node);
    if (r) {
        arr_node_destroy(node);
        purc_variant_unref(pos);
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }

    struct pcvar_rev_update_edge edge = {
        .parent        = array,
        .arr_me        = node,
    };
    r = pcvar_build_edge_to_parent(val, &edge);
    if (r) {
        pcvar_break_edge_to_parent(node->val, &edge);
        arr_node_destroy(node);
        purc_variant_unref(pos);
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }

    grown(array, pos, val, check);
    purc_variant_unref(pos);

    return 0;
}

static int
variant_arr_append(purc_variant_t array, purc_variant_t val,
        bool check)
{
    variant_arr_t data = (variant_arr_t)array->sz_ptr[1];
    struct pcutils_array_list *al = &data->al;
    size_t nr = pcutils_array_list_length(al);
    return variant_arr_insert_before(array, nr, val, check);
}

static int
variant_arr_prepend(purc_variant_t array, purc_variant_t val,
        bool check)
{
    return variant_arr_insert_before(array, 0, val, check);
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
variant_arr_set(purc_variant_t array, size_t idx, purc_variant_t val,
        bool check)
{
    variant_arr_t data = (variant_arr_t)array->sz_ptr[1];
    purc_variant_t pos = variant_arr_make_pos(data, idx);
    if (pos == PURC_VARIANT_INVALID)
        return -1;

    struct pcutils_array_list *al = &data->al;
    struct pcutils_array_list_node *p;
    p = pcutils_array_list_get(al, idx);
    if (p == NULL) {
        purc_variant_unref(pos);
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }
    struct arr_node *old_node;
    old_node = (struct arr_node*)container_of(p, struct arr_node, node);
    PC_ASSERT(old_node->val != PURC_VARIANT_INVALID);
    if (!change(array, pos, old_node->val, val, check)) {
        purc_variant_unref(pos);
        return -1;
    }

    struct arr_node *node;
    node = (struct arr_node*)calloc(1, sizeof(*node));
    if (!node) {
        purc_variant_unref(pos);
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }
    node->val = val;
    purc_variant_ref(val);

    struct pcutils_array_list_node *old;
    int r = pcutils_array_list_set(al, idx, &node->node, &old);
    PC_ASSERT(r == 0);
    PC_ASSERT(old == p);

    struct pcvar_rev_update_edge edge = {
        .parent        = array,
        .arr_me        = old_node,
    };
    pcvar_break_edge_to_parent(old_node->val, &edge);

    edge.parent = array;
    edge.arr_me = node;
    r = pcvar_build_edge_to_parent(node->val, &edge);
    // FIXME: recoverable?
    PC_ASSERT(r == 0);

    changed(array, pos, old_node->val, val, check);
    purc_variant_unref(pos);

    arr_node_destroy(old_node);

    return 0;
}

static int
variant_arr_remove(purc_variant_t array, size_t idx,
        bool check)
{
    variant_arr_t data = (variant_arr_t)array->sz_ptr[1];
    purc_variant_t pos = variant_arr_make_pos(data, idx);
    if (pos == PURC_VARIANT_INVALID)
        return -1;

    struct pcutils_array_list *al = &data->al;

    struct pcutils_array_list_node *p, *n;
    p = pcutils_array_list_get(al, idx);
    if (!p) {
        purc_variant_unref(pos);
        pcinst_set_error(PURC_ERROR_OVERFLOW);
        return -1;
    }

    struct arr_node *node;
    node = (struct arr_node*)container_of(p, struct arr_node, node);
    PC_ASSERT(node->val);

    if (!shrink(array, pos, node->val, check)) {
        purc_variant_unref(pos);
        return -1;
    }

    int r = pcutils_array_list_remove(al, idx, &n);
    PC_ASSERT(r == 0);
    PC_ASSERT(n == p);

    struct pcvar_rev_update_edge edge = {
        .parent        = array,
        .arr_me        = node,
    };
    pcvar_break_edge_to_parent(node->val, &edge);

    shrunk(array, pos, node->val, check);
    purc_variant_unref(pos);

    arr_node_destroy(node);

    return 0;
}

static inline void
array_release (purc_variant_t value)
{
    variant_arr_t data = (variant_arr_t)value->sz_ptr[1];
    if (!data)
        return;

    struct pcutils_array_list *al = &data->al;
    struct arr_node *p, *n;
    array_list_for_each_entry_reverse_safe(al, p, n, node) {
        struct pcutils_array_list_node *node;
        int r = pcutils_array_list_remove(al, p->node.idx, &node);
        PC_ASSERT(r==0 && node && node == &p->node);
        struct pcvar_rev_update_edge edge = {
            .parent        = value,
            .arr_me        = p,
        };
        pcvar_break_edge_to_parent(p->val, &edge);
        arr_node_destroy(p);
    };

    pcutils_array_list_reset(al);
    free(data);
    value->sz_ptr[1] = (uintptr_t)NULL;

    pcvariant_stat_set_extra_size(value, 0);
}

static void
refresh_extra(purc_variant_t array)
{
    size_t extra = 0;
    variant_arr_t data = (variant_arr_t)array->sz_ptr[1];
    if (data) {
        extra += sizeof(*data);
        struct pcutils_array_list *al = &data->al;
        extra += al->sz * sizeof(*al->nodes);
        extra += al->nr * sizeof(struct arr_node);
    }
    pcvariant_stat_set_extra_size(array, extra);
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

        INIT_LIST_HEAD(&data->rev_update_chain);

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
            // question: shall we track mem for al->array?
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
        purc_variant_t l = (purc_variant_t)lal->array[i];
        purc_variant_t r = (purc_variant_t)ral->array[i];
        int t = pcvariant_array_compare(l, r);
        if (t)
            return t;
    }

    return i<lnr ? 1 : -1;
}
*/

bool purc_variant_array_append (purc_variant_t array, purc_variant_t value)
{
    PCVARIANT_CHECK_FAIL_RET(array && array->type==PVT(_ARRAY) && value,
        PURC_VARIANT_INVALID);

    bool check = true;
    int r = variant_arr_append(array, value, check);
    refresh_extra(array);
    return r ? false : true;
}

bool purc_variant_array_prepend (purc_variant_t array, purc_variant_t value)
{
    PCVARIANT_CHECK_FAIL_RET(array && array->type==PVT(_ARRAY) && value,
        PURC_VARIANT_INVALID);

    bool check = true;
    int r = variant_arr_prepend(array, value, check);
    refresh_extra(array);
    return r ? false : true;
}

purc_variant_t purc_variant_array_get (purc_variant_t array, int idx)
{
    PCVARIANT_CHECK_FAIL_RET(array && array->type==PVT(_ARRAY) && idx>=0,
        PURC_VARIANT_INVALID);

    variant_arr_t data = (variant_arr_t)array->sz_ptr[1];

    return variant_arr_get(data, idx);
}

bool purc_variant_array_size(purc_variant_t array, size_t *sz)
{
    PC_ASSERT(array && sz);

    PCVARIANT_CHECK_FAIL_RET(array->type==PVT(_ARRAY), false);

    variant_arr_t data = (variant_arr_t)array->sz_ptr[1];
    *sz = variant_arr_length(data);
    return true;
}

bool purc_variant_array_set (purc_variant_t array, int idx,
        purc_variant_t value)
{
    PCVARIANT_CHECK_FAIL_RET(array && array->type==PVT(_ARRAY) &&
        idx>=0 && value && array != value,
        PURC_VARIANT_INVALID);

    bool check = true;
    int r = variant_arr_set(array, idx, value, check);
    refresh_extra(array);
    return r ? false : true;
}

bool purc_variant_array_remove (purc_variant_t array, int idx)
{
    PCVARIANT_CHECK_FAIL_RET(array && array->type==PVT(_ARRAY) && idx>=0,
        PURC_VARIANT_INVALID);

    bool check = true;
    int r = variant_arr_remove(array, idx, check);
    refresh_extra(array);
    return r ? false : true;
}

bool purc_variant_array_insert_before (purc_variant_t array, int idx,
        purc_variant_t value)
{
    PCVARIANT_CHECK_FAIL_RET(array && array->type==PVT(_ARRAY) &&
        idx>=0 && value && array != value,
        PURC_VARIANT_INVALID);

    bool check = true;
    int r = variant_arr_insert_before(array, idx, value, check);
    refresh_extra(array);
    return r ? false : true;
}

bool purc_variant_array_insert_after (purc_variant_t array, int idx,
        purc_variant_t value)
{
    return purc_variant_array_insert_before(array, idx+1, value);
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

int pcvariant_array_sort(purc_variant_t value, void *ud,
        int (*cmp)(purc_variant_t l, purc_variant_t r, void *ud))
{
    if (!value || value->type != PURC_VARIANT_TYPE_ARRAY)
        return -1;

    variant_arr_t data = (variant_arr_t)value->sz_ptr[1];

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
pcvar_array_break_rev_update_edges(purc_variant_t arr)
{
    PC_ASSERT(purc_variant_is_array(arr));

    variant_arr_t data = (variant_arr_t)arr->sz_ptr[1];
    if (!data)
        return;

    struct arr_node *p;
    foreach_in_variant_array(arr, p) {
        struct pcvar_rev_update_edge edge = {
            .parent         = arr,
            .arr_me         = p,
        };
        pcvar_break_edge_to_parent(p->val, &edge);
    }
}

void
pcvar_array_break_edge_to_parent(purc_variant_t arr,
        struct pcvar_rev_update_edge *edge)
{
    PC_ASSERT(purc_variant_is_array(arr));
    variant_arr_t data = (variant_arr_t)arr->sz_ptr[1];
    if (!data)
        return;

    pcvar_break_edge(arr, &data->rev_update_chain, edge);
}

int
pcvar_array_build_rev_update_edges(purc_variant_t arr)
{
    PC_ASSERT(purc_variant_is_array(arr));

    variant_arr_t data = (variant_arr_t)arr->sz_ptr[1];
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
    }

    return 0;
}

int
pcvar_array_build_edge_to_parent(purc_variant_t arr,
        struct pcvar_rev_update_edge *edge)
{
    PC_ASSERT(purc_variant_is_array(arr));
    variant_arr_t data = (variant_arr_t)arr->sz_ptr[1];
    if (!data)
        return 0;

    return pcvar_build_edge(arr, &data->rev_update_chain, edge);
}

