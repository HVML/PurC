/**
 * @file avl.c
 * @author 
 * @date 2021/07/02
 * @brief The complementation of html utils.
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

#include "private/errors.h"

#include "html/core/avl.h"


static inline short
pchtml_avl_node_height(pchtml_avl_node_t *node);

static inline short
pchtml_avl_node_balance_factor(pchtml_avl_node_t *node);

static inline void
pchtml_avl_node_set_height(pchtml_avl_node_t *node);

static pchtml_avl_node_t *
pchtml_avl_node_rotate_right(pchtml_avl_node_t *pos);

static pchtml_avl_node_t *
pchtml_avl_node_rotate_left(pchtml_avl_node_t *pos);

static pchtml_avl_node_t *
pchtml_avl_node_balance(pchtml_avl_node_t *node,
                             pchtml_avl_node_t **scope);

static inline pchtml_avl_node_t *
pchtml_avl_find_min(pchtml_avl_node_t *node);

static inline void
pchtml_avl_rotate_for_delete(pchtml_avl_node_t *delete_node,
                             pchtml_avl_node_t *node,
                             pchtml_avl_node_t **root);


pchtml_avl_t *
pchtml_avl_create(void)
{
    return pchtml_calloc(1, sizeof(pchtml_avl_t));
}

unsigned int
pchtml_avl_init(pchtml_avl_t *avl, size_t chunk_len)
{
    if (avl == NULL) {
        return PCHTML_STATUS_ERROR_OBJECT_IS_NULL;
    }

    if (chunk_len == 0) {
        return PCHTML_STATUS_ERROR_WRONG_ARGS;
    }

    avl->nodes = pchtml_dobject_create();
    return pchtml_dobject_init(avl->nodes,
                               chunk_len, sizeof(pchtml_avl_node_t));
}

void
pchtml_avl_clean(pchtml_avl_t *avl)
{
    pchtml_dobject_clean(avl->nodes);
}

pchtml_avl_t *
pchtml_avl_destroy(pchtml_avl_t *avl, bool self_destroy)
{
    if (avl == NULL)
        return NULL;

    avl->nodes = pchtml_dobject_destroy(avl->nodes, true);

    if (self_destroy) {
        return pchtml_free(avl);
    }

    return avl;
}

pchtml_avl_node_t *
pchtml_avl_node_make(pchtml_avl_t *avl, size_t type, void *value)
{
    pchtml_avl_node_t *node = pchtml_dobject_calloc(avl->nodes);
    if (node == NULL) {
        return NULL;
    }

    node->type = type;
    node->value = value;

    return node;
}

void
pchtml_avl_node_clean(pchtml_avl_node_t *node)
{
    memset(node, 0, sizeof(pchtml_avl_node_t));
}

pchtml_avl_node_t *
pchtml_avl_node_destroy(pchtml_avl_t *avl,
                        pchtml_avl_node_t *node, bool self_destroy)
{
    if (node == NULL) {
        return NULL;
    }

    if (self_destroy) {
        return pchtml_dobject_free(avl->nodes, node);
    }

    return node;
}

static inline short
pchtml_avl_node_height(pchtml_avl_node_t *node)
{
    return (node) ? node->height : 0;
}

static inline short
pchtml_avl_node_balance_factor(pchtml_avl_node_t *node)
{
    return (pchtml_avl_node_height(node->right)
            - pchtml_avl_node_height(node->left));
}

static inline void
pchtml_avl_node_set_height(pchtml_avl_node_t *node)
{
    short left_height = pchtml_avl_node_height(node->left);
    short right_height = pchtml_avl_node_height(node->right);

    node->height = ((left_height > right_height)
                    ? left_height : right_height) + 1;
}

static pchtml_avl_node_t *
pchtml_avl_node_rotate_right(pchtml_avl_node_t *pos)
{
    pchtml_avl_node_t *node = pos->left;

    node->parent = pos->parent;

    if (node->right) {
        node->right->parent = pos;
    }

    pos->left   = node->right;
    pos->parent = node;

    node->right = pos;

    pchtml_avl_node_set_height(pos);
    pchtml_avl_node_set_height(node);

    return node;
}

static pchtml_avl_node_t *
pchtml_avl_node_rotate_left(pchtml_avl_node_t *pos)
{
    pchtml_avl_node_t *node = pos->right;

    node->parent = pos->parent;

    if (node->left) {
        node->left->parent = pos;
    }

    pos->right  = node->left;
    pos->parent = node;

    node->left = pos;

    pchtml_avl_node_set_height(pos);
    pchtml_avl_node_set_height(node);

    return node;
}

static pchtml_avl_node_t *
pchtml_avl_node_balance(pchtml_avl_node_t *node, pchtml_avl_node_t **scope)
{
    /* Set height */
    pchtml_avl_node_t *parent;

    short left_height = pchtml_avl_node_height(node->left);
    short right_height = pchtml_avl_node_height(node->right);

    node->height = ((left_height > right_height)
                    ? left_height : right_height) + 1;

    /* Check balance */
    switch ((right_height - left_height)) {
        case 2: {
            if (pchtml_avl_node_balance_factor(node->right) < 0) {
                node->right = pchtml_avl_node_rotate_right(node->right);
            }

            parent = node->parent;

            if (parent != NULL) {
                if (parent->right == node) {
                    parent->right = pchtml_avl_node_rotate_left(node);
                    return parent->right;
                }
                else {
                    parent->left = pchtml_avl_node_rotate_left(node);
                    return parent->left;
                }
            }

            return pchtml_avl_node_rotate_left(node);
        }
        case -2: {
            if (pchtml_avl_node_balance_factor(node->left) > 0) {
                node->left = pchtml_avl_node_rotate_left(node->left);
            }

            parent = node->parent;

            if (parent != NULL) {
                if (parent->right == node) {
                    parent->right = pchtml_avl_node_rotate_right(node);
                    return parent->right;
                }
                else {
                    parent->left = pchtml_avl_node_rotate_right(node);
                    return parent->left;
                }
            }

            return pchtml_avl_node_rotate_right(node);
        }
        default:
            break;
    }

    if (node->parent == NULL) {
        *scope = node;
    }

    return node->parent;
}

pchtml_avl_node_t *
pchtml_avl_insert(pchtml_avl_t *avl, pchtml_avl_node_t **scope,
                  size_t type, void *value)
{
    pchtml_avl_node_t *node, *new_node;

    if (*scope == NULL) {
        *scope = pchtml_avl_node_make(avl, type, value);
        return *scope;
    }

    node = *scope;
    new_node = pchtml_dobject_calloc(avl->nodes);

    for (;;) {
        if (type == node->type) {
            node->value = value;
            return node;
        }
        else if (type < node->type) {
            if (node->left == NULL) {
                node->left = new_node;

                new_node->parent = node;
                new_node->type   = type;
                new_node->value  = value;

                node = new_node;
                break;
            }

            node = node->left;
        }
        else {
            if (node->right == NULL) {
                node->right = new_node;

                new_node->parent = node;
                new_node->type   = type;
                new_node->value  = value;

                node = new_node;
                break;
            }

            node = node->right;
        }
    }

    while (node != NULL) {
        node = pchtml_avl_node_balance(node, scope);
    }

    return new_node;
}

static inline pchtml_avl_node_t *
pchtml_avl_find_min(pchtml_avl_node_t *node)
{
    if (node == NULL) {
        return NULL;
    }

    while (node->right != NULL) {
        node = node->right;
    }

    return node;
}

static inline void
pchtml_avl_rotate_for_delete(pchtml_avl_node_t *delete_node,
                             pchtml_avl_node_t *node, pchtml_avl_node_t **scope)
{
    pchtml_avl_node_t *balance_node;

    if (node) {
        if (delete_node->left == node) {
            balance_node = (node->left) ? node->left : node;

            node->parent = delete_node->parent;
            node->right  = delete_node->right;

            if (delete_node->right)
                delete_node->right->parent = node;
        }
        else {
            balance_node = node;

            node->parent->right = NULL;

            node->parent = delete_node->parent;
            node->right  = delete_node->right;
            node->left   = delete_node->left;

            if (delete_node->left != NULL) {
                delete_node->left->parent = node;
            }

            if (delete_node->right != NULL) {
                delete_node->right->parent = node;
            }
        }

        if (delete_node->parent != NULL) {
            if (delete_node->parent->left == delete_node) {
                delete_node->parent->left = node;
            }
            else {
                delete_node->parent->right = node;
            }
        }
        else {
            *scope = node;
        }
    }
    else {
        balance_node = delete_node->parent;

        if (delete_node->parent != NULL) {
            if (delete_node->parent->left == delete_node) {
                delete_node->parent->left = delete_node->right;
            }
            else {
                delete_node->parent->right = delete_node->right;
            }
        }
        else {
            *scope = delete_node->right;
        }
    }

    while (balance_node != NULL) {
        balance_node = pchtml_avl_node_balance(balance_node, scope);
    }
}

void *
pchtml_avl_remove(pchtml_avl_t *avl, pchtml_avl_node_t **scope, size_t type)
{
    pchtml_avl_node_t *node = *scope;

    while (node != NULL) {
        if (type == node->type) {
            pchtml_avl_rotate_for_delete(node,
                                        pchtml_avl_find_min(node->left), scope);

            void *value = node->value;

            pchtml_dobject_free(avl->nodes, node);

            return value;
        }
        else if (type < node->type) {
            node = node->left;
        }
        else {
            node = node->right;
        }
    }

    return NULL;
}

pchtml_avl_node_t *
pchtml_avl_search(pchtml_avl_t *avl, pchtml_avl_node_t *node, size_t type)
{
    UNUSED_PARAM(avl);

    while (node != NULL) {
        if (type == node->type) {
            return node;
        }
        else if (type < node->type) {
            node = node->left;
        }
        else {
            node = node->right;
        }
    }

    return NULL;
}

void
pchtml_avl_foreach_recursion(pchtml_avl_t *avl, pchtml_avl_node_t *scope,
                             pchtml_avl_node_f callback, void *ctx)
{
    if (scope == NULL) {
        return;
    }

    callback(scope, ctx);

    pchtml_avl_foreach_recursion(avl, scope->left, callback, ctx);
    pchtml_avl_foreach_recursion(avl, scope->right, callback, ctx);
}
