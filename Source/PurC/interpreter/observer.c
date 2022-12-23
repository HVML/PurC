/**
 * @file observer.c
 * @author Xue Shuming
 * @date 2022/07/01
 * @brief The impl for observer
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
 *
 */

#include "config.h"

#include "internal.h"
#include "private/instance.h"
#include "private/msg-queue.h"
#include "private/interpreter.h"
#include "private/regex.h"

#include <sys/time.h>

#define BUILTIN_VAR_CRTN        PURC_PREDEF_VARNAME_CRTN

static void
release_observer(struct pcintr_observer *observer)
{
    if (!observer)
        return;

    list_del(&observer->node);

    if (observer->on_revoke) {
        observer->on_revoke(observer, observer->on_revoke_data);
    }

    if (observer->observed != PURC_VARIANT_INVALID) {
        if (purc_variant_is_native(observer->observed)) {
            struct purc_native_ops *ops = purc_variant_native_get_ops(
                    observer->observed);
            if (ops && ops->on_forget) {
                void *native_entity = purc_variant_native_get_entity(
                        observer->observed);
                ops->on_forget(native_entity,
                        purc_atom_to_string(observer->msg_type_atom),
                        observer->sub_type);
            }
        }

        PURC_VARIANT_SAFE_CLEAR(observer->observed);
    }

    free(observer->sub_type);
    observer->sub_type = NULL;
}


static void
free_observer(struct pcintr_observer *observer)
{
    if (!observer)
        return;

    release_observer(observer);
    free(observer);
}

static void
add_observer_into_list(pcintr_stack_t stack, struct list_head *list,
        struct pcintr_observer* observer)
{
    observer->list = list;
    list_add_tail(&observer->node, list);

    // TODO:
    PC_ASSERT(stack);
    PC_ASSERT(stack->co->waits >= 0);
    stack->co->waits++;
}

static
bool is_variant_match_observe(pcintr_coroutine_t co, purc_variant_t observed,
        purc_variant_t val)
{
    UNUSED_PARAM(co);
    if (observed == val || purc_variant_is_equal_to(observed, val)) {
        return true;
    }
    else if (purc_variant_is_native(observed)) {
        struct purc_native_ops *ops = purc_variant_native_get_ops(observed);
        if (ops == NULL || ops->did_matched == NULL) {
            return false;
        }
        return ops->did_matched(purc_variant_native_get_entity(observed),
                val);
    }
    else if (pcintr_is_crtn_observed(observed)) {
        if (pcintr_crtn_observed_is_match(observed, val)) {
            return true;
        }
    }
    else if (pcintr_is_request_id(observed)) {
        if (pcintr_request_id_is_match(observed, val)) {
            return true;
        }
    }
    return false;
}



void
pcintr_destroy_observer_list(struct list_head *observer_list)
{
    struct pcintr_observer *p, *n;
    list_for_each_entry_reverse_safe(p, n, observer_list, node) {
        free_observer(p);
    }
}

static bool
is_match_default(pcintr_coroutine_t co, struct pcintr_observer *observer,
        pcrdr_msg *msg, purc_variant_t observed, purc_atom_t type,
        const char *sub_type)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(msg);
    if ((is_variant_match_observe(co, observer->observed, observed)) &&
                (observer->msg_type_atom == type)) {
        if (observer->sub_type == sub_type ||
                pcregex_is_match(observer->sub_type, sub_type)) {
            return true;
        }
    }
    return false;
}

static int
observer_handle_default(pcintr_coroutine_t co, struct pcintr_observer *p,
        pcrdr_msg *msg, purc_atom_t type, const char *sub_type, void *data)
{
    UNUSED_PARAM(type);
    UNUSED_PARAM(sub_type);
    UNUSED_PARAM(data);

    struct pcintr_observer_task *task;
    task = (struct pcintr_observer_task*)calloc(1, sizeof(*task));

    task->cor_stage = p->cor_stage;
    task->cor_state = p->cor_state;
    task->pos = p->pos;
    task->scope = p->scope;
    task->edom_element = p->edom_element;
    task->stack = &co->stack;

    if (msg->requestId) {
        task->request_id = purc_variant_ref(msg->requestId);
    }

    if (msg->eventName) {
        task->event_name = msg->eventName;
        purc_variant_ref(task->event_name);
    }

    if (msg->sourceURI) {
        task->source = msg->sourceURI;
        purc_variant_ref(task->source);
    }

    if (msg->data) {
        task->payload = msg->data;
        purc_variant_ref(task->payload);
    }

    list_add_tail(&task->ln, &co->tasks);
    return 0;
}

static uint64_t
get_timestamp_us(void)
{
    struct timeval now;
    gettimeofday(&now, 0);
    return (uint64_t)now.tv_sec * 1000000 + now.tv_usec;
}

struct pcintr_observer*
pcintr_register_observer(pcintr_stack_t  stack,
        enum pcintr_observer_source source,
        int                         cor_stage,
        int                         cor_state,
        purc_variant_t              observed,
        purc_atom_t                 msg_type_atom,
        const char                 *sub_type,
        pcvdom_element_t            scope,
        pcdoc_element_t             edom_element,
        pcvdom_element_t            pos,
        observer_on_revoke_fn       on_revoke,
        void                       *on_revoke_data,
        observer_match_fn           is_match,
        observer_handle_fn          handle,
        void                       *handle_data,
        bool                        auto_remove
        )
{
    struct list_head *list = NULL;
    if (source == OBSERVER_SOURCE_INTR) {
        list = &stack->intr_observers;
    }
    else {
        list = &stack->hvml_observers;
    }


    struct pcintr_observer* observer =  (struct pcintr_observer*)calloc(1,
            sizeof(struct pcintr_observer));
    if (!observer) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    observer->source = source;
    observer->cor_stage = cor_stage;
    observer->cor_state = cor_state;
    observer->stack = stack;
    observer->observed = observed;
    purc_variant_ref(observed);
    observer->scope = scope;
    observer->edom_element = edom_element;
    observer->pos = pos;
    observer->msg_type_atom = msg_type_atom;
    observer->sub_type = sub_type ? strdup(sub_type) : NULL;
    observer->on_revoke = on_revoke;
    observer->on_revoke_data = on_revoke_data;
    observer->is_match = is_match ? is_match : is_match_default;
    observer->handle = handle ? handle : observer_handle_default;
    observer->handle_data = handle_data;
    observer->auto_remove = auto_remove;
    observer->timestamp = get_timestamp_us();
    add_observer_into_list(stack, list, observer);

    // observe idle
    purc_atom_t idle_atom = purc_atom_try_string_ex(ATOM_BUCKET_MSG,
            MSG_TYPE_IDLE);
    if (pcintr_is_crtn_observed(observed) &&
            msg_type_atom == idle_atom && sub_type == NULL) {
        stack->observe_idle = 1;
    }

    return observer;
}

struct pcintr_observer *
pcintr_register_inner_observer(
        pcintr_stack_t            stack,
        int                       cor_stage,
        int                       cor_state,
        purc_variant_t            observed,
        const char               *event_type,
        const char               *event_sub_type,
        observer_match_fn         is_match,
        observer_handle_fn        handle,
        void                     *handle_data,
        bool                      auto_remove
        )
{
    struct pcintr_observer *observer = NULL;
    purc_atom_t event_type_atom = purc_atom_try_string_ex(ATOM_BUCKET_MSG,
            event_type);
    if (event_type_atom == 0) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "unknown event type '%s'", event_type);
        goto out;
    }

    observer = pcintr_register_observer(stack,
            OBSERVER_SOURCE_INTR,
            cor_stage,
            cor_state,
            observed,
            event_type_atom,
            event_sub_type,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            is_match,
            handle,
            handle_data,
            auto_remove
        );

out:
    return observer;
}

void
pcintr_revoke_observer(struct pcintr_observer* observer)
{
    if (!observer)
        return;

    // TODO:
    pcintr_stack_t stack = observer->stack;
    PC_ASSERT(stack);
    PC_ASSERT(stack->co->waits >= 1);
    stack->co->waits--;

    // observe idle
    purc_variant_t hvml = pcintr_get_coroutine_variable(stack->co,
            BUILTIN_VAR_CRTN);
    if (observer->observed == hvml) {
        stack->observe_idle = 0;
    }

    free_observer(observer);
}

static void
revoke_observer_from_list(pcintr_coroutine_t co, struct list_head *list,
        purc_variant_t observed, purc_atom_t msg_type_atom, const char *sub_type)
{
    struct pcintr_observer *p, *n;
    list_for_each_entry_safe(p, n, list, node) {
        if (p->is_match(co, p, NULL, observed, msg_type_atom, sub_type)) {
            pcintr_revoke_observer(p);
            break;
        }
    }
}

void
pcintr_revoke_observer_ex(pcintr_stack_t stack, purc_variant_t observed,
        purc_atom_t msg_type_atom, const char *sub_type)
{
    revoke_observer_from_list(stack->co, &stack->hvml_observers, observed,
            msg_type_atom, sub_type);
    revoke_observer_from_list(stack->co, &stack->intr_observers, observed,
            msg_type_atom, sub_type);
}

