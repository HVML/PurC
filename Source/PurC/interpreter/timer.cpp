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


#define TO_DEBUG 1

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

        void setListener(struct pcvar_listener* listener)
        {
            m_listener = listener;
        }
        struct pcvar_listener* getListener(void) { return m_listener; }

        void setTimerVariant(purc_variant_t timer) { m_timer_var = timer; }
        purc_variant_t getTimerVariant(void) { return m_timer_var; }
    private:
        void fired() final { m_func(m_id, m_ctxt); }

    private:
        char* m_id;
        void* m_ctxt;
        pcintr_timer_fire_func m_func;

        uint32_t m_interval;
        struct pcvar_listener* m_listener;
        purc_variant_t m_timer_var;
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

void
pcintr_timer_set_listener(pcintr_timer_t timer, struct pcvar_listener* listener)
{
    if (timer) {
        return ((PurcTimer*)timer)-> setListener(listener);
    }
}

struct pcvar_listener*
pcintr_timer_get_listener(pcintr_timer_t timer)
{
    if (timer) {
        return ((PurcTimer*)timer)-> getListener();
    }
    return NULL;
}

void
pcintr_timer_set_timer_variant(pcintr_timer_t timer, purc_variant_t timer_var)
{
    if (timer) {
        return ((PurcTimer*)timer)-> setTimerVariant(timer_var);
    }
}

purc_variant_t
pcintr_timer_get_timer_variant(pcintr_timer_t timer)
{
    if (timer) {
        return ((PurcTimer*)timer)-> getTimerVariant();
    }
    return PURC_VARIANT_INVALID;
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
    struct pcvar_listener* grow_listener;
    struct pcvar_listener* shrink_listener;
    pcutils_map* timers_map; // id : pcintr_timer_t
};

static void* map_copy_val(const void* val)
{
    return (void*)val;
}

static void map_free_val(void* val)
{
    if (val) {
        pcintr_timer_t tm = (pcintr_timer_t)val;
        struct pcvar_listener* listener = pcintr_timer_get_listener(tm);
        purc_variant_t timer = pcintr_timer_get_timer_variant(tm);
        purc_variant_revoke_listener(timer, listener);
        pcintr_timer_destroy(tm);
    }
}

void timer_fire_func(const char* id, void* ctxt)
{
    pcintr_stack_t stack = (pcintr_stack_t) ctxt;
    purc_variant_t type = purc_variant_make_string(TIMERS_STR_EXPIRED, false);
    purc_variant_t sub_type = purc_variant_make_string(id, false);

    pcintr_dispatch_message((pcintr_stack_t)ctxt,
            stack->vdom->timers->timers_var,
            type, sub_type, PURC_VARIANT_INVALID);

    purc_variant_unref(type);
    purc_variant_unref(sub_type);
}

bool
timer_listener_handler(purc_variant_t source, purc_atom_t msg_type,
        void* ctxt, size_t nr_args, purc_variant_t* argv);

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
    if (0 == pcutils_map_insert(timers->timers_map, id, timer)) {
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
get_inner_timer(pcintr_stack_t stack, purc_variant_t timer_var)
{
    purc_variant_t id;
    id = purc_variant_object_get_by_ckey(timer_var, TIMERS_STR_ID, false);
    if (!id) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    const char* idstr = purc_variant_get_string_const(id);
    pcintr_timer_t timer = find_timer(stack->vdom->timers, idstr);
    if (timer) {
        return timer;
    }

    timer = pcintr_timer_create(idstr, stack, timer_fire_func);
    if (timer == NULL) {
        return NULL;
    }

    if (!add_timer(stack->vdom->timers, idstr, timer)) {
        pcintr_timer_destroy(timer);
        return NULL;
    }

    struct pcvar_listener* listener = purc_variant_register_post_listener(
            timer_var, pcvariant_atom_change, timer_listener_handler, stack);
    if (!listener) {
        remove_timer(stack->vdom->timers, idstr);
        return NULL;
    }
    pcintr_timer_set_listener(timer, listener);
    pcintr_timer_set_timer_variant(timer, timer_var);
    return timer;
}

static void
destroy_inner_timer(pcintr_stack_t stack, purc_variant_t timer_var)
{
    purc_variant_t id;
    id = purc_variant_object_get_by_ckey(timer_var, TIMERS_STR_ID, false);
    if (!id) {
        return;
    }

    const char* idstr = purc_variant_get_string_const(id);
    pcintr_timer_t timer = find_timer(stack->vdom->timers, idstr);
    if (timer) {
        struct pcvar_listener* listener = pcintr_timer_get_listener(timer);
        purc_variant_revoke_listener(timer_var, listener);
        remove_timer(stack->vdom->timers, idstr);
    }
}

bool
timer_listener_handler(purc_variant_t source, purc_atom_t msg_type,
        void* ctxt, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(nr_args);
    if (msg_type != pcvariant_atom_change) {
        return true;
    }
    pcintr_stack_t stack = (pcintr_stack_t) ctxt;
    pcintr_timer_t timer = get_inner_timer(stack, source);
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
        if (is_euqal(argv[1], TIMERS_STR_YES)) {
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
    UNUSED_PARAM(nr_args);
    pcintr_stack_t stack = (pcintr_stack_t) ctxt;
    if (msg_type == pcvariant_atom_grow) {
        purc_variant_t interval = purc_variant_object_get_by_ckey(argv[0],
                TIMERS_STR_INTERVAL, false);
        purc_variant_t active = purc_variant_object_get_by_ckey(argv[0],
                TIMERS_STR_ACTIVE, false);
        pcintr_timer_t timer = get_inner_timer(stack, argv[0]);
        if (!timer) {
            return false;
        }
        uint64_t ret = 0;
        purc_variant_cast_to_ulongint(interval, &ret, false);
        pcintr_timer_set_interval(timer, ret);
        if (is_euqal(active, TIMERS_STR_YES)) {
            pcintr_timer_start(timer);
        }
    }
    else if (msg_type == pcvariant_atom_shrink) {
        destroy_inner_timer(stack, argv[0]);
    }
    return true;
}

struct pcintr_timers*
pcintr_timers_init(pcintr_stack_t stack)
{
    purc_variant_t ret = purc_variant_make_set_by_ckey(0, TIMERS_STR_ID, NULL);
    if (!ret) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    if (!pcintr_bind_document_variable(stack->vdom, TIMERS_STR_TIMERS, ret)) {
        purc_variant_unref(ret);
        return NULL;
    }

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

    timers->grow_listener = purc_variant_register_post_listener(ret,
        pcvariant_atom_grow, timers_listener_handler, stack);
    if (!timers->grow_listener) {
        goto failure;
    }

    timers->shrink_listener = purc_variant_register_post_listener(ret,
        pcvariant_atom_shrink, timers_listener_handler, stack);
    if (!timers->shrink_listener) {
        goto failure;
    }

    purc_variant_unref(ret);
    return timers;

failure:
    if (timers)
        pcintr_timers_destroy(timers);
    pcintr_unbind_document_variable(stack->vdom, TIMERS_STR_TIMERS);
    purc_variant_unref(ret);
    return NULL;
}

void
pcintr_timers_destroy(struct pcintr_timers* timers)
{
    if (timers) {
        if (timers->grow_listener) {
            PC_ASSERT(timers->timers_var);
            purc_variant_revoke_listener(timers->timers_var,
                    timers->grow_listener);
            timers->grow_listener = NULL;
        }
        if (timers->shrink_listener) {
            PC_ASSERT(timers->timers_var);
            purc_variant_revoke_listener(timers->timers_var,
                    timers->shrink_listener);
            timers->shrink_listener = NULL;
        }

        // remove inner timer
        if (timers->timers_map) {
            pcutils_map_destroy(timers->timers_map);
            timers->timers_map = NULL;
        }

        PURC_VARIANT_SAFE_CLEAR(timers->timers_var);
        free(timers);
    }
}
