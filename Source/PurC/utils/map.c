/*
 * @file map.c
 * @author Vincent Wei
 * @date 2021/07/15
 * @brief the implementation of map based on red-black tree.
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

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "config.h"
#include "private/map.h"
#include "purc-ports.h"

#if HAVE(GLIB)
#include <gmodule.h>
#endif

#define WRLOCK_INIT(map)                        \
        purc_rwlock_init(&(map)->rwlock)

#define WRLOCK_CLEAR(map)                       \
    if ((map)->rwlock.native_impl)              \
        purc_rwlock_clear(&(map)->rwlock)

#define RDLOCK_MAP(map)                         \
    if ((map)->rwlock.native_impl)              \
        purc_rwlock_reader_lock(&(map)->rwlock)

#define RDUNLOCK_MAP(map)                       \
    if ((map)->rwlock.native_impl)              \
        purc_rwlock_reader_unlock(&(map)->rwlock)

#define WRLOCK_MAP(map)                         \
    if ((map)->rwlock.native_impl)              \
        purc_rwlock_writer_lock(&(map)->rwlock)

#define WRUNLOCK_MAP(map)                       \
    if ((map)->rwlock.native_impl)              \
        purc_rwlock_writer_unlock(&(map)->rwlock)

struct pcutils_map {
    struct rb_root  root;

    copy_key_fn     copy_key;   // If NULL, use the literal value.
    free_key_fn     free_key;   // Not NULL if copy_key is not NULL.
    copy_val_fn     copy_val;   // If NULL, use the literal value.
    free_val_fn     free_val;   // Not NULL if copy_key is not NULL.
    comp_key_fn     comp_key;   // If NULL, use the literal value of key
                                // as integer

    size_t          size;       // number of entries

    purc_rwlock     rwlock;     // read-write lock
};

pcutils_map* pcutils_map_create (copy_key_fn copy_key, free_key_fn free_key,
        copy_val_fn copy_val, free_val_fn free_val,
        comp_key_fn comp_key, bool threads)
{
    pcutils_map* map;

    if (!(map = calloc (1, sizeof(pcutils_map))))
        return NULL;

    map->copy_key = copy_key;
    map->free_key = free_key;
    map->copy_val = copy_val;
    map->free_val = free_val;
    map->comp_key = comp_key;

    if (threads)
        WRLOCK_INIT (map);
    return map;
}

size_t pcutils_map_get_size (pcutils_map* map)
{
    return map->size;
}

int pcutils_map_destroy (pcutils_map* map)
{
    if (map == NULL)
        return -1;

    pcutils_map_clear (map);

    WRLOCK_CLEAR (map);

    free (map);
    return 0;
}

#if HAVE(GLIB)
static inline UNUSED_FUNCTION pcutils_map_entry *alloc_entry(void) {
    return (pcutils_map_entry *)g_slice_alloc(sizeof(pcutils_map_entry));
}

static inline pcutils_map_entry *alloc_entry_0(void) {
    return (pcutils_map_entry *)g_slice_alloc0(sizeof(pcutils_map_entry));
}

static inline void free_entry(pcutils_map_entry *v) {
    return g_slice_free1(sizeof(pcutils_map_entry), (gpointer)v);
}
#else
static inline UNUSED_FUNCTION pcutils_map_entry *alloc_entry(void) {
    return (pcutils_map_entry *)malloc(sizeof(pcutils_map_entry));
}

static inline pcutils_map_entry *alloc_entry_0(void) {
    return (pcutils_map_entry *)calloc(1, sizeof(pcutils_map_entry));
}

static inline void free_entry(pcutils_map_entry *v) {
    return free(v);
}
#endif

static pcutils_map_entry* new_entry (pcutils_map* map, const void* key,
        const void* val, free_val_fn free_val_alt)
{
    pcutils_map_entry* entry;

    entry = alloc_entry ();
    if (entry) {
        if (map->copy_key) {
            entry->key = map->copy_key (key);
        }
        else
            entry->key = (void*)key;

        if (map->copy_val) {
            entry->val = map->copy_val (val);
        }
        else
            entry->val = (void*)val;

        entry->free_val_alt = free_val_alt;
    }

    return entry;
}

static void clear_node (pcutils_map* map, struct rb_node* node)
{
    if (node) {
        pcutils_map_entry *entry = (pcutils_map_entry*)node;

        if (map->free_key) {
            map->free_key (entry->key);
        }

        if (entry->free_val_alt) {
            entry->free_val_alt (entry->val);
        }
        else if (map->free_val) {
            map->free_val (entry->val);
        }

        clear_node (map, node->rb_left);
        clear_node (map, node->rb_right);

        free_entry (entry);
    }
}

static void erase_entry (pcutils_map* map, pcutils_map_entry *entry)
{
    pcutils_rbtree_erase (&entry->node, &map->root);
    clear_node (map, &entry->node);
    map->size--;
}

int pcutils_map_clear (pcutils_map* map)
{
    if (map == NULL)
        return -1;

    WRLOCK_MAP (map);

    clear_node (map, map->root.rb_node);
    map->root.rb_node = NULL;
    map->size = 0;

    WRUNLOCK_MAP (map);
    return 0;
}

static pcutils_map_entry *find_entry (pcutils_map* map, const void* key)
{
    pcutils_map_entry* entry = NULL;

    if (map == NULL)
        return NULL;

    entry = (pcutils_map_entry*)(map->root.rb_node);
    while (entry) {
        int ret;

        if (map->comp_key)
            ret = map->comp_key (key, entry->key);
        else
            ret = (int)((intptr_t)key - (intptr_t)entry->key);

        if (ret < 0) {
            entry = (pcutils_map_entry*)(entry->node.rb_left);
        }
        else if (ret > 0) {
            entry = (pcutils_map_entry*)(entry->node.rb_right);
        }
        else
            break;
    }

    return entry;
}

pcutils_map_entry* pcutils_map_find (pcutils_map* map, const void* key)
{
    pcutils_map_entry* entry = NULL;

    if (map == NULL)
        return NULL;

    RDLOCK_MAP (map);

    entry = find_entry (map, key);

    RDUNLOCK_MAP (map);
    return entry;
}

int pcutils_map_erase (pcutils_map* map, void* key)
{
    int retval = -1;
    pcutils_map_entry* entry = NULL;

    WRLOCK_MAP (map);

    entry = find_entry (map, key);
    if (entry) {
        erase_entry (map, entry);
        retval = 0;
    }

    WRUNLOCK_MAP (map);
    return retval;
}

int pcutils_map_replace (pcutils_map* map, const void* key,
        const void* val, free_val_fn free_val_alt)
{
    int retval = -1;
    pcutils_map_entry* entry = NULL;

    WRLOCK_MAP (map);

    entry = find_entry (map, key);
    if (entry == NULL) {
        goto ret;
    }

    retval = 0;
    /* XXX: is this reasonable? */
    if (val == entry->val) {
        goto ret;
    }

    if (entry->free_val_alt) {
        entry->free_val_alt (entry->val);
    }
    else if (map->free_val) {
        map->free_val (entry->val);
    }

    if (map->copy_val) {
        entry->val = map->copy_val (val);
    }
    else
        entry->val = (void*)val;

    entry->free_val_alt = free_val_alt;

ret:
    WRUNLOCK_MAP (map);
    return retval;
}

int pcutils_map_insert_ex (pcutils_map* map, const void* key,
        const void* val, free_val_fn free_val_alt)
{
    pcutils_map_entry **pentry;
    pcutils_map_entry *entry;
    pcutils_map_entry *parent;

    if (map == NULL)
        return -1;

    WRLOCK_MAP (map);

    pentry = (pcutils_map_entry**)&(map->root.rb_node);
    entry = NULL;
    parent = NULL;

    while (*pentry) {
        int ret;

        if (map->comp_key)
            ret = map->comp_key (key, (*pentry)->key);
        else
            ret = (int)((intptr_t)key - (intptr_t)(*pentry)->key);

        parent = *pentry;
        if (ret < 0)
            pentry = (pcutils_map_entry**)&((*pentry)->node.rb_left);
        else if (ret > 0)
            pentry = (pcutils_map_entry**)&((*pentry)->node.rb_right);
        else {
            entry = *pentry;
            break;
        }
    }

    if (!entry) {
        entry = new_entry (map, key, val, free_val_alt);
        pcutils_rbtree_link_node (&entry->node,
                (struct rb_node*)parent, (struct rb_node**)pentry);
        pcutils_rbtree_insert_color (&entry->node, &map->root);
        map->size++;
    }
    else {
        if (map->free_val) {
            map->free_val (entry->val);
        }

        if (map->copy_val) {
            entry->val = map->copy_val (val);
        }
        else
            entry->val = (void*)val;
    }

    WRUNLOCK_MAP (map);
    return 0;
}

int pcutils_map_find_replace_or_insert (pcutils_map* map, const void* key,
        const void* val, free_val_fn free_val_alt)
{
    pcutils_map_entry **pentry;
    pcutils_map_entry *entry;
    pcutils_map_entry *parent;

    if (map == NULL)
        return -1;

    WRLOCK_MAP (map);

    pentry = (pcutils_map_entry**)&(map->root.rb_node);
    entry = NULL;
    parent = NULL;
    while (*pentry) {
        int ret;

        if (map->comp_key) {
            ret = map->comp_key (key, (*pentry)->key);
        }
        else
            ret = (int)((intptr_t)key - (intptr_t)entry->key);

        parent = *pentry;
        if (ret < 0)
            pentry = (pcutils_map_entry**)&((*pentry)->node.rb_left);
        else if (ret > 0)
            pentry = (pcutils_map_entry**)&((*pentry)->node.rb_right);
        else {
            entry = *pentry;
            break;
        }
    }

    if (entry == NULL) {
        entry = new_entry (map, key, val, free_val_alt);

        pcutils_rbtree_link_node (&entry->node,
                (struct rb_node*)parent, (struct rb_node**)pentry);
        pcutils_rbtree_insert_color (&entry->node, &map->root);

        map->size++;
    }
    else {
        if (entry->free_val_alt) {
            entry->free_val_alt (entry->val);
        }
        else if (map->free_val) {
            map->free_val (entry->val);
        }

        if (map->copy_val) {
            entry->val = map->copy_val (val);
        }
        else
            entry->val = (void*)val;

        entry->free_val_alt = free_val_alt;
    }

    WRUNLOCK_MAP (map);
    return 0;
}


