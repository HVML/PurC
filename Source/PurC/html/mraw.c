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
 * <https://github.com/lexbor/lexbor>, which is licensed under Apahce 2.0:
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

#include "purc.h"
#include "config.h"
#include "private/instance.h"
#include "private/errors.h"
#include "html/mraw.h"


#if defined(PCHTML_HAVE_ADDRESS_SANITIZER)
    #include <sanitizer/asan_interface.h>
#endif


#define pchtml_mraw_meta_set(data, size)                                       \
    do {                                                                       \
        memcpy(data, size, sizeof(size_t));                                    \
    }                                                                          \
    while (0)

#define pchtml_mraw_data_begin(data)                                           \
    &((uint8_t *) (data))[ pchtml_mraw_meta_size() ]


static inline void *
pchtml_mraw_realloc_tail(pchtml_mraw_t *mraw, void *data, void *begin,
                         size_t size, size_t begin_len, size_t new_size,
                         bool *is_valid);


pchtml_mraw_t *
pchtml_mraw_create(void)
{
    return pchtml_calloc(1, sizeof(pchtml_mraw_t));
}

unsigned int
pchtml_mraw_init(pchtml_mraw_t *mraw, size_t chunk_size)
{
    unsigned int status;

    if (mraw == NULL) {
        pcinst_set_error (PCHTML_OBJECT_IS_NULL);
        return PCHTML_STATUS_ERROR_OBJECT_IS_NULL;
    }

    if (chunk_size == 0) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PCHTML_STATUS_ERROR_WRONG_ARGS;
    }

    /* Init memory */
    mraw->mem = pchtml_mem_create();

    status = pchtml_mem_init(mraw->mem, chunk_size + pchtml_mraw_meta_size());
    if (status) {
        return status;
    }

#if defined(PCHTML_HAVE_ADDRESS_SANITIZER)
    ASAN_POISON_MEMORY_REGION(mraw->mem->chunk->data, mraw->mem->chunk->size);
#endif

    /* Cache */
    mraw->cache = pchtml_bst_create();

    status = pchtml_bst_init(mraw->cache, 512);
    if (status) {
        return status;
    }

    return PCHTML_STATUS_OK;
}

void
pchtml_mraw_clean(pchtml_mraw_t *mraw)
{
    pchtml_mem_clean(mraw->mem);
    pchtml_bst_clean(mraw->cache);
}

pchtml_mraw_t *
pchtml_mraw_destroy(pchtml_mraw_t *mraw, bool destroy_self)
{
    if (mraw == NULL) {
        return NULL;
    }

    mraw->mem = pchtml_mem_destroy(mraw->mem, true);
    mraw->cache = pchtml_bst_destroy(mraw->cache, true);

    if (destroy_self) {
        return pchtml_free(mraw);
    }

    return mraw;
}

static inline void *
pchtml_mraw_mem_alloc(pchtml_mraw_t *mraw, size_t length)
{
    uint8_t *data;
    pchtml_mem_t *mem = mraw->mem;

    if (length == 0) {
        return NULL;
    }

    if ((mem->chunk->length + length) > mem->chunk->size) {
        pchtml_mem_chunk_t *chunk = mem->chunk;

        if ((SIZE_MAX - mem->chunk_length) == 0) {
            return NULL;
        }

        if (chunk->length == 0) {
            pchtml_mem_chunk_destroy(mem, chunk, false);
            pchtml_mem_chunk_init(mem, chunk, length);

            chunk->length = length;

#if defined(PCHTML_HAVE_ADDRESS_SANITIZER)
            ASAN_POISON_MEMORY_REGION(chunk->data, chunk->size);
#endif

            return chunk->data;
        }

        size_t diff = pchtml_mem_align_floor(chunk->size - chunk->length);

        /* Save tail to cache */
        if (diff > pchtml_mraw_meta_size()) {
            diff -= pchtml_mraw_meta_size();

#if defined(PCHTML_HAVE_ADDRESS_SANITIZER)
            ASAN_UNPOISON_MEMORY_REGION(&chunk->data[chunk->length],
                                        pchtml_mraw_meta_size());
#endif

            pchtml_mraw_meta_set(&chunk->data[chunk->length], &diff);

#if defined(PCHTML_HAVE_ADDRESS_SANITIZER)
            ASAN_POISON_MEMORY_REGION(&chunk->data[chunk->length],
                                      diff + pchtml_mraw_meta_size());
#endif

            pchtml_bst_insert(mraw->cache,
                              pchtml_bst_root_ref(mraw->cache), diff,
                              pchtml_mraw_data_begin(&chunk->data[chunk->length]));

            chunk->length = chunk->size;
        }

        chunk->next = pchtml_mem_chunk_make(mem, length);
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
pchtml_mraw_alloc(pchtml_mraw_t *mraw, size_t size)
{
    void *data;

    size = pchtml_mem_align(size);

    if (mraw->cache->tree_length != 0) {
        data = pchtml_bst_remove_close(mraw->cache,
                                       pchtml_bst_root_ref(mraw->cache),
                                       size, NULL);
        if (data != NULL) {

#if defined(PCHTML_HAVE_ADDRESS_SANITIZER)
            uint8_t *real_data = ((uint8_t *) data) - pchtml_mraw_meta_size();

            /* Set unpoison for current data size */
            ASAN_UNPOISON_MEMORY_REGION(real_data, size);

            size_t cur_size = pchtml_mraw_data_size(data);

            ASAN_UNPOISON_MEMORY_REGION(real_data,
                                        (cur_size + pchtml_mraw_meta_size()));
#endif

            return data;
        }
    }

    data = pchtml_mraw_mem_alloc(mraw, (size + pchtml_mraw_meta_size()));

    if (data == NULL) {
        return NULL;
    }

#if defined(PCHTML_HAVE_ADDRESS_SANITIZER)
    ASAN_UNPOISON_MEMORY_REGION(data, (size + pchtml_mraw_meta_size()));
#endif

    pchtml_mraw_meta_set(data, &size);
    return pchtml_mraw_data_begin(data);
}

void *
pchtml_mraw_calloc(pchtml_mraw_t *mraw, size_t size)
{
    void *data = pchtml_mraw_alloc(mraw, size);

    if (data != NULL) {
        memset(data, 0, pchtml_mraw_data_size(data));
    }

    return data;
}

/*
 * TODO: I don't really like this interface. Perhaps need to simplify.
 */
static inline void *
pchtml_mraw_realloc_tail(pchtml_mraw_t *mraw, void *data, void *begin,
                         size_t size, size_t begin_len, size_t new_size,
                         bool *is_valid)
{
    pchtml_mem_chunk_t *chunk = mraw->mem->chunk;

    if (chunk->size > (begin_len + new_size)) {
        *is_valid = true;

        if (new_size == 0) {
            chunk->length = begin_len - pchtml_mraw_meta_size();
            return NULL;
        }

#if defined(PCHTML_HAVE_ADDRESS_SANITIZER)
        ASAN_UNPOISON_MEMORY_REGION(begin, new_size + pchtml_mraw_meta_size());
#endif

        chunk->length = begin_len + new_size;
        memcpy(begin, &new_size, sizeof(size_t));

        return data;
    }

    /*
     * If the tail is short then we increase the current data.
     */
    if (begin_len == pchtml_mraw_meta_size()) {
        void *new_data;
        pchtml_mem_chunk_t new_chunk;

        *is_valid = true;

        pchtml_mem_chunk_init(mraw->mem, &new_chunk,
                              new_size + pchtml_mraw_meta_size());
        if(new_chunk.data == NULL) {
            return NULL;
        }

        pchtml_mraw_meta_set(new_chunk.data, &new_size);
        new_data = pchtml_mraw_data_begin(new_chunk.data);

        if (size != 0) {
            memcpy(new_data, data, sizeof(uint8_t) * size);
        }

#if defined(PCHTML_HAVE_ADDRESS_SANITIZER)
        ASAN_UNPOISON_MEMORY_REGION(chunk->data, chunk->size);
#endif

        pchtml_mem_chunk_destroy(mraw->mem, chunk, false);

        chunk->data = new_chunk.data;
        chunk->size = new_chunk.size;
        chunk->length = new_size + pchtml_mraw_meta_size();

        return new_data;
    }

    *is_valid = false;

    /*
     * Next, this piece will go into the cache.
     */
    size = pchtml_mem_align_floor(size + (chunk->size - chunk->length));
    memcpy(begin, &size, sizeof(size_t));

    chunk->length = chunk->size;

    return NULL;
}

void *
pchtml_mraw_realloc(pchtml_mraw_t *mraw, void *data, size_t new_size)
{
    void *begin;
    size_t size, begin_len;
    pchtml_mem_chunk_t *chunk = mraw->mem->chunk;

    begin = ((uint8_t *) data) - pchtml_mraw_meta_size();
    memcpy(&size, begin, sizeof(size_t));

    new_size = pchtml_mem_align(new_size);

    /*
     * Look, whether there is an opportunity
     * to prolong the current data in chunk?
     */
    if (chunk->length >= size) {
        begin_len = chunk->length - size;

        if (&chunk->data[begin_len] == data) {
            bool is_valid;
            void *ptr = pchtml_mraw_realloc_tail(mraw, data, begin,
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
            ASAN_POISON_MEMORY_REGION(begin, size + pchtml_mraw_meta_size());
#endif
            pchtml_bst_insert(mraw->cache, pchtml_bst_root_ref(mraw->cache),
                              size, data);
            return NULL;
        }

        size_t diff = pchtml_mem_align_floor(size - new_size);

        if (diff > pchtml_mraw_meta_size()) {
            memcpy(begin, &new_size, sizeof(size_t));

            new_size = diff - pchtml_mraw_meta_size();
            begin = &((uint8_t *) data)[diff];

            pchtml_mraw_meta_set(begin, &new_size);

#if defined(PCHTML_HAVE_ADDRESS_SANITIZER)
            ASAN_POISON_MEMORY_REGION(begin, new_size + pchtml_mraw_meta_size());
#endif
            pchtml_bst_insert(mraw->cache, pchtml_bst_root_ref(mraw->cache),
                              new_size, pchtml_mraw_data_begin(begin));
        }

        return data;
    }

    begin = pchtml_mraw_alloc(mraw, new_size);
    if (begin == NULL) {
        return NULL;
    }

    if (size != 0) {
        memcpy(begin, data, sizeof(uint8_t) * size);
    }

    pchtml_mraw_free(mraw, data);

    return begin;
}

void *
pchtml_mraw_free(pchtml_mraw_t *mraw, void *data)
{
    size_t size = pchtml_mraw_data_size(data);

#if defined(PCHTML_HAVE_ADDRESS_SANITIZER)
    uint8_t *real_data = ((uint8_t *) data) - pchtml_mraw_meta_size();
    ASAN_POISON_MEMORY_REGION(real_data, size + pchtml_mraw_meta_size());
#endif

    pchtml_bst_insert(mraw->cache, pchtml_bst_root_ref(mraw->cache),
                      size, data);

    return NULL;
}

/*
 * No inline functions for ABI.
 */
size_t
pchtml_mraw_data_size_noi(void *data)
{
    return pchtml_mraw_data_size(data);
}

void
pchtml_mraw_data_size_set_noi(void *data, size_t size)
{
    pchtml_mraw_data_size_set(data, size);
}

void *
pchtml_mraw_dup_noi(pchtml_mraw_t *mraw, const void *src, size_t size)
{
    return pchtml_mraw_dup(mraw, src, size);
}
