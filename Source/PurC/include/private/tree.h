/*
 * @file tree.h
 * @author XueShuming
 * @date 2021/07/07
 * @brief The interfaces for N-ary trees.
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

#ifndef PURC_PRIVATE_TREE_H
#define PURC_PRIVATE_TREE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define PURC_TREE_NODE_VCM_FUNC         0
#define PURC_TREE_NODE_VCM_VALUE        1
#define PURC_TREE_NODE_DOM_ELEMENT      2

typedef struct pctree_node {
    /* type of node */
    uint8_t type;

    /* number of children */
    size_t nr_children;

    struct pctree_node* parent;
    struct pctree_node* first_child;
    struct pctree_node* last_child;
    struct pctree_node* prev;
    struct pctree_node* next;
} pctree_node;

typedef struct pctree_node* pctree_node_t;

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 *
 * callback function for traverse node children
 *
 */
typedef void(pctree_node_for_each_fn)(pctree_node_t node,  void* data);

/**
 * Inserts a node as the last child of the given parent.
 *
 * @param parent: the node to place the new node under
 * @param node: the node to insert
 *
 * @return true on success, false on failure and the error code
 *         is set to indicate the error. The error code:
 *  - @PURC_ERROR_INVALID_VALUE: Invalid value
 *
 * Since: 0.0.1
 */
bool pctree_node_append_child (pctree_node_t parent,
        pctree_node_t node);

/**
 * Inserts a node as the first child of the given parent.
 *
 * @param parent: the node to place the new node under
 * @param node: the node to insert
 *
 * @return true on success, false on failure and the error code
 *         is set to indicate the error. The error code:
 *  - @PURC_ERROR_INVALID_VALUE: Invalid value
 *
 * Since: 0.0.1
 */
bool pctree_node_prepend_child (pctree_node_t parent,
        pctree_node_t node);

/**
 * Inserts a node before the given sibling.
 *
 * @param current: the sibling node to place node before.
 * @param node: the node to insert
 *
 * @return true on success, false on failure and the error code
 *         is set to indicate the error. The error code:
 *  - @PURC_ERROR_INVALID_VALUE: Invalid value
 *
 * Since: 0.0.1
 */
bool pctree_node_insert_before (pctree_node_t current,
        pctree_node_t node);

/**
 * Inserts a node after the given sibling.
 *
 * @param current: the sibling node to place node after.
 * @param node: the node to insert
 *
 * @return true on success, false on failure and the error code
 *         is set to indicate the error. The error code:
 *  - @PURC_ERROR_INVALID_VALUE: Invalid value
 *
 * Since: 0.0.1
 */
bool pctree_node_insert_after (pctree_node_t current,
        pctree_node_t node);

/**
 * Get the parent node of the given node.
 *
 * @param node: the given node
 *
 * @return the parent of node, NULL if node has no parent. The error code is
 *         set to indicate the error. The error code:
 *  - @PURC_ERROR_INVALID_VALUE: Invalid value
 *
 * Since: 0.0.1
 */
static inline
pctree_node_t pctree_node_parent (pctree_node_t node)
{
    return node->parent;
}

/**
 * Get first child node of the given node.
 *
 * @param node: the given node
 *
 * @return the first child node, NULL if node is NULL or has no children. The
 *         error code is set to indicate the error. The error code:
 *  - @PURC_ERROR_INVALID_VALUE: Invalid value
 *
 * Since: 0.0.1
 */
static inline
pctree_node_t pctree_node_child (pctree_node_t node)
{
    return node->first_child;
}

/**
 * Get last child node of the given node.
 *
 * @param node: the given node
 *
 * @return the last child of node, NULL if node has no children. The error code
 *         is set to indicate the error. The error code:
 *  - @PURC_ERROR_INVALID_VALUE: Invalid value
 *
 * Since: 0.0.1
 */
static inline
pctree_node_t pctree_node_last_child (pctree_node_t node)
{
    return node->last_child;
}

/**
 * Gets the next sibling of a node.
 *
 * @param node: the given node
 *
 * @return the next sibling of node, or NULL if node is the last node or NULL.
 *         The error code is set to indicate the error. The error code:
 *  - @PURC_ERROR_INVALID_VALUE: Invalid value
 *
 * Since: 0.0.1
 */
static inline
pctree_node_t pctree_node_next (pctree_node_t node)
{
    return node->next;
}

/**
 * Gets the previous sibling of a node.
 *
 * @param node: the given node
 *
 * @return the previous sibling of node, NULL if node is the first node or NULL.
 *         The error code is set to indicate the error. The error code:
 *  - @PURC_ERROR_INVALID_VALUE: Invalid value
 *
 * Since: 0.0.1
 */
static inline
pctree_node_t pctree_node_prev (pctree_node_t node)
{
    return node->prev;
}

/**
 * Gets the number of children of a node.
 *
 * @param node: the given node
 *
 * @return the number of children of node. The error code is set to indicate
 *         the error. The error code:
 *  - @PURC_ERROR_INVALID_VALUE: Invalid value
 *
 * Since: 0.0.1
 */
static inline
size_t pctree_node_children_number (pctree_node_t node)
{
    return node->nr_children;
}

/**
 * Gets the type of a node.
 *
 * @param node: the given node
 *
 * @return the type of node. The error code is set to indicate
 *         the error. The error code:
 *  - @PURC_ERROR_INVALID_VALUE: Invalid value
 *
 * Since: 0.0.1
 */
static inline
uint8_t pctree_node_type (pctree_node_t node)
{
    return node->type;
}


/**
 * Calls a function for each of the children of a node. Note that it doesn't
 * descend beneath the child nodes. func must not do anything that would
 * modify the structure of the tree.
 *
 * @param node: the given node
 * @param func: the function to call for each visited node
 * @param data: user data to pass to the function
 * Since: 0.0.1
 */
void pctree_node_children_for_each (pctree_node_t node,
        pctree_node_for_each_fn* func, void* data);

/**
 * Traverses a tree starting at the given root node. It calls the given
 * function for each node visited. func must not do anything that would
 * modify the structure of the tree
 *
 * @param node: the given node
 * @param func: the function to call for each visited node
 * @param data: user data to pass to the function
 * Since: 0.0.1
 */
void pctree_node_traverse (pctree_node_t node,
        pctree_node_for_each_fn* func, void* data);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* not defined PURC_PRIVATE_TREE_H */

