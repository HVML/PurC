/*
 * @file walk.c
 * @author Xu Xiaohong
 * @date 2022/06/29
 * @brief The implementation of public part for variant.
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

#include "variant-internals.h"

static int
parallel_walk(purc_variant_t l, purc_variant_t r, void *ctxt,
        int (*cb)(purc_variant_t l, purc_variant_t r, void *ctxt));

static int
obj_parallel_walk(purc_variant_t l, purc_variant_t r, void *ctxt,
        int (*cb)(purc_variant_t l, purc_variant_t r, void *ctxt))
{
    struct obj_iterator lit, rit;
    lit = pcvar_obj_it_first(l);
    rit = pcvar_obj_it_first(r);

    while (lit.curr && rit.curr) {
        int r = cb(lit.curr->key, rit.curr->key, ctxt);
        if (r)
            return r;

        r = parallel_walk(lit.curr->val, rit.curr->val, ctxt, cb);
        if (r)
            return r;

        pcvar_obj_it_next(&lit);
        pcvar_obj_it_next(&rit);
    }

    if (lit.curr == NULL && rit.curr == NULL)
        return 0;

    if (lit.curr)
        return parallel_walk(lit.curr->val, PURC_VARIANT_INVALID, ctxt, cb);
    else
        return parallel_walk(PURC_VARIANT_INVALID, rit.curr->val, ctxt, cb);
}

static int
arr_parallel_walk(purc_variant_t l, purc_variant_t r, void *ctxt,
        int (*cb)(purc_variant_t l, purc_variant_t r, void *ctxt))
{
    struct arr_iterator lit, rit;
    lit = pcvar_arr_it_first(l);
    rit = pcvar_arr_it_first(r);

    while (lit.curr && rit.curr) {
        int r = parallel_walk(lit.curr->val, rit.curr->val, ctxt, cb);
        if (r)
            return r;

        pcvar_arr_it_next(&lit);
        pcvar_arr_it_next(&rit);
    }

    if (lit.curr == NULL && rit.curr == NULL)
        return 0;

    if (lit.curr)
        return parallel_walk(lit.curr->val, PURC_VARIANT_INVALID, ctxt, cb);
    else
        return parallel_walk(PURC_VARIANT_INVALID, rit.curr->val, ctxt, cb);
}

static int
set_parallel_walk(purc_variant_t l, purc_variant_t r, void *ctxt,
        int (*cb)(purc_variant_t l, purc_variant_t r, void *ctxt))
{
    enum set_it_type it_type = SET_IT_RBTREE;

    struct set_iterator lit, rit;
    lit = pcvar_set_it_first(l, it_type);
    rit = pcvar_set_it_first(r, it_type);

    while (lit.curr && rit.curr) {
        int r = parallel_walk(lit.curr->val, rit.curr->val, ctxt, cb);
        if (r)
            return r;

        pcvar_set_it_next(&lit);
        pcvar_set_it_next(&rit);
    }

    if (lit.curr == NULL && rit.curr == NULL)
        return 0;

    if (lit.curr)
        return parallel_walk(lit.curr->val, PURC_VARIANT_INVALID, ctxt, cb);
    else
        return parallel_walk(PURC_VARIANT_INVALID, rit.curr->val, ctxt, cb);
}

static int
parallel_walk(purc_variant_t l, purc_variant_t r, void *ctxt,
        int (*cb)(purc_variant_t l, purc_variant_t r, void *ctxt))
{
    if (l == PURC_VARIANT_INVALID || r == PURC_VARIANT_INVALID)
        return cb(l, r, ctxt);

    if (pcvariant_is_scalar(l) || pcvariant_is_scalar(r)) {
        return cb(l, r, ctxt);
    }

    if (l->type != r->type) {
        return cb(l, r, ctxt);
    }

    // FIXME: what if comparing array and set ???

    switch (l->type) {
        case PURC_VARIANT_TYPE_OBJECT:
            return obj_parallel_walk(l, r, ctxt, cb);
            break;

        case PURC_VARIANT_TYPE_ARRAY:
            return arr_parallel_walk(l, r, ctxt, cb);
            break;

        case PURC_VARIANT_TYPE_SET:
            return set_parallel_walk(l, r, ctxt, cb);
            break;

        case PURC_VARIANT_TYPE_TUPLE:
            PC_ASSERT(0); // Not implemented yet
            // tuple_parallel_walk(l, r, ctxt, cb);
            break;

        default:
            PC_ASSERT(0);
    }

    return 0;
}

void
pcvar_parallel_walk(purc_variant_t l, purc_variant_t r, void *ctxt,
        int (*cb)(purc_variant_t l, purc_variant_t r, void *ctxt))
{
    PC_ASSERT(l != PURC_VARIANT_INVALID);
    PC_ASSERT(r != PURC_VARIANT_INVALID);

    parallel_walk(l, r, ctxt, cb);
}

