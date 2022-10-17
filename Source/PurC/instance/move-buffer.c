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

/* this feature needs C11 (stdatomic.h) or above */
#if HAVE(STDATOMIC_H)

#include "private/instance.h"
#include "private/list.h"
#include "private/sorted-array.h"
#include "private/utils.h"
#include "private/ports.h"
#include "private/debug.h"

#include <stdatomic.h>
#include <assert.h>

#if HAVE(GLIB)
    #include <gmodule.h>
#endif

#define NR_DEF_MAX_MSGS     4

// #define PRINT_DEBUG

struct pcinst_move_buffer {
    struct purc_rwlock  lock;
    struct list_head    msgs;

    unsigned int        flags;
    size_t              max_nr_msgs;
    size_t              nr_msgs;
};

/* the header of the struct pcrdr_msg */
struct pcrdr_msg_hdr {
    atomic_uint             owner;
    purc_atom_t             origin;
    struct list_head        ln;
};

/* Make sure the size of `struct list_head` is two times of sizeof(void *) */
#define _COMPILE_TIME_ASSERT(name, x)               \
       typedef int _dummy_ ## name[(x) * 2 - 1]
_COMPILE_TIME_ASSERT(onwer_atom,
        sizeof(atomic_uint) == sizeof(purc_atom_t));
_COMPILE_TIME_ASSERT(list_head,
        sizeof(struct list_head) == (sizeof(void *) * 2));
#undef _COMPILE_TIME_ASSERT

static struct purc_rwlock      mb_lock;
static struct sorted_array    *mb_atom2buff_map;

static void mvbuf_cleanup_once(void)
{
    if (mb_lock.native_impl) {
        purc_rwlock_clear(&mb_lock);
        mb_lock.native_impl = NULL;
    }

    if (mb_atom2buff_map) {
        pcutils_sorted_array_destroy(mb_atom2buff_map);
        mb_atom2buff_map = NULL;
    }
}

static int mvbuf_init_once(void)
{
    int r = 0;
    purc_rwlock_init(&mb_lock);
    if (mb_lock.native_impl == NULL)
        goto fail_lock;

    mb_atom2buff_map = pcutils_sorted_array_create(SAFLAG_DEFAULT, 0,
            NULL, NULL);
    if (mb_atom2buff_map == NULL)
        goto fail_map;

    r = atexit(mvbuf_cleanup_once);
    if (r)
        goto fail_atexit;

    return 0;

fail_atexit:
    pcutils_sorted_array_destroy(mb_atom2buff_map);

fail_map:
    purc_rwlock_clear(&mb_lock);

fail_lock:
    return -1;
}

pcrdr_msg *
pcinst_get_message(void)
{
    struct pcinst* inst = pcinst_current();
    pcrdr_msg *msg;

    if (inst == NULL) {
        purc_set_error(PURC_ERROR_NO_INSTANCE);
        return NULL;
    }

#if HAVE(GLIB)
    msg = (pcrdr_msg *)g_slice_alloc0(sizeof(pcrdr_msg));
#else
    msg = (pcrdr_msg *)calloc(1, sizeof(pcrdr_msg));
#endif

    if (msg) {
        struct pcrdr_msg_hdr *hdr = (struct pcrdr_msg_hdr *)msg;
        atomic_init(&hdr->owner, inst->endpoint_atom);
#ifdef PRINT_DEBUG            /* { */
        PC_DEBUG("New message in %s: %p\n", __func__, msg);
#endif                        /* }*/
    }
    else {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
    }

    return msg;
}

void
pcinst_put_message(pcrdr_msg *msg)
{
    struct pcinst* inst = pcinst_current();
    struct pcrdr_msg_hdr *hdr = (struct pcrdr_msg_hdr *)msg;
    purc_atom_t owner = (purc_atom_t)atomic_load(&hdr->owner);

#ifdef PRINT_DEBUG            /* { */
    PC_DEBUG("The current owner atom of message in %s: %x\n", __func__, owner);
#endif                        /* }*/
    if (owner == inst->endpoint_atom) {
#ifdef PRINT_DEBUG            /* { */
        PC_DEBUG("Freeing message in %s: %p\n", __func__, msg);
#endif                        /* }*/

        for (int i = 0; i < PCRDR_NR_MSG_VARIANTS; i++) {
            if (msg->variants[i])
                purc_variant_unref(msg->variants[i]);
        }

#if HAVE(GLIB)
        g_slice_free1(sizeof(pcrdr_msg), (gpointer)msg);
#else
        free(msg);
#endif
    }
}

purc_atom_t
purc_inst_create_move_buffer(unsigned int flags, size_t max_msgs)
{
    struct pcinst* inst = pcinst_current();
    if (inst == NULL) {
        purc_set_error(PURC_ERROR_NO_INSTANCE);
        return 0;
    }

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
                (void *)(uintptr_t)atom, mb) < 0) {
        errcode = PURC_ERROR_OUT_OF_MEMORY;
        goto done;
    }

    mb->flags = flags;
    mb->nr_msgs = 0;
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
        return 0;
    }

    return atom;
}

static void
pcinst_grind_message(pcrdr_msg *msg)
{
    struct pcrdr_msg_hdr *hdr = (struct pcrdr_msg_hdr *)msg;
    purc_atom_t owner = atomic_load(&hdr->owner);
#ifdef PRINT_DEBUG            /* { */
    PC_DEBUG("message owner in %s: %x\n", __func__, owner);
#endif                        /* }*/

    if (owner == 0) {
#ifdef PRINT_DEBUG            /* { */
        PC_DEBUG("Freeing message in %s: %p\n", __func__, msg);
#endif                        /* }*/

        for (int i = 0; i < PCRDR_NR_MSG_VARIANTS; i++) {
            if (msg->variants[i])
                purc_variant_unref(msg->variants[i]);
        }

#if HAVE(GLIB)
        g_slice_free1(sizeof(pcrdr_msg), (gpointer)msg);
#else
        free(msg);
#endif
    }
    else {
        PC_ERROR("Freeing a message not owned by the move buffer: %p\n", msg);
    }
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

    if (!pcutils_sorted_array_find(mb_atom2buff_map,
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
        mb->nr_msgs--;

        pcinst_grind_message((pcrdr_msg *)hdr);
        nr++;
    }
    pcvariant_use_norm_heap();
    purc_rwlock_writer_unlock(&mb->lock);

    pcutils_sorted_array_remove(mb_atom2buff_map, (void *)(uintptr_t)atom);
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
do_move_message(struct pcinst* inst, pcrdr_msg *msg)
{
    struct pcrdr_msg_hdr *hdr = (struct pcrdr_msg_hdr *)msg;

    if (atomic_compare_exchange_strong(&hdr->owner, &inst->endpoint_atom, 0)) {
        hdr->origin = inst->endpoint_atom;

        for (int i = 0; i < PCRDR_NR_MSG_VARIANTS; i++) {
            if (msg->variants[i])
                msg->variants[i] = pcvariant_move_heap_in(msg->variants[i]);
        }
    }
    else {
        PC_ERROR("Moving a message not owned by the current inst: %p\n", msg);
    }
}

static void
do_take_message(struct pcinst* inst, pcrdr_msg *msg)
{
    unsigned int mb_owner = 0;
    struct pcrdr_msg_hdr *hdr = (struct pcrdr_msg_hdr *)msg;

    if (atomic_compare_exchange_strong(&hdr->owner, &mb_owner,
                inst->endpoint_atom)) {
        for (int i = 0; i < PCRDR_NR_MSG_VARIANTS; i++) {
            if (msg->variants[i])
                msg->variants[i] = pcvariant_move_heap_out(msg->variants[i]);
        }
    }
    else {
        PC_ERROR("Taking a message not owned by the move buffer: %p\n", msg);
    }
}

size_t
purc_inst_move_message(purc_atom_t inst_to, pcrdr_msg *msg)
{
    int errcode = 0;
    size_t nr = 0;
    struct pcinst_move_buffer *mb;
    struct pcinst* inst = pcinst_current();

    if (inst == NULL) {
        purc_set_error(PURC_ERROR_NO_INSTANCE);
        return 0;
    }

    if (inst_to == (purc_atom_t)PURC_EVENT_TARGET_SELF) {
        return 0;
    }

    purc_rwlock_reader_lock(&mb_lock);

    if (inst_to != (purc_atom_t)PURC_EVENT_TARGET_BROADCAST) {
        if (!pcutils_sorted_array_find(mb_atom2buff_map,
                    (void *)(uintptr_t)inst_to, (void **)&mb)) {
            errcode = PURC_ERROR_NOT_EXISTS;
            goto done;
        }

        if (mb->nr_msgs >= mb->max_nr_msgs) {
            errcode = PURC_ERROR_TOO_SMALL_BUFF;
            goto done;
        }

        do_move_message(inst, msg);

        purc_rwlock_writer_lock(&mb->lock);
        struct pcrdr_msg_hdr *hdr = (struct pcrdr_msg_hdr *)msg;
        list_add_tail(&hdr->ln, &mb->msgs);
        mb->nr_msgs++;
        purc_rwlock_writer_unlock(&mb->lock);

        nr++;
    }
    else {
        size_t count = pcutils_sorted_array_count(mb_atom2buff_map);

        for (size_t i = 0; i < count; i++) {
            pcutils_sorted_array_get(mb_atom2buff_map, i, (void **)&mb);
            if (mb->flags & PCINST_MOVE_BUFFER_BROADCAST &&
                    mb->nr_msgs < mb->max_nr_msgs) {

                pcrdr_msg *my_msg;

                if (i == count - 1) {
                    my_msg = msg;
                    do_move_message(inst, msg);
                    // FIXME: if count > 1 and flags without PCINST_MOVE_BUFFER_BROADCAST
                    // not reatch here
                    msg = NULL;
                }
                else {
                    my_msg = pcrdr_clone_message(msg);
                    if (my_msg) {
                        do_move_message(inst, my_msg);
                        pcrdr_release_message(my_msg);
                    }
                    else {
                        PC_ERROR("failed to clone message to broadcast: %p\n",
                                msg);
                        break;
                    }
                }

                purc_rwlock_writer_lock(&mb->lock);
                struct pcrdr_msg_hdr *hdr = (struct pcrdr_msg_hdr *)my_msg;
                list_add_tail(&hdr->ln, &mb->msgs);
                mb->nr_msgs++;
                purc_rwlock_writer_unlock(&mb->lock);
                nr++;
            }
        }

        // FIXME:
        if (msg) {
            pcrdr_release_message(msg);
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
purc_inst_holding_messages_count(size_t *nr)
{
    struct pcinst* inst = pcinst_current();
    if (inst == NULL) {
        purc_set_error(PURC_ERROR_NO_INSTANCE);
        return PURC_ERROR_NO_INSTANCE;
    }

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
purc_inst_retrieve_message(size_t index)
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
    if (inst == NULL) {
        purc_set_error(PURC_ERROR_NO_INSTANCE);
        return NULL;
    }

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
                hdr->ln.next = hdr->ln.prev = NULL; /* mark as not linked */
                mb->nr_msgs--;
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
        do_take_message(inst, msg);

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

static int mvbuf_init_once(void)
{
    // do nothing.
    return 0;
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

size_t
purc_inst_move_message(purc_atom_t inst_to, pcrdr_msg *msg)
{
    UNUSED_PARAM(inst_to);
    UNUSED_PARAM(msg);
    return 0;
}

int
purc_inst_holding_messages_count(size_t *nr)
{
    UNUSED_PARAM(nr);
    purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    return PURC_ERROR_NOT_SUPPORTED;
}

const pcrdr_msg *
purc_inst_retrieve_message(size_t index)
{
    UNUSED_PARAM(index);
    purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    return NULL;
}

pcrdr_msg *
purc_inst_take_away_message(size_t index)
{
    UNUSED_PARAM(index);
    purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    return NULL;
}

#endif  /* !HAVE(STDATOMIC_H) */

struct pcmodule _module_mvbuf = {
    .id              = PURC_HAVE_VARIANT,
    .module_inited   = 0,

    .init_once       = mvbuf_init_once,
    .init_instance   = NULL,
};

