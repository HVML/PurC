/*
 * @file kvlist.c
 * @date 2021/07/07
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
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "private/avl.h"
#include "private/kvlist.h"
#include "private/utils.h"

void pcutils_kvlist_init(struct pcutils_kvlist *kv,
        size_t (*get_len)(struct pcutils_kvlist *kv, const void *data))
{
    pcutils_avl_init(&kv->avl, pcutils_avl_strcmp, false, NULL);
    kv->get_len = get_len;
}

static struct kvlist_node *__kvlist_get(struct pcutils_kvlist *kv, const char *name)
{
    struct kvlist_node *node;

    return avl_find_element(&kv->avl, name, node, avl);
}

void *pcutils_kvlist_get(struct pcutils_kvlist *kv, const char *name)
{
    struct kvlist_node *node;

    node = __kvlist_get(kv, name);
    if (!node)
        return NULL;

    return node->data;
}

bool pcutils_kvlist_remove(struct pcutils_kvlist *kv, const char *name)
{
    struct kvlist_node *node;

    node = __kvlist_get(kv, name);
    if (node) {
        pcutils_avl_delete(&kv->avl, &node->avl);
        free(node);
    }

    return !!node;
}

const char *pcutils_kvlist_set_ex(struct pcutils_kvlist *kv,
        const char *name, const void *data)
{
    struct kvlist_node *node;
    char *name_buf;

    int len = kv->get_len ? kv->get_len(kv, data) : (int)(sizeof (void *));
    node = calloc_a(sizeof(struct kvlist_node) + len,
        &name_buf, strlen(name) + 1);
    if (!node)
        return NULL;

    pcutils_kvlist_remove(kv, name);
    memcpy(node->data, data, len);
    node->avl.key = strcpy(name_buf, name);
    pcutils_avl_insert(&kv->avl, &node->avl);

    return node->avl.key;
}

void pcutils_kvlist_cleanup(struct pcutils_kvlist *kv)
{
    struct kvlist_node *node, *tmp;

    avl_remove_all_elements(&kv->avl, node, avl, tmp)
        free(node);
}

size_t
pcutils_kvlist_for_each(pcutils_kvlist_t kv, void *ctxt,
        int (*on_each)(void *ctxt, const char *name, void *data))
{
    size_t n = 0;
    const char *name;
    void *data;

    kvlist_for_each(kv, name, data) {
        n++;
        if (on_each(ctxt, name, data))
            break;
    }

    return n;
}

size_t
pcutils_kvlist_for_each_safe(pcutils_kvlist_t kv, void *ctxt,
        int (*on_each)(void *ctxt, const char *name, void *data))
{
    size_t n = 0;
    const char *name;
    void *next, *data;

    kvlist_for_each_safe(kv, name, next, data) {
        n++;
        if (on_each(ctxt, name, data))
            break;
    }

    return n;
}


