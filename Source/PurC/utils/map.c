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
#include "private/debug.h"
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

void pcutils_map_lock(pcutils_map *map)
{
    WRLOCK_MAP(map);
}

void pcutils_map_unlock(pcutils_map *map)
{
    WRUNLOCK_MAP(map);
}

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

static inline UNUSED_FUNCTION pcutils_map_entry *alloc_entry_0(void) {
    return (pcutils_map_entry *)g_slice_alloc0(sizeof(pcutils_map_entry));
}

static inline void free_entry(pcutils_map_entry *v) {
    return g_slice_free1(sizeof(pcutils_map_entry), (gpointer)v);
}
#else
static inline UNUSED_FUNCTION pcutils_map_entry *alloc_entry(void) {
    return (pcutils_map_entry *)malloc(sizeof(pcutils_map_entry));
}

static inline UNUSED_FUNCTION pcutils_map_entry *alloc_entry_0(void) {
    return (pcutils_map_entry *)calloc(1, sizeof(pcutils_map_entry));
}

static inline void free_entry(pcutils_map_entry *v) {
    return free(v);
}
#endif

static pcutils_map_entry* new_entry (pcutils_map* map, const void* key,
        const void* val, free_kv_fn free_kv_alt)
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

        entry->free_kv_alt = free_kv_alt;
    }

    return entry;
}

static void clear_node (pcutils_map* map, struct rb_node* node)
{
    if (node) {
        pcutils_map_entry *entry = (pcutils_map_entry*)node;

        if (entry->free_kv_alt) {
            entry->free_kv_alt (entry->key, entry->val);
        }
        else {
            if (map->free_key) {
                map->free_key (entry->key);
            }

            if (map->free_val) {
                map->free_val (entry->val);
            }
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

void pcutils_map_erase_entry (pcutils_map* map, pcutils_map_entry *entry)
{
    if (map == NULL || entry == NULL)
        return;

    WRLOCK_MAP (map);
    erase_entry (map, entry);
    WRUNLOCK_MAP (map);
}

int pcutils_map_replace (pcutils_map* map, const void* key,
        const void* val, free_kv_fn free_kv_alt)
{
    int retval = -1;
    pcutils_map_entry* entry = NULL;

    WRLOCK_MAP (map);

    entry = find_entry (map, key);
    if (entry == NULL) {
        goto ret;
    }

    retval = 0;
#if 0
    /* XXX: is this reasonable? */
    if (val == entry->val) {
        goto ret;
    }
#endif

    if (entry->free_kv_alt) {
        entry->free_kv_alt (NULL, entry->val);
    }
    else if (map->free_val) {
        map->free_val (entry->val);
    }

    if (map->copy_val) {
        entry->val = map->copy_val (val);
    }
    else
        entry->val = (void*)val;

    entry->free_kv_alt = free_kv_alt;

ret:
    WRUNLOCK_MAP (map);
    return retval;
}

int pcutils_map_insert_ex (pcutils_map* map, const void* key,
        const void* val, free_kv_fn free_kv_alt)
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

    int r = -1;
    if (!entry) {
        entry = new_entry (map, key, val, free_kv_alt);
        pcutils_rbtree_link_node (&entry->node,
                (struct rb_node*)parent, (struct rb_node**)pentry);
        pcutils_rbtree_insert_color (&entry->node, &map->root);
        map->size++;
        r = 0;
    }

    WRUNLOCK_MAP (map);
    return r ? -1 : 0;
}

int pcutils_map_find_replace_or_insert (pcutils_map* map, const void* key,
        const void* val, free_kv_fn free_kv_alt)
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
        entry = new_entry (map, key, val, free_kv_alt);
        pcutils_rbtree_link_node (&entry->node,
                (struct rb_node*)parent, (struct rb_node**)pentry);
        pcutils_rbtree_insert_color (&entry->node, &map->root);

        map->size++;
    }
    else {
        if (entry->free_kv_alt) {
            entry->free_kv_alt (NULL, entry->val);
        }
        else if (map->free_val) {
            map->free_val (entry->val);
        }

        if (map->copy_val) {
            entry->val = map->copy_val (val);
        }
        else
            entry->val = (void*)val;

        entry->free_kv_alt = free_kv_alt;
    }

    WRUNLOCK_MAP (map);
    return 0;
}

struct user_data {
    int (*cb)(void *key, void *val, void *ud);
    void *ud;
};

static inline int
visit_node(struct rb_node *node, void *ud)
{
    struct user_data *data = (struct user_data*)ud;
    pcutils_map_entry *entry;
    entry = container_of(node, pcutils_map_entry, node);
    return data->cb(entry->key, entry->val, data->ud);
}

static inline int
map_traverse (pcutils_map *map, void *ud,
        int (*cb)(void *key, void *val, void *ud))
{
    struct rb_root *root = &map->root;
    if (!root)
        return 0;

    struct user_data data = {
        .cb         = cb,
        .ud         = ud,
    };
    return pcutils_rbtree_traverse(root, &data, visit_node);
}

int pcutils_map_traverse (pcutils_map *map, void *ud,
        int (*cb)(void *key, void *val, void *ud))
{
    WRLOCK_MAP (map);
    int r = map_traverse(map, ud, cb);
    WRUNLOCK_MAP (map);
    return r;
}

#define IT_NEXT(curr) curr == NULL ? NULL : \
    container_of(pcutils_rbtree_next((&curr->node)), pcutils_map_entry, node)
#define IT_PREV(curr) curr == NULL ? NULL : \
    container_of(pcutils_rbtree_prev((&curr->node)), pcutils_map_entry, node)

struct pcutils_map_iterator
pcutils_map_it_begin_first(pcutils_map *map)
{
    struct pcutils_map_iterator it = {};
    if (!map)
        return it;

    /* VW: call pcutils_map_lock()/pcutils_map_unlok() explicitly instead
       PC_ASSERT((map)->rwlock.native_impl == NULL); */

    struct rb_root *root = &map->root;
    if (!root)
        return it;

    struct rb_node *first = pcutils_rbtree_first(root);
    if (first) {
        it.curr = container_of(first, pcutils_map_entry, node);
        it.next = IT_NEXT(it.curr);
    }

    return it;
}

struct pcutils_map_iterator
pcutils_map_it_begin_last(pcutils_map *map)
{
    struct pcutils_map_iterator it = {};
    if (!map)
        return it;

    /* VW: call pcutils_map_lock()/pcutils_map_unlok() explicitly instead
    PC_ASSERT((map)->rwlock.native_impl == NULL); */

    struct rb_root *root = &map->root;
    if (!root)
        return it;

    struct rb_node *last = pcutils_rbtree_last(root);
    if (last) {
        it.curr = container_of(last, pcutils_map_entry, node);
        it.prev = IT_PREV(it.curr);
    }

    return it;
}

struct pcutils_map_entry*
pcutils_map_it_value(struct pcutils_map_iterator *it)
{
    return it->curr;
}

struct pcutils_map_entry*
pcutils_map_it_next(struct pcutils_map_iterator *it)
{
    it->prev = it->curr;
    it->curr = it->next;
    it->next = IT_NEXT(it->curr);

    return it->curr;
}

struct pcutils_map_entry*
pcutils_map_it_prev(struct pcutils_map_iterator *it)
{
    it->next = it->curr;
    it->curr = it->prev;
    it->prev = IT_PREV(it->curr);

    return it->curr;
}

void
pcutils_map_it_end(struct pcutils_map_iterator *it)
{
    it->curr = NULL;
    it->next = NULL;
    it->prev = NULL;
}

