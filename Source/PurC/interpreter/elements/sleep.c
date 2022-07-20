/**
 * @file sleep.c
 * @author Xu Xiaohong
 * @date 2022/05/26
 * @brief The ops for <sleep>
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

#include "purc.h"

#include "../internal.h"

#include "private/debug.h"
#include "private/timer.h"

#include "../ops.h"

#include <errno.h>
#include <limits.h>

#define SLEEP_EVENT_HANDER  "_sleep_event_handler"

struct ctxt_for_sleep {
    struct pcvdom_node           *curr;
    purc_variant_t                with;
    purc_variant_t                v_for;

    int64_t                       for_ns;

    pcintr_timer_t                timer;
    pcintr_coroutine_t            co;
    purc_variant_t                element_value; // yield
    purc_variant_t                event_name;    // yield
};

static void
ctxt_for_sleep_destroy(struct ctxt_for_sleep *ctxt)
{
    if (ctxt) {
        PURC_VARIANT_SAFE_CLEAR(ctxt->with);
        PURC_VARIANT_SAFE_CLEAR(ctxt->v_for);
        if (ctxt->timer) {
            pcintr_timer_destroy(ctxt->timer);
            ctxt->timer = NULL;
        }
        PURC_VARIANT_SAFE_CLEAR(ctxt->element_value);
        PURC_VARIANT_SAFE_CLEAR(ctxt->event_name);

        free(ctxt);
    }
}

static void
ctxt_destroy(void *ctxt)
{
    ctxt_for_sleep_destroy((struct ctxt_for_sleep*)ctxt);
}

static int
process_attr_with(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_sleep *ctxt;
    ctxt = (struct ctxt_for_sleep*)frame->ctxt;
    if (ctxt->with != PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_DUPLICATED,
                "vdom attribute '%s' for element <%s>",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    if (val == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> undefined",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    bool force = true;
    int64_t secs;
    if (!purc_variant_cast_to_longint(val, &secs, force)) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> is not longint",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    if (secs <= 0) {
        secs = 0;
    }

    ctxt->with = purc_variant_ref(val);
    ctxt->for_ns = secs * 1000 * 1000 * 1000;

    return 0;
}

static int
process_attr_for(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_sleep *ctxt;
    ctxt = (struct ctxt_for_sleep*)frame->ctxt;
    if (val == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> undefined",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    if (!purc_variant_is_string(val)) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> is not string",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }

    const char *s = purc_variant_get_string_const(val);
    if (!s || !*s) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> is empty string",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    long int n;
    char *end;
    errno = 0;
    n = strtol(s, &end, 0);
    if (n == LONG_MIN || n == LONG_MAX) {
        if (errno == ERANGE) {
            purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                    "vdom attribute '%s' for element <%s> "
                    "is overflow/underflow",
                    purc_atom_to_string(name), element->tag_name);
            return -1;
        }
    }
    if (n < 0)
        n = 0;

    if (!end) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> no unit specified",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }

    if (strcmp(end, "ns") == 0) {
        ctxt->for_ns = n;
    }
    else if (strcmp(end, "us") == 0) {
        ctxt->for_ns = n * 1000;
    }
    else if (strcmp(end, "ms") == 0) {
        ctxt->for_ns = n * 1000 * 1000;
    }
    else if (strcmp(end, "s") == 0) {
        ctxt->for_ns = n * 1000 * 1000 * 1000;
    }
    else if (strcmp(end, "m") == 0) {
        ctxt->for_ns = n * 1000 * 1000 * 1000 * 60;
    }
    else if (strcmp(end, "h") == 0) {
        ctxt->for_ns = n * 1000 * 1000 * 1000 * 60 * 60;
    }
    else if (strcmp(end, "d") == 0) {
        ctxt->for_ns = n * 1000 * 1000 * 1000 * 60 * 60 * 24;
    }
    else {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> unknown unit",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }

    ctxt->v_for = purc_variant_ref(val);

    return 0;
}

static int
attr_found_val(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val,
        struct pcvdom_attr *attr,
        void *ud)
{
    UNUSED_PARAM(ud);

    PC_ASSERT(name);
    PC_ASSERT(attr->op == PCHVML_ATTRIBUTE_OPERATOR);

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, WITH)) == name) {
        return process_attr_with(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, FOR)) == name) {
        return process_attr_for(frame, element, name, val);
    }

    purc_set_error_with_info(PURC_ERROR_NOT_IMPLEMENTED,
            "vdom attribute '%s' for element <%s>",
            purc_atom_to_string(name), element->tag_name);

    return -1;
}

static int
attr_found(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name,
        struct pcvdom_attr *attr,
        void *ud)
{
    PC_ASSERT(name);
    PC_ASSERT(attr->op == PCHVML_ATTRIBUTE_OPERATOR);

    pcintr_stack_t stack = (pcintr_stack_t) ud;
    purc_variant_t val = pcintr_eval_vdom_attr(stack, attr);
    if (val == PURC_VARIANT_INVALID)
        return -1;

    int r = attr_found_val(frame, element, name, val, attr, ud);
    purc_variant_unref(val);

    return r ? -1 : 0;
}

static void on_continuation(void *ud, pcrdr_msg *msg)
{
    UNUSED_PARAM(msg);

    struct pcintr_stack_frame *frame;
    frame = (struct pcintr_stack_frame*)ud;
    PC_ASSERT(frame);

    pcintr_coroutine_t co = pcintr_get_coroutine();
    PC_ASSERT(co);
    PC_ASSERT(co->state == CO_STATE_RUNNING);
    pcintr_stack_t stack = &co->stack;
    PC_ASSERT(frame == pcintr_stack_get_bottom_frame(stack));

    struct ctxt_for_sleep *ctxt;
    ctxt = (struct ctxt_for_sleep*)frame->ctxt;
    PC_ASSERT(ctxt);
    PC_ASSERT(ctxt->timer);

    // NOTE: not interrupted
    purc_variant_t result = purc_variant_make_ulongint(0);
    if (result != PURC_VARIANT_INVALID) {
        pcintr_set_question_var(frame, result);
        purc_variant_unref(result);
    }
}

static void on_sleep_timeout(pcintr_timer_t timer, const char *id, void *data)
{
    UNUSED_PARAM(timer);
    UNUSED_PARAM(id);
    struct ctxt_for_sleep *ctxt = data;
    if (ctxt->co->stack.exited) {
        return;
    }

    pcintr_coroutine_post_event(ctxt->co->cid,
        PCRDR_MSG_EVENT_REDUCE_OPT_KEEP,
        ctxt->element_value,
        MSG_TYPE_SLEEP, MSG_SUB_TYPE_TIMEOUT,
        PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
}

static bool
is_sleep_event_handler_match(struct pcintr_event_handler *handler,
        pcintr_coroutine_t co, pcrdr_msg *msg, bool *observed)
{
    UNUSED_PARAM(co);

    struct ctxt_for_sleep *ctxt = handler->data;
    if (msg->requestId == PURC_VARIANT_INVALID &&
            purc_variant_is_equal_to(msg->elementValue, ctxt->element_value) &&
            purc_variant_is_equal_to(msg->eventName, ctxt->event_name)) {
        *observed = true;
        return true;
    }

    return false;
}

static int sleep_event_handle(struct pcintr_event_handler *handler,
        pcintr_coroutine_t co, pcrdr_msg *msg, bool *remove_handler,
        bool *performed)
{
    UNUSED_PARAM(handler);

    *performed = false;
    *remove_handler = false;
    int ret = PURC_ERROR_INCOMPLETED;
    struct ctxt_for_sleep *ctxt = handler->data;
    if (msg->requestId == PURC_VARIANT_INVALID &&
            purc_variant_is_equal_to(msg->elementValue, ctxt->element_value) &&
            purc_variant_is_equal_to(msg->eventName, ctxt->event_name)) {
        // sleep timeout return PURC_ERROR_OK to clear msg
        pcintr_set_current_co(co);
        pcintr_resume(co, NULL);
        pcintr_set_current_co(NULL);
        *remove_handler = true;
        *performed = true;
        ret = PURC_ERROR_OK;
    }
    else {
        //TODO: wake up by other event return INCOMPLETED to keep msg for other handler
    }

    return ret;
}

static void*
after_pushed(pcintr_stack_t stack, pcvdom_element_t pos)
{
    PC_ASSERT(stack && pos);

    if (stack->except)
        return NULL;

    pcintr_check_insertion_mode_for_normal_element(stack);

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    struct ctxt_for_sleep *ctxt;
    ctxt = (struct ctxt_for_sleep*)calloc(1, sizeof(*ctxt));
    if (!ctxt) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    frame->ctxt = ctxt;
    frame->ctxt_destroy = ctxt_destroy;

    frame->pos = pos; // ATTENTION!!

    struct pcvdom_element *element = frame->pos;
    PC_ASSERT(element);

    int r;
    r = pcintr_vdom_walk_attrs(frame, element, stack, attr_found);
    if (r)
        return ctxt;

    if (ctxt->for_ns < 1 * 1000 * 1000) {
        // less than 1ms
        // FIXME: nanosleep or round-up to 1 ms?
        ctxt->for_ns = 1 * 1000 * 1000;
    }


    ctxt->element_value = purc_variant_make_native(frame, NULL);
    if (!ctxt->element_value) {
        return ctxt;
    }

    char *event =
        malloc(strlen(MSG_TYPE_SLEEP) + strlen(MSG_SUB_TYPE_TIMEOUT) + 2);
    if (!event) {
        return ctxt;
    }
    sprintf(event, "%s:%s", MSG_TYPE_SLEEP, MSG_SUB_TYPE_TIMEOUT);
    ctxt->event_name = purc_variant_make_string_reuse_buff(event,
            strlen(event), false);
    if (!ctxt->event_name) {
        free(event);
        return ctxt;
    }

    ctxt->co = stack->co;
    ctxt->timer = pcintr_timer_create(NULL, NULL, on_sleep_timeout, ctxt);
    if (!ctxt->timer)
        return ctxt;

    pcintr_timer_set_interval(ctxt->timer, ctxt->for_ns / (1000 * 1000));
    pcintr_timer_start_oneshot(ctxt->timer);

    PC_ASSERT(ctxt->co->sleep_handler == NULL);
    ctxt->co->sleep_handler = pcintr_event_handler_create(
            SLEEP_EVENT_HANDER,
            CO_STAGE_FIRST_RUN | CO_STAGE_OBSERVING, CO_STATE_STOPPED,
            ctxt, sleep_event_handle, is_sleep_event_handler_match, false);

    pcintr_yield(frame, on_continuation, PURC_VARIANT_INVALID,
            ctxt->element_value, ctxt->event_name, true);

    purc_clr_error();

    // NOTE: no element to process if succeeds
    return NULL;
}

static bool
on_popping(pcintr_stack_t stack, void* ud)
{
    PC_ASSERT(stack);

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame);
    PC_ASSERT(ud == frame->ctxt);

    if (frame->ctxt == NULL)
        return true;

    struct pcvdom_element *element = frame->pos;
    PC_ASSERT(element);

    struct ctxt_for_sleep *ctxt;
    ctxt = (struct ctxt_for_sleep*)frame->ctxt;
    if (ctxt) {
        ctxt_for_sleep_destroy(ctxt);
        frame->ctxt = NULL;
    }

    return true;
}

static int
on_element(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_element *element)
{
    UNUSED_PARAM(frame);
    UNUSED_PARAM(element);

    pcintr_stack_t stack = &co->stack;

    if (stack->except)
        return 0;

    PC_ASSERT(0);
}

static int
on_content(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_content *content)
{
    UNUSED_PARAM(frame);
    PC_ASSERT(content);

    pcintr_stack_t stack = &co->stack;

    if (stack->except)
        return 0;

    PC_ASSERT(0);
}

static int
on_comment(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_comment *comment)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);
    PC_ASSERT(comment);
    return 0;
}

static int
on_child_finished(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(frame);

    pcintr_stack_t stack = &co->stack;

    if (stack->except)
        return 0;

    return 0;
}

static pcvdom_element_t
select_child(pcintr_stack_t stack, void* ud)
{
    PC_ASSERT(stack);

    pcintr_coroutine_t co = stack->co;
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(ud == frame->ctxt);

    if (stack->back_anchor == frame)
        stack->back_anchor = NULL;

    if (frame->ctxt == NULL)
        return NULL;

    if (stack->back_anchor)
        return NULL;

    struct ctxt_for_sleep *ctxt;
    ctxt = (struct ctxt_for_sleep*)frame->ctxt;

    struct pcvdom_node *curr;

again:
    curr = ctxt->curr;

    if (curr == NULL) {
        struct pcvdom_element *element = frame->pos;
        struct pcvdom_node *node = &element->node;
        node = pcvdom_node_first_child(node);
        curr = node;
        purc_clr_error();
    }
    else {
        curr = pcvdom_node_next_sibling(curr);
        purc_clr_error();
    }

    ctxt->curr = curr;

    if (curr == NULL) {
        on_child_finished(co, frame);
        return NULL;
    }

    switch (curr->type) {
        case PCVDOM_NODE_DOCUMENT:
            PC_ASSERT(0); // Not implemented yet
            break;
        case PCVDOM_NODE_ELEMENT:
            {
                pcvdom_element_t element = PCVDOM_ELEMENT_FROM_NODE(curr);
                if (on_element(co, frame, element))
                    return NULL;
                return element;
            }
        case PCVDOM_NODE_CONTENT:
            if (on_content(co, frame, PCVDOM_CONTENT_FROM_NODE(curr)))
                return NULL;
            goto again;
        case PCVDOM_NODE_COMMENT:
            if (on_comment(co, frame, PCVDOM_COMMENT_FROM_NODE(curr)))
                return NULL;
            goto again;
        default:
            PC_ASSERT(0); // Not implemented yet
    }

    PC_ASSERT(0);
    return NULL; // NOTE: never reached here!!!
}

static struct pcintr_element_ops
ops = {
    .after_pushed       = after_pushed,
    .on_popping         = on_popping,
    .rerun              = NULL,
    .select_child       = select_child,
};

struct pcintr_element_ops* pcintr_get_sleep_ops(void)
{
    return &ops;
}

