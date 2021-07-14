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
#include <stdbool.h>

#define PURC_TREE_NODE_VCM_FUNC         0
#define PURC_TREE_NODE_VCM_VALUE        1
#define PURC_TREE_NODE_DOM_ELEMENT      2

typedef struct purc_tree_node {
    /* type of node */
    unsigned int type:8;

    /* number of children */
    size_t nr_children;

    struct purc_tree_node* parent;
    struct purc_tree_node* child;
    struct purc_tree_node* prev;
    struct purc_tree_node* next;
} purc_tree_node;

typedef struct purc_tree_node* purc_tree_node_t;

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 *
 * callback function for traverse node children
 *
 */
typedef void(purc_tree_node_for_each_fn)(purc_tree_node_t node,  void* data);

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
bool purc_tree_node_append_child (purc_tree_node_t parent,
        purc_tree_node_t node);

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
bool purc_tree_node_prepend_child (purc_tree_node_t parent,
        purc_tree_node_t node);

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
bool purc_tree_node_insert_before (purc_tree_node_t current,
        purc_tree_node_t node);

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
bool purc_tree_node_insert_after (purc_tree_node_t current,
        purc_tree_node_t node);

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
purc_tree_node_t purc_tree_node_parent (purc_tree_node_t node);

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
purc_tree_node_t purc_tree_node_child (purc_tree_node_t node);

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
purc_tree_node_t purc_tree_node_last_child (purc_tree_node_t node);

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
purc_tree_node_t purc_tree_node_next (purc_tree_node_t node);

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
purc_tree_node_t purc_tree_node_prev (purc_tree_node_t node);

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
size_t purc_tree_node_children_number (purc_tree_node_t node);

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
unsigned int purc_tree_node_type (purc_tree_node_t node);


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
void purc_tree_node_children_for_each (purc_tree_node_t node,
        purc_tree_node_for_each_fn* func, void* data);

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
void purc_tree_node_traverse (purc_tree_node_t node,
        purc_tree_node_for_each_fn* func, void* data);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* not defined PURC_PRIVATE_TREE_H */

