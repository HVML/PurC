/*
 * kvlist - simple key/value store
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
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "private/avl.h"
#include "private/kvlist.h"
#include "private/utils.h"

int pcutils_kvlist_strlen(struct kvlist *kv, const void *data)
{
    UNUSED_PARAM(kv);
    return strlen(data) + 1;
}

void pcutils_kvlist_init(struct kvlist *kv, int (*get_len)(struct kvlist *kv, const void *data))
{
    pcutils_avl_init(&kv->avl, pcutils_avl_strcmp, false, NULL);
    kv->get_len = get_len;
}

static struct kvlist_node *__kvlist_get(struct kvlist *kv, const char *name)
{
    struct kvlist_node *node;

    return avl_find_element(&kv->avl, name, node, avl);
}

void *pcutils_kvlist_get(struct kvlist *kv, const char *name)
{
    struct kvlist_node *node;

    node = __kvlist_get(kv, name);
    if (!node)
        return NULL;

    return node->data;
}

bool pcutils_kvlist_delete(struct kvlist *kv, const char *name)
{
    struct kvlist_node *node;

    node = __kvlist_get(kv, name);
    if (node) {
        pcutils_avl_delete(&kv->avl, &node->avl);
        free(node);
    }

    return !!node;
}

bool pcutils_kvlist_set(struct kvlist *kv, const char *name, const void *data)
{
    struct kvlist_node *node;
    char *name_buf;
    int len = kv->get_len ? kv->get_len(kv, data) : (int)(sizeof (void *));

    node = calloc_a(sizeof(struct kvlist_node) + len,
        &name_buf, strlen(name) + 1);
    if (!node)
        return false;

    pcutils_kvlist_delete(kv, name);

    memcpy(node->data, data, len);

    node->avl.key = strcpy(name_buf, name);
    pcutils_avl_insert(&kv->avl, &node->avl);

    return true;
}

void pcutils_kvlist_free(struct kvlist *kv)
{
    struct kvlist_node *node, *tmp;

    avl_remove_all_elements(&kv->avl, node, avl, tmp)
        free(node);
}

