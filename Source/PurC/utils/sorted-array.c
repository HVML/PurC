/*
 * @file sorted-array.c
 * @author Vincent Wei
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

static int
def_cmp(const void *sortv1, const void *sortv2)
{
    uintptr_t v1 = (uintptr_t)sortv1;
    uintptr_t v2 = (uintptr_t)sortv2;

    if (v1 > v2)
        return 1;
    else if (v1 < v2)
        return -1;

    return 0;
}

struct sorted_array *
pcutils_sorted_array_create(unsigned int flags, size_t sz_init,
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
    sa->cmp_fn = (cmp_fn == NULL) ? def_cmp : cmp_fn;

    if (sa->members == NULL) {
        free(sa);
        sa = NULL;
    }

    return sa;
}

void pcutils_sorted_array_destroy(struct sorted_array *sa)
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

int pcutils_sorted_array_add(struct sorted_array *sa, void *sortv, void *data,
        ssize_t *index)
{
    ssize_t i, idx;
    ssize_t low, high, mid;

    if (!(sa->flags & SAFLAG_DUPLCATE_SORTV)) {
        if (pcutils_sorted_array_find(sa, sortv, NULL)) {
            return -1;
        }
    }

    if ((sa->nr_members + 1) > (SIZE_MAX >> 1)) {
        return -2;
    }

    if ((sa->nr_members + 1) >= sa->sz_array) {
        size_t new_sz = sa->sz_array + SASZ_DEFAULT;
        struct sorted_array_member *old_members = sa->members;

        sa->members = realloc(sa->members,
                sizeof(struct sorted_array_member) * new_sz);
        if (sa->members == NULL) {
            sa->members = old_members;
            return -3;
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
    else {
        idx = low;
    }

    for (i = sa->nr_members; i > idx; i--) {
        sa->members[i].sortv = sa->members[i - 1].sortv;
        sa->members[i].data = sa->members[i - 1].data;
    }

    sa->members[idx].sortv = sortv;
    sa->members[idx].data = data;

    sa->nr_members++;
    if (index) {
        *index = idx;
    }
    return 0;
}

bool pcutils_sorted_array_remove(struct sorted_array *sa, const void* sortv)
{
    ssize_t low, high, mid;
    size_t i;

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

    sa->nr_members--;

    for (i = mid; i < sa->nr_members; i++) {
        sa->members[i].sortv = sa->members[i + 1].sortv;
        sa->members[i].data = sa->members[i + 1].data;
    }

    return true;
}

bool pcutils_sorted_array_find(struct sorted_array *sa, const void *sortv, void **data)
{
    ssize_t low, high, mid;

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

size_t pcutils_sorted_array_count(struct sorted_array *sa)
{
    return sa->nr_members;
}

const void* pcutils_sorted_array_get(struct sorted_array *sa, size_t idx, void **data)
{
    assert (idx < sa->nr_members);

    if (data) {
        *data = sa->members[idx].data;
    }

    return sa->members[idx].sortv;
}

void pcutils_sorted_array_delete(struct sorted_array *sa, size_t idx)
{
    size_t i;

    assert (idx < sa->nr_members);

    if (sa->free_fn) {
        sa->free_fn(sa->members[idx].sortv, sa->members[idx].data);
    }

    sa->nr_members--;
    for (i = idx; i < sa->nr_members; i++) {
        sa->members[i].sortv = sa->members[i + 1].sortv;
        sa->members[i].data = sa->members[i + 1].data;
    }
}

