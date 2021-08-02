/**
 * @file array.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for array.
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
 * The code is derived from Lexbor (<https://github.com/lexbor/lexbor>),
 * which is licensed under the Apache License, Version 2.0.
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

#ifndef PCUTILS_ARRAY_H
#define PCUTILS_ARRAY_H

#include <stddef.h>
#include <stdbool.h>

typedef struct {
    void   **list;
    size_t size;
    size_t length;
}
pcutils_array_t;

#ifdef __cplusplus
extern "C" {
#endif

pcutils_array_t * pcutils_array_create(void) WTF_INTERNAL;

unsigned int
pcutils_array_init(pcutils_array_t *array, size_t size) WTF_INTERNAL;

void pcutils_array_clean(pcutils_array_t *array) WTF_INTERNAL;

pcutils_array_t *
pcutils_array_destroy(pcutils_array_t *array, bool self_destroy) WTF_INTERNAL;

void **
pcutils_array_expand(pcutils_array_t *array, size_t up_to) WTF_INTERNAL;

unsigned int
pcutils_array_push(pcutils_array_t *array, void *value) WTF_INTERNAL;

void * pcutils_array_pop(pcutils_array_t *array) WTF_INTERNAL;

unsigned int
pcutils_array_insert(pcutils_array_t *array,
        size_t idx, void *value) WTF_INTERNAL;

unsigned int
pcutils_array_set(pcutils_array_t *array,
        size_t idx, void *value) WTF_INTERNAL;

void
pcutils_array_delete(pcutils_array_t *array,
        size_t begin, size_t length) WTF_INTERNAL;


/*
 * Inline functions
 */
static inline void * pcutils_array_get(pcutils_array_t *array, size_t idx)
{
    if (idx >= array->length) {
        return NULL;
    }

    return array->list[idx];
}

static inline size_t pcutils_array_length(pcutils_array_t *array)
{
    return array->length;
}

static inline size_t pcutils_array_size(pcutils_array_t *array)
{
    return array->size;
}


/*
 * No inline functions for ABI.
 */
void * pcutils_array_get_noi(pcutils_array_t *array, size_t idx) WTF_INTERNAL;

size_t pcutils_array_length_noi(pcutils_array_t *array) WTF_INTERNAL;

size_t pcutils_array_size_noi(pcutils_array_t *array) WTF_INTERNAL;


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCUTILS_ARRAY_H */
