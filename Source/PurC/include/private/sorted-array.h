/**
 * @file sorted-array.h
 * @author Vincent Wei
 * @date 2021/11/17
 * @brief The hearder file for sorted array.
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
#ifndef PURC_PRIVATE_SORTED_ARRAY_H
#define PURC_PRIVATE_SORTED_ARRAY_H

#include <stdbool.h>

struct sorted_array;

typedef void (*sacb_free)(void *sortv, void *data);
typedef int  (*sacb_compare)(const void *sortv1, const void *sortv2);

#define SAFLAG_ORDER_ASC            0x0000
#define SAFLAG_ORDER_DESC           0x0001
#define SAFLAG_DUPLCATE_SORTV       0x0002

#define SAFLAG_DEFAULT              0x0000

#ifdef __cplusplus
extern "C" {
#endif

/* create an empty sorted array; free_fn can be NULL */
struct sorted_array *
pcutils_sorted_array_create(unsigned int flags, size_t sz_init,
        sacb_free free_fn, sacb_compare cmp_fn);

/* destroy a sorted array */
void pcutils_sorted_array_destroy(struct sorted_array *sa);

/* add a new member with the sort value and the data. */
int pcutils_sorted_array_add(struct sorted_array *sa, void *sortv, void *data,
        ssize_t *index);

/* remove one member which has the same sort value. */
bool pcutils_sorted_array_remove(struct sorted_array *sa, const void* sortv);

/* find the first member which has the same sort value. */
bool pcutils_sorted_array_find(struct sorted_array *sa,
        const void *sortv, void **data, ssize_t *index);

/* retrieve the number of the members of the sorted array */
size_t pcutils_sorted_array_count(struct sorted_array *sa);

/* retrieve the member by the index and return the sort value;
   data can be NULL. */
const void* pcutils_sorted_array_get(struct sorted_array *sa,
        size_t idx, void **data);

/* delete the member by the index */
void pcutils_sorted_array_delete(struct sorted_array *sa, size_t idx);

#ifdef __cplusplus
}
#endif

#endif  /* PURC_PRIVATE_SORTED_ARRAY_H */
