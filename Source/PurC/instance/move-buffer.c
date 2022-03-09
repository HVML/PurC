/*
 * move-buffer.c -- The implementation of move buffer.
 *      Created on 8 Mar 2022
 *
 * Copyright (C) 2022 FMSoft (http://www.fmsoft.cn)
 *
 * Authors:
 *  Vincent Wei (https://github.com/VincentWei), 2022
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

#include "config.h"
#include "purc-pcrdr.h"
#include "purc-errors.h"
#include "private/instance.h"
#include "private/list.h"
#include "private/sorted-array.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/ports.h"

#include <assert.h>

#define NR_DEF_MAX_MSGS     4

struct pcinst_move_buffer {
    struct purc_mutex   lock;
    struct list_head    msgs;

    unsigned int        flags;
    size_t              max_nr_msgs;
    size_t              nr_msgs;
};

/* the header of the struct pcrdr_msg */
struct pcrdr_msg_hdr {
    purc_atom_t             owner;
    struct list_head        ln;
};

/* Make sure the size of `struct list_head` is two times of sizeof(void *) */
#define _COMPILE_TIME_ASSERT(name, x)               \
       typedef int _dummy_ ## name[(x) * 2 - 1]
_COMPILE_TIME_ASSERT(list_head,
        sizeof(struct list_head) == (sizeof(void *) * 2));
#undef _COMPILE_TIME_ASSERT

static struct purc_mutex       mb_mutex;
static struct sorted_array    *mb_atom2buff_map;

void pcinst_move_buffer_init_once(void)
{
    purc_mutex_init(&mb_mutex);
    if (mb_mutex.native_impl == NULL)
        PC_ASSERT(0);

    mb_atom2buff_map = pcutils_sorted_array_create(SAFLAG_DEFAULT, 0,
            NULL, NULL);
    if (mb_atom2buff_map == NULL) {
        purc_mutex_clear(&mb_mutex);
        PC_ASSERT(0);
    }
}

void pcinst_move_buffer_term_once(void)
{
    if (mb_mutex.native_impl) {
        purc_mutex_clear(&mb_mutex);
    }

    if (mb_atom2buff_map) {
        pcutils_sorted_array_destroy(mb_atom2buff_map);
    }
}

int
purc_inst_create_move_buffer(unsigned int flags, size_t max_msgs)
{
    struct pcinst* inst = pcinst_current();
    if (inst == NULL)
        return PURC_ERROR_NO_INSTANCE;

    purc_atom_t atom = inst->endpoint_atom;
    int errcode = 0;
    struct pcinst_move_buffer *mb = NULL;

    purc_mutex_lock(&mb_mutex);

    if (pcutils_sorted_array_find(mb_atom2buff_map,
                (void *)(uintptr_t)atom, (void **)&mb)) {
        mb = NULL;
        errcode = PURC_ERROR_DUPLICATED;
        goto done;
    }

    if ((mb = malloc(sizeof(*mb))) == NULL) {
        errcode = PURC_ERROR_OUT_OF_MEMORY;
        goto done;
    }

    purc_mutex_init(&mb->lock);
    if (mb->lock.native_impl == NULL) {
        errcode = PURC_ERROR_BAD_SYSTEM_CALL;
        goto done;
    }

    if (pcutils_sorted_array_add(mb_atom2buff_map,
                (void *)(uintptr_t)atom, &mb) < 0) {
        errcode = PURC_ERROR_OUT_OF_MEMORY;
        goto done;
    }

    mb->flags = flags;
    mb->max_nr_msgs = (max_msgs > 0) ? max_msgs : NR_DEF_MAX_MSGS;
    list_head_init(&mb->msgs);

done:
    purc_mutex_unlock(&mb_mutex);

    if (errcode) {
        if (mb) {
            if (mb->lock.native_impl) {
                purc_mutex_clear(&mb->lock);
            }

            free(mb);
        }

        purc_set_error(errcode);
        return false;
    }

    return errcode;
}

ssize_t
purc_inst_destroy_move_buffer(void)
{
    ssize_t nr = 0;
    struct pcinst* inst = pcinst_current();
    if (inst == NULL)
        return -1;

    purc_atom_t atom = inst->endpoint_atom;
    int errcode = 0;
    struct pcinst_move_buffer *mb = NULL;

    purc_mutex_lock(&mb_mutex);

    if (pcutils_sorted_array_find(mb_atom2buff_map,
                (void *)(uintptr_t)atom, (void **)&mb)) {
        mb = NULL;
        errcode = PURC_ERROR_NOT_EXISTS;
        goto done;
    }

    struct list_head *p, *n;
    purc_mutex_lock(&mb->lock);
    list_for_each_safe(p, n, &mb->msgs) {

        struct pcrdr_msg_hdr *hdr;

        hdr = list_entry(p, struct pcrdr_msg_hdr, ln);

        list_del(p);

        free(hdr);  // TODO: use slice allocator.
        nr++;
    }
    purc_mutex_unlock(&mb->lock);

    purc_mutex_clear(&mb->lock);
    free(mb);

done:
    purc_mutex_unlock(&mb_mutex);

    if (errcode) {
        purc_set_error(errcode);
        return -1;
    }

    return nr;
}

size_t
purc_inst_move_msg(purc_atom_t inst_to, pcrdr_msg *msg)
{
    UNUSED_PARAM(inst_to);
    UNUSED_PARAM(msg);
    return 0;
}


size_t
purc_inst_nr_moving_msgs(void)
{
    return 0;
}

const pcrdr_msg *
purc_inst_retrieve_msg(size_t index)
{
    UNUSED_PARAM(index);
    return NULL;
}

pcrdr_msg *
purc_inst_take_away_msg(size_t index)
{
    UNUSED_PARAM(index);
    return NULL;
}
