/*
 * @file unordered-map.c
 * @author Vincent Wei
 * @date 2023/01/29
 * @brief the implementation of unordered map based on hash table.
 *
 * Copyright (C) 2023 FMSoft <https://www.fmsoft.cn>
 * All rights reseved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "private/map.h"

int pcutils_uomap_traverse(pcutils_uomap *map, void *ud,
        int (*cb)(void *key, void *val, void *ud))
{
    pchash_entry *entry;

    for (size_t i = 0; i < map->size; i++) {
        struct list_head *p, *n;
        list_for_each_safe(p, n, map->table + i) {
            entry = list_entry(p, pchash_entry, list);
            int r = cb(entry->key, entry->val, ud);
            if (r)
                return r;
        }
    }

    return 0;
}

struct pcutils_uomap_iterator
pcutils_uomap_it_begin_first(pcutils_uomap *map)
{
    struct pcutils_uomap_iterator it = { map, NULL };

    for (size_t i = 0; i < map->size; i++) {
        if (!list_empty(map->table + i)) {
            it.curr = list_first_entry(map->table + i, pchash_entry, list);
            break;
        }
    }

    return it;
}

struct pcutils_uomap_iterator
pcutils_uomap_it_begin_last(pcutils_uomap *map)
{
    struct pcutils_uomap_iterator it = { map, NULL };

    for (size_t i = map->size; i > 0; i--) {
        size_t slot = i - 1;
        if (!list_empty(map->table + slot)) {
            it.curr = list_last_entry(map->table + slot, pchash_entry, list);
            break;
        }
    }

    return it;
}

pcutils_uomap_entry *
pcutils_uomap_it_value(struct pcutils_uomap_iterator *it)
{
    return it->curr;
}

pcutils_uomap_entry *
pcutils_uomap_it_next(struct pcutils_uomap_iterator *it)
{
    if (list_is_last(&it->curr->list, it->map->table + it->curr->slot)) {
        for (size_t i = it->curr->slot + 1; i < it->map->size; i++) {
            if (!list_empty(it->map->table + i)) {
                it->curr = list_first_entry(it->map->table + i,
                        pchash_entry, list);
                goto done;
            }
        }

        it->curr = NULL;
    }
    else {
        it->curr = list_entry(it->curr->list.next, pchash_entry, list);
    }

done:
    return it->curr;
}

pcutils_uomap_entry *
pcutils_uomap_it_prev(struct pcutils_uomap_iterator *it)
{
    if (list_is_first(&it->curr->list, it->map->table + it->curr->slot)) {
        for (size_t i = it->curr->slot; i > 0; i--) {
            size_t slot = i - 1;
            if (!list_empty(it->map->table + slot)) {
                it->curr = list_last_entry(it->map->table + slot,
                        pchash_entry, list);
                goto done;
            }
        }

        it->curr = NULL;
    }
    else {
        it->curr = list_entry(it->curr->list.prev, pchash_entry, list);
    }

done:
    return it->curr;
}

void
pcutils_uomap_it_end(struct pcutils_uomap_iterator *it)
{
    it->map = NULL;
    it->curr = NULL;
}

