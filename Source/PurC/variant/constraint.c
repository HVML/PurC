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
rev_update_grow(
        bool pre,
        purc_variant_t src,
        struct pcvar_rev_update_edge *edge,
        size_t nr_args,
        purc_variant_t *argv)
{
    UNUSED_PARAM(pre);
    UNUSED_PARAM(src);
    UNUSED_PARAM(edge);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    PC_ASSERT(0);
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
    UNUSED_PARAM(pre);
    UNUSED_PARAM(src);
    UNUSED_PARAM(edge);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    PC_ASSERT(0);
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
    UNUSED_PARAM(pre);
    UNUSED_PARAM(obj);
    UNUSED_PARAM(edge);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    variant_obj_t data = pcvar_obj_get_data(obj);
    PC_ASSERT(data);
    PC_ASSERT(&data->rev_update_chain == edge);

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
        case PURC_VARIANT_TYPE_OBJECT:
            return obj_rev_update_change(pre, src, edge, nr_args, argv);
        default:
            PC_DEBUGX("Not supported for `%s` variant",
                    pcvariant_get_typename(src->type));
            PC_ASSERT(0);
    }

    return true;
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
    UNUSED_PARAM(pre);
    UNUSED_PARAM(src);
    UNUSED_PARAM(op);
    UNUSED_PARAM(edge);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

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

