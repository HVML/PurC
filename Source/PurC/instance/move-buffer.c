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

/* this feature needs C11 (stdatomic.h) or above */
#if HAVE(STDATOMIC_H)

#include "purc-pcrdr.h"
#include "purc-errors.h"
#include "private/instance.h"
#include "private/list.h"
#include "private/sorted-array.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/ports.h"

#include <stdatomic.h>
#include <assert.h>

#if HAVE(GLIB)
    #include <gmodule.h>
#endif

#define NR_DEF_MAX_MSGS     4

struct pcinst_move_buffer {
    struct purc_rwlock  lock;
    struct list_head    msgs;

    unsigned int        flags;
    size_t              max_nr_msgs;
    size_t              nr_msgs;
};

/* the header of the struct pcrdr_msg */
struct pcrdr_msg_hdr {
    atomic_uint             refc;
    struct list_head        ln;
};

/* Make sure the size of `struct list_head` is two times of sizeof(void *) */
#define _COMPILE_TIME_ASSERT(name, x)               \
       typedef int _dummy_ ## name[(x) * 2 - 1]
_COMPILE_TIME_ASSERT(list_head,
        sizeof(struct list_head) == (sizeof(void *) * 2));
#undef _COMPILE_TIME_ASSERT

static struct purc_rwlock      mb_lock;
static struct sorted_array    *mb_atom2buff_map;

void pcinst_move_buffer_init_once(void)
{
    purc_rwlock_init(&mb_lock);
    if (mb_lock.native_impl == NULL)
        PC_ASSERT(0);

    mb_atom2buff_map = pcutils_sorted_array_create(SAFLAG_DEFAULT, 0,
            NULL, NULL);
    if (mb_atom2buff_map == NULL) {
        purc_rwlock_clear(&mb_lock);
        PC_ASSERT(0);
    }
}

void pcinst_move_buffer_term_once(void)
{
    if (mb_lock.native_impl) {
        purc_rwlock_clear(&mb_lock);
    }

    if (mb_atom2buff_map) {
        pcutils_sorted_array_destroy(mb_atom2buff_map);
    }
}

pcrdr_msg *
pcinst_get_message(void)
{
    pcrdr_msg *msg;

#if HAVE(GLIB)
    msg = (pcrdr_msg *)g_slice_alloc0(sizeof(pcrdr_msg));
#else
    msg = (pcrdr_msg *)calloc(1, sizeof(pcrdr_msg));
#endif

    if (msg) {
        struct pcrdr_msg_hdr *hdr = (struct pcrdr_msg_hdr *)msg;
        atomic_init(&hdr->refc, 1);
    }

    return msg;
}

void
pcinst_put_message(pcrdr_msg *msg)
{
    struct pcrdr_msg_hdr *hdr = (struct pcrdr_msg_hdr *)msg;
    unsigned int old_refc = atomic_fetch_sub(&hdr->refc, 1);

    PC_INFO("The old refc of message: %d\n", old_refc);
    if (old_refc <= 1) {
        PC_INFO("Freeing message: %p\n", msg);

        if (msg->operation)
            purc_variant_unref(msg->operation);

        if (msg->element)
            purc_variant_unref(msg->element);

        if (msg->property)
            purc_variant_unref(msg->property);

        if (msg->event)
            purc_variant_unref(msg->event);

        if (msg->requestId)
            purc_variant_unref(msg->requestId);

        if (msg->data)
            purc_variant_unref(msg->data);

#if HAVE(GLIB)
        g_slice_free1(sizeof(pcrdr_msg), (gpointer)msg);
#else
        free(msg);
#endif
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

    purc_rwlock_writer_lock(&mb_lock);

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

    purc_rwlock_init(&mb->lock);
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
    purc_rwlock_writer_unlock(&mb_lock);

    if (errcode) {
        if (mb) {
            if (mb->lock.native_impl) {
                purc_rwlock_clear(&mb->lock);
            }

            free(mb);
        }

        purc_set_error(errcode);
        return false;
    }

    return errcode;
}

static void
pcinst_grind_message(pcrdr_msg *msg)
{
    if (msg->operation)
        purc_variant_unref(msg->operation);

    if (msg->element)
        purc_variant_unref(msg->element);

    if (msg->property)
        purc_variant_unref(msg->property);

    if (msg->event)
        purc_variant_unref(msg->event);

    if (msg->requestId)
        purc_variant_unref(msg->requestId);

    if (msg->data)
        purc_variant_unref(msg->data);

#if HAVE(GLIB)
    g_slice_free1(sizeof(pcrdr_msg), (gpointer)msg);
#else
    free(msg);
#endif
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

    purc_rwlock_writer_lock(&mb_lock);

    if (pcutils_sorted_array_find(mb_atom2buff_map,
                (void *)(uintptr_t)atom, (void **)&mb)) {
        mb = NULL;
        errcode = PURC_ERROR_NOT_EXISTS;
        goto done;
    }

    struct list_head *p, *n;
    purc_rwlock_writer_lock(&mb->lock);
    pcvariant_use_move_heap();
    list_for_each_safe(p, n, &mb->msgs) {

        struct pcrdr_msg_hdr *hdr;

        hdr = list_entry(p, struct pcrdr_msg_hdr, ln);

        list_del(p);

        pcinst_grind_message((pcrdr_msg *)hdr);
        nr++;
    }
    pcvariant_use_norm_heap();
    purc_rwlock_writer_unlock(&mb->lock);

    purc_rwlock_clear(&mb->lock);
    free(mb);

done:
    purc_rwlock_writer_unlock(&mb_lock);

    if (errcode) {
        purc_set_error(errcode);
        return -1;
    }

    return nr;
}

static void
do_move_message(pcrdr_msg *msg)
{
    struct pcrdr_msg_hdr *hdr = (struct pcrdr_msg_hdr *)msg;
    atomic_fetch_add(&hdr->refc, 1);

    if (msg->operation)
        msg->operation = pcvariant_move_from(msg->operation);
    if (msg->element)
        msg->element = pcvariant_move_from(msg->element);
    if (msg->property)
        msg->property = pcvariant_move_from(msg->property);
    if (msg->event)
        msg->event = pcvariant_move_from(msg->event);
    if (msg->requestId)
        msg->requestId = pcvariant_move_from(msg->requestId);
    if (msg->data)
        msg->data = pcvariant_move_from(msg->data);
}

static void
do_take_message(pcrdr_msg *msg)
{
    struct pcrdr_msg_hdr *hdr = (struct pcrdr_msg_hdr *)msg;
    atomic_fetch_add(&hdr->refc, 1);

    if (msg->operation)
        msg->operation = pcvariant_move_to(msg->operation);
    if (msg->element)
        msg->element = pcvariant_move_to(msg->element);
    if (msg->property)
        msg->property = pcvariant_move_to(msg->property);
    if (msg->event)
        msg->event = pcvariant_move_to(msg->event);
    if (msg->requestId)
        msg->requestId = pcvariant_move_to(msg->requestId);
    if (msg->data)
        msg->data = pcvariant_move_to(msg->data);
}

size_t
purc_inst_move_message(purc_atom_t inst_to, pcrdr_msg *msg)
{
    int errcode = 0;
    size_t nr = 0;
    struct pcinst_move_buffer *mb;

    purc_rwlock_reader_lock(&mb_lock);

    if (inst_to) {
        if (!pcutils_sorted_array_find(mb_atom2buff_map,
                    (void *)(uintptr_t)inst_to, (void **)&mb)) {
            errcode = PURC_ERROR_NOT_EXISTS;
            goto done;
        }

        if (mb->nr_msgs >= mb->max_nr_msgs) {
            errcode = PURC_ERROR_TOO_SMALL_BUFF;
            goto done;
        }

        do_move_message(msg);

        purc_rwlock_writer_lock(&mb->lock);
        struct pcrdr_msg_hdr *hdr = (struct pcrdr_msg_hdr *)msg;
        list_add_tail(&hdr->ln, &mb->msgs);
        purc_rwlock_writer_unlock(&mb->lock);

        nr++;
    }
    else {
        size_t count = pcutils_sorted_array_count(mb_atom2buff_map);

        for (size_t i = 0; i < count; i++) {
            pcutils_sorted_array_get(mb_atom2buff_map, i, (void **)&mb);
            if (mb->flags & PCINST_MOVE_BUFFER_BROADCAST &&
                    mb->nr_msgs < mb->max_nr_msgs) {
                do_move_message(msg);
                purc_rwlock_writer_lock(&mb->lock);
                struct pcrdr_msg_hdr *hdr = (struct pcrdr_msg_hdr *)msg;
                list_add_tail(&hdr->ln, &mb->msgs);
                purc_rwlock_writer_unlock(&mb->lock);
                nr++;
            }
        }
    }

done:
    purc_rwlock_reader_unlock(&mb_lock);

    if (errcode) {
        purc_set_error(errcode);
    }

    return nr;
}

int
purc_inst_moving_messages_count(size_t *nr)
{
    struct pcinst* inst = pcinst_current();
    if (inst == NULL)
        return PURC_ERROR_NO_INSTANCE;

    int errcode = 0;
    struct pcinst_move_buffer *mb;

    purc_rwlock_reader_lock(&mb_lock);

    if (!pcutils_sorted_array_find(mb_atom2buff_map,
                (void *)(uintptr_t)inst->endpoint_atom, (void **)&mb)) {
        errcode = PURC_ERROR_NOT_EXISTS;
        goto done;
    }

    /* no need to lock the buffer */
    *nr = mb->nr_msgs;

done:
    purc_rwlock_reader_unlock(&mb_lock);

    if (errcode) {
        purc_set_error(errcode);
    }

    return errcode;
}

const pcrdr_msg *
purc_inst_retrieve_messages(size_t index)
{
    struct pcinst* inst = pcinst_current();
    if (inst == NULL)
        return NULL;

    int errcode = 0;
    const pcrdr_msg *msg = NULL;
    struct pcinst_move_buffer *mb;

    purc_rwlock_reader_lock(&mb_lock);

    if (!pcutils_sorted_array_find(mb_atom2buff_map,
                (void *)(uintptr_t)inst->endpoint_atom, (void **)&mb)) {
        errcode = PURC_ERROR_NOT_EXISTS;
        goto done;
    }

    purc_rwlock_reader_lock(&mb->lock);
    if (index < mb->nr_msgs) {
        struct list_head *p;
        struct pcrdr_msg_hdr *hdr;
        size_t i = 0;

        list_for_each(p, &mb->msgs) {
            hdr = list_entry(p, struct pcrdr_msg_hdr, ln);
            if (i == index) {
                msg = (pcrdr_msg *)hdr;
                break;
            }

            i++;
        }
    }
    purc_rwlock_reader_unlock(&mb->lock);

done:
    purc_rwlock_reader_unlock(&mb_lock);

    if (errcode) {
        purc_set_error(errcode);
    }

    return msg;
}

pcrdr_msg *
purc_inst_take_away_message(size_t index)
{
    struct pcinst* inst = pcinst_current();
    if (inst == NULL)
        return NULL;

    int errcode = 0;
    pcrdr_msg *msg = NULL;
    struct pcinst_move_buffer *mb;

    purc_rwlock_reader_lock(&mb_lock);

    if (!pcutils_sorted_array_find(mb_atom2buff_map,
                (void *)(uintptr_t)inst->endpoint_atom, (void **)&mb)) {
        errcode = PURC_ERROR_NOT_EXISTS;
        goto done;
    }

    purc_rwlock_writer_lock(&mb->lock);
    if (index < mb->nr_msgs) {
        struct list_head *p, *n;
        struct pcrdr_msg_hdr *hdr;
        size_t i = 0;

        list_for_each_safe(p, n, &mb->msgs) {
            hdr = list_entry(p, struct pcrdr_msg_hdr, ln);
            if (i == index) {
                msg = (pcrdr_msg *)hdr;
                list_del(p);
                break;
            }

            i++;
        }
    }
    else {
        errcode = PURC_ERROR_NOT_EXISTS;
    }
    purc_rwlock_writer_unlock(&mb->lock);

    if (msg)
        do_take_message(msg);

done:
    purc_rwlock_reader_unlock(&mb_lock);

    if (errcode) {
        purc_set_error(errcode);
    }

    return msg;
}

#else   /* HAVE(STDATOMIC_H) */

#if HAVE(GLIB)
    #include <gmodule.h>
#endif

void pcinst_move_buffer_init_once(void)
{
    // do nothing.
}

void pcinst_move_buffer_term_once(void)
{
    // do nothing.
}

int
purc_inst_create_move_buffer(unsigned int flags, size_t max_msgs)
{
    UNUSED_PARAM(flags);
    UNUSED_PARAM(max_msgs);

    return PURC_ERROR_NOT_SUPPORTED;
}

ssize_t
purc_inst_destroy_move_buffer(void)
{
    return 0;
}

pcrdr_msg *
pcinst_get_message(void)
{
#if HAVE(GLIB)
    return g_slice_alloc0(sizeof(pcrdr_msg));
#else
    return calloc(1, sizeof(pcrdr_msg));
#endif
}

void
pcinst_put_message(pcrdr_msg *msg)
{
#if HAVE(GLIB)
    g_slice_free1(sizeof(pcrdr_msg), (gpointer)msg);
#else
    free(msg);
#endif
}

#endif  /* !HAVE(STDATOMIC_H) */

