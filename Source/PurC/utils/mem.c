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
#include "private/instance.h"
#include "private/errors.h"
#include "private/mem.h"


pcutils_mem_t *
pcutils_mem_create(void)
{
    return pchtml_calloc(1, sizeof(pcutils_mem_t));
}

unsigned int
pcutils_mem_init(pcutils_mem_t *mem, size_t min_chunk_size)
{
    if (mem == NULL) {
        return PURC_ERROR_NULL_OBJECT;
    }

    if (min_chunk_size == 0) {
        return PURC_ERROR_INVALID_VALUE;
    }

    mem->chunk_min_size = pcutils_mem_align(min_chunk_size);

    /* Create first chunk */
    mem->chunk = pcutils_mem_chunk_make(mem, mem->chunk_min_size);
    if (mem->chunk == NULL) {
        return PURC_ERROR_OUT_OF_MEMORY;
    }

    mem->chunk_length = 1;
    mem->chunk_first = mem->chunk;

    return PURC_ERROR_OK;
}

void
pcutils_mem_clean(pcutils_mem_t *mem)
{
    pcutils_mem_chunk_t *prev;
    pcutils_mem_chunk_t *chunk = mem->chunk;

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

pcutils_mem_t *
pcutils_mem_destroy(pcutils_mem_t *mem, bool destroy_self)
{
    pcutils_mem_chunk_t *chunk, *prev;

    if (mem == NULL) {
        return NULL;
    }

    /* Destroy all chunk */
    if (mem->chunk) {
        chunk = mem->chunk;

        while (chunk) {
            prev = chunk->prev;
            pcutils_mem_chunk_destroy(mem, chunk, true);
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
pcutils_mem_chunk_init(pcutils_mem_t *mem,
                      pcutils_mem_chunk_t *chunk, size_t length)
{
    length = pcutils_mem_align(length);

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

pcutils_mem_chunk_t *
pcutils_mem_chunk_make(pcutils_mem_t *mem, size_t length)
{
    pcutils_mem_chunk_t *chunk = pchtml_calloc(1, sizeof(pcutils_mem_chunk_t));

    if (chunk == NULL) {
        return NULL;
    }

    if (pcutils_mem_chunk_init(mem, chunk, length) == NULL) {
        return pchtml_free(chunk);
    }

    return chunk;
}

pcutils_mem_chunk_t *
pcutils_mem_chunk_destroy(pcutils_mem_t *mem,
                         pcutils_mem_chunk_t *chunk, bool self_destroy)
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
pcutils_mem_alloc(pcutils_mem_t *mem, size_t length)
{
    if (length == 0) {
        return NULL;
    }

    length = pcutils_mem_align(length);

    if ((mem->chunk->length + length) > mem->chunk->size) {
        if ((SIZE_MAX - mem->chunk_length) == 0) {
            return NULL;
        }

        mem->chunk->next = pcutils_mem_chunk_make(mem, length);
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
pcutils_mem_calloc(pcutils_mem_t *mem, size_t length)
{
    void *data = pcutils_mem_alloc(mem, length);

    if (data != NULL) {
        memset(data, 0, length);
    }

    return data;
}
