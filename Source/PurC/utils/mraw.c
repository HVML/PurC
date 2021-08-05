/**
 * @file mraw.c
 * @author 
 * @date 2021/07/02
 * @brief The complementation of mraw.
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
#include "private/mraw.h"


#if defined(PCHTML_HAVE_ADDRESS_SANITIZER)
    #include <sanitizer/asan_interface.h>
#endif


#define pcutils_mraw_meta_set(data, size)                                       \
    do {                                                                       \
        memcpy(data, size, sizeof(size_t));                                    \
    }                                                                          \
    while (0)

#define pcutils_mraw_data_begin(data)                                           \
    &((uint8_t *) (data))[ pcutils_mraw_meta_size() ]


static inline void *
pcutils_mraw_realloc_tail(pcutils_mraw_t *mraw, void *data, void *begin,
                         size_t size, size_t begin_len, size_t new_size,
                         bool *is_valid);


pcutils_mraw_t *
pcutils_mraw_create(void)
{
    return pcutils_calloc(1, sizeof(pcutils_mraw_t));
}

unsigned int
pcutils_mraw_init(pcutils_mraw_t *mraw, size_t chunk_size)
{
    unsigned int status;

    if (mraw == NULL) {
        return PURC_ERROR_NULL_OBJECT;
    }

    if (chunk_size == 0) {
        return PURC_ERROR_INVALID_VALUE;
    }

    /* Init memory */
    mraw->mem = pcutils_mem_create();

    status = pcutils_mem_init(mraw->mem, chunk_size + pcutils_mraw_meta_size());
    if (status) {
        return status;
    }

#if defined(PCHTML_HAVE_ADDRESS_SANITIZER)
    ASAN_POISON_MEMORY_REGION(mraw->mem->chunk->data, mraw->mem->chunk->size);
#endif

    /* Cache */
    mraw->cache = pcutils_bst_create();

    status = pcutils_bst_init(mraw->cache, 512);
    if (status) {
        return status;
    }

    return PURC_ERROR_OK;
}

void
pcutils_mraw_clean(pcutils_mraw_t *mraw)
{
    pcutils_mem_clean(mraw->mem);
    pcutils_bst_clean(mraw->cache);
}

pcutils_mraw_t *
pcutils_mraw_destroy(pcutils_mraw_t *mraw, bool destroy_self)
{
    if (mraw == NULL) {
        return NULL;
    }

    mraw->mem = pcutils_mem_destroy(mraw->mem, true);
    mraw->cache = pcutils_bst_destroy(mraw->cache, true);

    if (destroy_self) {
        return pcutils_free(mraw);
    }

    return mraw;
}

static inline void *
pcutils_mraw_mem_alloc(pcutils_mraw_t *mraw, size_t length)
{
    uint8_t *data;
    pcutils_mem_t *mem = mraw->mem;

    if (length == 0) {
        return NULL;
    }

    if ((mem->chunk->length + length) > mem->chunk->size) {
        pcutils_mem_chunk_t *chunk = mem->chunk;

        if ((SIZE_MAX - mem->chunk_length) == 0) {
            return NULL;
        }

        if (chunk->length == 0) {
            pcutils_mem_chunk_destroy(mem, chunk, false);
            pcutils_mem_chunk_init(mem, chunk, length);

            chunk->length = length;

#if defined(PCHTML_HAVE_ADDRESS_SANITIZER)
            ASAN_POISON_MEMORY_REGION(chunk->data, chunk->size);
#endif

            return chunk->data;
        }

        size_t diff = pcutils_mem_align_floor(chunk->size - chunk->length);

        /* Save tail to cache */
        if (diff > pcutils_mraw_meta_size()) {
            diff -= pcutils_mraw_meta_size();

#if defined(PCHTML_HAVE_ADDRESS_SANITIZER)
            ASAN_UNPOISON_MEMORY_REGION(&chunk->data[chunk->length],
                                        pcutils_mraw_meta_size());
#endif

            pcutils_mraw_meta_set(&chunk->data[chunk->length], &diff);

#if defined(PCHTML_HAVE_ADDRESS_SANITIZER)
            ASAN_POISON_MEMORY_REGION(&chunk->data[chunk->length],
                                      diff + pcutils_mraw_meta_size());
#endif

            pcutils_bst_insert(mraw->cache,
                              pcutils_bst_root_ref(mraw->cache), diff,
                              pcutils_mraw_data_begin(&chunk->data[chunk->length]));

            chunk->length = chunk->size;
        }

        chunk->next = pcutils_mem_chunk_make(mem, length);
        if (chunk->next == NULL) {
            return NULL;
        }

        chunk->next->prev = chunk;
        mem->chunk = chunk->next;

        mem->chunk_length++;

#if defined(PCHTML_HAVE_ADDRESS_SANITIZER)
        ASAN_POISON_MEMORY_REGION(mem->chunk->data, mem->chunk->size);
#endif
    }

    data = &mem->chunk->data[ mem->chunk->length ];
    mem->chunk->length += length;

    return data;
}

void *
pcutils_mraw_alloc(pcutils_mraw_t *mraw, size_t size)
{
    void *data;

    size = pcutils_mem_align(size);

    if (mraw->cache->tree_length != 0) {
        data = pcutils_bst_remove_close(mraw->cache,
                                       pcutils_bst_root_ref(mraw->cache),
                                       size, NULL);
        if (data != NULL) {

#if defined(PCHTML_HAVE_ADDRESS_SANITIZER)
            uint8_t *real_data = ((uint8_t *) data) - pcutils_mraw_meta_size();

            /* Set unpoison for current data size */
            ASAN_UNPOISON_MEMORY_REGION(real_data, size);

            size_t cur_size = pcutils_mraw_data_size(data);

            ASAN_UNPOISON_MEMORY_REGION(real_data,
                                        (cur_size + pcutils_mraw_meta_size()));
#endif

            return data;
        }
    }

    data = pcutils_mraw_mem_alloc(mraw, (size + pcutils_mraw_meta_size()));

    if (data == NULL) {
        return NULL;
    }

#if defined(PCHTML_HAVE_ADDRESS_SANITIZER)
    ASAN_UNPOISON_MEMORY_REGION(data, (size + pcutils_mraw_meta_size()));
#endif

    pcutils_mraw_meta_set(data, &size);
    return pcutils_mraw_data_begin(data);
}

void *
pcutils_mraw_calloc(pcutils_mraw_t *mraw, size_t size)
{
    void *data = pcutils_mraw_alloc(mraw, size);

    if (data != NULL) {
        memset(data, 0, pcutils_mraw_data_size(data));
    }

    return data;
}

/*
 * TODO: I don't really like this interface. Perhaps need to simplify.
 */
static inline void *
pcutils_mraw_realloc_tail(pcutils_mraw_t *mraw, void *data, void *begin,
                         size_t size, size_t begin_len, size_t new_size,
                         bool *is_valid)
{
    pcutils_mem_chunk_t *chunk = mraw->mem->chunk;

    if (chunk->size > (begin_len + new_size)) {
        *is_valid = true;

        if (new_size == 0) {
            chunk->length = begin_len - pcutils_mraw_meta_size();
            return NULL;
        }

#if defined(PCHTML_HAVE_ADDRESS_SANITIZER)
        ASAN_UNPOISON_MEMORY_REGION(begin, new_size + pcutils_mraw_meta_size());
#endif

        chunk->length = begin_len + new_size;
        memcpy(begin, &new_size, sizeof(size_t));

        return data;
    }

    /*
     * If the tail is short then we increase the current data.
     */
    if (begin_len == pcutils_mraw_meta_size()) {
        void *new_data;
        pcutils_mem_chunk_t new_chunk;

        *is_valid = true;

        pcutils_mem_chunk_init(mraw->mem, &new_chunk,
                              new_size + pcutils_mraw_meta_size());
        if(new_chunk.data == NULL) {
            return NULL;
        }

        pcutils_mraw_meta_set(new_chunk.data, &new_size);
        new_data = pcutils_mraw_data_begin(new_chunk.data);

        if (size != 0) {
            memcpy(new_data, data, sizeof(uint8_t) * size);
        }

#if defined(PCHTML_HAVE_ADDRESS_SANITIZER)
        ASAN_UNPOISON_MEMORY_REGION(chunk->data, chunk->size);
#endif

        pcutils_mem_chunk_destroy(mraw->mem, chunk, false);

        chunk->data = new_chunk.data;
        chunk->size = new_chunk.size;
        chunk->length = new_size + pcutils_mraw_meta_size();

        return new_data;
    }

    *is_valid = false;

    /*
     * Next, this piece will go into the cache.
     */
    size = pcutils_mem_align_floor(size + (chunk->size - chunk->length));
    memcpy(begin, &size, sizeof(size_t));

    chunk->length = chunk->size;

    return NULL;
}

void *
pcutils_mraw_realloc(pcutils_mraw_t *mraw, void *data, size_t new_size)
{
    void *begin;
    size_t size, begin_len;
    pcutils_mem_chunk_t *chunk = mraw->mem->chunk;

    begin = ((uint8_t *) data) - pcutils_mraw_meta_size();
    memcpy(&size, begin, sizeof(size_t));

    new_size = pcutils_mem_align(new_size);

    /*
     * Look, whether there is an opportunity
     * to prolong the current data in chunk?
     */
    if (chunk->length >= size) {
        begin_len = chunk->length - size;

        if (&chunk->data[begin_len] == data) {
            bool is_valid;
            void *ptr = pcutils_mraw_realloc_tail(mraw, data, begin,
                                                 size, begin_len, new_size,
                                                 &is_valid);
            if (is_valid == true) {
                return ptr;
            }
        }
    }

    if (new_size < size) {
        if (new_size == 0) {

#if defined(PCHTML_HAVE_ADDRESS_SANITIZER)
            ASAN_POISON_MEMORY_REGION(begin, size + pcutils_mraw_meta_size());
#endif
            pcutils_bst_insert(mraw->cache, pcutils_bst_root_ref(mraw->cache),
                              size, data);
            return NULL;
        }

        size_t diff = pcutils_mem_align_floor(size - new_size);

        if (diff > pcutils_mraw_meta_size()) {
            memcpy(begin, &new_size, sizeof(size_t));

            new_size = diff - pcutils_mraw_meta_size();
            begin = &((uint8_t *) data)[diff];

            pcutils_mraw_meta_set(begin, &new_size);

#if defined(PCHTML_HAVE_ADDRESS_SANITIZER)
            ASAN_POISON_MEMORY_REGION(begin, new_size + pcutils_mraw_meta_size());
#endif
            pcutils_bst_insert(mraw->cache, pcutils_bst_root_ref(mraw->cache),
                              new_size, pcutils_mraw_data_begin(begin));
        }

        return data;
    }

    begin = pcutils_mraw_alloc(mraw, new_size);
    if (begin == NULL) {
        return NULL;
    }

    if (size != 0) {
        memcpy(begin, data, sizeof(uint8_t) * size);
    }

    pcutils_mraw_free(mraw, data);

    return begin;
}

void *
pcutils_mraw_free(pcutils_mraw_t *mraw, void *data)
{
    size_t size = pcutils_mraw_data_size(data);

#if defined(PCHTML_HAVE_ADDRESS_SANITIZER)
    uint8_t *real_data = ((uint8_t *) data) - pcutils_mraw_meta_size();
    ASAN_POISON_MEMORY_REGION(real_data, size + pcutils_mraw_meta_size());
#endif

    pcutils_bst_insert(mraw->cache, pcutils_bst_root_ref(mraw->cache),
                      size, data);

    return NULL;
}
