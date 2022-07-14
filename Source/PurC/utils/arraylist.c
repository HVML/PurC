/*
 * @file arraylist.c
 * @author Michael Clark <michael@metaparadigm.com>
 * @date 2021/07/08
 *
 * Cleaned up by Vincent Wei.
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

#include "purc-utils.h"

#include <limits.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

struct pcutils_arrlist *pcutils_arrlist_new_ex(array_list_free_fn *free_fn,
        size_t initial_size)
{
    struct pcutils_arrlist *arr;

    if (initial_size >= SIZE_MAX / sizeof(void *))
        return NULL;
    if (initial_size == 0)
        initial_size = 1;

    arr = (struct pcutils_arrlist *)malloc(sizeof(struct pcutils_arrlist));
    if (!arr)
        return NULL;

    arr->size = initial_size;
    arr->length = 0;
    arr->free_fn = free_fn;

    if (!(arr->array = (void **)malloc(arr->size * sizeof(void *)))) {
        free(arr);
        return NULL;
    }
    return arr;
}

void pcutils_arrlist_free(struct pcutils_arrlist *arr)
{
    size_t i;

    if (arr->free_fn) {
        for (i = 0; i < arr->length; i++) {
            if (arr->array[i]) {
                arr->free_fn(arr->array[i]);
            }
        }
    }

    free(arr->array);
    free(arr);
}

void *pcutils_arrlist_get_idx(struct pcutils_arrlist *arr, size_t i)
{
    if (i >= arr->length)
        return NULL;
    return arr->array[i];
}

bool pcutils_arrlist_swap(struct pcutils_arrlist *arr, size_t idx1, size_t idx2)
{
    if (idx1 >= arr->length || idx2 >= arr->length || idx1 == idx2)
        return false;

    void* tmp = arr->array[idx1];
    arr->array[idx1] = arr->array[idx2];
    arr->array[idx2] = tmp;
    return true;
}

static int pcutils_arrlist_expand_internal(struct pcutils_arrlist *arr, size_t max)
{
    void *t;
    size_t new_size;

    if (max < arr->size)
        return 0;
    /* Avoid undefined behaviour on size_t overflow */
    if (arr->size >= SIZE_MAX / 2)
        new_size = max;
    else
    {
        new_size = arr->size << 1;
        if (new_size < max)
            new_size = max;
    }
    if (new_size > (~((size_t)0)) / sizeof(void *))
        return -1;
    if (!(t = realloc(arr->array, new_size * sizeof(void *))))
        return -1;
    arr->array = (void **)t;
    arr->size = new_size;
    return 0;
}

int pcutils_arrlist_shrink(struct pcutils_arrlist *arr, size_t empty_slots)
{
    void *t;
    size_t new_size;

    if (empty_slots >= SIZE_MAX / sizeof(void *) - arr->length)
        return -1;
    new_size = arr->length + empty_slots;
    if (new_size == arr->size)
        return 0;
    if (new_size > arr->size)
        return pcutils_arrlist_expand_internal(arr, new_size);
    if (new_size == 0)
        new_size = 1;

    if (!(t = realloc(arr->array, new_size * sizeof(void *))))
        return -1;
    arr->array = (void **)t;
    arr->size = new_size;
    return 0;
}

int pcutils_arrlist_put_idx(struct pcutils_arrlist *arr, size_t idx, void *data)
{
    if (idx > SIZE_MAX - 1)
        return -1;
    if (pcutils_arrlist_expand_internal(arr, idx + 1))
        return -1;
    if (idx < arr->length && arr->array[idx]) {
        if (arr->array[idx]!=data && arr->free_fn) {
            // avoid double-free
            arr->free_fn(arr->array[idx]);
        }
    }
    arr->array[idx] = data;
    if (idx > arr->length) {
        /* Zero out the arraylist slots in between the old length
           and the newly added entry so we know those entries are
           empty.
           e.g. when setting array[7] in an array that used to be 
           only 5 elements longs, array[5] and array[6] need to be
           set to 0.
         */
        memset(arr->array + arr->length, 0, (idx - arr->length)*sizeof(void *));
    }
    if (arr->length <= idx)
        arr->length = idx + 1;
    return 0;
}

int pcutils_arrlist_append(struct pcutils_arrlist *arr, void *data)
{
    /* Repeat some of pcutils_arrlist_put_idx() so we can skip several
       checks that we know are unnecessary when appending at the end
     */
    size_t idx = arr->length;
    if (idx > SIZE_MAX - 1)
        return -1;
    if (pcutils_arrlist_expand_internal(arr, idx + 1))
        return -1;
    arr->array[idx] = data;
    arr->length++;
    return 0;
}

void pcutils_arrlist_sort(struct pcutils_arrlist *arr,
        int (*compar)(const void *, const void *))
{
    qsort(arr->array, arr->length, sizeof(arr->array[0]), compar);
}

void *pcutils_arrlist_bsearch(const void **key, struct pcutils_arrlist *arr,
                         int (*compar)(const void *, const void *))
{
    return bsearch(key, arr->array, arr->length,
            sizeof(arr->array[0]), compar);
}

size_t pcutils_arrlist_length(struct pcutils_arrlist *arr)
{
    return arr->length;
}

int
pcutils_arrlist_del_idx(struct pcutils_arrlist *arr, size_t idx, size_t count)
{
    size_t i, stop;

    /* Avoid overflow in calculation with large indices. */
    if (idx > SIZE_MAX - count)
        return -1;
    stop = idx + count;
    if (idx >= arr->length || stop > arr->length)
        return -1;
    for (i = idx; i < stop; ++i)
    {
        // Because put_idx can skip entries, we need to check if
        // there's actually anything in each slot we're erasing.
        if (arr->array[i] && arr->free_fn)
            arr->free_fn(arr->array[i]);
    }
    memmove(arr->array + idx, arr->array + stop, (arr->length - stop) * sizeof(void *));
    arr->length -= count;
    return 0;
}

void*
pcutils_arrlist_get_first(struct pcutils_arrlist *arr)
{
    if (arr->length == 0)
        return NULL;
    return arr->array[0];
}

void*
pcutils_arrlist_get_last(struct pcutils_arrlist *arr)
{
    if (arr->length == 0)
        return NULL;
    return arr->array[arr->length-1];
}

