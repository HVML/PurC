/**
 * @file array_list.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for array-list.
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

#ifndef PURC_PRIVATE_ARRAY_LIST_H
#define PURC_PRIVATE_ARRAY_LIST_H

#include "purc-macros.h"

#include "private/list.h"

struct pcutils_array_list_node {
    struct list_head             node;
    size_t                       idx;
};

typedef struct pcutils_array_list       pcutils_array_list;

struct pcutils_array_list {
    struct pcutils_array_list_node          **nodes;
    size_t                                    sz;
    size_t                                    nr;

    struct list_head                          list;
};

#define array_list_first(__arrlist) \
    container_of((__arrlist)->list.next, struct pcutils_array_list_node, node)
#define array_list_last(__arrlist) \
    container_of((__arrlist)->list.prev, struct pcutils_array_list_node, node)

#define array_list_next(__ptr) \
    container_of((__ptr)->node.next, struct pcutils_array_list_node, node)
#define array_list_prev(__ptr) \
    container_of((__ptr)->node.prev, struct pcutils_array_list_node, node)

// struct pcutils_array_list *_arrlist;
// struct pcutils_array_list_node *_p;
#define array_list_for_each(_arrlist, _p)                       \
    for(_p = array_list_first(_arrlist);                        \
        &(_p)->node != &(_arrlist)->list;                       \
        _p = array_list_next(_p))

// struct pcutils_array_list *_arrlist;
// struct pcutils_array_list_node *_p, *_n;
#define array_list_for_each_safe(_arrlist, _p, _n)              \
    for(_p = array_list_first(_arrlist);                        \
        ({_n = _p ? array_list_next(_p) : NULL;                 \
          &(_p)->node != &(_arrlist)->list;});                  \
        _p = _n)

// struct pcutils_array_list *_arrlist;
// struct pcutils_array_list_node *_p;
#define array_list_for_each_reverse(_arrlist, _p)               \
    for(_p = array_list_last(_arrlist);                         \
        &(_p)->node != &(_arrlist)->list;                       \
        _p = array_list_prev(_p))

// struct pcutils_array_list *_arrlist;
// struct pcutils_array_list_node *_p, *_n;
#define array_list_for_each_reverse_safe(_arrlist, _p, _n)      \
    for(_p = array_list_last(_arrlist);                         \
        ({_n = _p ? array_list_prev(_p) : NULL;                 \
          &(_p)->node != &(_arrlist)->list;});                  \
        _p = _n)

#define array_list_for_each_entry(_arrlist, _p, field)          \
    for(_p = container_of(array_list_first(_arrlist),           \
            __typeof__(*_p), field);                            \
        &(_p)->field.node != &(_arrlist)->list;                 \
        _p = container_of(array_list_next(&(_p)->field),        \
            __typeof__(*_p), field))

#define array_list_for_each_entry_safe(_arrlist, _p, _n, field) \
    for(_p = container_of(array_list_first(_arrlist),           \
            __typeof__(*_p), field);                            \
        ({_n = _p ? container_of(array_list_next(&(_p)->field), \
                    __typeof__(*_p), field) : NULL;             \
          &(_p)->field.node != &(_arrlist)->list;});            \
        _p = _n)

#define array_list_for_each_entry_reverse(_arrlist, _p, field)  \
    for(_p = container_of(array_list_last(_arrlist),            \
            __typeof__(*_p), field);                            \
        &(_p)->field.node != &(_arrlist)->list;                 \
        _p = container_of(array_list_prev(&(_p)->field),        \
            __typeof__(*_p), field))

#define array_list_for_each_entry_reverse_safe(_arrlist, _p, _n, field) \
    for(_p = container_of(array_list_last(_arrlist),                    \
            __typeof__(*_p), field);                                    \
        ({_n = _p ? container_of(array_list_prev(&(_p)->field),         \
                    __typeof__(*_p), field) : NULL;                     \
          &(_p)->field.node != &(_arrlist)->list;});                    \
        _p = _n)

PCA_EXTERN_C_BEGIN

int
pcutils_array_list_init(struct pcutils_array_list *arrlist);

void
pcutils_array_list_reset(struct pcutils_array_list *arrlist);

static inline size_t
pcutils_array_list_length(struct pcutils_array_list *arrlist)
{
    return arrlist->nr;
}

static inline size_t
pcutils_array_list_capacity(struct pcutils_array_list *arrlist)
{
    return arrlist->sz;
}

int
pcutils_array_list_expand(struct pcutils_array_list *arrlist, size_t capacity);

int
pcutils_array_list_set(struct pcutils_array_list *arrlist,
        size_t idx,
        struct pcutils_array_list_node *node,
        struct pcutils_array_list_node **old);

int
pcutils_array_list_insert_before(struct pcutils_array_list *arrlist,
        size_t idx,
        struct pcutils_array_list_node *node);

static inline int
pcutils_array_list_insert_after(struct pcutils_array_list *arrlist,
        size_t idx,
        struct pcutils_array_list_node *node)
{
    return pcutils_array_list_insert_before(arrlist, idx+1, node);
}

static inline int
pcutils_array_list_append(struct pcutils_array_list *arrlist,
        struct pcutils_array_list_node *node)
{
    return pcutils_array_list_insert_before(arrlist, arrlist->nr, node);
}

static inline int
pcutils_array_list_prepend(struct pcutils_array_list *arrlist,
        struct pcutils_array_list_node *node)
{
    return pcutils_array_list_insert_before(arrlist, 0, node);
}

int
pcutils_array_list_remove(struct pcutils_array_list *arrlist,
        size_t idx,
        struct pcutils_array_list_node **old);

int
pcutils_array_list_get(struct pcutils_array_list *arrlist,
        size_t idx,
        struct pcutils_array_list_node **old);

int
pcutils_array_list_swap(struct pcutils_array_list *arrlist,
        size_t i, size_t j);

int
pcutils_array_list_sort(struct pcutils_array_list *arrlist,
        void *ud, int (*cmp)(struct pcutils_array_list_node *l,
                struct pcutils_array_list_node *r, void *ud));

PCA_EXTERN_C_END

#endif // PURC_PRIVATE_ARRAY_LIST_H

