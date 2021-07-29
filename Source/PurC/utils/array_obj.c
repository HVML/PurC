/**
 * @file array_obj.c
 * @author 
 * @date 2021/07/02
 * @brief The complementation of array object.
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

#include "purc.h"
#include "config.h"
#include "private/instance.h"
#include "private/errors.h"
#include "private/array_obj.h"


pcutils_array_obj_t *
pcutils_array_obj_create(void)
{
    return pchtml_calloc(1, sizeof(pcutils_array_obj_t));
}

unsigned int
pcutils_array_obj_init(pcutils_array_obj_t *array,
                      size_t size, size_t struct_size)
{
    if (array == NULL) {
        pcinst_set_error (PCHTML_OBJECT_IS_NULL);
        return PCHTML_STATUS_ERROR_OBJECT_IS_NULL;
    }

    if (size == 0 || struct_size == 0) {
        pcinst_set_error (PCHTML_TOO_SMALL_SIZE);
        return PCHTML_STATUS_ERROR_TOO_SMALL_SIZE;
    }

    array->length = 0;
    array->size = size;
    array->struct_size = struct_size;

    array->list = pchtml_malloc(sizeof(uint8_t *)
                                * (array->size * struct_size));
    if (array->list == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }

    return PCHTML_STATUS_OK;
}

void
pcutils_array_obj_clean(pcutils_array_obj_t *array)
{
    array->length = 0;
}

pcutils_array_obj_t *
pcutils_array_obj_destroy(pcutils_array_obj_t *array, bool self_destroy)
{
    if (array == NULL)
        return NULL;

    if (array->list) {
        array->length = 0;
        array->size = 0;
        array->list = pchtml_free(array->list);
    }

    if (self_destroy) {
        return pchtml_free(array);
    }

    return array;
}

uint8_t *
pcutils_array_obj_expand(pcutils_array_obj_t *array, size_t up_to)
{
    uint8_t *list;
    size_t new_size;

    if (array->length > (SIZE_MAX - up_to)) {
        return NULL;
    }

    new_size = array->length + up_to;

    list = pchtml_realloc(array->list, sizeof(uint8_t *)
                          * (new_size * array->struct_size));
    if (list == NULL) {
        return NULL;
    }

    array->list = list;
    array->size = new_size;

    return list;
}

void *
pcutils_array_obj_push(pcutils_array_obj_t *array)
{
    void *entry;

    if (array->length >= array->size)
    {
        if ((pcutils_array_obj_expand(array, 128) == NULL)) {
            return NULL;
        }
    }

    entry = array->list + (array->length * array->struct_size);
    array->length++;

    memset(entry, 0, sizeof(array->struct_size));

    return entry;
}

void *
pcutils_array_obj_pop(pcutils_array_obj_t *array)
{
    if (array->length == 0) {
        return NULL;
    }

    array->length--;
    return array->list + (array->length * array->struct_size);
}

void
pcutils_array_obj_delete(pcutils_array_obj_t *array, size_t begin, size_t length)
{
    if (begin >= array->length || length == 0) {
        return;
    }

    size_t end_len = begin + length;

    if (end_len >= array->length) {
        array->length = begin;
        return;
    }

    memmove(&array->list[ begin * array->struct_size ],
            &array->list[ end_len * array->struct_size ],
            sizeof(uint8_t *)
            * ((array->length - end_len) * array->struct_size));

    array->length -= length;
}

/*
 * No inline functions.
 */
void
pcutils_array_obj_erase_noi(pcutils_array_obj_t *array)
{
    pcutils_array_obj_erase(array);
}

void *
pcutils_array_obj_get_noi(pcutils_array_obj_t *array, size_t idx)
{
    return pcutils_array_obj_get(array, idx);
}

size_t
pcutils_array_obj_length_noi(pcutils_array_obj_t *array)
{
    return pcutils_array_obj_length(array);
}

size_t
pcutils_array_obj_size_noi(pcutils_array_obj_t *array)
{
    return pcutils_array_obj_size(array);
}

size_t
pcutils_array_obj_struct_size_noi(pcutils_array_obj_t *array)
{
    return pcutils_array_obj_struct_size(array);
}

void *
pcutils_array_obj_last_noi(pcutils_array_obj_t *array)
{
    return pcutils_array_obj_last(array);
}
