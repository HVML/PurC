/**
 * @file hvml.c
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

#define ATTR_KEY_ID         "id"

struct ctxt_for_hvml {
    struct pcvdom_node           *curr;
    pcvdom_element_t              body;
};

static void
ctxt_for_hvml_destroy(struct ctxt_for_hvml *ctxt)
{
    if (ctxt) {
        free(ctxt);
    }
}

static void
ctxt_destroy(void *ctxt)
{
    ctxt_for_hvml_destroy((struct ctxt_for_hvml*)ctxt);
}

static int
attr_found_val(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val,
        struct pcvdom_attr *attr,
        void *ud)
{
    UNUSED_PARAM(element);
    UNUSED_PARAM(name);
    UNUSED_PARAM(ud);

    PC_ASSERT(attr->op == PCHVML_ATTRIBUTE_OPERATOR);
    PC_ASSERT(attr->key);
    PC_ASSERT(purc_variant_is_string(val));
    const char *sv = purc_variant_get_string_const(val);
    PC_ASSERT(sv);

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, TARGET)) == name) {
        pcintr_stack_t stack = (pcintr_stack_t) ud;
        if (stack->co->target) {
            char *target = strdup(sv);
            if (!target) {
                purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
                return -1;
            }
            free(stack->co->target);
            stack->co->target = target;
        }
    }
    else {
        /* VW: only set attributes other than `target` to
           the root element of eDOM */
        int r = pcintr_util_set_attribute(frame->owner->doc,
                frame->edom_element, PCDOC_OP_DISPLACE, attr->key, sv, 0);
        PC_ASSERT(r == 0);
    }

    return 0;
}

static int
attr_found(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name,
        struct pcvdom_attr *attr,
        void *ud)
{
    PC_ASSERT(attr->op == PCHVML_ATTRIBUTE_OPERATOR);

    pcintr_stack_t stack = (pcintr_stack_t) ud;
    purc_variant_t val = pcintr_eval_vdom_attr(stack, attr);
    if (val == PURC_VARIANT_INVALID)
        return -1;

    int r = attr_found_val(frame, element, name, val, attr, ud);
    purc_variant_unref(val);

    return r ? -1 : 0;
}

static bool
is_match_body_id(pcintr_stack_t stack, struct pcvdom_element *element)
{
    bool match = false;
    if (!stack->body_id) {
        match = true;
        goto ret;
    }

    purc_variant_t elem_id = pcvdom_element_eval_attr_val(stack, element,
            ATTR_KEY_ID);
    if (!elem_id || !purc_variant_is_string(elem_id)) {
        goto out;
    }

    const char *eid = purc_variant_get_string_const(elem_id);
    if (strcmp(eid, stack->body_id) == 0) {
        match = true;
    }

out:
    if (elem_id) {
        purc_variant_unref(elem_id);
    }

ret:
    return match;
}

static pcvdom_element_t
find_body(pcintr_stack_t stack)
{
    purc_vdom_t vdom = stack->vdom;
    struct pcvdom_element *ret = NULL;
    size_t nr = pcutils_arrlist_length(vdom->bodies);
    if (nr == 0) {
        goto out;
    }

    for (size_t i = 0; i < nr; i++) {
        void *p = pcutils_arrlist_get_idx(vdom->bodies, i);
        struct pcvdom_element *body = (struct pcvdom_element*)p;
        if (is_match_body_id(stack, body)) {
            ret = body;
            goto out;
        }
    }

    ret = pcutils_arrlist_get_idx(vdom->bodies, 0);
out:
    return ret;
}

static void*
after_pushed(pcintr_stack_t stack, pcvdom_element_t pos)
{
    PC_ASSERT(stack && pos);
    PC_ASSERT(stack->mode == STACK_VDOM_BEFORE_HVML);
    stack->mode = STACK_VDOM_BEFORE_HEAD;

    if (stack->except)
        return NULL;

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame);

    struct ctxt_for_hvml *ctxt;
    ctxt = (struct ctxt_for_hvml*)calloc(1, sizeof(*ctxt));
    if (!ctxt) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    frame->ctxt = ctxt;
    frame->ctxt_destroy = ctxt_destroy;

    frame->pos = pos; // ATTENTION!!
    frame->edom_element = purc_document_root(stack->doc);
    int r;
    r = pcintr_refresh_at_var(frame);
    if (r)
        return ctxt;

    struct pcvdom_element *element = frame->pos;
    PC_ASSERT(element);

    r = pcintr_vdom_walk_attrs(frame, element, stack, attr_found);
    if (r)
        return ctxt;

    pcintr_calc_and_set_caret_symbol(stack, frame);

    ctxt->body = find_body(stack);

    purc_clr_error();

    return ctxt;
}

static bool
on_popping(pcintr_stack_t stack, void* ud)
{
    PC_ASSERT(stack);
    switch (stack->mode) {
        case STACK_VDOM_BEFORE_HVML:
            PC_ASSERT(0);
            break;
        case STACK_VDOM_BEFORE_HEAD:
            stack->mode = STACK_VDOM_AFTER_HVML;
            break;
        case STACK_VDOM_IN_HEAD:
            PC_ASSERT(0);
            break;
        case STACK_VDOM_AFTER_HEAD:
            stack->mode = STACK_VDOM_AFTER_HVML;
            break;
        case STACK_VDOM_IN_BODY:
            //PC_ASSERT(0);
            break;
        case STACK_VDOM_AFTER_BODY:
            stack->mode = STACK_VDOM_AFTER_HVML;
            break;
        case STACK_VDOM_AFTER_HVML:
            PC_ASSERT(0);
            break;
        default:
            PC_ASSERT(0);
            break;
    }

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame);
    PC_ASSERT(ud == frame->ctxt);

    if (frame->ctxt == NULL)
        return true;

    struct pcvdom_element *element = frame->pos;
    PC_ASSERT(element);

    struct ctxt_for_hvml *ctxt;
    ctxt = (struct ctxt_for_hvml*)frame->ctxt;
    if (ctxt) {
        ctxt_for_hvml_destroy(ctxt);
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

    struct pcvcm_node *vcm = content->vcm;
    if (!vcm) {
        return;
    }

    purc_variant_t v = pcvcm_eval(vcm, &co->stack, frame->silently);
    if (v == PURC_VARIANT_INVALID) {
        return;
    }
    pcintr_set_question_var(frame, v);
    purc_variant_unref(v);
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

    struct ctxt_for_hvml *ctxt;
    ctxt = (struct ctxt_for_hvml*)frame->ctxt;

    struct pcvdom_node *curr;

again:
    curr = ctxt->curr;

    if (curr == NULL) {
        struct pcvdom_element *element = frame->pos;
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
            PC_ASSERT(0); // Not implemented yet
            break;
        case PCVDOM_NODE_ELEMENT:
            {
                pcvdom_element_t element = PCVDOM_ELEMENT_FROM_NODE(curr);
                on_element(co, frame, element);
                enum pchvml_tag_id tag_id = element->tag_id;
                if (tag_id != PCHVML_TAG_BODY) {
                    return element;
                }
                else if (stack->mode == STACK_VDOM_AFTER_BODY) {
                    goto again;
                }
                else if (element == ctxt->body) {
                    return element;
                }
                goto again;
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

struct pcintr_element_ops* pcintr_get_hvml_ops(void)
{
    return &ops;
}

