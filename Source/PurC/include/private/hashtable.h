/*
 * @file hashtable.h
 * @author Michael Clark <michael@metaparadigm.com>
 * @date 2021/07/07
 * @brief The interfaces for hash table.
 *
 * Cleaned up by Vincent Wei
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
 * Note that the code is derived from json-c which is licensed under MIT Licence.
 *
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

#ifndef PURC_PRIVATE_HASHTABLE_H
#define PURC_PRIVATE_HASHTABLE_H

#define PCHASH_OBJECT_KEY_IS_NEW        (1 << 1)
#define PCHASH_OBJECT_KEY_IS_CONSTANT   (1 << 2)

#ifdef __cplusplus
extern "C" {
#endif

#define HASHTABLE_DEFAULT_SIZE    32

unsigned long pchash_default_char_hash(const void *k);
unsigned long pchash_perllike_str_hash(const void *k);

struct pchash_entry;

/**
 * callback function prototypes
 */
typedef void(pchash_entry_free_fn)(struct pchash_entry *e);
/**
 * callback function prototypes
 */
typedef unsigned long(pchash_hash_fn)(const void *k);
/**
 * callback function prototypes
 */
typedef int(pchash_equal_fn)(const void *k1, const void *k2);

/**
 * An entry in the hash table
 */
struct pchash_entry {
    /**
     * The key.  Use pchash_entry_k() instead of accessing this directly.
     */
    const void *k;
    /**
     * A flag for users of hash to know whether or not they
     * need to free k.
     */
    int k_is_constant;
    /**
     * The value.  Use pchash_entry_v() instead of accessing this directly.
     */
    const void *v;
    /**
     * The next entry
     */
    struct pchash_entry *next;
    /**
     * The previous entry.
     */
    struct pchash_entry *prev;
};

/**
 * The hash table structure.
 */
struct pchash_table {
    /**
     * Size of our hash.
     */
    int size;
    /**
     * Numbers of entries.
     */
    int count;

    /**
     * The first entry.
     */
    struct pchash_entry *head;

    /**
     * The last entry.
     */
    struct pchash_entry *tail;

    struct pchash_entry *table;

    /**
     * A pointer onto the function responsible for freeing an entry.
     */
    pchash_entry_free_fn *free_fn;
    pchash_hash_fn *hash_fn;
    pchash_equal_fn *equal_fn;
};

typedef struct pchash_table pchash_table;

/**
 * Convenience list iterator.
 */
#define pchash_foreach(table, entry) for (entry = table->head; entry; entry = entry->next)

/**
 * pchash_foreach_safe allows calling of deletion routine while iterating.
 *
 * @param table a struct pchash_table * to iterate over
 * @param entry a struct pchash_entry * variable to hold each element
 * @param tmp a struct pchash_entry * variable to hold a temporary pointer to the next element
 */
#define pchash_foreach_safe(table, entry, tmp) \
    for (entry = table->head; entry && ((tmp = entry->next) || 1); entry = tmp)

/**
 * Create a new hash table.
 *
 * @param size initial table size. The table is automatically resized
 * although this incurs a performance penalty.
 * @param free_fn callback function used to free memory for entries
 * when pchash_table_free or pchash_table_delete is called.
 * If NULL is provided, then memory for keys and values
 * must be freed by the caller.
 * @param hash_fn  function used to hash keys. 2 standard ones are defined:
 * pchash_ptr_hash and pchash_char_hash for hashing pointer values
 * and C strings respectively.
 * @param equal_fn comparison function to compare keys. 2 standard ones defined:
 * pchash_ptr_hash and pchash_char_hash for comparing pointer values
 * and C strings respectively.
 * @return On success, a pointer to the new hash table is returned.
 *     On error, a null pointer is returned.
 */
struct pchash_table *pchash_table_new(int size, pchash_entry_free_fn *free_fn,
        pchash_hash_fn *hash_fn, pchash_equal_fn *equal_fn);

/**
 * Convenience function to create a new hash table with char keys
 * by using the @pchash_default_char_hash hash function.
 *
 * @param size initial table size.
 * @param free_fn callback function used to free memory for entries.
 * @return On success, a pointer to the new hash table is returned.
 *     On error, a null pointer is returned.
 */
struct pchash_table *pchash_kchar_table_new(int size, pchash_entry_free_fn *free_fn);


/**
 * Convenience function to create a new hash table with string keys
 * by using the @pchash_perllike_str_hash hash function.
 *
 * @param size initial table size.
 * @param free_fn callback function used to free memory for entries.
 * @return On success, a pointer to the new hash table is returned.
 *     On error, a null pointer is returned.
 */
struct pchash_table *pchash_kstr_table_new(int size, pchash_entry_free_fn *free_fn);

/**
 * Convenience function to create a new hash table with ptr keys.
 *
 * @param size initial table size.
 * @param free_fn callback function used to free memory for entries.
 * @return On success, a pointer to the new hash table is returned.
 *     On error, a null pointer is returned.
 */
struct pchash_table *pchash_kptr_table_new(int size, pchash_entry_free_fn *free_fn);

/**
 * Free a hash table.
 *
 * If a pchash_entry_free_fn callback free function was provided then it is
 * called for all entries in the table.
 *
 * @param t table to free.
 */
void pchash_table_free(struct pchash_table *t);

/**
 * Insert a record into the table.
 *
 * @param t the table to insert into.
 * @param k a pointer to the key to insert.
 * @param v a pointer to the value to insert.
 *
 * @return On success, <code>0</code> is returned.
 *     On error, a negative value is returned.
 */
int pchash_table_insert(struct pchash_table *t, const void *k, const void *v);

/**
 * Insert a record into the table using a precalculated key hash.
 *
 * The hash h, which should be calculated with pchash_get_hash() on k, is provided by
 *  the caller, to allow for optimization when multiple operations with the same
 *  key are known to be needed.
 *
 * @param t the table to insert into.
 * @param k a pointer to the key to insert.
 * @param v a pointer to the value to insert.
 * @param h hash value of the key to insert
 * @param opts if set to PCUTILS_JSON_C_OBJECT_KEY_IS_CONSTANT, sets pchash_entry.k_is_constant
 *             so t's free function knows to avoid freeing the key.
 */
int pchash_table_insert_w_hash(struct pchash_table *t, const void *k, const void *v,
                                  const unsigned long h, const unsigned opts);

/**
 * Lookup a record in the table.
 *
 * @param t the table to lookup
 * @param k a pointer to the key to lookup
 * @return a pointer to the record structure of the value or NULL if it does not exist.
 */
struct pchash_entry *pchash_table_lookup_entry(struct pchash_table *t, const void *k);

/**
 * Lookup a record in the table using a precalculated key hash.
 *
 * The hash h, which should be calculated with pchash_get_hash() on k, is provided by
 *  the caller, to allow for optimization when multiple operations with the same
 *  key are known to be needed.
 *
 * @param t the table to lookup
 * @param k a pointer to the key to lookup
 * @param h hash value of the key to lookup
 * @return a pointer to the record structure of the value or NULL if it does not exist.
 */
struct pchash_entry *pchash_table_lookup_entry_w_hash(struct pchash_table *t, const void *k,
                                                     const unsigned long h);

/**
 * Lookup a record in the table.
 *
 * @param t the table to lookup
 * @param k a pointer to the key to lookup
 * @param v a pointer to a where to store the found value (set to NULL if it doesn't exist).
 * @return whether or not the key was found
 */
bool pchash_table_lookup_ex(struct pchash_table *t, const void *k, void **v);

/**
 * Delete a record from the table.
 *
 * If a callback free function is provided then it is called for the
 * for the item being deleted.
 * @param t the table to delete from.
 * @param e a pointer to the entry to delete.
 * @return 0 if the item was deleted.
 * @return -1 if it was not found.
 */
int pchash_table_delete_entry(struct pchash_table *t, struct pchash_entry *e);

/**
 * Delete a record from the table.
 *
 * If a callback free function is provided then it is called for the
 * for the item being deleted.
 * @param t the table to delete from.
 * @param k a pointer to the key to delete.
 * @return 0 if the item was deleted.
 * @return -1 if it was not found.
 */
int pchash_table_delete(struct pchash_table *t, const void *k);

int pchash_table_length(struct pchash_table *t);

/**
 * Resizes the specified table.
 *
 * @param t Pointer to table to resize.
 * @param new_size New table size. Must be positive.
 *
 * @return On success, <code>0</code> is returned.
 *     On error, a negative value is returned.
 */
int pchash_table_resize(struct pchash_table *t, int new_size);

/**
 * Calculate the hash of a key for a given table.
 *
 * This is an exension to support functions that need to calculate
 * the hash several times and allows them to do it just once and then pass
 * in the hash to all utility functions. Depending on use case, this can be a
 * considerable performance improvement.
 * @param t the table (used to obtain hash function)
 * @param k a pointer to the key to lookup
 * @return the key's hash
 */
static inline unsigned long pchash_get_hash(const struct pchash_table *t, const void *k)
{
    return t->hash_fn(k);
}

/**
 * @deprecated Don't use this outside of hash.h:
 */
#ifdef __UNCONST
#define _PCHASH_UNCONST(a) __UNCONST(a)
#else
#define _PCHASH_UNCONST(a) ((void *)(uintptr_t)(const void *)(a))
#endif

/**
 * Return a non-const version of pchash_entry.k.
 *
 * pchash_entry.k is const to indicate and help ensure that hash itself doesn't modify
 * it, but callers are allowed to do what they want with it.
 * See also pchash_entry.k_is_constant
 */
#define pchash_entry_k(entry) _PCHASH_UNCONST((entry)->k)

/**
 * Return a non-const version of pchash_entry.v.
 *
 * v is const to indicate and help ensure that hash itself doesn't modify
 * it, but callers are allowed to do what they want with it.
 */
#define pchash_entry_v(entry) _PCHASH_UNCONST((entry)->v)

#ifdef __cplusplus
}
#endif

#endif /* PURC_PRIVATE_HASHTABLE_H */
