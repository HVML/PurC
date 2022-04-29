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

#define safe_container_of(_ptr, _type, _member)                 \
    ({                                                          \
        const __typeof__(*_ptr) *_tmp = (_ptr);                 \
        _tmp ? container_of(_tmp, _type, _member) : NULL;       \
    })

// struct pcutils_array_list *_al;
// struct pcutils_array_list_node *_p;
#define array_list_for_each(_al, _p)                       \
    for(_p = pcutils_array_list_get_first(_al);            \
        _p;                                                \
        _p = pcutils_array_list_get(_al, _p->idx+1))

// struct pcutils_array_list *_al;
// struct pcutils_array_list_node *_p, *_n;
#define array_list_for_each_safe(_al, _p, _n)              \
    for(_p = pcutils_array_list_get_first(_al);            \
        ({_n = _p ? pcutils_array_list_get(_al, _p->idx+1) \
                  : NULL; _p; });                          \
        _p = _n)

// struct pcutils_array_list *_al;
// struct pcutils_array_list_node *_p;
#define array_list_for_each_reverse(_al, _p)               \
    for(_p = pcutils_array_list_get_last(_al);             \
        _p;                                                \
        _p = pcutils_array_list_get(_al, _p->idx-1))

// struct pcutils_array_list *_al;
// struct pcutils_array_list_node *_p, *_n;
#define array_list_for_each_reverse_safe(_al, _p, _n)      \
    for(_p = pcutils_array_list_get_last(_al);             \
        ({_n = _p ? pcutils_array_list_get(_al, _p->idx-1) \
                  : NULL; _p; });                          \
        _p = _n)

#define array_list_for_each_entry(_al, _p, field)                   \
    for(_p = safe_container_of(pcutils_array_list_get_first(_al),   \
                    __typeof__(*_p), field);                        \
        _p;                                                         \
        _p = safe_container_of(pcutils_array_list_get(_al,          \
                                   _p->field.idx+1),                \
                    __typeof__(*_p), field))

#define array_list_for_each_entry_safe(_al, _p, _n, field)          \
    for(_p = safe_container_of(pcutils_array_list_get_first(_al),   \
                    __typeof__(*_p), field);                        \
        ({ _n = _p ? safe_container_of(pcutils_array_list_get(_al,  \
                                   _p->field.idx+1),                \
                    __typeof__(*_p), field) : NULL; _p; });         \
        _p = _n)

#define array_list_for_each_entry_reverse(_al, _p, field)           \
    for(_p = safe_container_of(pcutils_array_list_get_last(_al),    \
                    __typeof__(*_p), field);                        \
        _p;                                                         \
        _p = safe_container_of(pcutils_array_list_get(_al,          \
                                   _p->field.idx-1),                \
                    __typeof__(*_p), field))

#define array_list_for_each_entry_reverse_safe(_al, _p, _n, field)  \
    for(_p = safe_container_of(pcutils_array_list_get_last(_al),    \
                    __typeof__(*_p), field);                        \
        ({ _n = _p ? safe_container_of(pcutils_array_list_get(_al,  \
                                   _p->field.idx-1),                \
                    __typeof__(*_p), field) : NULL; _p; });         \
        _p = _n)

PCA_EXTERN_C_BEGIN

void
pcutils_array_list_init(struct pcutils_array_list *al);

void
pcutils_array_list_reset(struct pcutils_array_list *al);

static inline size_t
pcutils_array_list_length(struct pcutils_array_list *al)
{
    return al->nr;
}

static inline size_t
pcutils_array_list_capacity(struct pcutils_array_list *al)
{
    return al->sz;
}

int
pcutils_array_list_expand(struct pcutils_array_list *al, size_t capacity);

int
pcutils_array_list_set(struct pcutils_array_list *al,
        size_t idx,
        struct pcutils_array_list_node *node,
        struct pcutils_array_list_node **old);

int
pcutils_array_list_insert_before(struct pcutils_array_list *al,
        size_t idx,
        struct pcutils_array_list_node *node);

static inline int
pcutils_array_list_insert_after(struct pcutils_array_list *al,
        size_t idx,
        struct pcutils_array_list_node *node)
{
    return pcutils_array_list_insert_before(al, idx+1, node);
}

static inline int
pcutils_array_list_append(struct pcutils_array_list *al,
        struct pcutils_array_list_node *node)
{
    return pcutils_array_list_insert_before(al, al->nr, node);
}

static inline int
pcutils_array_list_prepend(struct pcutils_array_list *al,
        struct pcutils_array_list_node *node)
{
    return pcutils_array_list_insert_before(al, 0, node);
}

int
pcutils_array_list_remove(struct pcutils_array_list *al,
        size_t idx,
        struct pcutils_array_list_node **old);

struct pcutils_array_list_node*
pcutils_array_list_get(struct pcutils_array_list *al,
        size_t idx);

static inline struct pcutils_array_list_node*
pcutils_array_list_get_first(struct pcutils_array_list *al)
{
    return pcutils_array_list_get(al, 0);
}

static inline struct pcutils_array_list_node*
pcutils_array_list_get_last(struct pcutils_array_list *al)
{
    // NOTE: let pcutils_array_list_get to take care al->nr - 1
    return pcutils_array_list_get(al, al->nr - 1);
}

int
pcutils_array_list_swap(struct pcutils_array_list *al,
        size_t i, size_t j);

void
pcutils_array_list_sort(struct pcutils_array_list *al,
        void *ud, int (*cmp)(struct pcutils_array_list_node *l,
                struct pcutils_array_list_node *r, void *ud));

PCA_EXTERN_C_END

#endif // PURC_PRIVATE_ARRAY_LIST_H

