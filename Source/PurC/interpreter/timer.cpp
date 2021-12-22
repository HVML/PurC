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

#define TIMERS_STR_ID               "id"
#define TIMERS_STR_INTERVAL         "interval"
#define TIMERS_STR_ACTIVE           "active"
#define TIMERS_STR_ON               "on"

#define TIMERS_STR_GROWN            "grown"
#define TIMERS_STR_SHRUNK           "shrunk"
#define TIMERS_STR_TIMERS           "timers"
#define TIMERS_STR_TIMER            "timer"
#define TIMERS_STR_CHANGE           "change"
#define TIMERS_STR_HANDLE           "__handle"

purc_atom_t pcatom_grown;
purc_atom_t pcatom_shrunk;
purc_atom_t pcatom_timers;
purc_atom_t pcatom_timer;
purc_atom_t pcatom_change;

#if 0
static bool
is_euqal(purc_variant_t var, const char* comp)
{
    if (var && comp) {
        return (strcmp(purc_variant_get_string_const(var), comp) == 0);
    }
    return false;
}
#endif

pcintr_timer_t pcintr_timer_get_by_id(purc_vdom_t dom, const char* id)
{
    UNUSED_PARAM(dom);
    UNUSED_PARAM(id);
#if 0
    purc_variant_t timers = pcvdom_document_get_variable(dom, TIMERS_STR_TIMERS);

    purc_variant_t dest_obj = purc_variant_set_get_member_by_key_values(timers,
            id);
    if (!dest_obj) {
        return NULL;
    }

    purc_variant_t vtimer = purc_variant_object_get_by_ckey(dest_obj,
            TIMERS_STR_HANDLE);
    if (vtimer) {
        uint64_t ret = 0;
        purc_variant_cast_to_ulongint(vtimer, &ret, false);
        return (pctimer_t)ret;
    }

    pcintr_timer_t timer = pcintr_timer_create(id, doc pcintr_timer_fire_func);
    purc_variant_object_set_by_static_ckey(dest_obj, TIMERS_STR_HANDLE,
            purc_variant_make_ulongint((uint64_t)timer));
    return timer;
#endif
    return NULL;
}

bool
timer_listener_handler(purc_variant_t source, purc_atom_t msg_type,
        void* ctxt, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(source);
    UNUSED_PARAM(msg_type);
    UNUSED_PARAM(ctxt);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
#if 0
    if (msg_type  !=  pcatom_change) {
        return true;
    }
    purc_vdom_t dom = (purc_vdom_t) ctxt;
    purc_variant_t id = purc_variant_object_get_by_ckey(observer->observed,
            TIMERS_STR_ID);
    pctimer_id timer = pcintr_timer_get_by_id(dom,
            purc_variant_get_string_const(id));
    // argv key-new, value-new, key-old, value-old
    if (is_euqal(argv[0], TIMERS_STR_INTERVAL)) {
        pcintr_timer_set_interval(timer, purc_variant_cast_to_ulongint(argv[0]));
    }
    else if (is_euqal(argv[0], TIMERS_STR_ACTIVE)) {
        if (is_euqal(argv[1], TIMERS_STR_ON)) {
            pcintr_timer_start(timer);
        }
        else {
            pcintr_timer_stop(timer);
        }
    }
#endif
    return true;
}

bool
timers_listener_handler(purc_variant_t source, purc_atom_t msg_type,
        void* ctxt, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(source);
    //purc_vdom_t dom = (purc_vdom_t) ctxt;
    if (msg_type == pcatom_grown) {
        for (size_t i = 0; i < nr_args; i++) {
#if 0
            purc_variant_t id = purc_variant_object_get_by_ckey(argv[i],
                    TIMERS_STR_ID);
            purc_variant_t intrval = purc_variant_object_get_by_ckey(argv[i],
                    TIMERS_STR_INTERVAL);
            purc_variant_t active = purc_variant_object_get_by_ckey(argv[i],
                    TIMERS_STR_ACTIVE);

            pctimer_id timer = pcintr_timer_get_by_id(dom,
                    purc_variant_get_string_const(id));
            pcintr_timer_set_interval(timer,
                    purc_variant_cast_to_ulongint(interval));
            if (is_euqal(active, TIMERS_STR_ON)) {
                pcintr_timer_start(timer);
            }
#endif
            purc_variant_register_listener(argv[i], pcatom_timer,
                    timer_listener_handler, ctxt);
        }
    }
    else if (msg_type == pcatom_shrunk) {
        for (size_t i = 0; i < nr_args; i++) {
//            purc_variant_t id = purc_variant_object_get_by_ckey(argv[i],
//                    TIMERS_STR_ON);
            purc_variant_revoke_listener(argv[i], pcatom_timer);
//            pcintr_timer_destory_by_id(dom, purc_variant_get_string_const(id));
        }
    }
    return true;
}

// init $TIMERS
bool
pcintr_init_timers(void)
{
    pcintr_stack_t stack = purc_get_stack();
    if (stack == NULL || stack->vdom == NULL) {
        purc_set_error(PURC_ERROR_NO_INSTANCE);
        return false;
    }

    purc_variant_t ret = purc_variant_make_set_by_ckey(0, TIMERS_STR_ID, NULL);
    if (!ret) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return false;
    }

    if (!pcintr_bind_document_variable(stack->vdom, TIMERS_STR_TIMERS, ret)) {
        purc_variant_unref(ret);
        return false;
    }

    // regist listener
    bool regist = purc_variant_register_listener(ret, pcatom_timers,
            timers_listener_handler, stack->vdom);
    if (!regist) {
        purc_variant_unref(ret);
        return false;
    }

    return true;
}

