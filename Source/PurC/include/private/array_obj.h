/**
 * @file array_object.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for array object.
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

#ifndef PURC_PRIVATE_ARRAY_OBJ_H
#define PURC_PRIVATE_ARRAY_OBJ_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

typedef struct {
    uint8_t *list;
    size_t  size;
    size_t  length;
    size_t  struct_size;
}
pcutils_array_obj_t;

#ifdef __cplusplus
extern "C" {
#endif

pcutils_array_obj_t * pcutils_array_obj_create(void) WTF_INTERNAL;

unsigned int
pcutils_array_obj_init(pcutils_array_obj_t *array,
        size_t size, size_t struct_size) WTF_INTERNAL;

void
pcutils_array_obj_clean(pcutils_array_obj_t *array) WTF_INTERNAL;

pcutils_array_obj_t *
pcutils_array_obj_destroy(pcutils_array_obj_t *array,
        bool self_destroy) WTF_INTERNAL;

uint8_t *
pcutils_array_obj_expand(pcutils_array_obj_t *array, size_t up_to) WTF_INTERNAL;

void *
pcutils_array_obj_push(pcutils_array_obj_t *array) WTF_INTERNAL;

void *
pcutils_array_obj_pop(pcutils_array_obj_t *array) WTF_INTERNAL;

void
pcutils_array_obj_delete(pcutils_array_obj_t *array,
        size_t begin, size_t length) WTF_INTERNAL;


/*
 * Inline functions
 */
static inline void
pcutils_array_obj_erase(pcutils_array_obj_t *array)
{
    memset(array, 0, sizeof(pcutils_array_obj_t));
}

static inline void *
pcutils_array_obj_get(pcutils_array_obj_t *array, size_t idx)
{
    if (idx >= array->length) {
        return NULL;
    }

    return array->list + (idx * array->struct_size);
}

static inline size_t
pcutils_array_obj_length(pcutils_array_obj_t *array)
{
    return array->length;
}

static inline size_t
pcutils_array_obj_size(pcutils_array_obj_t *array)
{
    return array->size;
}

static inline size_t
pcutils_array_obj_struct_size(pcutils_array_obj_t *array)
{
    return array->struct_size;
}

static inline void *
pcutils_array_obj_last(pcutils_array_obj_t *array)
{
    if (array->length == 0) {
        return NULL;
    }

    return array->list + ((array->length - 1) * array->struct_size);
}

#ifdef __cplusplus
}
#endif

#endif  /* PURC_PRIVATE_ARRAY_OBJ_H */
