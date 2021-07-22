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

#ifndef PCHTML_ARRAY_OBJ_H
#define PCHTML_ARRAY_OBJ_H

#ifdef __cplusplus
extern "C" {
#endif

#include "html/core/base.h"


typedef struct {
    uint8_t *list;
    size_t  size;
    size_t  length;
    size_t  struct_size;
}
pchtml_array_obj_t;


pchtml_array_obj_t * pchtml_array_obj_create(void) WTF_INTERNAL;

unsigned int 
pchtml_array_obj_init(pchtml_array_obj_t *array,
                      size_t size, size_t struct_size) WTF_INTERNAL;

void 
pchtml_array_obj_clean(pchtml_array_obj_t *array) WTF_INTERNAL;

pchtml_array_obj_t * 
pchtml_array_obj_destroy(pchtml_array_obj_t *array, bool self_destroy) WTF_INTERNAL;

uint8_t *
pchtml_array_obj_expand(pchtml_array_obj_t *array, size_t up_to) WTF_INTERNAL;

void * 
pchtml_array_obj_push(pchtml_array_obj_t *array) WTF_INTERNAL;

void * 
pchtml_array_obj_pop(pchtml_array_obj_t *array) WTF_INTERNAL;

void
pchtml_array_obj_delete(pchtml_array_obj_t *array, size_t begin, size_t length) WTF_INTERNAL;


/*
 * Inline functions
 */
static inline void
pchtml_array_obj_erase(pchtml_array_obj_t *array)
{
    memset(array, 0, sizeof(pchtml_array_obj_t));
}

static inline void *
pchtml_array_obj_get(pchtml_array_obj_t *array, size_t idx)
{
    if (idx >= array->length) {
        return NULL;
    }

    return array->list + (idx * array->struct_size);
}

static inline size_t
pchtml_array_obj_length(pchtml_array_obj_t *array)
{
    return array->length;
}

static inline size_t
pchtml_array_obj_size(pchtml_array_obj_t *array)
{
    return array->size;
}

static inline size_t
pchtml_array_obj_struct_size(pchtml_array_obj_t *array)
{
    return array->struct_size;
}

static inline void *
pchtml_array_obj_last(pchtml_array_obj_t *array)
{
    if (array->length == 0) {
        return NULL;
    }

    return array->list + ((array->length - 1) * array->struct_size);
}


/*
 * No inline functions for ABI.
 */
void
pchtml_array_obj_erase_noi(pchtml_array_obj_t *array) WTF_INTERNAL;

void *
pchtml_array_obj_get_noi(pchtml_array_obj_t *array, size_t idx) WTF_INTERNAL;

size_t
pchtml_array_obj_length_noi(pchtml_array_obj_t *array) WTF_INTERNAL;

size_t
pchtml_array_obj_size_noi(pchtml_array_obj_t *array) WTF_INTERNAL;

size_t
pchtml_array_obj_struct_size_noi(pchtml_array_obj_t *array) WTF_INTERNAL;

void *
pchtml_array_obj_last_noi(pchtml_array_obj_t *array) WTF_INTERNAL;


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_ARRAY_OBJ_H */
