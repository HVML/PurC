/*
 * @file timer.cpp
 * @author XueShuming
 * @date 2021/12/20
 * @brief The C api for timer.
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

#include "purc.h"

#include "config.h"

#include "internal.h"

#include "private/errors.h"
#include "private/timer.h"
#include "private/interpreter.h"
#include "purc-runloop.h"

#include <wtf/RunLoop.h>
#include <wtf/Seconds.h>

#include <stdlib.h>
#include <string.h>

class Timer : public PurCWTF::RunLoop::TimerBase {
    public:
        Timer(const char *id, pcintr_timer_fire_func func, RunLoop& runLoop,
                void *data)
            : TimerBase(runLoop)
            , m_id(NULL)
            , m_func(func)
            , m_data(data)
        {
            m_id = id ? strdup(id) : NULL;
        }

        ~Timer()
        {
            stop();
            if (m_id) {
                free(m_id);
            }
        }

        void setInterval(uint32_t interval) { m_interval = interval; }
        uint32_t getInterval() { return m_interval; }
        const char *getId() { return m_id; }
        void *getData() { return m_data; }

        virtual void fired()
        {
            m_func(this, m_id, m_data);
        }

        virtual void processed(void) {}

    private:
        char *m_id;
        pcintr_timer_fire_func m_func;
        void *m_data;
        uint32_t m_interval;
};

pcintr_timer_t
pcintr_timer_create(purc_runloop_t runloop, const char* id,
        pcintr_timer_fire_func func, void *data)
{
    RunLoop* loop = runloop ? (RunLoop*)runloop : &RunLoop::current();
    Timer* timer = new Timer(id, func, *loop, data);
    if (!timer) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    return timer;
}

void
pcintr_timer_set_interval(pcintr_timer_t timer, uint32_t interval)
{
    if (timer) {
        ((Timer*)timer)->setInterval(interval);
    }
}

uint32_t
pcintr_timer_get_interval(pcintr_timer_t timer)
{
    if (timer) {
        return ((Timer*)timer)->getInterval();
    }
    return 0;
}

void
pcintr_timer_start(pcintr_timer_t timer)
{
    if (timer) {
        Timer* tm = (Timer*)timer;
        tm->startRepeating(
                PurCWTF::Seconds::fromMilliseconds(tm->getInterval()));
    }
}

void
pcintr_timer_start_oneshot(pcintr_timer_t timer)
{
    if (timer) {
        Timer* tm = (Timer*)timer;
        tm->startOneShot(
                PurCWTF::Seconds::fromMilliseconds(tm->getInterval()));
    }
}

void
pcintr_timer_stop(pcintr_timer_t timer)
{
    if (timer) {
        ((Timer*)timer)->stop();
    }
}

bool
pcintr_timer_is_active(pcintr_timer_t timer)
{
    return timer ? ((Timer*)timer)->isActive() : false;
}

void
pcintr_timer_destroy(pcintr_timer_t timer)
{
    if (timer) {
        Timer* tm = (Timer*)timer;
        delete tm;
    }
}

//  $TIMERS begin

#define TIMERS_STR_ID               "id"
#define TIMERS_STR_INTERVAL         "interval"
#define TIMERS_STR_ACTIVE           "active"
#define TIMERS_STR_YES              "yes"
#define TIMERS_STR_TIMERS           "TIMERS"
#define TIMERS_STR_EXPIRED          "expired"

struct pcintr_timers {
    purc_variant_t timers_var;
    struct pcvar_listener* timer_listener;
    pcutils_map* timers_map; // id : pcintr_timer_t
    pcutils_map* listener_map; // variant : struct pcvar_listener
};

int
listener_map_comp_by_key(const void *key1, const void *key2)
{
    uintptr_t k1 = (uintptr_t)key1;
    uintptr_t k2 = (uintptr_t)key2;
    return k1 - k2;
}

int
listener_map_set_listener(pcutils_map *map, purc_variant_t obj,
        struct pcvar_listener *listener)
{
    pcutils_map_entry *entry = pcutils_map_find(map, obj);
    if (entry) {
        return -1;
    }

    return pcutils_map_insert(map, obj, listener);
}

void
listener_map_remove_listener(pcutils_map *map, purc_variant_t obj)
{
    pcutils_map_entry *entry = pcutils_map_find(map, obj);
    if (entry == NULL) {
        return;
    }

    struct pcvar_listener *listener = (struct pcvar_listener*)(entry->val);

    pcutils_map_erase(map, obj);

    purc_variant_revoke_listener(obj, listener);
}



static void* map_copy_val(const void* val)
{
    return (void*)val;
}

static void map_free_val(void* val)
{
    if (val) {
        pcintr_timer_destroy((pcintr_timer_t)val);
    }
}

static void timer_fire_func(pcintr_timer_t timer, const char *id, void *data)
{
    UNUSED_PARAM(timer);
    PC_ASSERT(pcintr_get_heap());

    purc_coroutine_t cor = (purc_coroutine_t)data;
    if (cor->stack.exited) {
        return;
    }

    pcintr_coroutine_post_event(cor->cid,
        PCRDR_MSG_EVENT_REDUCE_OPT_OVERLAY,
        cor->timers->timers_var, TIMERS_STR_EXPIRED, id,
        PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
}

static bool
is_euqal(purc_variant_t var, const char* comp)
{
    if (var && comp) {
        return (strcmp(purc_variant_get_string_const(var), comp) == 0);
    }
    return false;
}

pcintr_timer_t
find_timer(struct pcintr_timers* timers, const char* id)
{
    pcutils_map_entry* entry = pcutils_map_find(timers->timers_map, id);
    return entry ? (pcintr_timer_t) entry->val : NULL;
}

bool
add_timer(struct pcintr_timers* timers, const char* id, pcintr_timer_t timer)
{
    int r;
    r = pcutils_map_replace_or_insert(timers->timers_map, id, timer, NULL);
    if (0 == r) {
        return true;
    }
    return false;
}

void
remove_timer(struct pcintr_timers* timers, const char* id)
{
    pcutils_map_erase(timers->timers_map, (void*)id);
}

static pcintr_timer_t
get_inner_timer(purc_coroutine_t cor , purc_variant_t timer_var)
{
    purc_variant_t id = purc_variant_object_get_by_ckey_ex(timer_var,
            TIMERS_STR_ID, true);
    if (!id) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    const char* idstr = purc_variant_get_string_const(id);
    pcintr_timer_t timer = find_timer(cor->timers, idstr);
    if (timer) {
        return timer;
    }

    timer = pcintr_timer_create(NULL, idstr, timer_fire_func, cor);
    if (timer == NULL) {
        return NULL;
    }

    if (!add_timer(cor->timers, idstr, timer)) {
        pcintr_timer_destroy(timer);
        return NULL;
    }
    return timer;
}

static void
destroy_inner_timer(purc_coroutine_t cor, purc_variant_t timer_var)
{
    purc_variant_t id;
    id = purc_variant_object_get_by_ckey_ex(timer_var, TIMERS_STR_ID, true);
    if (!id) {
        return;
    }

    const char* idstr = purc_variant_get_string_const(id);
    pcintr_timer_t timer = find_timer(cor->timers, idstr);
    if (timer) {
        remove_timer(cor->timers, idstr);
    }
}

bool
timer_listener_handler(purc_variant_t source, pcvar_op_t msg_type,
        void* ctxt, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(source);
    UNUSED_PARAM(msg_type);
    UNUSED_PARAM(ctxt);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    purc_variant_t nv = source;
    pcintr_timer_t timer = (pcintr_timer_t)ctxt;

    purc_variant_t interval = purc_variant_object_get_by_ckey_ex(nv,
            TIMERS_STR_INTERVAL, true);
    purc_variant_t active = purc_variant_object_get_by_ckey_ex(nv,
            TIMERS_STR_ACTIVE, true);
    if (interval != PURC_VARIANT_INVALID) {
        uint64_t ret = 0;
        purc_variant_cast_to_ulongint(interval, &ret, false);
        uint32_t oval = pcintr_timer_get_interval(timer);
        if (oval != ret) {
            pcintr_timer_set_interval(timer, ret);
        }
    }

    bool next_active = pcintr_timer_is_active(timer);
    if (active != PURC_VARIANT_INVALID) {
        if (is_euqal(active, TIMERS_STR_YES)) {
            next_active = true;
        }
        else {
            next_active = false;
        }
    }

    if (next_active) {
        pcintr_timer_start(timer);
    }
    else {
        pcintr_timer_stop(timer);
    }
    return true;
}

bool
timers_set_grow(purc_variant_t source, pcvar_op_t msg_type,
        void *ctxt, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(source);
    UNUSED_PARAM(msg_type);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(ctxt);

    purc_coroutine_t cor = (purc_coroutine_t)ctxt;
    struct pcvar_listener *listener = NULL;

    pcintr_timer_t timer = get_inner_timer(cor, argv[0]);
    if (!timer) {
        return false;
    }

    purc_variant_t interval = purc_variant_object_get_by_ckey_ex(argv[0],
            TIMERS_STR_INTERVAL, true);
    purc_variant_t active = purc_variant_object_get_by_ckey_ex(argv[0],
            TIMERS_STR_ACTIVE, true);
    // TODO: check validation of interval and active here.

    listener = purc_variant_register_post_listener(argv[0],
            PCVAR_OPERATION_CHANGE, timer_listener_handler, timer);
    if (!listener) {
        return false;
    }
    listener_map_set_listener(cor->timers->listener_map, argv[0], listener);

    uint64_t ret = 0;
    purc_variant_cast_to_ulongint(interval, &ret, false);
    pcintr_timer_set_interval(timer, ret);
    if (is_euqal(active, TIMERS_STR_YES)) {
        pcintr_timer_start(timer);
    }
    return true;
}

bool
timers_set_shrink(purc_variant_t source, pcvar_op_t msg_type,
        void* ctxt, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(source);
    UNUSED_PARAM(msg_type);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(ctxt);

    purc_coroutine_t cor = (purc_coroutine_t)ctxt;
    listener_map_remove_listener(cor->timers->listener_map, argv[0]);
    destroy_inner_timer(cor, argv[0]);
    return true;
}

bool
timers_set_change(purc_variant_t source, pcvar_op_t msg_type,
        void* ctxt, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(source);
    UNUSED_PARAM(msg_type);
    UNUSED_PARAM(ctxt);
    UNUSED_PARAM(nr_args);

    purc_coroutine_t cor = (purc_coroutine_t)ctxt;
    struct pcvar_listener *listener = NULL;

    purc_variant_t nv = argv[1];
    pcintr_timer_t timer = get_inner_timer(cor, nv);
    if (!timer) {
        return false;
    }

    listener_map_remove_listener(cor->timers->listener_map, argv[0]);
    listener = purc_variant_register_post_listener(nv,
            PCVAR_OPERATION_CHANGE, timer_listener_handler, timer);
    if (!listener) {
        return false;
    }
    listener_map_set_listener(cor->timers->listener_map, nv, listener);

    purc_variant_t interval = purc_variant_object_get_by_ckey_ex(nv,
            TIMERS_STR_INTERVAL, true);
    purc_variant_t active = purc_variant_object_get_by_ckey_ex(nv,
            TIMERS_STR_ACTIVE, true);
    if (interval != PURC_VARIANT_INVALID) {
        uint64_t ret = 0;
        purc_variant_cast_to_ulongint(interval, &ret, false);
        uint32_t oval = pcintr_timer_get_interval(timer);
        if (oval != ret) {
            pcintr_timer_set_interval(timer, ret);
        }
    }

    bool next_active = pcintr_timer_is_active(timer);
    if (active != PURC_VARIANT_INVALID) {
        if (is_euqal(active, TIMERS_STR_YES)) {
            next_active = true;
        }
        else {
            next_active = false;
        }
    }

    if (next_active) {
        pcintr_timer_start(timer);
    }
    else {
        pcintr_timer_stop(timer);
    }
    return true;
}

bool
timers_set_listener_handler(purc_variant_t source, pcvar_op_t msg_type,
        void *ctxt, size_t nr_args, purc_variant_t *argv)
{
    switch (msg_type) {
    case PCVAR_OPERATION_GROW:
        return timers_set_grow(source, msg_type, ctxt, nr_args, argv);

    case PCVAR_OPERATION_SHRINK:
        return timers_set_shrink(source, msg_type, ctxt, nr_args, argv);

    case PCVAR_OPERATION_CHANGE:
        return timers_set_change(source, msg_type, ctxt, nr_args, argv);

    default:
        return true;
    }
    return true;
}

struct pcintr_timers*
pcintr_timers_init(purc_coroutine_t cor)
{
    purc_variant_t ret = purc_variant_make_set_by_ckey(0, TIMERS_STR_ID, NULL);
    if (!ret) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    if (!pcintr_bind_coroutine_variable(cor, TIMERS_STR_TIMERS, ret)) {
        purc_variant_unref(ret);
        return NULL;
    }

    int op = PCVAR_OPERATION_GROW | PCVAR_OPERATION_SHRINK |
        PCVAR_OPERATION_CHANGE;
    struct pcintr_timers* timers = (struct pcintr_timers*) calloc(1,
            sizeof(struct pcintr_timers));
    if (!timers) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failure;
    }

    timers->timers_var = ret;
    purc_variant_ref(ret);

    timers->timers_map = pcutils_map_create (copy_key_string, free_key_string,
                          map_copy_val, map_free_val, comp_key_string, false);
    if (!timers->timers_map) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failure;
    }

    timers->listener_map = pcutils_map_create (NULL, NULL, NULL, NULL,
            listener_map_comp_by_key, false);
    if (!timers->listener_map) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failure;
    }

    timers->timer_listener = purc_variant_register_post_listener(ret,
            (pcvar_op_t)op, timers_set_listener_handler, cor);
    if (!timers->timer_listener) {
        goto failure;
    }

    purc_variant_unref(ret);
    return timers;

failure:
    if (timers)
        pcintr_timers_destroy(timers);
    pcintr_unbind_coroutine_variable(cor, TIMERS_STR_TIMERS);
    purc_variant_unref(ret);
    return NULL;
}

void
pcintr_timers_destroy(struct pcintr_timers* timers)
{
    if (timers) {
        // remove set member -> remove object listener
        while (1) {
            ssize_t nr = purc_variant_set_get_size(timers->timers_var);
            if (nr == 0) {
                break;
            }
            purc_variant_t obj = purc_variant_set_remove_by_index(
                    timers->timers_var, 0);
            PURC_VARIANT_SAFE_CLEAR(obj);
        }

        if (timers->timer_listener) {
            PC_ASSERT(timers->timers_var);
            purc_variant_revoke_listener(timers->timers_var,
                    timers->timer_listener);
            timers->timer_listener = NULL;
        }

        // remove inner timer
        if (timers->timers_map) {
            pcutils_map_destroy(timers->timers_map);
            timers->timers_map = NULL;
        }

        if (timers->listener_map) {
            pcutils_map_destroy(timers->listener_map);
            timers->listener_map = NULL;
        }

        PURC_VARIANT_SAFE_CLEAR(timers->timers_var);
        free(timers);
    }
}

bool
pcintr_is_timers(purc_coroutine_t cor, purc_variant_t v)
{
    if (!cor) {
        return false;
    }
    return (v == cor->timers->timers_var);
}
