/**
 * @file mem.c
 * @author 
 * @date 2021/07/02
 * @brief The complementation of memory operation.
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

#include "html/core/mem.h"


pchtml_mem_t *
pchtml_mem_create(void)
{
    return pchtml_calloc(1, sizeof(pchtml_mem_t));
}

unsigned int
pchtml_mem_init(pchtml_mem_t *mem, size_t min_chunk_size)
{
    if (mem == NULL) {
        return PCHTML_STATUS_ERROR_OBJECT_IS_NULL;
    }

    if (min_chunk_size == 0) {
        return PCHTML_STATUS_ERROR_WRONG_ARGS;
    }

    mem->chunk_min_size = pchtml_mem_align(min_chunk_size);

    /* Create first chunk */
    mem->chunk = pchtml_mem_chunk_make(mem, mem->chunk_min_size);
    if (mem->chunk == NULL) {
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }

    mem->chunk_length = 1;
    mem->chunk_first = mem->chunk;

    return PCHTML_STATUS_OK;
}

void
pchtml_mem_clean(pchtml_mem_t *mem)
{
    pchtml_mem_chunk_t *prev;
    pchtml_mem_chunk_t *chunk = mem->chunk;

    while (chunk->prev) {
        prev = chunk->prev;

        chunk->data = pchtml_free(chunk->data);
        pchtml_free(chunk);

        chunk = prev;
    }

    chunk->next = NULL;
    chunk->length = 0;

    mem->chunk = mem->chunk_first;
    mem->chunk_length = 1;
}

pchtml_mem_t *
pchtml_mem_destroy(pchtml_mem_t *mem, bool destroy_self)
{
    pchtml_mem_chunk_t *chunk, *prev;

    if (mem == NULL) {
        return NULL;
    }

    /* Destroy all chunk */
    if (mem->chunk) {
        chunk = mem->chunk;

        while (chunk) {
            prev = chunk->prev;
            pchtml_mem_chunk_destroy(mem, chunk, true);
            chunk = prev;
        }

        mem->chunk = NULL;
    }

    if (destroy_self) {
        return pchtml_free(mem);
    }

    return mem;
}

uint8_t *
pchtml_mem_chunk_init(pchtml_mem_t *mem,
                      pchtml_mem_chunk_t *chunk, size_t length)
{
    length = pchtml_mem_align(length);

    if (length > mem->chunk_min_size) {
        if (mem->chunk_min_size > (SIZE_MAX - length)) {
            chunk->size = length;
        }
        else {
            chunk->size = length + mem->chunk_min_size;
        }
    }
    else {
        chunk->size = mem->chunk_min_size;
    }

    chunk->length = 0;
    chunk->data = pchtml_malloc(chunk->size * sizeof(uint8_t));

    return chunk->data;
}

pchtml_mem_chunk_t *
pchtml_mem_chunk_make(pchtml_mem_t *mem, size_t length)
{
    pchtml_mem_chunk_t *chunk = pchtml_calloc(1, sizeof(pchtml_mem_chunk_t));

    if (chunk == NULL) {
        return NULL;
    }

    if (pchtml_mem_chunk_init(mem, chunk, length) == NULL) {
        return pchtml_free(chunk);
    }

    return chunk;
}

pchtml_mem_chunk_t *
pchtml_mem_chunk_destroy(pchtml_mem_t *mem,
                         pchtml_mem_chunk_t *chunk, bool self_destroy)
{
    if (chunk == NULL || mem == NULL) {
        return NULL;
    }

    if (chunk->data) {
        chunk->data = pchtml_free(chunk->data);
    }

    if (self_destroy) {
        return pchtml_free(chunk);
    }

    return chunk;
}

void *
pchtml_mem_alloc(pchtml_mem_t *mem, size_t length)
{
    if (length == 0) {
        return NULL;
    }

    length = pchtml_mem_align(length);

    if ((mem->chunk->length + length) > mem->chunk->size) {
        if ((SIZE_MAX - mem->chunk_length) == 0) {
            return NULL;
        }

        mem->chunk->next = pchtml_mem_chunk_make(mem, length);
        if (mem->chunk->next == NULL) {
            return NULL;
        }

        mem->chunk->next->prev = mem->chunk;
        mem->chunk = mem->chunk->next;

        mem->chunk_length++;
    }

    mem->chunk->length += length;

    return &mem->chunk->data[(mem->chunk->length - length)];
}

void *
pchtml_mem_calloc(pchtml_mem_t *mem, size_t length)
{
    void *data = pchtml_mem_alloc(mem, length);

    if (data != NULL) {
        memset(data, 0, length);
    }

    return data;
}

/*
 * No inline functions for ABI.
 */
size_t
pchtml_mem_current_length_noi(pchtml_mem_t *mem)
{
    return pchtml_mem_current_length(mem);
}

size_t
pchtml_mem_current_size_noi(pchtml_mem_t *mem)
{
    return pchtml_mem_current_size(mem);
}

size_t
pchtml_mem_chunk_length_noi(pchtml_mem_t *mem)
{
    return pchtml_mem_chunk_length(mem);
}
size_t
pchtml_mem_align_noi(size_t size)
{
    return pchtml_mem_align(size);
}

size_t
pchtml_mem_align_floor_noi(size_t size)
{
    return pchtml_mem_align_floor(size);
}
