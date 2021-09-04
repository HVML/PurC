/*
 * @file hvml-entity.h
 * @author XueShuming
 * @date 2021/09/03
 * @brief The impl for hvml entity.
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

#include "hvml-entity.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#if HAVE(GLIB)
#include <gmodule.h>
#else
#include <stdlib.h>
#endif

#define PCHVML_END_OF_FILE       0

#if HAVE(GLIB)
#define    PCHVML_ALLOC(sz)   g_slice_alloc0(sz)
#define    PCHVML_FREE(p)     g_slice_free1(sizeof(*p), (gpointer)p)
#else
#define    PCHVML_ALLOC(sz)   calloc(1, sz)
#define    PCHVML_FREE(p)     free(p)
#endif

struct pchvml_entity_search {
    const struct pchvml_entity* first;
    const struct pchvml_entity* last;
    const struct pchvml_entity* most_recent_match;
    first_entry_starting_with_fn* first_starting_with;
    last_entry_starting_with_fn* last_starting_with;
    size_t current_length;
};

struct pchvml_entity_search* pchvml_entity_search_new_ex(
        const struct pchvml_entity* first,
        const struct pchvml_entity* last,
        first_entry_starting_with_fn* first_starting_with,
        last_entry_starting_with_fn* last_starting_with)
{
    struct pchvml_entity_search* search = (struct pchvml_entity_search*)
        PCHVML_ALLOC(sizeof(struct pchvml_entity_search));

    if (!search) {
        return NULL;
    }

    search->first = first;
    search->last = last;
    search->most_recent_match = NULL;
    search->first_starting_with = first_starting_with;
    search->last_starting_with = last_starting_with;

    return search;
}

void pchvml_entity_search_destroy(struct pchvml_entity_search* search)
{
    if (search) {
        PCHVML_FREE(search);
    }
}

const struct pchvml_entity* pchvml_entity_search_most_recent_match(
        struct pchvml_entity_search* search)
{
    return search->most_recent_match;
}

size_t pchvml_entity_search_current_length(struct pchvml_entity_search* search)
{
    return search->current_length;
}

enum pchvml_entity_search_compare_result {
    PCHVML_ENTITY_SEARCH_COMPARE_RESULT_BEFORE,
    PCHVML_ENTITY_SEARCH_COMPARE_RESULT_PREFIX,
    PCHVML_ENTITY_SEARCH_COMPARE_RESULT_AFTER,
};

const struct pchvml_entity* pchvml_entity_search_halfway(
        const struct pchvml_entity* left, const struct pchvml_entity* right)
{
    return &left[(right - left) / 2];
}

enum pchvml_entity_search_compare_result pchvml_entity_search_compare(
        struct pchvml_entity_search* search, const struct pchvml_entity* entry,
        wchar_t next_character)
{
    if (entry->nr_entity < search->current_length + 1) {
        return PCHVML_ENTITY_SEARCH_COMPARE_RESULT_BEFORE;
    }
    wchar_t entry_next_character = entry->entity[search->current_length];
    if (entry_next_character == next_character) {
        return PCHVML_ENTITY_SEARCH_COMPARE_RESULT_PREFIX;
    }
    return entry_next_character < next_character ?
        PCHVML_ENTITY_SEARCH_COMPARE_RESULT_BEFORE :
        PCHVML_ENTITY_SEARCH_COMPARE_RESULT_AFTER;
}

const struct pchvml_entity* pchvml_entity_search_find_first(
        struct pchvml_entity_search* search, wchar_t next_character)
{
    const struct pchvml_entity* left = search->first;
    const struct pchvml_entity* right = search->last;
    if (left == right) {
        return left;
    }
    enum pchvml_entity_search_compare_result result =
        pchvml_entity_search_compare(search, left, next_character);
    if (result == PCHVML_ENTITY_SEARCH_COMPARE_RESULT_PREFIX) {
        return left;
    }
    if (result == PCHVML_ENTITY_SEARCH_COMPARE_RESULT_AFTER) {
        return right;
    }
    while (left + 1 < right) {
        const struct pchvml_entity* probe = pchvml_entity_search_halfway(left,
                right);
        result = pchvml_entity_search_compare(search, probe, next_character);
        if (result == PCHVML_ENTITY_SEARCH_COMPARE_RESULT_BEFORE) {
            left = probe;
        }
        else {
            right = probe;
        }
    }
    return right;
}

const struct pchvml_entity* pchvml_entity_search_find_last(
        struct pchvml_entity_search* search, wchar_t next_character)
{
    const struct pchvml_entity* left = search->first;
    const struct pchvml_entity* right = search->last;
    if (left == right) {
        return right;
    }
    enum pchvml_entity_search_compare_result result =
        pchvml_entity_search_compare(search, right, next_character);
    if (result == PCHVML_ENTITY_SEARCH_COMPARE_RESULT_PREFIX) {
        return right;
    }
    if (result == PCHVML_ENTITY_SEARCH_COMPARE_RESULT_BEFORE) {
        return left;
    }
    while (left + 1 < right) {
        const struct pchvml_entity* probe = pchvml_entity_search_halfway(left,
                right);
        result = pchvml_entity_search_compare(search, probe, next_character);
        if (result == PCHVML_ENTITY_SEARCH_COMPARE_RESULT_AFTER) {
            right = probe;
        }
        else {
            left = probe;
        }
    }
    return left;
}

bool pchvml_entity_advance(struct pchvml_entity_search* search,
        wchar_t next_character)
{
    if (!search->current_length && search->first_starting_with
            && search->last_starting_with) {
        search->first = search->first_starting_with(next_character);
        search->last = search->last_starting_with(next_character);
        if (!search->first || !search->last) {
            search->first = NULL;
            search->last = NULL;
            return false;
        }
    } else {
        search->first = pchvml_entity_search_find_first(search, next_character);
        search->last = pchvml_entity_search_find_last(search, next_character);
        if (search->first == search->last &&
            pchvml_entity_search_compare(search, search->first,
               next_character) != PCHVML_ENTITY_SEARCH_COMPARE_RESULT_PREFIX) {
            search->first = NULL;
            search->last = NULL;
            return false;
        }
    }
    ++search->current_length;
    if (search->first->nr_entity != search->current_length) {
        return true;
    }
    search->most_recent_match = search->first;
    return true;
}


