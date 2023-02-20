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
#include "private/stringbuilder.h"
#include "purc-errors.h"
#include "variant-internals.h"


#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

static bool
grow(purc_variant_t set, purc_variant_t value,
        bool check)
{
    if (!check)
        return true;

    purc_variant_t vals[] = { value };

    return pcvariant_on_pre_fired(set, PCVAR_OPERATION_GROW,
            PCA_TABLESIZE(vals), vals);
}

static bool
shrink(purc_variant_t set, purc_variant_t value,
        bool check)
{
    if (!check)
        return true;

    purc_variant_t vals[] = { value };

    return pcvariant_on_pre_fired(set, PCVAR_OPERATION_SHRINK,
            PCA_TABLESIZE(vals), vals);
}

static bool
change(purc_variant_t set,
        purc_variant_t o, purc_variant_t n,
        bool check)
{
    if (!check)
        return true;

    purc_variant_t vals[] = { o, n };

    return pcvariant_on_pre_fired(set, PCVAR_OPERATION_CHANGE,
            PCA_TABLESIZE(vals), vals);
}

static void
grown(purc_variant_t set, purc_variant_t value,
        bool check)
{
    if (!check)
        return;

    purc_variant_t vals[] = { value };

    pcvariant_on_post_fired(set, PCVAR_OPERATION_GROW,
            PCA_TABLESIZE(vals), vals);
}

static void
shrunk(purc_variant_t set, purc_variant_t value,
        bool check)
{
    if (!check)
        return;

    purc_variant_t vals[] = { value };

    pcvariant_on_post_fired(set, PCVAR_OPERATION_SHRINK,
            PCA_TABLESIZE(vals), vals);
}

static void
changed(purc_variant_t set,
        purc_variant_t o, purc_variant_t n,
        bool check)
{
    if (!check)
        return;

    purc_variant_t vals[] = { o, n };

    pcvariant_on_post_fired(set, PCVAR_OPERATION_CHANGE,
            PCA_TABLESIZE(vals), vals);
}

variant_set_t
pcvar_set_get_data(purc_variant_t set)
{
    return (variant_set_t)set->sz_ptr[1];
}

static size_t
variant_set_get_extra_size(variant_set_t data)
{
    size_t extra = 0;
    if (data->unique_key) {
        extra += strlen(data->unique_key) + 1;
        extra += sizeof(*data->keynames) * data->nr_keynames;
    }
    size_t sz_record = sizeof(struct set_node) +
        sizeof(purc_variant_t) * data->nr_keynames;
    size_t count = 0;
    count = pcutils_array_list_length(&data->al);

    extra += sz_record * count;
    extra += sizeof(struct set_node*)*(data->al.nr);

    return extra;
}

static void
pcv_set_set_data(purc_variant_t set, variant_set_t data)
{
    set->sz_ptr[1]     = (uintptr_t)data;
}

static int
obj_node_diff(struct obj_node *l, struct obj_node *r)
{
    int diff = 0;

    purc_variant_t lk, rk;
    lk = l->key;
    rk = r->key;
    PC_ASSERT(lk);
    PC_ASSERT(rk);

    if (lk != rk) {
        diff = pcvariant_diff(lk, rk);
        if (diff)
            return diff;
    }

    purc_variant_t lv, rv;
    lv = l->val;
    rv = r->val;
    PC_ASSERT(lv);
    PC_ASSERT(rv);

    if (lv != rv) {
        diff = pcvariant_diff(lv, rv);
    }

    return diff;
}

static int
variant_set_compare_by_set_keys(purc_variant_t set,
        purc_variant_t l, purc_variant_t r)
{
    PC_ASSERT(set != PURC_VARIANT_INVALID);
    PC_ASSERT(purc_variant_is_set(set));
    PC_ASSERT(l != PURC_VARIANT_INVALID);
    PC_ASSERT(r != PURC_VARIANT_INVALID);
    PC_ASSERT(pcvariant_is_mutable(l));
    PC_ASSERT(pcvariant_is_mutable(r));

    int diff;

    struct kv_iterator lit, rit;
    lit = pcvar_kv_it_first(set, l);
    rit = pcvar_kv_it_first(set, r);
    while (1) {
        struct obj_node *ln = lit.it.curr;
        struct obj_node *rn = rit.it.curr;
        if (ln == NULL && rn == NULL)
            return 0;
        if (ln == NULL)
            return -1;
        if (rn == NULL)
            return 1;

        diff = obj_node_diff(ln, rn);
        if (diff)
            return diff;

        pcvar_kv_it_next(&lit);
        pcvar_kv_it_next(&rit);
    }
}

static int
variant_set_init(variant_set_t data, const char *unique_key, bool caseless)
{
    data->caseless = caseless;

    data->elems = RB_ROOT;
    pcutils_array_list_init(&data->al);

    if (!unique_key || !*unique_key) {
        // empty key
        data->nr_keynames = 1;
        PC_ASSERT(data->keynames == NULL);
        PC_ASSERT(data->unique_key == NULL);
        return 0;
    }

    data->unique_key = strdup(unique_key);
    if (!data->unique_key) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }

    size_t n = strlen(data->unique_key);
    data->keynames = (const char**)calloc(n, sizeof(*data->keynames));
    if (!data->keynames) {
        free(data->unique_key);
        data->unique_key = NULL;
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }

    // strcpy(data->unique_key, unique_key);
    char *ctx = data->unique_key;
    char *tok = strtok_r(ctx, " ", &ctx);
    size_t idx = 0;
    while (tok) {
        data->keynames[idx++] = tok;
        tok = strtok_r(ctx, " ", &ctx);
    }

    if (idx==0) {
        // no content in key
        free(data->unique_key);
        data->unique_key = NULL;
        data->nr_keynames = 1;
        return 0;
    }

    PC_ASSERT(idx>0);
    data->nr_keynames = idx;

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
    set->flags         = PCVRNT_FLAG_EXTRA_SIZE;

    variant_set_t data  = (variant_set_t)calloc(1, sizeof(*data));
    pcv_set_set_data(set, data);

    if (!data) {
        pcvariant_put(set);
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    set->refc          = 1;

    size_t extra = variant_set_get_extra_size(data);
    pcvariant_stat_set_extra_size(set, extra);

    // a valid empty set
    return set;
}

static void
break_rev_update_chain(purc_variant_t set, struct set_node *node)
{
    PC_ASSERT(node);
    PC_ASSERT(node->val);

    if (!pcvariant_is_mutable(node->val))
        return;

    struct pcvar_rev_update_edge edge = {
        .parent        = set,
        .set_me        = node,
    };
    pcvar_break_edge_to_parent(node->val, &edge);

    if (node->val->type == PVT(_OBJECT)) {
        struct kv_iterator it;
        it = pcvar_kv_it_first(set, node->val);
        while (1) {
            struct obj_node *on = it.it.curr;
            if (on == NULL)
                break;
            if (pcvariant_is_mutable(on->val)) {
                struct pcvar_rev_update_edge edge = {
                    .parent        = node->val,
                    .obj_me        = on,
                };
                pcvar_break_edge_to_parent(on->val, &edge);
                pcvar_break_rue_downward(on->val);
            }
            pcvar_kv_it_next(&it);
        }
    }
    else if (node->val->type == PVT(_ARRAY) ||
            node->val->type == PVT(_SET) ||
            node->val->type == PVT(_TUPLE))
    {
        pcvar_break_rue_downward(node->val);
    }
    else {
        PC_ASSERT(0);
    }
}

static void
elem_node_revoke_constraints(purc_variant_t set, struct set_node *node)
{
    if (!node)
        return;

    if (node->val == PURC_VARIANT_INVALID)
        return;

    break_rev_update_chain(set, node);
}

struct element_rb_node {
    struct rb_node     **pnode;
    struct rb_node      *parent;
    struct rb_node      *entry;
};

static int
_compare_generic(purc_variant_t _new, purc_variant_t _old, bool caseless)
{
    pcvrnt_compare_method_k opt = PCVRNT_COMPARE_METHOD_CASE;
    if (caseless)
        opt = PCVRNT_COMPARE_METHOD_CASELESS;

    int diff;
    diff = purc_variant_compare_ex(_new, _old, opt);

    // FIXME: what if allocation failed internally in purc_variant_compare_ex?

    return diff;
}

static purc_variant_t
_get_by_key(purc_variant_t val, const char *key)
{
    purc_variant_t v = PURC_VARIANT_INVALID;

    if (purc_variant_is_object(val)) {
        v = purc_variant_object_get_by_ckey(val, key);

        if (v == PURC_VARIANT_INVALID) {
            PC_ASSERT(purc_get_last_error() != PURC_ERROR_OUT_OF_MEMORY);
            purc_clr_error();
        }
    }

    if (v != PURC_VARIANT_INVALID)
        return purc_variant_ref(v);

    return purc_variant_make_undefined();
}

static int
_compare_by_unique_keys(purc_variant_t _new, purc_variant_t _old,
        variant_set_t data)
{
    int diff = 0;

    for (size_t i=0; i<data->nr_keynames; ++i) {
        const char *key = data->keynames[i];

        purc_variant_t _new_v = _get_by_key(_new, key);
        purc_variant_t _old_v = _get_by_key(_old, key);

        PC_ASSERT(_new_v != PURC_VARIANT_INVALID);
        PC_ASSERT(_old_v != PURC_VARIANT_INVALID);

        diff = _compare_generic(_new_v, _old_v, data->caseless);
        PURC_VARIANT_SAFE_CLEAR(_new_v);
        PURC_VARIANT_SAFE_CLEAR(_old_v);

        if (diff)
            break;
    }

    return diff;
}

static int
_compare(purc_variant_t _new, purc_variant_t _old,
        variant_set_t data)
{
    PC_ASSERT(_new != PURC_VARIANT_INVALID);
    PC_ASSERT(_old != PURC_VARIANT_INVALID);
    if (data->unique_key == NULL) {
        // generic set
        return _compare_generic(_new, _old, data->caseless);
    }

    return _compare_by_unique_keys(_new, _old, data);
}

static void
find_element_rb_node(struct element_rb_node *node,
        purc_variant_t set, purc_variant_t kvs)
{
    variant_set_t data = pcvar_set_get_data(set);
    struct rb_root *root = &data->elems;
    struct rb_node **pnode = &root->rb_node;
    struct rb_node *parent = NULL;
    struct rb_node *entry = NULL;
    char md5[33];
    pcvariant_md5_by_set(md5, kvs, set);

    while (*pnode) {
        struct set_node *on;
        on = container_of(*pnode, struct set_node, rbnode);
        int diff;
        if (0) {
            diff = variant_set_compare_by_set_keys(set, kvs, on->val);
        }
        else if (0) {
            diff = pcvariant_diff_by_set(md5, kvs, on->md5, on->val, set);
        }
        else {
            diff = _compare(kvs, on->val, data);
        }

        parent = *pnode;

        if (diff < 0) {
            pnode = &parent->rb_left;
        }
        else if (diff > 0) {
            pnode = &parent->rb_right;
        }
        else{
            entry = *pnode;
            break;
        }
    }

    node->pnode  = pnode;
    node->parent = parent;
    node->entry  = entry;
}

static struct set_node*
find_element(purc_variant_t set, purc_variant_t kvs)
{
    struct element_rb_node node;
    find_element_rb_node(&node, set, kvs);

    if (!node.entry)
        return NULL;

    return container_of(node.entry, struct set_node, rbnode);
}

static int
build_rev_update_chain(purc_variant_t set, struct set_node *node)
{
    PC_ASSERT(node);
    PC_ASSERT(node->val);

    if (!pcvariant_is_mutable(node->val))
        return 0;

    struct pcvar_rev_update_edge edge = {
        .parent        = set,
        .set_me        = node,
    };
    int r = pcvar_build_edge_to_parent(node->val, &edge);
    if (r)
        return -1;

    if (node->val->type == PVT(_OBJECT)) {
        struct kv_iterator it;
        it = pcvar_kv_it_first(set, node->val);
        while (1) {
            struct obj_node *on = it.it.curr;
            if (on == NULL)
                break;
            if (pcvariant_is_mutable(on->val)) {
                struct pcvar_rev_update_edge edge = {
                    .parent        = node->val,
                    .obj_me        = on,
                };
                int r;
                r = pcvar_build_edge_to_parent(on->val, &edge);
                if (r == 0) {
                    r = pcvar_build_rue_downward(on->val);
                }
                if (r)
                    return -1;
            }
            pcvar_kv_it_next(&it);
        }
    }
    else if (node->val->type == PVT(_ARRAY) ||
            node->val->type == PVT(_SET) ||
            node->val->type == PVT(_TUPLE))
    {
        int r;
        r = pcvar_build_rue_downward(node->val);
        if (r)
            return -1;
    }
    else {
        PC_ASSERT(0);
    }

    return 0;
}

static bool
elem_node_setup_constraints(purc_variant_t set, struct set_node *node)
{
    variant_set_t data = pcvar_set_get_data(set);
    PC_ASSERT(data);

    purc_variant_t elem = node->val;
    PC_ASSERT(elem != PURC_VARIANT_INVALID);

    int r;
    r = build_rev_update_chain(set, node);
    if (r)
        return false;

    return true;
}

static void
elem_node_remove(purc_variant_t set, struct set_node *node)
{
    if (node->alnode.idx == (size_t)-1)
        return;

    variant_set_t data = pcvar_set_get_data(set);
    PC_ASSERT(data);

    pcutils_rbtree_erase(&node->rbnode, &data->elems);

    int r;
    struct pcutils_array_list_node *old;
    struct pcutils_array_list *al = &data->al;
    r = pcutils_array_list_remove(al, node->alnode.idx, &old);
    PC_ASSERT(r == 0);
    PC_ASSERT(old == &node->alnode);
    PC_ASSERT(node->alnode.idx == (size_t)-1);
}

static void
elem_node_release(purc_variant_t set, struct set_node *node)
{
    if (!node)
        return;

    elem_node_revoke_constraints(set, node);
    elem_node_remove(set, node);

    PURC_VARIANT_SAFE_CLEAR(node->val);
}

static void
elem_node_destroy(purc_variant_t set, struct set_node *node)
{
    if (!node)
        return;

    elem_node_release(set, node);
    free(node);
}

static int
elem_node_replace(purc_variant_t set, struct set_node *node,
        purc_variant_t val, bool check)
{
    PC_ASSERT(node->val != PURC_VARIANT_INVALID);

    purc_variant_ref(val);

    if (check) {
        elem_node_revoke_constraints(set, node);
    }

    PURC_VARIANT_SAFE_CLEAR(node->val);

    node->val = val;

    if (check) {
        if (!elem_node_setup_constraints(set, node))
            return -1;
    }

    return 0;
}

static void
variant_set_release_elems(purc_variant_t set, variant_set_t data)
{
    struct pcutils_array_list *al = &data->al;
    struct pcutils_array_list_node *p, *n;
    for (p = pcutils_array_list_get_last(al);
            ({ n = p ? pcutils_array_list_get(al,  p->idx-1) : NULL;
             p; });
            p = n)
    {
        struct set_node *sn;
        sn = container_of(p, struct set_node, alnode);
        elem_node_destroy(set, sn);
    }

    pcutils_array_list_reset(&data->al);
}

static void
variant_set_release(purc_variant_t set, variant_set_t data)
{
    variant_set_release_elems(set, data);

    if (data->rev_update_chain) {
        pcvar_destroy_rev_update_chain(data->rev_update_chain);
        data->rev_update_chain = NULL;
    }

    free(data->keynames);
    data->keynames = NULL;
    data->nr_keynames = 0;
    free(data->unique_key);
    data->unique_key = NULL;
}

static purc_variant_t
variant_set_create_kvs_n(variant_set_t set, purc_variant_t v1, va_list ap)
{
    PC_ASSERT(set->keynames);
    PC_ASSERT(v1 != PURC_VARIANT_INVALID);

    purc_variant_t kvs;
    kvs = pcvar_make_obj();
    if (kvs == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    for (size_t i=0; i<set->nr_keynames; ++i) {
        purc_variant_t v;
        if (i == 0)
            v = v1;
        else
            v = va_arg(ap, purc_variant_t);

        if (v == PURC_VARIANT_INVALID) {
            purc_variant_unref(kvs);
            pcinst_set_error(PURC_ERROR_INVALID_VALUE);
            return PURC_VARIANT_INVALID;
        }
        if (purc_variant_is_undefined(v))
            continue;

        const char *sk = set->keynames[i];
        bool ok;
        ok = purc_variant_object_set_by_static_ckey(kvs, sk, v);
        if (!ok) {
            purc_variant_unref(kvs);
            return PURC_VARIANT_INVALID;
        }
    }

    return kvs;
}

static struct set_node*
variant_set_create_elem_node(purc_variant_t set, purc_variant_t val)
{
    variant_set_t data = pcvar_set_get_data(set);
    PC_ASSERT(data);

    struct set_node *_new = (struct set_node*)calloc(1, sizeof(*_new));
    if (!_new) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    pcvariant_md5_by_set(_new->md5, val, set);

    _new->alnode.idx = (size_t)-1;
    _new->val = val;
    purc_variant_ref(val);

    return _new;
}

static int
check_shrink(purc_variant_t set, struct set_node *node)
{
    if (!pcvar_container_belongs_to_set(set))
        return 0;

    purc_variant_t _new = pcvar_set_clone_struct(set);
    if (_new == PURC_VARIANT_INVALID)
        return -1;

    int r = 0;
    do {
        bool found = false;
        purc_variant_t v;
        foreach_value_in_variant_set(set, v) {
            if (node->val == v) {
                PC_ASSERT(!found);
                found = true;
                continue;
            }
            r = pcvar_set_add(_new, v);
            if (r)
                break;
        } end_foreach;

        if (r)
            break;

        if (!found)
            break;

        r = pcvar_reverse_check(set, _new);
        if (r)
            break;

        PURC_VARIANT_SAFE_CLEAR(_new);

        return 0;
    } while (0);

    PURC_VARIANT_SAFE_CLEAR(_new);
    return -1;
}

static int
set_remove(purc_variant_t set, struct set_node *node,
        bool check)
{
    do {
        if (check) {
            if (!shrink(set, node->val, check))
                break;

            if (check_shrink(set, node))
                break;

            elem_node_revoke_constraints(set, node);
        }

        elem_node_remove(set, node);

        if (check) {
            pcvar_adjust_set_by_descendant(set);

            shrunk(set, node->val, check);
        }

        elem_node_destroy(set, node);

        return 0;
    } while (0);

    return -1;
}

static int
check_grow(purc_variant_t set, purc_variant_t val)
{
    if (!pcvar_container_belongs_to_set(set))
        return 0;

    purc_variant_t _new = pcvar_set_clone_struct(set);
    if (_new == PURC_VARIANT_INVALID)
        return -1;

    int r = 0;
    do {
        purc_variant_t v;
        foreach_value_in_variant_set(set, v) {
            r = pcvar_set_add(_new, v);
            if (r)
                break;
        } end_foreach;

        if (r)
            break;

        r = pcvar_set_add(_new, val);
        if (r)
            break;

        r = pcvar_reverse_check(set, _new);
        if (r)
            break;

        PURC_VARIANT_SAFE_CLEAR(_new);

        return 0;
    } while (0);

    PURC_VARIANT_SAFE_CLEAR(_new);
    return -1;
}

static int
insert(purc_variant_t set, variant_set_t data,
        purc_variant_t val,
        struct rb_node *parent, struct rb_node **pnode,
        bool check)
{
    struct set_node *node = NULL;

    do {
        if (check) {
            if (!grow(set, val, check))
                break;

            if (check_grow(set, val))
                break;
        }

        node = variant_set_create_elem_node(set, val);
        if (!node)
            break;

        PC_ASSERT(node->alnode.idx == (size_t)-1);
        int r = pcutils_array_list_append(&data->al, &node->alnode);
        if (r)
            break;
        PC_ASSERT(node->alnode.idx != (size_t)-1);

        size_t count = pcutils_array_list_length(&data->al);
        node->alnode.idx = count - 1;

        struct rb_node *entry = &node->rbnode;

        pcutils_rbtree_link_node(entry, parent, pnode);
        pcutils_rbtree_insert_color(entry, &data->elems);

        if (check) {
            if (!elem_node_setup_constraints(set, node))
                break;

            pcvar_adjust_set_by_descendant(set);

            grown(set, node->val, check);
        }

        return 0;
    } while (0);

    elem_node_destroy(set, node);

    return -1;
}

int
pcvar_set_add(purc_variant_t set, purc_variant_t val)
{
    variant_set_t data = pcvar_set_get_data(set);
    PC_ASSERT(data);

    struct element_rb_node rbn;
    find_element_rb_node(&rbn, set, val);

    if (rbn.entry) {
        purc_set_error(PURC_ERROR_DUPLICATED);
        return -1;
    }

    bool check = false;
    return insert(set, data, val, rbn.parent, rbn.pnode, check);
}

static int
check_change(purc_variant_t set, struct set_node *node, purc_variant_t val)
{
    if (!pcvar_container_belongs_to_set(set))
        return 0;

    purc_variant_t _new = pcvar_set_clone_struct(set);
    if (_new == PURC_VARIANT_INVALID)
        return -1;

    int r = 0;
    do {
        bool found = false;
        purc_variant_t v;
        foreach_value_in_variant_set(set, v) {
            if (node->val == v) {
                PC_ASSERT(!found);
                found = true;
                r = pcvar_set_add(_new, val);
            }
            else {
                r = pcvar_set_add(_new, v);
            }
            if (r)
                break;
        } end_foreach;

        if (r)
            break;

        if (!found)
            break;

        r = pcvar_reverse_check(set, _new);
        if (r)
            break;

        PURC_VARIANT_SAFE_CLEAR(_new);

        return 0;
    } while (0);

    PURC_VARIANT_SAFE_CLEAR(_new);
    return -1;
}

 /* returns: The number of new members or changed members (1 or 0),
  -1 for error. */
static int
insert_or_replace(purc_variant_t set,
        variant_set_t data, purc_variant_t val, pcvrnt_cr_method_k cr_method,
        bool check)
{
    struct element_rb_node rbn;
    find_element_rb_node(&rbn, set, val);

    if (!rbn.entry) {
        int r = insert(set, data, val, rbn.parent, rbn.pnode, check);

        return (r == 0) ? 1 : 0;
    }

    struct set_node *curr;
    curr = container_of(rbn.entry, struct set_node, rbnode);

    if (curr->val == val) {
        return 0;
    }

    switch (cr_method) {
    case PCVRNT_CR_METHOD_IGNORE:
        return 0;

    case PCVRNT_CR_METHOD_OVERWRITE:
        break;

    case PCVRNT_CR_METHOD_COMPLAIN:
        purc_set_error(PURC_ERROR_DUPLICATED);
        break;
    }

    purc_variant_t _old = purc_variant_ref(curr->val);

    do {
        if (check) {
            if (!change(set, _old, val, check))
                break;

            if (check_change(set, curr, val))
                break;
        }

        if (elem_node_replace(set, curr, val, check))
            break;

        if (check) {
            pcvar_adjust_set_by_descendant(set);

            changed(set, _old, val, check);
        }

        PURC_VARIANT_SAFE_CLEAR(_old);

        return 1;
    } while (0);

    PURC_VARIANT_SAFE_CLEAR(_old);

    return -1;
}

 /* returns: The number of new members or changed members (1 or 0),
  -1 for error. */
static int
variant_set_add_val(purc_variant_t set,
        variant_set_t data, purc_variant_t val, pcvrnt_cr_method_k cr_method,
        bool check)
{
    if (!val) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    return insert_or_replace(set, data, val, cr_method, check);
}

static int
variant_set_add_valsn(purc_variant_t set, variant_set_t data,
        pcvrnt_cr_method_k cr_method, bool check, size_t sz, va_list ap)
{
    size_t i = 0;
    while (i<sz) {
        purc_variant_t v = va_arg(ap, purc_variant_t);
        if (!v) {
            pcinst_set_error(PURC_ERROR_INVALID_VALUE);
            break;
        }

        if (-1 == variant_set_add_val(set, data, v, cr_method, check)) {
            break;
        }

        ++i;
    }
    return i<sz ? -1 : 0;
}

static purc_variant_t
make_set_0(const char *unique_key, bool caseless)
{
    purc_variant_t set = pcv_set_new();
    if (set==PURC_VARIANT_INVALID) {
        return PURC_VARIANT_INVALID;
    }

    do {
        variant_set_t data = pcvar_set_get_data(set);
        if (variant_set_init(data, unique_key, caseless))
            break;

        size_t extra = variant_set_get_extra_size(data);
        pcvariant_stat_set_extra_size(set, extra);
        return set;
    } while (0);

    // cleanup
    purc_variant_unref(set);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
make_set_c(bool check, size_t sz, const char *unique_key,
    bool caseless, purc_variant_t value0, va_list ap)
{
    purc_variant_t set = make_set_0(unique_key, caseless);
    if (set==PURC_VARIANT_INVALID) {
        return PURC_VARIANT_INVALID;
    }

    do {
        variant_set_t data = pcvar_set_get_data(set);

        if (sz>0) {
            purc_variant_t  v = value0;
            if (-1 == variant_set_add_val(set, data, v,
                        PCVRNT_CR_METHOD_OVERWRITE, check))
                break;

            int r = variant_set_add_valsn(set, data,
                    PCVRNT_CR_METHOD_OVERWRITE, check, sz-1, ap);
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

purc_variant_t
purc_variant_make_set_by_ckey_ex(size_t sz, const char* unique_key,
    bool caseless, purc_variant_t value0, ...)
{
    PCVRNT_CHECK_FAIL_RET((sz==0 && value0==NULL) || (sz>0 && value0),
        PURC_VARIANT_INVALID);

    bool check = true;
    purc_variant_t v;
    va_list ap;
    va_start(ap, value0);
    v = make_set_c(check, sz, unique_key, caseless, value0, ap);
    va_end(ap);

    return v;
}

ssize_t
purc_variant_set_add(purc_variant_t set, purc_variant_t value,
        pcvrnt_cr_method_k cr_method)
{
    if (!(set && set->type==PVT(_SET) && value)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    // FIXME: shall clear error here???
    purc_clr_error();

    variant_set_t data = pcvar_set_get_data(set);
    PC_ASSERT(data);

    ssize_t r = variant_set_add_val(set, data, value, cr_method, true);

    if (r > 0) {
        size_t extra = variant_set_get_extra_size(data);
        pcvariant_stat_set_extra_size(set, extra);
    }

    return r;
}

ssize_t
purc_variant_set_remove(purc_variant_t set, purc_variant_t value,
        pcvrnt_nr_method_k nr_method)
{
    if (!(set && set->type==PVT(_SET) && value)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    variant_set_t data = pcvar_set_get_data(set);
    PC_ASSERT(data);
    PC_ASSERT(data->nr_keynames);

    bool check = true;
    int r = 0;
    struct set_node *p;
    p = find_element(set, value);
    if (p) {
        r = set_remove(set, p, check);
        if (r == 0) {
            return 1;
        }
        return -1;
    }

    if (nr_method == PCVRNT_NR_METHOD_COMPLAIN) {
        purc_set_error(PURC_ERROR_NOT_FOUND);
        return -1;
    }

    return 0;
}

purc_variant_t
purc_variant_set_get_member_by_key_values(purc_variant_t set,
        purc_variant_t v1, ...)
{
    PCVRNT_CHECK_FAIL_RET(set && set->type==PVT(_SET) && v1,
        PURC_VARIANT_INVALID);

    variant_set_t data = pcvar_set_get_data(set);
    if (!data || !data->unique_key || data->nr_keynames==0) {
        pcinst_set_error(PURC_ERROR_NOT_SUPPORTED);
        return PURC_VARIANT_INVALID;
    }

    va_list ap;
    va_start(ap, v1);
    purc_variant_t kvs = variant_set_create_kvs_n(data, v1, ap);
    va_end(ap);
    if (kvs == PURC_VARIANT_INVALID)
        return false;

    struct set_node *p;
    p = find_element(set, kvs);
    purc_variant_unref(kvs);

    return p ? p->val: PURC_VARIANT_INVALID;
}

purc_variant_t
purc_variant_set_remove_member_by_key_values(purc_variant_t set,
        purc_variant_t v1, ...)
{
    PCVRNT_CHECK_FAIL_RET(set && set->type==PVT(_SET) && v1,
        PURC_VARIANT_INVALID);

    variant_set_t data = pcvar_set_get_data(set);
    if (!data || !data->unique_key || data->nr_keynames==0) {
        pcinst_set_error(PURC_ERROR_NOT_SUPPORTED);
        return PURC_VARIANT_INVALID;
    }

    va_list ap;
    va_start(ap, v1);
    purc_variant_t kvs = variant_set_create_kvs_n(data, v1, ap);
    va_end(ap);
    if (kvs == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    struct set_node *p;
    p = find_element(set, kvs);
    purc_variant_unref(kvs);

    if (!p) {
        pcinst_set_error(PCVRNT_ERROR_NOT_FOUND);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t v = p->val;
    purc_variant_ref(v);

    bool check = true;
    int r = set_remove(set, p, check);
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

    PCVRNT_CHECK_FAIL_RET(set->type == PVT(_SET), false);

    variant_set_t data = pcvar_set_get_data(set);

    PC_ASSERT(data);
    size_t count = pcutils_array_list_length(&data->al);
    *sz = count;

    return true;
}

purc_variant_t
purc_variant_set_get_by_index(purc_variant_t set, size_t idx)
{
    PC_ASSERT(set);

    variant_set_t data = pcvar_set_get_data(set);
    size_t count = pcutils_array_list_length(&data->al);

    if (idx >= count)
        return PURC_VARIANT_INVALID;

    struct pcutils_array_list_node *alnode;
    alnode = pcutils_array_list_get(&data->al, idx);
    struct set_node *node;
    node = container_of(alnode, struct set_node, alnode);
    PC_ASSERT(node);
    PC_ASSERT(node->alnode.idx == (size_t)idx);
    PC_ASSERT(node->val != PURC_VARIANT_INVALID);

    return node->val;
}

PCA_EXPORT purc_variant_t
purc_variant_set_remove_by_index(purc_variant_t set, size_t idx)
{
    PC_ASSERT(set);

    variant_set_t data = pcvar_set_get_data(set);
    size_t count = pcutils_array_list_length(&data->al);

    if (idx >= count) {
        pcinst_set_error(PCVRNT_ERROR_OUT_OF_BOUNDS);
        return PURC_VARIANT_INVALID;
    }

    struct pcutils_array_list_node *alnode;
    alnode = pcutils_array_list_get(&data->al, idx);
    PC_ASSERT(alnode);
    struct set_node *node;
    node = container_of(alnode, struct set_node, alnode);
    PC_ASSERT(node);
    PC_ASSERT(node->alnode.idx == (size_t)idx);

    purc_variant_t v = node->val;
    purc_variant_ref(v);

    bool check = true;
    int r = set_remove(set, node, check);
    if (r) {
        purc_variant_unref(v);
        return PURC_VARIANT_INVALID;
    }

    size_t extra = variant_set_get_extra_size(data);
    pcvariant_stat_set_extra_size(set, extra);

    return v;
}

PCA_EXPORT bool
purc_variant_set_set_by_index(purc_variant_t set,
        size_t idx, purc_variant_t val)
{
    PC_ASSERT(set);

    variant_set_t data = pcvar_set_get_data(set);
    size_t count = pcutils_array_list_length(&data->al);

    if (idx >= count) {
        pcinst_set_error(PCVRNT_ERROR_OUT_OF_BOUNDS);
        return false;
    }

    struct pcutils_array_list_node *alnode;
    alnode = pcutils_array_list_get(&data->al, idx);
    PC_ASSERT(alnode);
    struct set_node *node;
    node = container_of(alnode, struct set_node, alnode);
    if (node->val == val)
        return true;

    purc_variant_t v = purc_variant_set_remove_by_index(set, idx);
    PC_ASSERT(v != PURC_VARIANT_INVALID);
    bool ok = purc_variant_set_add(set, val, true);
    PC_ASSERT(ok);
    purc_variant_unref(v);
    return ok;
}

struct pcvrnt_set_iterator {
    purc_variant_t      set;
    struct rb_node     *curr;
    struct rb_node     *prev, *next;
};

static void
iterator_refresh(struct pcvrnt_set_iterator *it)
{
    if (it->curr == NULL) {
        it->next = NULL;
        it->prev = NULL;
        return;
    }
    variant_set_t data = pcvar_set_get_data(it->set);
    size_t count = pcutils_array_list_length(&data->al);
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

struct pcvrnt_set_iterator*
pcvrnt_set_iterator_create_begin(purc_variant_t set)
{
    PCVRNT_CHECK_FAIL_RET(set && set->type==PVT(_SET),
        NULL);

    variant_set_t data = pcvar_set_get_data(set);
    PC_ASSERT(data);

    size_t count = pcutils_array_list_length(&data->al);
    if (count == 0) {
        pcinst_set_error(PCVRNT_ERROR_NOT_FOUND);
        return NULL;
    }

    struct pcvrnt_set_iterator *it;
    it = (struct pcvrnt_set_iterator*)calloc(1, sizeof(*it));
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

struct pcvrnt_set_iterator*
pcvrnt_set_iterator_create_end(purc_variant_t set)
{
    PCVRNT_CHECK_FAIL_RET(set && set->type==PVT(_SET),
        NULL);

    variant_set_t data = pcvar_set_get_data(set);
    PC_ASSERT(data);

    size_t count = pcutils_array_list_length(&data->al);
    if (count == 0) {
        pcinst_set_error(PCVRNT_ERROR_NOT_FOUND);
        return NULL;
    }

    struct pcvrnt_set_iterator *it;
    it = (struct pcvrnt_set_iterator*)calloc(1, sizeof(*it));
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
pcvrnt_set_iterator_release(struct pcvrnt_set_iterator* it)
{
    if (!it)
        return;
    free(it);
}

bool
pcvrnt_set_iterator_next(struct pcvrnt_set_iterator* it)
{
    PCVRNT_CHECK_FAIL_RET(it && it->set &&
        it->set->type==PVT(_SET) && it->curr,
        false);

    variant_set_t data = pcvar_set_get_data(it->set);
    PC_ASSERT(data);

    it->curr = it->next;
    iterator_refresh(it);

    return it->curr ? true : false;
}

bool
pcvrnt_set_iterator_prev(struct pcvrnt_set_iterator* it)
{
    PCVRNT_CHECK_FAIL_RET(it && it->set &&
        it->set->type==PVT(_SET) && it->curr,
        false);

    variant_set_t data = pcvar_set_get_data(it->set);
    PC_ASSERT(data);

    it->curr = it->prev;
    iterator_refresh(it);

    return it->curr ? true : false;
}

purc_variant_t
pcvrnt_set_iterator_get_value(struct pcvrnt_set_iterator* it)
{
    PCVRNT_CHECK_FAIL_RET(it && it->set &&
        it->set->type==PVT(_SET) && it->curr,
        PURC_VARIANT_INVALID);

    struct set_node *p;
    p = container_of(it->curr, struct set_node, rbnode);
    return p->val;
}

void
pcvariant_set_release(purc_variant_t value)
{
    variant_set_t data = pcvar_set_get_data(value);
    PC_ASSERT(data);

    variant_set_release(value, data);
    free(data);
    pcv_set_set_data(value, NULL);

    pcvariant_stat_set_extra_size(value, 0);
}

/* VWNOTE: unnecessary
int pcvariant_set_compare(purc_variant_t lv, purc_variant_t rv)
{
    variant_set_t ldata = _pcvar_set_get_data(lv);
    variant_set_t rdata = _pcvar_set_get_data(rv);
    PC_ASSERT(ldata && rdata);

    struct set_node *ln, *rn;
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
    int (*cmp)(purc_variant_t l, purc_variant_t r, void *ud);
    void *ud;
};

static int vrtcmp(purc_variant_t l, purc_variant_t r, void *ud)
{
    uintptr_t sort_flags;
    pcvrnt_compare_method_k cmpopt;

    sort_flags = (uintptr_t)ud;
    cmpopt = (pcvrnt_compare_method_k)(sort_flags & PCVRNT_CMPOPT_MASK);

    int retv = purc_variant_compare_ex(l, r, cmpopt);
    if (sort_flags & PCVRNT_SORT_DESC)
        retv = -retv;
    return retv;
}

static int
cmp_f(struct pcutils_array_list_node *l, struct pcutils_array_list_node *r,
        void *ud)
{
    struct set_user_data *d;
    d = (struct set_user_data*)ud;
    PC_ASSERT(d);
    PC_ASSERT(d->cmp);

    struct set_node *nl = container_of(l, struct set_node, alnode);
    struct set_node *nr = container_of(r, struct set_node, alnode);

    return d->cmp(nl->val, nr->val, d->ud);
}

int pcvariant_set_sort(purc_variant_t value, void *ud,
        int (*cmp)(purc_variant_t l, purc_variant_t r, void *ud))
{
    PC_ASSERT(value != PURC_VARIANT_INVALID);

    variant_set_t data = pcvar_set_get_data(value);
    struct pcutils_array_list *al = &data->al;

    struct set_user_data d = {
        .cmp = cmp ? cmp : vrtcmp,
        .ud  = ud,
    };

    pcutils_array_list_sort(al, &d, cmp_f);

    return 0;
}

purc_variant_t
pcvariant_set_find(purc_variant_t set, purc_variant_t value)
{
    PCVRNT_CHECK_FAIL_RET(set && set->type==PVT(_SET) && value,
            PURC_VARIANT_INVALID);

    variant_set_t data = pcvar_set_get_data(set);
    PC_ASSERT(data);
    PC_ASSERT(data->nr_keynames);

    struct set_node *p;
    p = find_element(set, value);

    return p ? p->val : PURC_VARIANT_INVALID;
}

int pcvariant_set_get_uniqkeys(purc_variant_t set, size_t *nr_keynames,
        const char ***keynames)
{
    PCVRNT_CHECK_FAIL_RET(set && set->type==PVT(_SET) &&
            nr_keynames && keynames, -1);

    variant_set_t data = pcvar_set_get_data(set);
    PC_ASSERT(data);
    PC_ASSERT(data->nr_keynames);

    *nr_keynames = data->nr_keynames;
    *keynames = data->keynames;

    return 0;
}

purc_variant_t
pcvariant_set_clone(purc_variant_t set, bool recursively)
{
    purc_variant_t var;
    var = pcvar_set_clone_struct(set);
    if (var == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    purc_variant_t v;
    // NOTE: keep document-order
    foreach_value_in_variant_set(set, v) {
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
        ok = purc_variant_set_add(var, val, false);
        purc_variant_unref(val);
        if (!ok) {
            purc_variant_unref(var);
            return PURC_VARIANT_INVALID;
        }
    } end_foreach;

    PC_ASSERT(var != set);
    return var;
}

void
pcvar_set_break_edge_to_parent(purc_variant_t set,
        struct pcvar_rev_update_edge *edge)
{
    PC_ASSERT(purc_variant_is_set(set));
    variant_set_t data = pcvar_set_get_data(set);
    if (!data)
        return;

    if (!data->rev_update_chain)
        return;

    pcutils_map_erase(data->rev_update_chain, edge->set_me);
}

int
pcvar_set_build_edge_to_parent(purc_variant_t set,
        struct pcvar_rev_update_edge *edge)
{
    PC_ASSERT(purc_variant_is_set(set));
    variant_set_t data = pcvar_set_get_data(set);
    if (!data)
        return 0;

    if (!data->rev_update_chain) {
        data->rev_update_chain = pcvar_create_rev_update_chain();
        if (!data->rev_update_chain)
            return -1;
    }

    pcutils_map_entry *entry;
    entry = pcutils_map_find(data->rev_update_chain, edge->set_me);
    if (entry)
        return 0;

    int r;
    r = pcutils_map_insert(data->rev_update_chain,
            edge->set_me, edge->parent);

    return r ? -1 : 0;
}

static struct set_node*
next_node(struct set_iterator *it, struct set_node *curr)
{
    variant_set_t data = pcvar_set_get_data(it->set);
    PC_ASSERT(data);

    if (it->it_type == SET_IT_ARRAY) {
        struct pcutils_array_list *arr = &data->al;
        PC_ASSERT(arr);
        size_t count = pcutils_array_list_length(arr);
        size_t idx = curr->alnode.idx + 1;
        if (idx >= count)
            return NULL;

        struct pcutils_array_list_node *alnode;
        alnode = pcutils_array_list_get(arr, idx);
        PC_ASSERT(alnode);
        return container_of(alnode, struct set_node, alnode);
    }

    if (it->it_type == SET_IT_RBTREE) {
        struct rb_node *p = pcutils_rbtree_next(&curr->rbnode);
        if (!p)
            return NULL;
        return container_of(p, struct set_node, rbnode);
    }

    PC_ASSERT(0);
    return NULL;
}

static struct set_node*
prev_node(struct set_iterator *it, struct set_node *curr)
{
    variant_set_t data = pcvar_set_get_data(it->set);
    PC_ASSERT(data);

    if (it->it_type == SET_IT_ARRAY) {
        if (curr->alnode.idx == 0)
            return NULL;

        struct pcutils_array_list *arr = &data->al;
        PC_ASSERT(arr);
        size_t count = pcutils_array_list_length(arr);
        size_t idx = curr->alnode.idx - 1;
        if (idx >= count)
            return NULL;

        struct pcutils_array_list_node *alnode;
        alnode = pcutils_array_list_get(arr, idx);
        PC_ASSERT(alnode);
        return container_of(alnode, struct set_node, alnode);
    }

    if (it->it_type == SET_IT_RBTREE) {
        struct rb_node *p = pcutils_rbtree_prev(&curr->rbnode);
        if (!p)
            return NULL;
        return container_of(p, struct set_node, rbnode);
    }

    PC_ASSERT(0);
    return NULL;
}

static void
it_refresh(struct set_iterator *it, struct set_node *curr)
{
    struct set_node *next  = NULL;
    struct set_node *prev  = NULL;
    if (curr) {
        next = next_node(it, curr);
        prev = prev_node(it, curr);
    }

    it->curr = curr;
    it->next = next;
    it->prev = prev;
}

struct set_iterator
pcvar_set_it_first(purc_variant_t set, enum set_it_type it_type)
{
    struct set_iterator it = {
        .set         = set,
        .it_type     = it_type,
    };
    if (set == PURC_VARIANT_INVALID)
        return it;

    variant_set_t data = pcvar_set_get_data(set);
    if (data == NULL)
        return it;

    struct rb_root *root = &data->elems;

    struct pcutils_array_list *arr = &data->al;
    if (arr == NULL)
        return it;

    size_t count = pcutils_array_list_length(arr);
    if (count == 0)
        return it;

    struct set_node *curr = NULL;

    if (it_type == SET_IT_ARRAY) {
        struct pcutils_array_list_node *alnode;
        alnode = pcutils_array_list_get(arr, 0);
        PC_ASSERT(alnode);
        curr = container_of(alnode, struct set_node, alnode);
    }
    else if (it_type == SET_IT_RBTREE) {
        struct rb_node *p = pcutils_rbtree_first(root);
        PC_ASSERT(p);
        curr = container_of(p, struct set_node, rbnode);
    }
    else {
        PC_ASSERT(0);
    }

    it_refresh(&it, curr);
    return it;
}

struct set_iterator
pcvar_set_it_last(purc_variant_t set, enum set_it_type it_type)
{
    struct set_iterator it = {
        .set         = set,
        .it_type     = it_type,
    };
    if (set == PURC_VARIANT_INVALID)
        return it;

    variant_set_t data = pcvar_set_get_data(set);
    if (data == NULL)
        return it;

    struct rb_root *root = &data->elems;

    struct pcutils_array_list *arr = &data->al;
    if (arr == NULL)
        return it;

    size_t count = pcutils_array_list_length(arr);
    if (count == 0)
        return it;

    struct set_node *curr = NULL;

    if (it_type == SET_IT_ARRAY) {
        struct pcutils_array_list_node *alnode;
        alnode = pcutils_array_list_get(arr, count-1);
        PC_ASSERT(alnode);
        curr = container_of(alnode, struct set_node, alnode);
    }
    else if (it_type == SET_IT_RBTREE) {
        struct rb_node *p = pcutils_rbtree_last(root);
        PC_ASSERT(p);
        curr = container_of(p, struct set_node, rbnode);
    }
    else {
        PC_ASSERT(0);
    }

    it_refresh(&it, curr);
    return it;
}

void
pcvar_set_it_next(struct set_iterator *it)
{
    if (it->curr == NULL)
        return;

    if (it->next) {
        it_refresh(it, it->next);
    }
    else {
        it->curr = NULL;
        it->next = NULL;
        it->prev = NULL;
    }
}

void
pcvar_set_it_prev(struct set_iterator *it)
{
    if (it->curr == NULL)
        return;

    if (it->prev) {
        it_refresh(it, it->prev);
    }
    else {
        it->curr = NULL;
        it->next = NULL;
        it->prev = NULL;
    }
}


struct kv_iterator
pcvar_kv_it_first(purc_variant_t set, purc_variant_t obj)
{
    struct kv_iterator it = {
        .set         = set,
    };
    if (set == PURC_VARIANT_INVALID)
        return it;

    variant_set_t data = pcvar_set_get_data(set);
    if (!data)
        return it;

    if (obj == PURC_VARIANT_INVALID)
        return it;

    if (data->keynames == NULL) {
        it.it = pcvar_obj_it_first(obj);
        return it;
    }

    PC_ASSERT(data->nr_keynames > 0);

    it.it = pcvar_obj_it_first(obj);

    while (it.it.curr) {
        struct obj_node *curr = it.it.curr;
        purc_variant_t key = curr->key;
        const char *sk = purc_variant_get_string_const(key);
        for (size_t i=0; i<data->nr_keynames; ++i) {
            const char *s = data->keynames[i];
            if (data->caseless) {
                if (pcutils_strcasecmp(s, sk) == 0) {
                    it.accu = 1;
                    return it;
                }
            }
            else {
                if (strcmp(s, sk) == 0) {
                    it.accu = 1;
                    return it;
                }
            }
        }
        pcvar_obj_it_next(&it.it);
    }

    return it;
}

void
pcvar_kv_it_next(struct kv_iterator *it)
{
    if (it->it.curr == NULL)
        return;

    variant_set_t data = pcvar_set_get_data(it->set);

    if (data->keynames == NULL) {
        pcvar_obj_it_next(&it->it);
        return;
    }

    if (it->accu >= data->nr_keynames) {
        it->it.curr = NULL;
        it->it.next = NULL;
        it->it.prev = NULL;
        return;
    }

    while (1) {
        pcvar_obj_it_next(&it->it);
        if (it->it.curr == NULL)
            return;
        struct obj_node *curr = it->it.curr;
        purc_variant_t key = curr->key;
        const char *sk = purc_variant_get_string_const(key);
        for (size_t i=0; i<data->nr_keynames; ++i) {
            const char *s = data->keynames[i];
            if (data->caseless) {
                if (pcutils_strcasecmp(s, sk) == 0) {
                    it->accu += 1;
                    return;
                }
            }
            else {
                if (strcmp(s, sk) == 0) {
                    it->accu += 1;
                    return;
                }
            }
        }
    }
}

purc_variant_t
pcvar_set_clone_struct(purc_variant_t set)
{
    PC_ASSERT(set != PURC_VARIANT_INVALID);
    PC_ASSERT(purc_variant_is_set(set));

    int r;
    struct pcutils_string str;
    pcutils_string_init(&str, 32);
    variant_set_t data = pcvar_set_get_data(set);
    if (data->keynames) {
        r = 0;
        for (size_t i=0; i<data->nr_keynames; ++i) {
            if (i) {
                r = pcutils_string_append_chunk(&str, " ", 1);
                if (r) {
                    purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
                    break;
                }
            }
            r = pcutils_string_append_str(&str, data->keynames[i]);
            if (r) {
                purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
                break;
            }
        }
        if (r) {
            pcutils_string_reset(&str);
            return PURC_VARIANT_INVALID;
        }
    }

    purc_variant_t var;
    var = purc_variant_make_set_by_ckey(0, str.abuf, PURC_VARIANT_INVALID);
    pcutils_string_reset(&str);

    return var;
}

purc_variant_t
pcvar_make_set(variant_set_t data)
{
    int r;
    struct pcutils_string str;
    pcutils_string_init(&str, 32);

    bool caseless = data->caseless;

    if (data->keynames) {
        r = 0;
        for (size_t i=0; i<data->nr_keynames; ++i) {
            if (i) {
                r = pcutils_string_append_chunk(&str, " ", 1);
                if (r) {
                    purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
                    break;
                }
            }
            r = pcutils_string_append_str(&str, data->keynames[i]);
            if (r) {
                purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
                break;
            }
        }
        if (r) {
            pcutils_string_reset(&str);
            return PURC_VARIANT_INVALID;
        }
    }

    purc_variant_t var;
    var = make_set_0(str.abuf, caseless);
    pcutils_string_reset(&str);

    return var;
}

void
pcvar_adjust_set_by_edge(purc_variant_t set,
        struct pcvar_rev_update_edge *edge)
{
    PC_ASSERT(set != PURC_VARIANT_INVALID);
    PC_ASSERT(purc_variant_is_set(set));
    PC_ASSERT(edge);
    PC_ASSERT(set == edge->parent);

    PC_ASSERT(0);
}

int
pcvar_readjust_set(purc_variant_t set, struct set_node *node)
{
    PC_ASSERT(set != PURC_VARIANT_INVALID);
    PC_ASSERT(purc_variant_is_set(set));
    variant_set_t data = pcvar_set_get_data(set);

    pcutils_rbtree_erase(&node->rbnode, &data->elems);

    struct element_rb_node rbn;
    find_element_rb_node(&rbn, set, node->val);
    PC_ASSERT(rbn.entry == NULL);

    struct rb_node *entry = &node->rbnode;

    pcutils_rbtree_link_node(entry, rbn.parent, rbn.pnode);
    pcutils_rbtree_insert_color(entry, &data->elems);

    return 0;
}

ssize_t
purc_variant_set_unite(purc_variant_t set, purc_variant_t value,
            pcvrnt_cr_method_k cr_method)
{
    ssize_t ret = -1;
    ssize_t r;
    if (set == PURC_VARIANT_INVALID || value == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    if (set == value) {
        purc_set_error(PURC_ERROR_INVALID_OPERAND);
        goto out;
    }

    if (!purc_variant_is_set(set) || !pcvariant_is_linear_container(value)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }

    ssize_t sz = purc_variant_linear_container_get_size(value);
    ret = 0;
    for (ssize_t i = 0; i < sz; i++) {
        purc_variant_t v = purc_variant_linear_container_get(value, i);
        if (!v) {
            continue;
        }
        r = purc_variant_set_add(set, v, cr_method);
        if (r == -1) {
            ret = -1;
            goto out;
        }
        ret += r;
    }

out:
    return ret;
}

static bool
is_in_array(purc_variant_t array, purc_variant_t v, int* idx)
{
    bool ret = false;
    purc_variant_t val;
    size_t curr;
    UNUSED_VARIABLE(val);
    foreach_value_in_variant_array_safe(array, val, curr)
        if (val == v) {
            if (idx) {
                *idx = curr;
            }
            ret = true;
            goto end;
        }
    end_foreach;

end:
    return ret;
}


ssize_t
purc_variant_set_intersect(purc_variant_t set, purc_variant_t value)
{
    ssize_t ret = -1;
    purc_variant_t tmp = PURC_VARIANT_INVALID;
    if (set == PURC_VARIANT_INVALID || value == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    if (set == value) {
        purc_set_error(PURC_ERROR_INVALID_OPERAND);
        goto out;
    }

    if (!purc_variant_is_set(set) || !pcvariant_is_linear_container(value)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }

    tmp = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    if (tmp == PURC_VARIANT_INVALID) {
        goto out;
    }

    ssize_t sz = purc_variant_linear_container_get_size(value);
    for (ssize_t i = 0; i < sz; i++) {
        purc_variant_t v = purc_variant_linear_container_get(value, i);
        if (!v) {
            continue;
        }

        purc_variant_t vf = pcvariant_set_find(set, v);
        if (vf == PURC_VARIANT_INVALID) {
            continue;
        }

        if (!purc_variant_array_append(tmp, vf)) {
            ret = -1;
            goto out;
        }
    }

    purc_variant_t v;
    foreach_value_in_variant_set_safe(set, v)
        if (is_in_array(tmp, v, NULL)) {
            continue;
        }

        if (-1 == purc_variant_set_remove(set, v, PCVRNT_NR_METHOD_COMPLAIN)) {
            goto out;
        }
    end_foreach;


    ret = purc_variant_set_get_size(set);
out:
    if (tmp) {
        purc_variant_unref(tmp);
    }
    return ret;
}

ssize_t
purc_variant_set_subtract(purc_variant_t set, purc_variant_t value)
{
    ssize_t ret = -1;
    if (set == PURC_VARIANT_INVALID || value == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    if (set == value) {
        purc_set_error(PURC_ERROR_INVALID_OPERAND);
        goto out;
    }

    if (!purc_variant_is_set(set) || !pcvariant_is_linear_container(value)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }

    ssize_t sz = purc_variant_linear_container_get_size(value);
    for (ssize_t i = 0; i < sz; i++) {
        purc_variant_t v = purc_variant_linear_container_get(value, i);
        if (!v) {
            continue;
        }

        if (-1 == purc_variant_set_remove(set, v, PCVRNT_NR_METHOD_IGNORE)) {
            goto out;
        }
    }

    ret = purc_variant_set_get_size(set);
out:
    return ret;
}

ssize_t
purc_variant_set_xor(purc_variant_t set, purc_variant_t value)
{
    ssize_t ret = -1;
    if (set == PURC_VARIANT_INVALID || value == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    if (set == value) {
        purc_set_error(PURC_ERROR_INVALID_OPERAND);
        goto out;
    }

    if (!purc_variant_is_set(set) || !pcvariant_is_linear_container(value)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }

    ssize_t sz = purc_variant_linear_container_get_size(value);
    for (ssize_t i = 0; i < sz; i++) {
        purc_variant_t v = purc_variant_linear_container_get(value, i);
        if (!v) {
            continue;
        }

        ssize_t r = purc_variant_set_remove(set, v, PCVRNT_NR_METHOD_IGNORE);
        if (r == 0) {
            if (-1 == purc_variant_set_add(set, v, PCVRNT_CR_METHOD_COMPLAIN)) {
                goto out;
            }
        }
        else if (r == -1) {
            goto out;
        }
    }

    ret = purc_variant_set_get_size(set);
out:
    return ret;
}

ssize_t
purc_variant_set_overwrite(purc_variant_t set, purc_variant_t value,
        pcvrnt_nr_method_k nr_method)
{
    ssize_t ret = -1;
    if (set == PURC_VARIANT_INVALID || value == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    if (set == value) {
        purc_set_error(PURC_ERROR_INVALID_OPERAND);
        goto out;
    }

    if (!purc_variant_is_set(set)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }

    if (purc_variant_is_object(value)) {
        purc_variant_t vf = pcvariant_set_find(set, value);
        if (vf != PURC_VARIANT_INVALID) {
            if (-1 == purc_variant_set_add(set, value,
                        PCVRNT_CR_METHOD_OVERWRITE)) {
                goto out;
            }
        }
        else if (nr_method == PCVRNT_NR_METHOD_COMPLAIN) {
            purc_set_error(PCVRNT_ERROR_NOT_FOUND);
            goto out;
        }
    }
    else if (pcvariant_is_linear_container(value)) {
        ssize_t sz = purc_variant_linear_container_get_size(value);
        for (ssize_t i = 0; i < sz; i++) {
            purc_variant_t v = purc_variant_linear_container_get(value, i);
            if (!v) {
                continue;
            }

            purc_variant_t vf = pcvariant_set_find(set, v);
            if (vf != PURC_VARIANT_INVALID) {
                if (-1 == purc_variant_set_add(set, v,
                            PCVRNT_CR_METHOD_OVERWRITE)) {
                    goto out;
                }
            }
            else if (nr_method == PCVRNT_NR_METHOD_COMPLAIN) {
                purc_set_error(PCVRNT_ERROR_NOT_FOUND);
                goto out;
            }
        }
    }
    else {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }

    ret = purc_variant_set_get_size(set);
out:
    return ret;
}

