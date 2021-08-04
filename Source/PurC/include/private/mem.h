/**
 * @file mem.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for memory operation.
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


#ifndef PCHTML_MEM_H
#define PCHTML_MEM_H

#include "config.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define PCHTML_MEM_ALIGN_STEP sizeof(void *)

#define pchtml_malloc(sz)       malloc(sz)
#define pchtml_realloc(ptr, sz) realloc(ptr, sz)
#define pchtml_calloc(n, sz)    calloc(n, sz)

static inline void*
pchtml_free(void *ptr)
{
    free(ptr);
    return NULL;
}

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pchtml_mem_chunk pchtml_mem_chunk_t;
typedef struct pchtml_mem pchtml_mem_t;

struct pchtml_mem_chunk {
    uint8_t            *data;
    size_t             length;
    size_t             size;

    pchtml_mem_chunk_t *next;
    pchtml_mem_chunk_t *prev;
};

struct pchtml_mem {
    pchtml_mem_chunk_t *chunk;
    pchtml_mem_chunk_t *chunk_first;

    size_t             chunk_min_size;
    size_t             chunk_length;
};


pchtml_mem_t *
pchtml_mem_create(void) WTF_INTERNAL;

unsigned int
pchtml_mem_init(pchtml_mem_t *mem, size_t min_chunk_size) WTF_INTERNAL;

void
pchtml_mem_clean(pchtml_mem_t *mem) WTF_INTERNAL;

pchtml_mem_t *
pchtml_mem_destroy(pchtml_mem_t *mem, bool destroy_self) WTF_INTERNAL;


/*
 * The memory allocated in pchtml_mem_chunk_* functions needs to be freed
 * by pchtml_mem_chunk_destroy function.
 *
 * This memory will not be automatically freed by a function pchtml_mem_destroy.
 */
uint8_t *
pchtml_mem_chunk_init(pchtml_mem_t *mem,
                pchtml_mem_chunk_t *chunk, size_t length) WTF_INTERNAL;

pchtml_mem_chunk_t *
pchtml_mem_chunk_make(pchtml_mem_t *mem, size_t length) WTF_INTERNAL;

pchtml_mem_chunk_t *
pchtml_mem_chunk_destroy(pchtml_mem_t *mem,
                pchtml_mem_chunk_t *chunk, bool self_destroy) WTF_INTERNAL;

/*
 * The memory allocated in pchtml_mem_alloc and pchtml_mem_calloc function
 * will be freeds after calling pchtml_mem_destroy function.
 */
void *
pchtml_mem_alloc(pchtml_mem_t *mem, size_t length) WTF_INTERNAL;

void *
pchtml_mem_calloc(pchtml_mem_t *mem, size_t length) WTF_INTERNAL;


/*
 * Inline functions
 */
static inline size_t
pchtml_mem_current_length(pchtml_mem_t *mem)
{
    return mem->chunk->length;
}

static inline size_t
pchtml_mem_current_size(pchtml_mem_t *mem)
{
    return mem->chunk->size;
}

static inline size_t
pchtml_mem_chunk_length(pchtml_mem_t *mem)
{
    return mem->chunk_length;
}

static inline size_t
pchtml_mem_align(size_t size)
{
    return ((size % PCHTML_MEM_ALIGN_STEP) != 0)
           ? size + (PCHTML_MEM_ALIGN_STEP - (size % PCHTML_MEM_ALIGN_STEP))
           : size;
}

static inline size_t
pchtml_mem_align_floor(size_t size)
{
    return ((size % PCHTML_MEM_ALIGN_STEP) != 0)
           ? size - (size % PCHTML_MEM_ALIGN_STEP)
           : size;
}

#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_MEM_H */
