/**
 * @file array_list.c
 * @author Xu Xiaohong
 * @date 2022/01/08
 * @brief The internal interfaces for array-list
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
 *
 */

#define _GNU_SOURCE

#include "private/array_list.h"

#include "private/debug.h"

#include <stdlib.h>
#include <string.h>

static inline size_t
align(size_t n)
{
    return (n + 15) / 16 * 16;
}

int
pcutils_array_list_init(struct pcutils_array_list *al)
{
    memset(al, 0, sizeof(*al));
    INIT_LIST_HEAD(&al->list);

    return 0;
}

void
pcutils_array_list_reset(struct pcutils_array_list *al)
{
    if (al->nodes) {
        PC_ASSERT(al->nr == 0);
        free(al->nodes);
        al->nodes = NULL;
        al->sz = 0;
        al->nr = 0;
    }
}

int
pcutils_array_list_expand(struct pcutils_array_list *al, size_t capacity)
{
    if (capacity == 0)
        capacity = 1;

    size_t n = align(capacity);
    PC_ASSERT(n >= capacity);
    if (al->sz < n) {
        struct pcutils_array_list_node **nodes;
        nodes = (struct pcutils_array_list_node**)realloc(al->nodes,
                n * sizeof(*nodes));

        if (!nodes)
            return -1;

        al->nodes = nodes;
        al->sz    = n;
    }

    return 0;
}

int
pcutils_array_list_set(struct pcutils_array_list *al,
        size_t idx,
        struct pcutils_array_list_node *node,
        struct pcutils_array_list_node **old)
{
    PC_ASSERT(node);
    PC_ASSERT(node->node.prev == NULL);
    PC_ASSERT(node->node.next == NULL);
    PC_ASSERT(node->idx == (size_t)-1);
    PC_ASSERT(old);

    if (idx >= al->nr)
        return -1;

    PC_ASSERT(al->nodes);
    struct pcutils_array_list_node *curr = al->nodes[idx];
    list_del(&curr->node);
    list_add_tail(&node->node, &al->list);

    *old = al->nodes[idx];
    al->nodes[idx] = node;

    return 0;
}

int
pcutils_array_list_insert_before(struct pcutils_array_list *al,
        size_t idx,
        struct pcutils_array_list_node *node)
{
    PC_ASSERT(node);
    PC_ASSERT(node->node.prev == NULL);
    PC_ASSERT(node->node.next == NULL);
    PC_ASSERT(node->idx == (size_t)-1);

    int r;

    if (al->nr == al->sz) {
        r = pcutils_array_list_expand(al, al->sz + 1);
        if (r)
            return -1;
    }

    if (idx >= al->nr)
        idx = al->nr;

    if (al->nr > 0) {
        for (size_t i=al->nr-1; i>=idx; --i) {
            al->nodes[i+1] = al->nodes[i];
            al->nodes[i+1]->idx = i+1;
            if (i==0)
                break;
        }
    }
    al->nodes[idx] = node;
    al->nodes[idx]->idx = idx;

    list_add_tail(&node->node, &al->list);

    al->nr += 1;

    return 0;
}

int
pcutils_array_list_remove(struct pcutils_array_list *al,
        size_t idx,
        struct pcutils_array_list_node **old)
{
    if (idx >= al->nr)
        return -1;

    struct pcutils_array_list_node *node = al->nodes[idx];

    for (size_t i=node->idx; i+1<al->nr; ++i) {
        al->nodes[i] = al->nodes[i+1];
        al->nodes[i]->idx = i;
    }
    al->nodes[al->nr-1] = NULL;

    list_del(&node->node);
    node->idx = -1;

    *old = node;

    al->nr -= 1;

    return 0;
}

struct pcutils_array_list_node*
pcutils_array_list_get(struct pcutils_array_list *al,
        size_t idx)
{
    if (idx < al->nr)
        return al->nodes[idx];

    return NULL;
}

int
pcutils_array_list_swap(struct pcutils_array_list *al,
        size_t i, size_t j)
{
    if (i >= al->nr || j >= al->nr)
        return -1;

    if (i == j)
        return 0;

    struct pcutils_array_list_node *l = al->nodes[i];
    struct pcutils_array_list_node *r = al->nodes[j];

    al->nodes[i] = r;
    r->idx = i;
    al->nodes[j] = l;
    l->idx = j;

    return 0;
}

struct arr_user_data {
    int (*cmp)(struct pcutils_array_list_node *l,
        struct pcutils_array_list_node *r, void *ud);
    void *ud;
};

#if OS(HURD) || OS(LINUX)
static int cmp_f(const void *l, const void *r, void *ud)
{
    struct pcutils_array_list_node *l_n, *r_n;
    l_n = *(struct pcutils_array_list_node**)l;
    r_n = *(struct pcutils_array_list_node**)r;

    struct arr_user_data *d = (struct arr_user_data*)ud;
    return d->cmp(l_n, r_n, d->ud);
}
#elif OS(DARWIN) || OS(FREEBSD) || OS(NETBSD) || OS(OPENBSD) || OS(WINDOWS)
static int cmp_f(void *ud, const void *l, const void *r)
{
    struct pcutils_array_list_node *l_n, *r_n;
    l_n = *(struct pcutils_array_list_node**)l;
    r_n = *(struct pcutils_array_list_node**)r;

    struct arr_user_data *d = (struct arr_user_data*)ud;
    return d->cmp(l_n, r_n, d->ud);
}
#else
#error Unsupported operating system.
#endif

int
pcutils_array_list_sort(struct pcutils_array_list *al,
        void *ud, int (*cmp)(struct pcutils_array_list_node *l,
                struct pcutils_array_list_node *r, void *ud))
{
    struct pcutils_array_list_node **nodes = al->nodes;
    size_t nr = al->nr;

    struct arr_user_data d = {
        .cmp = cmp,
        .ud  = ud,
    };

#if OS(HURD) || OS(LINUX)
    qsort_r(nodes, nr, sizeof(*nodes), cmp_f, &d);
#elif OS(DARWIN) || OS(FREEBSD) || OS(NETBSD) || OS(OPENBSD)
    qsort_r(nodes, nr, sizeof(*nodes), &d, cmp_f);
#elif OS(WINDOWS)
    qsort_s(nodes, nr, sizeof(*nodes), cmp_f, &d);
#endif

    for (size_t i=0; i<al->nr; ++i) {
        struct pcutils_array_list_node *l = al->nodes[i];
        l->idx = i;
    }

    return 0;
}

