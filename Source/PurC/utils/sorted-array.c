/*
 * @file sorted-array.c
 * @date 2021/11/17
 * @brief The implementation of sorted array.
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
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
 *
 */
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "private/sorted-array.h"

struct sorted_array_member {
    void    *sortv;
    void    *data;
};

struct sorted_array {
    /* ascending order (true) or descending order (false) */
    unsigned int                flags;

    /* the size of pre-allocated array */
    size_t                      sz_array;

    /* the number of members */
    size_t                      nr_members;

    /* the pointer to an array contains the members */
    struct sorted_array_member *members;

    /* callback function to free member; nullable */
    sacb_free                   free_fn;

    /* callback function to compare two members */
    sacb_compare                cmp_fn;
};

#define SASZ_DEFAULT            4

struct sorted_array *
sorted_array_create(unsigned int flags, size_t sz_init,
        sacb_free free_fn, sacb_compare cmp_fn)
{
    struct sorted_array *sa;

    sa = calloc(1, sizeof(struct sorted_array));
    if (sa == NULL)
        return NULL;

    sa->flags = flags;
    sa->sz_array = (sz_init == 0) ? SASZ_DEFAULT : sz_init;
    sa->nr_members = 0;
    sa->members = calloc(sa->sz_array, sizeof(struct sorted_array_member));
    sa->free_fn = free_fn;
    sa->cmp_fn = cmp_fn;

    if (sa->members == NULL) {
        free(sa);
        sa = NULL;
    }

    return sa;
}

void sorted_array_destroy(struct sorted_array *sa)
{
    size_t idx;

    assert (sa != NULL && sa->members != NULL);

    if (sa->free_fn) {
        for (idx = 0; idx < sa->nr_members; idx++) {
            sa->free_fn(sa->members[idx].sortv, sa->members[idx].data);
        }
    }

    free(sa->members);
    free(sa);
}

int sorted_array_add(struct sorted_array *sa, void *sortv, void *data)
{
    size_t i, idx;
    size_t low, high, mid;

    if (!(sa->flags & SAFLAG_DUPLCATE_SORTV)) {
        if (sorted_array_find(sa, sortv, NULL)) {
            return -1;
        }
    }

    if ((sa->nr_members + 1) >= sa->sz_array) {
        size_t new_sz = sa->sz_array + SASZ_DEFAULT;
        struct sorted_array_member *old_members = sa->members;

        sa->members = realloc(sa->members,
                sizeof(struct sorted_array_member) * new_sz);
        if (sa->members == NULL) {
            sa->members = old_members;
            return -2;
        }

        sa->sz_array = new_sz;
    }

    low = 0;
    high = sa->nr_members - 1;
    while (low <= high) {
        int cmp;

        mid = (low + high) / 2;
        cmp = sa->cmp_fn(sortv, sa->members[mid].sortv);
        if (cmp == 0) {
            break;
        }
        else if (sa->flags & SAFLAG_ORDER_DESC) {
            if (cmp > 0) {
                high = mid - 1;
            }
            else {
                low = mid + 1;
            }
        }
        else {
            if (cmp < 0) {
                high = mid - 1;
            }
            else {
                low = mid + 1;
            }
        }
    }

    if (low <= high) {
        idx = mid;
    }
    else if (sa->flags & SAFLAG_ORDER_DESC) {
        idx = low;
    }
    else {
        idx = high;
    }

    for (i = sa->nr_members; i > idx; i++) {
        sa->members[i].sortv = sa->members[i - 1].sortv;
        sa->members[i].data = sa->members[i - 1].data;
    }

    sa->members[idx].sortv = sortv;
    sa->members[idx].data = data;

    sa->nr_members++;
    return 0;
}

bool sorted_array_remove(struct sorted_array *sa, const void* sortv)
{
    size_t i, low, high, mid;

    low = 0;
    high = sa->nr_members - 1;
    while (low <= high) {
        int cmp;

        mid = (low + high) / 2;
        cmp = sa->cmp_fn(sortv, sa->members[mid].sortv);
        if (cmp == 0) {
            goto found;
        }
        else if (sa->flags & SAFLAG_ORDER_DESC) {
            if (cmp > 0) {
                high = mid - 1;
            }
            else {
                low = mid + 1;
            }
        }
        else {
            if (cmp < 0) {
                high = mid - 1;
            }
            else {
                low = mid + 1;
            }
        }
    }

    return false;

found:
    if (sa->free_fn) {
        sa->free_fn(sa->members[mid].sortv, sa->members[mid].data);
    }

    for (i = sa->nr_members; i > mid; i++) {
        sa->members[i].sortv = sa->members[i - 1].sortv;
        sa->members[i].data = sa->members[i - 1].data;
    }

    sa->nr_members--;
    return true;
}

bool sorted_array_find(struct sorted_array *sa, const void *sortv, void **data)
{
    size_t low, high, mid;

    low = 0;
    high = sa->nr_members - 1;

    while (low <= high) {
        int cmp;

        mid = (low + high) / 2;
        cmp = sa->cmp_fn(sortv, sa->members[mid].sortv);
        if (cmp == 0) {
            goto found;
        }
        else if (sa->flags & SAFLAG_ORDER_DESC) {
            if (cmp > 0) {
                high = mid - 1;
            }
            else {
                low = mid + 1;
            }
        }
        else {
            if (cmp < 0) {
                high = mid - 1;
            }
            else {
                low = mid + 1;
            }
        }
    }

    return false;

found:
    if (data) {
        *data = sa->members[mid].data;
    }

    return true;
}

size_t sorted_array_count(struct sorted_array *sa)
{
    return sa->nr_members;
}

const void* sorted_array_get(struct sorted_array *sa, size_t idx, void **data)
{
    assert (idx < sa->nr_members);

    if (data) {
        *data = sa->members[idx].data;
    }

    return sa->members[idx].sortv;
}

void sorted_array_delete(struct sorted_array *sa, size_t idx)
{
    size_t i;

    assert (idx < sa->nr_members);

    if (sa->free_fn) {
        sa->free_fn(sa->members[idx].sortv, sa->members[idx].data);
    }

    for (i = idx + 1; i < sa->nr_members; i++) {
        sa->members[i - 1].sortv = sa->members[i].sortv;
        sa->members[i - 1].data = sa->members[i].data;
    }
}

