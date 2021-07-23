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
 */

#define PCHTML_HASH_EXTERN
#include "html/core/hash.h"
#undef PCHTML_HASH_EXTERN

#include "html/core/str.h"

#define PCHTML_STR_RES_MAP_LOWERCASE
#define PCHTML_STR_RES_MAP_UPPERCASE
#include "html/core/str_res.h"


/* Insert variable. */
const pchtml_hash_insert_t pchtml_hash_insert_var = {
    .hash = pchtml_hash_make_id,
    .copy = pchtml_hash_copy,
    .cmp = pchtml_str_data_ncmp
};

const pchtml_hash_insert_t pchtml_hash_insert_lower_var = {
    .hash = pchtml_hash_make_id_lower,
    .copy = pchtml_hash_copy_lower,
    .cmp = pchtml_str_data_nlocmp_right
};

const pchtml_hash_insert_t pchtml_hash_insert_upper_var = {
    .hash = pchtml_hash_make_id_upper,
    .copy = pchtml_hash_copy_upper,
    .cmp = pchtml_str_data_nupcmp_right
};

const pchtml_hash_insert_t
*pchtml_hash_insert_raw = &pchtml_hash_insert_var;

const pchtml_hash_insert_t
*pchtml_hash_insert_lower = &pchtml_hash_insert_lower_var;

const pchtml_hash_insert_t
*pchtml_hash_insert_upper = &pchtml_hash_insert_upper_var;

/* Search variable. */
const pchtml_hash_search_t pchtml_hash_search_var = {
    .hash = pchtml_hash_make_id,
    .cmp = pchtml_str_data_ncmp
};

const pchtml_hash_search_t pchtml_hash_search_lower_var = {
    .hash = pchtml_hash_make_id_lower,
    .cmp = pchtml_str_data_nlocmp_right
};

const pchtml_hash_search_t pchtml_hash_search_upper_var = {
    .hash = pchtml_hash_make_id_upper,
    .cmp = pchtml_str_data_nupcmp_right
};

const pchtml_hash_search_t
*pchtml_hash_search_raw = &pchtml_hash_search_var;

const pchtml_hash_search_t
*pchtml_hash_search_lower = &pchtml_hash_search_lower_var;

const pchtml_hash_search_t
*pchtml_hash_search_upper = &pchtml_hash_search_upper_var;


static inline pchtml_hash_entry_t **
pchtml_hash_table_create(pchtml_hash_t *hash)
{
    return pchtml_calloc(hash->table_size, sizeof(pchtml_hash_entry_t *));
}

static inline void
pchtml_hash_table_clean(pchtml_hash_t *hash)
{
    memset(hash->table, 0, sizeof(pchtml_hash_t *) * hash->table_size);
}

static inline pchtml_hash_entry_t **
pchtml_hash_table_destroy(pchtml_hash_t *hash)
{
    if (hash->table != NULL) {
        return pchtml_free(hash->table);
    }

    return NULL;
}

static inline pchtml_hash_entry_t *
_pchtml_hash_entry_create(pchtml_hash_t *hash, const pchtml_hash_copy_f copy_func,
                          const unsigned char *key, size_t length)
{
    pchtml_hash_entry_t *entry = pchtml_dobject_calloc(hash->entries);
    if (entry == NULL) {
        return NULL;
    }

    entry->length = length;

    if (copy_func(hash, entry, key, length) != PCHTML_STATUS_OK) {
        pchtml_dobject_free(hash->entries, entry);
        return NULL;
    }

    return entry;
}

pchtml_hash_t *
pchtml_hash_create(void)
{
    return pchtml_calloc(1, sizeof(pchtml_hash_t));
}

unsigned int
pchtml_hash_init(pchtml_hash_t *hash, size_t table_size, size_t struct_size)
{
    unsigned int status;
    size_t chunk_size;

    if (hash == NULL) {
        return PCHTML_STATUS_ERROR_OBJECT_IS_NULL;
    }

    if (table_size < PCHTML_HASH_TABLE_MIN_SIZE) {
        table_size = PCHTML_HASH_TABLE_MIN_SIZE;
    }

    chunk_size = table_size / 2;

    hash->table_size = table_size;

    hash->entries = pchtml_dobject_create();
    status = pchtml_dobject_init(hash->entries, chunk_size, struct_size);
    if (status != PCHTML_STATUS_OK) {
        return status;
    }

    hash->mraw = pchtml_mraw_create();
    status = pchtml_mraw_init(hash->mraw, chunk_size * 12);
    if (status != PCHTML_STATUS_OK) {
        return status;
    }

    hash->table = pchtml_hash_table_create(hash);
    if (hash->table == NULL) {
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }

    hash->struct_size = struct_size;

    return PCHTML_STATUS_OK;
}

void
pchtml_hash_clean(pchtml_hash_t *hash)
{
    pchtml_dobject_clean(hash->entries);
    pchtml_mraw_clean(hash->mraw);
    pchtml_hash_table_clean(hash);
}

pchtml_hash_t *
pchtml_hash_destroy(pchtml_hash_t *hash, bool destroy_obj)
{
    if (hash == NULL) {
        return NULL;
    }

    hash->entries = pchtml_dobject_destroy(hash->entries, true);
    hash->mraw = pchtml_mraw_destroy(hash->mraw, true);
    hash->table = pchtml_hash_table_destroy(hash);

    if (destroy_obj) {
        return pchtml_free(hash);
    }

    return hash;
}

void *
pchtml_hash_insert(pchtml_hash_t *hash, const pchtml_hash_insert_t *insert,
                   const unsigned char *key, size_t length)
{
    unsigned char *str;
    uint32_t hash_id, table_idx;
    pchtml_hash_entry_t *entry;

    hash_id = insert->hash(key, length);
    table_idx = hash_id % hash->table_size;

    entry = hash->table[table_idx];

    if (entry == NULL) {
        entry = _pchtml_hash_entry_create(hash, insert->copy, key, length);
        hash->table[table_idx] = entry;

        return entry;
    }

    do {
        str = pchtml_hash_entry_str(entry);

        if (entry->length == length && insert->cmp(str, key, length)) {
            return entry;
        }

        if (entry->next == NULL) {
            break;
        }

        entry = entry->next;
    }
    while (1);

    entry->next = _pchtml_hash_entry_create(hash, insert->copy, key, length);

    return entry->next;
}

void *
pchtml_hash_insert_by_entry(pchtml_hash_t *hash, pchtml_hash_entry_t *entry,
                            const pchtml_hash_search_t *search,
                            const unsigned char *key, size_t length)
{
    unsigned char *str;
    uint32_t hash_id, table_idx;
    pchtml_hash_entry_t *item;

    hash_id = search->hash(key, length);
    table_idx = hash_id % hash->table_size;

    item = hash->table[table_idx];

    if (item == NULL) {
        hash->table[table_idx] = entry;

        return entry;
    }

    do {
        str = pchtml_hash_entry_str(item);

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
pchtml_hash_remove(pchtml_hash_t *hash, const pchtml_hash_search_t *search,
                   const unsigned char *key, size_t length)
{
    pchtml_hash_remove_by_hash_id(hash, search->hash(key, length),
                                  key, length, search->cmp);
}

void *
pchtml_hash_search(pchtml_hash_t *hash, const pchtml_hash_search_t *search,
                   const unsigned char *key, size_t length)
{
    return pchtml_hash_search_by_hash_id(hash, search->hash(key, length),
                                         key, length, search->cmp);
}

void
pchtml_hash_remove_by_hash_id(pchtml_hash_t *hash, uint32_t hash_id,
                              const unsigned char *key, size_t length,
                              const pchtml_hash_cmp_f cmp_func)
{
    uint32_t table_idx;
    unsigned char *str;
    pchtml_hash_entry_t *entry, *prev;

    table_idx = hash_id % hash->table_size;
    entry = hash->table[table_idx];
    prev = NULL;

    while (entry != NULL) {
        str = pchtml_hash_entry_str(entry);

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

            pchtml_dobject_free(hash->entries, entry);

            return;
        }

        prev = entry;
        entry = entry->next;
    }
}

void *
pchtml_hash_search_by_hash_id(pchtml_hash_t *hash, uint32_t hash_id,
                              const unsigned char *key, size_t length,
                              const pchtml_hash_cmp_f cmp_func)
{
    unsigned char *str;
    pchtml_hash_entry_t *entry;

    entry = hash->table[ hash_id % hash->table_size ];

    while (entry != NULL) {
        str = pchtml_hash_entry_str(entry);

        if (entry->length == length && cmp_func(str, key, length)) {
            return entry;
        }

        entry = entry->next;
    }

    return NULL;
}

uint32_t
pchtml_hash_make_id(const unsigned char *key, size_t length)
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
pchtml_hash_make_id_lower(const unsigned char *key, size_t length)
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
pchtml_hash_make_id_upper(const unsigned char *key, size_t length)
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
pchtml_hash_copy(pchtml_hash_t *hash, pchtml_hash_entry_t *entry,
                 const unsigned char *key, size_t length)
{
    unsigned char *to;

    if (length <= PCHTML_HASH_SHORT_SIZE) {
        to = entry->u.short_str;
    }
    else {
        entry->u.long_str = pchtml_mraw_alloc(hash->mraw, length + 1);
        if (entry->u.long_str == NULL) {
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }

        to = entry->u.long_str;
    }

    memcpy(to, key, length);

    to[length] = '\0';

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_hash_copy_lower(pchtml_hash_t *hash, pchtml_hash_entry_t *entry,
                       const unsigned char *key, size_t length)
{
    unsigned char *to;

    if (length <= PCHTML_HASH_SHORT_SIZE) {
        to = entry->u.short_str;
    }
    else {
        entry->u.long_str = pchtml_mraw_alloc(hash->mraw, length + 1);
        if (entry->u.long_str == NULL) {
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }

        to = entry->u.long_str;
    }

    for (size_t i = 0; i < length; i++) {
        to[i] = pchtml_str_res_map_lowercase[ key[i] ];
    }

    to[length] = '\0';

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_hash_copy_upper(pchtml_hash_t *hash, pchtml_hash_entry_t *entry,
                       const unsigned char *key, size_t length)
{
    unsigned char *to;

    if (length <= PCHTML_HASH_SHORT_SIZE) {
        to = entry->u.short_str;
    }
    else {
        entry->u.long_str = pchtml_mraw_alloc(hash->mraw, length + 1);
        if (entry->u.long_str == NULL) {
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }

        to = entry->u.long_str;
    }

    for (size_t i = 0; i < length; i++) {
        to[i] = pchtml_str_res_map_uppercase[ key[i] ];
    }

    to[length] = '\0';

    return PCHTML_STATUS_OK;
}
