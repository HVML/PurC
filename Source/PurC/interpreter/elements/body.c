/**
 * @file body.c
 * @author Xu Xiaohong
 * @date 2021/12/06
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
#include "purc-runloop.h"

#include "../ops.h"

#include <pthread.h>
#include <unistd.h>

struct ctxt_for_body {
    struct pcvdom_node           *curr;
};

static void
ctxt_for_body_destroy(struct ctxt_for_body *ctxt)
{
    if (ctxt) {
        free(ctxt);
    }
}

static void
ctxt_destroy(void *ctxt)
{
    ctxt_for_body_destroy((struct ctxt_for_body*)ctxt);
}

static int
attr_found(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name,
        purc_variant_t val,
        struct pcvdom_attr *attr,
        void *ud)
{
    UNUSED_PARAM(frame);
    UNUSED_PARAM(element);
    UNUSED_PARAM(name);
    UNUSED_PARAM(val);
    UNUSED_PARAM(ud);

    if (pcintr_is_hvml_attr(attr->key)) {
        return 0;
    }

    pcintr_stack_t stack = (pcintr_stack_t) ud;
    int r = pcintr_set_edom_attribute(stack, attr, val);

    return r ? -1 : 0;
}

static void*
after_pushed(pcintr_stack_t stack, pcvdom_element_t pos)
{
    switch (stack->mode) {
        case STACK_VDOM_BEFORE_HVML:
            purc_set_error(PURC_ERROR_NOT_SUPPORTED);
            break;
        case STACK_VDOM_BEFORE_HEAD:
            stack->mode = STACK_VDOM_IN_BODY;
            break;
        case STACK_VDOM_IN_HEAD:
            purc_set_error(PURC_ERROR_NOT_SUPPORTED);
            break;
        case STACK_VDOM_AFTER_HEAD:
            stack->mode = STACK_VDOM_IN_BODY;
            break;
        case STACK_VDOM_IN_BODY:
            purc_set_error(PURC_ERROR_NOT_SUPPORTED);
            break;
        case STACK_VDOM_AFTER_BODY:
            purc_set_error(PURC_ERROR_NOT_SUPPORTED);
            break;
        case STACK_VDOM_AFTER_HVML:
            purc_set_error(PURC_ERROR_NOT_SUPPORTED);
            break;
        default:
            purc_set_error(PURC_ERROR_NOT_SUPPORTED);
            break;
    }

    if (stack->except)
        return NULL;

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    struct ctxt_for_body *ctxt = frame->ctxt;
    if (!ctxt) {
        ctxt = (struct ctxt_for_body*)calloc(1, sizeof(*ctxt));
        if (!ctxt) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            return NULL;
        }

        frame->ctxt = ctxt;
        frame->ctxt_destroy = ctxt_destroy;

        frame->pos = pos; // ATTENTION!!
    }

    if (0 != pcintr_stack_frame_eval_attr_and_content(stack, frame, true)) {
        return NULL;
    }

    frame->edom_element = purc_document_body(stack->doc);
    int r;
    r = pcintr_refresh_at_var(frame);
    if (r)
        return ctxt;

    struct pcvdom_element *element = frame->pos;

    r = pcintr_walk_attrs(frame, element, stack, attr_found);
    if (r)
        return ctxt;

    purc_clr_error();

    return ctxt;
}

static bool
on_popping(pcintr_stack_t stack, void* ud)
{
    UNUSED_PARAM(ud);
    stack->mode = STACK_VDOM_AFTER_BODY;

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    if (frame->ctxt == NULL)
        return true;

    struct ctxt_for_body *ctxt;
    ctxt = (struct ctxt_for_body*)frame->ctxt;
    if (ctxt) {
        ctxt_for_body_destroy(ctxt);
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

static int
on_content(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_content *content)
{
    int err = 0;
    struct pcvcm_node *vcm = content->vcm;
    if (!vcm) {
        goto out;
    }

    pcintr_stack_t stack = &co->stack;
    purc_variant_t v = pcintr_eval_vcm(stack, vcm, frame->silently);
    if (v == PURC_VARIANT_INVALID) {
        err = purc_get_last_error();
        goto out;
    }

    pcintr_set_question_var(frame, v);
    if (purc_variant_is_string(v)) {
        size_t sz;
        const char *text = purc_variant_get_string_const_ex(v, &sz);
        if (sz > 0) {
            pcintr_util_new_text_content(frame->owner->doc,
                    frame->edom_element, PCDOC_OP_APPEND, text, sz,
                    !stack->inherit);
        }
    }
    purc_variant_unref(v);

#if 0 // VW
    if (purc_variant_is_string(v)) {
        size_t sz;
        const char *text = purc_variant_get_string_const_ex(v, &sz);
        pcdom_text_t *content;
        content = pcintr_util_append_content(frame->edom_element, text);
        purc_variant_unref(v);
    }
    else {
        char *sv = pcvariant_to_string(v);
        int r;
        r = pcintr_util_add_child_chunk(frame->edom_element, sv);
        free(sv);
        purc_variant_unref(v);
    }
#endif
out:
    return err;
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
    UNUSED_PARAM(ud);
    pcintr_coroutine_t co = stack->co;
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    if (stack->back_anchor == frame)
        stack->back_anchor = NULL;

    if (frame->ctxt == NULL)
        return NULL;

    if (stack->back_anchor)
        return NULL;

    struct ctxt_for_body *ctxt;
    ctxt = (struct ctxt_for_body*)frame->ctxt;

    struct pcvdom_node *curr;

again:
    curr = ctxt->curr;

    if (curr == NULL) {
        struct pcvdom_element *element;
        if (co->stack.entry == NULL) {
            element = frame->pos;
        }
        else {
            element = co->stack.entry;
        }
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
        return NULL;
    }

    switch (curr->type) {
        case PCVDOM_NODE_DOCUMENT:
            purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
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
            purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
    }

    purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    return NULL; // NOTE: never reached here!!!
}

static struct pcintr_element_ops
ops = {
    .after_pushed       = after_pushed,
    .on_popping         = on_popping,
    .rerun              = NULL,
    .select_child       = select_child,
};

struct pcintr_element_ops* pcintr_get_body_ops(void)
{
    return &ops;
}

