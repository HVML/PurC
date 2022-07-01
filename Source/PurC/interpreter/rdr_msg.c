/**
 * @file rdr_msc
 * @author Xu Xiaohong
 * @date 2022/06/08
 * @brief The internal interfaces for interpreter
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

static void
process_rdr_msg_by_event(pcrdr_msg *msg)
{
    switch (msg->target) {
        case PCRDR_MSG_TARGET_SESSION:
            // NOTE: not implemented yet
            PC_ASSERT(0);
            break;
        case PCRDR_MSG_TARGET_WORKSPACE:
            // NOTE: not implemented yet
            PC_ASSERT(0);
            break;
        case PCRDR_MSG_TARGET_PLAINWINDOW:
            // NOTE: not implemented yet
            PC_ASSERT(0);
            break;
        case PCRDR_MSG_TARGET_WIDGET:
            // NOTE: not implemented yet
            PC_ASSERT(0);
            break;
        case PCRDR_MSG_TARGET_DOM:
            // NOTE: not implemented yet
            PC_ASSERT(0);
            break;
        case PCRDR_MSG_TARGET_INSTANCE:
            // NOTE: not implemented yet
            PC_ASSERT(0);
            break;
        case PCRDR_MSG_TARGET_COROUTINE:
            // NOTE: not implemented yet
            PC_ASSERT(0);
            break;
        case PCRDR_MSG_TARGET_USER:
            // NOTE: not implemented yet
            PC_ASSERT(0);
            break;
        default:
            // NOTE: shouldn't happen, no way to recover gracefully, fail-fast
            PC_ASSERT(0);
    }
}

void
pcintr_check_and_dispatch_msg(void)
{
    pcintr_coroutine_t co = pcintr_get_coroutine();
    if (!co) {
        return;
    }

    int r;
    size_t n;
    r = purc_inst_holding_messages_count(&n);
    PC_ASSERT(r == 0);
    if (n <= 0)
        return;

    pcrdr_msg *msg;
    msg = purc_inst_take_away_message(0);
    if (msg == NULL) {
        PC_ASSERT(purc_get_last_error() == 0);
        return;
    }

    switch (msg->type) {
        case PCRDR_MSG_TYPE_VOID:
            // NOTE: not implemented yet
            PC_ASSERT(0);
            break;
        case PCRDR_MSG_TYPE_REQUEST:
            // NOTE: not implemented yet
            PC_ASSERT(0);
            break;
        case PCRDR_MSG_TYPE_RESPONSE:
            // NOTE: not implemented yet
            PC_ASSERT(0);
            break;
        case PCRDR_MSG_TYPE_EVENT:
            return process_rdr_msg_by_event(msg);
            break;
        default:
            // NOTE: shouldn't happen, no way to recover gracefully, fail-fast
            PC_ASSERT(0);
    }

    PC_ASSERT(0);
}

#if 0
static
bool is_variant_match_observe(purc_variant_t observed, purc_variant_t val)
{
    if (observed == val) {
        return true;
    }
    if (purc_variant_is_native(observed)) {
        struct purc_native_ops *ops = purc_variant_native_get_ops(observed);
        if (ops == NULL || ops->match_observe == NULL) {
            return false;
        }
        return ops->match_observe(purc_variant_native_get_entity(observed),
                val);
    }
    return false;
}

static bool
is_observer_match(struct pcintr_observer *observer,
        purc_variant_t observed, purc_atom_t type_atom, const char *sub_type)
{
    if ((is_variant_match_observe(observer->observed, observed)) &&
                (observer->msg_type_atom == type_atom)) {
        if (observer->sub_type == sub_type ||
                pcregex_is_match(observer->sub_type, sub_type)) {
            return true;
        }
    }
    return false;
}

static struct list_head*
get_observer_list(pcintr_stack_t stack, purc_variant_t observed)
{
    PC_ASSERT(observed != PURC_VARIANT_INVALID);

    struct list_head *list = NULL;
    if (purc_variant_is_type(observed, PURC_VARIANT_TYPE_DYNAMIC)) {
        list = &stack->dynamic_variant_observer_list;
    }
    else if (purc_variant_is_type(observed, PURC_VARIANT_TYPE_NATIVE)) {
        list = &stack->native_variant_observer_list;
    }
    else if (purc_variant_is_string(observed)) {
        // XXX: optimization
        // CSS selector used string
        // handle by elements.c match_observe
        const char *s = purc_variant_get_string_const(observed);
        if (strlen(s) > 0 && (s[0] == '#' || s[0] == '.')) {
            list = &stack->native_variant_observer_list;
        }
        else {
            list = &stack->common_variant_observer_list;
        }
    }
    else if (pcintr_is_named_var_for_event(observed)) {
        list = &stack->native_variant_observer_list;
    }
    else {
        list = &stack->common_variant_observer_list;
    }
    return list;
}

void
process_event_msg(pcintr_coroutine_t co, pcrdr_msg *msg)
{
    pcintr_stack_t stack = &co->stack;
    PC_ASSERT(stack);
    PC_ASSERT(co->state == CO_STATE_RUN);

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame == NULL);


    purc_variant_t msg_type = PURC_VARIANT_INVALID;
    purc_variant_t msg_sub_type = PURC_VARIANT_INVALID;
    const char *event = purc_variant_get_string_const(msg->eventName);
    if (!pcintr_parse_event(event, &msg_type, &msg_sub_type)) {
        return;
    }

    const char *msg_type_s = purc_variant_get_string_const(msg_type);
    PC_ASSERT(msg_type_s);

    const char *sub_type_s = NULL;
    if (msg_sub_type != PURC_VARIANT_INVALID) {
        sub_type_s = purc_variant_get_string_const(msg_sub_type);
    }

    purc_atom_t msg_type_atom = purc_atom_try_string_ex(ATOM_BUCKET_MSG,
            msg_type_s);
    PC_ASSERT(msg_type_atom);

#if 0
    purc_variant_t observed = msg->elementValue;

    bool handle = false;
    struct list_head* list = get_observer_list(stack, observed); {
        struct pcintr_observer *p, *n;
        list_for_each_entry_safe(p, n, list, node) {
            if (is_observer_match(p, observed, msg_type_atom, sub_type_s)) {
                handle = true;
                observer_matched(p, msg->data);
            }
        }
    }
#endif

#if 0
    if (!handle && purc_variant_is_native(observed)) {
        void *dest = purc_variant_native_get_entity(observed);
        // window close event dispatch to vdom
        if (dest == stack->vdom) {
            handle_vdom_event(stack, stack->vdom, msg_type_atom,
                    msg_sub_type, msg->data);
        }
    }
#endif

    pcinst_put_message(msg);
    if (msg_type) {
        purc_variant_unref(msg_type);
    }
    if (msg_sub_type) {
        purc_variant_unref(msg_sub_type);
    }

    PC_ASSERT(co->state == CO_STATE_RUN);
}
#endif

void
pcintr_check_and_dispatch_coroutine_event(pcintr_coroutine_t co)
{
    if (co->state == CO_STATE_WAIT) {
        return;
    }

    pcrdr_msg *msg = pcinst_msg_queue_get_msg(co->mq);
    if (msg == NULL) {
        return;
    }

    switch (msg->type) {
        case PCRDR_MSG_TYPE_VOID:
            // NOTE: not implemented yet
            PC_ASSERT(0);
            break;
        case PCRDR_MSG_TYPE_REQUEST:
            // NOTE: not implemented yet
            PC_ASSERT(0);
            break;
        case PCRDR_MSG_TYPE_RESPONSE:
            // NOTE: not implemented yet
            PC_ASSERT(0);
            break;
        case PCRDR_MSG_TYPE_EVENT:
            return process_rdr_msg_by_event(msg);
            break;
        default:
            // NOTE: shouldn't happen, no way to recover gracefully, fail-fast
            PC_ASSERT(0);
    }
}
