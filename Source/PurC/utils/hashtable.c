/*
 * @file hashtable.c
 * @author Vincent Wei <https://github.com/VincentWei>
 * @date 2021/07/07
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
 * Note that the code is derived from json-c which is licensed under MIT Licence.
 *
 * Author: Michael Clark <michael@metaparadigm.com>
 * Copyright (c) 2004, 2005 Metaparadigm Pte. Ltd.
 * Copyright (c) 2009 Hewlett-Packard Development Company, L.P.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

// #undef NDEBUG

#include "config.h"
#include "purc-utils.h"
#include "private/hashtable.h"
#include "private/debug.h"

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#if HAVE(GLIB)
#include <gmodule.h>
#endif

/** The minimum/default size of the hash table (must be larger than 4) */
#define PCHASH_DEFAULT_SIZE     4

/** Define this if you want to sort the entries in a slot */
#define PCHASH_FLAG_SORTED      0x0001

#define WRLOCK_INIT(t)                          \
        purc_rwlock_init(&(t)->rwlock)

#define WRLOCK_CLEAR(t)                         \
    if ((t)->rwlock.native_impl)                \
        purc_rwlock_clear(&(t)->rwlock)

#define RDLOCK_TABLE(t)                         \
    if ((t)->rwlock.native_impl)                \
        purc_rwlock_reader_lock(&(t)->rwlock)

#define RDUNLOCK_TABLE(t)                       \
    if ((t)->rwlock.native_impl)                \
        purc_rwlock_reader_unlock(&(t)->rwlock)

#define WRLOCK_TABLE(t)                         \
    if ((t)->rwlock.native_impl)                \
        purc_rwlock_writer_lock(&(t)->rwlock)

#define WRUNLOCK_TABLE(t)                       \
    if ((t)->rwlock.native_impl)                \
        purc_rwlock_writer_unlock(&(t)->rwlock)

#if HAVE(GLIB)
static inline UNUSED_FUNCTION pchash_entry *alloc_entry(void) {
    return (pchash_entry *)g_slice_alloc(sizeof(pchash_entry));
}

static inline UNUSED_FUNCTION pchash_entry *alloc_entry_0(void) {
    return (pchash_entry *)g_slice_alloc0(sizeof(pchash_entry));
}

static inline void free_entry(pchash_entry *v) {
    return g_slice_free1(sizeof(pchash_entry), (gpointer)v);
}
#else
static inline UNUSED_FUNCTION pchash_entry *alloc_entry(void) {
    return (pchash_entry *)malloc(sizeof(pchash_entry));
}

static inline UNUSED_FUNCTION pchash_entry *alloc_entry_0(void) {
    return (pchash_entry *)calloc(1, sizeof(pchash_entry));
}

static inline void free_entry(pchash_entry *v) {
    return free(v);
}
#endif

static inline size_t normalize_size(size_t expected)
{
    size_t normalized;

    if (expected < PCHASH_DEFAULT_SIZE)
        expected = PCHASH_DEFAULT_SIZE;

    normalized = pcutils_get_next_fibonacci_number(expected / 4 * 5);
    if (normalized > UINT32_MAX)
        normalized = UINT32_MAX;

    return normalized;
}

struct pchash_table *pchash_table_new(size_t size,
        pchash_copy_key_fn copy_key, pchash_free_key_fn free_key,
        pchash_copy_val_fn copy_val, pchash_free_val_fn free_val,
        pchash_hash_fn hash_fn, pchash_keycmp_fn keycmp_fn,
        bool threads, bool sorted)
{
    struct pchash_table *t;
    t = (pchash_table *)calloc(1, sizeof(pchash_table));
    if (!t)
        return NULL;

    if (sorted)
        t->flags |= PCHASH_FLAG_SORTED;

    t->size = normalize_size(size);
    t->table = (struct list_head *)calloc(t->size, sizeof(struct list_head));
    if (!t->table) {
        free(t);
        return NULL;
    }

    t->count = 0;
    t->copy_key = copy_key;
    t->free_key = free_key;
    t->copy_val = copy_val;
    t->free_val = free_val;
    t->hash_fn = hash_fn;
    t->keycmp_fn = keycmp_fn;

    for (size_t i = 0; i < t->size; i++) {
        list_head_init(t->table + i);
    }

    if (threads)
        WRLOCK_INIT(t);

    return t;
}

static void add_entry_sorted(struct pchash_table *t, pchash_entry *ent)
{
    struct list_head *list;
    struct list_head *slot = t->table + ent->slot;

    /* compare with the last entry first */
    if (!list_empty(slot)) {
        struct pchash_entry *node;
        node = list_last_entry(slot, pchash_entry, list);
        if (t->keycmp_fn(ent->key, node->key) > 0) {
            list = slot;
            goto done;
        }
    }

    list_for_each(list, slot) {
        struct pchash_entry *node = list_entry(list, pchash_entry, list);
        if (t->keycmp_fn(ent->key, node->key) <= 0) {
            break;
        }
    }

done:
    list_add_tail(&ent->list, list);
    t->count++;
}

static inline void add_entry(struct pchash_table *t, pchash_entry *ent)
{
    list_add_tail(&ent->list, t->table + ent->slot);
    t->count++;
}

int pchash_table_resize(struct pchash_table *t, size_t new_size)
{
    size_t normalized = normalize_size(new_size);
    if (normalized == t->size) {
        return 0;
    }

    struct pchash_table nt = { };
    nt.flags = t->flags;
    nt.count = 0;
    nt.size = normalized;
    nt.hash_fn = t->hash_fn;
    nt.keycmp_fn = t->keycmp_fn;
    nt.table = (struct list_head *)calloc(nt.size, sizeof(struct list_head));
    if (nt.table == NULL) {
        return -1;
    }

    for (size_t i = 0; i < nt.size; i++) {
        list_head_init(nt.table + i);
    }

    struct pchash_entry *ent;
    for (size_t i = 0; i < t->size; i++) {
        struct list_head *p, *n;
        list_for_each_safe(p, n, t->table + i) {
            ent = list_entry(p, pchash_entry, list);
            list_del(&ent->list);
            ent->slot = ent->hash % nt.size;
            if (t->flags & PCHASH_FLAG_SORTED)
                add_entry_sorted(&nt, ent);
            else
                add_entry(&nt, ent);
        }
    }

    t->size = nt.size;
    free(t->table);
    t->table = nt.table;

    return 0;
}

void pchash_table_reset(struct pchash_table *t)
{
    struct pchash_entry *c;

    for (size_t i = 0; i < t->size; i++) {
        struct list_head *p, *n;
        list_for_each_safe(p, n, t->table + i) {
            c = list_entry(p, pchash_entry, list);
            if (c->free_kv_alt) {
                c->free_kv_alt(c->key, c->val);
            }
            else {
                if (t->free_key) {
                    t->free_key(c->key);
                }

                if (t->free_val) {
                    t->free_val(c->val);
                }
            }

            list_del(&c->list);
            free_entry(c);
        }

        assert(list_empty(t->table + i));
    }

    t->count = 0;
}

void pchash_table_delete(struct pchash_table *t)
{
    pchash_table_reset(t);
    WRLOCK_CLEAR(t);
    free(t->table);
    free(t);
}

static int insert_entry(struct pchash_table *t,
        const void *k, const void *v, const uint32_t h,
        pchash_free_kv_fn free_kv_alt)
{
    if (pchash_table_resize(t, t->count + 1))
        return -1;

    pchash_entry *ent = alloc_entry_0();
    if (ent == NULL)
        return -1;

    ent->key = (t->copy_key != NULL) ? t->copy_key(k) : (void *)k;
    ent->val = (t->copy_val != NULL) ? t->copy_val(v) : (void *)v;
    ent->free_kv_alt = free_kv_alt;
    ent->hash = h;
    ent->slot = h % t->size;
    if (t->flags & PCHASH_FLAG_SORTED)
        add_entry_sorted(t, ent);
    else
        add_entry(t, ent);

    return 0;
}

int pchash_table_insert_w_hash(struct pchash_table *t,
        const void *k, const void *v, const uint32_t h,
        pchash_free_kv_fn free_kv_alt)
{
    int retv;

    WRLOCK_TABLE(t);
    retv = insert_entry(t, k, v, h, free_kv_alt);
    WRUNLOCK_TABLE(t);
    return retv;
}

int pchash_table_insert_ex(struct pchash_table *t,
        const void *k, const void *v, pchash_free_kv_fn free_kv_alt)
{
    return pchash_table_insert_w_hash(t, k, v,
            pchash_get_hash(t, k), free_kv_alt);
}

static pchash_entry_t find_entry(struct pchash_table *t,
        const void *k, const uint32_t h)
{
    struct list_head *slot = t->table + h % t->size;
    struct pchash_entry *found = NULL;

    if (list_empty(slot)) {
        goto done;
    }
    else if (t->flags & PCHASH_FLAG_SORTED) {
        /* check if the key out of the rang of this slot. */
        pchash_entry_t e;
        e = list_first_entry(slot, pchash_entry, list);
        if (t->keycmp_fn(k, e->key) < 0)
            goto done;

        e = list_last_entry(slot, pchash_entry, list);
        if (t->keycmp_fn(k, e->key) > 0)
            goto done;
    }

    struct list_head *p;
    list_for_each(p, slot) {
        pchash_entry_t ent;
        ent = list_entry(p, pchash_entry, list);
        if (t->keycmp_fn(ent->key, k) == 0) {
            found = ent;
            break;
        }
    }

done:
    return found;
}

pchash_entry_t pchash_table_lookup_entry_w_hash(struct pchash_table *t,
        const void *k, const uint32_t h)
{
    pchash_entry_t found = NULL;

    RDLOCK_TABLE(t);
    found = find_entry(t, k, h);
    RDUNLOCK_TABLE(t);

    return found;
}

pchash_entry_t pchash_table_lookup_entry(struct pchash_table *t,
        const void *k)
{
    return pchash_table_lookup_entry_w_hash(t, k, pchash_get_hash(t, k));
}

pchash_entry_t pchash_table_lookup_and_lock_w_hash(
        struct pchash_table *t, const void *k, const uint32_t h)
{
    pchash_entry_t found = NULL;

    WRLOCK_TABLE(t);
    found = find_entry(t, k, h);
    if (found == NULL)
        WRUNLOCK_TABLE(t);

    return found;
}

pchash_entry_t pchash_table_lookup_and_lock(struct pchash_table *t,
        const void *k)
{
    return pchash_table_lookup_and_lock_w_hash(t, k, pchash_get_hash(t, k));
}

bool pchash_table_lookup_ex(struct pchash_table *t, const void *k, void **v)
{
    pchash_entry_t e = pchash_table_lookup_entry(t, k);

    if (e != NULL) {
        if (v != NULL)
            *v = e->val;
        return true; /* key found */
    }

    if (v != NULL)
        *v = NULL;
    return false; /* key not found */
}

static int erase_entry(struct pchash_table *t, pchash_entry_t e)
{
    assert(e->slot < t->size);

    if (e->free_kv_alt) {
        e->free_kv_alt(e->key, e->val);
    }
    else {
        if (t->free_key) {
            t->free_key(e->key);
        }

        if (t->free_val) {
            t->free_val(e->val);
        }
    }

    list_del(&e->list);
    free_entry(e);

    t->count--;
    if (pchash_table_resize(t, t->count))
        return -1;

    return 0;
}

int pchash_table_erase_entry(struct pchash_table *t, pchash_entry_t e)
{
    int retv;

    WRLOCK_TABLE(t);
    retv = erase_entry(t, e);
    WRUNLOCK_TABLE(t);
    return retv;
}

int pchash_table_erase(struct pchash_table *t, const void *k)
{
    int retv = -1;
    pchash_entry_t e;

    WRLOCK_TABLE(t);

    e = find_entry(t, k, pchash_get_hash(t, k));
    if (e) {
        retv = erase_entry(t, e);
        assert(retv == 0);
    }

    WRUNLOCK_TABLE(t);
    return retv;
}

int pchash_table_erase_nolock(struct pchash_table *t, pchash_entry_t e)
{
    return erase_entry(t, e);
}

int pchash_table_replace(struct pchash_table *t,
        const void *k, const void *v, pchash_free_kv_fn free_kv_alt)
{
    int retv = -1;
    pchash_entry_t e;

    WRLOCK_TABLE(t);

    e = find_entry(t, k, pchash_get_hash(t, k));
    if (e == NULL) {
        goto ret;
    }

    retv = 0;

    if (e->free_kv_alt) {
        e->free_kv_alt(NULL, e->val);
    }
    else if (t->free_val) {
        t->free_val(e->val);
    }

    if (t->copy_val) {
        e->val = t->copy_val(v);
    }
    else
        e->val = (void*)v;

    e->free_kv_alt = free_kv_alt;

ret:
    WRUNLOCK_TABLE(t);
    return retv;
}

int pchash_table_replace_or_insert(struct pchash_table *t,
        const void *k, const void *v, pchash_free_kv_fn free_kv_alt)
{
    int retv = -1;
    pchash_entry_t e;
    uint32_t h = pchash_get_hash(t, k);

    WRLOCK_TABLE(t);

    e = find_entry(t, k, h);
    if (e) {
        retv = 0;

        if (e->free_kv_alt) {
            e->free_kv_alt(NULL, e->val);
        }
        else if (t->free_val) {
            t->free_val(e->val);
        }

        if (t->copy_val) {
            e->val = t->copy_val(v);
        }
        else
            e->val = (void*)v;

        e->free_kv_alt = free_kv_alt;
    }
    else {
        retv = insert_entry(t, k, v, h, free_kv_alt);
    }

    WRUNLOCK_TABLE(t);
    return retv;
}

