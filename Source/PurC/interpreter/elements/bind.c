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

#include "../internal.h"

#include "private/debug.h"
#include "purc-runloop.h"

#include "../ops.h"

#include <pthread.h>
#include <unistd.h>

#define ATTR_ON         "on"

struct ctxt_for_bind {
    struct pcvdom_node           *curr;
    struct pcvcm_node            *vcm_ev;

    purc_variant_t                as;
    purc_variant_t                at;
    purc_variant_t                against;

    unsigned int                  under_head:1;
    unsigned int                  temporarily:1;
    unsigned int                  constantly:1;
};

static void
ctxt_for_bind_destroy(struct ctxt_for_bind *ctxt)
{
    if (ctxt) {
        PURC_VARIANT_SAFE_CLEAR(ctxt->as);
        PURC_VARIANT_SAFE_CLEAR(ctxt->at);
        PURC_VARIANT_SAFE_CLEAR(ctxt->against);
        free(ctxt);
    }
}

static void
ctxt_destroy(void *ctxt)
{
    ctxt_for_bind_destroy((struct ctxt_for_bind*)ctxt);
}

#define UNDEFINED       "undefined"

static const char*
get_name(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(co);

    struct ctxt_for_bind *ctxt;
    ctxt = (struct ctxt_for_bind*)frame->ctxt;

    purc_variant_t name = ctxt->as;

    if (name == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    if (purc_variant_is_string(name) == false) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    const char *s_name = purc_variant_get_string_const(name);
    return s_name;
}


static int
post_process_bind_at_frame(pcintr_coroutine_t co, struct ctxt_for_bind *ctxt,
        struct pcintr_stack_frame *frame, purc_variant_t val)
{
    UNUSED_PARAM(co);

    purc_variant_t name = ctxt->as;

    if (name == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    if (purc_variant_is_string(name) == false) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    purc_variant_t exclamation_var;
    exclamation_var = pcintr_get_exclamation_var(frame);
    if (purc_variant_is_object(exclamation_var) == false) {
        purc_set_error_with_info(PURC_ERROR_INTERNAL_FAILURE,
                "temporary variable on stack frame is not object");
        return -1;
    }

    bool ok = purc_variant_object_set(exclamation_var, name, val);
    if (ok)
        purc_clr_error();
    return ok ? 0 : -1;
}

static int
post_process_bind_at_vdom(pcintr_coroutine_t co,
        struct pcintr_stack_frame *frame,
        struct pcvdom_element *elem, purc_variant_t val)
{
    UNUSED_PARAM(co);

    const char *s_name = get_name(co, frame);
    if (!s_name)
        return -1;

    bool ok;
    ok = pcintr_bind_scope_variable(co, elem, s_name, val);
    return ok ? 0 : -1;
}

int
post_process_val_by_level(pcintr_coroutine_t co,
        struct pcintr_stack_frame *frame, purc_variant_t val, uint64_t level)
{
    PC_ASSERT(level > 0);

    struct ctxt_for_bind *ctxt;
    ctxt = (struct ctxt_for_bind*)frame->ctxt;

    bool silently = frame->silently;

    if (ctxt->temporarily) {
        struct pcintr_stack_frame *p = frame;
        struct pcintr_stack_frame *parent;
        parent = pcintr_stack_frame_get_parent(frame);
        if (parent == NULL) {
            purc_set_error_with_info(PURC_ERROR_ENTITY_NOT_FOUND,
                    "no frame exists");
            return -1;
        }
        for (uint64_t i=0; i<level; ++i) {
            if (p == NULL)
                break;
            p = pcintr_stack_frame_get_parent(p);
        }
        if (p == NULL) {
            if (!silently) {
                purc_set_error_with_info(PURC_ERROR_ENTITY_NOT_FOUND,
                        "no frame exists");
                return -1;
            }
            p = parent;
        }
        return post_process_bind_at_frame(co, ctxt, p, val);
    }
    else {
        struct pcvdom_element *p = frame->pos;
        struct pcvdom_element *parent = NULL;
        if (p)
            parent = pcvdom_element_parent(p);

        if (parent == NULL) {
            purc_set_error_with_info(PURC_ERROR_ENTITY_NOT_FOUND,
                    "no vdom element exists");
            return -1;
        }
        for (uint64_t i=0; i<level; ++i) {
            if (p == NULL)
                break;
            p = pcvdom_element_parent(p);
        }
        if (p == NULL) {
            if (!silently) {
                purc_set_error_with_info(PURC_ERROR_ENTITY_NOT_FOUND,
                        "no vdom element exists");
                return -1;
            }
            p = parent;
        }
        return post_process_bind_at_vdom(co, frame, p, val);
    }
}

static int
post_process_val(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        purc_variant_t val)
{
    struct ctxt_for_bind *ctxt;
    ctxt = (struct ctxt_for_bind*)frame->ctxt;

    const char *s_name = get_name(co, frame);
    if (!s_name) {
        return -1;
    }

    return pcintr_bind_named_variable(&co->stack, frame, s_name, ctxt->at,
            ctxt->temporarily, false, val);
}

static int
post_process(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);
    struct ctxt_for_bind *ctxt;
    ctxt = (struct ctxt_for_bind*)frame->ctxt;

    const char *method_name = PCVCM_EV_DEFAULT_METHOD_NAME;
    if (ctxt->against && purc_variant_is_string(ctxt->against)) {
        const char *name = purc_variant_get_string_const(ctxt->against);
        if (name[0] != '#' && name[0] != '_') {
            method_name = name;
        }
    }

    purc_variant_t val = pcvcm_to_expression_variable(ctxt->vcm_ev,
            method_name, ctxt->constantly, false);
    if (val == PURC_VARIANT_INVALID) {
        return -1;
    }

    int r = post_process_val(co, frame, val);
    purc_variant_unref(val);

    return r ? -1 : 0;
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
process_attr_at(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_bind *ctxt;
    ctxt = (struct ctxt_for_bind*)frame->ctxt;
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
    ctxt->at = val;
    purc_variant_ref(val);

    return 0;
}

static int
process_attr_against(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_bind *ctxt;
    ctxt = (struct ctxt_for_bind*)frame->ctxt;
    if (ctxt->against != PURC_VARIANT_INVALID) {
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
    ctxt->against = val;
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

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, AS)) == name) {
        return process_attr_as(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, AT)) == name) {
        return process_attr_at(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, AGAINST)) == name) {
        return process_attr_against(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, TEMPORARILY)) == name ||
            pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, TEMP)) == name) {
        PC_ASSERT(purc_variant_is_undefined(val));
        ctxt->temporarily = 1;
        return 0;
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, ON)) == name) {
        struct ctxt_for_bind *ctxt;
        ctxt = (struct ctxt_for_bind*)frame->ctxt;
        ctxt->vcm_ev = attr->val;
        return 0;
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, SILENTLY)) == name) {
        return 0;
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, CONSTANTLY)) == name
            || pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, CONST)) == name) {
        PC_ASSERT(purc_variant_is_undefined(val));
        ctxt->constantly= 1;
        return 0;
    }

    /* ignore other attr */
    return 0;
}

bool before_eval_attr(pcintr_stack_t stack,
        struct pcintr_stack_frame *frame, const char *attr_name,
        struct pcvcm_node *vcm)
{
    UNUSED_PARAM(stack);
    UNUSED_PARAM(frame);
    UNUSED_PARAM(attr_name);
    UNUSED_PARAM(vcm);
    if (strcmp(attr_name, ATTR_ON) == 0) {
        return true;
    }
    return false;
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

    if (0 != pcintr_stack_frame_eval_attr_and_content_full(stack, frame,
                before_eval_attr, true)) {
        return NULL;
    }

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
    r = pcintr_walk_attrs(frame, element, stack, attr_found_val);
    if (r)
        return ctxt;

    if (ctxt->as == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_ARGUMENT_MISSED,
                "lack of vdom attribute 'as' for element <%s>",
                element->tag_name);
        return ctxt;
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
    UNUSED_PARAM(frame);
    UNUSED_PARAM(element);

    PC_ASSERT(element->tag_id == PCHVML_TAG_CATCH);

    pcintr_stack_t stack = &co->stack;

    if (stack->except)
        return 0;

    return 0;
}

static int
on_content(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_content *content)
{
    PC_ASSERT(content);

    pcintr_stack_t stack = &co->stack;

    if (stack->except)
        return 0;

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

    pcintr_stack_t stack = &co->stack;

    if (stack->except)
        return 0;

    if (ctxt->vcm_ev) {
        return post_process(co, frame);
    }

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
        int r = on_child_finished(co, frame);
        PC_ASSERT(0 == r);
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

