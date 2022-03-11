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
pcvar_break_rev_update_edges(purc_variant_t val)
{
    PC_ASSERT(val != PURC_VARIANT_INVALID);
    switch (val->type) {
        case PURC_VARIANT_TYPE_ARRAY:
            pcvar_array_break_rev_update_edges(val);
            return;
        case PURC_VARIANT_TYPE_OBJECT:
            pcvar_object_break_rev_update_edges(val);
            return;
        case PURC_VARIANT_TYPE_SET:
            return;
        default:
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
                break;

            case PURC_VARIANT_TYPE_OBJECT:
                PC_ASSERT(edge->obj_me->val == val);
                if (edge->obj_me != node->edge.obj_me)
                    continue;
                list_del(p);
                free(node);
                break;

            case PURC_VARIANT_TYPE_SET:
                PC_ASSERT(edge->set_me->elem == val);
                if (edge->set_me != node->edge.set_me)
                    continue;
                list_del(p);
                free(node);
                break;

            default:
                PC_ASSERT(0);
        }

        break;
    }

    if (list_empty(chain) == false)
        return;

    pcvar_break_rev_update_edges(val);
}

#if 0                      /* { */
struct edge_node {
    struct rb_node     **pnode;
    struct rb_node      *parent;
    struct rb_node      *entry;
};

static int
edge_cmp(struct pcvar_rev_update_edge *edge, purc_variant_t parent,
        purc_variant_t child)
{
    PC_ASSERT(parent != PURC_VARIANT_INVALID);
    PC_ASSERT(pcvariant_is_mutable(parent));

    if (edge->parent != parent)
        return edge->parent - parent;

    switch (parent->type) {
        case PURC_VARIANT_TYPE_ARRAY:
            return (edge->arr_child - (struct arr_node*)child_node);
        case PURC_VARIANT_TYPE_OBJECT:
            return (edge->obj_child - (struct obj_node*)child_node);
        case PURC_VARIANT_TYPE_SET:
            return (edge->set_child - (struct elem_node*)child_node);
        default:
            PC_ASSERT(0);
            return 0; // never reached here
    }
}

static void
find_edge_node(struct edge_node *node, struct rb_root *root,
        purc_variant_t v_parent, purc_variant_t v_child)
{
    struct rb_node **pnode = &root->rb_node;
    struct rb_node *parent = NULL;
    struct rb_node *entry = NULL;
    while (*pnode) {
        struct pcvar_rev_update_edge *on;
        on = container_of(*pnode, struct pcvar_rev_update_edge, node);
        int ret = edge_cmp(on, v_parent, v_child);

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

    node->pnode  = pnode;
    node->parent = parent;
    node->entry  = entry;
}
#endif                     /* } */

