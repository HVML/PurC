/**
 * @file dobject.c
 * @author 
 * @date 2021/07/02
 * @brief The complementation of object.
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
 * This implementation of HTML parser is derived from Lexbor
 * <https://github.com/lexbor/lexbor>, which is licensed under the Apache
 * License, Version 2.0:
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

#include "purc.h"
#include "config.h"
#include "private/dobject.h"


#if defined(PCHTML_HAVE_ADDRESS_SANITIZER)
    #include <sanitizer/asan_interface.h>
#endif


pcutils_dobject_t *
pcutils_dobject_create(void)
{
    return pcutils_calloc(1, sizeof(pcutils_dobject_t));
}

unsigned int
pcutils_dobject_init(pcutils_dobject_t *dobject,
                    size_t chunk_size, size_t struct_size)
{
    unsigned int status;

    if (dobject == NULL) {
        return PURC_ERROR_NULL_OBJECT;
    }

    if (chunk_size == 0 || struct_size == 0) {
        return PURC_ERROR_INVALID_VALUE;
    }

    /* Set params */
    dobject->allocated = 0UL;
    dobject->struct_size = struct_size;

    /* Init memory */
    dobject->mem = pcutils_mem_create();

    status = pcutils_mem_init(dobject->mem,
                           pcutils_mem_align(chunk_size * dobject->struct_size));
    if (status) {
        return status;
    }

#if defined(PCHTML_HAVE_ADDRESS_SANITIZER)
    ASAN_POISON_MEMORY_REGION(dobject->mem->chunk->data,
                              dobject->mem->chunk->size);
#endif

    /* Array */
    dobject->cache = pcutils_array_create();

    status = pcutils_array_init(dobject->cache, chunk_size);
    if (status)
        return status;

    return PURC_ERROR_OK;
}

void
pcutils_dobject_clean(pcutils_dobject_t *dobject)
{
    dobject->allocated = 0UL;

    pcutils_mem_clean(dobject->mem);
    pcutils_array_clean(dobject->cache);
}

pcutils_dobject_t *
pcutils_dobject_destroy(pcutils_dobject_t *dobject, bool destroy_self)
{
    if (dobject == NULL)
        return NULL;

    dobject->mem = pcutils_mem_destroy(dobject->mem, true);
    dobject->cache = pcutils_array_destroy(dobject->cache, true);

    if (destroy_self == true) {
        return pcutils_free(dobject);
    }

    return dobject;
}

void *
pcutils_dobject_alloc(pcutils_dobject_t *dobject)
{
    void *data;

    if (pcutils_array_length(dobject->cache) != 0) {
        dobject->allocated++;

#if defined(PCHTML_HAVE_ADDRESS_SANITIZER)
        data = pcutils_array_pop(dobject->cache);
        ASAN_UNPOISON_MEMORY_REGION(data, dobject->struct_size);

        return data;
#else
        return pcutils_array_pop(dobject->cache);
#endif
    }

    data = pcutils_mem_alloc(dobject->mem, dobject->struct_size);
    if (data == NULL) {
        return NULL;
    }

#if defined(PCHTML_HAVE_ADDRESS_SANITIZER)
    ASAN_UNPOISON_MEMORY_REGION(data, dobject->struct_size);
#endif

    dobject->allocated++;

    return data;
}

void *
pcutils_dobject_calloc(pcutils_dobject_t *dobject)
{
    void *data = pcutils_dobject_alloc(dobject);

    if (data != NULL) {
        memset(data, 0, dobject->struct_size);
    }

    return data;
}

void *
pcutils_dobject_free(pcutils_dobject_t *dobject, void *data)
{
    if (data == NULL) {
        return NULL;
    }

#if defined(PCHTML_HAVE_ADDRESS_SANITIZER)
    ASAN_POISON_MEMORY_REGION(data, dobject->struct_size);
#endif

    if (pcutils_array_push(dobject->cache, data) == PURC_ERROR_OK) {
        dobject->allocated--;
        return NULL;
    }

    return data;
}

void *
pcutils_dobject_by_absolute_position(pcutils_dobject_t *dobject, size_t pos)
{
    size_t chunk_idx, chunk_pos, i;
    pcutils_mem_chunk_t *chunk;

    if (pos >= dobject->allocated) {
        return NULL;
    }

    chunk = dobject->mem->chunk_first;
    chunk_pos = pos * dobject->struct_size;
    chunk_idx = chunk_pos / dobject->mem->chunk_min_size;

    for (i = 0; i < chunk_idx; i++) {
        chunk = chunk->next;
    }

    return &chunk->data[chunk_pos % chunk->size];
}
