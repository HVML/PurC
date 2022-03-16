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
variant_set_get_extra_size(variant_set_t set)
{
    size_t extra = 0;
    if (set->unique_key) {
        extra += strlen(set->unique_key) + 1;
        extra += sizeof(*set->keynames) * set->nr_keynames;
    }
    size_t sz_record = sizeof(struct set_node) +
        sizeof(purc_variant_t) * set->nr_keynames;
    size_t count = pcutils_arrlist_length(set->arr);
    extra += sz_record * count;
    extra += sizeof(*set->arr);
    extra += sizeof(struct set_node*)*(set->arr->size);

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
        diff = pcvariant_equal(lk, rk);
        if (diff)
            return diff;
    }

    purc_variant_t lv, rv;
    lv = l->val;
    rv = r->val;
    PC_ASSERT(lv);
    PC_ASSERT(rv);

    if (lv != rv) {
        diff = pcvariant_equal(lv, rv);
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
        PC_ASSERT(set->keynames == NULL);
        PC_ASSERT(set->unique_key == NULL);
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
elem_node_revoke_constraints(struct set_node *node)
{
    if (!node)
        return;

    if (node->set == PURC_VARIANT_INVALID)
        return;

    if (node->elem == PURC_VARIANT_INVALID)
        return;

    if (node->constraints) {
        PC_ASSERT(node->elem);
        bool ok;
        ok = purc_variant_revoke_listener(node->elem, node->constraints);
        PC_ASSERT(ok);
        node->constraints = NULL;
    }

    struct pcvar_rev_update_edge edge = {
        .parent        = node->set,
        .set_me        = node,
    };
    pcvar_break_edge_to_parent(node->elem, &edge);

    struct kv_iterator it;
    it = pcvar_kv_it_first(node->set, node->elem);
    while (1) {
        struct obj_node *on = it.it.curr;
        if (on == NULL)
            break;
        if (pcvariant_is_mutable(on->val)) {
            struct pcvar_rev_update_edge edge = {
                .parent        = node->elem,
                .obj_me        = on,
            };
            pcvar_break_edge_to_parent(on->val, &edge);
            pcvar_break_rue_downward(on->val);
        }
        pcvar_kv_it_next(&it);
    }
}

static bool
variant_set_constraint_grow_handler(
        purc_variant_t source,  // the source variant.
        void *ctxt,             // the context stored when registering the handler.
        size_t nr_args,         // the number of the relevant child variants.
        purc_variant_t *argv    // the array of all relevant child variants.
        )
{
    PC_ASSERT(source);
    PC_ASSERT(purc_variant_is_object(source));
    purc_variant_t set = (purc_variant_t)ctxt;
    PC_ASSERT(set);
    PC_ASSERT(purc_variant_is_set(set));
    PC_ASSERT(nr_args == 2);
    purc_variant_t k = (purc_variant_t)argv[0];
    PC_ASSERT(k);
    PC_ASSERT(purc_variant_is_string(k));
    purc_variant_t v = (purc_variant_t)argv[1];
    PC_ASSERT(v);

    return true;
}

static bool
variant_set_constraint_shrink_handler(
        purc_variant_t source,  // the source variant.
        void *ctxt,             // the context stored when registering the handler.
        size_t nr_args,         // the number of the relevant child variants.
        purc_variant_t *argv    // the array of all relevant child variants.
        )
{
    PC_ASSERT(source);
    PC_ASSERT(purc_variant_is_object(source));
    purc_variant_t set = (purc_variant_t)ctxt;
    PC_ASSERT(set);
    PC_ASSERT(purc_variant_is_set(set));
    PC_ASSERT(nr_args == 2);
    purc_variant_t k = (purc_variant_t)argv[0];
    PC_ASSERT(k);
    PC_ASSERT(purc_variant_is_string(k));
    purc_variant_t v = (purc_variant_t)argv[1];
    PC_ASSERT(v);

    return true;
}

struct element_rb_node {
    struct rb_node     **pnode;
    struct rb_node      *parent;
    struct rb_node      *entry;
};

static void
find_element_rb_node(struct element_rb_node *node,
        purc_variant_t set, purc_variant_t kvs)
{
    variant_set_t data = pcvar_set_get_data(set);
    struct rb_root *root = &data->elems;
    struct rb_node **pnode = &root->rb_node;
    struct rb_node *parent = NULL;
    struct rb_node *entry = NULL;
    while (*pnode) {
        struct set_node *on;
        on = container_of(*pnode, struct set_node, node);
        int diff;
        diff = variant_set_compare_by_set_keys(set, kvs, on->elem);

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

    return container_of(node.entry, struct set_node, node);
}

static bool
variant_set_constraint_change_handler(
        purc_variant_t source,  // the source variant.
        void *ctxt,             // the context stored when registering the handler.
        size_t nr_args,         // the number of the relevant child variants.
        purc_variant_t *argv    // the array of all relevant child variants.
        )
{
    PC_ASSERT(source);
    PC_ASSERT(purc_variant_is_object(source));
    purc_variant_t set = (purc_variant_t)ctxt;
    PC_ASSERT(set);
    PC_ASSERT(purc_variant_is_set(set));
    PC_ASSERT(nr_args == 4);
    purc_variant_t ko = (purc_variant_t)argv[0];
    PC_ASSERT(ko);
    PC_ASSERT(purc_variant_is_string(ko));
    purc_variant_t vo = (purc_variant_t)argv[1];
    PC_ASSERT(vo);
    purc_variant_t kn = (purc_variant_t)argv[2];
    PC_ASSERT(kn);
    PC_ASSERT(purc_variant_is_string(kn));
    purc_variant_t vn = (purc_variant_t)argv[3];
    PC_ASSERT(vn);
    PC_ASSERT(0 == pcvariant_equal(ko, kn));

    purc_variant_t tmp;
    tmp = purc_variant_container_clone(source);
    PC_ASSERT(tmp != PURC_VARIANT_INVALID);
    bool ok;
    ok = purc_variant_object_set(tmp, kn, vn);
    PC_ASSERT(ok);

    struct set_node *p;
    p = find_element(set, tmp);
    PURC_VARIANT_SAFE_CLEAR(tmp);
    if (p && p->elem != source)
        return false;

    return true;
}

static bool
variant_set_constraints_handler(
        purc_variant_t source,  // the source variant.
        pcvar_op_t op,          // the operation identifier.
        void *ctxt,             // the context stored when registering the handler.
        size_t nr_args,         // the number of the relevant child variants.
        purc_variant_t *argv    // the array of all relevant child variants.
        )
{
    switch (op) {
        case PCVAR_OPERATION_GROW:
            return variant_set_constraint_grow_handler(source, ctxt,
                    nr_args, argv);
        case PCVAR_OPERATION_SHRINK:
            return variant_set_constraint_shrink_handler(source, ctxt,
                    nr_args, argv);
        case PCVAR_OPERATION_CHANGE:
            return variant_set_constraint_change_handler(source, ctxt,
                    nr_args, argv);
        default:
            PC_ASSERT(0);
            return false;
    }
}

static bool
elem_node_setup_constraints(struct set_node *node)
{
    PC_ASSERT(node->set != PURC_VARIANT_INVALID);
    purc_variant_t set = node->set;
    variant_set_t data = pcvar_set_get_data(set);
    PC_ASSERT(data);

    purc_variant_t elem = node->elem;
    PC_ASSERT(elem != PURC_VARIANT_INVALID);
    PC_ASSERT(purc_variant_is_object(elem));

    struct pcvar_rev_update_edge edge = {
        .parent        = set,
        .set_me        = node,
    };
    int r = pcvar_build_edge_to_parent(node->elem, &edge);
    // FIXME: recoverable???
    PC_ASSERT(r == 0);

    struct kv_iterator it;
    it = pcvar_kv_it_first(node->set, node->elem);
    while (1) {
        struct obj_node *on = it.it.curr;
        if (on == NULL)
            break;
        if (pcvariant_is_mutable(on->val)) {
            struct pcvar_rev_update_edge edge = {
                .parent        = node->elem,
                .obj_me        = on,
            };
            int r;
            r = pcvar_build_edge_to_parent(on->val, &edge);
            if (r == 0) {
                r = pcvar_build_rue_downward(on->val);
            }
            // FIXME: recoverable???
            PC_ASSERT(r == 0);
        }
        pcvar_kv_it_next(&it);
    }

    node->constraints = purc_variant_register_pre_listener(node->elem,
            PCVAR_OPERATION_ALL, variant_set_constraints_handler, set);

    // FIXME: recoverable???
    PC_ASSERT(node->constraints);

    return true;
}

static void
refresh_arr(struct pcutils_arrlist *arr, size_t idx)
{
    if (idx == (size_t)-1)
        return;

    size_t count = pcutils_arrlist_length(arr);
    for (; idx < count; ++idx) {
        struct set_node *p;
        p = (struct set_node*)pcutils_arrlist_get_idx(arr, idx);
        p->idx = idx;
    }
}

static void
elem_node_remove(struct set_node *node)
{
    if (node->set == PURC_VARIANT_INVALID)
        return;

    purc_variant_t set = node->set;
    variant_set_t data = pcvar_set_get_data(set);
    if (!data)
        return;

    if (node->idx == (size_t)-1)
        return;

    struct pcutils_arrlist *al = data->arr;
    PC_ASSERT(al);

    pcutils_rbtree_erase(&node->node, &data->elems);

    int r;
    r = pcutils_arrlist_del_idx(al, node->idx, 1);
    PC_ASSERT(r == 0);
    refresh_arr(al, node->idx);
    node->idx = -1;
}

static void
elem_node_release(struct set_node *node)
{
    elem_node_revoke_constraints(node);
    elem_node_remove(node);

    PURC_VARIANT_SAFE_CLEAR(node->elem);
    node->set = PURC_VARIANT_INVALID;
}

static int
elem_node_replace(struct set_node *node,
        purc_variant_t val)
{
    PC_ASSERT(node->set != PURC_VARIANT_INVALID);
    PC_ASSERT(node->elem != PURC_VARIANT_INVALID);

    purc_variant_t set = node->set;
    variant_set_t data = pcvar_set_get_data(set);

    purc_variant_ref(val);

    elem_node_revoke_constraints(node);
    pcutils_rbtree_erase(&node->node, &data->elems);

    PURC_VARIANT_SAFE_CLEAR(node->elem);

    node->elem = val;
    node->set  = set;

    struct element_rb_node rbn;
    find_element_rb_node(&rbn, set, val);
    PC_ASSERT(rbn.entry == NULL);

    struct rb_node *entry = &node->node;

    pcutils_rbtree_link_node(entry, rbn.parent, rbn.pnode);
    pcutils_rbtree_insert_color(entry, &data->elems);

    if (!elem_node_setup_constraints(node))
        return -1;

    return 0;
}

static void
variant_set_release_elems(variant_set_t data)
{
    if (!data->arr)
        return;

    struct pcutils_arrlist *al = data->arr;
    size_t count = pcutils_arrlist_length(al);
    if (count > 0) {
        for (size_t i=count; i-->0; ) {
            void *p = pcutils_arrlist_get_idx(al, i);
            PC_ASSERT(p);
            struct set_node *node;
            node = (struct set_node*)p;

            elem_node_release(node);
            free(node);
        }
    }

    pcutils_arrlist_free(data->arr);
    data->arr = NULL;
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

static purc_variant_t
variant_set_create_kvs_n(variant_set_t set, purc_variant_t v1, va_list ap)
{
    PC_ASSERT(set->keynames);
    PC_ASSERT(v1 != PURC_VARIANT_INVALID);

    purc_variant_t kvs;
    kvs = purc_variant_make_object(0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
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

        if (pcvar_container_belongs_to_set(v)) {
            PC_ASSERT(0);
        }

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

    _new->set  = set;
    _new->elem = val;
    purc_variant_ref(val);

    return _new;
}

static int
set_remove(purc_variant_t set, variant_set_t data, struct set_node *node,
        bool check)
{
    UNUSED_PARAM(data);

    if (!shrink(set, node->elem, check)) {
        return -1;
    }

    elem_node_revoke_constraints(node);
    elem_node_remove(node);

    shrunk(set, node->elem, check);

    elem_node_release(node);
    free(node);

    return 0;
}

static int
insert(purc_variant_t set, variant_set_t data,
        purc_variant_t val,
        struct rb_node *parent, struct rb_node **pnode,
        bool check)
{
    if (!grow(set, val, check))
        return -1;

    struct set_node *node;
    node = variant_set_create_elem_node(set, val);
    if (!node)
        return -1;

    int r = pcutils_arrlist_add(data->arr, node);
    if (r) {
        elem_node_release(node);
        free(node);
        return -1;
    }

    size_t count = pcutils_arrlist_length(data->arr);
    node->idx = count - 1;

    struct rb_node *entry = &node->node;

    pcutils_rbtree_link_node(entry, parent, pnode);
    pcutils_rbtree_insert_color(entry, &data->elems);

    if (!elem_node_setup_constraints(node)) {
        bool check = false;
        r = set_remove(set, data, node, check);
        PC_ASSERT(r == 0);
        return -1;
    }

    grown(set, node->elem, check);

    return 0;
}

static purc_variant_t
variant_set_union(variant_set_t data, purc_variant_t old, purc_variant_t _new)
{
    UNUSED_PARAM(data);
    // FIXME: performance, performance, performance!!!
    purc_variant_t output;
    output = purc_variant_make_object(0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    if (output == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    purc_variant_t k, v;
    foreach_key_value_in_variant_object(_new, k, v) {
        PC_ASSERT(purc_variant_is_undefined(v) == false);
        bool ok;
        ok = purc_variant_object_set(output, k, v);
        if (!ok) {
            purc_variant_unref(output);
            return PURC_VARIANT_INVALID;
        }
    } end_foreach;

    foreach_key_value_in_variant_object(old, k, v) {
        PC_ASSERT(purc_variant_is_undefined(v) == false);
        bool silently = true;
        purc_variant_t t = purc_variant_object_get(output, k, silently);
        if (t != PURC_VARIANT_INVALID)
            continue;

        bool ok;
        ok = purc_variant_object_set(output, k, v);
        if (!ok) {
            purc_variant_unref(output);
            return PURC_VARIANT_INVALID;
        }
    } end_foreach;

    return output;
}

static bool
is_keyname(variant_set_t data, const char *s)
{
    for (size_t i=0; i<data->nr_keynames; ++i) {
        const char *sk = data->keynames[i];
        if (strcmp(sk, s) == 0)
            return true;
    }

    return false;
}

static purc_variant_t
prepare_variant(variant_set_t data, purc_variant_t val)
{
    purc_variant_t obj;
    obj = purc_variant_make_object(0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    if ( obj == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    int generic = data->keynames ? 0 : 1;
    size_t nr_cloned_kvs = 0;

    purc_variant_t k, v;
    foreach_key_value_in_variant_object(val, k, v) {
        purc_variant_t cloned = purc_variant_ref(v);

        if (generic) {
            PURC_VARIANT_SAFE_CLEAR(cloned);
            cloned = purc_variant_container_clone_recursively(v);
            if (cloned == PURC_VARIANT_INVALID) {
                purc_variant_unref(obj);
                return PURC_VARIANT_INVALID;
            }
        }
        else if (nr_cloned_kvs < data->nr_keynames) {
            const char *sk = purc_variant_get_string_const(k);
            if (is_keyname(data, sk)) {
                PURC_VARIANT_SAFE_CLEAR(cloned);
                cloned = purc_variant_container_clone_recursively(v);
                if (cloned == PURC_VARIANT_INVALID) {
                    purc_variant_unref(obj);
                    return PURC_VARIANT_INVALID;
                }
                ++nr_cloned_kvs;
            }
            else {
                PURC_VARIANT_SAFE_CLEAR(cloned);
                cloned = purc_variant_container_clone_recursively(v);
                if (cloned == PURC_VARIANT_INVALID) {
                    purc_variant_unref(obj);
                    return PURC_VARIANT_INVALID;
                }
            }
        }
        else {
            PURC_VARIANT_SAFE_CLEAR(cloned);
            cloned = purc_variant_container_clone_recursively(v);
            if (cloned == PURC_VARIANT_INVALID) {
                purc_variant_unref(obj);
                return PURC_VARIANT_INVALID;
            }
        }

        bool ok = purc_variant_object_set(obj, k, cloned);;
        purc_variant_unref(cloned);
        if (!ok) {
            purc_variant_unref(obj);
            return PURC_VARIANT_INVALID;
        }
    } end_foreach;

    return obj;
}

static int
insert_or_replace(purc_variant_t set,
        variant_set_t data, purc_variant_t val, bool overwrite,
        bool check)
{
    struct element_rb_node rbn;
    find_element_rb_node(&rbn, set, val);

    if (!rbn.entry) {
        purc_variant_t cloned;
        cloned = prepare_variant(data, val);
        if (cloned == PURC_VARIANT_INVALID)
            return -1;

        int r = insert(set, data, cloned, rbn.parent, rbn.pnode, check);
        purc_variant_unref(cloned);

        return r ? -1 : 0;
    }

    if (!overwrite) {
        purc_set_error(PURC_ERROR_NOT_SUPPORTED);
        return -1;
    }

    struct set_node *curr;
    curr = container_of(rbn.entry, struct set_node, node);

    PC_ASSERT(curr->set != PURC_VARIANT_INVALID);

    if (curr->elem == val)
        return 0;

    purc_variant_t cloned = prepare_variant(data, val);
    if (cloned == PURC_VARIANT_INVALID)
        return -1;

    purc_variant_t tmp = variant_set_union(data, curr->elem, val);
    PURC_VARIANT_SAFE_CLEAR(cloned);

    do {
        if (tmp == PURC_VARIANT_INVALID)
            break;

        if (!change(set, curr->elem, tmp, check))
            break;

        if (elem_node_replace(curr, tmp))
            break;

        changed(set, curr->elem, tmp, check);

        PURC_VARIANT_SAFE_CLEAR(tmp);

        return 0;
    } while (0);

    PURC_VARIANT_SAFE_CLEAR(tmp);

    return -1;
}

static int
variant_set_add_val(purc_variant_t set,
        variant_set_t data, purc_variant_t val, bool overwrite,
        bool check)
{
    if (!val) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    if (purc_variant_is_object(val) == false) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    if (pcvar_container_belongs_to_set(val)) {
        PC_ASSERT(0);
    }

    if (insert_or_replace(set, data, val, overwrite, check))
        return -1;

    return 0;
}

static int
variant_set_add_valsn(purc_variant_t set, variant_set_t data, bool overwrite,
    bool check, size_t sz, va_list ap)
{
    size_t i = 0;
    while (i<sz) {
        purc_variant_t v = va_arg(ap, purc_variant_t);
        if (!v) {
            pcinst_set_error(PURC_ERROR_INVALID_VALUE);
            break;
        }

        if (variant_set_add_val(set, data, v, overwrite, check)) {
            break;
        }

        ++i;
    }
    return i<sz ? -1 : 0;
}

static purc_variant_t
make_set_c(bool check, size_t sz, const char *unique_key,
    purc_variant_t value0, va_list ap)
{
    purc_variant_t set = pcv_set_new();
    if (set==PURC_VARIANT_INVALID) {
        return PURC_VARIANT_INVALID;
    }

    do {
        variant_set_t data = pcvar_set_get_data(set);
        if (variant_set_init(data, unique_key))
            break;

        if (sz>0) {
            purc_variant_t  v = value0;
            if (variant_set_add_val(set, data, v, true, check))
                break;

            int r = variant_set_add_valsn(set, data, true, check, sz-1, ap);
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
pv_make_set_by_ckey_n(bool check, size_t sz, const char* unique_key,
    purc_variant_t value0, va_list ap)
{
    purc_variant_t v = make_set_c(check, sz, unique_key, value0, ap);

    return v;
}

purc_variant_t
purc_variant_make_set_by_ckey(size_t sz, const char* unique_key,
    purc_variant_t value0, ...)
{
    PCVARIANT_CHECK_FAIL_RET((sz==0 && value0==NULL) || (sz>0 && value0),
        PURC_VARIANT_INVALID);

    bool check = true;
    purc_variant_t v;
    va_list ap;
    va_start(ap, value0);
    v = pv_make_set_by_ckey_n(check, sz, unique_key, value0, ap);
    va_end(ap);

    return v;
}

static purc_variant_t
pv_make_set_n(bool check, size_t sz, purc_variant_t unique_key,
    purc_variant_t value0, va_list ap)
{
    const char *uk = NULL;
    if (unique_key) {
        uk = purc_variant_get_string_const(unique_key);
        PC_ASSERT(uk);
    }

    purc_variant_t v = make_set_c(check, sz, uk, value0, ap);

    return v;
}

purc_variant_t
purc_variant_make_set(size_t sz, purc_variant_t unique_key,
    purc_variant_t value0, ...)
{
    PCVARIANT_CHECK_FAIL_RET((sz==0 && value0==NULL) ||
        (sz>0 && value0),
        PURC_VARIANT_INVALID);

    PCVARIANT_CHECK_FAIL_RET(!unique_key || unique_key->type==PVT(_STRING),
        PURC_VARIANT_INVALID);

    bool check = true;
    purc_variant_t v;
    va_list ap;
    va_start(ap, value0);
    v = pv_make_set_n(check, sz, unique_key, value0, ap);
    va_end(ap);

    return v;
}

bool
purc_variant_set_add(purc_variant_t set, purc_variant_t value, bool overwrite)
{
    PCVARIANT_CHECK_FAIL_RET(set && set->type==PVT(_SET) &&
        value && value->type==PVT(_OBJECT),
        PURC_VARIANT_INVALID);

    variant_set_t data = pcvar_set_get_data(set);
    PC_ASSERT(data);

    bool check = true;
    if (variant_set_add_val(set, data, value, overwrite, check))
        return false;

    size_t extra = variant_set_get_extra_size(data);
    pcvariant_stat_set_extra_size(set, extra);
    return true;
}

bool
purc_variant_set_remove(purc_variant_t set, purc_variant_t value,
        bool silently)
{
    PCVARIANT_CHECK_FAIL_RET(set && set->type==PVT(_SET) &&
            value && value->type==PVT(_OBJECT),
            PURC_VARIANT_INVALID);

    variant_set_t data = pcvar_set_get_data(set);
    PC_ASSERT(data);
    PC_ASSERT(data->nr_keynames);

    bool check = true;
    int r = 0;
    struct set_node *p;
    p = find_element(set, value);
    if (p)
        r = set_remove(set, data, p, check);

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

    return p ? p->elem : PURC_VARIANT_INVALID;
}

purc_variant_t
purc_variant_set_remove_member_by_key_values(purc_variant_t set,
        purc_variant_t v1, ...)
{
    PCVARIANT_CHECK_FAIL_RET(set && set->type==PVT(_SET) && v1,
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
        pcinst_set_error(PCVARIANT_ERROR_NOT_FOUND);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t v = p->elem;
    purc_variant_ref(v);

    bool check = true;
    int r = set_remove(set, data, p, check);
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

    variant_set_t data = pcvar_set_get_data(set);

    PC_ASSERT(data);
    size_t count = pcutils_arrlist_length(data->arr);
    *sz = count;

    return true;
}

purc_variant_t
purc_variant_set_get_by_index(purc_variant_t set, int idx)
{
    PC_ASSERT(set);

    variant_set_t data = pcvar_set_get_data(set);
    size_t count = pcutils_arrlist_length(data->arr);

    if (idx < 0 || (size_t)idx >= count)
        return PURC_VARIANT_INVALID;

    struct set_node *node;
    node = (struct set_node*)pcutils_arrlist_get_idx(data->arr, idx);
    PC_ASSERT(node);
    PC_ASSERT(node->idx == (size_t)idx);
    PC_ASSERT(node->elem != PURC_VARIANT_INVALID);

    return node->elem;
}

PCA_EXPORT purc_variant_t
purc_variant_set_remove_by_index(purc_variant_t set, int idx)
{
    PC_ASSERT(set);

    variant_set_t data = pcvar_set_get_data(set);
    size_t count = pcutils_arrlist_length(data->arr);

    if (idx < 0 || (size_t)idx >= count) {
        pcinst_set_error(PCVARIANT_ERROR_OUT_OF_BOUNDS);
        return PURC_VARIANT_INVALID;
    }

    struct set_node *node;
    node = (struct set_node*)pcutils_arrlist_get_idx(data->arr, idx);
    PC_ASSERT(node);
    PC_ASSERT(node->idx == (size_t)idx);

    purc_variant_t v = node->elem;
    purc_variant_ref(v);

    bool check = true;
    int r = set_remove(set, data, node, check);
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

    variant_set_t data = pcvar_set_get_data(set);
    size_t count = pcutils_arrlist_length(data->arr);

    if (idx < 0 || (size_t)idx >= count) {
        pcinst_set_error(PCVARIANT_ERROR_OUT_OF_BOUNDS);
        return false;
    }

    struct set_node *node;
    node = (struct set_node*)pcutils_arrlist_get_idx(data->arr, idx);
    if (node->elem == val)
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
    variant_set_t data = pcvar_set_get_data(it->set);
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
purc_variant_set_make_iterator_begin(purc_variant_t set)
{
    PCVARIANT_CHECK_FAIL_RET(set && set->type==PVT(_SET),
        NULL);

    variant_set_t data = pcvar_set_get_data(set);
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
purc_variant_set_make_iterator_end(purc_variant_t set)
{
    PCVARIANT_CHECK_FAIL_RET(set && set->type==PVT(_SET),
        NULL);

    variant_set_t data = pcvar_set_get_data(set);
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
purc_variant_set_release_iterator(struct purc_variant_set_iterator* it)
{
    if (!it)
        return;
    free(it);
}

bool
purc_variant_set_iterator_next(struct purc_variant_set_iterator* it)
{
    PCVARIANT_CHECK_FAIL_RET(it && it->set &&
        it->set->type==PVT(_SET) && it->curr,
        false);

    variant_set_t data = pcvar_set_get_data(it->set);
    PC_ASSERT(data);

    it->curr = it->next;
    iterator_refresh(it);

    return it->curr ? true : false;
}

bool
purc_variant_set_iterator_prev(struct purc_variant_set_iterator* it)
{
    PCVARIANT_CHECK_FAIL_RET(it && it->set &&
        it->set->type==PVT(_SET) && it->curr,
        false);

    variant_set_t data = pcvar_set_get_data(it->set);
    PC_ASSERT(data);

    it->curr = it->prev;
    iterator_refresh(it);

    return it->curr ? true : false;
}

purc_variant_t
purc_variant_set_iterator_get_value(struct purc_variant_set_iterator* it)
{
    PCVARIANT_CHECK_FAIL_RET(it && it->set &&
        it->set->type==PVT(_SET) && it->curr,
        PURC_VARIANT_INVALID);

    struct set_node *p;
    p = container_of(it->curr, struct set_node, node);
    return p->elem;
}

void
pcvariant_set_release(purc_variant_t value)
{
    variant_set_t data = pcvar_set_get_data(value);
    PC_ASSERT(data);

    variant_set_release(data);
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
    int (*cmp)(size_t nr_keynames,
            purc_variant_t l[], purc_variant_t r[], void *ud);
    void *ud;
    size_t               nr_keynames;
};

#if OS(HURD) || OS(LINUX)
static int cmp_variant(const void *l, const void *r, void *ud)
{
    purc_variant_t set = (purc_variant_t)ud;

    struct set_node *nl = *(struct set_node**)l;
    struct set_node *nr = *(struct set_node**)r;

    return variant_set_compare_by_set_keys(set, nl->elem, nr->elem);
}
#elif OS(DARWIN) || OS(FREEBSD) || OS(NETBSD) || OS(OPENBSD) || OS(WINDOWS)
static int cmp_variant(void *ud, const void *l, const void *r)
{
    purc_variant_t set = (purc_variant_t)ud;

    struct set_node *nl = *(struct set_node**)l;
    struct set_node *nr = *(struct set_node**)r;
    return variant_set_compare_by_set_keys(set, nl->elem, nr->elem);
}
#else
#error Unsupported operating system.
#endif

int pcvariant_set_sort(purc_variant_t value)
{
    if (!value || value->type != PURC_VARIANT_TYPE_SET)
        return -1;

    variant_set_t data = pcvar_set_get_data(value);
    struct pcutils_arrlist *al = data->arr;
    if (!al)
        return -1;
    void *arr = al->array;

#if OS(HURD) || OS(LINUX)
    qsort_r(arr, al->length, sizeof(struct set_node*), cmp_variant, value);
#elif OS(DARWIN) || OS(FREEBSD) || OS(NETBSD) || OS(OPENBSD)
    qsort_r(arr, al->length, sizeof(struct set_node*), value, cmp_variant);
#elif OS(WINDOWS)
    qsort_s(arr, al->length, sizeof(struct set_node*), cmp_variant, value);
#endif

    refresh_arr(al, 0);

    return 0;
}

purc_variant_t
pcvariant_set_find(purc_variant_t set, purc_variant_t value)
{
    PCVARIANT_CHECK_FAIL_RET(set && set->type==PVT(_SET) &&
            value && value->type==PVT(_OBJECT),
            PURC_VARIANT_INVALID);

    variant_set_t data = pcvar_set_get_data(set);
    PC_ASSERT(data);
    PC_ASSERT(data->nr_keynames);

    struct set_node *p;
    p = find_element(set, value);

    return p ? p->elem : PURC_VARIANT_INVALID;
}

int pcvariant_set_get_uniqkeys(purc_variant_t set, size_t *nr_keynames,
        const char ***keynames)
{
    PCVARIANT_CHECK_FAIL_RET(set && set->type==PVT(_SET) &&
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
        PC_ASSERT(pcvar_container_belongs_to_set(v));
        if (recursively) {
            val = pcvariant_container_clone(v, recursively);
            PC_ASSERT(pcvar_container_belongs_to_set(val) == false);
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

    pcvar_break_edge(set, &data->rev_update_chain, edge);
}

int
pcvar_set_build_edge_to_parent(purc_variant_t set,
        struct pcvar_rev_update_edge *edge)
{
    PC_ASSERT(purc_variant_is_set(set));
    variant_set_t data = pcvar_set_get_data(set);
    if (!data)
        return 0;

    return pcvar_build_edge(set, &data->rev_update_chain, edge);
}

static struct set_node*
next_node(struct set_iterator *it, struct set_node *curr)
{
    variant_set_t data = pcvar_set_get_data(it->set);
    PC_ASSERT(data);

    if (it->it_type == SET_IT_ARRAY) {
        struct pcutils_arrlist *arr = data->arr;
        PC_ASSERT(arr);
        size_t count = pcutils_arrlist_length(arr);
        size_t idx = curr->idx + 1;
        if (idx >= count)
            return NULL;

        void *p = pcutils_arrlist_get_idx(arr, idx);
        PC_ASSERT(p);
        return (struct set_node*)p;
    }

    if (it->it_type == SET_IT_RBTREE) {
        struct rb_node *p = pcutils_rbtree_next(&curr->node);
        if (!p)
            return NULL;
        return container_of(p, struct set_node, node);
    }
    PC_ASSERT(0);
}

static struct set_node*
prev_node(struct set_iterator *it, struct set_node *curr)
{
    variant_set_t data = pcvar_set_get_data(it->set);
    PC_ASSERT(data);

    if (it->it_type == SET_IT_ARRAY) {
        if (curr->idx == 0)
            return NULL;

        struct pcutils_arrlist *arr = data->arr;
        PC_ASSERT(arr);
        size_t count = pcutils_arrlist_length(arr);
        size_t idx = curr->idx - 1;
        if (idx >= count)
            return NULL;

        void *p = pcutils_arrlist_get_idx(arr, idx);
        PC_ASSERT(p);
        return (struct set_node*)p;
    }

    if (it->it_type == SET_IT_RBTREE) {
        struct rb_node *p = pcutils_rbtree_prev(&curr->node);
        if (!p)
            return NULL;
        return container_of(p, struct set_node, node);
    }
    PC_ASSERT(0);
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

    struct pcutils_arrlist *arr = data->arr;
    if (arr == NULL)
        return it;

    size_t count = pcutils_arrlist_length(arr);
    if (count == 0)
        return it;

    struct set_node *curr = NULL;

    if (it_type == SET_IT_ARRAY) {
        void *p = pcutils_arrlist_get_idx(arr, 0);
        PC_ASSERT(p);
        curr = (struct set_node*)p;

    }
    else if (it_type == SET_IT_RBTREE) {
        struct rb_node *p = pcutils_rbtree_first(root);
        PC_ASSERT(p);
        curr = container_of(p, struct set_node, node);
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

    struct pcutils_arrlist *arr = data->arr;
    if (arr == NULL)
        return it;

    size_t count = pcutils_arrlist_length(arr);
    if (count == 0)
        return it;

    struct set_node *curr = NULL;

    if (it_type == SET_IT_ARRAY) {
        void *p = pcutils_arrlist_get_idx(arr, count-1);
        PC_ASSERT(p);
        curr = (struct set_node*)p;

    }
    else if (it_type == SET_IT_RBTREE) {
        struct rb_node *p = pcutils_rbtree_last(root);
        PC_ASSERT(p);
        curr = container_of(p, struct set_node, node);
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
            if (strcmp(s, sk) == 0) {
                it.accu = 1;
                return it;
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
            if (strcmp(s, sk) == 0) {
                it->accu += 1;
                return;
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
                r = pcutils_string_append_chunk(&str, " ");
                if (r) {
                    purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
                    break;
                }
            }
            r = pcutils_string_append_chunk(&str, data->keynames[i]);
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

void
pcvar_adjust_set_by_descendant(purc_variant_t val)
{
    struct pcvar_rev_update_edge *top;
    top = pcvar_container_get_top_edge(val);
    PC_ASSERT(top);

    purc_variant_t set = top->parent;
    PC_ASSERT(set != PURC_VARIANT_INVALID);
    PC_ASSERT(purc_variant_is_set(set));

    variant_set_t data = pcvar_set_get_data(set);
    PC_ASSERT(data);

    struct set_node *node = top->set_me;
    pcutils_rbtree_erase(&node->node, &data->elems);

    struct element_rb_node rbn;
    find_element_rb_node(&rbn, set, node->elem);
    PC_ASSERT(rbn.entry == NULL);

    struct rb_node *entry = &node->node;

    pcutils_rbtree_link_node(entry, rbn.parent, rbn.pnode);
    pcutils_rbtree_insert_color(entry, &data->elems);
}

