/*
 * kvlist - simple key/value store
 *
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
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
#ifndef _purc_util_kvlist_h
#define _purc_util_kvlist_h

#include "avl-cmp.h"
#include "avl.h"

struct kvlist {
    struct avl_tree avl;

    /* VW: can be NULL for pointer */
    int (*get_len)(struct kvlist *kv, const void *data);
};

struct kvlist_node {
    struct avl_node avl;

    /* VW: use the maximum alignment instead of 4 (pointer safe)*/
    char data[0] __attribute__((aligned));
};

#define KVLIST_INIT(_name, _get_len)                        \
    {                                    \
        .avl = AVL_TREE_INIT(_name.avl, avl_strcmp, false, NULL),    \
        .get_len = _get_len                        \
    }

#define KVLIST(_name, _get_len)                            \
    struct kvlist _name = KVLIST_INIT(_name, _get_len)

#define __ptr_to_kv(_ptr) container_of(((char *) (_ptr)), struct kvlist_node, data[0])
#define __avl_list_to_kv(_l) container_of(_l, struct kvlist_node, avl.list)

#define kvlist_for_each(kv, name, value) \
    for (value = (void *) __avl_list_to_kv((kv)->avl.list_head.next)->data,            \
         name = (const char *) __ptr_to_kv(value)->avl.key, (void) name;            \
         &__ptr_to_kv(value)->avl.list != &(kv)->avl.list_head;                \
         value = (void *) (__avl_list_to_kv(__ptr_to_kv(value)->avl.list.next))->data,    \
         name = (const char *) __ptr_to_kv(value)->avl.key)

#define kvlist_for_each_safe(kv, name, next, value) \
    for (value = (void *) __avl_list_to_kv((kv)->avl.list_head.next)->data,             \
         name = (const char *) __ptr_to_kv(value)->avl.key, (void) name,                \
         next = (void *) (__avl_list_to_kv(__ptr_to_kv(value)->avl.list.next))->data;   \
         &__ptr_to_kv(value)->avl.list != &(kv)->avl.list_head;                         \
         value = next, name = (const char *) __ptr_to_kv(value)->avl.key,               \
         next = (void *) (__avl_list_to_kv(__ptr_to_kv(value)->avl.list.next))->data)

#ifdef __cplusplus
extern "C" {
#endif

/* get_len can be NULL for pointer */
void kvlist_init(struct kvlist *kv,
        int (*get_len)(struct kvlist *kv, const void *data));
void kvlist_free(struct kvlist *kv);

void *kvlist_get(struct kvlist *kv, const char *name);
const char *kvlist_set_ex(struct kvlist *kv,
        const char *name, const void *data);

static inline bool
kvlist_set(struct kvlist *kv, const char *name, const void *data) {
    return kvlist_set_ex(kv, name, data) != NULL;
}

bool kvlist_delete(struct kvlist *kv, const char *name);

int kvlist_strlen(struct kvlist *kv, const void *data);

#ifdef __cplusplus
}
#endif

#endif  /* not defined _purc_util_kvlist_h */
