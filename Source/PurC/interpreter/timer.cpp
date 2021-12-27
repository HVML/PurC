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
#include "private/runloop.h"

#include <wtf/RunLoop.h>
#include <wtf/Seconds.h>

#include <stdlib.h>
#include <string.h>


class PurcTimer : public WTF::RunLoop::TimerBase {
    public:
        PurcTimer(const char* id, void* ctxt, pcintr_timer_fire_func func,
                RunLoop& runLoop)
            : TimerBase(runLoop)
            , m_id(NULL)
            , m_ctxt(ctxt)
            , m_func(func)
        {
            m_id = strdup(id);
        }

        ~PurcTimer()
        {
            if (m_id) {
                free(m_id);
            }
        }

        void setInterval(uint32_t interval) { m_interval = interval; }
        uint32_t getInterval() { return m_interval; }
    private:
        void fired() final { m_func(m_id, m_ctxt); }

    private:
        char* m_id;
        void* m_ctxt;
        pcintr_timer_fire_func m_func;

        uint32_t m_interval;
};

pcintr_timer_t
pcintr_timer_create(const char* id, void* ctxt, pcintr_timer_fire_func func)
{
    PurcTimer* timer = new PurcTimer(id, ctxt, func, RunLoop::current());
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
        ((PurcTimer*)timer)->setInterval(interval);
    }
}

uint32_t
pcintr_timer_get_interval(pcintr_timer_t timer)
{
    if (timer) {
        return ((PurcTimer*)timer)->getInterval();
    }
    return 0;
}

void
pcintr_timer_start(pcintr_timer_t timer)
{
    if (timer) {
        PurcTimer* tm = (PurcTimer*)timer;
        return tm->startRepeating(
                WTF::Seconds::fromMilliseconds(tm->getInterval()));
    }
}

void
pcintr_timer_start_oneshot(pcintr_timer_t timer)
{
    if (timer) {
        PurcTimer* tm = (PurcTimer*)timer;
        return tm->startOneShot(
                WTF::Seconds::fromMilliseconds(tm->getInterval()));
    }
}

void
pcintr_timer_stop(pcintr_timer_t timer)
{
    if (timer) {
        return ((PurcTimer*)timer)->stop();
    }
}

void
pcintr_timer_destroy(pcintr_timer_t timer)
{
    if (timer) {
        PurcTimer* tm = (PurcTimer*)timer;
        delete tm;
    }
}

//  $TIMERS begin

#define TIMERS_STR_ID               "id"
#define TIMERS_STR_INTERVAL         "interval"
#define TIMERS_STR_ACTIVE           "active"
#define TIMERS_STR_ON               "on"
#define TIMERS_STR_TIMERS           "timers"
#define TIMERS_STR_HANDLE           "__handle"

struct pcintr_timers {
    purc_variant_t timers_var;
    struct pcvar_listener* grow_listener;
    struct pcvar_listener* shrink_listener;
};

void timer_fire_func(const char* id, void* ctxt)
{
    UNUSED_PARAM(id);
    UNUSED_PARAM(ctxt);  // vdom
}

static bool
is_euqal(purc_variant_t var, const char* comp)
{
    if (var && comp) {
        return (strcmp(purc_variant_get_string_const(var), comp) == 0);
    }
    return false;
}

static purc_variant_t
pointer_to_variant(void* p)
{
    return p ? purc_variant_make_native(p, NULL) : PURC_VARIANT_INVALID;
}

static void*
variant_to_pointer(purc_variant_t var)
{
    if (var && purc_variant_is_type(var, PURC_VARIANT_TYPE_NATIVE)) {
        return purc_variant_native_get_entity(var);
    }
    return NULL;
}

static pcintr_timer_t
get_inner_timer(purc_vdom_t vdom, purc_variant_t timer_var)
{
    purc_variant_t tm = purc_variant_object_get_by_ckey(timer_var,
            TIMERS_STR_HANDLE);
    pcintr_timer_t timer = variant_to_pointer(tm);
    if (timer) {
        return timer;
    }

    purc_variant_t id = purc_variant_object_get_by_ckey(timer_var, TIMERS_STR_ID);
    if (!id) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    timer = pcintr_timer_create(purc_variant_get_string_const(id),
            vdom, timer_fire_func);
    if (timer == NULL) {
        return NULL;
    }

    purc_variant_t native = pointer_to_variant(timer);
    purc_variant_object_set_by_static_ckey(timer_var, TIMERS_STR_HANDLE, native);
    purc_variant_unref(native);
    return timer;
}

#if 0
static void
destroy_inner_timer(purc_variant_t timer_var)
{
    purc_variant_t tm = purc_variant_object_get_by_ckey(timer_var,
            TIMERS_STR_HANDLE);
    pcintr_timer_t timer = variant_to_pointer(tm);
    if (timer) {
        pcintr_timer_destroy(timer);
    }
}
#endif // 0

bool
timer_listener_handler(purc_variant_t source, purc_atom_t msg_type,
        void* ctxt, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(nr_args);
    if (msg_type != pcvariant_atom_change) {
        return true;
    }
    purc_vdom_t dom = (purc_vdom_t) ctxt;
    pcintr_timer_t timer = get_inner_timer(dom, source);
    if (!timer) {
        return false;
    }

    // argv key-new, value-new, key-old, value-old
    if (is_euqal(argv[0], TIMERS_STR_INTERVAL)) {
        uint64_t ret = 0;
        purc_variant_cast_to_ulongint(argv[0], &ret, false);
        pcintr_timer_set_interval(timer, ret);
    }
    else if (is_euqal(argv[0], TIMERS_STR_ACTIVE)) {
        if (is_euqal(argv[1], TIMERS_STR_ON)) {
            pcintr_timer_start(timer);
        }
        else {
            pcintr_timer_stop(timer);
        }
    }
    return true;
}

bool
timers_listener_handler(purc_variant_t source, purc_atom_t msg_type,
        void* ctxt, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(source);
    UNUSED_PARAM(msg_type);
    UNUSED_PARAM(ctxt);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
#if 0
    purc_vdom_t dom = (purc_vdom_t) ctxt;
    if (msg_type == pcvariant_atom_grown) {
        for (size_t i = 0; i < nr_args; i++) {
            purc_variant_t interval = purc_variant_object_get_by_ckey(argv[i],
                    TIMERS_STR_INTERVAL);
            purc_variant_t active = purc_variant_object_get_by_ckey(argv[i],
                    TIMERS_STR_ACTIVE);

            pcintr_timer_t timer = get_inner_timer(dom, argv[i]);
            if (!timer) {
                return false;
            }
            uint64_t ret = 0;
            purc_variant_cast_to_ulongint(interval, &ret, false);
            pcintr_timer_set_interval(timer, ret);
            if (is_euqal(active, TIMERS_STR_ON)) {
                pcintr_timer_start(timer);
            }
            purc_variant_register_listener(argv[i], pcvariant_atom_timer,
                    timer_listener_handler, ctxt);
        }
    }
    else if (msg_type == pcvariant_atom_shrunk) {
        for (size_t i = 0; i < nr_args; i++) {
            purc_variant_revoke_listener(argv[i], pcvariant_atom_timer);
            destroy_inner_timer(argv[1]);
        }
    }
#endif // 0
    return true;
}

struct pcintr_timers*
pcintr_timers_init(purc_vdom_t vdom)
{
    if (vdom == NULL) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        return NULL;
    }

    purc_variant_t ret = purc_variant_make_set_by_ckey(0, TIMERS_STR_ID, NULL);
    if (!ret) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    if (!pcintr_bind_document_variable(vdom, TIMERS_STR_TIMERS, ret)) {
        purc_variant_unref(ret);
        return NULL;
    }

    struct pcintr_timers* timers = (struct pcintr_timers*) calloc(1, 
            sizeof(struct pcintr_timers));
    if (!timers) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        purc_variant_unref(ret);
        return NULL;
    }

    timers->timers_var = ret;
    timers->grow_listener = purc_variant_register_post_listener(ret,
        pcvariant_atom_grow, timers_listener_handler, vdom);
    if (!timers->grow_listener) {
        free(timers);
        purc_variant_unref(ret);
        return NULL;
    }

    timers->shrink_listener = purc_variant_register_post_listener(ret,
        pcvariant_atom_shrink, timers_listener_handler, vdom);
    if (!timers->shrink_listener) {
        purc_variant_revoke_listener(ret, timers->grow_listener);
        free(timers);
        purc_variant_unref(ret);
        return NULL;
    }

    return timers;
}

void
pcintr_timers_destroy(struct pcintr_timers* timers)
{
    if (timers) {
        purc_variant_revoke_listener(timers->timers_var,
                timers->grow_listener);
        purc_variant_revoke_listener(timers->timers_var,
                timers->shrink_listener);

        // TODO revoke listener of timer

        //
        free(timers);
    }
}
