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
        list_add_tail(&listener->list_node, listeners);
    } else {
        list_add(&listener->list_node, listeners);
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
            pcvar_array_break_rue_downward(val);
            return;
        case PURC_VARIANT_TYPE_OBJECT:
            pcvar_object_break_rue_downward(val);
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
        default:
            PC_ASSERT(0);
    }
}

void
pcvar_break_edge(purc_variant_t val, struct list_head *chain,
        struct pcvar_rev_update_edge *edge)
{
    PC_ASSERT(val != PURC_VARIANT_INVALID);
    PC_ASSERT(chain);
    PC_ASSERT(edge);

    struct list_head *p, *n;
    list_for_each_safe(p, n, chain) {
        struct pcvar_rev_update_edge_node *node;
        node = container_of(p, struct pcvar_rev_update_edge_node, node);
        if (edge->parent != node->edge.parent)
            continue;

        switch (edge->parent->type) {
            case PURC_VARIANT_TYPE_ARRAY:
                PC_ASSERT(edge->arr_me->val == val);
                if (edge->arr_me != node->edge.arr_me)
                    continue;
                list_del(p);
                free(node);
                return;

            case PURC_VARIANT_TYPE_OBJECT:
                PC_ASSERT(edge->obj_me->val == val);
                if (edge->obj_me != node->edge.obj_me)
                    continue;
                list_del(p);
                free(node);
                return;

            case PURC_VARIANT_TYPE_SET:
                PC_ASSERT(edge->set_me->elem == val);
                if (edge->set_me != node->edge.set_me)
                    continue;
                list_del(p);
                free(node);
                return;

            default:
                PC_ASSERT(0);
        }
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
    }
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
        default:
            PC_ASSERT(0);
    }
}

int
pcvar_build_edge(purc_variant_t val, struct list_head *chain,
        struct pcvar_rev_update_edge *edge)
{
    PC_ASSERT(val != PURC_VARIANT_INVALID);
    PC_ASSERT(chain);
    PC_ASSERT(edge);

    bool parent_is_set = purc_variant_is_set(edge->parent);
    PRINT_VARIANT(edge->parent);
    PRINT_VARIANT(val);
    if (!parent_is_set &&
            pcvar_container_belongs_to_set(edge->parent) == false)
    {
        PC_ASSERT(0);
    }

    if (list_empty(chain) == false)
        PC_ASSERT(0);

    struct list_head *p, *n;
    list_for_each_safe(p, n, chain) {
        struct pcvar_rev_update_edge_node *node;
        node = container_of(p, struct pcvar_rev_update_edge_node, node);
        if (edge->parent != node->edge.parent)
            continue;

        switch (edge->parent->type) {
            case PURC_VARIANT_TYPE_ARRAY:
                PC_ASSERT(edge->arr_me->val == val);
                if (edge->arr_me != node->edge.arr_me)
                    continue;
                return 0;

            case PURC_VARIANT_TYPE_OBJECT:
                PC_ASSERT(edge->obj_me->val == val);
                if (edge->obj_me != node->edge.obj_me)
                    continue;
                return 0;

            case PURC_VARIANT_TYPE_SET:
                PC_ASSERT(edge->set_me->elem == val);
                if (edge->set_me != node->edge.set_me)
                    continue;
                return 0;

            default:
                PC_ASSERT(0);
        }

        break;
    }

    struct pcvar_rev_update_edge_node *_new;
    _new = (struct pcvar_rev_update_edge_node*)calloc(1, sizeof(*_new));
    if (!_new) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }
    _new->edge = *edge;
    list_add_tail(&_new->node, chain);

    return 0;
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
                if (list_empty(&data->rev_update_chain) == false)
                    return true;
                return false;
            }
        case PURC_VARIANT_TYPE_OBJECT:
            {
                variant_obj_t data = pcvar_obj_get_data(val);
                PC_ASSERT(data);
                if (list_empty(&data->rev_update_chain) == false)
                    return true;
                return false;
            }
        case PURC_VARIANT_TYPE_SET:
            {
                variant_set_t data = pcvar_set_get_data(val);
                PC_ASSERT(data);
                if (list_empty(&data->rev_update_chain) == false)
                    return true;
                return false;
            }
        default:
            return false;
    }
}

