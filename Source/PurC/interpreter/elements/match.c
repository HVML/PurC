/**
 * @file match.c
 * @author Xue Shuming
 * @date 2022/01/13
 * @brief The ops for <match>
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

#include "purc-executor.h"

// FIXME:
#include "../executors/match_for.h"

#include <pthread.h>
#include <unistd.h>

struct ctxt_for_match {
    struct pcvdom_node *curr;
    purc_variant_t for_var;
    purc_variant_t on;
    purc_variant_t with;

    struct match_for_param param;
    bool is_exclusively;
    bool matched;
};

static void
ctxt_for_match_destroy(struct ctxt_for_match *ctxt)
{
    if (ctxt) {
        PURC_VARIANT_SAFE_CLEAR(ctxt->for_var);
        PURC_VARIANT_SAFE_CLEAR(ctxt->on);
        PURC_VARIANT_SAFE_CLEAR(ctxt->with);

        match_for_param_reset(&ctxt->param);
        free(ctxt);
    }
}

static void
ctxt_destroy(void *ctxt)
{
    ctxt_for_match_destroy((struct ctxt_for_match*)ctxt);
}

static int
post_process(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(co);
    int r;

    struct ctxt_for_match *ctxt;
    ctxt = (struct ctxt_for_match*)frame->ctxt;

    purc_variant_t for_var = ctxt->for_var;
    bool matched = false;
    if (for_var) {
        const char *for_value = purc_variant_get_string_const(for_var);
        r = match_for_parse(for_value, strlen(for_value), &ctxt->param);

        struct pcintr_stack_frame *parent;
        parent = pcintr_stack_frame_get_parent(frame);

        purc_variant_t parent_result;
        parent_result = pcintr_get_question_var(parent);

        r = match_for_rule_eval(&ctxt->param.rule, parent_result, &matched);
    }
    else if (ctxt->with) {
        matched = purc_variant_booleanize(ctxt->with);
    }
    else {
        matched = true;
    }

    ctxt->matched = matched;

    if (matched) {
        if (ctxt->on) {
            r = pcintr_set_question_var(frame, ctxt->on);
        }
        else {
            struct pcintr_stack_frame *parent;
            parent = pcintr_stack_frame_get_parent(frame);
            purc_variant_t parent_result;
            parent_result = pcintr_get_question_var(parent);
            r = pcintr_set_question_var(frame, parent_result);
        }
        return r ? -1 : 0;
    }

    return 0;
}

static int
process_attr_for(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_match *ctxt;
    ctxt = (struct ctxt_for_match*)frame->ctxt;
    if (ctxt->for_var != PURC_VARIANT_INVALID) {
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
    ctxt->for_var = val;
    purc_variant_ref(val);

    return 0;
}

static int
process_attr_on(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_match *ctxt;
    ctxt = (struct ctxt_for_match*)frame->ctxt;
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
    struct ctxt_for_match *ctxt;
    ctxt = (struct ctxt_for_match*)frame->ctxt;
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
attr_found_val(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val,
        struct pcvdom_attr *attr,
        void *ud)
{
    UNUSED_PARAM(attr);
    UNUSED_PARAM(ud);

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, FOR)) == name) {
        return process_attr_for(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, EXCLUSIVELY)) == name ||
            pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, EXCL)) == name) {
        struct ctxt_for_match *ctxt = (struct ctxt_for_match*)frame->ctxt;
        ctxt->is_exclusively = true;
        return 0;
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, ON)) == name) {
        return process_attr_on(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, WITH)) == name) {
        return process_attr_with(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, SILENTLY)) == name) {
        return 0;
    }

    /* ignore other attr */
    return 0;
}

static void*
after_pushed(pcintr_stack_t stack, pcvdom_element_t pos)
{
    if (stack->except)
        return NULL;

    pcintr_check_insertion_mode_for_normal_element(stack);

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    struct ctxt_for_match *ctxt = frame->ctxt;
    if (!ctxt) {
        ctxt = (struct ctxt_for_match*)calloc(1, sizeof(*ctxt));
        if (!ctxt) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            return NULL;
        }

        frame->ctxt = ctxt;
        frame->ctxt_destroy = ctxt_destroy;

        frame->pos = pos; // ATTENTION!!
    }

    if (0 != pcintr_stack_frame_eval_attr_and_content(stack, frame, false)) {
        return NULL;
    }

    struct pcintr_stack_frame *parent;
    parent = pcintr_stack_frame_get_parent(frame);
    if (!parent || !parent->pos || parent->pos->tag_id != PCHVML_TAG_TEST) {
        purc_set_error_with_info(PURC_ERROR_ENTITY_NOT_FOUND,
                "no matching <test> for <match>");
        // FIXME: shall not happen!!!
        return ctxt;
    }

    struct pcvdom_element *element = frame->pos;

    int r;
    r = pcintr_walk_attrs(frame, element, stack, attr_found_val);
    if (r)
        return ctxt;

    purc_clr_error();

    r = post_process(stack->co, frame);
    if (r)
        return ctxt;

    return ctxt;
}

static bool
on_popping(pcintr_stack_t stack, void* ud)
{
    UNUSED_PARAM(ud);

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    if (frame->ctxt == NULL)
        return true;

    struct ctxt_for_match *ctxt;
    ctxt = (struct ctxt_for_match*)frame->ctxt;
    if (ctxt) {
        if (ctxt->is_exclusively && ctxt->matched) {
            struct pcintr_stack_frame *parent;
            parent = pcintr_stack_frame_get_parent(frame);
            PURC_VARIANT_SAFE_CLEAR(parent->result_from_child);
            parent->result_from_child = purc_variant_make_boolean(true);
        }
        ctxt_for_match_destroy(ctxt);
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
    UNUSED_PARAM(content);
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

    struct ctxt_for_match *ctxt;
    ctxt = (struct ctxt_for_match*)frame->ctxt;

    struct pcvdom_node *curr;

    if (!ctxt->matched) {
        if (stack->except == 0)
            return NULL;
    }

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

struct pcintr_element_ops* pcintr_get_match_ops(void)
{
    return &ops;
}

