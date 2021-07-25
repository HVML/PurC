/**
 * @file mraw.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for mraw.
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

#ifndef PCHTML_MRAW_H
#define PCHTML_MRAW_H

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>

#include "config.h"
#include "html/core/base.h"
#include "html/core/mem.h"
#include "html/core/bst.h"


#define pchtml_mraw_meta_size()                                                \
    (((sizeof(size_t) % PCHTML_MEM_ALIGN_STEP) != 0)                           \
    ? sizeof(size_t)                                                           \
        + (PCHTML_MEM_ALIGN_STEP - (sizeof(size_t) % PCHTML_MEM_ALIGN_STEP))   \
    : sizeof(size_t))


typedef struct {
    pchtml_mem_t *mem;
    pchtml_bst_t *cache;
}
pchtml_mraw_t;


pchtml_mraw_t *
pchtml_mraw_create(void) WTF_INTERNAL;

unsigned int
pchtml_mraw_init(pchtml_mraw_t *mraw, size_t chunk_size) WTF_INTERNAL;

void
pchtml_mraw_clean(pchtml_mraw_t *mraw) WTF_INTERNAL;

pchtml_mraw_t *
pchtml_mraw_destroy(pchtml_mraw_t *mraw, bool destroy_self) WTF_INTERNAL;


void *
pchtml_mraw_alloc(pchtml_mraw_t *mraw, size_t size) WTF_INTERNAL;

void *
pchtml_mraw_calloc(pchtml_mraw_t *mraw, size_t size) WTF_INTERNAL;

void *
pchtml_mraw_realloc(pchtml_mraw_t *mraw, void *data, 
                size_t new_size) WTF_INTERNAL;

void *
pchtml_mraw_free(pchtml_mraw_t *mraw, void *data) WTF_INTERNAL;


/*
 * Inline functions
 */
static inline size_t
pchtml_mraw_data_size(void *data)
{
    return *((size_t *) (((uint8_t *) data) - pchtml_mraw_meta_size()));
}

static inline void
pchtml_mraw_data_size_set(void *data, size_t size)
{
    data = (((uint8_t *) data) - pchtml_mraw_meta_size());
    memcpy(data, &size, sizeof(size_t));
}

static inline void *
pchtml_mraw_dup(pchtml_mraw_t *mraw, const void *src, size_t size)
{
    void *data = pchtml_mraw_alloc(mraw, size);

    if (data != NULL) {
        memcpy(data, src, size);
    }

    return data;
}


/*
 * No inline functions for ABI.
 */
size_t
pchtml_mraw_data_size_noi(void *data);

void
pchtml_mraw_data_size_set_noi(void *data, size_t size);

void *
pchtml_mraw_dup_noi(pchtml_mraw_t *mraw, const void *src, size_t size);


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_MRAW_H */
