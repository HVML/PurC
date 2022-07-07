/*
 * @file rbtree.h
 * @author Vincent Wei (https://github.com/VincentWei)
 * @date 2021/07/08
 * @brief The interface of red-black tree.
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

#ifndef PURC_PRIVATE_RBTREE_H
#define PURC_PRIVATE_RBTREE_H

#include "private/list.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RB_RED      0
#define RB_BLACK    1

struct rb_node {
    unsigned int    rb_color;
    struct rb_node *rb_parent;
    struct rb_node *rb_right;
    struct rb_node *rb_left;
};

#define pcutils_rbtree_parent(r)    ((r)->rb_parent)
#define pcutils_rbtree_color(r)     ((r)->rb_color)
#define pcutils_rbtree_is_red(r)    (!pcutils_rbtree_color(r))
#define pcutils_rbtree_is_black(r)  pcutils_rbtree_color(r)
#define pcutils_rbtree_set_red(r)   do { (r)->rb_color = 0; } while (0)
#define pcutils_rbtree_set_black(r) do { (r)->rb_color = 1; } while (0)

static inline void
pcutils_rbtree_set_parent(struct rb_node *rb, struct rb_node *p)
{
    rb->rb_parent = p;
}

static inline void
pcutils_rbtree_set_color(struct rb_node *rb, int color)
{
    rb->rb_color = color;
}

struct rb_root {
    struct rb_node *rb_node;
};

#define RB_ROOT    (struct rb_root) { NULL, }
#define pcutils_rbtree_entry(ptr, type, member) container_of(ptr, type, member)

#define RB_EMPTY_ROOT(root)    ((root)->rb_node == NULL)
#define RB_EMPTY_NODE(node)    (pcutils_rbtree_parent(node) == node)
#define RB_CLEAR_NODE(node)    (pcutils_rbtree_set_parent(node, node))

static inline void pcutils_rbtree_init_node(struct rb_node *rb)
{
    rb->rb_color = 0;
    rb->rb_parent = NULL;
    rb->rb_right = NULL;
    rb->rb_left = NULL;
    RB_CLEAR_NODE(rb);
}

void pcutils_rbtree_insert_color(struct rb_node *, struct rb_root *);
void pcutils_rbtree_erase(struct rb_node *, struct rb_root *);

typedef void (*rb_augment_f)(struct rb_node *node, void *data);

void pcutils_rbtree_augment_insert(struct rb_node *node,
                  rb_augment_f func, void *data);
struct rb_node *pcutils_rbtree_augment_erase_begin(struct rb_node *node);
void pcutils_rbtree_augment_erase_end(struct rb_node *node,
                 rb_augment_f func, void *data);

/* Find logical next and previous nodes in a tree */
struct rb_node *pcutils_rbtree_next(const struct rb_node *);
struct rb_node *pcutils_rbtree_prev(const struct rb_node *);
struct rb_node *pcutils_rbtree_first(const struct rb_root *);
struct rb_node *pcutils_rbtree_last(const struct rb_root *);

/* Fast replacement of a single node without remove/rebalance/add/rebalance */
void pcutils_rbtree_replace_node(struct rb_node *victim, struct rb_node *newnode,
                struct rb_root *root);

static inline void
pcutils_rbtree_link_node(struct rb_node * node,
        struct rb_node * parent, struct rb_node ** rb_link)
{
    node->rb_color = 0;
    node->rb_parent = parent;
    node->rb_left = node->rb_right = NULL;

    *rb_link = node;
}

int pcutils_rbtree_traverse(struct rb_root *root, void *ud,
        int (*cb)(struct rb_node *node, void *ud));

int pcutils_rbtree_insert(struct rb_root *root, void *ud,
        int (*cmp)(struct rb_node *node, void *ud),
        struct rb_node* (*new_entry)(void *ud));

int pcutils_rbtree_insert_or_get(struct rb_root *root, void *ud,
        int (*cmp)(struct rb_node *node, void *ud),
        struct rb_node* (*new_entry)(void *ud),
        struct rb_node **node);

struct rb_node* pcutils_rbtree_find(struct rb_root *root, void *ud,
        int (*cmp)(struct rb_node *node, void *ud));

int pcutils_rbtree_insert_only(struct rb_root *root, void *ud,
        int (*cmp)(struct rb_node *node, void *ud),
        struct rb_node *node);

void pcutils_rbtree_insert_or_replace(struct rb_root *root, void *ud,
        int (*cmp)(struct rb_node *node, void *ud),
        struct rb_node *node, struct rb_node **old);

/* struct rb_node *_p */
#define pcutils_rbtree_for_each(_rb_node, _p)                          \
    for (_p = _rb_node;                                                \
            _p; _p = pcutils_rbtree_next(_p))

/* struct rb_node *_p */
#define pcutils_rbtree_for_each_reverse(_rb_node, _p)                  \
    for (_p = _rb_node;                                                \
            _p; _p = pcutils_rbtree_prev(_p))

/* struct rb_node *_p, *_n */
#define pcutils_rbtree_for_each_safe(_rb_node, _p, _n)                 \
    for (_p = _rb_node;                                                \
            _n = _p ? pcutils_rbtree_next(_p) : NULL, _p;              \
            _p = _n)

/* struct rb_node *_p, *_n */
#define pcutils_rbtree_for_each_reverse_safe(_rb_node, _p, _n)         \
    for (_p = _rb_node;                                                \
            _n = _p ? pcutils_rbtree_prev(_p) : NULL, _p;              \
            _p = _n)

#ifdef __cplusplus
}
#endif

#endif /* PURC_PRIVATE_RBTREE_H */
