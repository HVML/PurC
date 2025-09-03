/*
 * @file tree.c
 * @author XueShuming
 * @date 2021/07/02
 * @brief The API for tree.
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

#include "private/tree.h"
#include "private/errors.h"
#include "config.h"

#if HAVE(GLIB)
#include <gmodule.h>
#else
#include <stdlib.h>
#endif

#if HAVE(GLIB)
static inline UNUSED_FUNCTION struct pctree_node* alloc_pctree_node(void) {
    return (struct pctree_node*)g_slice_alloc(sizeof(struct pctree_node));
}

static inline UNUSED_FUNCTION struct pctree_node* alloc_pctree_node_0(void) {
    return (struct pctree_node*)g_slice_alloc0(sizeof(struct pctree_node));
}

static inline void free_pctree_node(struct pctree_node* v) {
    return g_slice_free1(sizeof(struct pctree_node), (gpointer)v);
}
#else
static inline UNUSED_FUNCTION struct pctree_node* alloc_pctree_node(void) {
    return (struct pctree_node*)malloc(sizeof(struct pctree_node));
}

static inline UNUSED_FUNCTION struct pctree_node* alloc_pctree_node_0(void) {
    return (struct pctree_node*)calloc(1, sizeof(struct pctree_node));
}

static inline void free_pctree_node(struct pctree_node* v) {
    return free(v);
}
#endif

struct pctree_node* pctree_node_new (void* user_data)
{
    struct pctree_node* node = alloc_pctree_node_0 ();
    if (node) {
        node->user_data = user_data;
    }
    return node;
}

void pctree_node_destroy (struct pctree_node* node,
        pctree_node_destroy_callback callback)
{
    while (node)
    {
        struct pctree_node* next = node->next;
        if (node->first_child) {
            pctree_node_destroy (node->first_child, callback);
        }
        if (callback) {
            callback (node->user_data);
        }
        free_pctree_node (node);
        node = next;
    }
}

bool pctree_node_append_child (struct pctree_node* parent,
        struct pctree_node* node)
{
    parent->nr_children++;
    node->parent = parent;
    if (parent->last_child) {
        node->prev = parent->last_child;
        parent->last_child->next = node;
    }
    else {
        parent->first_child = node;
    }
    parent->last_child = node;
    return true;
}

bool pctree_node_prepend_child (struct pctree_node* parent,
        struct pctree_node* node)
{
    parent->nr_children++;
    node->parent = parent;
    if (parent->first_child) {
        node->next = parent->first_child;
        parent->first_child->prev = node;
    }
    else {
        parent->last_child = node;
    }
    parent->first_child = node;
    return true;
}

bool pctree_node_insert_before (struct pctree_node* current,
        struct pctree_node* node)
{
    node->parent = current->parent;
    node->prev = current->prev;
    if (node->parent) {
        node->parent->nr_children++;
    }

    if (current->prev) {
        node->prev->next = node;
    }
    else if (node->parent) {
        node->parent->first_child = node;
    }
    node->next = current;
    current->prev = node;
    return true;
}

bool pctree_node_insert_after (struct pctree_node* current,
        struct pctree_node* node)
{
    node->parent = current->parent;
    node->parent->nr_children++;
    if (current->next) {
        current->next->prev = node;
    }
    else {
        node->parent->last_child = node;
    }
    node->next = current->next;
    node->prev = current;
    current->next = node;
    return true;
}

void pctree_node_children_for_each (struct pctree_node* node,
        pctree_node_for_each_fn* func, void* data)
{
    node = node->first_child;
    while (node) {
        struct pctree_node* current = node;
        node = current->next;
        func (current, data);
    }
}

void pctree_node_pre_order_traversal (struct pctree_node* node,
        pctree_node_for_each_fn* func, void* data)
{
    func (node, data);
    node = node->first_child;
    while (node) {
        struct pctree_node* current = node;
        node = current->next;
        pctree_node_pre_order_traversal (current, func, data);
    }
}

void pctree_node_in_order_traversal (struct pctree_node* node,
        pctree_node_for_each_fn* func, void* data)
{
    if (node->first_child)
    {
        struct pctree_node* child = node->first_child;
        struct pctree_node* current = child;
        child = current->next;
        pctree_node_in_order_traversal(current, func, data);
        func (node, data);
        while (child)
        {
            current = child;
            child = current->next;
            pctree_node_in_order_traversal(current, func, data);
        }
    }
    else {
        func (node, data);
    }
}

void pctree_node_post_order_traversal (struct pctree_node* node,
        pctree_node_for_each_fn* func, void* data)
{
    if (node->first_child)
    {
        struct pctree_node* child = node->first_child;
        struct pctree_node* current = NULL;
        while (child)
        {
            current = child;
            child = current->next;
            pctree_node_post_order_traversal(current, func, data);
        }
        func(node, data);
    }
    else {
        func (node, data);
    }
}

static void pctree_traverse_level (struct pctree_node* node,
        pctree_node_for_each_fn* func, void* data, size_t level,
        bool* more_levels)
{
    if (level == 0)
    {
        if (node->first_child)
        {
            *more_levels = true;
            func(node, data);
        }
        else
        {
            func(node, data);
        }
    }
    else
    {
        node = node->first_child;
        while (node)
        {
            pctree_traverse_level (node, func, data, level - 1, more_levels);
            node = node->next;
        }
    }
}

void pctree_node_level_order_traversal (struct pctree_node* node,
        pctree_node_for_each_fn* func, void* data)
{
    size_t level = 0;
    bool more_levels = false;
    while (true) {
        more_levels = false;
        pctree_traverse_level (node, func, data, level, &more_levels);
        if (!more_levels) {
            break;
        }
        level++;
    }
}

void pctree_node_remove(struct pctree_node* node)
{
    struct pctree_node *parent = node->parent;
    struct pctree_node *next   = node->next;
    struct pctree_node *prev   = node->prev;

    if (parent==NULL)
        return;

    if (next)
        next->prev = prev;
    else
        parent->last_child = prev;

    if (prev)
        prev->next = next;
    else
        parent->first_child = next;

    node->parent = NULL;
    node->next   = NULL;
    node->prev   = NULL;

    parent->nr_children--;
}

static void
node_walk(struct pctree_node *node, int level,
        pctree_node_walk_cb cb, void *ctxt)
{
    cb(node, level, 1, ctxt);
    struct pctree_node *child = node->first_child;
    while (child) {
        struct pctree_node* next = child->next;
        node_walk(child, level+1, cb, ctxt);
        child = next;
    }
    cb(node, level, 0, ctxt);
}

void
pctree_node_walk(struct pctree_node *node, int level,
        pctree_node_walk_cb cb, void *ctxt)
{
    if (!node)
        return;

    node_walk(node, level, cb, ctxt);
}

