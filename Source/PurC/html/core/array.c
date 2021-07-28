/**
 * @file array.c
 * @author 
 * @date 2021/07/02
 * @brief The complementation of array.
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
#include "html/core/array.h"

pchtml_array_t * pchtml_array_create(void)
{
    return pchtml_calloc(1, sizeof(pchtml_array_t));
}

unsigned int pchtml_array_init(pchtml_array_t *array, size_t size)
{
    if (array == NULL) {
        pcinst_set_error (PCHTML_OBJECT_IS_NULL);
        return PCHTML_STATUS_ERROR_OBJECT_IS_NULL;
    }

    if (size == 0) {
        pcinst_set_error (PCHTML_TOO_SMALL_SIZE);
        return PCHTML_STATUS_ERROR_TOO_SMALL_SIZE;
    }

    array->length = 0;
    array->size = size;

    array->list = pchtml_malloc(sizeof(void *) * size);
    if (array->list == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }

    return PCHTML_STATUS_OK;
}

void pchtml_array_clean(pchtml_array_t *array)
{
    array->length = 0;
}

pchtml_array_t * pchtml_array_destroy(pchtml_array_t *array, bool self_destroy)
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

void ** pchtml_array_expand(pchtml_array_t *array, size_t up_to)
{
    void **list;
    size_t new_size;

    if (array->length > (SIZE_MAX - up_to))
        return NULL;

    new_size = array->length + up_to;
    list = pchtml_realloc(array->list, sizeof(void *) * new_size);

    if (list == NULL)
        return NULL;

    array->list = list;
    array->size = new_size;

    return list;
}

unsigned int pchtml_array_push(pchtml_array_t *array, void *value)
{
    if (array->length >= array->size) {
        if ((pchtml_array_expand(array, 128) == NULL)) {
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }
    }

    array->list[ array->length ] = value;
    array->length++;

    return PCHTML_STATUS_OK;
}

void * pchtml_array_pop(pchtml_array_t *array)
{
    if (array->length == 0) {
        return NULL;
    }

    array->length--;
    return array->list[ array->length ];
}

unsigned int pchtml_array_insert(pchtml_array_t *array, size_t idx, void *value)
{
    if (idx >= array->length) {
        size_t up_to = (idx - array->length) + 1;

        if (idx >= array->size) {
            if ((pchtml_array_expand(array, up_to) == NULL)) {
                pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
                return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
            }
        }

        memset(&array->list[array->length], 0, sizeof(void *) * up_to);

        array->list[ idx ] = value;
        array->length += up_to;

        return PCHTML_STATUS_OK;
    }

    if (array->length >= array->size) {
        if ((pchtml_array_expand(array, 32) == NULL)) {
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }
    }

    memmove(&array->list[idx + 1], &array->list[idx],
            sizeof(void *) * (array->length - idx));

    array->list[ idx ] = value;
    array->length++;

    return PCHTML_STATUS_OK;
}

unsigned int 
pchtml_array_set(pchtml_array_t *array, size_t idx, void *value)
{
    if (idx >= array->length) {
        size_t up_to = (idx - array->length) + 1;

        if (idx >= array->size) {
            if ((pchtml_array_expand(array, up_to) == NULL)) {
                pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
                return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
            }
        }

        memset(&array->list[array->length], 0, sizeof(void *) * up_to);

        array->length += up_to;
    }

    array->list[idx] = value;

    return PCHTML_STATUS_OK;
}

void pchtml_array_delete(pchtml_array_t *array, size_t begin, size_t length)
{
    if (begin >= array->length || length == 0) {
        return;
    }

    size_t end_len = begin + length;

    if (end_len >= array->length) {
        array->length = begin;
        return;
    }

    memmove(&array->list[begin], &array->list[end_len],
            sizeof(void *) * (array->length - end_len));

    array->length -= length;
}

/*
 * No inline functions.
 */
void * pchtml_array_get_noi(pchtml_array_t *array, size_t idx)
{
    return pchtml_array_get(array, idx);
}

size_t pchtml_array_length_noi(pchtml_array_t *array)
{
    return pchtml_array_length(array);
}

size_t pchtml_array_size_noi(pchtml_array_t *array)
{
    return pchtml_array_size(array);
}
