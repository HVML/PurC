/*
 * @file constraint.c
 * @author Xu Xiaohong
 * @date 2022/03/16
 * @brief The implementation for variant constraint
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

#include "purc-errors.h"
#include "private/debug.h"
#include "private/errors.h"
#include "variant-internals.h"

#include <stdlib.h>

static bool
wind_up_and_check(struct pcvar_rev_update_edge *edge, purc_variant_t _new)
{
    struct arr_node      *an;
    struct obj_node      *on;
    struct set_node      *sn;

    variant_arr_t         arr_data;
    variant_obj_t         obj_data;
    variant_set_t         set_data;

    purc_variant_t parent = edge->parent;
    purc_variant_t cloned;
    bool ok;

again:

    switch (parent->type) {
        case PURC_VARIANT_TYPE_ARRAY:
            an = edge->arr_me;
            cloned = purc_variant_make_array(0, PURC_VARIANT_INVALID);
            if (cloned == PURC_VARIANT_INVALID) {
                purc_variant_unref(_new);
                return false;
            }
            size_t nr;
            ok = purc_variant_array_size(parent, &nr);
            PC_ASSERT(ok);
            PC_ASSERT(nr > 0);
            PC_ASSERT(nr > an->node.idx);

            for (size_t i=0; i<nr; ++i) {
                purc_variant_t v;
                if (an->node.idx == i) {
                    v = purc_variant_ref(_new);
                    PURC_VARIANT_SAFE_CLEAR(_new);
                }
                else {
                    v = purc_variant_array_get(parent, i);
                    PC_ASSERT(v != PURC_VARIANT_INVALID);
                    v = purc_variant_container_clone_recursively(v);
                    if (v == PURC_VARIANT_INVALID) {
                        PURC_VARIANT_SAFE_CLEAR(_new);
                        purc_variant_unref(cloned);
                        return false;
                    }
                }
                ok = purc_variant_array_append(cloned, v);
                purc_variant_unref(v);
                if (!ok) {
                    purc_variant_unref(cloned);
                    return false;
                }
            }

            PC_ASSERT(_new == PURC_VARIANT_INVALID);
            _new = cloned;
            cloned = PURC_VARIANT_INVALID;
            arr_data = pcvar_arr_get_data(parent);
            edge = &arr_data->rev_update_chain;
            PC_ASSERT(edge);
            parent = edge->parent;
            PC_ASSERT(parent != PURC_VARIANT_INVALID);
            goto again;

        case PURC_VARIANT_TYPE_OBJECT:
            on = edge->obj_me;
            cloned = purc_variant_make_object(0,
                    PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
            if (cloned == PURC_VARIANT_INVALID) {
                purc_variant_unref(_new);
                return false;
            }
            obj_data = pcvar_obj_get_data(parent);
            struct rb_root *root = &obj_data->kvs;
            struct rb_node *p = pcutils_rbtree_first(root);
            for (; p; p = pcutils_rbtree_next(p)) {
                purc_variant_t v;
                struct obj_node *node;
                node = container_of(p, struct obj_node, node);
                if (node == on) {
                    v = purc_variant_ref(_new);
                    PURC_VARIANT_SAFE_CLEAR(_new);
                }
                else {
                    v = node->val;
                    PC_ASSERT(v != PURC_VARIANT_INVALID);
                    v = purc_variant_container_clone_recursively(v);
                    if (v == PURC_VARIANT_INVALID) {
                        PURC_VARIANT_SAFE_CLEAR(_new);
                        purc_variant_unref(cloned);
                        return false;
                    }
                }

                ok = purc_variant_object_set(cloned, node->key, v);
                purc_variant_unref(v);
                if (!ok) {
                    purc_variant_unref(cloned);
                    return false;
                }
            }

            PC_ASSERT(_new == PURC_VARIANT_INVALID);
            _new = cloned;
            cloned = PURC_VARIANT_INVALID;
            edge = &obj_data->rev_update_chain;
            PC_ASSERT(edge);
            parent = edge->parent;
            PC_ASSERT(parent != PURC_VARIANT_INVALID);
            goto again;

        case PURC_VARIANT_TYPE_SET:
            sn = edge->set_me;
            cloned = pcvar_set_clone_struct(parent);
            if (cloned == PURC_VARIANT_INVALID) {
                purc_variant_unref(_new);
                return false;
            }
            set_data = pcvar_set_get_data(parent);
            struct rb_node *first;
            first = pcutils_rbtree_first(&set_data->elems);
            PC_ASSERT(first);
            struct rb_node *pn;
            pcutils_rbtree_for_each(first, pn) {
                purc_variant_t v;
                struct set_node *node;
                node = container_of(pn, struct set_node, node);
                if (node == sn) {
                    v = purc_variant_ref(_new);
                    PURC_VARIANT_SAFE_CLEAR(_new);
                }
                else {
                    v = node->elem;
                    PC_ASSERT(v != PURC_VARIANT_INVALID);
                    v = purc_variant_container_clone_recursively(v);
                    if (v == PURC_VARIANT_INVALID) {
                        PURC_VARIANT_SAFE_CLEAR(_new);
                        purc_variant_unref(cloned);
                        return false;
                    }
                }

                bool overwrite = false;
                ok = purc_variant_set_add(cloned, v, overwrite);
                purc_variant_unref(v);
                if (!ok) {
                    purc_variant_unref(cloned);
                    return false;
                }
            }

            PC_ASSERT(_new == PURC_VARIANT_INVALID);
            purc_variant_unref(cloned);
            return true;

        default:
            PC_ASSERT(0);
    }
}

static bool
arr_rev_update_grow(
        bool pre,
        purc_variant_t arr,
        struct pcvar_rev_update_edge *edge,
        size_t nr_args,
        purc_variant_t *argv)
{
    variant_arr_t data = pcvar_arr_get_data(arr);
    PC_ASSERT(data);
    PC_ASSERT(&data->rev_update_chain == edge);

    PC_ASSERT(nr_args == 2);

    purc_variant_t set;
    set = pcvar_top_in_rev_update_chain(arr);
    PC_ASSERT(set != PURC_VARIANT_INVALID);
    PC_ASSERT(purc_variant_is_set(set));

    if (pre) {
        purc_variant_t cloned;
        cloned = purc_variant_container_clone_recursively(arr);
        if (cloned == PURC_VARIANT_INVALID)
            return false;

        bool ok;
        size_t nr;
        ok = purc_variant_array_size(cloned, &nr);
        if (!ok) {
            purc_variant_unref(cloned);
            return false;
        }

        int64_t i64;
        bool parse_str = false;
        ok = purc_variant_cast_to_longint(argv[0], &i64, parse_str);
        if (!ok) {
            purc_variant_unref(cloned);
            return false;
        }
        if (i64 < 0) {
            purc_variant_unref(cloned);
            return false;
        }
        if ((size_t)i64 >= nr) {
            ok = purc_variant_array_append(cloned, argv[1]);
        }
        else {
            ok = purc_variant_array_set(cloned, i64, argv[1]);
        }
        if (!ok) {
            purc_variant_unref(cloned);
            return false;
        }

        return wind_up_and_check(edge, cloned);
    }

    return true;
}

static bool
obj_rev_update_grow(
        bool pre,
        purc_variant_t obj,
        struct pcvar_rev_update_edge *edge,
        size_t nr_args,
        purc_variant_t *argv)
{
    variant_obj_t data = pcvar_obj_get_data(obj);
    PC_ASSERT(data);
    PC_ASSERT(&data->rev_update_chain == edge);

    PC_ASSERT(nr_args == 2);

    purc_variant_t set;
    set = pcvar_top_in_rev_update_chain(obj);
    PC_ASSERT(set != PURC_VARIANT_INVALID);
    PC_ASSERT(purc_variant_is_set(set));

    if (pre) {
        purc_variant_t cloned;
        cloned = purc_variant_container_clone_recursively(obj);
        if (cloned == PURC_VARIANT_INVALID)
            return false;

        bool ok = purc_variant_object_set(cloned, argv[0], argv[1]);
        if (!ok) {
            purc_variant_unref(cloned);
            return false;
        }

        return wind_up_and_check(edge, cloned);
    }

    return true;
}

static bool
rev_update_grow(
        bool pre,
        purc_variant_t src,
        struct pcvar_rev_update_edge *edge,
        size_t nr_args,
        purc_variant_t *argv)
{
    switch (src->type) {
        case PURC_VARIANT_TYPE_ARRAY:
            return arr_rev_update_grow(pre, src, edge, nr_args, argv);
        case PURC_VARIANT_TYPE_OBJECT:
            return obj_rev_update_grow(pre, src, edge, nr_args, argv);
        default:
            PC_DEBUGX("Not supported for `%s` variant",
                    pcvariant_get_typename(src->type));
            PC_ASSERT(0);
    }
}

static bool
arr_rev_update_shrink(
        bool pre,
        purc_variant_t arr,
        struct pcvar_rev_update_edge *edge,
        size_t nr_args,
        purc_variant_t *argv)
{
    variant_arr_t data = pcvar_arr_get_data(arr);
    PC_ASSERT(data);
    PC_ASSERT(&data->rev_update_chain == edge);

    PC_ASSERT(nr_args == 2);

    purc_variant_t set;
    set = pcvar_top_in_rev_update_chain(arr);
    PC_ASSERT(set != PURC_VARIANT_INVALID);
    PC_ASSERT(purc_variant_is_set(set));

    if (pre) {
        purc_variant_t cloned;
        cloned = purc_variant_container_clone_recursively(arr);
        if (cloned == PURC_VARIANT_INVALID)
            return false;

        bool ok;
        int64_t i64;
        bool parse_str = false;
        ok = purc_variant_cast_to_longint(argv[0], &i64, parse_str);
        if (!ok) {
            purc_variant_unref(cloned);
            return false;
        }
        ok = purc_variant_array_remove(cloned, i64);
        if (!ok) {
            purc_variant_unref(cloned);
            return false;
        }

        return wind_up_and_check(edge, cloned);
    }

    return true;
}

static bool
obj_rev_update_shrink(
        bool pre,
        purc_variant_t obj,
        struct pcvar_rev_update_edge *edge,
        size_t nr_args,
        purc_variant_t *argv)
{
    variant_obj_t data = pcvar_obj_get_data(obj);
    PC_ASSERT(data);
    PC_ASSERT(&data->rev_update_chain == edge);

    PC_ASSERT(nr_args == 2);

    purc_variant_t set;
    set = pcvar_top_in_rev_update_chain(obj);
    PC_ASSERT(set != PURC_VARIANT_INVALID);
    PC_ASSERT(purc_variant_is_set(set));

    if (pre) {
        purc_variant_t cloned;
        cloned = purc_variant_container_clone_recursively(obj);
        if (cloned == PURC_VARIANT_INVALID)
            return false;

        bool silently = true;
        bool ok = purc_variant_object_remove(cloned, argv[0], silently);
        if (!ok) {
            purc_variant_unref(cloned);
            return false;
        }

        return wind_up_and_check(edge, cloned);
    }

    return true;
}

static bool
rev_update_shrink(
        bool pre,
        purc_variant_t src,
        struct pcvar_rev_update_edge *edge,
        size_t nr_args,
        purc_variant_t *argv)
{
    switch (src->type) {
        case PURC_VARIANT_TYPE_ARRAY:
            return arr_rev_update_shrink(pre, src, edge, nr_args, argv);
        case PURC_VARIANT_TYPE_OBJECT:
            return obj_rev_update_shrink(pre, src, edge, nr_args, argv);
        default:
            PC_DEBUGX("Not supported for `%s` variant",
                    pcvariant_get_typename(src->type));
            PC_ASSERT(0);
    }
}

static bool
arr_rev_update_change(
        bool pre,
        purc_variant_t arr,
        struct pcvar_rev_update_edge *edge,
        size_t nr_args,
        purc_variant_t *argv)
{
    variant_arr_t data = pcvar_arr_get_data(arr);
    PC_ASSERT(data);
    PC_ASSERT(&data->rev_update_chain == edge);

    PC_ASSERT(nr_args == 3);

    purc_variant_t set;
    set = pcvar_top_in_rev_update_chain(arr);
    PC_ASSERT(set != PURC_VARIANT_INVALID);
    PC_ASSERT(purc_variant_is_set(set));

    if (pre) {
        purc_variant_t cloned;
        cloned = purc_variant_container_clone_recursively(arr);
        if (cloned == PURC_VARIANT_INVALID)
            return false;

        bool ok;
        int64_t i64;
        bool parse_str = false;
        ok = purc_variant_cast_to_longint(argv[0], &i64, parse_str);
        if (!ok) {
            purc_variant_unref(cloned);
            return false;
        }
        ok = purc_variant_array_set(cloned, i64, argv[2]);
        if (!ok) {
            purc_variant_unref(cloned);
            return false;
        }

        return wind_up_and_check(edge, cloned);
    }

    return true;
}

static bool
obj_rev_update_change(
        bool pre,
        purc_variant_t obj,
        struct pcvar_rev_update_edge *edge,
        size_t nr_args,
        purc_variant_t *argv)
{
    variant_obj_t data = pcvar_obj_get_data(obj);
    PC_ASSERT(data);
    PC_ASSERT(&data->rev_update_chain == edge);

    PC_ASSERT(nr_args == 4);

    purc_variant_t set;
    set = pcvar_top_in_rev_update_chain(obj);
    PC_ASSERT(set != PURC_VARIANT_INVALID);
    PC_ASSERT(purc_variant_is_set(set));

    if (pre) {
        purc_variant_t cloned;
        cloned = purc_variant_container_clone_recursively(obj);
        if (cloned == PURC_VARIANT_INVALID)
            return false;

        bool ok = purc_variant_object_set(cloned, argv[2], argv[3]);
        if (!ok) {
            purc_variant_unref(cloned);
            return false;
        }

        return wind_up_and_check(edge, cloned);
    }

    return true;
}


static bool
rev_update_change(
        bool pre,
        purc_variant_t src,
        struct pcvar_rev_update_edge *edge,
        size_t nr_args,
        purc_variant_t *argv)
{
    switch (src->type) {
        case PURC_VARIANT_TYPE_ARRAY:
            return arr_rev_update_change(pre, src, edge, nr_args, argv);
        case PURC_VARIANT_TYPE_OBJECT:
            return obj_rev_update_change(pre, src, edge, nr_args, argv);
        default:
            PC_DEBUGX("Not supported for `%s` variant",
                    pcvariant_get_typename(src->type));
            PC_ASSERT(0);
    }
}

static bool
rev_update(
        bool pre,
        purc_variant_t src,
        pcvar_op_t op,
        struct pcvar_rev_update_edge *edge,
        size_t nr_args,
        purc_variant_t *argv)
{
    switch (op) {
        case PCVAR_OPERATION_GROW:
            return rev_update_grow(pre, src, edge, nr_args, argv);
        case PCVAR_OPERATION_SHRINK:
            return rev_update_shrink(pre, src, edge, nr_args, argv);
        case PCVAR_OPERATION_CHANGE:
            return rev_update_change(pre, src, edge, nr_args, argv);
        default:
            PC_ASSERT(0);
    }
}

bool
pcvar_rev_update_chain_pre_handler(
        purc_variant_t src,  // the source variant.
        pcvar_op_t op,       // the operation identifier.
        void *ctxt,          // the context stored when registering the handler.
        size_t nr_args,      // the number of the relevant child variants.
        purc_variant_t *argv // the array of all relevant child variants.
        )
{
    PC_ASSERT(ctxt);
    struct pcvar_rev_update_edge *edge;
    edge = (struct pcvar_rev_update_edge*)ctxt;
    PC_ASSERT(edge->parent);
    PC_ASSERT(edge->parent != src);

    bool pre = true;
    return rev_update(pre, src, op, edge, nr_args, argv);
}

bool
pcvar_rev_update_chain_post_handler(
        purc_variant_t src,  // the source variant.
        pcvar_op_t op,       // the operation identifier.
        void *ctxt,          // the context stored when registering the handler.
        size_t nr_args,      // the number of the relevant child variants.
        purc_variant_t *argv // the array of all relevant child variants.
        )
{
    PC_ASSERT(ctxt);
    struct pcvar_rev_update_edge *edge;
    edge = (struct pcvar_rev_update_edge*)ctxt;
    PC_ASSERT(edge->parent);

    bool pre = false;
    return rev_update(pre, src, op, edge, nr_args, argv);
}

