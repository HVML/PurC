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
    pcutils_uomap_entry entry;
    pchash_foreach(map, entry) {
        int r = cb(entry->key, entry->val, ud);
        if (r)
            return r;
    }

    return 0;
}

struct pcutils_uomap_iterator
pcutils_uomap_it_begin_first(pcutils_uomap *map)
{
    struct pcutils_uomap_iterator it = {};
    it.curr = map->head;
    it.next = map->head ? map->head->next : NULL;
    return it;
}

struct pcutils_uomap_iterator
pcutils_uomap_it_begin_last(pcutils_uomap *map)
{
    struct pcutils_uomap_iterator it = {};
    it.curr = map->tail;
    it.prev = map->tail ? map->head->prev : NULL;
    return it;
}

pcutils_uomap_entry
pcutils_uomap_it_value(struct pcutils_uomap_iterator *it)
{
    return it->curr;
}

pcutils_uomap_entry
pcutils_uomap_it_next(struct pcutils_uomap_iterator *it)
{
    it->prev = it->curr;
    it->curr = it->next;
    it->next = it->curr ? it->curr->next : NULL;
    return it->curr;
}

pcutils_uomap_entry
pcutils_uomap_it_prev(struct pcutils_uomap_iterator *it)
{
    it->next = it->curr;
    it->curr = it->prev;
    it->prev = it->curr ? it->curr->prev : NULL;
    return it->curr;
}

void
pcutils_uomap_it_end(struct pcutils_uomap_iterator *it)
{
    it->curr = NULL;
    it->next = NULL;
    it->prev = NULL;
}

