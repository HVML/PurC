/*
 * @file arraylist.h
 * @author Michael Clark <michael@metaparadigm.com>
 * @date 2021/07/08
 * @brief The interfaces for array list.
 *
 * Cleaned up by Vincent Wei
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
 * Note that the code is derived from json-c which is licensed under MIT Licence.
 *
 * Copyright (c) 2004, 2005 Metaparadigm Pte. Ltd.
 * Copyright (c) 2009 Hewlett-Packard Development Company, L.P.
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

#ifndef PURC_PRIVATE_ARRAYLIST_H
#define PURC_PRIVATE_ARRAYLIST_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ARRAY_LIST_DEFAULT_SIZE 32

typedef void(array_list_free_fn)(void *data);

struct pcutils_arrlist {
    void **array;
    size_t length;
    size_t size;
    array_list_free_fn *free_fn;
};
typedef struct pcutils_arrlist pcutils_arrlist;

/**
 * Allocate an pcutils_arrlist of the desired size.
 *
 * If possible, the size should be chosen to closely match
 * the actual number of elements expected to be used.
 * If the exact size is unknown, there are tradeoffs to be made:
 * - too small - the pcutils_arrlist code will need to call realloc() more
 *   often (which might incur an additional memory copy).
 * - too large - will waste memory, but that can be mitigated
 *   by calling pcutils_arrlist_shrink() once the final size is known.
 *
 * @see pcutils_arrlist_shrink
 */
struct pcutils_arrlist *pcutils_arrlist_new_ex(array_list_free_fn *free_fn, int initial_size);

/**
 * Allocate an pcutils_arrlist of the default size (32).
 * @deprecated Use pcutils_arrlist_new_ex() instead.
 */
static inline struct pcutils_arrlist *pcutils_arrlist_new(array_list_free_fn *free_fn) {
    return pcutils_arrlist_new_ex(free_fn, ARRAY_LIST_DEFAULT_SIZE);
}

void pcutils_arrlist_free(struct pcutils_arrlist *al);

void *pcutils_arrlist_get_idx(struct pcutils_arrlist *al, size_t i);

int pcutils_arrlist_put_idx(struct pcutils_arrlist *al, size_t i, void *data);

int pcutils_arrlist_add(struct pcutils_arrlist *al, void *data);

size_t pcutils_arrlist_length(struct pcutils_arrlist *al);

void pcutils_arrlist_sort(struct pcutils_arrlist *arr, int (*compar)(const void *, const void *));

void *pcutils_arrlist_bsearch(const void **key, struct pcutils_arrlist *arr,
                                int (*compar)(const void *, const void *));

int pcutils_arrlist_del_idx(struct pcutils_arrlist *arr, size_t idx, size_t count);

/**
 * Shrink the array list to just enough to fit the number of elements in it,
 * plus empty_slots.
 */
int pcutils_arrlist_shrink(struct pcutils_arrlist *arr, size_t empty_slots);

#ifdef __cplusplus
}
#endif

#endif /* PURC_PRIVATE_ARRAYLIST_H */
