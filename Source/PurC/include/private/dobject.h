/**
 * @file dobject.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for dobject.
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

#ifndef PURC_PRIVATE_DOBJECT_H
#define PURC_PRIVATE_DOBJECT_H

#include "config.h"

#include "private/array.h"

#include "private/mem.h"

typedef struct {
    pcutils_mem_t   *mem;
    pcutils_array_t *cache;

    size_t         allocated;
    size_t         struct_size;
}
pcutils_dobject_t;


#ifdef __cplusplus
extern "C" {
#endif

pcutils_dobject_t *
pcutils_dobject_create(void) WTF_INTERNAL;

unsigned int
pcutils_dobject_init(pcutils_dobject_t *dobject,
                    size_t chunk_size, size_t struct_size) WTF_INTERNAL;

void
pcutils_dobject_clean(pcutils_dobject_t *dobject) WTF_INTERNAL;

pcutils_dobject_t *
pcutils_dobject_destroy(pcutils_dobject_t *dobject, bool destroy_self) WTF_INTERNAL;

uint8_t *
pcutils_dobject_init_list_entries(pcutils_dobject_t *dobject, size_t pos) WTF_INTERNAL;

void *
pcutils_dobject_alloc(pcutils_dobject_t *dobject) WTF_INTERNAL;

void *
pcutils_dobject_calloc(pcutils_dobject_t *dobject) WTF_INTERNAL;

void *
pcutils_dobject_free(pcutils_dobject_t *dobject, void *data) WTF_INTERNAL;

void *
pcutils_dobject_by_absolute_position(pcutils_dobject_t *dobject, size_t pos) WTF_INTERNAL;


/*
 * Inline functions
 */
static inline size_t
pcutils_dobject_allocated(pcutils_dobject_t *dobject)
{
    return dobject->allocated;
}

static inline size_t
pcutils_dobject_cache_length(pcutils_dobject_t *dobject)
{
    return pcutils_array_length(dobject->cache);
}


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PURC_PRIVATE_DOBJECT_H */


