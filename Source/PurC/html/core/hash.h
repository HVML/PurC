/**
 * @file hash.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for hash algorithm.
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

#ifndef PCHTML_HASH_H
#define PCHTML_HASH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/core/dobject.h"
#include "html/core/mraw.h"


#define PCHTML_HASH_SHORT_SIZE     16
#define PCHTML_HASH_TABLE_MIN_SIZE 32


typedef struct pchtml_hash_search pchtml_hash_search_t;
typedef struct pchtml_hash_insert pchtml_hash_insert_t;

#ifndef PCHTML_HASH_EXTERN
LXB_EXTERN const pchtml_hash_insert_t *pchtml_hash_insert_raw;
LXB_EXTERN const pchtml_hash_insert_t *pchtml_hash_insert_lower;
LXB_EXTERN const pchtml_hash_insert_t *pchtml_hash_insert_upper;

LXB_EXTERN const pchtml_hash_search_t *pchtml_hash_search_raw;
LXB_EXTERN const pchtml_hash_search_t *pchtml_hash_search_lower;
LXB_EXTERN const pchtml_hash_search_t *pchtml_hash_search_upper;
#endif

/*
 * FIXME:
 * It is necessary to add the rebuild of a hash table
 * and optimize collisions.
 */

typedef struct pchtml_hash pchtml_hash_t;
typedef struct pchtml_hash_entry pchtml_hash_entry_t;

typedef uint32_t
(*pchtml_hash_id_f)(const unsigned char *key, size_t size);

typedef unsigned int
(*pchtml_hash_copy_f)(pchtml_hash_t *hash, pchtml_hash_entry_t *entry,
                      const unsigned char *key, size_t size);

typedef bool
(*pchtml_hash_cmp_f)(const unsigned char *first,
                     const unsigned char *second, size_t size);

struct pchtml_hash_entry {
    union {
        unsigned char *long_str;
        unsigned char short_str[PCHTML_HASH_SHORT_SIZE + 1];
    } u;

    size_t              length;

    pchtml_hash_entry_t *next;
};

struct pchtml_hash {
    pchtml_dobject_t    *entries;
    pchtml_mraw_t       *mraw;

    pchtml_hash_entry_t **table;
    size_t              table_size;

    size_t              struct_size;
};

struct pchtml_hash_insert {
    pchtml_hash_id_f   hash; /* For generate a hash id. */
    pchtml_hash_cmp_f  cmp;  /* For compare key. */
    pchtml_hash_copy_f copy; /* For copy key. */
};

struct pchtml_hash_search {
    pchtml_hash_id_f   hash; /* For generate a hash id. */
    pchtml_hash_cmp_f  cmp;  /* For compare key. */
};


pchtml_hash_t *
pchtml_hash_create(void) WTF_INTERNAL;

unsigned int
pchtml_hash_init(pchtml_hash_t *hash, size_t table_size, size_t struct_size) WTF_INTERNAL;

void
pchtml_hash_clean(pchtml_hash_t *hash) WTF_INTERNAL;

pchtml_hash_t *
pchtml_hash_destroy(pchtml_hash_t *hash, bool destroy_obj) WTF_INTERNAL;


void *
pchtml_hash_insert(pchtml_hash_t *hash, const pchtml_hash_insert_t *insert,
                   const unsigned char *key, size_t length) WTF_INTERNAL;

void *
pchtml_hash_insert_by_entry(pchtml_hash_t *hash, pchtml_hash_entry_t *entry,
                            const pchtml_hash_search_t *search,
                            const unsigned char *key, size_t length) WTF_INTERNAL;

void
pchtml_hash_remove(pchtml_hash_t *hash, const pchtml_hash_search_t *search,
                   const unsigned char *key, size_t length) WTF_INTERNAL;

void *
pchtml_hash_search(pchtml_hash_t *hash, const pchtml_hash_search_t *search,
                   const unsigned char *key, size_t length);

void
pchtml_hash_remove_by_hash_id(pchtml_hash_t *hash, uint32_t hash_id,
                              const unsigned char *key, size_t length,
                              const pchtml_hash_cmp_f cmp_func) WTF_INTERNAL;

void *
pchtml_hash_search_by_hash_id(pchtml_hash_t *hash, uint32_t hash_id,
                              const unsigned char *key, size_t length,
                              const pchtml_hash_cmp_f cmp_func) WTF_INTERNAL;


uint32_t
pchtml_hash_make_id(const unsigned char *key, size_t length) WTF_INTERNAL;

uint32_t
pchtml_hash_make_id_lower(const unsigned char *key, size_t length) WTF_INTERNAL;

uint32_t
pchtml_hash_make_id_upper(const unsigned char *key, size_t length) WTF_INTERNAL;

unsigned int
pchtml_hash_copy(pchtml_hash_t *hash, pchtml_hash_entry_t *entry,
                 const unsigned char *key, size_t length) WTF_INTERNAL;

unsigned int
pchtml_hash_copy_lower(pchtml_hash_t *hash, pchtml_hash_entry_t *entry,
                       const unsigned char *key, size_t length) WTF_INTERNAL;

unsigned int
pchtml_hash_copy_upper(pchtml_hash_t *hash, pchtml_hash_entry_t *entry,
                       const unsigned char *key, size_t length) WTF_INTERNAL;


/*
 * Inline functions
 */
static inline pchtml_mraw_t *
pchtml_hash_mraw(const pchtml_hash_t *hash)
{
    return hash->mraw;
}

static inline unsigned char *
pchtml_hash_entry_str(const pchtml_hash_entry_t *entry)
{
    if (entry->length <= PCHTML_HASH_SHORT_SIZE) {
        return (unsigned char *) entry->u.short_str;
    }

    return entry->u.long_str;
}

static inline unsigned char *
pchtml_hash_entry_str_set(pchtml_hash_entry_t *entry,
                          unsigned char *data, size_t length)
{
    entry->length = length;

    if (length <= PCHTML_HASH_SHORT_SIZE) {
        memcpy(entry->u.short_str, data, length);
        return (unsigned char *) entry->u.short_str;
    }

    entry->u.long_str = data;
    return entry->u.long_str;
}

static inline void
pchtml_hash_entry_str_free(pchtml_hash_t *hash, pchtml_hash_entry_t *entry)
{
    if (entry->length > PCHTML_HASH_SHORT_SIZE) {
        pchtml_mraw_free(hash->mraw, entry->u.long_str);
    }

    entry->length = 0;
}

static inline pchtml_hash_entry_t *
pchtml_hash_entry_create(pchtml_hash_t *hash)
{
    return (pchtml_hash_entry_t *) pchtml_dobject_calloc(hash->entries);
}

static inline pchtml_hash_entry_t *
pchtml_hash_entry_destroy(pchtml_hash_t *hash, pchtml_hash_entry_t *entry)
{
    return (pchtml_hash_entry_t *) pchtml_dobject_free(hash->entries, entry);
}

static inline size_t
pchtml_hash_entries_count(pchtml_hash_t *hash)
{
    return pchtml_dobject_allocated(hash->entries);
}


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_HASH_H */
