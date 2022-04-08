/**
 * @file bind.c
 * @author Xue Shuming
 * @date 2022/04/02
 * @brief The ops for <bind>
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

#include "internal.h"

#include "private/debug.h"
#include "purc-runloop.h"

#include "ops.h"

#include <pthread.h>
#include <unistd.h>

struct ctxt_for_bind {
    struct pcvdom_node           *curr;
    struct pcvcm_node            *vcm_ev;
    purc_variant_t                as;

    unsigned int                  under_head:1;
    unsigned int                  locally:1;
};

static void
ctxt_for_bind_destroy(struct ctxt_for_bind *ctxt)
{
    if (ctxt) {
        PURC_VARIANT_SAFE_CLEAR(ctxt->as);
        free(ctxt);
    }
}

static void
ctxt_destroy(void *ctxt)
{
    ctxt_for_bind_destroy((struct ctxt_for_bind*)ctxt);
}

static int
post_process(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);
    struct ctxt_for_bind *ctxt;
    ctxt = (struct ctxt_for_bind*)frame->ctxt;

    purc_variant_t val = pcvcm_to_expression_variable(ctxt->vcm_ev, false);
    if (val == PURC_VARIANT_INVALID) {
        return -1;
    }

    bool ok = false;
    purc_variant_t name = ctxt->as;
    if (ctxt->locally) {
        struct pcintr_stack_frame *parent = pcintr_stack_frame_get_parent(frame);
        PC_ASSERT(parent);
        purc_variant_t exclamation_var;
        exclamation_var = pcintr_get_exclamation_var(parent);
        PC_ASSERT(exclamation_var != PURC_VARIANT_INVALID);
        if (purc_variant_is_object(exclamation_var)) {
            ok = purc_variant_object_set(exclamation_var, name, val);
        }
    }
    else {
        struct pcvdom_element *element = frame->scope;
        const char *s_name = purc_variant_get_string_const(name);
        if (ctxt->under_head) {
            ok = purc_bind_document_variable(co->stack->vdom, s_name, val);
        } else {
            element = pcvdom_element_parent(element);
            PC_ASSERT(element);
            ok = pcintr_bind_scope_variable(element, s_name, val);
        }
    }

    purc_variant_unref(val);
    if (ok) {
        purc_clr_error();
        return 0;
    }
    return -1;
}

static int
process_attr_as(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_bind *ctxt;
    ctxt = (struct ctxt_for_bind*)frame->ctxt;
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
    ctxt->as = val;
    purc_variant_ref(val);

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

    struct ctxt_for_bind *ctxt;
    ctxt = (struct ctxt_for_bind*)frame->ctxt;

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, ON)) == name) {
        ctxt->vcm_ev = attr->val;
        return 0;
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, AS)) == name) {
        return process_attr_as(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, LOCALLY)) == name) {
        PC_ASSERT(purc_variant_is_undefined(val));
        ctxt->locally = 1;
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
    PC_ASSERT(name);
    PC_ASSERT(attr->op == PCHVML_ATTRIBUTE_OPERATOR);

    purc_variant_t val = pcintr_eval_vdom_attr(pcintr_get_stack(), attr);
    if (val == PURC_VARIANT_INVALID) {
        return -1;
    }

    int r = attr_found_val(frame, element, name, val, attr, ud);
    purc_variant_unref(val);

    return r ? -1 : 0;
}

static void*
after_pushed(pcintr_stack_t stack, pcvdom_element_t pos)
{
    PC_ASSERT(stack && pos);
    PC_ASSERT(stack == pcintr_get_stack());

    if (stack->except)
        return NULL;

    if (pcintr_check_insertion_mode_for_normal_element(stack))
        return NULL;

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    struct ctxt_for_bind *ctxt;
    ctxt = (struct ctxt_for_bind*)calloc(1, sizeof(*ctxt));
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
    r = pcintr_vdom_walk_attrs(frame, element, NULL, attr_found);
    if (r)
        return NULL;


    if (ctxt->as == PURC_VARIANT_INVALID) {
        return NULL;
    }

    while ((element=pcvdom_element_parent(element))) {
        if (element->tag_id == PCHVML_TAG_HEAD) {
            ctxt->under_head = 1;
        }
    }

    purc_clr_error();

    return ctxt;
}

static bool
on_popping(pcintr_stack_t stack, void* ud)
{
    PC_ASSERT(stack);
    PC_ASSERT(stack == pcintr_get_stack());

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame);
    PC_ASSERT(ud == frame->ctxt);

    if (frame->ctxt == NULL)
        return true;

    struct pcvdom_element *element = frame->pos;
    PC_ASSERT(element);

    struct ctxt_for_bind *ctxt;
    ctxt = (struct ctxt_for_bind*)frame->ctxt;
    if (ctxt) {
        ctxt_for_bind_destroy(ctxt);
        frame->ctxt = NULL;
    }

    return true;
}

static int
on_element(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_element *element)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);
    UNUSED_PARAM(element);

    purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
            "no element is permitted ");
    return -1;

}

static int
on_content(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_content *content)
{
    UNUSED_PARAM(co);
    PC_ASSERT(content);

    struct ctxt_for_bind *ctxt;
    ctxt = (struct ctxt_for_bind*)frame->ctxt;
    PC_ASSERT(ctxt);

    struct pcvcm_node *vcm = content->vcm;
    if (!vcm)
        return 0;

    if (ctxt->vcm_ev) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "no content is permitted "
                "since there's no `on` attribute");
        return -1;
    }

    ctxt->vcm_ev = vcm;
    return 0;
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
    struct ctxt_for_bind *ctxt;
    ctxt = (struct ctxt_for_bind*)frame->ctxt;
    PC_ASSERT(ctxt);

    if (ctxt->vcm_ev) {
        return post_process(co, frame);
    }
    return -1;
}

static pcvdom_element_t
select_child(pcintr_stack_t stack, void* ud)
{
    PC_ASSERT(stack);
    PC_ASSERT(stack == pcintr_get_stack());

    pcintr_coroutine_t co = &stack->co;
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(ud == frame->ctxt);

    if (stack->back_anchor == frame)
        stack->back_anchor = NULL;

    if (frame->ctxt == NULL)
        return NULL;

    if (stack->back_anchor)
        return NULL;

    struct ctxt_for_bind *ctxt;
    ctxt = (struct ctxt_for_bind*)frame->ctxt;

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
        PC_ASSERT(0 == on_child_finished(co, frame));
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

struct pcintr_element_ops* pcintr_get_bind_ops(void)
{
    return &ops;
}

