/*
 * @file unordered-map.c
 * @author Vincent Wei
 * @date 2023/01/29
 * @brief the implementation of unordered map based on hash table.
 *
 * Copyright (C) 2023 FMSoft <https://www.fmsoft.cn>
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
 * Note that the code is copied from GPL'd MiniGUI developed by FMSoft.
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
        for (size_t i = it->curr->slot + 1; i > 0; i--) {
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

