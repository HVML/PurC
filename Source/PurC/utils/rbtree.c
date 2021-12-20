/*
 * @file rbtree.c
 * @author VincentWei
 * @date 2021/07/08
 * @brief The implementation of Red-Black Tree.
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

#include "private/rbtree.h"

#include <stdio.h>

static void __rb_rotate_left(struct rb_node *node, struct rb_root *root)
{
    struct rb_node *right = node->rb_right;
    struct rb_node *parent = pcutils_rbtree_parent(node);

    if ((node->rb_right = right->rb_left))
        pcutils_rbtree_set_parent(right->rb_left, node);
    right->rb_left = node;

    pcutils_rbtree_set_parent(right, parent);

    if (parent)
    {
        if (node == parent->rb_left)
            parent->rb_left = right;
        else
            parent->rb_right = right;
    }
    else
        root->rb_node = right;
    pcutils_rbtree_set_parent(node, right);
}

static void __rb_rotate_right(struct rb_node *node, struct rb_root *root)
{
    struct rb_node *left = node->rb_left;
    struct rb_node *parent = pcutils_rbtree_parent(node);

    if ((node->rb_left = left->rb_right))
        pcutils_rbtree_set_parent(left->rb_right, node);
    left->rb_right = node;

    pcutils_rbtree_set_parent(left, parent);

    if (parent)
    {
        if (node == parent->rb_right)
            parent->rb_right = left;
        else
            parent->rb_left = left;
    }
    else
        root->rb_node = left;
    pcutils_rbtree_set_parent(node, left);
}

void pcutils_rbtree_insert_color(struct rb_node *node, struct rb_root *root)
{
    struct rb_node *parent, *gparent;

    while ((parent = pcutils_rbtree_parent(node)) && pcutils_rbtree_is_red(parent))
    {
        gparent = pcutils_rbtree_parent(parent);

        if (parent == gparent->rb_left)
        {
            {
                register struct rb_node *uncle = gparent->rb_right;
                if (uncle && pcutils_rbtree_is_red(uncle))
                {
                    pcutils_rbtree_set_black(uncle);
                    pcutils_rbtree_set_black(parent);
                    pcutils_rbtree_set_red(gparent);
                    node = gparent;
                    continue;
                }
            }

            if (parent->rb_right == node)
            {
                register struct rb_node *tmp;
                __rb_rotate_left(parent, root);
                tmp = parent;
                parent = node;
                node = tmp;
            }

            pcutils_rbtree_set_black(parent);
            pcutils_rbtree_set_red(gparent);
            __rb_rotate_right(gparent, root);
        } else {
            {
                register struct rb_node *uncle = gparent->rb_left;
                if (uncle && pcutils_rbtree_is_red(uncle))
                {
                    pcutils_rbtree_set_black(uncle);
                    pcutils_rbtree_set_black(parent);
                    pcutils_rbtree_set_red(gparent);
                    node = gparent;
                    continue;
                }
            }

            if (parent->rb_left == node)
            {
                register struct rb_node *tmp;
                __rb_rotate_right(parent, root);
                tmp = parent;
                parent = node;
                node = tmp;
            }

            pcutils_rbtree_set_black(parent);
            pcutils_rbtree_set_red(gparent);
            __rb_rotate_left(gparent, root);
        }
    }

    pcutils_rbtree_set_black(root->rb_node);
}

static void __rb_erase_color(struct rb_node *node, struct rb_node *parent,
                 struct rb_root *root)
{
    struct rb_node *other;

    while ((!node || pcutils_rbtree_is_black(node)) && node != root->rb_node)
    {
        if (parent->rb_left == node)
        {
            other = parent->rb_right;
            if (pcutils_rbtree_is_red(other))
            {
                pcutils_rbtree_set_black(other);
                pcutils_rbtree_set_red(parent);
                __rb_rotate_left(parent, root);
                other = parent->rb_right;
            }
            if ((!other->rb_left || pcutils_rbtree_is_black(other->rb_left)) &&
                (!other->rb_right || pcutils_rbtree_is_black(other->rb_right)))
            {
                pcutils_rbtree_set_red(other);
                node = parent;
                parent = pcutils_rbtree_parent(node);
            }
            else
            {
                if (!other->rb_right || pcutils_rbtree_is_black(other->rb_right))
                {
                    pcutils_rbtree_set_black(other->rb_left);
                    pcutils_rbtree_set_red(other);
                    __rb_rotate_right(other, root);
                    other = parent->rb_right;
                }
                pcutils_rbtree_set_color(other, pcutils_rbtree_color(parent));
                pcutils_rbtree_set_black(parent);
                pcutils_rbtree_set_black(other->rb_right);
                __rb_rotate_left(parent, root);
                node = root->rb_node;
                break;
            }
        }
        else
        {
            other = parent->rb_left;
            if (pcutils_rbtree_is_red(other))
            {
                pcutils_rbtree_set_black(other);
                pcutils_rbtree_set_red(parent);
                __rb_rotate_right(parent, root);
                other = parent->rb_left;
            }
            if ((!other->rb_left || pcutils_rbtree_is_black(other->rb_left)) &&
                (!other->rb_right || pcutils_rbtree_is_black(other->rb_right)))
            {
                pcutils_rbtree_set_red(other);
                node = parent;
                parent = pcutils_rbtree_parent(node);
            }
            else
            {
                if (!other->rb_left || pcutils_rbtree_is_black(other->rb_left))
                {
                    pcutils_rbtree_set_black(other->rb_right);
                    pcutils_rbtree_set_red(other);
                    __rb_rotate_left(other, root);
                    other = parent->rb_left;
                }
                pcutils_rbtree_set_color(other, pcutils_rbtree_color(parent));
                pcutils_rbtree_set_black(parent);
                pcutils_rbtree_set_black(other->rb_left);
                __rb_rotate_right(parent, root);
                node = root->rb_node;
                break;
            }
        }
    }
    if (node)
        pcutils_rbtree_set_black(node);
}

void pcutils_rbtree_erase(struct rb_node *node, struct rb_root *root)
{
    struct rb_node *child, *parent;
    int color;

    if (!node->rb_left)
        child = node->rb_right;
    else if (!node->rb_right)
        child = node->rb_left;
    else
    {
        struct rb_node *old = node, *left;

        node = node->rb_right;
        while ((left = node->rb_left) != NULL)
            node = left;

        if (pcutils_rbtree_parent(old)) {
            if (pcutils_rbtree_parent(old)->rb_left == old)
                pcutils_rbtree_parent(old)->rb_left = node;
            else
                pcutils_rbtree_parent(old)->rb_right = node;
        } else
            root->rb_node = node;

        child = node->rb_right;
        parent = pcutils_rbtree_parent(node);
        color = pcutils_rbtree_color(node);

        if (parent == old) {
            parent = node;
        } else {
            if (child)
                pcutils_rbtree_set_parent(child, parent);
            parent->rb_left = child;

            node->rb_right = old->rb_right;
            pcutils_rbtree_set_parent(old->rb_right, node);
        }

        node->rb_color = old->rb_color;
        node->rb_parent = old->rb_parent;
        node->rb_left = old->rb_left;
        pcutils_rbtree_set_parent(old->rb_left, node);

        old->rb_left = NULL;
        old->rb_right = NULL;

        goto color;
    }

    parent = pcutils_rbtree_parent(node);
    color = pcutils_rbtree_color(node);

    if (child)
        pcutils_rbtree_set_parent(child, parent);
    if (parent)
    {
        if (parent->rb_left == node)
            parent->rb_left = child;
        else
            parent->rb_right = child;
    }
    else
        root->rb_node = child;

    node->rb_left = NULL;
    node->rb_right = NULL;

 color:
    if (color == RB_BLACK)
        __rb_erase_color(child, parent, root);
}

static void pcutils_rbtree_augment_path(struct rb_node *node, rb_augment_f func, void *data)
{
    struct rb_node *parent;

up:
    func(node, data);
    parent = pcutils_rbtree_parent(node);
    if (!parent)
        return;

    if (node == parent->rb_left && parent->rb_right)
        func(parent->rb_right, data);
    else if (parent->rb_left)
        func(parent->rb_left, data);

    node = parent;
    goto up;
}

/*
 * after inserting @node into the tree, update the tree to account for
 * both the new entry and any damage done by rebalance
 */
void pcutils_rbtree_augment_insert(struct rb_node *node, rb_augment_f func, void *data)
{
    if (node->rb_left)
        node = node->rb_left;
    else if (node->rb_right)
        node = node->rb_right;

    pcutils_rbtree_augment_path(node, func, data);
}

/*
 * before removing the node, find the deepest node on the rebalance path
 * that will still be there after @node gets removed
 */
struct rb_node *rb_augment_erase_begin(struct rb_node *node)
{
    struct rb_node *deepest;

    if (!node->rb_right && !node->rb_left)
        deepest = pcutils_rbtree_parent(node);
    else if (!node->rb_right)
        deepest = node->rb_left;
    else if (!node->rb_left)
        deepest = node->rb_right;
    else {
        deepest = pcutils_rbtree_next(node);
        if (deepest->rb_right)
            deepest = deepest->rb_right;
        else if (pcutils_rbtree_parent(deepest) != node)
            deepest = pcutils_rbtree_parent(deepest);
    }

    return deepest;
}

/*
 * after removal, update the tree to account for the removed entry
 * and any rebalance damage.
 */
void pcutils_rbtree_augment_erase_end(struct rb_node *node, rb_augment_f func, void *data)
{
    if (node)
        pcutils_rbtree_augment_path(node, func, data);
}

/*
 * This function returns the first node (in sort order) of the tree.
 */
struct rb_node *pcutils_rbtree_first(const struct rb_root *root)
{
    struct rb_node    *n;

    n = root->rb_node;
    if (!n)
        return NULL;
    while (n && n->rb_left)
        n = n->rb_left;
    return n;
}

struct rb_node *pcutils_rbtree_last(const struct rb_root *root)
{
    struct rb_node    *n;

    n = root->rb_node;
    if (!n)
        return NULL;
    while (n && n->rb_right)
        n = n->rb_right;
    return n;
}

struct rb_node *pcutils_rbtree_next(const struct rb_node *node)
{
    struct rb_node *parent;

    if (pcutils_rbtree_parent(node) == node)
        return NULL;

    /* If we have a right-hand child, go down and then left as far
       as we can. */
    if (node->rb_right) {
        node = node->rb_right;
        while (node->rb_left)
            node=node->rb_left;
        return (struct rb_node *)node;
    }

    /* No right-hand children.  Everything down and left is
       smaller than us, so any 'next' node must be in the general
       direction of our parent. Go up the tree; any time the
       ancestor is a right-hand child of its parent, keep going
       up. First time it's a left-hand child of its parent, said
       parent is our 'next' node. */
    while ((parent = pcutils_rbtree_parent(node)) && node == parent->rb_right)
        node = parent;

    return parent;
}

struct rb_node *pcutils_rbtree_prev(const struct rb_node *node)
{
    struct rb_node *parent;

    if (pcutils_rbtree_parent(node) == node)
        return NULL;

    /* If we have a left-hand child, go down and then right as far
       as we can. */
    if (node->rb_left) {
        node = node->rb_left;
        while (node->rb_right)
            node=node->rb_right;
        return (struct rb_node *)node;
    }

    /* No left-hand children. Go up till we find an ancestor which
       is a right-hand child of its parent */
    while ((parent = pcutils_rbtree_parent(node)) && node == parent->rb_left)
        node = parent;

    return parent;
}

void pcutils_rbtree_replace_node(struct rb_node *victim, struct rb_node *newnode,
             struct rb_root *root)
{
    struct rb_node *parent = pcutils_rbtree_parent(victim);

    /* Set the surrounding nodes to point to the replacement */
    if (parent) {
        if (victim == parent->rb_left)
            parent->rb_left = newnode;
        else
            parent->rb_right = newnode;
    } else {
        root->rb_node = newnode;
    }
    if (victim->rb_left)
        pcutils_rbtree_set_parent(victim->rb_left, newnode);
    if (victim->rb_right)
        pcutils_rbtree_set_parent(victim->rb_right, newnode);

    /* Copy the pointers/colour from the victim to the replacement */
    *newnode = *victim;
}

static inline int
rbtree_traverse(struct rb_node *node, void *ud,
        int (*cb)(struct rb_node *node, void *ud))
{
    while (node) {
        int r = cb(node, ud);
        if (r)
            return r;
        node = pcutils_rbtree_next(node);
    }
    return 0;
}

int pcutils_rbtree_traverse(struct rb_root *root, void *ud,
        int (*cb)(struct rb_node *node, void *ud))
{
    struct rb_node *node = pcutils_rbtree_first(root);

    return rbtree_traverse(node, ud, cb);
}

