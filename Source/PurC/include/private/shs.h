/**
 * @file shs.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for shs algorithm..
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

#ifndef PURC_PRIVATE_SHS_H
#define PURC_PRIVATE_SHS_H

#include "config.h"

#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char   *key;
    void   *value;

    size_t key_len;
    size_t next;
}
pcutils_shs_entry_t;

typedef struct {
    uint32_t key;
    void     *value;

    size_t   next;
}
pcutils_shs_hash_t;


const pcutils_shs_entry_t *
pcutils_shs_entry_get_static(const pcutils_shs_entry_t *tree,
                const unsigned char *key, size_t size) WTF_INTERNAL;

const pcutils_shs_entry_t *
pcutils_shs_entry_get_lower_static(const pcutils_shs_entry_t *root,
                const unsigned char *key, size_t key_len) WTF_INTERNAL;

const pcutils_shs_entry_t *
pcutils_shs_entry_get_upper_static(const pcutils_shs_entry_t *root,
                const unsigned char *key, size_t key_len) WTF_INTERNAL;

/*
 * Inline functions
 */
static inline const pcutils_shs_hash_t *
pcutils_shs_hash_get_static(const pcutils_shs_hash_t *table,
                           const size_t table_size, const uint32_t key)
{
    const pcutils_shs_hash_t *entry;

    entry = &table[ (key % table_size) + 1 ];

    do {
        if (entry->key == key) {
            return entry;
        }

        entry = &table[entry->next];
    }
    while (entry != table);

    return NULL;
}


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PURC_PRIVATE_SHS_H */





