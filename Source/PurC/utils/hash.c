/**
 * @file hash.c
 * @author 
 * @date 2021/07/02
 * @brief The complementation of hash algorithm.
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

#define PCHTML_HASH_EXTERN
#include "private/hash.h"
#undef PCHTML_HASH_EXTERN

#include "private/str.h"

#define PCHTML_STR_RES_MAP_LOWERCASE
#define PCHTML_STR_RES_MAP_UPPERCASE
#include "str_res.h"


/* Insert variable. */
const pcutils_hash_insert_t pcutils_hash_insert_var = {
    .hash = pcutils_hash_make_id,
    .copy = pcutils_hash_copy,
    .cmp = pchtml_str_data_ncmp
};

const pcutils_hash_insert_t pcutils_hash_insert_lower_var = {
    .hash = pcutils_hash_make_id_lower,
    .copy = pcutils_hash_copy_lower,
    .cmp = pchtml_str_data_nlocmp_right
};

const pcutils_hash_insert_t pcutils_hash_insert_upper_var = {
    .hash = pcutils_hash_make_id_upper,
    .copy = pcutils_hash_copy_upper,
    .cmp = pchtml_str_data_nupcmp_right
};

const pcutils_hash_insert_t
*pcutils_hash_insert_raw = &pcutils_hash_insert_var;

const pcutils_hash_insert_t
*pcutils_hash_insert_lower = &pcutils_hash_insert_lower_var;

const pcutils_hash_insert_t
*pcutils_hash_insert_upper = &pcutils_hash_insert_upper_var;

/* Search variable. */
const pcutils_hash_search_t pcutils_hash_search_var = {
    .hash = pcutils_hash_make_id,
    .cmp = pchtml_str_data_ncmp
};

const pcutils_hash_search_t pcutils_hash_search_lower_var = {
    .hash = pcutils_hash_make_id_lower,
    .cmp = pchtml_str_data_nlocmp_right
};

const pcutils_hash_search_t pcutils_hash_search_upper_var = {
    .hash = pcutils_hash_make_id_upper,
    .cmp = pchtml_str_data_nupcmp_right
};

const pcutils_hash_search_t
*pcutils_hash_search_raw = &pcutils_hash_search_var;

const pcutils_hash_search_t
*pcutils_hash_search_lower = &pcutils_hash_search_lower_var;

const pcutils_hash_search_t
*pcutils_hash_search_upper = &pcutils_hash_search_upper_var;


static inline pcutils_hash_entry_t **
pcutils_hash_table_create(pcutils_hash_t *hash)
{
    return pchtml_calloc(hash->table_size, sizeof(pcutils_hash_entry_t *));
}

static inline void
pcutils_hash_table_clean(pcutils_hash_t *hash)
{
    memset(hash->table, 0, sizeof(pcutils_hash_t *) * hash->table_size);
}

static inline pcutils_hash_entry_t **
pcutils_hash_table_destroy(pcutils_hash_t *hash)
{
    if (hash->table != NULL) {
        return pchtml_free(hash->table);
    }

    return NULL;
}

static inline pcutils_hash_entry_t *
_pcutils_hash_entry_create(pcutils_hash_t *hash, const pcutils_hash_copy_f copy_func,
                          const unsigned char *key, size_t length)
{
    pcutils_hash_entry_t *entry = pcutils_dobject_calloc(hash->entries);
    if (entry == NULL) {
        return NULL;
    }

    entry->length = length;

    if (copy_func(hash, entry, key, length) != PURC_ERROR_OK) {
        pcutils_dobject_free(hash->entries, entry);
        return NULL;
    }

    return entry;
}

pcutils_hash_t *
pcutils_hash_create(void)
{
    return pchtml_calloc(1, sizeof(pcutils_hash_t));
}

unsigned int
pcutils_hash_init(pcutils_hash_t *hash, size_t table_size, size_t struct_size)
{
    unsigned int status;
    size_t chunk_size;

    if (hash == NULL) {
        return PURC_ERROR_NULL_OBJECT;
    }

    if (table_size < PCHTML_HASH_TABLE_MIN_SIZE) {
        table_size = PCHTML_HASH_TABLE_MIN_SIZE;
    }

    chunk_size = table_size / 2;

    hash->table_size = table_size;

    hash->entries = pcutils_dobject_create();
    status = pcutils_dobject_init(hash->entries, chunk_size, struct_size);
    if (status != PURC_ERROR_OK) {
        return status;
    }

    hash->mraw = pchtml_mraw_create();
    status = pchtml_mraw_init(hash->mraw, chunk_size * 12);
    if (status != PURC_ERROR_OK) {
        return status;
    }

    hash->table = pcutils_hash_table_create(hash);
    if (hash->table == NULL) {
        return PURC_ERROR_OUT_OF_MEMORY;
    }

    hash->struct_size = struct_size;

    return PURC_ERROR_OK;
}

void
pcutils_hash_clean(pcutils_hash_t *hash)
{
    pcutils_dobject_clean(hash->entries);
    pchtml_mraw_clean(hash->mraw);
    pcutils_hash_table_clean(hash);
}

pcutils_hash_t *
pcutils_hash_destroy(pcutils_hash_t *hash, bool destroy_obj)
{
    if (hash == NULL) {
        return NULL;
    }

    hash->entries = pcutils_dobject_destroy(hash->entries, true);
    hash->mraw = pchtml_mraw_destroy(hash->mraw, true);
    hash->table = pcutils_hash_table_destroy(hash);

    if (destroy_obj) {
        return pchtml_free(hash);
    }

    return hash;
}

void *
pcutils_hash_insert(pcutils_hash_t *hash, const pcutils_hash_insert_t *insert,
                   const unsigned char *key, size_t length)
{
    unsigned char *str;
    uint32_t hash_id, table_idx;
    pcutils_hash_entry_t *entry;

    hash_id = insert->hash(key, length);
    table_idx = hash_id % hash->table_size;

    entry = hash->table[table_idx];

    if (entry == NULL) {
        entry = _pcutils_hash_entry_create(hash, insert->copy, key, length);
        hash->table[table_idx] = entry;

        return entry;
    }

    do {
        str = pcutils_hash_entry_str(entry);

        if (entry->length == length && insert->cmp(str, key, length)) {
            return entry;
        }

        if (entry->next == NULL) {
            break;
        }

        entry = entry->next;
    }
    while (1);

    entry->next = _pcutils_hash_entry_create(hash, insert->copy, key, length);

    return entry->next;
}

void *
pcutils_hash_insert_by_entry(pcutils_hash_t *hash, pcutils_hash_entry_t *entry,
                            const pcutils_hash_search_t *search,
                            const unsigned char *key, size_t length)
{
    unsigned char *str;
    uint32_t hash_id, table_idx;
    pcutils_hash_entry_t *item;

    hash_id = search->hash(key, length);
    table_idx = hash_id % hash->table_size;

    item = hash->table[table_idx];

    if (item == NULL) {
        hash->table[table_idx] = entry;

        return entry;
    }

    do {
        str = pcutils_hash_entry_str(item);

        if (item->length == length && search->cmp(str, key, length)) {
            return item;
        }

        if (item->next == NULL) {
            break;
        }

        item = item->next;
    }
    while (1);

    item->next = entry;

    return entry;
}

void
pcutils_hash_remove(pcutils_hash_t *hash, const pcutils_hash_search_t *search,
                   const unsigned char *key, size_t length)
{
    pcutils_hash_remove_by_hash_id(hash, search->hash(key, length),
                                  key, length, search->cmp);
}

void *
pcutils_hash_search(pcutils_hash_t *hash, const pcutils_hash_search_t *search,
                   const unsigned char *key, size_t length)
{
    return pcutils_hash_search_by_hash_id(hash, search->hash(key, length),
                                         key, length, search->cmp);
}

void
pcutils_hash_remove_by_hash_id(pcutils_hash_t *hash, uint32_t hash_id,
                              const unsigned char *key, size_t length,
                              const pcutils_hash_cmp_f cmp_func)
{
    uint32_t table_idx;
    unsigned char *str;
    pcutils_hash_entry_t *entry, *prev;

    table_idx = hash_id % hash->table_size;
    entry = hash->table[table_idx];
    prev = NULL;

    while (entry != NULL) {
        str = pcutils_hash_entry_str(entry);

        if (entry->length == length && cmp_func(str, key, length)) {
            if (prev == NULL) {
                hash->table[table_idx] = entry->next;
            }
            else {
                prev->next = entry->next;
            }

            if (length > PCHTML_HASH_SHORT_SIZE) {
                pchtml_mraw_free(hash->mraw, entry->u.long_str);
            }

            pcutils_dobject_free(hash->entries, entry);

            return;
        }

        prev = entry;
        entry = entry->next;
    }
}

void *
pcutils_hash_search_by_hash_id(pcutils_hash_t *hash, uint32_t hash_id,
                              const unsigned char *key, size_t length,
                              const pcutils_hash_cmp_f cmp_func)
{
    unsigned char *str;
    pcutils_hash_entry_t *entry;

    entry = hash->table[ hash_id % hash->table_size ];

    while (entry != NULL) {
        str = pcutils_hash_entry_str(entry);

        if (entry->length == length && cmp_func(str, key, length)) {
            return entry;
        }

        entry = entry->next;
    }

    return NULL;
}

uint32_t
pcutils_hash_make_id(const unsigned char *key, size_t length)
{
    size_t i;
    uint32_t hash_id;

    for (i = hash_id = 0; i < length; i++) {
        hash_id += key[i];
        hash_id += (hash_id << 10);
        hash_id ^= (hash_id >> 6);
    }

    hash_id += (hash_id << 3);
    hash_id ^= (hash_id >> 11);
    hash_id += (hash_id << 15);

    return hash_id;
}

uint32_t
pcutils_hash_make_id_lower(const unsigned char *key, size_t length)
{
    size_t i;
    uint32_t hash_id;

    for (i = hash_id = 0; i < length; i++) {
        hash_id += pchtml_str_res_map_lowercase[ key[i] ];
        hash_id += (hash_id << 10);
        hash_id ^= (hash_id >> 6);
    }

    hash_id += (hash_id << 3);
    hash_id ^= (hash_id >> 11);
    hash_id += (hash_id << 15);

    return hash_id;
}

uint32_t
pcutils_hash_make_id_upper(const unsigned char *key, size_t length)
{
    size_t i;
    uint32_t hash_id;

    for (i = hash_id = 0; i < length; i++) {
        hash_id += pchtml_str_res_map_uppercase[ key[i] ];
        hash_id += (hash_id << 10);
        hash_id ^= (hash_id >> 6);
    }

    hash_id += (hash_id << 3);
    hash_id ^= (hash_id >> 11);
    hash_id += (hash_id << 15);

    return hash_id;
}

unsigned int
pcutils_hash_copy(pcutils_hash_t *hash, pcutils_hash_entry_t *entry,
                 const unsigned char *key, size_t length)
{
    unsigned char *to;

    if (length <= PCHTML_HASH_SHORT_SIZE) {
        to = entry->u.short_str;
    }
    else {
        entry->u.long_str = pchtml_mraw_alloc(hash->mraw, length + 1);
        if (entry->u.long_str == NULL) {
            return PURC_ERROR_OUT_OF_MEMORY;
        }

        to = entry->u.long_str;
    }

    memcpy(to, key, length);

    to[length] = '\0';

    return PURC_ERROR_OK;
}

unsigned int
pcutils_hash_copy_lower(pcutils_hash_t *hash, pcutils_hash_entry_t *entry,
                       const unsigned char *key, size_t length)
{
    unsigned char *to;

    if (length <= PCHTML_HASH_SHORT_SIZE) {
        to = entry->u.short_str;
    }
    else {
        entry->u.long_str = pchtml_mraw_alloc(hash->mraw, length + 1);
        if (entry->u.long_str == NULL) {
            return PURC_ERROR_OUT_OF_MEMORY;
        }

        to = entry->u.long_str;
    }

    for (size_t i = 0; i < length; i++) {
        to[i] = pchtml_str_res_map_lowercase[ key[i] ];
    }

    to[length] = '\0';

    return PURC_ERROR_OK;
}

unsigned int
pcutils_hash_copy_upper(pcutils_hash_t *hash, pcutils_hash_entry_t *entry,
                       const unsigned char *key, size_t length)
{
    unsigned char *to;

    if (length <= PCHTML_HASH_SHORT_SIZE) {
        to = entry->u.short_str;
    }
    else {
        entry->u.long_str = pchtml_mraw_alloc(hash->mraw, length + 1);
        if (entry->u.long_str == NULL) {
            return PURC_ERROR_OUT_OF_MEMORY;
        }

        to = entry->u.long_str;
    }

    for (size_t i = 0; i < length; i++) {
        to[i] = pchtml_str_res_map_uppercase[ key[i] ];
    }

    to[length] = '\0';

    return PURC_ERROR_OK;
}
