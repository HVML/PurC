/*
 * @file atomstring.c
 * @author Vincent Wei
 * @date 2021/07/07
 * @brief The implementation of atom string.
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

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "purc-ports.h"
#include "purc-utils.h"
#include "purc-errors.h"
#include "private/instance.h"
#include "private/map.h"
#include "private/utils.h"

#if PURC_ATOM_BUCKET_BITS > 16
#error "Too many bits reserved for bucket"
#endif

static struct atom_bucket {
    purc_atom_t     bucket_bits;
    purc_atom_t     atom_seq_id;

    pcutils_uomap   *atom_map;
    char**          quarks;
} atom_buckets[PURC_ATOM_BUCKETS_NR];

#define ATOM_BITS_NR        (sizeof(purc_atom_t) << 3)

#define BUCKET_BITS(bucket)       \
    ((purc_atom_t)bucket << (ATOM_BITS_NR - PURC_ATOM_BUCKET_BITS))

#define ATOM_TO_BUCKET(atom)       \
    ((int)(atom >> (ATOM_BITS_NR - PURC_ATOM_BUCKET_BITS)))

#define ATOM_TO_SEQUENCE(atom)       \
    ((atom << PURC_ATOM_BUCKET_BITS) >> PURC_ATOM_BUCKET_BITS)

#define IS_VALID_SEQ_ID(seq)    \
    (seq < ((purc_atom_t)1 << (ATOM_BITS_NR - PURC_ATOM_BUCKET_BITS)))

#define ATOM_BLOCK_SIZE         (1024 >> PURC_ATOM_BUCKET_BITS)
#define ATOM_STRING_BLOCK_SIZE  (4096 - sizeof (size_t))

static inline purc_atom_t
atom_new(struct atom_bucket *bucket, char *string, bool need_free);

static purc_rwlock atom_rwlock;
static char *atom_block = NULL;
static int  atom_block_offset = 0;

static void atom_init_bucket(struct atom_bucket *bucket)
{
    assert (bucket->atom_seq_id == 0);

    bucket->atom_map = pcutils_uomap_create(NULL, NULL, NULL, NULL,
            NULL, comp_key_string, false);
    bucket->quarks = (char **)malloc(sizeof(char *) * ATOM_BLOCK_SIZE);
    bucket->quarks[0] = NULL;
    bucket->atom_seq_id = 1;

    assert(bucket->quarks != NULL);
}

static inline struct atom_bucket *atom_get_bucket(int bucket)
{
    assert(bucket >= 0 && bucket < PURC_ATOM_BUCKETS_NR);

    struct atom_bucket *atom_bucket = atom_buckets + bucket;
    if (UNLIKELY(atom_bucket->atom_seq_id == 0)) {
        atom_init_bucket(atom_bucket);
        atom_bucket->bucket_bits = BUCKET_BITS(bucket);
    }

    return atom_bucket;
}

static inline void atom_put_bucket(int bucket)
{
    assert(bucket >= 0 && bucket < PURC_ATOM_BUCKETS_NR);

    struct atom_bucket *atom_bucket = atom_buckets + bucket;
    if (LIKELY(atom_bucket->atom_map)) {
        pcutils_uomap_destroy(atom_bucket->atom_map);
        free(atom_bucket->quarks);
        memset(atom_bucket, 0, sizeof(*atom_bucket));
    }
}

purc_atom_t
purc_atom_try_string_ex(int bucket, const char *string)
{
    struct atom_bucket *atom_bucket = atom_get_bucket(bucket);
    const pcutils_uomap_entry* entry = NULL;
    purc_atom_t atom = 0;

    if (string == NULL || atom_bucket == NULL)
        return 0;

    purc_rwlock_reader_lock(&atom_rwlock);
    if ((entry = pcutils_uomap_find(atom_bucket->atom_map, string))) {
        atom = (purc_atom_t)(uintptr_t)pcutils_uomap_entry_val(entry);
    }
    purc_rwlock_reader_unlock(&atom_rwlock);

    return atom;
}

bool
purc_atom_remove_string_ex(int bucket, const char *string)
{
    struct atom_bucket *atom_bucket = atom_get_bucket(bucket);

    if (string == NULL || atom_bucket == NULL)
        return false;

    const pcutils_uomap_entry* entry;
    bool ret;
    purc_atom_t atom;

    purc_rwlock_writer_lock(&atom_rwlock);
    if ((entry = pcutils_uomap_find(atom_bucket->atom_map, string))) {
        atom = (purc_atom_t)(uintptr_t)pcutils_uomap_entry_val(entry);
        pcutils_uomap_erase(atom_bucket->atom_map, (void *)string);
        atom = ATOM_TO_SEQUENCE(atom);
        atom_bucket->quarks[atom] = NULL;
        ret = true;
    }
    else {
        ret = false;
    }
    purc_rwlock_writer_unlock(&atom_rwlock);

    return ret;
}

/* HOLDS: atom_rwlock_lock */
static char *
atom_strdup(const char *string, bool *need_free)
{
    char *copy;
    size_t len;

    len = strlen(string) + 1;

    /* For strings longer than half the block size, fall back
       to strdup so that we fill our blocks at least 50%. */
    if (len > ATOM_STRING_BLOCK_SIZE / 2 ||
            atom_block_offset + len > ATOM_STRING_BLOCK_SIZE) {
        *need_free = true;
        return strdup(string);
    }

    *need_free = false;
    if (atom_block == NULL) {
        atom_block = malloc(ATOM_STRING_BLOCK_SIZE);
    }

    copy = atom_block + atom_block_offset;
    memcpy(copy, string, len);
    atom_block_offset += len;

    return copy;
}

/* HOLDS: purc_atom_rwlock_writer_lock */
static inline purc_atom_t
atom_from_string(struct atom_bucket *bucket, const char *string,
        bool duplicate, bool *newly_created)
{
    purc_atom_t atom = 0;
    pcutils_uomap_entry* entry;

    entry = pcutils_uomap_find(bucket->atom_map, string);
    if (entry) {
        atom = (purc_atom_t)(uintptr_t)pcutils_uomap_entry_val(entry);
        assert(atom);

        if (newly_created)
            *newly_created = false;
    }
    else {
        bool need_free;
        if (duplicate)
            string = atom_strdup(string, &need_free);
        else
            need_free = false;
        atom = atom_new(bucket, (char *)string, need_free);

        if (newly_created)
            *newly_created = true;
    }

    return atom;
}

static inline purc_atom_t
atom_from_string_locked(struct atom_bucket *bucket, const char *string,
        bool duplicate, bool *newly_created)
{
    purc_atom_t atom = 0;
    purc_rwlock_writer_lock(&atom_rwlock);
    atom = atom_from_string(bucket, string, duplicate, newly_created);
    purc_rwlock_writer_unlock(&atom_rwlock);

    return atom;
}

purc_atom_t
purc_atom_from_string_ex2(int bucket, const char *string, bool *newly_created)
{
    if (!string)
        return 0;

    return atom_from_string_locked(atom_get_bucket(bucket), string,
            true, newly_created);
}

purc_atom_t
purc_atom_from_static_string_ex2(int bucket, const char *string,
        bool *newly_created)
{
    if (!string)
        return 0;

    return atom_from_string_locked(atom_get_bucket(bucket), string,
            false, newly_created);
}

const char *
purc_atom_to_string(purc_atom_t atom)
{
    int bucket;
    char* result = NULL;

    if (atom == 0)
        return NULL;

    bucket = ATOM_TO_BUCKET(atom);
    struct atom_bucket *atom_bucket = atom_get_bucket(bucket);
    atom = ATOM_TO_SEQUENCE(atom);
    purc_rwlock_reader_lock(&atom_rwlock);
    if (atom < atom_bucket->atom_seq_id)
        result = atom_bucket->quarks[atom];
    purc_rwlock_reader_unlock(&atom_rwlock);

    return result;
}

static void free_dup_key(void *key, void *data)
{
    UNUSED_PARAM(data);
    if (key)
        free(key);
}

/* HOLDS: purc_atom_rwlock_writer_lock */
static inline purc_atom_t
atom_new(struct atom_bucket *bucket, char *string, bool need_free)
{
    purc_atom_t atom;
    char **atoms_new;

    if (bucket->atom_seq_id % ATOM_BLOCK_SIZE == 0) {
        atoms_new = (char **)malloc(sizeof (char *) *
                (bucket->atom_seq_id + ATOM_BLOCK_SIZE));
        if (bucket->atom_seq_id != 0)
            memcpy(atoms_new, bucket->quarks,
                    sizeof (char *) * bucket->atom_seq_id);
        memset(atoms_new + bucket->atom_seq_id, 0,
                sizeof (char *) * ATOM_BLOCK_SIZE);

        /*
         * The implementation in glib did not free the old quarks array.
         * The author said: `This leaks the old quarks array. Its unfortunate,
         * but it allows us to do lockless lookup of the arrays, and there
         * shouldn't be that many quarks in an app`.
         *
         * In our implementation, we do free the old quarks.
         * Indeed, we can use realloc() instead of malloc() and memcpy()
         */
        free(bucket->quarks);
        bucket->quarks = atoms_new;
    }

    atom = bucket->atom_seq_id;
    bucket->quarks[atom] = string;
    atom |= bucket->bucket_bits;
    pcutils_uomap_insert_ex(bucket->atom_map,
                string, (void *)(uintptr_t)atom,
                need_free ? free_dup_key : NULL);
    bucket->atom_seq_id++;

    assert(IS_VALID_SEQ_ID(bucket->atom_seq_id));

    return atom;
}

static void
atom_cleanup_once(void)
{
    int bucket;

    for (bucket = 0; bucket < PURC_ATOM_BUCKETS_NR; bucket++) {
        atom_put_bucket(bucket);
    }

    if (atom_rwlock.native_impl)
        purc_rwlock_clear(&atom_rwlock);
    if (atom_block)
        free(atom_block);
}

static int
atom_init_once(void)
{
    int r = 0;

    purc_rwlock_init(&atom_rwlock);
    if (atom_rwlock.native_impl == NULL)
        goto fail_lock;

    /* init the default bucket only */
    if (!atom_get_bucket(0))
        goto fail_atom;

    r = atexit(atom_cleanup_once);
    if (r)
        goto fail_atexit;

    return 0;

fail_atexit:
    atom_put_bucket(0);
    if (atom_block) {
        free(atom_block);
        atom_block = NULL;
    }

fail_atom:
    purc_rwlock_clear(&atom_rwlock);

fail_lock:
    return -1;
}

struct pcmodule _module_atom = {
    .id              = PURC_HAVE_UTILS,
    .module_inited   = 0,

    .init_once       = atom_init_once,
    .init_instance   = NULL,
};

