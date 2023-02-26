/*
 * @file map.h
 * @author Vincent Wei
 * @date 2021/07/15
 * @brief The header for ordered map based on red-black tree and
 *      unordered map based on hash table.
 *
 * Copyright (C) 2021 ~ 2023 FMSoft <https://www.fmsoft.cn>
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
 * Note that the code is copied from GPL'd MiniGUI developed by FMSoft.
 */

#ifndef PURC_PRIVATE_MAP_H
#define PURC_PRIVATE_MAP_H

#include <stdlib.h>
#include <string.h>

#include "private/callbacks.h"
#include "private/rbtree.h"
#include "private/hashtable.h"

#define copy_key_fn pcutils_copy_fn
#define free_key_fn pcutils_free_fn
#define copy_val_fn pcutils_copy_fn
#define free_val_fn pcutils_free_fn
#define comp_key_fn pcutils_comp_fn
#define hash_key_fn pcutils_hash_fn
#define free_kv_fn  pcutils_free_kv_fn

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* hash functions */
uint32_t pchash_default_str_hash(const void *k);
uint32_t pchash_perlish_str_hash(const void *k);
uint32_t pchash_fnv1a_str_hash(const void *k);
uint32_t pchash_ptr_hash(const void *k);

/* common functions for string key */
static inline void* copy_key_string(const void *key)
{
    return strdup((const char*)key);
}

static inline void free_key_string(void *key)
{
    free(key);
}

static inline int comp_key_string(const void *key1, const void *key2)
{
    return strcmp((const char*)key1, (const char*)key2);
}

/* pcutils_map_xxx: interfaces for ordered map based on red-black tree */

typedef struct pcutils_map pcutils_map;
typedef struct pcutils_map_entry {
    struct rb_node  node;
    void*           key;
    void*           val;
    free_kv_fn      free_kv_alt;   // alternative free function per entry
} pcutils_map_entry;

#define pcutils_map_entry_key(entry) (((*entry))->key)
#define pcutils_map_entry_val(entry) (((*entry))->val)
#define pcutils_map_entry_field(entry, field) (((*entry))->field)

pcutils_map* pcutils_map_create(copy_key_fn copy_key, free_key_fn free_key,
        copy_val_fn copy_val, free_val_fn free_val,
        comp_key_fn comp_key, bool threads);
int pcutils_map_destroy(pcutils_map* map);
int pcutils_map_clear(pcutils_map* map);
size_t pcutils_map_get_size(pcutils_map* map);

pcutils_map_entry *pcutils_map_find(pcutils_map* map, const void* key);

pcutils_map_entry *
pcutils_map_find_and_lock(pcutils_map* map, const void* key);

int pcutils_map_insert_ex(pcutils_map* map, const void* key,
        const void* val, free_kv_fn free_kv_alt);
static inline int pcutils_map_insert(pcutils_map* map, const void* key,
        const void* val)
{
    return pcutils_map_insert_ex(map, key, val, NULL);
}

int pcutils_map_replace_or_insert(pcutils_map* map, const void* key,
        const void* val, free_kv_fn free_kv_alt);

int pcutils_map_replace(pcutils_map* map, const void* key,
        const void* val, free_kv_fn free_kv_alt);

int pcutils_map_erase(pcutils_map* map, const void* key);

void
pcutils_map_erase_entry_nolock(pcutils_map* map, pcutils_map_entry *entry);

int pcutils_map_traverse(pcutils_map *map, void *ud,
        int (*cb)(void *key, void *val, void *ud));

void pcutils_map_lock(pcutils_map *map);
void pcutils_map_unlock(pcutils_map *map);

struct pcutils_map_iterator {
    pcutils_map_entry       *curr;
    pcutils_map_entry       *next;
    pcutils_map_entry       *prev;
    void                    *ctx;
};

struct pcutils_map_iterator
pcutils_map_it_begin_first(pcutils_map *map);

struct pcutils_map_iterator
pcutils_map_it_begin_last(pcutils_map *map);

pcutils_map_entry *
pcutils_map_it_value(struct pcutils_map_iterator *it);

pcutils_map_entry *
pcutils_map_it_next(struct pcutils_map_iterator *it);

pcutils_map_entry *
pcutils_map_it_prev(struct pcutils_map_iterator *it);

void
pcutils_map_it_end(struct pcutils_map_iterator *it);

/* pcutils_uomap_xxx: interfaces for ordered map based on red-black tree */

typedef struct pchash_table pcutils_uomap;
typedef struct pchash_entry pcutils_uomap_entry;

#define pcutils_uomap_entry_key(entry) pchash_entry_key(entry)
#define pcutils_uomap_entry_val(entry) pchash_entry_val(entry)
#define pcutils_uomap_entry_field(entry, field) \
    pchash_entry_field(entry, field)

static inline pcutils_uomap* pcutils_uomap_create(
        copy_key_fn copy_key, free_key_fn free_key,
        copy_val_fn copy_val, free_val_fn free_val,
        hash_key_fn hash_key, comp_key_fn comp_key, bool threads, bool sorted)
{
    return pchash_table_new(0, copy_key, free_key,
            copy_val, free_val,
            (hash_key == NULL) ? pchash_default_str_hash : hash_key,
            (comp_key == NULL) ? pchash_str_equal : comp_key, threads, sorted);
}

static inline int pcutils_uomap_destroy(pcutils_uomap* map)
{
    pchash_table_delete(map);
    return 0;
}

static inline int pcutils_uomap_clear(pcutils_uomap* map)
{
    pchash_table_reset(map);
    return 0;
}

static inline size_t pcutils_uomap_get_size(pcutils_uomap* map)
{
    return pchash_table_length(map);
}

static inline pcutils_uomap_entry *
pcutils_uomap_find(pcutils_uomap* map, const void* key)
{
    return pchash_table_lookup_entry(map, key);
}

static inline pcutils_uomap_entry *
pcutils_uomap_find_and_lock(pcutils_uomap* map, const void* key)
{
    return pchash_table_lookup_and_lock(map, key);
}

static inline int pcutils_uomap_insert_ex(pcutils_uomap* map,
        const void* key, const void* val, free_kv_fn free_kv_alt)
{
    return pchash_table_insert_ex(map, key, val, free_kv_alt);
}

static inline int pcutils_uomap_insert(pcutils_uomap* map, const void* key,
        const void* val)
{
    return pcutils_uomap_insert_ex(map, key, val, NULL);
}

static inline int pcutils_uomap_replace_or_insert(pcutils_uomap* map,
        const void* key, const void* val, free_kv_fn free_kv_alt)
{
    return pchash_table_replace_or_insert(map, key, val, free_kv_alt);
}

static inline int pcutils_uomap_replace(pcutils_uomap* map,
        const void* key, const void* val, free_kv_fn free_kv_alt)
{
    return pchash_table_replace(map, key, val, free_kv_alt);
}

static inline int pcutils_uomap_erase(pcutils_uomap* map, const void* key)
{
    return pchash_table_erase(map, key);
}

static inline int pcutils_uomap_erase_entry_nolock(pcutils_uomap* map,
        pcutils_uomap_entry *entry)
{
    return pchash_table_erase_entry(map, entry);
}

int pcutils_uomap_traverse(pcutils_uomap *map, void *ud,
        int (*cb)(void *key, void *val, void *ud));

void pcutils_uomap_lock(pcutils_uomap *map);
void pcutils_uomap_unlock(pcutils_uomap *map);

struct pcutils_uomap_iterator {
    pcutils_uomap          *map;
    pcutils_uomap_entry    *curr;
};

struct pcutils_uomap_iterator
pcutils_uomap_it_begin_first(pcutils_uomap *map);

struct pcutils_uomap_iterator
pcutils_uomap_it_begin_last(pcutils_uomap *map);

pcutils_uomap_entry *
pcutils_uomap_it_value(struct pcutils_uomap_iterator *it);

pcutils_uomap_entry *
pcutils_uomap_it_next(struct pcutils_uomap_iterator *it);

pcutils_uomap_entry *
pcutils_uomap_it_prev(struct pcutils_uomap_iterator *it);

void
pcutils_uomap_it_end(struct pcutils_uomap_iterator *it);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* PURC_PRIVATE_MAP_H */

