/*
 * @file kvlist.h
 * @date 2021/07/05
 * @brief A simple key/value store based on AVL tree.
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
 * The code is derived from uBox, which is licensed under MIT licence.
 *
 * Copyright (C) 2014 Felix Fietkau <nbd@openwrt.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef PURC_PRIVATE_KVLIST_H
#define PURC_PRIVATE_KVLIST_H

#include "purc-utils.h"
#include "private/avl.h"

struct pcutils_kvlist {
    struct avl_tree avl;

    /* VW: can be NULL for pointer */
    size_t (*get_len)(struct pcutils_kvlist *kv, const void *data);
};

struct kvlist_node {
    struct avl_node avl;

    /* VW: use the maximum alignment instead of 4 (pointer safe)*/
    char data[0] __attribute__((aligned));
};

#define KVLIST_INIT(_name, _get_len)                                        \
    {                                                                       \
        .avl = AVL_TREE_INIT(_name.avl, pcutils_avl_strcmp, false, NULL),   \
        .get_len = _get_len                                                 \
    }

#define KVLIST(_name, _get_len)                                             \
    struct pcutils_kvlist _name = KVLIST_INIT(_name, _get_len)

#define __ptr_to_kv(_ptr)                                                   \
    container_of(((char *) (_ptr)), struct kvlist_node, data[0])
#define __avl_list_to_kv(_l)                                                \
    container_of(_l, struct kvlist_node, avl.list)

#define kvlist_for_each(kv, name, value)                                    \
    for (value = (void *) __avl_list_to_kv((kv)->avl.list_head.next)->data, \
         name = (const char *) __ptr_to_kv(value)->avl.key, (void) name;    \
         &__ptr_to_kv(value)->avl.list != &(kv)->avl.list_head;             \
         value = (void *)(__avl_list_to_kv(                                 \
                 __ptr_to_kv(value)->avl.list.next))->data,                 \
         name = (const char *) __ptr_to_kv(value)->avl.key)

#define kvlist_for_each_safe(kv, name, next, value) \
    for (value = (void *) __avl_list_to_kv((kv)->avl.list_head.next)->data, \
         name = (const char *) __ptr_to_kv(value)->avl.key, (void) name,    \
         next = (void *) (__avl_list_to_kv(                                 \
                 __ptr_to_kv(value)->avl.list.next))->data;                 \
         &__ptr_to_kv(value)->avl.list != &(kv)->avl.list_head;             \
         value = next, name = (const char *) __ptr_to_kv(value)->avl.key,   \
         next = (void *) (__avl_list_to_kv(                                 \
                 __ptr_to_kv(value)->avl.list.next))->data)

#ifdef __cplusplus
extern "C" {
#endif

/* internal interfaces */
void pcutils_kvlist_init_ex(struct pcutils_kvlist *kv,
        size_t (*get_len)(struct pcutils_kvlist *kv, const void *data),
        bool caseless);
static inline void pcutils_kvlist_init(struct pcutils_kvlist *kv,
        size_t (*get_len)(struct pcutils_kvlist *kv, const void *data))
{
    pcutils_kvlist_init_ex(kv, get_len, false);
}

void pcutils_kvlist_cleanup(struct pcutils_kvlist *kv);

#ifdef __cplusplus
}
#endif

#endif  /* PURC_PRIVATE_KVLIST_H */
