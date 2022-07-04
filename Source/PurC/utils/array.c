/*
 * @file array.c
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
 *
 * This code is derived from Lexbor (<https://github.com/lexbor/lexbor>),
 * which is licensed under the Apache License, Version 2.0.
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

#include "config.h"
#include "purc-errors.h"
#include "private/mem.h"
#include "private/array.h"

#include <string.h>

pcutils_array_t * pcutils_array_create(void)
{
    return pcutils_calloc(1, sizeof(pcutils_array_t));
}

unsigned int pcutils_array_init(pcutils_array_t *array, size_t size)
{
    if (array == NULL) {
        return PURC_ERROR_NULL_OBJECT;
    }

    if (size == 0) {
        return PURC_ERROR_TOO_SMALL_SIZE;
    }

    array->length = 0;
    array->size = size;

    array->list = pcutils_malloc(sizeof(void *) * size);
    if (array->list == NULL) {
        return PURC_ERROR_OUT_OF_MEMORY;
    }

    return PURC_ERROR_OK;
}

void pcutils_array_clean(pcutils_array_t *array)
{
    array->length = 0;
}

pcutils_array_t *
pcutils_array_destroy(pcutils_array_t *array, bool self_destroy)
{
    if (array == NULL)
        return NULL;

    if (array->list) {
        array->length = 0;
        array->size = 0;
        array->list = pcutils_free(array->list);
    }

    if (self_destroy) {
        return pcutils_free(array);
    }

    return array;
}

void **pcutils_array_expand(pcutils_array_t *array, size_t up_to)
{
    void **list;
    size_t new_size;

    if (array->length > (SIZE_MAX - up_to))
        return NULL;

    new_size = array->length + up_to;
    list = pcutils_realloc(array->list, sizeof(void *) * new_size);

    if (list == NULL)
        return NULL;

    array->list = list;
    array->size = new_size;

    return list;
}

unsigned int pcutils_array_push(pcutils_array_t *array, void *value)
{
    if (array->length >= array->size) {
        if ((pcutils_array_expand(array, 8) == NULL)) {
            return PURC_ERROR_OUT_OF_MEMORY;
        }
    }

    array->list[array->length] = value;
    array->length++;

    return PURC_ERROR_OK;
}

void *
pcutils_array_pop(pcutils_array_t *array)
{
    if (array->length == 0) {
        return NULL;
    }

    array->length--;
    return array->list[ array->length ];
}

unsigned int
pcutils_array_insert(pcutils_array_t *array, size_t idx, void *value)
{
    if (idx >= array->length) {
        size_t up_to = (idx - array->length) + 1;

        if (idx >= array->size) {
            if ((pcutils_array_expand(array, up_to) == NULL)) {
                return PURC_ERROR_OUT_OF_MEMORY;
            }
        }

        memset(&array->list[array->length], 0, sizeof(void *) * up_to);

        array->list[ idx ] = value;
        array->length += up_to;

        return PURC_ERROR_OK;
    }

    if (array->length >= array->size) {
        if ((pcutils_array_expand(array, 8) == NULL)) {
            return PURC_ERROR_OUT_OF_MEMORY;
        }
    }

    memmove(&array->list[idx + 1], &array->list[idx],
            sizeof(void *) * (array->length - idx));

    array->list[ idx ] = value;
    array->length++;

    return PURC_ERROR_OK;
}

unsigned int
pcutils_array_set(pcutils_array_t *array, size_t idx, void *value)
{
    if (idx >= array->length) {
        size_t up_to = (idx - array->length) + 1;

        if (idx >= array->size) {
            if ((pcutils_array_expand(array, up_to) == NULL)) {
                return PURC_ERROR_OUT_OF_MEMORY;
            }
        }

        memset(&array->list[array->length], 0, sizeof(void *) * up_to);

        array->length += up_to;
    }

    array->list[idx] = value;

    return PURC_ERROR_OK;
}

void pcutils_array_delete(pcutils_array_t *array, size_t begin, size_t length)
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
