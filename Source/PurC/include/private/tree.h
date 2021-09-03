/*
 * @file tree.h
 * @author XueShuming
 * @date 2021/07/07
 * @brief The interfaces vcm.
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

struct pctree_node {
    void* user_data;

    /* number of children */
    size_t nr_children;

    struct pctree_node* parent;
    struct pctree_node* first_child;
    struct pctree_node* last_child;
    struct pctree_node* prev;
    struct pctree_node* next;
};

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/*
 *
 * callback function for traverse node children
 *
 */
typedef void(pctree_node_for_each_fn)(struct pctree_node* node,  void* data);
typedef void (pctree_node_destroy_callback)(void* data);


/*
 * Creates a new node containing the given data
 *
 * @param user_data: the user data of the new node
 *
 * @return A new struct pctree_node on success, @NULL on failure and the error code
 *         is set to indicate the error. The error code:
 *  - @PURC_ERROR_INVALID_VALUE: Invalid value
 *  - @PURC_ERROR_OUT_OF_MEMORY: Out of memory
 *
 * Since: 0.0.1
 */
struct pctree_node* pctree_node_new (void* user_data);

/*
 * Removes root and its children from the tree, freeing any memory allocated.
 *
 * @param node: the root of the tree/subtree to destroy
 * @param callback: the function to call when the node is destroy.
 *                  This function will be called with the user_data and
 *                  can be used to free any memory allocated for it.
 *
 * Since: 0.0.1
 */
void pctree_node_destroy (struct pctree_node* node,
        pctree_node_destroy_callback callback);

/*
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
bool pctree_node_append_child (struct pctree_node* parent,
        struct pctree_node* node);

/*
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
bool pctree_node_prepend_child (struct pctree_node* parent,
        struct pctree_node* node);

/*
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
bool pctree_node_insert_before (struct pctree_node* current,
        struct pctree_node* node);

/*
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
bool pctree_node_insert_after (struct pctree_node* current,
        struct pctree_node* node);

/*
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
struct pctree_node* pctree_node_parent (struct pctree_node* node)
{
    return node->parent;
}

/*
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
struct pctree_node* pctree_node_child (struct pctree_node* node)
{
    return node->first_child;
}

/*
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
struct pctree_node* pctree_node_last_child (struct pctree_node* node)
{
    return node->last_child;
}

/*
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
struct pctree_node* pctree_node_next (struct pctree_node* node)
{
    return node->next;
}

/*
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
struct pctree_node* pctree_node_prev (struct pctree_node* node)
{
    return node->prev;
}

/*
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
size_t pctree_node_children_number (struct pctree_node* node)
{
    return node->nr_children;
}

/*
 * Gets the user_data of a node.
 *
 * @param node: the given node
 *
 * @return the user_data of node.
 *
 * Since: 0.0.1
 */
static inline
void* pctree_node_user_data (struct pctree_node* node)
{
    return node->user_data;
}


/*
 * Calls a function for each of the children of a node. Note that it doesn't
 * descend beneath the child nodes. func must not do anything that would
 * modify the structure of the tree.
 *
 * @param node: the given node
 * @param func: the function to call for each visited node
 * @param data: user data to pass to the function
 *
 * Since: 0.0.1
 */
void pctree_node_children_for_each (struct pctree_node* node,
        pctree_node_for_each_fn* func, void* data);

/*
 * Pre order traversal a tree starting at the given root node. It calls
 * the given function for each node visited. func must not do anything
 * that would modify the structure of the tree
 *
 * @param node: the given node
 * @param func: the function to call for each visited node
 * @param data: user data to pass to the function
 *
 * Since: 0.0.1
 */
void pctree_node_pre_order_traversal (struct pctree_node* node,
        pctree_node_for_each_fn* func, void* data);

/*
 * In order traversal a tree starting at the given root node. It calls
 * the given function for each node visited. func must not do anything
 * that would modify the structure of the tree
 *
 * @param node: the given node
 * @param func: the function to call for each visited node
 * @param data: user data to pass to the function
 *
 * Since: 0.0.1
 */
void pctree_node_in_order_traversal (struct pctree_node* node,
        pctree_node_for_each_fn* func, void* data);

/*
 * Post order traversal a tree starting at the given root node. It calls
 * the given function for each node visited. func must not do anything
 * that would modify the structure of the tree
 *
 * @param node: the given node
 * @param func: the function to call for each visited node
 * @param data: user data to pass to the function
 *
 * Since: 0.0.1
 */
void pctree_node_post_order_traversal (struct pctree_node* node,
        pctree_node_for_each_fn* func, void* data);

/*
 * Level order traversal a tree starting at the given root node. It calls
 * the given function for each node visited. func must not do anything
 * that would modify the structure of the tree
 *
 * @param node: the given node
 * @param func: the function to call for each visited node
 * @param data: user data to pass to the function
 *
 * Since: 0.0.1
 */
void pctree_node_level_order_traversal (struct pctree_node* node,
        pctree_node_for_each_fn* func, void* data);

/*
 * remove subtree from it's parent
 *
 * @param node: the given node which is the root of the subtree
 *
 * Since: 0.0.1
 */
void pctree_node_remove(struct pctree_node* node);


/*
 * Get first node under the specified tree in post_order
 * @param _node: root node of the specified tree, could be subtree
 */
#define pctree_first_post_order(_node)     \
    ({                                     \
        struct pctree_node *_p = _node;    \
        while (_p && _p->first_child) {    \
            _p = _p->first_child;          \
        };                                 \
        _p; })

/*
 * Get next node in post_order
 * @param _node: current node
 * @param _next: next node to return
 */
#define pctree_next_post_order(_node)                      \
    ({                                                     \
        struct pctree_node *_p;                            \
        if (_node->next) {                                 \
            _p = pctree_first_post_order(_node->next);     \
        } else {                                           \
            _p = _node->parent;                            \
        }                                                  \
        _p; })

/*
 * Loop over a block of nodes of the tree/subtree in post_order,
 * used similar to a for() command.
 * @param _top: top node of tree/subtree
 * @param _node: current node that is looped in this iterate
 * @param _next: next node that is to be looped,
 *               can be uninitialized before looping
 */
#define pctree_for_each_post_order(_top, _node, _next)              \
    for (_node = pctree_first_post_order(_top);                     \
         _node && ({_next = pctree_next_post_order(_node), 1;});    \
         _node = (_node==_top) ? NULL : _next)


/*
 * Get next node in pre_order
 * @param _node: current node
 * @param _next: next node to return
 * @param _top:  top node of tree/subtree that is currently looped for
 */
#define pctree_next_pre_order(_node, _top)            \
    ({                                                \
        struct pctree_node *_p = NULL;                \
        if (_node->first_child) {                     \
            _p = _node->first_child;                  \
        } else if (_node->next) {                     \
            if (_node==_top) {                        \
                _p = NULL;                            \
            } else {                                  \
                _p = _node->next;                     \
            }                                         \
        } else if (_node==_top) {                     \
            _p = NULL;                                \
        } else {                                      \
            _p = NULL;                                \
            struct pctree_node *_t = _node;           \
            while (_t->parent) {                      \
                if (_t->parent==_top)                 \
                    break;                            \
                _p = _t->parent->next;                \
                if (_p)                               \
                    break;                            \
                _t = _t->parent;                      \
            }                                         \
        }                                             \
        _p; })

/*
 * Loop over a block of nodes of the tree/subtree in pre_order,
 * used similar to a for() command.
 * @param _top: top node of tree/subtree
 * @param _node: current node that is looped in this iterate
 * @param _next: next node that is to be looped,
 *               can be uninitialized before looping
 */
#define pctree_for_each_pre_order(_top, _node, _next)                  \
    for (_node=_top,                                                   \
         _next=NULL;                                                   \
         _node && ({_next = pctree_next_pre_order(_node, _top), 1;});  \
         _node = _next)


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* not defined PURC_PRIVATE_TREE_H */

