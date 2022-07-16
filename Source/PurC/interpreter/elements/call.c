/**
 * @file call.c
 * @author Xu Xiaohong
 * @date 2022/04/14
 * @brief
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
#include "private/instance.h"
#include "purc-runloop.h"

#include "../ops.h"

#include <pthread.h>
#include <unistd.h>

#define EVENT_SEPARATOR                 ':'

struct ctxt_for_call {
    struct pcvdom_node    *curr;

    purc_variant_t         on;
    purc_variant_t         with;
    purc_variant_t         within;
    char                   within_app_name[PURC_LEN_APP_NAME + 1];
    char                   within_runner_name[PURC_LEN_RUNNER_NAME + 1];

    purc_variant_t         as;
    const char            *s_as;

    purc_variant_t         at;
    const char            *s_at;

    pcvdom_element_t       define;

    char               endpoint_name_within[PURC_LEN_ENDPOINT_NAME + 1];
    purc_atom_t        endpoint_atom_within;

    unsigned int                  within_self:1;
    unsigned int                  concurrently:1;
    unsigned int                  synchronously:1;

    purc_variant_t         request_id;
};

static void
ctxt_for_call_destroy(struct ctxt_for_call *ctxt)
{
    if (ctxt) {
        PURC_VARIANT_SAFE_CLEAR(ctxt->on);
        PURC_VARIANT_SAFE_CLEAR(ctxt->with);
        PURC_VARIANT_SAFE_CLEAR(ctxt->within);
        PURC_VARIANT_SAFE_CLEAR(ctxt->as);
        PURC_VARIANT_SAFE_CLEAR(ctxt->at);
        PURC_VARIANT_SAFE_CLEAR(ctxt->request_id);
        if (ctxt->endpoint_atom_within) {
            PC_ASSERT(purc_atom_remove_string_ex(PURC_ATOM_BUCKET_USER,
                    ctxt->endpoint_name_within));
            ctxt->endpoint_atom_within = 0;
        }
        free(ctxt);
    }
}

static void
ctxt_destroy(void *ctxt)
{
    ctxt_for_call_destroy((struct ctxt_for_call*)ctxt);
}

static void
on_continuation(void *ud, pcrdr_msg *msg)
{
    pcintr_stack_frame_t frame = (pcintr_stack_frame_t)ud;
    PC_ASSERT(frame);

    const char *s = purc_variant_get_string_const(msg->eventName);
    const char *p = strchr(s, EVENT_SEPARATOR);
    const char *s_msg_sub_type = NULL;
    if (!p) {
        return;
    }

    s_msg_sub_type = p + 1;
    if (0 == strcmp(s_msg_sub_type, "success")) {
        purc_variant_t payload = msg->data;

        int r = pcintr_set_question_var(frame, payload);
        PC_ASSERT(r == 0);
        return;
    }

    if (0 == strcmp(s_msg_sub_type, "except")) {
        purc_variant_t payload = msg->data;

        const char *s = purc_variant_get_string_const(payload);
        purc_set_error_with_info(PURC_ERROR_UNKNOWN,
                "sub coroutine failed with except: %s", s);
        return;
    }

    PC_ASSERT(0);
}

static int
post_process(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(co);

    struct ctxt_for_call *ctxt;
    ctxt = (struct ctxt_for_call*)frame->ctxt;

    if (ctxt->on == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_ARGUMENT_MISSED,
                "lack of vdom attribute 'on' for element <%s>",
                frame->pos->tag_name);
        return -1;
    }

    pcvdom_element_t define = pcintr_get_vdom_from_variant(ctxt->on);
    if (define == NULL)
        return -1;

    if (0 && pcvdom_element_first_child_element(define) == NULL) {
        purc_set_error(PURC_ERROR_NO_DATA);
        return -1;
    }

    if (ctxt->with != PURC_VARIANT_INVALID) {
        int r = pcintr_set_question_var(frame, ctxt->with);
        if (r)
            return -1;
    }

    if (!ctxt->synchronously) {
        if (ctxt->as == PURC_VARIANT_INVALID) {
            purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                    "vdom attribute 'as' for element <call> undefined");
            return -1;
        }
        if (!purc_variant_is_string(ctxt->as)) {
            purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                    "vdom attribute 'as' for element <call> is not string");
            return -1;
        }
    }

    if (ctxt->within == PURC_VARIANT_INVALID) {
        ctxt->within_self = 1;
    }

    if (ctxt->within_self) {
        if (ctxt->concurrently == 0) {
            if (ctxt->synchronously) {
                ctxt->define = define;
                frame->scope = define;
                return 0;
            }
            purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                    "can not call directly in asynchronous mode");
            return -1;
        }

        pcintr_coroutine_t child;
        child = pcintr_create_child_co(define, ctxt->as, ctxt->within);
        if (!child)
            return -1;

        if (ctxt->synchronously) {
            ctxt->request_id = purc_variant_make_native(ctxt, NULL);
            pcintr_yield(frame, on_continuation, ctxt->request_id,
                    PURC_VARIANT_INVALID, PURC_VARIANT_INVALID, false);
            return 0;
        }
        PC_ASSERT(0); // Not implemented yet
    }

    PC_ASSERT(0); // Not implemented yet

    pcintr_coroutine_t child;
    child = pcintr_create_child_co(define, ctxt->as, ctxt->within);
    if (!child)
        return -1;

    if (ctxt->synchronously) {
        ctxt->request_id = purc_variant_make_native(ctxt, NULL);
        pcintr_yield(frame, on_continuation, ctxt->request_id ,
                PURC_VARIANT_INVALID, PURC_VARIANT_INVALID, false);
        return 0;
    }

    PC_ASSERT(0);
}

static int
process_attr_on(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_call *ctxt;
    ctxt = (struct ctxt_for_call*)frame->ctxt;
    if (ctxt->on != PURC_VARIANT_INVALID) {
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
    ctxt->on = val;
    purc_variant_ref(val);

    return 0;
}

static int
process_attr_with(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_call *ctxt;
    ctxt = (struct ctxt_for_call*)frame->ctxt;
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
    ctxt->with = val;
    purc_variant_ref(val);

    return 0;
}

static int
process_attr_within(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_call *ctxt;
    ctxt = (struct ctxt_for_call*)frame->ctxt;
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

    if (strcmp(s, "_self")) {
        int r;
        r = purc_extract_app_name(s, ctxt->within_app_name) &&
            purc_extract_runner_name(s, ctxt->within_runner_name);

        if (r == 0) {
            purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                    "vdom attribute '%s' for element <%s> is not valid",
                    purc_atom_to_string(name), element->tag_name);
            return -1;
        }

        PC_ASSERT(purc_is_valid_app_name(ctxt->within_app_name));
        PC_ASSERT(purc_is_valid_runner_name(ctxt->within_runner_name));
        ctxt->within_self = 0;
    }
    else {
        ctxt->within_self = 1;
    }

    PURC_VARIANT_SAFE_CLEAR(ctxt->within);
    ctxt->within = purc_variant_ref(val);

    return 0;
}

static int
process_attr_as(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_call *ctxt;
    ctxt = (struct ctxt_for_call*)frame->ctxt;
    if (ctxt->as != PURC_VARIANT_INVALID) {
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
    ctxt->as = purc_variant_ref(val);
    ctxt->s_as = purc_variant_get_string_const(ctxt->as);

    return 0;
}

static int
process_attr_at(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_call *ctxt;
    ctxt = (struct ctxt_for_call*)frame->ctxt;
    if (ctxt->at != PURC_VARIANT_INVALID) {
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
    ctxt->at = purc_variant_ref(val);
    ctxt->s_at = purc_variant_get_string_const(ctxt->at);

    return 0;
}

static int
attr_found_val(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val,
        struct pcvdom_attr *attr,
        struct ctxt_for_call *ctxt)
{
    PC_ASSERT(name);
    PC_ASSERT(attr->op == PCHVML_ATTRIBUTE_OPERATOR);

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, ON)) == name) {
        return process_attr_on(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, WITH)) == name) {
        return process_attr_with(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, WITHIN)) == name) {
        return process_attr_within(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, AS)) == name) {
        return process_attr_as(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, AT)) == name) {
        return process_attr_at(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, CONCURRENTLY)) == name) {
        ctxt->concurrently = 1;
        return 0;
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, SYNCHRONOUSLY)) == name) {
        ctxt->synchronously = 1;
        return 0;
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, SYNC)) == name) {
        ctxt->synchronously = 1;
        return 0;
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, ASYNCHRONOUSLY)) == name) {
        ctxt->synchronously = 0;
        return 0;
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, ASYNC)) == name) {
        ctxt->synchronously = 0;
        return 0;
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
    UNUSED_PARAM(ud);

    PC_ASSERT(name);
    PC_ASSERT(attr->op == PCHVML_ATTRIBUTE_OPERATOR);

    struct ctxt_for_call *ctxt;
    ctxt = (struct ctxt_for_call*)frame->ctxt;

    pcintr_stack_t stack = (pcintr_stack_t) ud;
    purc_variant_t val = pcintr_eval_vdom_attr(stack, attr);
    if (val == PURC_VARIANT_INVALID)
        return -1;

    int r = attr_found_val(frame, element, name, val, attr, ctxt);
    purc_variant_unref(val);

    return r ? -1 : 0;
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

    struct ctxt_for_call *ctxt;
    ctxt = (struct ctxt_for_call*)calloc(1, sizeof(*ctxt));
    if (!ctxt) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    ctxt->synchronously = 1;

    frame->ctxt = ctxt;
    frame->ctxt_destroy = ctxt_destroy;

    frame->pos = pos; // ATTENTION!!

    frame->attr_vars = purc_variant_make_object(0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    if (frame->attr_vars == PURC_VARIANT_INVALID)
        return ctxt;

    struct pcvdom_element *element = frame->pos;
    PC_ASSERT(element);

    int r;
    r = pcintr_vdom_walk_attrs(frame, element, stack, attr_found);
    if (r)
        return ctxt;

    r = post_process(stack->co, frame);
    if (r)
        return ctxt;

    return ctxt;
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

    struct ctxt_for_call *ctxt;
    ctxt = (struct ctxt_for_call*)frame->ctxt;
    if (ctxt) {
        ctxt_for_call_destroy(ctxt);
        frame->ctxt = NULL;
    }

    return true;
}

static void
on_element(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_element *element)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);
    UNUSED_PARAM(element);
}

static void
on_content(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_content *content)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);
    PC_ASSERT(content);
}

static void
on_comment(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_comment *comment)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);
    UNUSED_PARAM(comment);
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

    struct ctxt_for_call *ctxt;
    ctxt = (struct ctxt_for_call*)frame->ctxt;

    struct pcvdom_node *curr;

again:
    curr = ctxt->curr;

    if (curr == NULL) {
        struct pcvdom_element *element = frame->pos;
        if (ctxt->define)
            element = ctxt->define;

        struct pcvdom_node *node = &element->node;
        node = pcvdom_node_first_child(node);
        curr = node;
    }
    else {
        curr = pcvdom_node_next_sibling(curr);
    }

    ctxt->curr = curr;

    if (curr == NULL) {
        purc_clr_error();
        if (ctxt->define) {
            ctxt->define = NULL;
            goto again;
        }
        return NULL;
    }

    switch (curr->type) {
        case PCVDOM_NODE_DOCUMENT:
            PC_ASSERT(0); // Not implemented yet
            break;
        case PCVDOM_NODE_ELEMENT:
            {
                pcvdom_element_t element = PCVDOM_ELEMENT_FROM_NODE(curr);
                on_element(co, frame, element);
                return element;
            }
        case PCVDOM_NODE_CONTENT:
            on_content(co, frame, PCVDOM_CONTENT_FROM_NODE(curr));
            goto again;
        case PCVDOM_NODE_COMMENT:
            on_comment(co, frame, PCVDOM_COMMENT_FROM_NODE(curr));
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

struct pcintr_element_ops* pcintr_get_call_ops(void)
{
    return &ops;
}

