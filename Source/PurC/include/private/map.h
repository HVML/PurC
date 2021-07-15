/*
 * @file map.h
 * @author Vincent Wei
 * @date 2021/07/15
 * @brief the header for map based on red-black tree.
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
 * Note that the code is copied from GPL'd MiniGUI developed by FMSoft.
 */

#ifndef PURC_PRIVATE_MAP_H
#define PURC_PRIVATE_MAP_H

#include <stdlib.h>
#include <string.h>

#include "private/rbtree.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef void *(*copy_key_fn) (const void *key);
typedef void  (*free_key_fn) (void *key);
typedef void *(*copy_val_fn) (const void *val);
typedef int   (*comp_key_fn) (const void *key1, const void *key2);
typedef void  (*free_val_fn) (void *val);

/* common functions for string key */
static inline void* copy_key_string (const void *key)
{
    return strdup (key);
}

static inline void free_key_string (void *key)
{
    free (key);
}

static inline int comp_key_string (const void *key1, const void *key2)
{
    return strcmp (key1, key2);
}

typedef struct pcutils_map pcutils_map;
typedef struct pcutils_map_entry {
    struct rb_node  node;
    void*           key;
    void*           val;
    free_val_fn     free_val_alt;   // alternative free function per entry
} pcutils_map_entry;

pcutils_map* pcutils_map_create (copy_key_fn copy_key, free_key_fn free_key,
        copy_val_fn copy_val, free_val_fn free_val,
        comp_key_fn comp_key, bool threads);
int pcutils_map_destroy (pcutils_map* map);
int pcutils_map_clear (pcutils_map* map);
int pcutils_map_get_size (pcutils_map* map);

pcutils_map_entry* pcutils_map_find (pcutils_map* map, const void* key);

int pcutils_map_insert_ex (pcutils_map* map, const void* key,
        const void* val, free_val_fn free_val_alt);
static inline int  pcutils_map_insert (pcutils_map* map, const void* key,
        const void* val)
{
    return pcutils_map_insert_ex (map, key, val, NULL);
}

int pcutils_map_find_replace_or_insert (pcutils_map* map, const void* key,
        const void* val, free_val_fn free_val_alt);

int pcutils_map_replace (pcutils_map* map, const void* key,
        const void* val, free_val_fn free_val_alt);

int pcutils_map_erase (pcutils_map* map, void* key);
int pcutils_map_get_size (pcutils_map* map);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* PURC_PRIVATE_MAP_H */

