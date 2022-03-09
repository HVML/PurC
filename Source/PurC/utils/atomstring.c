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
#include "private/kvlist.h"
#include "private/utils.h"

#if PURC_ATOM_BUCKET_BITS > 16
#error "Too many bits reserved for bucket"
#endif

static struct atom_bucket {
    purc_atom_t     bucket_bits;
    purc_atom_t     atom_seq_id;

    struct kvlist   atom_ht;
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

static inline purc_atom_t atom_new (struct atom_bucket *bucket, char *string);

// static KVLIST (atom_ht, NULL);
// static char **quarks = NULL;
// static uintptr_t atom_seq_id = 0;

static purc_rwlock atom_rwlock;
static char *atom_block = NULL;
static int  atom_block_offset = 0;

static void atom_init_bucket (struct atom_bucket *bucket)
{
    assert (bucket->atom_seq_id == 0);

    pcutils_kvlist_init(&bucket->atom_ht, NULL);
    bucket->quarks = (char **) malloc (sizeof (char *) * ATOM_BLOCK_SIZE);
    bucket->quarks[0] = NULL;
    bucket->atom_seq_id = 1;

    if (bucket->quarks == NULL)
        assert (0);
}

static inline struct atom_bucket *atom_get_bucket (int bucket)
{
    struct atom_bucket *atom_bucket;

    if (LIKELY(bucket >= 0 && bucket < PURC_ATOM_BUCKETS_NR)) {
        atom_bucket = atom_buckets + bucket;
        if (UNLIKELY(atom_bucket->atom_seq_id == 0)) {
            atom_init_bucket (atom_bucket);
            atom_bucket->bucket_bits = BUCKET_BITS (bucket);
        }

        return atom_bucket;
    }

    assert(0);
    return NULL;
}

void
pcutils_atom_init_once (void)
{
    purc_rwlock_init (&atom_rwlock);
    if (atom_rwlock.native_impl == NULL)
        assert (0);

    /* init the default bucket only */
    atom_get_bucket(0);
}

void
pcutils_atom_term_once (void)
{
    if (atom_rwlock.native_impl)
        purc_rwlock_clear (&atom_rwlock);
    if (atom_block)
        free(atom_block);
}

purc_atom_t
purc_atom_try_string_ex (int bucket, const char *string)
{
    struct atom_bucket *atom_bucket = atom_get_bucket(bucket);
    purc_atom_t *data;
    purc_atom_t atom = 0;

    if (string == NULL || atom_bucket == NULL)
        return 0;

    purc_rwlock_reader_lock (&atom_rwlock);
    data = pcutils_kvlist_get (&atom_bucket->atom_ht, string);
    if (data)
        atom = *data;
    // memcpy (&atom, data, sizeof(purc_atom_t));
    //atom = (uint32_t)(uintptr_t)pcutils_kvlist_get (&atom_ht, string);
    purc_rwlock_reader_unlock (&atom_rwlock);

    return atom;
}

/* HOLDS: atom_rwlock_lock */
static char *
atom_strdup (const char *string)
{
    char *copy;
    size_t len;

    len = strlen (string) + 1;

    /* For strings longer than half the block size, fall back
       to strdup so that we fill our blocks at least 50%. */
    if (len > ATOM_STRING_BLOCK_SIZE / 2)
        return strdup (string);

    if (atom_block == NULL ||
            ATOM_STRING_BLOCK_SIZE - atom_block_offset < len) {
        atom_block = malloc (ATOM_STRING_BLOCK_SIZE);
        atom_block_offset = 0;
    }

    copy = atom_block + atom_block_offset;
    memcpy (copy, string, len);
    atom_block_offset += len;

    return copy;
}

/* HOLDS: purc_atom_rwlock_writer_lock */
static inline purc_atom_t
atom_from_string (struct atom_bucket *bucket,
        const char *string, bool duplicate)
{
    purc_atom_t *data;
    purc_atom_t atom = 0;

    data = pcutils_kvlist_get (&bucket->atom_ht, string);
    if (data) {
        atom = *data;
        assert (atom);
    }
    else {
        atom = atom_new (bucket,
                duplicate ? atom_strdup (string) : (char *)string);
    }

    return atom;
}

static inline purc_atom_t
atom_from_string_locked (struct atom_bucket *bucket,
        const char *string, bool duplicate)
{
    purc_atom_t atom = 0;
    if (!string)
        return 0;

    purc_rwlock_writer_lock (&atom_rwlock);
    atom = atom_from_string (bucket, string, duplicate);
    purc_rwlock_writer_unlock (&atom_rwlock);

    return atom;
}

purc_atom_t
purc_atom_from_string_ex (int bucket, const char *string)
{
    return atom_from_string_locked (atom_get_bucket(bucket), string, true);
}

purc_atom_t
purc_atom_from_static_string_ex (int bucket, const char *string)
{
    return atom_from_string_locked (atom_get_bucket(bucket), string, false);
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

    purc_rwlock_reader_lock (&atom_rwlock);
    if (atom < atom_bucket->atom_seq_id)
        result = atom_bucket->quarks[atom];
    purc_rwlock_reader_unlock (&atom_rwlock);

    return result;
}

/* HOLDS: purc_atom_rwlock_writer_lock */
static inline purc_atom_t
atom_new (struct atom_bucket *bucket, char *string)
{
    purc_atom_t atom;
    char **atoms_new;

    if (bucket->atom_seq_id % ATOM_BLOCK_SIZE == 0) {
        atoms_new = (char **)malloc (sizeof (char *) *
                (bucket->atom_seq_id + ATOM_BLOCK_SIZE));
        if (bucket->atom_seq_id != 0)
            memcpy (atoms_new, bucket->quarks,
                    sizeof (char *) * bucket->atom_seq_id);
        memset (atoms_new + bucket->atom_seq_id, 0,
                sizeof (char *) * ATOM_BLOCK_SIZE);
        bucket->quarks = atoms_new;
    }

    atom = bucket->atom_seq_id;
    bucket->quarks[atom] = string;
    atom |= bucket->bucket_bits;
    pcutils_kvlist_set (&bucket->atom_ht, string, &atom);
    bucket->atom_seq_id++;

    assert(IS_VALID_SEQ_ID(bucket->atom_seq_id));

    return atom;
}

