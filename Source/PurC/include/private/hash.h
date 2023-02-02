/**
 * @file hash.h
 * @author Alexander Borisov <borisov@lexbor.com>
 * @date 2021/07/02
 * @brief The hearder file for hash algorithm.
 *
 * Cleaned up and enhanced by Vincent Wei
 *
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
 * Copyright (C) 2018-2020 Alexander Borisov
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
 */

#ifndef PURC_PRIVATE_HASH_H
#define PURC_PRIVATE_HASH_H

#include "config.h"

#include "purc-utils.h"

#include "private/dobject.h"
#include "private/mraw.h"

/* VW MOTE: moved to purc-utils.h
#define PCUTILS_HASH_SHORT_SIZE     16 */
#define PCUTILS_HASH_TABLE_MIN_SIZE 32

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pcutils_hash_search pcutils_hash_search_t;
typedef struct pcutils_hash_insert pcutils_hash_insert_t;

#ifndef PCUTILS_HASH_EXTERN
extern const pcutils_hash_insert_t *pcutils_hash_insert_raw;
extern const pcutils_hash_insert_t *pcutils_hash_insert_lower;
extern const pcutils_hash_insert_t *pcutils_hash_insert_upper;

extern const pcutils_hash_search_t *pcutils_hash_search_raw;
extern const pcutils_hash_search_t *pcutils_hash_search_lower;
extern const pcutils_hash_search_t *pcutils_hash_search_upper;
#endif

/*
 * FIXME:
 * It is necessary to add the rebuild of a hash table
 * and optimize collisions.
 */
/* VW NOTE: moved to purc-utils.h
typedef struct pcutils_hash pcutils_hash_t;
typedef struct pcutils_hash_entry pcutils_hash_entry_t; */

typedef uint32_t
(*pcutils_hash_id_f)(const unsigned char *key, size_t size);

typedef unsigned int
(*pcutils_hash_copy_f)(pcutils_hash_t *hash, pcutils_hash_entry_t *entry,
                      const unsigned char *key, size_t size);

typedef bool
(*pcutils_hash_cmp_f)(const unsigned char *first,
                     const unsigned char *second, size_t size);

/* VW NOTE: moved to purc-utils.h
struct pcutils_hash_entry {
    union {
        unsigned char *long_str;
        unsigned char short_str[PCUTILS_HASH_SHORT_SIZE + 1];
    } u;

    size_t              length;

    pcutils_hash_entry_t *next;
}; */

struct pcutils_hash {
    pcutils_dobject_t    *entries;
    pcutils_mraw_t       *mraw;

    pcutils_hash_entry_t **table;
    size_t              table_size;

    size_t              struct_size;
};

struct pcutils_hash_insert {
    pcutils_hash_id_f   hash; /* For generate a hash id. */
    pcutils_hash_cmp_f  cmp;  /* For compare key. */
    pcutils_hash_copy_f copy; /* For copy key. */
};

struct pcutils_hash_search {
    pcutils_hash_id_f   hash; /* For generate a hash id. */
    pcutils_hash_cmp_f  cmp;  /* For compare key. */
};


pcutils_hash_t *
pcutils_hash_create(void) WTF_INTERNAL;

unsigned int
pcutils_hash_init(pcutils_hash_t *hash,
        size_t table_size, size_t struct_size) WTF_INTERNAL;

void
pcutils_hash_clean(pcutils_hash_t *hash) WTF_INTERNAL;

pcutils_hash_t *
pcutils_hash_destroy(pcutils_hash_t *hash, bool destroy_obj) WTF_INTERNAL;


void *
pcutils_hash_insert(pcutils_hash_t *hash, const pcutils_hash_insert_t *insert,
        const unsigned char *key, size_t length) WTF_INTERNAL;

void *
pcutils_hash_insert_by_entry(pcutils_hash_t *hash, pcutils_hash_entry_t *entry,
        const pcutils_hash_search_t *search,
        const unsigned char *key, size_t length) WTF_INTERNAL;

void
pcutils_hash_remove(pcutils_hash_t *hash, const pcutils_hash_search_t *search,
        const unsigned char *key, size_t length) WTF_INTERNAL;

void *
pcutils_hash_search(pcutils_hash_t *hash, const pcutils_hash_search_t *search,
        const unsigned char *key, size_t length);

void
pcutils_hash_remove_by_hash_id(pcutils_hash_t *hash, uint32_t hash_id,
        const unsigned char *key, size_t length,
        const pcutils_hash_cmp_f cmp_func) WTF_INTERNAL;

void *
pcutils_hash_search_by_hash_id(pcutils_hash_t *hash, uint32_t hash_id,
        const unsigned char *key, size_t length,
        const pcutils_hash_cmp_f cmp_func) WTF_INTERNAL;

uint32_t
pcutils_hash_make_id(const unsigned char *key, size_t length) WTF_INTERNAL;

uint32_t
pcutils_hash_make_id_lower(const unsigned char *key, size_t length) WTF_INTERNAL;

uint32_t
pcutils_hash_make_id_upper(const unsigned char *key, size_t length) WTF_INTERNAL;

unsigned int
pcutils_hash_copy(pcutils_hash_t *hash, pcutils_hash_entry_t *entry,
        const unsigned char *key, size_t length) WTF_INTERNAL;

unsigned int
pcutils_hash_copy_lower(pcutils_hash_t *hash, pcutils_hash_entry_t *entry,
        const unsigned char *key, size_t length) WTF_INTERNAL;

unsigned int
pcutils_hash_copy_upper(pcutils_hash_t *hash, pcutils_hash_entry_t *entry,
        const unsigned char *key, size_t length) WTF_INTERNAL;

/*
 * Inline functions
 */
static inline pcutils_mraw_t *
pcutils_hash_mraw(const pcutils_hash_t *hash)
{
    return hash->mraw;
}

/* VW NOTE: moved to purc-utils.h
static inline unsigned char *
pcutils_hash_entry_str(const pcutils_hash_entry_t *entry)
{
    if (entry->length <= PCUTILS_HASH_SHORT_SIZE) {
        return (unsigned char *) entry->u.short_str;
    }

    return entry->u.long_str;
} */

static inline unsigned char *
pcutils_hash_entry_str_set(pcutils_hash_entry_t *entry,
                          unsigned char *data, size_t length)
{
    entry->length = length;

    if (length <= PCUTILS_HASH_SHORT_SIZE) {
        memcpy(entry->u.short_str, data, length);
        return (unsigned char *) entry->u.short_str;
    }

    entry->u.long_str = data;
    return entry->u.long_str;
}

static inline void
pcutils_hash_entry_str_free(pcutils_hash_t *hash, pcutils_hash_entry_t *entry)
{
    if (entry->length > PCUTILS_HASH_SHORT_SIZE) {
        pcutils_mraw_free(hash->mraw, entry->u.long_str);
    }

    entry->length = 0;
}

static inline pcutils_hash_entry_t *
pcutils_hash_entry_create(pcutils_hash_t *hash)
{
    return (pcutils_hash_entry_t *) pcutils_dobject_calloc(hash->entries);
}

static inline pcutils_hash_entry_t *
pcutils_hash_entry_destroy(pcutils_hash_t *hash, pcutils_hash_entry_t *entry)
{
    return (pcutils_hash_entry_t *) pcutils_dobject_free(hash->entries, entry);
}

static inline size_t
pcutils_hash_entries_count(pcutils_hash_t *hash)
{
    return pcutils_dobject_allocated(hash->entries);
}


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PURC_PRIVATE_HASH_H */
