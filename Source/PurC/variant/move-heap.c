/*
 * @file move-heap.c
 * @author Vincent Wei
 * @date 2022/03/08
 * @brief The implementation of internal interfaces to move variant.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
 *
 * Authors:
 *  Vincent Wei (https://github.com/VincentWei), 2022
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

#include "private/instance.h"
#include "private/variant.h"

#include "variant-internals.h"

#include <stdlib.h>
#include <string.h>

static struct purc_mutex        mh_lock;
static struct pcvariant_heap    move_heap;

static void mvheap_cleanup_once(void)
{
    if (mh_lock.native_impl)
        purc_mutex_clear(&mh_lock);

    struct purc_variant_stat *stat = &move_heap.stat;

    PC_DEBUG("refc of v_undefined in move heap: %u\n", move_heap.v_undefined.refc);
    PC_DEBUG("refc of v_null in move heap: %u\n", move_heap.v_null.refc);
    PC_DEBUG("refc of v_true in move heap: %u\n", move_heap.v_true.refc);
    PC_DEBUG("refc of v_false in move heap: %u\n", move_heap.v_false.refc);
    PC_DEBUG("refc of v_false in move heap: %u\n", move_heap.v_false.refc);
    PC_DEBUG("total values in move heap: %zu\n", stat->nr_total_values);
    PC_DEBUG("total memory used by move heap: %zu\n", stat->sz_total_mem);
    PC_DEBUG("total ordinary values reserved in move heap: %zu\n", stat->nr_reserved_ord);
    PC_DEBUG("total out of ordinary values reserved in move heap: %zu\n", stat->nr_reserved_out);

    PC_ASSERT(move_heap.v_undefined.refc == 0);
    PC_ASSERT(move_heap.v_null.refc == 0);
    PC_ASSERT(move_heap.v_true.refc == 0);
    PC_ASSERT(move_heap.v_false.refc == 0);

    for (int t = PURC_VARIANT_TYPE_FIRST; t < PURC_VARIANT_TYPE_LAST; t++) {
        PC_DEBUG("values of type (%s): %u\n", purc_variant_typename(t),
                (unsigned int)stat->nr_values[t]);
    }

    PC_ASSERT(stat->nr_total_values == 4);
    PC_ASSERT(stat->sz_total_mem == 4 * sizeof(purc_variant_ord));
}

static int mvheap_init_once(void)
{
    move_heap.v_undefined.type = PURC_VARIANT_TYPE_UNDEFINED;
    move_heap.v_undefined.refc = 0;
    move_heap.v_undefined.flags = PCVRNT_FLAG_NOFREE;

    move_heap.v_null.type = PURC_VARIANT_TYPE_NULL;
    move_heap.v_null.refc = 0;
    move_heap.v_null.flags = PCVRNT_FLAG_NOFREE;

    move_heap.v_false.type = PURC_VARIANT_TYPE_BOOLEAN;
    move_heap.v_false.refc = 0;
    move_heap.v_false.flags = PCVRNT_FLAG_NOFREE;
    move_heap.v_false.b = false;

    move_heap.v_true.type = PURC_VARIANT_TYPE_BOOLEAN;
    move_heap.v_true.refc = 0;
    move_heap.v_true.flags = PCVRNT_FLAG_NOFREE;
    move_heap.v_true.b = true;

    struct purc_variant_stat *stat = &move_heap.stat;
    stat->nr_values[PURC_VARIANT_TYPE_UNDEFINED] = 0;
    stat->sz_mem[PURC_VARIANT_TYPE_UNDEFINED] = sizeof(purc_variant_ord);
    stat->nr_values[PURC_VARIANT_TYPE_NULL] = 0;
    stat->sz_mem[PURC_VARIANT_TYPE_NULL] = sizeof(purc_variant_ord);
    stat->nr_values[PURC_VARIANT_TYPE_BOOLEAN] = 0;
    stat->sz_mem[PURC_VARIANT_TYPE_BOOLEAN] = sizeof(purc_variant_ord) * 2;
    stat->nr_total_values = 4;
    stat->sz_total_mem = 4 * sizeof(purc_variant_ord);

    stat->nr_reserved_ord = 0;
    stat->nr_reserved_out = 0;
    stat->nr_max_reserved_ord = 0;  // no need to reserve variants
    stat->nr_max_reserved_out = 0;  // for move heap.

    INIT_LIST_HEAD(&move_heap.v_reserved);

    purc_mutex_init(&mh_lock);
    if (mh_lock.native_impl == NULL)
        return -1;

    int r;
    r = atexit(mvheap_cleanup_once);
    if (r)
        goto fail_atexit;

    return 0;

fail_atexit:
    purc_mutex_clear(&mh_lock);

    return -1;
}

struct pcmodule _module_mvheap = {
    .id              = PURC_HAVE_VARIANT,
    .module_inited   = 0,

    .init_once       = mvheap_init_once,
    .init_instance   = NULL,
};

static void
move_variant_in(struct pcinst *inst, purc_variant_t v)
{
    /* move directly and change the stat info */

    if (IS_CONTAINER(v->type) ||
            ((v->type == PURC_VARIANT_TYPE_STRING ||
                v->type == PURC_VARIANT_TYPE_BSEQUENCE) &&
            (v->flags & PCVRNT_FLAG_EXTRA_SIZE))) {
        inst->org_vrt_heap->stat.sz_mem[v->type] -= v->extra_size;
        inst->org_vrt_heap->stat.sz_total_mem -= v->extra_size;

        move_heap.stat.sz_mem[v->type] += v->extra_size;
        move_heap.stat.sz_total_mem += v->extra_size;
    }

    inst->org_vrt_heap->stat.nr_values[v->type]--;
    inst->org_vrt_heap->stat.nr_total_values--;
    move_heap.stat.nr_values[v->type]++;
    move_heap.stat.nr_total_values++;

    inst->org_vrt_heap->stat.sz_mem[v->type] -= sizeof(purc_variant);
    inst->org_vrt_heap->stat.sz_total_mem -= sizeof(purc_variant);
    move_heap.stat.sz_mem[v->type] += sizeof(purc_variant);
    move_heap.stat.sz_total_mem += sizeof(purc_variant);
}

static purc_variant_t
move_or_clone_immutable(struct pcinst *inst, purc_variant_t v)
{
    purc_variant_t retv = PURC_VARIANT_INVALID;

    if (IS_CONTAINER(v->type))
        return retv;

    if (v == (purc_variant_t)&inst->org_vrt_heap->v_undefined) {
        retv = (purc_variant_t)&move_heap.v_undefined;
        v->refc--;
        retv->refc++;
    }
    else if (v == (purc_variant_t)&inst->org_vrt_heap->v_null) {
        retv = (purc_variant_t)&move_heap.v_null;
        v->refc--;
        retv->refc++;
    }
    else if (v == (purc_variant_t)&inst->org_vrt_heap->v_false) {
        retv = (purc_variant_t)&move_heap.v_false;
        v->refc--;
        retv->refc++;
    }
    else if (v == (purc_variant_t)&inst->org_vrt_heap->v_true) {
        retv = (purc_variant_t)&move_heap.v_true;
        v->refc--;
        retv->refc++;
    }
    else if (v->refc == 1) {
        PC_NONE("Move in variant type %s (%u): %s\n",
                purc_variant_typename(v->type),
                (unsigned)move_heap.stat.nr_values[v->type],
                purc_variant_is_string(v) ? purc_variant_get_string_const(v): NULL);

        retv = v;
        move_variant_in(inst, v);
    }
    else {
        // clone the immutable variant
        PC_NONE("Clone a variant type %s (%u): %s (%lu/%lu)\n",
                purc_variant_typename(v->type),
                (unsigned)move_heap.stat.nr_values[v->type],
                purc_variant_is_string(v) ? purc_variant_get_string_const(v): NULL,
                (size_t)v->len,
                v->extra_size);

        bool ordinary = is_variant_ordinary(v);
        retv = pcvariant_alloc(ordinary);
        memcpy(retv, v, ordinary ? sizeof(purc_variant_ord) : sizeof(*v));
        retv->refc = 1;

        /* copy the extra space */
        if ((v->type == PURC_VARIANT_TYPE_STRING ||
                v->type == PURC_VARIANT_TYPE_BSEQUENCE) &&
                (v->flags & PCVRNT_FLAG_EXTRA_SIZE)) {

            retv->ptr2 = malloc(v->extra_size);
            memcpy(retv->ptr2, v->ptr2, v->extra_size);

            move_heap.stat.sz_mem[v->type] += v->extra_size;
            move_heap.stat.sz_total_mem += v->extra_size;
        }

        move_heap.stat.nr_values[v->type]++;
        move_heap.stat.nr_total_values++;
        move_heap.stat.sz_mem[v->type] += sizeof(purc_variant);
        move_heap.stat.sz_total_mem += sizeof(purc_variant);
    }

    return retv;
}

struct travel_context {
    struct pcinst *inst;
    struct pcutils_arrlist *vrts_to_unref;
};

static bool
move_keys_in_cloned_array(struct travel_context *ctxt, purc_variant_t arr);
static bool
move_keys_in_cloned_object(struct travel_context *ctxt, purc_variant_t arr);
static bool
move_keys_in_cloned_set(struct travel_context *ctxt, purc_variant_t arr);
static bool
move_keys_in_cloned_tuple(struct travel_context *ctxt, purc_variant_t arr);

static bool
move_keys_in_cloned_array(struct travel_context *ctxt, purc_variant_t arr)
{
    size_t idx;
    purc_variant_t v;
    foreach_value_in_variant_array(arr, v, idx) {
        UNUSED_PARAM(idx);

        switch (v->type) {
        case PURC_VARIANT_TYPE_ARRAY:
            move_keys_in_cloned_array(ctxt, v);
            break;

        case PURC_VARIANT_TYPE_OBJECT:
            move_keys_in_cloned_object(ctxt, v);
            break;

        case PURC_VARIANT_TYPE_SET:
            move_keys_in_cloned_set(ctxt, v);
            break;

        case PURC_VARIANT_TYPE_TUPLE:
            move_keys_in_cloned_tuple(ctxt, v);
            break;

        default:
            // immutable element
            break;
        }

    } end_foreach;

    return true;
}

static bool
move_keys_in_cloned_object(struct travel_context *ctxt, purc_variant_t obj)
{
    purc_variant_t k,v;
    foreach_key_value_in_variant_object(obj, k, v) {

        if (IS_CONTAINER(v->type)) {
            PC_NONE("Move in a key %s (%u): %s\n",
                    purc_variant_typename(k->type),
                    (unsigned)move_heap.stat.nr_values[k->type],
                    purc_variant_get_string_const(k));
        }

        switch (v->type) {
        case PURC_VARIANT_TYPE_ARRAY:
            move_variant_in(ctxt->inst, k);
            move_keys_in_cloned_array(ctxt, v);
            break;

        case PURC_VARIANT_TYPE_OBJECT:
            move_variant_in(ctxt->inst, k);
            move_keys_in_cloned_object(ctxt, v);
            break;

        case PURC_VARIANT_TYPE_SET:
            move_variant_in(ctxt->inst, k);
            move_keys_in_cloned_set(ctxt, v);
            break;

        case PURC_VARIANT_TYPE_TUPLE:
            move_variant_in(ctxt->inst, k);
            move_keys_in_cloned_tuple(ctxt, v);
            break;

        default:
            break;
        }

    } end_foreach;

    return true;
}

static bool
move_keys_in_cloned_set(struct travel_context *ctxt, purc_variant_t set)
{
    purc_variant_t v;
    foreach_value_in_variant_set(set, v) {

        switch (v->type) {
        case PURC_VARIANT_TYPE_ARRAY:
            move_keys_in_cloned_array(ctxt, v);
            break;

        case PURC_VARIANT_TYPE_OBJECT:
            move_keys_in_cloned_object(ctxt, v);
            break;

        case PURC_VARIANT_TYPE_SET:
            move_keys_in_cloned_set(ctxt, v);
            break;

        case PURC_VARIANT_TYPE_TUPLE:
            move_keys_in_cloned_tuple(ctxt, v);
            break;

        default:
            // immutable element
            break;
        }

    } end_foreach;

    return true;
}

static bool
move_keys_in_cloned_tuple(struct travel_context *ctxt, purc_variant_t arr)
{
    size_t sz;
    purc_variant_t *members;
    members = tuple_members(arr, &sz);
    assert(members);

    purc_variant_t v;
    for (size_t idx = 0; idx < sz; idx++) {
        v = members[idx];
        switch (v->type) {
        case PURC_VARIANT_TYPE_ARRAY:
            move_keys_in_cloned_array(ctxt, v);
            break;

        case PURC_VARIANT_TYPE_OBJECT:
            move_keys_in_cloned_object(ctxt, v);
            break;

        case PURC_VARIANT_TYPE_SET:
            move_keys_in_cloned_set(ctxt, v);
            break;

        case PURC_VARIANT_TYPE_TUPLE:
            move_keys_in_cloned_tuple(ctxt, v);
            break;

        default:
            // immutable element
            break;
        }

    }

    return true;
}

static bool
move_keys_in_cloned_container(struct travel_context *ctxt,
        purc_variant_t cntr)
{
    switch (cntr->type) {
        case PURC_VARIANT_TYPE_ARRAY:
            move_keys_in_cloned_array(ctxt, cntr);
            break;

        case PURC_VARIANT_TYPE_OBJECT:
            move_keys_in_cloned_object(ctxt, cntr);
            break;

        case PURC_VARIANT_TYPE_SET:
            move_keys_in_cloned_set(ctxt, cntr);
            break;

        case PURC_VARIANT_TYPE_TUPLE:
            move_keys_in_cloned_tuple(ctxt, cntr);
            break;

        default:
            assert(0);
            break;
    }

    return true;
}

static bool
move_or_clone_mutable_descendants_in_array(struct travel_context *ctxt,
        purc_variant_t v);
static bool
move_or_clone_mutable_descendants_in_object(struct travel_context *ctxt,
        purc_variant_t v);
static bool
move_or_clone_mutable_descendants_in_set(struct travel_context *ctxt,
        purc_variant_t v);
static bool
move_or_clone_mutable_descendants_in_tuple(struct travel_context *ctxt,
        purc_variant_t v);

static bool
move_or_clone_mutable_descendants_in_array(struct travel_context *ctxt,
        purc_variant_t arr)
{
    size_t idx;
    purc_variant_t v;
    foreach_value_in_variant_array(arr, v, idx) {
        purc_variant_t retv;

        UNUSED_PARAM(idx);
        switch (v->type) {
        case PURC_VARIANT_TYPE_ARRAY:
            if (v->refc == 1) {
                move_variant_in(ctxt->inst, v);
                move_or_clone_mutable_descendants_in_array(ctxt, v);
            }
            break;

        case PURC_VARIANT_TYPE_OBJECT:
            if (v->refc == 1) {
                move_variant_in(ctxt->inst, v);
                move_or_clone_mutable_descendants_in_object(ctxt, v);
            }
            break;

        case PURC_VARIANT_TYPE_SET:
            if (v->refc == 1) {
                move_variant_in(ctxt->inst, v);
                move_or_clone_mutable_descendants_in_set(ctxt, v);
            }
            break;

        default:
            // immutable element
            break;
        }

        if (IS_CONTAINER(v->type) && v->refc > 1) {
            retv = purc_variant_container_clone_recursively(v);
            if (retv == PURC_VARIANT_INVALID) {
                purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
                return false;
            }

            move_keys_in_cloned_container(ctxt, retv);

            _p->val = retv;
            pcutils_arrlist_append(ctxt->vrts_to_unref, v);
        }

    } end_foreach;

    return true;
}

static bool
move_or_clone_mutable_descendants_in_object(struct travel_context *ctxt,
        purc_variant_t obj)
{
    purc_variant_t k,v;
    foreach_key_value_in_variant_object(obj, k, v) {
        purc_variant_t retk, retv;

        PC_NONE("a key when handling mutable variant: %s (%u)\n",
                purc_variant_get_string_const(k), (unsigned)v->refc);

        switch (v->type) {
        case PURC_VARIANT_TYPE_ARRAY:
            if (v->refc == 1) {
                move_variant_in(ctxt->inst, v);
                move_or_clone_mutable_descendants_in_array(ctxt, v);
            }
            break;

        case PURC_VARIANT_TYPE_OBJECT:
            if (v->refc == 1) {
                move_variant_in(ctxt->inst, v);
                move_or_clone_mutable_descendants_in_object(ctxt, v);
            }
            break;

        case PURC_VARIANT_TYPE_SET:
            if (v->refc == 1) {
                move_variant_in(ctxt->inst, v);
                move_or_clone_mutable_descendants_in_set(ctxt, v);
            }
            break;

        default:
            // immutable element
            break;
        }

        if (IS_CONTAINER(v->type)) {
            retk = move_or_clone_immutable(ctxt->inst, k);
            if (retk != k) {
                _node->key = retk;
                pcutils_arrlist_append(ctxt->vrts_to_unref, k);
            }

            if (v->refc > 1) {
                retv = purc_variant_container_clone_recursively(v);
                if (retv == PURC_VARIANT_INVALID) {
                    purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
                    return false;
                }

                /* XXX: for cloned object, we need to move in the cloned keys,
                 * cause purc_variant_container_clone_recursively() only
                 * references the keys */
                PC_NONE("a container cloned for key %s: %s (%u)\n",
                        purc_variant_get_string_const(k),
                        purc_variant_typename(retv->type),
                        (unsigned)retv->refc);
                move_keys_in_cloned_object(ctxt, retv);

                _node->val = retv;
                pcutils_arrlist_append(ctxt->vrts_to_unref, v);
            }
        }

    } end_foreach;

    return true;
}

static bool
move_or_clone_mutable_descendants_in_set(struct travel_context *ctxt,
        purc_variant_t set)
{
    purc_variant_t v;
    foreach_value_in_variant_set(set, v) {
        purc_variant_t retv;

        switch (v->type) {
        case PURC_VARIANT_TYPE_ARRAY:
            if (v->refc == 1) {
                move_variant_in(ctxt->inst, v);
                move_or_clone_mutable_descendants_in_array(ctxt, v);
            }
            break;

        case PURC_VARIANT_TYPE_OBJECT:
            if (v->refc == 1) {
                move_variant_in(ctxt->inst, v);
                move_or_clone_mutable_descendants_in_object(ctxt, v);
            }
            break;

        case PURC_VARIANT_TYPE_SET:
            if (v->refc == 1) {
                move_variant_in(ctxt->inst, v);
                move_or_clone_mutable_descendants_in_set(ctxt, v);
            }
            break;

        default:
            // immutable element
            break;
        }

        if (IS_CONTAINER(v->type) && v->refc > 1) {
            retv = purc_variant_container_clone_recursively(v);
            if (retv == PURC_VARIANT_INVALID) {
                purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
                return false;
            }

            move_keys_in_cloned_container(ctxt, retv);

            _sn->val = retv;
            pcutils_arrlist_append(ctxt->vrts_to_unref, v);
        }

    } end_foreach;

    return true;
}

static bool
move_or_clone_mutable_descendants_in_tuple(struct travel_context *ctxt,
        purc_variant_t tuple)
{
    size_t sz;
    purc_variant_t *members;
    members = tuple_members(tuple, &sz);
    assert(members);

    purc_variant_t v;
    for (size_t idx = 0; idx < sz; idx++) {
        v = members[idx];
        purc_variant_t retv;

        switch (v->type) {
        case PURC_VARIANT_TYPE_ARRAY:
            if (v->refc == 1) {
                move_variant_in(ctxt->inst, v);
                move_or_clone_mutable_descendants_in_array(ctxt, v);
            }
            break;

        case PURC_VARIANT_TYPE_OBJECT:
            if (v->refc == 1) {
                move_variant_in(ctxt->inst, v);
                move_or_clone_mutable_descendants_in_object(ctxt, v);
            }
            break;

        case PURC_VARIANT_TYPE_SET:
            if (v->refc == 1) {
                move_variant_in(ctxt->inst, v);
                move_or_clone_mutable_descendants_in_set(ctxt, v);
            }
            break;

        default:
            // immutable element
            break;
        }

        if (IS_CONTAINER(v->type) && v->refc > 1) {
            retv = purc_variant_container_clone_recursively(v);
            if (retv == PURC_VARIANT_INVALID) {
                purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
                return false;
            }

            move_keys_in_cloned_container(ctxt, retv);

            pcutils_arrlist_append(ctxt->vrts_to_unref, v);
        }

    };

    return true;
}

static bool
move_or_clone_mutable_descendants(struct travel_context *ctxt,
        purc_variant_t v)
{
    switch (v->type) {
        case PURC_VARIANT_TYPE_ARRAY:
            return move_or_clone_mutable_descendants_in_array(ctxt, v);
        case PURC_VARIANT_TYPE_OBJECT:
            return move_or_clone_mutable_descendants_in_object(ctxt, v);
        case PURC_VARIANT_TYPE_SET:
            return move_or_clone_mutable_descendants_in_set(ctxt, v);
        case PURC_VARIANT_TYPE_TUPLE:
            return move_or_clone_mutable_descendants_in_tuple(ctxt, v);
        default:
            break;
    }

    return true;
}

static bool
move_or_clone_immutable_descendants_in_array(struct travel_context *ctxt,
        purc_variant_t v);
static bool
move_or_clone_immutable_descendants_in_object(struct travel_context *ctxt,
        purc_variant_t v);
static bool
move_or_clone_immutable_descendants_in_set(struct travel_context *ctxt,
        purc_variant_t v);
static bool
move_or_clone_immutable_descendants_in_tuple(struct travel_context *ctxt,
        purc_variant_t v);

static bool
move_or_clone_immutable_descendants_in_array(struct travel_context *ctxt,
        purc_variant_t arr)
{
    size_t idx;
    purc_variant_t v;
    foreach_value_in_variant_array(arr, v, idx) {
        purc_variant_t retv;

        UNUSED_PARAM(idx);
        switch (v->type) {
        case PURC_VARIANT_TYPE_ARRAY:
            move_or_clone_immutable_descendants_in_array(ctxt, v);
            retv = v;
            break;

        case PURC_VARIANT_TYPE_OBJECT:
            move_or_clone_immutable_descendants_in_object(ctxt, v);
            retv = v;
            break;

        case PURC_VARIANT_TYPE_SET:
            move_or_clone_immutable_descendants_in_set(ctxt, v);
            retv = v;
            break;

        case PURC_VARIANT_TYPE_TUPLE:
            move_or_clone_immutable_descendants_in_tuple(ctxt, v);
            retv = v;
            break;

        default:
            retv = move_or_clone_immutable(ctxt->inst, v);
            if (retv == PURC_VARIANT_INVALID) {
                purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
                return false;
            }
            break;
        }

        if (retv != v) {
            _p->val = retv;
            if (!(v->flags & PCVRNT_FLAG_NOFREE))
                pcutils_arrlist_append(ctxt->vrts_to_unref, v);
        }

    } end_foreach;

    return true;
}

static bool
move_or_clone_immutable_descendants_in_object(struct travel_context *ctxt,
        purc_variant_t obj)
{
    purc_variant_t k,v;
    foreach_key_value_in_variant_object(obj, k, v) {
        purc_variant_t retk, retv;

        switch (v->type) {
        case PURC_VARIANT_TYPE_ARRAY:
            move_or_clone_immutable_descendants_in_array(ctxt, v);
            break;

        case PURC_VARIANT_TYPE_OBJECT:
            move_or_clone_immutable_descendants_in_object(ctxt, v);
            break;

        case PURC_VARIANT_TYPE_SET:
            move_or_clone_immutable_descendants_in_set(ctxt, v);
            break;

        case PURC_VARIANT_TYPE_TUPLE:
            move_or_clone_immutable_descendants_in_tuple(ctxt, v);
            break;

        default:
            retk = move_or_clone_immutable(ctxt->inst, k);
            if (retk != k) {
                _node->key = retk;
                pcutils_arrlist_append(ctxt->vrts_to_unref, k);
            }

            retv = move_or_clone_immutable(ctxt->inst, v);
            if (retv != v) {
                _node->val = retv;
                if (!(v->flags & PCVRNT_FLAG_NOFREE))
                    pcutils_arrlist_append(ctxt->vrts_to_unref, v);
            }
            break;
        }

    } end_foreach;

    return true;
}

static bool
move_or_clone_immutable_descendants_in_set(struct travel_context *ctxt,
        purc_variant_t set)
{
    purc_variant_t v;
    foreach_value_in_variant_set(set, v) {
        purc_variant_t retv;

        switch (v->type) {
        case PURC_VARIANT_TYPE_ARRAY:
            move_or_clone_immutable_descendants_in_array(ctxt, v);
            retv = v;
            break;

        case PURC_VARIANT_TYPE_OBJECT:
            move_or_clone_immutable_descendants_in_object(ctxt, v);
            retv = v;
            break;

        case PURC_VARIANT_TYPE_SET:
            move_or_clone_immutable_descendants_in_set(ctxt, v);
            retv = v;
            break;

        case PURC_VARIANT_TYPE_TUPLE:
            move_or_clone_immutable_descendants_in_tuple(ctxt, v);
            retv = v;
            break;

        default:
            retv = move_or_clone_immutable(ctxt->inst, v);
            if (retv == PURC_VARIANT_INVALID) {
                purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
                return false;
            }
            break;
        }

        if (retv != v) {
            _sn->val = retv;
            if (!(v->flags & PCVRNT_FLAG_NOFREE))
                pcutils_arrlist_append(ctxt->vrts_to_unref, v);
        }

    } end_foreach;

    return true;
}

static bool
move_or_clone_immutable_descendants_in_tuple(struct travel_context *ctxt,
        purc_variant_t tuple)
{
    size_t sz;
    purc_variant_t *members;
    members = tuple_members(tuple, &sz);
    assert(members);

    purc_variant_t v;
    for (size_t idx = 0; idx < sz; idx++) {
        v = members[idx];
        purc_variant_t retv;

        switch (v->type) {
        case PURC_VARIANT_TYPE_ARRAY:
            move_or_clone_immutable_descendants_in_array(ctxt, v);
            retv = v;
            break;

        case PURC_VARIANT_TYPE_OBJECT:
            move_or_clone_immutable_descendants_in_object(ctxt, v);
            retv = v;
            break;

        case PURC_VARIANT_TYPE_SET:
            move_or_clone_immutable_descendants_in_set(ctxt, v);
            retv = v;
            break;

        case PURC_VARIANT_TYPE_TUPLE:
            move_or_clone_immutable_descendants_in_tuple(ctxt, v);
            retv = v;
            break;

        default:
            retv = move_or_clone_immutable(ctxt->inst, v);
            if (retv == PURC_VARIANT_INVALID) {
                purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
                return false;
            }
            break;
        }

        if (retv != v) {
            if (!(v->flags & PCVRNT_FLAG_NOFREE))
                pcutils_arrlist_append(ctxt->vrts_to_unref, v);
        }

    };

    return true;
}

static bool
move_or_clone_immutable_descendants(struct travel_context *ctxt,
        purc_variant_t v)
{
    switch (v->type) {
        case PURC_VARIANT_TYPE_ARRAY:
            return move_or_clone_immutable_descendants_in_array(ctxt, v);
        case PURC_VARIANT_TYPE_OBJECT:
            return move_or_clone_immutable_descendants_in_object(ctxt, v);
        case PURC_VARIANT_TYPE_SET:
            return move_or_clone_immutable_descendants_in_set(ctxt, v);
        case PURC_VARIANT_TYPE_TUPLE:
            return move_or_clone_immutable_descendants_in_tuple(ctxt, v);
        default:
            break;
    }

    return true;
}

static void cb_free_element(void *data)
{
    purc_variant_unref(data);
}

// move the variant from the current instance to the move heap.
purc_variant_t pcvariant_move_heap_in(purc_variant_t v)
{
    purc_variant_t retv = PURC_VARIANT_INVALID;
    struct pcinst *inst = pcinst_current();
    struct travel_context ctxt;

    ctxt.inst = pcinst_current();
    ctxt.vrts_to_unref = pcutils_arrlist_new(cb_free_element);
    if (ctxt.vrts_to_unref == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return retv;
    }

    pcvariant_use_move_heap();

    if (IS_CONTAINER(v->type)) {
        if (v->refc == 1) {
            retv = v;
            move_variant_in(inst, v);
            move_or_clone_mutable_descendants(&ctxt, v);

        }
        else {
            retv = purc_variant_container_clone_recursively(v);

            /* XXX: for cloned container, we need to move in the cloned keys
             * of descendant objects,
             * cause purc_variant_container_clone_recursively() only
             * references the keys */
            move_keys_in_cloned_container(&ctxt, retv);
        }

        move_or_clone_immutable_descendants(&ctxt, retv);
    }
    else {
        retv = move_or_clone_immutable(inst, v);
    }

    pcvariant_use_norm_heap();

    if (retv != PURC_VARIANT_INVALID && retv != v &&
            !(v->flags & PCVRNT_FLAG_NOFREE)) {
        purc_variant_unref(v);
    }

    // the cloned immutable descendants will be unreferenced in cb_free_element
    pcutils_arrlist_free(ctxt.vrts_to_unref);

    return retv;
}

// move the variant from the move heap to the current instance.
// we only need to update the stat information.
static void move_container_self_out(purc_variant_t v)
{
    struct pcinst *inst = pcinst_current();

    inst->org_vrt_heap->stat.sz_mem[v->type] += v->extra_size;
    inst->org_vrt_heap->stat.sz_total_mem += v->extra_size;

    move_heap.stat.sz_mem[v->type] -= v->extra_size;
    move_heap.stat.sz_total_mem -= v->extra_size;

    inst->org_vrt_heap->stat.nr_values[v->type]++;
    inst->org_vrt_heap->stat.nr_total_values++;

    move_heap.stat.nr_values[v->type]--;
    move_heap.stat.nr_total_values--;

    inst->org_vrt_heap->stat.sz_mem[v->type] += sizeof(purc_variant);
    inst->org_vrt_heap->stat.sz_total_mem += sizeof(purc_variant);
    move_heap.stat.sz_mem[v->type] -= sizeof(purc_variant);
    move_heap.stat.sz_total_mem -= sizeof(purc_variant);
}

static purc_variant_t move_variant_out(purc_variant_t v);
static purc_variant_t move_array_descendants_out(purc_variant_t v);
static purc_variant_t move_object_descendants_out(purc_variant_t v);
static purc_variant_t move_set_descendants_out(purc_variant_t v);
static purc_variant_t move_tuple_descendants_out(purc_variant_t v);

static purc_variant_t move_array_descendants_out(purc_variant_t arr)
{
    size_t idx;
    purc_variant_t v;

    foreach_value_in_variant_array(arr, v, idx) {
        purc_variant_t retv;

        UNUSED_PARAM(idx);
        switch (v->type) {
        case PURC_VARIANT_TYPE_ARRAY:
            retv = move_array_descendants_out(v);
            move_container_self_out(retv);
            break;

        case PURC_VARIANT_TYPE_OBJECT:
            retv = move_object_descendants_out(v);
            move_container_self_out(retv);
            break;

        case PURC_VARIANT_TYPE_SET:
            retv = move_set_descendants_out(v);
            move_container_self_out(retv);
            break;

        case PURC_VARIANT_TYPE_TUPLE:
            retv = move_tuple_descendants_out(v);
            move_container_self_out(retv);
            break;

        default:
            retv = move_variant_out(v);
            break;
        }

        _p->val = retv;

    } end_foreach;

    return arr;
}

static purc_variant_t move_object_descendants_out(purc_variant_t obj)
{
    purc_variant_t k,v;
    foreach_key_value_in_variant_object(obj, k, v) {
        purc_variant_t retk, retv;

        retk = move_variant_out(k);
        switch (v->type) {
        case PURC_VARIANT_TYPE_ARRAY:
            retv = move_array_descendants_out(v);
            move_container_self_out(retv);
            break;

        case PURC_VARIANT_TYPE_OBJECT:
            retv = move_object_descendants_out(v);
            move_container_self_out(retv);
            break;

        case PURC_VARIANT_TYPE_SET:
            retv = move_set_descendants_out(v);
            move_container_self_out(retv);
            break;

        case PURC_VARIANT_TYPE_TUPLE:
            retv = move_tuple_descendants_out(v);
            move_container_self_out(retv);
            break;

        default:
            retv = move_variant_out(v);
            break;
        }

        _node->key = retk;
        _node->val = retv;

    } end_foreach;

    return obj;
}

static purc_variant_t move_set_descendants_out(purc_variant_t set)
{
    purc_variant_t v;
    foreach_value_in_variant_set(set, v) {
        purc_variant_t retv;

        switch (v->type) {
        case PURC_VARIANT_TYPE_ARRAY:
            retv = move_array_descendants_out(v);
            move_container_self_out(retv);
            break;

        case PURC_VARIANT_TYPE_OBJECT:
            retv = move_object_descendants_out(v);
            move_container_self_out(retv);
            break;

        case PURC_VARIANT_TYPE_SET:
            retv = move_set_descendants_out(v);
            move_container_self_out(retv);
            break;

        case PURC_VARIANT_TYPE_TUPLE:
            retv = move_tuple_descendants_out(v);
            move_container_self_out(retv);
            break;

        default:
            retv = move_variant_out(v);
            break;
        }

        _sn->val = retv;
    } end_foreach;

    return set;
}

static purc_variant_t move_tuple_descendants_out(purc_variant_t tuple)
{
    size_t sz;
    purc_variant_t *members;
    members = tuple_members(tuple, &sz);
    assert(members);

    purc_variant_t v;
    for (size_t idx = 0; idx < sz; idx++) {
        v = members[idx];
        purc_variant_t retv;

        switch (v->type) {
        case PURC_VARIANT_TYPE_ARRAY:
            retv = move_array_descendants_out(v);
            move_container_self_out(retv);
            break;

        case PURC_VARIANT_TYPE_OBJECT:
            retv = move_object_descendants_out(v);
            move_container_self_out(retv);
            break;

        case PURC_VARIANT_TYPE_SET:
            retv = move_set_descendants_out(v);
            move_container_self_out(retv);
            break;

        case PURC_VARIANT_TYPE_TUPLE:
            retv = move_tuple_descendants_out(v);
            move_container_self_out(retv);
            break;

        default:
            retv = move_variant_out(v);
            break;
        }

    }

    return tuple;
}

static purc_variant_t move_variant_out(purc_variant_t v)
{
    purc_variant_t retv = v;
    struct pcinst *inst = pcinst_current();

    if (v == (purc_variant_t)&move_heap.v_undefined) {
        retv = (purc_variant_t)&inst->org_vrt_heap->v_undefined;
        v->refc--;
        retv->refc++;
        return retv;
    }
    else if (v == (purc_variant_t)&move_heap.v_null) {
        retv = (purc_variant_t)&inst->org_vrt_heap->v_null;
        v->refc--;
        retv->refc++;
        return retv;
    }
    else if (v == (purc_variant_t)&move_heap.v_false) {
        retv = (purc_variant_t)&inst->org_vrt_heap->v_false;
        v->refc--;
        retv->refc++;
        return retv;
    }
    else if (v == (purc_variant_t)&move_heap.v_true) {
        retv = (purc_variant_t)&inst->org_vrt_heap->v_true;
        v->refc--;
        retv->refc++;
        return retv;
    }
    else if ((v->type == PURC_VARIANT_TYPE_STRING ||
                  v->type == PURC_VARIANT_TYPE_BSEQUENCE) &&
                 (v->flags & PCVRNT_FLAG_EXTRA_SIZE)) {
        inst->org_vrt_heap->stat.sz_mem[v->type] += v->extra_size;
        inst->org_vrt_heap->stat.sz_total_mem += v->extra_size;

        move_heap.stat.sz_mem[v->type] -= v->extra_size;
        move_heap.stat.sz_total_mem -= v->extra_size;
    }
    else if (IS_CONTAINER(v->type)) {
        inst->org_vrt_heap->stat.sz_mem[v->type] += v->extra_size;
        inst->org_vrt_heap->stat.sz_total_mem += v->extra_size;

        move_heap.stat.sz_mem[v->type] -= v->extra_size;
        move_heap.stat.sz_total_mem -= v->extra_size;

        if (v->type == PURC_VARIANT_TYPE_ARRAY) {
            retv = move_array_descendants_out(v);
        }
        else if (v->type == PURC_VARIANT_TYPE_OBJECT) {
            retv = move_object_descendants_out(v);
        }
        else if (v->type == PURC_VARIANT_TYPE_SET) {
            retv = move_set_descendants_out(v);
        }
        else if (v->type == PURC_VARIANT_TYPE_TUPLE) {
            retv = move_tuple_descendants_out(v);
        }
    }

    inst->org_vrt_heap->stat.nr_values[v->type]++;
    inst->org_vrt_heap->stat.nr_total_values++;

    PC_NONE("Move out a variant type: %s (%u): %s\n",
            purc_variant_typename(v->type),
            (unsigned)move_heap.stat.nr_values[v->type],
            purc_variant_is_string(v) ? purc_variant_get_string_const(v): NULL);

    assert(move_heap.stat.nr_values[v->type] > 0);
    assert(move_heap.stat.nr_total_values > 0);

    move_heap.stat.nr_values[v->type]--;
    move_heap.stat.nr_total_values--;

    inst->org_vrt_heap->stat.sz_mem[v->type] += sizeof(purc_variant);
    inst->org_vrt_heap->stat.sz_total_mem += sizeof(purc_variant);
    move_heap.stat.sz_mem[v->type] -= sizeof(purc_variant);
    move_heap.stat.sz_total_mem -= sizeof(purc_variant);

    return retv;
}

purc_variant_t pcvariant_move_heap_out(purc_variant_t v)
{
    purc_variant_t retv = PURC_VARIANT_INVALID;

    pcvariant_use_move_heap();
    retv = move_variant_out(v);
    pcvariant_use_norm_heap();

    return retv;
}

void pcvariant_use_move_heap(void)
{
    struct pcinst *inst = pcinst_current();
    purc_mutex_lock(&mh_lock);
    inst->variant_heap = &move_heap;
}

void pcvariant_use_norm_heap(void)
{
    struct pcinst *inst = pcinst_current();
    inst->variant_heap = inst->org_vrt_heap;
    purc_mutex_unlock(&mh_lock);
}

