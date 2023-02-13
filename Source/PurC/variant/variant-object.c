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
#include "private/errors.h"
#include "purc-errors.h"
#include "variant-internals.h"


#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define OBJ_EXTRA_SIZE(data) (sizeof(*data) + \
        (data->size) * sizeof(struct obj_node))

static inline bool
grow(purc_variant_t obj, purc_variant_t key, purc_variant_t val,
        bool check)
{
    if (!check)
        return true;

    purc_variant_t vals[] = { key, val };

    return pcvariant_on_pre_fired(obj, PCVAR_OPERATION_GROW,
            PCA_TABLESIZE(vals), vals);
}

static inline bool
shrink(purc_variant_t obj, purc_variant_t key, purc_variant_t val,
        bool check)
{
    if (!check)
        return true;

    purc_variant_t vals[] = { key, val };

    return pcvariant_on_pre_fired(obj, PCVAR_OPERATION_SHRINK,
            PCA_TABLESIZE(vals), vals);
}

static inline bool
change(purc_variant_t obj,
        purc_variant_t ko, purc_variant_t vo,
        purc_variant_t kn, purc_variant_t vn,
        bool check)
{
    if (!check)
        return true;

    purc_variant_t vals[] = { ko, vo, kn, vn };

    return pcvariant_on_pre_fired(obj, PCVAR_OPERATION_CHANGE,
            PCA_TABLESIZE(vals), vals);
}

static inline void
grown(purc_variant_t obj, purc_variant_t key, purc_variant_t val,
        bool check)
{
    if (!check)
        return;

    purc_variant_t vals[] = { key, val };

    pcvariant_on_post_fired(obj, PCVAR_OPERATION_GROW,
            PCA_TABLESIZE(vals), vals);
}

static inline void
shrunk(purc_variant_t obj, purc_variant_t key, purc_variant_t val,
        bool check)
{
    if (!check)
        return;

    purc_variant_t vals[] = { key, val };

    pcvariant_on_post_fired(obj, PCVAR_OPERATION_SHRINK,
            PCA_TABLESIZE(vals), vals);
}

static inline void
changed(purc_variant_t obj,
        purc_variant_t ko, purc_variant_t vo,
        purc_variant_t kn, purc_variant_t vn,
        bool check)
{
    if (!check)
        return;

    purc_variant_t vals[] = { ko, vo, kn, vn };

    pcvariant_on_post_fired(obj, PCVAR_OPERATION_CHANGE,
            PCA_TABLESIZE(vals), vals);
}

variant_obj_t
pcvar_obj_get_data(purc_variant_t obj)
{
    variant_obj_t data = (variant_obj_t)obj->sz_ptr[1];
    return data;
}

#if USE(UOMAP_FOR_OBJECT)
static void* copy_key_var(const void *key)
{
    return purc_variant_ref((purc_variant_t)key);
}

static void free_key_var(void *key)
{
    purc_variant_unref((purc_variant_t)key);
}

static int comp_key_var(const void *key1, const void *key2)
{
    purc_variant_t l = (purc_variant_t) key1;
    purc_variant_t r = (purc_variant_t) key2;
    PC_ASSERT((l->type == PURC_VARIANT_TYPE_STRING)
            && (r->type == PURC_VARIANT_TYPE_STRING));

    const char *k1;
    if ((l->flags & PCVRNT_FLAG_EXTRA_SIZE) ||
            (l->flags & PCVRNT_FLAG_STRING_STATIC)) {
        k1 = (const char *)l->sz_ptr[1];
    }
    else {
        k1 = (const char *)l->bytes;
    }

    const char *k2;
    if ((r->flags & PCVRNT_FLAG_EXTRA_SIZE) ||
            (r->flags & PCVRNT_FLAG_STRING_STATIC)) {
        k2 = (const char *)r->sz_ptr[1];
    }
    else {
        k2 = (const char *)r->bytes;
    }

    return strcmp(k1, k2);
}

static unsigned long hash_key_var(const void *key)
{
    purc_variant_t v = (purc_variant_t) key;
    PC_ASSERT(v->type == PURC_VARIANT_TYPE_STRING);

    const char *k;
    if ((v->flags & PCVRNT_FLAG_EXTRA_SIZE) ||
            (v->flags & PCVRNT_FLAG_STRING_STATIC)) {
        k = (const char *)v->sz_ptr[1];
    }
    else {
        k = (const char *)v->bytes;
    }
    return pchash_default_str_hash(k);
}
#endif

static purc_variant_t v_object_new_with_capacity(void)
{
    purc_variant_t var = pcvariant_get(PVT(_OBJECT));
    if (!var) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    var->type          = PVT(_OBJECT);
    var->flags         = PCVRNT_FLAG_EXTRA_SIZE;

    variant_obj_t data;
    data = (variant_obj_t)calloc(1, sizeof(*data));

    if (!data) {
        pcvariant_put(var);
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

#if USE(UOMAP_FOR_OBJECT)
    data->kvs = pcutils_uomap_create(copy_key_var,
                free_key_var, NULL, NULL, hash_key_var,
                comp_key_var, false);
#else
    data->kvs = RB_ROOT;
#endif

    var->sz_ptr[1]     = (uintptr_t)data;
    var->refc          = 1;

    size_t extra = OBJ_EXTRA_SIZE(data);
    pcvariant_stat_set_extra_size(var, extra);

    return var;
}

static void
break_rev_update_chain(purc_variant_t obj, struct obj_node *node)
{
    struct pcvar_rev_update_edge edge = {
        .parent        = obj,
        .obj_me        = node,
    };

    pcvar_break_edge_to_parent(node->val, &edge);
    pcvar_break_rue_downward(node->val);
}

static void
obj_node_release(purc_variant_t obj, struct obj_node *node)
{
    if (!node)
        return;

    break_rev_update_chain(obj, node);

#if !USE(UOMAP_FOR_OBJECT)
    variant_obj_t data = pcvar_obj_get_data(obj);
    PC_ASSERT(data);

    struct rb_root *root = &data->kvs;
    if (&node->node == root->rb_node || node->node.rb_parent) {
        --data->size;
        pcutils_rbtree_erase(&node->node, root);
        node->node.rb_parent = NULL;
    }
#endif

    PURC_VARIANT_SAFE_CLEAR(node->key);
    PURC_VARIANT_SAFE_CLEAR(node->val);
}

static void
obj_node_destroy(purc_variant_t obj, struct obj_node *node)
{
    if (!node)
        return;

    obj_node_release(obj, node);

    free(node);
}

static struct obj_node*
obj_node_create(purc_variant_t k, purc_variant_t v)
{
    if (k->type != PVT(_STRING)) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    struct obj_node *node;
    node = (struct obj_node*)calloc(1, sizeof(*node));
    if (!node) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    node->key = purc_variant_ref(k);
    node->val = purc_variant_ref(v);

    return node;
}

static int
build_rev_update_chain(purc_variant_t obj, struct obj_node *node)
{
    if (!pcvar_container_belongs_to_set(obj))
        return 0;

    int r;

    struct pcvar_rev_update_edge edge = {
        .parent        = obj,
        .obj_me        = node,
    };

    r = pcvar_build_edge_to_parent(node->val, &edge);
    if (r == 0) {
        r = pcvar_build_rue_downward(node->val);
    }

    return r ? -1 : 0;
}

static int
check_shrink(purc_variant_t obj, struct obj_node *node)
{
    if (!pcvar_container_belongs_to_set(obj))
        return 0;

    purc_variant_t _new = purc_variant_make_object(0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    if (_new == PURC_VARIANT_INVALID)
        return -1;

    int r = 0;
    do {
        bool found = false;
        purc_variant_t kk, vv;
        foreach_key_value_in_variant_object(obj, kk, vv) {
            if (kk == node->key) {
                PC_ASSERT(!found);
                found = true;
                continue;
            }
            r = pcvar_obj_set(_new, kk, vv);
            if (r)
                break;
        } end_foreach;

        if (r)
            break;

        if (!found)
            break;

        r = pcvar_reverse_check(obj, _new);
        if (r)
            break;

        PURC_VARIANT_SAFE_CLEAR(_new);

        return 0;
    } while (0);

    PURC_VARIANT_SAFE_CLEAR(_new);
    return -1;
}

static int
v_object_remove(purc_variant_t obj, purc_variant_t key, bool silently,
        bool check)
{
    variant_obj_t data = pcvar_obj_get_data(obj);
#if USE(UOMAP_FOR_OBJECT)
    pcutils_uomap_entry *entry = pcutils_uomap_find(data->kvs, key);
    if (!entry) {
        if (silently) {
            return 0;
        }

        pcinst_set_error(PCVRNT_ERROR_NO_SUCH_KEY);
        return -1;
    }

    struct obj_node *node = (struct obj_node *) entry->val;
    purc_variant_t k = node->key;
    purc_variant_t v = node->val;

    do {
        if (check) {
            if (!shrink(obj, k, v, check))
                break;

            if (check_shrink(obj, node))
                break;

            break_rev_update_chain(obj, node);
        }

        --data->size;
        pcutils_uomap_erase_entry_nolock(data->kvs, entry);

        if (check) {
            pcvar_adjust_set_by_descendant(obj);

            shrunk(obj, k, v, check);
        }

        obj_node_destroy(obj, node);

        return 0;
    } while (0);

    return -1;

#else
    struct rb_root *root = &data->kvs;
    struct rb_node **pnode = &root->rb_node;
    struct rb_node *parent = NULL;
    struct rb_node *entry = NULL;
    const char *s_key = purc_variant_get_string_const(key);
    while (*pnode) {
        struct obj_node *node;
        node = container_of(*pnode, struct obj_node, node);
        const char *sk = purc_variant_get_string_const(node->key);
        int ret = strcmp(s_key, sk);

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
        if (silently)
            return 0;

        pcinst_set_error(PCVRNT_ERROR_NO_SUCH_KEY);
        return -1;
    }

    struct obj_node *node;
    node = container_of(entry, struct obj_node, node);
    purc_variant_t k = node->key;
    purc_variant_t v = node->val;

    do {
        if (check) {
            if (!shrink(obj, k, v, check))
                break;

            if (check_shrink(obj, node))
                break;

            break_rev_update_chain(obj, node);
        }

        --data->size;
        PC_ASSERT(entry == root->rb_node || entry->rb_parent);
        pcutils_rbtree_erase(entry, root);
        entry->rb_parent = NULL;

        if (check) {
            pcvar_adjust_set_by_descendant(obj);

            shrunk(obj, k, v, check);
        }

        obj_node_destroy(obj, node);

        return 0;
    } while (0);

    return -1;
#endif
}

static int
check_grow(purc_variant_t obj, purc_variant_t k, purc_variant_t v)
{
    if (!pcvar_container_belongs_to_set(obj))
        return 0;

    purc_variant_t _new = purc_variant_make_object(0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    if (_new == PURC_VARIANT_INVALID)
        return -1;

    int r = 0;
    do {
        purc_variant_t kk, vv;
        foreach_key_value_in_variant_object(obj, kk, vv) {
            r = pcvar_obj_set(_new, kk, vv);
            if (r)
                break;
        } end_foreach;

        if (r)
            break;

        r = pcvar_obj_set(_new, k, v);
        if (r)
            break;

        int r = pcvar_reverse_check(obj, _new);
        if (r)
            break;

        PURC_VARIANT_SAFE_CLEAR(_new);

        return 0;
    } while (0);

    PURC_VARIANT_SAFE_CLEAR(_new);
    return -1;
}

static int
check_change(purc_variant_t obj, struct obj_node *node,
        purc_variant_t k, purc_variant_t v)
{
    if (!pcvar_container_belongs_to_set(obj))
        return 0;

    purc_variant_t _new = purc_variant_make_object(0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    if (_new == PURC_VARIANT_INVALID)
        return -1;

    int r = 0;
    do {
        bool found = false;
        purc_variant_t kk, vv;
        foreach_key_value_in_variant_object(obj, kk, vv) {
            if (node->key == kk) {
                PC_ASSERT(!found);
                found = true;
                r = pcvar_obj_set(_new, k, v);
            }
            else {
                r = pcvar_obj_set(_new, kk, vv);
            }
            if (r)
                break;
        } end_foreach;

        if (r)
            break;

        if (!found)
            break;

        int r = pcvar_reverse_check(obj, _new);
        if (r)
            break;

        PURC_VARIANT_SAFE_CLEAR(_new);

        return 0;
    } while (0);

    PURC_VARIANT_SAFE_CLEAR(_new);
    return -1;
}

static int
v_object_set(purc_variant_t obj, purc_variant_t key, purc_variant_t val,
        bool check)
{
    if (!key || !val) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    if (purc_variant_is_undefined(val)) {
        bool silently = true;
        v_object_remove(obj, key, silently, check);
        return 0;
    }

    if (key->type != PVT(_STRING)) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    variant_obj_t data = pcvar_obj_get_data(obj);
    PC_ASSERT(data);

#if USE(UOMAP_FOR_OBJECT)
    pcutils_uomap_entry *entry = pcutils_uomap_find(data->kvs, key);

    if (!entry) { //new the entry
        struct obj_node *node = obj_node_create(key, val);
        if (!node) {
            return -1;
        }

        do {
            if (check) {
                if (!grow(obj, key, val, check))
                    break;

                if (check_grow(obj, key, val))
                    break;
            }


            ++data->size;
            pcutils_uomap_insert(data->kvs, key, node);

            if (check) {
                if (build_rev_update_chain(obj, node))
                    break;

                pcvar_adjust_set_by_descendant(obj);

                grown(obj, key, val, check);
            }

            size_t extra = OBJ_EXTRA_SIZE(data);
            pcvariant_stat_set_extra_size(obj, extra);

            return 0;
        } while (0);

        obj_node_destroy(obj, node);

        return -1;
    }

    struct obj_node *node = (struct obj_node *) entry->val;
    if (node->val == val) {
        // NOTE: keep refc intact
        return 0;
    }

    do {
        purc_variant_t ko = node->key;
        purc_variant_t vo = node->val;

        if (check) {
            if (!change(obj, ko, vo, key, val, check))
                break;

            if (check_change(obj, node, key, val))
                break;

            node->key = key;
            node->val = val;
            if (build_rev_update_chain(obj, node)) {
                break_rev_update_chain(obj, node);
                node->key = ko;
                node->val = vo;
                break;
            }

            node->key = ko;
            node->val = vo;
            break_rev_update_chain(obj, node);
        }

        node->key = purc_variant_ref(key);
        node->val = purc_variant_ref(val);

        if (check) {
            pcvar_adjust_set_by_descendant(obj);

            changed(obj, ko, vo, key, val, check);
        }

        purc_variant_unref(ko);
        purc_variant_unref(vo);

        size_t extra = OBJ_EXTRA_SIZE(data);
        pcvariant_stat_set_extra_size(obj, extra);

        return 0;
    } while (0);

    return -1;
#else

    struct rb_root *root = &data->kvs;
    struct rb_node **pnode = &root->rb_node;
    struct rb_node *parent = NULL;
    struct rb_node *entry = NULL;
    const char *sk = purc_variant_get_string_const(key);
    while (*pnode) {
        struct obj_node *node;
        node = container_of(*pnode, struct obj_node, node);
        const char *sko = purc_variant_get_string_const(node->key);
        int ret = strcmp(sk, sko);

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

    if (!entry) { //new the entry
        struct obj_node *node = obj_node_create(key, val);
        if (!node)
            return -1;

        do {
            if (check) {
                if (!grow(obj, key, val, check))
                    break;

                if (check_grow(obj, key, val))
                    break;
            }

            entry = &node->node;

            pcutils_rbtree_link_node(entry, parent, pnode);
            pcutils_rbtree_insert_color(entry, root);

            ++data->size;

            if (check) {
                if (build_rev_update_chain(obj, node))
                    break;

                pcvar_adjust_set_by_descendant(obj);

                grown(obj, key, val, check);
            }

            size_t extra = OBJ_EXTRA_SIZE(data);
            pcvariant_stat_set_extra_size(obj, extra);

            return 0;
        } while (0);

        obj_node_destroy(obj, node);

        return -1;
    }

    struct obj_node *node;
    node = container_of(entry, struct obj_node, node);
    if (node->val == val) {
        // NOTE: keep refc intact
        return 0;
    }

    do {
        purc_variant_t ko = node->key;
        purc_variant_t vo = node->val;

        if (check) {
            if (!change(obj, ko, vo, key, val, check))
                break;

            if (check_change(obj, node, key, val))
                break;

            node->key = key;
            node->val = val;
            if (build_rev_update_chain(obj, node)) {
                break_rev_update_chain(obj, node);
                node->key = ko;
                node->val = vo;
                break;
            }

            node->key = ko;
            node->val = vo;
            break_rev_update_chain(obj, node);
        }

        node->key = purc_variant_ref(key);
        node->val = purc_variant_ref(val);

        if (check) {
            pcvar_adjust_set_by_descendant(obj);

            changed(obj, ko, vo, key, val, check);
        }

        purc_variant_unref(ko);
        purc_variant_unref(vo);

        size_t extra = OBJ_EXTRA_SIZE(data);
        pcvariant_stat_set_extra_size(obj, extra);

        return 0;
    } while (0);

    return -1;
#endif
}

purc_variant_t
pcvar_make_obj(void)
{
    return v_object_new_with_capacity();
}

int
pcvar_obj_set(purc_variant_t obj, purc_variant_t key, purc_variant_t val)
{
    bool check = false;
    return v_object_set(obj, key, val, check);
}

static int
v_object_set_kvs_n(purc_variant_t obj, bool check, size_t nr_kv_pairs,
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

        int r = v_object_set(obj, k, v, check);
        if (is_c)
            purc_variant_unref(k);

        if (r)
            break;
        ++i;
    }
    return i<nr_kv_pairs ? -1 : 0;
}

static purc_variant_t
pv_make_object_by_static_ckey_n (bool check, size_t nr_kv_pairs,
    const char* key0, purc_variant_t value0, va_list ap)
{
    PCVRNT_CHECK_FAIL_RET((nr_kv_pairs==0 && key0==NULL && value0==NULL) ||
                         (nr_kv_pairs>0 && key0 && value0),
        PURC_VARIANT_INVALID);

    purc_variant_t obj = v_object_new_with_capacity();
    if (!obj)
        return PURC_VARIANT_INVALID;

    variant_obj_t data = pcvar_obj_get_data(obj);
    PC_ASSERT(data);

    do {
        int r;
        if (nr_kv_pairs > 0) {
            purc_variant_t k = purc_variant_make_string(key0, true);
            purc_variant_t v = value0;
            r = v_object_set(obj, k, v, check);
            purc_variant_unref(k);
            if (r)
                break;
        }

        if (nr_kv_pairs > 1) {
            r = v_object_set_kvs_n(obj, check, nr_kv_pairs-1, 1, ap);
            if (r)
                break;
        }

        size_t extra = OBJ_EXTRA_SIZE(data);
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
    bool check = true;
    purc_variant_t v;
    va_list ap;
    va_start(ap, value0);
    v = pv_make_object_by_static_ckey_n(check, nr_kv_pairs, key0, value0, ap);
    va_end(ap);

    return v;
}

static purc_variant_t
pv_make_object_n(bool check, size_t nr_kv_pairs,
    purc_variant_t key0, purc_variant_t value0, va_list ap)
{
    PCVRNT_CHECK_FAIL_RET((nr_kv_pairs==0 && key0==NULL && value0==NULL) ||
                         (nr_kv_pairs>0 && key0 && value0),
        PURC_VARIANT_INVALID);

    purc_variant_t obj = v_object_new_with_capacity();
    if (!obj)
        return PURC_VARIANT_INVALID;

    variant_obj_t data = pcvar_obj_get_data(obj);
    PC_ASSERT(data);

    do {
        if (nr_kv_pairs > 0) {
            purc_variant_t v = value0;
            if (v_object_set(obj, key0, v, check))
                break;
        }

        if (nr_kv_pairs > 1) {
            int r = v_object_set_kvs_n(obj, check, nr_kv_pairs-1, 0, ap);
            if (r)
                break;
        }

        variant_obj_t data = pcvar_obj_get_data(obj);
        size_t extra = OBJ_EXTRA_SIZE(data);
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
    bool check = true;
    purc_variant_t v;
    va_list ap;
    va_start(ap, value0);
    v = pv_make_object_n(check, nr_kv_pairs, key0, value0, ap);
    va_end(ap);

    return v;
}

#if USE(UOMAP_FOR_OBJECT)
static int uomap_release_node(void *key, void *val, void *ud)
{
    UNUSED_PARAM(key);
    purc_variant_t obj = (purc_variant_t)ud;
    struct obj_node *node = (struct obj_node *)val;
    obj_node_destroy(obj, node);
    return 0;
}
#endif

void pcvariant_object_release (purc_variant_t value)
{
    variant_obj_t data = pcvar_obj_get_data(value);


#if USE(UOMAP_FOR_OBJECT)
    if (data->kvs) {
        pcutils_uomap_traverse(data->kvs, value, uomap_release_node);
        pcutils_uomap_destroy(data->kvs);
        data->kvs = NULL;
    }
#else
    struct rb_root *root = &data->kvs;

    struct rb_node *p, *n;
    pcutils_rbtree_for_each_safe(pcutils_rbtree_first(root), p, n) {
        struct obj_node *node;
        node = container_of(p, struct obj_node, node);

        obj_node_destroy(value, node);
    }
#endif

    if (data->rev_update_chain) {
        pcvar_destroy_rev_update_chain(data->rev_update_chain);
        data->rev_update_chain = NULL;
    }

    free(data);

    value->sz_ptr[1] = (uintptr_t)NULL; // say no to double free

    pcvariant_stat_set_extra_size(value, 0);
}

/* VWNOTE: unnecessary
int pcvariant_object_compare (purc_variant_t lv, purc_variant_t rv)
{
    // only called via purc_variant_compare
    struct pchash_table *lht = pcvar_obj_get_data(lv);
    struct pchash_table *rht = pcvar_obj_get_data(rv);

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

purc_variant_t
purc_variant_object_get(purc_variant_t obj, purc_variant_t key)
{
    PCVRNT_CHECK_FAIL_RET((obj && obj->type==PVT(_OBJECT) &&
        obj->sz_ptr[1] && key),
        PURC_VARIANT_INVALID);

    variant_obj_t data = pcvar_obj_get_data(obj);
#if USE(UOMAP_FOR_OBJECT)
    pcutils_uomap_entry *entry = pcutils_uomap_find(data->kvs, key);
    if (entry) {
        struct obj_node *node = (struct obj_node *) entry->val;
        return (purc_variant_t) node->val;
    }

    pcinst_set_error(PCVRNT_ERROR_NO_SUCH_KEY);
    return PURC_VARIANT_INVALID;
#else
    struct rb_root *root = &data->kvs;

    struct rb_node **pnode = &root->rb_node;
    struct rb_node *parent = NULL;
    struct rb_node *entry = NULL;
    const char *s_key = purc_variant_get_string_const(key);
    while (*pnode) {
        struct obj_node *node;
        node = container_of(*pnode, struct obj_node, node);
        const char *sk = purc_variant_get_string_const(node->key);

        int ret = strcmp(s_key, sk);

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
        pcinst_set_error(PCVRNT_ERROR_NO_SUCH_KEY);

        return PURC_VARIANT_INVALID;
    }

    struct obj_node *node;
    node = container_of(entry, struct obj_node, node);
    return node->val;
#endif
}

bool purc_variant_object_set (purc_variant_t obj,
    purc_variant_t key, purc_variant_t value)
{
    PCVRNT_CHECK_FAIL_RET(obj && obj->type==PVT(_OBJECT) &&
        obj->sz_ptr[1] && key && value,
        false);

    bool check = true;
    int r = v_object_set(obj, key, value, check);

    return r ? false : true;
}

bool
purc_variant_object_remove(purc_variant_t obj, purc_variant_t key,
        bool silently)
{
    PCVRNT_CHECK_FAIL_RET(obj && obj->type==PVT(_OBJECT) &&
        obj->sz_ptr[1] && key,
        false);

    bool check = true;
    if (v_object_remove(obj, key, silently, check))
        return false;

    return true;
}

bool purc_variant_object_size (purc_variant_t obj, size_t *sz)
{
    PC_ASSERT(obj && sz);

    PCVRNT_CHECK_FAIL_RET(obj->type == PVT(_OBJECT) && obj->sz_ptr[1],
        false);

    variant_obj_t data = pcvar_obj_get_data(obj);
    *sz = (size_t)data->size;

    return true;
}

struct pcvrnt_object_iterator {
    struct obj_iterator it;
};

struct pcvrnt_object_iterator*
pcvrnt_object_iterator_create_begin (purc_variant_t object)
{
    PCVRNT_CHECK_FAIL_RET((object && object->type==PVT(_OBJECT) &&
        object->sz_ptr[1]),
        NULL);

    variant_obj_t data = pcvar_obj_get_data(object);
    if (data->size==0) {
        pcinst_set_error(PCVRNT_ERROR_NO_SUCH_KEY);
        return NULL;
    }

    struct pcvrnt_object_iterator *it;
    it = (struct pcvrnt_object_iterator*)malloc(sizeof(*it));
    if (!it) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    it->it = pcvar_obj_it_first(object);

    return it;
}

struct pcvrnt_object_iterator*
pcvrnt_object_iterator_create_end (purc_variant_t object)
{
    PCVRNT_CHECK_FAIL_RET((object && object->type==PVT(_OBJECT) &&
        object->sz_ptr[1]),
        NULL);

    variant_obj_t data = pcvar_obj_get_data(object);
    if (data->size==0) {
        pcinst_set_error(PCVRNT_ERROR_NO_SUCH_KEY);
        return NULL;
    }

    struct pcvrnt_object_iterator *it;
    it = (struct pcvrnt_object_iterator*)malloc(sizeof(*it));
    if (!it) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    it->it = pcvar_obj_it_last(object);

    return it;
}

void
pcvrnt_object_iterator_release (struct pcvrnt_object_iterator* it)
{
    if (!it)
        return;

#if USE(UOMAP_FOR_OBJECT)
    it->it.obj  = PURC_VARIANT_INVALID;
    it->it.uomap_it.map = NULL;
    it->it.uomap_it.curr = NULL;
#else
    it->it.obj  = PURC_VARIANT_INVALID;
    it->it.curr = NULL;
    it->it.next = NULL;
    it->it.prev = NULL;
#endif

    free(it);
}

bool
pcvrnt_object_iterator_next (struct pcvrnt_object_iterator* it)
{
    PC_ASSERT(it);

    pcvar_obj_it_next(&it->it);

    return pcvar_obj_it_is_valid(&it->it);
}

bool
pcvrnt_object_iterator_prev (struct pcvrnt_object_iterator* it)
{
    PC_ASSERT(it);

    pcvar_obj_it_prev(&it->it);

    return pcvar_obj_it_is_valid(&it->it);
}

purc_variant_t
pcvrnt_object_iterator_get_key (struct pcvrnt_object_iterator* it)
{
    PC_ASSERT(it);

    return pcvar_obj_it_get_key(&it->it);
}

purc_variant_t
pcvrnt_object_iterator_get_value(struct pcvrnt_object_iterator* it)
{
    PC_ASSERT(it);

    return pcvar_obj_it_get_value(&it->it);
}

purc_variant_t
pcvariant_object_clone(purc_variant_t obj, bool recursively)
{
    purc_variant_t var;
    var = purc_variant_make_object(0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    if (var == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    purc_variant_t k,v;
    foreach_key_value_in_variant_object(obj, k, v) {
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
        ok = purc_variant_object_set(var, k, val);
        purc_variant_unref(val);
        if (!ok) {
            purc_variant_unref(var);
            return PURC_VARIANT_INVALID;
        }
    } end_foreach;

    PC_ASSERT(var != obj);
    return var;
}

#if USE(UOMAP_FOR_OBJECT)
static int uomap_break_rue_downward(void *key, void *val, void *ud)
{
    UNUSED_PARAM(key);
    purc_variant_t obj = (purc_variant_t)ud;
    struct obj_node *node = (struct obj_node *)val;
    struct pcvar_rev_update_edge edge = {
        .parent         = obj,
        .obj_me         = node,
    };
    pcvar_break_edge_to_parent(node->val, &edge);
    pcvar_break_rue_downward(node->val);
    return 0;
}
#endif

void
pcvar_object_break_rue_downward(purc_variant_t obj)
{
    PC_ASSERT(purc_variant_is_object(obj));

    variant_obj_t data = (variant_obj_t)obj->sz_ptr[1];
    if (!data) {
        return;
    }

#if USE(UOMAP_FOR_OBJECT)
    pcutils_uomap_traverse(data->kvs, obj, uomap_break_rue_downward);
#else
    struct rb_root *root = &data->kvs;
    struct rb_node *p = pcutils_rbtree_first(root);
    for (; p; p = pcutils_rbtree_next(p)) {
        struct obj_node *node;
        node = container_of(p, struct obj_node, node);
        struct pcvar_rev_update_edge edge = {
            .parent         = obj,
            .obj_me         = node,
        };
        pcvar_break_edge_to_parent(node->val, &edge);
        pcvar_break_rue_downward(node->val);
    }
#endif
}

void
pcvar_object_break_edge_to_parent(purc_variant_t obj,
        struct pcvar_rev_update_edge *edge)
{
    PC_ASSERT(purc_variant_is_object(obj));
    variant_obj_t data = (variant_obj_t)obj->sz_ptr[1];
    if (!data)
        return;

    if (!data->rev_update_chain)
        return;

    pcutils_map_erase(data->rev_update_chain, edge->obj_me);
}

#if USE(UOMAP_FOR_OBJECT)
static int uomap_build_rue_downward(void *key, void *val, void *ud)
{
    UNUSED_PARAM(key);
    purc_variant_t obj = (purc_variant_t)ud;
    struct obj_node *node = (struct obj_node *)val;

    struct pcvar_rev_update_edge edge = {
        .parent         = obj,
        .obj_me         = node,
    };

    int r = pcvar_build_edge_to_parent(node->val, &edge);
    if (r) {
        return -1;
    }

    r = pcvar_build_rue_downward(node->val);
    if (r) {
        return -1;
    }

    return 0;
}
#endif

int
pcvar_object_build_rue_downward(purc_variant_t obj)
{
    PC_ASSERT(purc_variant_is_object(obj));
    variant_obj_t data = (variant_obj_t)obj->sz_ptr[1];
    if (!data)
        return 0;

#if USE(UOMAP_FOR_OBJECT)
    pcutils_uomap_traverse(data->kvs, obj, uomap_build_rue_downward);
#else
    struct rb_root *root = &data->kvs;
    struct rb_node *p = pcutils_rbtree_first(root);
    for (; p; p = pcutils_rbtree_next(p)) {
        struct obj_node *node;
        node = container_of(p, struct obj_node, node);
        struct pcvar_rev_update_edge edge = {
            .parent         = obj,
            .obj_me         = node,
        };
        int r = pcvar_build_edge_to_parent(node->val, &edge);
        if (r)
            return -1;
        r = pcvar_build_rue_downward(node->val);
        if (r)
            return -1;
    }
#endif

    return 0;
}

int
pcvar_object_build_edge_to_parent(purc_variant_t obj,
        struct pcvar_rev_update_edge *edge)
{
    PC_ASSERT(purc_variant_is_object(obj));
    variant_obj_t data = (variant_obj_t)obj->sz_ptr[1];
    if (!data)
        return 0;

    if (!data->rev_update_chain) {
        data->rev_update_chain = pcvar_create_rev_update_chain();
        if (!data->rev_update_chain)
            return -1;
    }

    pcutils_map_entry *entry;
    entry = pcutils_map_find(data->rev_update_chain, edge->obj_me);
    if (entry)
        return 0;

    int r;
    r = pcutils_map_insert(data->rev_update_chain,
            edge->obj_me, edge->parent);

    return r ? -1 : 0;
}

#if !USE(UOMAP_FOR_OBJECT)
static void
it_refresh(struct obj_iterator *it, struct rb_node *curr)
{
    struct rb_node *next  = NULL;
    struct rb_node *prev  = NULL;
    if (curr) {
        next  = pcutils_rbtree_next(curr);
        prev  = pcutils_rbtree_prev(curr);
    }

    if (curr) {
        it->curr = container_of(curr, struct obj_node, node);
    }
    else {
        it->curr = NULL;
    }

    if (next) {
        it->next = container_of(next, struct obj_node, node);
    }
    else {
        it->next = NULL;
    }

    if (prev) {
        it->prev = container_of(prev, struct obj_node, node);
    }
    else {
        it->prev = NULL;
    }
}
#endif

struct obj_iterator
pcvar_obj_it_first(purc_variant_t obj)
{
    struct obj_iterator it = {
        .obj         = obj,
    };

    if (obj == PURC_VARIANT_INVALID) {
        return it;
    }

    variant_obj_t data = pcvar_obj_get_data(obj);

#if USE(UOMAP_FOR_OBJECT)
    it.uomap_it = pcutils_uomap_it_begin_first(data->kvs);
#else
    if (data->size == 0) {
        return it;
    }

    struct rb_root *root = &data->kvs;

    struct rb_node *first = pcutils_rbtree_first(root);
    it_refresh(&it, first);
#endif

    return it;
}

struct obj_iterator
pcvar_obj_it_last(purc_variant_t obj)
{
    struct obj_iterator it = {
        .obj         = obj,
    };
    if (obj == PURC_VARIANT_INVALID) {
        return it;
    }

    variant_obj_t data = pcvar_obj_get_data(obj);
#if USE(UOMAP_FOR_OBJECT)
    it.uomap_it = pcutils_uomap_it_begin_last(data->kvs);
#else
    if (data->size==0)
        return it;

    struct rb_root *root = &data->kvs;

    struct rb_node *last = pcutils_rbtree_last(root);
    it_refresh(&it, last);
#endif

    return it;
}

void
pcvar_obj_it_next(struct obj_iterator *it)
{
#if USE(UOMAP_FOR_OBJECT)
    pcutils_uomap_it_next(&it->uomap_it);
#else
    if (it->curr == NULL)
        return;

    if (it->next) {
        struct rb_node *next = &it->next->node;
        it_refresh(it, next);
    }
    else {
        it->curr = NULL;
        it->next = NULL;
        it->prev = NULL;
    }
#endif
}

void
pcvar_obj_it_prev(struct obj_iterator *it)
{
#if USE(UOMAP_FOR_OBJECT)
    pcutils_uomap_it_prev(&it->uomap_it);
#else
    if (it->curr == NULL)
        return;

    if (it->prev) {
        struct rb_node *prev = &it->prev->node;
        it_refresh(it, prev);
    }
    else {
        it->curr = NULL;
        it->next = NULL;
        it->prev = NULL;
    }
#endif
}

bool
pcvar_obj_it_is_valid(struct obj_iterator *it)
{
#if USE(UOMAP_FOR_OBJECT)
    return it && it->uomap_it.curr;
#else
    return it && it->curr;
#endif
}

struct obj_node *
pcvar_obj_it_get_curr(struct obj_iterator *it)
{
#if USE(UOMAP_FOR_OBJECT)
    if (it && it->uomap_it.curr) {
        return (struct obj_node *) it->uomap_it.curr->val;
    }
    return NULL;
#else
    return it ? it->curr : NULL;
#endif
}

purc_variant_t
pcvar_obj_it_get_key(struct obj_iterator *it)
{
    struct obj_node *node = pcvar_obj_it_get_curr(it);
    return node ? node->key : PURC_VARIANT_INVALID;
}

purc_variant_t
pcvar_obj_it_get_value(struct obj_iterator *it)
{
    struct obj_node *node = pcvar_obj_it_get_curr(it);
    return node ? node->val : PURC_VARIANT_INVALID;
}

ssize_t
purc_variant_object_unite(purc_variant_t dst,
        purc_variant_t src, pcvrnt_cr_method_k cr_method)
{
    UNUSED_PARAM(dst);
    UNUSED_PARAM(src);
    UNUSED_PARAM(cr_method);
    ssize_t ret = -1;

    if (dst == PURC_VARIANT_INVALID || src == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    if (dst == src) {
        purc_set_error(PURC_ERROR_INVALID_OPERAND);
        goto out;
    }

    if (!purc_variant_is_object(dst) || !purc_variant_is_object(src)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }

    ssize_t sz = purc_variant_object_get_size(src);
    if (sz <= 0) {
        ret = 0;
        goto out;
    }

    ret = 0;
    purc_variant_t k, v;
    foreach_key_value_in_variant_object(src, k, v)
        purc_variant_t o = purc_variant_object_get(dst, k);
        if (!o) {
            /* clr PCVRNT_ERROR_NO_SUCH_KEY */
            purc_clr_error();
            if (!purc_variant_object_set(dst, k, v)) {
                ret = -1;
                goto out;
            }
            ret++;
        }
        else {
            switch (cr_method) {
            case PCVRNT_CR_METHOD_IGNORE:
                break;

            case PCVRNT_CR_METHOD_OVERWRITE:
                if (!purc_variant_object_set(dst, k, v)) {
                    ret = -1;
                    goto out;
                }
                ret++;
                break;

            case PCVRNT_CR_METHOD_COMPLAIN:
                ret = -1;
                purc_set_error(PURC_ERROR_DUPLICATED);
                break;

            default:
                ret = -1;
                purc_set_error(PURC_ERROR_NOT_ALLOWED);
                goto out;
            }
        }
    end_foreach;

out:
    return ret;
}


ssize_t
purc_variant_object_intersect(purc_variant_t dst, purc_variant_t src)
{
    ssize_t ret = -1;

    if (dst == PURC_VARIANT_INVALID || src == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    if (dst == src) {
        purc_set_error(PURC_ERROR_INVALID_OPERAND);
        goto out;
    }

    if (!purc_variant_is_object(dst) || !purc_variant_is_object(src)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }

    ssize_t sz = purc_variant_object_get_size(src);
    if (sz <= 0) {
        ret = 0;
        goto out;
    }

    purc_variant_t k, v;
    UNUSED_VARIABLE(v);
    foreach_in_variant_object_safe_x(dst, k, v)
        purc_variant_t o = purc_variant_object_get(src, k);
        if (!o) {
            /* clr PCVRNT_ERROR_NO_SUCH_KEY */
            purc_clr_error();
            if (!purc_variant_object_remove(dst, k, true)) {
                goto out;
            }
        }
    end_foreach;
    ret = purc_variant_object_get_size(dst);

out:
    return ret;
}

ssize_t
purc_variant_object_subtract(purc_variant_t dst, purc_variant_t src)
{
    ssize_t ret = -1;

    if (dst == PURC_VARIANT_INVALID || src == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    if (dst == src) {
        purc_set_error(PURC_ERROR_INVALID_OPERAND);
        goto out;
    }

    if (!purc_variant_is_object(dst) || !purc_variant_is_object(src)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }

    ssize_t sz = purc_variant_object_get_size(src);
    if (sz <= 0) {
        ret = 0;
        goto out;
    }

    purc_variant_t k, v;
    UNUSED_VARIABLE(v);
    foreach_in_variant_object_safe_x(dst, k, v)
        purc_variant_t o = purc_variant_object_get(src, k);
        purc_clr_error();
        if (o) {
            if (!purc_variant_object_remove(dst, k, true)) {
                goto out;
            }
        }
    end_foreach;
    ret = purc_variant_object_get_size(dst);

out:
    return ret;
}

ssize_t
purc_variant_object_xor(purc_variant_t dst, purc_variant_t src)
{
    ssize_t ret = -1;

    if (dst == PURC_VARIANT_INVALID || src == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    if (dst == src) {
        purc_set_error(PURC_ERROR_INVALID_OPERAND);
        goto out;
    }

    if (!purc_variant_is_object(dst) || !purc_variant_is_object(src)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }

    ssize_t sz = purc_variant_object_get_size(src);
    if (sz <= 0) {
        ret = 0;
        goto out;
    }

    purc_variant_t k, v;
    UNUSED_VARIABLE(v);
    foreach_in_variant_object_safe_x(src, k, v)
        purc_variant_t o = purc_variant_object_get(dst, k);
        purc_clr_error();
        if (o) {
            if (!purc_variant_object_remove(dst, k, true)) {
                goto out;
            }
        }
        else if (!purc_variant_object_set(dst, k, v)) {
            goto out;
        }
    end_foreach;
    ret = purc_variant_object_get_size(dst);

out:
    return ret;
}

ssize_t
purc_variant_object_overwrite(purc_variant_t dst, purc_variant_t src,
        pcvrnt_nr_method_k nr_method)
{
    ssize_t ret = -1;

    if (dst == PURC_VARIANT_INVALID || src == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    if (dst == src) {
        purc_set_error(PURC_ERROR_INVALID_OPERAND);
        goto out;
    }

    if (!purc_variant_is_object(dst) || !purc_variant_is_object(src)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }

    ssize_t sz = purc_variant_object_get_size(src);
    if (sz <= 0) {
        ret = 0;
        goto out;
    }

    purc_variant_t k, v;
    UNUSED_VARIABLE(v);
    foreach_in_variant_object_safe_x(src, k, v)
        purc_variant_t o = purc_variant_object_get(dst, k);
        purc_clr_error();
        if (o) {
            if (!purc_variant_object_set(dst, k, v)) {
                goto out;
            }
        }
        else if (nr_method == PCVRNT_NR_METHOD_COMPLAIN) {
            purc_set_error(PCVRNT_ERROR_NOT_FOUND);
            goto out;
        }
    end_foreach;

    ret = purc_variant_object_get_size(dst);
out:
    return ret;
}

