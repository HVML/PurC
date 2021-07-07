/*
 * @file atom.c
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

#define ATOM_BLOCK_SIZE         2048
#define ATOM_STRING_BLOCK_SIZE (4096 - sizeof (size_t))

static inline purc_atom_t atom_new (char *string);

static purc_rwlock atom_rwlock;
static KVLIST (atom_ht, NULL);
static char **quarks = NULL;
static uintptr_t atom_seq_id = 0;
static char *atom_block = NULL;
static int  atom_block_offset = 0;

void
pcutils_init_atom (void)
{
    assert (atom_seq_id == 0);

    purc_rwlock_init (&atom_rwlock);
    quarks = (char **) malloc (sizeof (char *) * ATOM_BLOCK_SIZE);
    quarks[0] = NULL;
    atom_seq_id = 1;

    if (atom_rwlock.native_impl == NULL || quarks == NULL)
        assert (0);
}

purc_atom_t
purc_atom_try_string (const char *string)
{
    purc_atom_t atom = 0;

    if (string == NULL)
        return 0;

    purc_rwlock_reader_lock (&atom_rwlock);
    atom = (uint32_t)(uintptr_t)pcutils_kvlist_get (&atom_ht, string);
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
atom_from_string (const char *string, bool duplicate)
{
    purc_atom_t atom = 0;

    atom = (uintptr_t)pcutils_kvlist_get (&atom_ht, string);
    if (!atom) {
        atom = atom_new (duplicate ? atom_strdup (string) : (char *)string);
    }

    return atom;
}

static inline purc_atom_t
atom_from_string_locked (const char *string, bool duplicate)
{
    purc_atom_t atom = 0;
    if (!string)
        return 0;

    purc_rwlock_writer_lock (&atom_rwlock);
    atom = atom_from_string (string, duplicate);
    purc_rwlock_writer_unlock (&atom_rwlock);

    return atom;
}

purc_atom_t
purc_atom_from_string (const char *string)
{
  return atom_from_string_locked (string, true);
}

purc_atom_t
purc_atom_from_static_string (const char *string)
{
  return atom_from_string_locked (string, false);
}

const char *
purc_atom_to_string (purc_atom_t atom)
{
    char* result = NULL;

    purc_rwlock_reader_lock (&atom_rwlock);
    if (atom < atom_seq_id)
        result = quarks[atom];
    purc_rwlock_reader_unlock (&atom_rwlock);

    return result;
}

/* HOLDS: purc_atom_rwlock_writer_lock */
static inline purc_atom_t
atom_new (char *string)
{
    purc_atom_t atom;
    char **atoms_new;

    if (atom_seq_id % ATOM_BLOCK_SIZE == 0) {
        atoms_new = (char **)malloc (sizeof (char *) * (atom_seq_id + ATOM_BLOCK_SIZE));
        if (atom_seq_id != 0)
            memcpy (atoms_new, quarks, sizeof (char *) * atom_seq_id);
        memset (atoms_new + atom_seq_id, 0, sizeof (char *) * ATOM_BLOCK_SIZE);
        quarks = atoms_new;
    }

    atom = atom_seq_id;
    quarks[atom] = string;
    pcutils_kvlist_set (&atom_ht, string, (void *)atom);
    atom_seq_id++;

    return atom;
}

