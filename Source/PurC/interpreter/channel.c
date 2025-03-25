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

#define MSG_TYPE_SENDABLE       "sendable"
#define MSG_TYPE_RECEIVABLE     "receivable"
#define MSG_TYPE_CLOSED         "closed"

#define KEY_FLAG                "__chan_observe"
#define KEY_NAME                "name"

static purc_variant_t
build_event_observed(const char *name)
{
    purc_variant_t v = purc_variant_make_object(0, PURC_VARIANT_INVALID,
            PURC_VARIANT_INVALID);
    if (!v) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t flag = purc_variant_make_boolean(true);
    if (!v) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    if (!purc_variant_object_set_by_static_ckey(v, KEY_FLAG, flag)) {
        purc_variant_unref(flag);
        goto failed;
    }
    purc_variant_unref(flag);

    purc_variant_t name_val = purc_variant_make_string(name, true);
    if (!name_val) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }
    if (!purc_variant_object_set_by_static_ckey(v, KEY_NAME, name_val)) {
        goto failed;
    }
    purc_variant_unref(name_val);

    return v;

failed:
    purc_variant_unref(v);

    return PURC_VARIANT_INVALID;
}

static int
post_event(pcchan_t chan, const char *type, const char *sub_type,
        purc_variant_t data)
{
    int ret = -1;
    struct pcinst* inst = pcinst_current();
    purc_variant_t source_uri = purc_variant_make_string(
            inst->endpoint_name, false);
    if (!source_uri) {
        goto out;
    }

    purc_variant_t source = build_event_observed(chan->name);
    if (!source) {
        purc_variant_unref(source_uri);
        goto out;
    }

    pcintr_post_event_by_ctype(0, PURC_EVENT_TARGET_BROADCAST,
            PCRDR_MSG_EVENT_REDUCE_OPT_OVERLAY, source_uri,
            source, type, sub_type, data,
            PURC_VARIANT_INVALID);

    purc_variant_unref(source);
    purc_variant_unref(source_uri);

    ret = 0;
out:
    return ret;
}

static void remove_tmp_chan_file(pcchan_t chan)
{
    if (strncmp(chan->name, TEMP_CHAN_PREEFIX,
                sizeof(TEMP_CHAN_PREEFIX) - 1) == 0 &&
            strlen(chan->name) == sizeof(TEMP_CHAN_TEMPLATE_FILE) - 1) {
        /* this is a temporary channel */
        char temp_chan_path[] = TEMP_CHAN_TEMPLATE_PATH;
        strcpy(temp_chan_path + sizeof(TEMP_CHAN_PATH) - 1,
                chan->name);
        if (access(temp_chan_path, F_OK) == 0) {
            remove(temp_chan_path);
        }
        else {
            PC_WARN("The corresponding file for temporary channel dose not"
                    "exist: %s\n", strerror(errno));
        }
    }
}

void
pcchan_destroy(pcchan_t chan)
{
    if (chan->qsize > 0) {
        PC_WARN("destroying a channel not closed: %s (%u)\n",
                chan->name, chan->qcount);
        remove_tmp_chan_file(chan);
    }

    free(chan->data);
    free(chan->name);
    free(chan);
}

pcchan_t
pcchan_open(const char *chan_name, unsigned int cap)
{
    struct pcinst* inst = pcinst_current();
    if (UNLIKELY((inst = pcinst_current()) == NULL ||
                inst->intr_heap == NULL)) {
        inst->errcode = PURC_ERROR_NO_INSTANCE;
        return NULL;
    }

    if (UNLIKELY(chan_name == NULL || chan_name[0] == '\0' || cap == 0)) {
        inst->errcode = PURC_ERROR_INVALID_VALUE;
        return NULL;
    }

    pcintr_heap_t heap = inst->intr_heap;
    pcutils_map_entry* entry;
    pcchan_t chan;

    entry = pcutils_map_find(heap->name_chan_map, chan_name);
    if (entry) {
        chan = entry->val;
        if (chan->qsize > 0) {
            inst->errcode = PURC_ERROR_EXISTS;
            return NULL;
        }
        else {
            // reopen the existed channel
            chan->data = realloc(chan->data, sizeof(purc_variant_t) * cap);
            if (chan->data == NULL) {
                inst->errcode = PURC_ERROR_OUT_OF_MEMORY;
                return NULL;
            }
            entry->val = chan;
        }
    }
    else {
        chan = calloc(1, sizeof(*chan));
        if (chan == NULL) {
            inst->errcode = PURC_ERROR_OUT_OF_MEMORY;
            return NULL;
        }

        chan->data = calloc(cap, sizeof(purc_variant_t));
        if (chan->data == NULL) {
            inst->errcode = PURC_ERROR_OUT_OF_MEMORY;
            free(chan);
            return NULL;
        }

        chan->name = strdup(chan_name);
        if (pcutils_map_insert(heap->name_chan_map, chan->name, chan)) {
            inst->errcode = PURC_ERROR_OUT_OF_MEMORY;
            return NULL;
        }
    }

    chan->qsize = cap;
    chan->qcount = 0;
    chan->refc = 0;
    chan->sendx = 0;
    chan->recvx = 0;
    list_head_init(&chan->send_crtns);
    list_head_init(&chan->recv_crtns);

    return chan;
}

static unsigned int
discard_data(pcchan_t chan)
{
    unsigned int nr = 0;

    while (chan->qcount > 0) {
        purc_variant_t vrt = chan->data[chan->recvx];

        assert(vrt);
        purc_variant_unref(vrt);

        chan->data[chan->recvx] = PURC_VARIANT_INVALID;
        chan->recvx++;
        if (chan->recvx == chan->qsize)
            chan->recvx = 0;

        chan->qcount--;
        nr++;
    }

    struct list_head *p, *n;
    list_for_each_safe(p, n, &chan->send_crtns) {

        struct pcintr_coroutine *crtn;
        crtn = list_entry(p, struct pcintr_coroutine, ln_stopped);
        pcintr_resume_coroutine(crtn);

        list_del(p);
    }

    /* wake up all waiting coroutines */
    list_for_each_safe(p, n, &chan->recv_crtns) {

        struct pcintr_coroutine *crtn;
        crtn = list_entry(p, struct pcintr_coroutine, ln_stopped);
        pcintr_resume_coroutine(crtn);

        list_del(p);
    }

    return nr;
}

bool
pcchan_ctrl(pcchan_t chan, unsigned int new_cap)
{
    struct pcinst* inst = pcinst_current();
    assert(inst);
    pcintr_heap_t heap = inst->intr_heap;
    assert(heap);

#if 0 // redundant code
    pcutils_map_entry* entry;
    entry = pcutils_map_find(heap->name_chan_map, chan->name);
    assert(entry);
#endif

    if (new_cap == 0) {
        // no native entity variant bound to this channel
        if (chan->refc == 0) {
            int r = pcutils_map_erase(heap->name_chan_map, chan->name);
            PC_ASSERT(r == 0);
        }
        else {
            discard_data(chan);
            assert(chan->qcount == 0);

            chan->qsize = 0;
            chan->qcount = 0;
            chan->recvx = 0;
            chan->sendx = 0;
        }

    }
    else if (new_cap > chan->qcount) {
        purc_variant_t *newdata = malloc(sizeof(purc_variant_t) * new_cap);
        if (newdata == NULL) {
            inst->errcode = PURC_ERROR_OUT_OF_MEMORY;
            goto failed;
        }

        // copy channel fields and data
        unsigned int i = 0;
        while (chan->qcount > 0) {
            newdata[i] = chan->data[chan->recvx];
            chan->recvx++;
            if (chan->recvx == chan->qsize)
                chan->recvx = 0;
            chan->qcount--;
            i++;
        }

        chan->qsize = new_cap;
        chan->recvx = 0;
        chan->sendx = i;

        free(chan->data);
        chan->data = newdata;
    }

    return true;

failed:
    return false;
}

pcchan_t
pcchan_retrieve(const char *chan_name)
{
    struct pcinst* inst;
    const pcutils_map_entry* entry = NULL;

    if (UNLIKELY((inst = pcinst_current()) == NULL ||
                inst->intr_heap == NULL)) {
        inst->errcode = PURC_ERROR_NO_INSTANCE;
        return NULL;
    }

    if (UNLIKELY(chan_name == NULL || chan_name[0] == '\0')) {
        inst->errcode = PURC_ERROR_INVALID_VALUE;
        return NULL;
    }

    pcintr_heap_t heap = inst->intr_heap;
    if ((entry = pcutils_map_find(heap->name_chan_map, chan_name))) {
        return entry->val;
    }

    inst->errcode = PURC_ERROR_NOT_EXISTS;
    return NULL;
}

static purc_variant_t
send_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);

    pcchan_t chan = native_entity;
    pcintr_coroutine_t crtn = pcintr_get_coroutine();

    if (call_flags & PCVRT_CALL_FLAG_AGAIN &&
            call_flags & PCVRT_CALL_FLAG_TIMEOUT) {

        if (crtn) {
            struct list_head *p, *n;
            list_for_each_safe(p, n, &chan->send_crtns) {
                struct pcintr_coroutine *_crtn;
                _crtn = list_entry(p, struct pcintr_coroutine, ln_stopped);
                if (_crtn == crtn) {
                    list_del(&crtn->ln_stopped);
                    purc_set_error(PURC_ERROR_TIMEOUT);
                    goto failed;
                }
            }
        }

        purc_set_error(PURC_ERROR_INTERNAL_FAILURE);
        goto failed;
    }

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if (chan->qsize == 0) {
        purc_set_error(PURC_ERROR_ENTITY_GONE);
        goto failed;
    }

    if (purc_variant_is_undefined(argv[0])) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if (chan->qcount < chan->qsize) {
        chan->data[chan->sendx] = purc_variant_ref(argv[0]);
        chan->sendx++;
        if (chan->sendx == chan->qsize)
            chan->sendx = 0;
        chan->qcount++;

        post_event(chan, MSG_TYPE_RECEIVABLE, NULL, PURC_VARIANT_INVALID);

        // if there is any coroutine waiting to receive, resume the first one.
        if (!list_empty(&chan->recv_crtns)) {
            pcintr_coroutine_t crtn = list_first_entry(&chan->recv_crtns,
                    struct pcintr_coroutine, ln_stopped);
            pcintr_resume_coroutine(crtn);
            list_del(&crtn->ln_stopped);
        }
    }
    else {
        if (crtn) {
            // stop the current coroutine
            pcintr_stop_coroutine(crtn, &crtn->timeout);
            list_add_tail(&crtn->ln_stopped, &chan->send_crtns);
        }

        purc_set_error(PURC_ERROR_AGAIN);
        return PURC_VARIANT_INVALID;
    }

    return purc_variant_make_boolean(true);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
recv_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    pcchan_t chan = native_entity;
    pcintr_coroutine_t crtn = pcintr_get_coroutine();

    if (call_flags & PCVRT_CALL_FLAG_AGAIN &&
            call_flags & PCVRT_CALL_FLAG_TIMEOUT) {

        if (crtn) {
            struct list_head *p, *n;
            list_for_each_safe(p, n, &chan->recv_crtns) {
                struct pcintr_coroutine *_crtn;
                _crtn = list_entry(p, struct pcintr_coroutine, ln_stopped);
                if (_crtn == crtn) {
                    list_del(&crtn->ln_stopped);
                    purc_set_error(PURC_ERROR_TIMEOUT);
                    goto failed;
                }
            }
        }

        purc_set_error(PURC_ERROR_INTERNAL_FAILURE);
        goto failed;
    }

    if (chan->qsize == 0) {
        purc_set_error(PURC_ERROR_ENTITY_GONE);
        goto failed;
    }

    purc_variant_t vrt = PURC_VARIANT_INVALID;
    if (chan->qcount > 0) {
        vrt = chan->data[chan->recvx];
        chan->recvx++;
        if (chan->recvx == chan->qsize)
            chan->recvx = 0;
        chan->qcount--;

        post_event(chan, MSG_TYPE_SENDABLE, NULL, PURC_VARIANT_INVALID);

        // if there is any coroutine waiting to send, resume the first one.
        if (!list_empty(&chan->send_crtns)) {
            pcintr_coroutine_t crtn = list_first_entry(&chan->send_crtns,
                    struct pcintr_coroutine, ln_stopped);
            pcintr_resume_coroutine(crtn);
            list_del(&crtn->ln_stopped);
        }
    }
    else {
        if (crtn) {
            // stop the current coroutine
            pcintr_stop_coroutine(crtn, &crtn->timeout);
            list_add_tail(&crtn->ln_stopped, &chan->recv_crtns);
        }

        purc_set_error(PURC_ERROR_AGAIN);
        return PURC_VARIANT_INVALID;
    }

    return vrt;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
cap_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    pcchan_t chan = native_entity;
    if (chan->qsize == 0) {
        purc_set_error(PURC_ERROR_ENTITY_GONE);
        goto failed;
    }

    return purc_variant_make_ulongint(chan->qsize);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
len_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    pcchan_t chan = native_entity;
    if (chan->qsize == 0) {
        purc_set_error(PURC_ERROR_ENTITY_GONE);
        goto failed;
    }

    return purc_variant_make_ulongint(chan->qcount);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

static purc_nvariant_method
property_getter(void *entity, const char *name)
{
    UNUSED_PARAM(entity);
    if (name == NULL) {
        goto failed;
    }

    switch (name[0]) {
    case 's':
        if (strcmp(name, "send") == 0) {
            return send_getter;
        }
        break;

    case 'r':
        if (strcmp(name, "recv") == 0) {
            return recv_getter;
        }
        break;

    case 'c':
        if (strcmp(name, "cap") == 0) {
            return cap_getter;
        }
        break;

    case 'l':
        if (strcmp(name, "len") == 0) {
            return len_getter;
        }
        break;

    default:
        break;
    }

failed:
    purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    return NULL;
}

static purc_variant_t
cap_setter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    pcchan_t chan = native_entity;
    if (chan->qsize == 0) {
        purc_set_error(PURC_ERROR_ENTITY_GONE);
        goto failed;
    }

    uint32_t cap = 1;
    if (nr_args > 0) {
        if (!purc_variant_cast_to_uint32(argv[0], &cap, false)) {
            pcinst_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }
    }

    if (!pcchan_ctrl(chan, cap)) {
        // error set by pcchan_ctrl()
        goto failed;
    }

    return purc_variant_make_boolean(true);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

static purc_nvariant_method
property_setter(void *entity, const char *name)
{
    UNUSED_PARAM(entity);
    if (name == NULL) {
        goto failed;
    }

    if (strcmp(name, "cap") == 0) {
        return cap_setter;
    }

failed:
    purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    return NULL;
}

static void
on_release(void *native_entity)
{
    pcchan_t chan = native_entity;
    assert(chan->refc > 0);
    chan->refc--;

    if (chan->qsize == 0 && chan->refc == 0) {
        // already closed
        remove_tmp_chan_file(chan);

        struct pcinst* inst = pcinst_current();
        assert(inst);

        pcintr_heap_t heap = inst->intr_heap;
        assert(heap);

        int r = pcutils_map_erase(heap->name_chan_map, chan->name);
        PC_ASSERT(r == 0);
    }
}

static bool
on_observe(void *native_entity,
        const char *event_name, const char *event_subname)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(event_name);
    UNUSED_PARAM(event_subname);
    return true;
}

static bool
did_matched(void *native_entity, purc_variant_t val)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(val);

    if (purc_variant_is_native(val)) {
        void *comp = purc_variant_native_get_entity(val);
        if (comp == native_entity) {
            return true;
        }

        purc_clr_error();
        return false;
    }
    else if (purc_variant_is_object(val)) {
        purc_variant_t flag = purc_variant_object_get_by_ckey(val, KEY_FLAG);
        if (!flag) {
            purc_clr_error();
            return false;
        }

        purc_variant_t name_val = purc_variant_object_get_by_ckey(val, KEY_NAME);
        if (!name_val) {
            purc_clr_error();
            return false;
        }

        pcchan_t chan = native_entity;
        if (strcmp(chan->name, purc_variant_get_string_const(name_val)) == 0) {
            return true;
        }
    }

    return false;
}

purc_variant_t
pcchan_make_entity(pcchan_t chan)
{
    static struct purc_native_ops ops = {
        .property_getter = property_getter,
        .property_setter = property_setter,
        .did_matched = did_matched,
        .on_observe = on_observe,
        .on_forget = NULL,
        .on_release = on_release,
    };

    if (chan->qsize == 0) {
        purc_set_error(PURC_ERROR_ENTITY_GONE);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t retv = purc_variant_make_native(chan, &ops);
    if (retv) {
        chan->refc++;
    }

    return retv;
}

