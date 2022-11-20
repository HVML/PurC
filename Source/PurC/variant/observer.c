/*
 * @file observer.c
 * @author Geng Yue
 * @date 2021/07/02
 * @brief The implementation for variant observer
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

static pcvar_listener*
register_listener(purc_variant_t v, unsigned int flags,
        pcvar_op_t op, pcvar_op_handler handler, void *ctxt)
{
    struct list_head *listeners;
    listeners = &v->listeners;

    struct pcvar_listener *listener;
    listener = (struct pcvar_listener*)calloc(1, sizeof(*listener));
    if (!listener) {
        pcinst_set_error(PCVARIANT_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    listener->flags          = flags;
    listener->op             = op;
    listener->ctxt           = ctxt;
    listener->handler        = handler;

    if ((flags & PCVAR_LISTENER_PRE_OR_POST) == PCVAR_LISTENER_PRE) {
        list_add(&listener->list_node, listeners);
    } else {
        list_add_tail(&listener->list_node, listeners);
    }

    return listener;
}

struct pcvar_listener*
purc_variant_register_pre_listener(purc_variant_t v,
        pcvar_op_t op, pcvar_op_handler handler, void *ctxt)
{
    if ((op & PCVAR_OPERATION_ALL) != op) {
        pcinst_set_error(PCVARIANT_ERROR_WRONG_ARGS);
        return NULL;
    }

    if (v == PURC_VARIANT_INVALID || !op || !handler) {
        pcinst_set_error(PCVARIANT_ERROR_WRONG_ARGS);
        return NULL;
    }

    if (!IS_CONTAINER(v->type)) {
        pcinst_set_error(PCVARIANT_ERROR_NOT_SUPPORTED);
        return NULL;
    }

    return register_listener(v, PCVAR_LISTENER_PRE, op, handler, ctxt);
}

struct pcvar_listener*
purc_variant_register_post_listener(purc_variant_t v,
        pcvar_op_t op, pcvar_op_handler handler, void *ctxt)
{
    if ((op & PCVAR_OPERATION_ALL) != op) {
        pcinst_set_error(PCVARIANT_ERROR_WRONG_ARGS);
        return NULL;
    }

    if (v == PURC_VARIANT_INVALID || !op || !handler) {
        pcinst_set_error(PCVARIANT_ERROR_WRONG_ARGS);
        return NULL;
    }

    if (!IS_CONTAINER(v->type)) {
        pcinst_set_error(PCVARIANT_ERROR_NOT_SUPPORTED);
        return NULL;
    }

    return register_listener(v, PCVAR_LISTENER_POST, op, handler, ctxt);
}

bool
purc_variant_revoke_listener(purc_variant_t v,
        struct pcvar_listener *listener)
{
    if (v == PURC_VARIANT_INVALID || !listener) {
        pcinst_set_error(PCVARIANT_ERROR_WRONG_ARGS);
        return false;
    }

    if (!IS_CONTAINER(v->type)) {
        pcinst_set_error(PCVARIANT_ERROR_NOT_SUPPORTED);
        return false;
    }

    struct list_head *listeners;
    listeners = &v->listeners;

    struct list_head *p, *n;
    list_for_each_safe(p, n, listeners) {
        struct pcvar_listener *curr;
        curr = container_of(p, struct pcvar_listener, list_node);
        if (curr != listener)
            continue;

        list_del(p);
        free(curr);
        return true;
    }

    return false;
}

bool pcvariant_on_pre_fired(
        purc_variant_t source,  // the source variant.
        pcvar_op_t op,          // the operation identifier.
        size_t nr_args,         // the number of the relevant child variants.
        purc_variant_t *argv    // the array of all relevant child variants.
        )
{
    op &= PCVAR_OPERATION_ALL;
    PC_ASSERT(op != PCVAR_OPERATION_ALL);

    struct list_head *listeners;
    listeners = &source->listeners;

    struct list_head *p, *n;
    list_for_each_safe(p, n, listeners) {
        struct pcvar_listener *curr;
        curr = container_of(p, struct pcvar_listener, list_node);
        if ((curr->op & op) == 0)
            continue;

        if ((curr->flags & PCVAR_LISTENER_PRE_OR_POST) != PCVAR_LISTENER_PRE)
            break;

        bool ok = curr->handler(source, op, curr->ctxt, nr_args, argv);
        if (!ok)
            return false;
    }

    return true;
}

void pcvariant_on_post_fired(
        purc_variant_t source,  // the source variant.
        pcvar_op_t op,          // the operation identifier.
        size_t nr_args,         // the number of the relevant child variants.
        purc_variant_t *argv    // the array of all relevant child variants.
        )
{
    op &= PCVAR_OPERATION_ALL;
    PC_ASSERT(op != PCVAR_OPERATION_ALL);

    struct list_head *listeners;
    listeners = &source->listeners;

    struct pcvar_listener *p, *n;
    list_for_each_entry_reverse_safe(p, n, listeners, list_node) {
        struct pcvar_listener *curr = p;
        PC_ASSERT(curr);
        if ((curr->op & op) == 0)
            continue;

        if ((curr->flags & PCVAR_LISTENER_PRE_OR_POST) == PCVAR_LISTENER_PRE)
            break;

        bool ok = curr->handler(source, op, curr->ctxt, nr_args, argv);
        PC_ASSERT(ok);
    }
}

void
pcvar_break_rue_downward(purc_variant_t val)
{
    PC_ASSERT(val != PURC_VARIANT_INVALID);
    switch (val->type) {
        case PURC_VARIANT_TYPE_ARRAY:
            if (pcvar_container_belongs_to_set(val))
                return;
            pcvar_array_break_rue_downward(val);
            return;
        case PURC_VARIANT_TYPE_OBJECT:
            if (pcvar_container_belongs_to_set(val))
                return;
            pcvar_object_break_rue_downward(val);
            return;
        case PURC_VARIANT_TYPE_TUPLE:
            if (pcvar_container_belongs_to_set(val))
                return;
            pcvar_tuple_break_rue_downward(val);
            return;
        case PURC_VARIANT_TYPE_SET:
        case PURC_VARIANT_TYPE_NULL:
        case PURC_VARIANT_TYPE_BOOLEAN:
        case PURC_VARIANT_TYPE_EXCEPTION:
        case PURC_VARIANT_TYPE_NUMBER:
        case PURC_VARIANT_TYPE_LONGINT:
        case PURC_VARIANT_TYPE_ULONGINT:
        case PURC_VARIANT_TYPE_LONGDOUBLE:
        case PURC_VARIANT_TYPE_ATOMSTRING:
        case PURC_VARIANT_TYPE_STRING:
        case PURC_VARIANT_TYPE_BSEQUENCE:
        case PURC_VARIANT_TYPE_DYNAMIC:
        case PURC_VARIANT_TYPE_NATIVE:
            return;
        default:
            PC_DEBUGX("%d", val->type);
            PC_ASSERT(0);
    }
}

void
pcvar_break_edge_to_parent(purc_variant_t val,
        struct pcvar_rev_update_edge *edge)
{
    PC_ASSERT(val != PURC_VARIANT_INVALID);
    if (pcvariant_is_mutable(val) == false)
        return;

    switch (val->type) {
        case PURC_VARIANT_TYPE_ARRAY:
            pcvar_array_break_edge_to_parent(val, edge);
            return;
        case PURC_VARIANT_TYPE_OBJECT:
            pcvar_object_break_edge_to_parent(val, edge);
            return;
        case PURC_VARIANT_TYPE_SET:
            pcvar_set_break_edge_to_parent(val, edge);
            return;
        case PURC_VARIANT_TYPE_TUPLE:
            pcvar_tuple_break_edge_to_parent(val, edge);
            return;
        default:
            PC_ASSERT(0);
    }
}

int
pcvar_build_rue_downward(purc_variant_t val)
{
    PC_ASSERT(val != PURC_VARIANT_INVALID);
    switch (val->type) {
        case PURC_VARIANT_TYPE_ARRAY:
            return pcvar_array_build_rue_downward(val);
        case PURC_VARIANT_TYPE_OBJECT:
            return pcvar_object_build_rue_downward(val);
        case PURC_VARIANT_TYPE_TUPLE:
            return pcvar_tuple_build_rue_downward(val);
        case PURC_VARIANT_TYPE_SET:
        case PURC_VARIANT_TYPE_NULL:
        case PURC_VARIANT_TYPE_BOOLEAN:
        case PURC_VARIANT_TYPE_EXCEPTION:
        case PURC_VARIANT_TYPE_NUMBER:
        case PURC_VARIANT_TYPE_LONGINT:
        case PURC_VARIANT_TYPE_ULONGINT:
        case PURC_VARIANT_TYPE_LONGDOUBLE:
        case PURC_VARIANT_TYPE_ATOMSTRING:
        case PURC_VARIANT_TYPE_STRING:
        case PURC_VARIANT_TYPE_BSEQUENCE:
        case PURC_VARIANT_TYPE_DYNAMIC:
        case PURC_VARIANT_TYPE_NATIVE:
            return 0;
        default:
            PC_DEBUGX("%d", val->type);
            PC_ASSERT(0);
            break;
    }

    return 0;
}

int
pcvar_build_edge_to_parent(purc_variant_t val,
        struct pcvar_rev_update_edge *edge)
{
    PC_ASSERT(val != PURC_VARIANT_INVALID);
    if (pcvariant_is_mutable(val) == false)
        return 0;

    switch (val->type) {
        case PURC_VARIANT_TYPE_ARRAY:
            return pcvar_array_build_edge_to_parent(val, edge);
        case PURC_VARIANT_TYPE_OBJECT:
            return pcvar_object_build_edge_to_parent(val, edge);
        case PURC_VARIANT_TYPE_SET:
            return pcvar_set_build_edge_to_parent(val, edge);
        case PURC_VARIANT_TYPE_TUPLE:
            return pcvar_tuple_build_edge_to_parent(val, edge);
        default:
            PC_ASSERT(0);
            break;
    }

    return -1;
}

static bool
is_rev_update_chain_empty(pcutils_map *chain)
{
    if (!chain)
        return true;

    size_t nr = pcutils_map_get_size(chain);
    return nr == 0 ? true : false;
}

bool
pcvar_container_belongs_to_set(purc_variant_t val)
{
    PC_ASSERT(val != PURC_VARIANT_INVALID);
    switch (val->type) {
        case PURC_VARIANT_TYPE_ARRAY:
            {
                variant_arr_t data = pcvar_arr_get_data(val);
                PC_ASSERT(data);
                if (is_rev_update_chain_empty(data->rev_update_chain))
                    return false;
                return true;
            }
        case PURC_VARIANT_TYPE_OBJECT:
            {
                variant_obj_t data = pcvar_obj_get_data(val);
                PC_ASSERT(data);
                if (is_rev_update_chain_empty(data->rev_update_chain))
                    return false;
                return true;
            }
        case PURC_VARIANT_TYPE_SET:
            {
                variant_set_t data = pcvar_set_get_data(val);
                PC_ASSERT(data);
                if (is_rev_update_chain_empty(data->rev_update_chain))
                    return false;
                return true;
            }
        case PURC_VARIANT_TYPE_TUPLE:
            {
                variant_tuple_t data = pcvar_tuple_get_data(val);
                PC_ASSERT(data);
                if (is_rev_update_chain_empty(data->rev_update_chain))
                    return false;
                return true;
            }
        default:
            return false;
    }
}

