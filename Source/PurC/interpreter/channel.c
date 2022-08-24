/*
 * @file channel.c
 * @author Vincent Wei
 * @date 2022/08/23
 * @brief The implementation of channel.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
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

//#undef NDEBUG

#include "config.h"

#include "purc-variant.h"
#include "purc-helpers.h"
#include "private/channel.h"
#include "private/instance.h"
#include "private/interpreter.h"

#include <assert.h>
#include <errno.h>

void pcchan_destroy(pcchan_t chan)
{
    while (chan->sendx != chan->recvx) {
        purc_variant_t vrt = chan->data[chan->recvx];

        assert(vrt);
        purc_variant_unref(vrt);

        chan->data[chan->recvx] = PURC_VARIANT_INVALID;
        chan->recvx = (chan->recvx + 1) % chan->qsize;

        chan->qcount--;
    }

    /* TODO: wake up waiting coroutines with the exception EntityGone
    list_for_each_entry_safe(&chan->send_crtns, ...)
    list_for_each_entry_safe(&chan->recv_crtns, ...)
    */

    free(chan);
}

pcchan_t
pcchan_open(const char *chan_name, unsigned int cap)
{
    struct pcinst* inst = pcinst_current();
    if ((inst = pcinst_current()) == NULL || inst->intr_heap == NULL) {
        inst->errcode = PURC_ERROR_NO_INSTANCE;
        return NULL;
    }

    if (chan_name == NULL || cap == 0 ||
            !purc_is_valid_token(chan_name, PCCHAN_MAX_LEN_NAME)) {
        inst->errcode = PURC_ERROR_INVALID_VALUE;
        return NULL;
    }

    pcintr_heap_t heap = inst->intr_heap;
    const pcutils_map_entry* entry = NULL;
    if ((entry = pcutils_map_find(heap->name_chan_map, chan_name))) {
        inst->errcode = PURC_ERROR_DUPLICATE_NAME;
        return NULL;
    }

    pcchan_t chan = calloc(1, sizeof(*chan) + sizeof(purc_variant_t) * cap);
    if (chan == NULL) {
        inst->errcode = PURC_ERROR_OUT_OF_MEMORY;
        return NULL;
    }

    if (pcutils_map_insert(heap->name_chan_map, chan_name, chan)) {
        inst->errcode = PURC_ERROR_OUT_OF_MEMORY;
        return NULL;
    }

    chan->qsize = cap;
    chan->qcount = 0;
    chan->sendx = 0;
    chan->recvx = 0;
    list_head_init(&chan->send_crtns);
    list_head_init(&chan->recv_crtns);

    return chan;
}

bool
pcchan_ctrl(const char *chan_name, unsigned int new_cap)
{
    struct pcinst* inst = pcinst_current();
    if ((inst = pcinst_current()) == NULL || inst->intr_heap == NULL) {
        inst->errcode = PURC_ERROR_NO_INSTANCE;
        goto failed;
    }

    if (chan_name == NULL ||
            !purc_is_valid_token(chan_name, PCCHAN_MAX_LEN_NAME)) {
        inst->errcode = PURC_ERROR_INVALID_VALUE;
        goto failed;
    }

    pcintr_heap_t heap = inst->intr_heap;
    pcutils_map_entry* entry = NULL;
    if (!(entry = pcutils_map_find(heap->name_chan_map, chan_name))) {
        inst->errcode = PURC_ERROR_NOT_EXISTS;
        goto failed;
    }

    pcchan_t chan = entry->val;
    if (new_cap == 0) {
        if (pcutils_map_erase(heap->name_chan_map, (void *)chan_name) == 0)
            goto done;
        else
            assert(0);
    }

    if (new_cap > chan->qcount) {
        // FIXME: re-arrange data
        unsigned int i = 0;
        while (chan->sendx != chan->recvx) {
            purc_variant_t tmp = chan->data[i];
            chan->data[i] = chan->data[chan->recvx];
            chan->data[chan->recvx] = tmp;
            chan->recvx = (chan->recvx + 1) % chan->qsize;
            i++;
        }

        chan = realloc(chan, sizeof(*chan) + sizeof(purc_variant_t) * new_cap);
        chan->recvx = 0;
        chan->sendx = chan->qcount - 1;
        chan->qsize = new_cap;

        for (i = chan->qcount; i < chan->qsize; i++) {
            chan->data[i] = PURC_VARIANT_INVALID;
        }


        entry->val = chan;
    }

done:
    return true;

failed:
    return false;
}

pcchan_t
pcchan_retrieve(const char *chan_name)
{
    struct pcinst* inst;
    const pcutils_map_entry* entry = NULL;

    if ((inst = pcinst_current()) == NULL || inst->intr_heap == NULL) {
        inst->errcode = PURC_ERROR_NO_INSTANCE;
        return NULL;
    }

    if (chan_name == NULL ||
            !purc_is_valid_token(chan_name, PCCHAN_MAX_LEN_NAME)) {
        inst->errcode = PURC_ERROR_INVALID_VALUE;
        return NULL;
    }

    pcintr_heap_t heap = inst->intr_heap;
    if ((entry = pcutils_map_find(heap->name_chan_map, chan_name))) {
        return entry->val;
    }

    return NULL;
}

static purc_variant_t
send_getter(void *native_entity, size_t nr_args, purc_variant_t *argv,
                bool silently)
{
    pcchan_t chan = native_entity;

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if ((chan->sendx + 1) % chan->qsize != chan->recvx) {
        chan->data[chan->sendx] = purc_variant_ref(argv[0]);
        chan->sendx = (chan->sendx + 1) % chan->qsize;
        chan->qcount++;

        // TODO: if there is any coroutine waiting for data, awake one here
    }
    else {
        // TODO: block the current coroutine
        purc_set_error(PURC_ERROR_AGAIN);
        return PURC_VARIANT_INVALID;
    }

failed:
    if (silently)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
recv_getter(void *native_entity, size_t nr_args, purc_variant_t *argv,
                bool silently)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(silently);

    pcchan_t chan = native_entity;

    purc_variant_t vrt;
    if (chan->recvx != chan->sendx) {
        vrt = chan->data[chan->recvx];
        purc_variant_unref(vrt);

        chan->recvx = (chan->recvx + 1) % chan->qsize;
        chan->qcount--;

        // TODO: if there is any coroutine waiting for sending data,
        // awake one here
    }
    else {
        // TODO: block the current coroutine
        purc_set_error(PURC_ERROR_AGAIN);
        return PURC_VARIANT_INVALID;
    }

    return purc_variant_ref(vrt);

failed:
    if (silently)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
cap_getter(void *native_entity, size_t nr_args, purc_variant_t *argv,
                bool silently)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(silently);

    pcchan_t chan = native_entity;
    return purc_variant_make_ulongint(chan->qsize);
}

static purc_variant_t
len_getter(void *native_entity, size_t nr_args, purc_variant_t *argv,
                bool silently)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(silently);

    pcchan_t chan = native_entity;
    return purc_variant_make_ulongint(chan->qcount);
}

static purc_nvariant_method
property_getter(const char *name)
{
    if (strcmp(name, "send") == 0) {
        return send_getter;
    }
    else if (strcmp(name, "recv") == 0) {
        return recv_getter;
    }
    else if (strcmp(name, "cap") == 0) {
        return cap_getter;
    }
    else if (strcmp(name, "len") == 0) {
        return len_getter;
    }

    return NULL;
}

purc_variant_t
pcchan_make_entity(pcchan_t chan)
{
    // setup a callback for `on_release` to destroy the stream automatically
    static const struct purc_native_ops ops = {
        .property_getter = property_getter,
        .on_observe = NULL,
        .on_forget = NULL,
        .on_release = NULL,
    };

    return purc_variant_make_native(chan, &ops);
}
