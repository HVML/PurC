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
 */

#include "purc.h"
#include "config.h"
#include "private/instance.h"
#include "private/errors.h"
#include "html/core/dobject.h"


#if defined(PCHTML_HAVE_ADDRESS_SANITIZER)
    #include <sanitizer/asan_interface.h>
#endif


pchtml_dobject_t *
pchtml_dobject_create(void)
{
    return pchtml_calloc(1, sizeof(pchtml_dobject_t));
}

unsigned int
pchtml_dobject_init(pchtml_dobject_t *dobject,
                    size_t chunk_size, size_t struct_size)
{
    unsigned int status;

    if (dobject == NULL) {
        pcinst_set_error (PCHTML_OBJECT_IS_NULL);
        return PCHTML_STATUS_ERROR_OBJECT_IS_NULL;
    }

    if (chunk_size == 0 || struct_size == 0) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PCHTML_STATUS_ERROR_WRONG_ARGS;
    }

    /* Set params */
    dobject->allocated = 0UL;
    dobject->struct_size = struct_size;

    /* Init memory */
    dobject->mem = pchtml_mem_create();

    status = pchtml_mem_init(dobject->mem,
                           pchtml_mem_align(chunk_size * dobject->struct_size));
    if (status) {
        return status;
    }

#if defined(PCHTML_HAVE_ADDRESS_SANITIZER)
    ASAN_POISON_MEMORY_REGION(dobject->mem->chunk->data,
                              dobject->mem->chunk->size);
#endif

    /* Array */
    dobject->cache = pchtml_array_create();

    status = pchtml_array_init(dobject->cache, chunk_size);
    if (status)
        return status;

    return PCHTML_STATUS_OK;
}

void
pchtml_dobject_clean(pchtml_dobject_t *dobject)
{
    dobject->allocated = 0UL;

    pchtml_mem_clean(dobject->mem);
    pchtml_array_clean(dobject->cache);
}

pchtml_dobject_t *
pchtml_dobject_destroy(pchtml_dobject_t *dobject, bool destroy_self)
{
    if (dobject == NULL)
        return NULL;

    dobject->mem = pchtml_mem_destroy(dobject->mem, true);
    dobject->cache = pchtml_array_destroy(dobject->cache, true);

    if (destroy_self == true) {
        return pchtml_free(dobject);
    }

    return dobject;
}

void *
pchtml_dobject_alloc(pchtml_dobject_t *dobject)
{
    void *data;

    if (pchtml_array_length(dobject->cache) != 0) {
        dobject->allocated++;

#if defined(PCHTML_HAVE_ADDRESS_SANITIZER)
        data = pchtml_array_pop(dobject->cache);
        ASAN_UNPOISON_MEMORY_REGION(data, dobject->struct_size);

        return data;
#else
        return pchtml_array_pop(dobject->cache);
#endif
    }

    data = pchtml_mem_alloc(dobject->mem, dobject->struct_size);
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
pchtml_dobject_calloc(pchtml_dobject_t *dobject)
{
    void *data = pchtml_dobject_alloc(dobject);

    if (data != NULL) {
        memset(data, 0, dobject->struct_size);
    }

    return data;
}

void *
pchtml_dobject_free(pchtml_dobject_t *dobject, void *data)
{
    if (data == NULL) {
        return NULL;
    }

#if defined(PCHTML_HAVE_ADDRESS_SANITIZER)
    ASAN_POISON_MEMORY_REGION(data, dobject->struct_size);
#endif

    if (pchtml_array_push(dobject->cache, data) == PCHTML_STATUS_OK) {
        dobject->allocated--;
        return NULL;
    }

    return data;
}

void *
pchtml_dobject_by_absolute_position(pchtml_dobject_t *dobject, size_t pos)
{
    size_t chunk_idx, chunk_pos, i;
    pchtml_mem_chunk_t *chunk;

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

/*
 * No inline functions for ABI.
 */
size_t
pchtml_dobject_allocated_noi(pchtml_dobject_t *dobject)
{
    return pchtml_dobject_allocated(dobject);
}

size_t
pchtml_dobject_cache_length_noi(pchtml_dobject_t *dobject)
{
    return pchtml_dobject_cache_length(dobject);
}
