/**
 * @file back.c
 * @author Xu Xiaohong
 * @date 2022/04/07
 * @brief The ops for <back>
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

#include <pthread.h>
#include <unistd.h>

struct ctxt_for_back {
    struct pcvdom_node           *curr;

    struct pcintr_stack_frame    *back_anchor;
    purc_variant_t                with;
};

static void
ctxt_for_back_destroy(struct ctxt_for_back *ctxt)
{
    if (ctxt) {
        PURC_VARIANT_SAFE_CLEAR(ctxt->with);

        free(ctxt);
    }
}

static void
ctxt_destroy(void *ctxt)
{
    ctxt_for_back_destroy((struct ctxt_for_back*)ctxt);
}

static int
process_back_level(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, int64_t back_level);

static int
post_process_data(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(co);

    struct ctxt_for_back *ctxt;
    ctxt = (struct ctxt_for_back*)frame->ctxt;

    struct pcvdom_element *element = frame->pos;

    purc_atom_t name = pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, TO));

    if (ctxt->back_anchor == NULL && frame->silently) {
        process_back_level(frame, element, name, -1);
    }

    if (ctxt->back_anchor == NULL) {
        purc_set_error_with_info(PURC_ERROR_ENTITY_NOT_FOUND,
                "vdom attribute 'to' for element <back> undefined");
        return -1;
    }

    if (ctxt->with != PURC_VARIANT_INVALID) {
        if (pcintr_set_question_var(ctxt->back_anchor, ctxt->with))
            return -1;
    }

    co->stack.back_anchor = ctxt->back_anchor;

    return 0;
}

static int
post_process(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(co);

    int r = post_process_data(co, frame);
    if (r)
        return r;

    return 0;
}

int
process_back_level(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, int64_t back_level)
{
    struct ctxt_for_back *ctxt;
    ctxt = (struct ctxt_for_back*)frame->ctxt;

    struct pcintr_stack_frame *p = pcintr_stack_frame_get_parent(frame);
    while (p && (back_level == -1 || back_level > 0)) {
        struct pcintr_stack_frame *parent = pcintr_stack_frame_get_parent(p);
        if (back_level != -1) {
            --back_level;
        }

        if (back_level == -1) {
            if (parent == NULL)
                break;
        }
        p = parent;
    }

    if (p == NULL) {
        purc_set_error_with_info(PURC_ERROR_ENTITY_NOT_FOUND,
                "vdom attribute '%s' for element <%s>",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }

    ctxt->back_anchor = p;
    return 0;
}

static int
post_process_to_by_id(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, const char *id)
{
    struct ctxt_for_back *ctxt;
    ctxt = (struct ctxt_for_back*)frame->ctxt;

    struct pcintr_stack_frame *p = pcintr_stack_frame_get_parent(frame);
    while (p) {
        if (p->elem_id) {
            const char *s = purc_variant_get_string_const(p->elem_id);
            if (s && strcmp(s, id)==0) {
                break;
            }
        }
        struct pcintr_stack_frame *parent = pcintr_stack_frame_get_parent(p);
        p = parent;
    }

    if (p == NULL) {
        purc_set_error_with_info(PURC_ERROR_ENTITY_NOT_FOUND,
                "vdom attribute '%s' for element <%s>",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }

    ctxt->back_anchor = p;
    return 0;
}

static int
post_process_to_by_atom(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_atom_t atom)
{
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, _LAST)) == atom) {
        return process_back_level(frame, element, name, 1);
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, _NEXTTOLAST)) == atom ) {
        return process_back_level(frame, element, name, 2);
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, _TOPMOST)) == atom ) {
        return process_back_level(frame, element, name, -1);
    }

    purc_set_error_with_info(PURC_ERROR_BAD_NAME,
            "to = '%s'", purc_atom_to_string(atom));
    return -1;
}

static int
process_attr_to(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_back *ctxt;
    ctxt = (struct ctxt_for_back*)frame->ctxt;
    if (ctxt->back_anchor) {
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

    if (purc_variant_is_string(val)) {
        const char *s_to = purc_variant_get_string_const(val);
        if (s_to[0] == '#') {
            return post_process_to_by_id(frame, element, name, s_to+1);
        }

        if (s_to[0] == '_') {
            purc_atom_t atom = PCHVML_KEYWORD_ATOM(HVML, s_to);
            if (atom == 0) {
                purc_set_error_with_info(PURC_ERROR_BAD_NAME,
                        "<%s to = %s>", element->tag_name, s_to);
                return -1;
            }
            return post_process_to_by_atom(frame, element, name, atom);
        }

        int64_t back_level = (int64_t)purc_variant_numerify(val);
        if (back_level <= 0) {
            purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                    "<%s to = %s>", element->tag_name, s_to);
            return -1;
        }
        return process_back_level(frame, element, name, back_level);
    }
    else if (purc_variant_is_ulongint(val)) {
        return process_back_level(frame, element, name, val->u64);
    }
    else if (purc_variant_is_longint(val)) {
        int64_t back_level = val->i64;
        if (back_level <= 0) {
            purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                    "<%s to = %lld>", element->tag_name, (long long)back_level);
            return -1;
        }
        return process_back_level(frame, element, name, back_level);
    }
    else if (purc_variant_is_number(val)) {
        uint64_t back_level = 0;
        if (!purc_variant_cast_to_ulongint(val, &back_level, true) ||
                back_level <= 0) {
            purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                    "<%s to = invalid number", element->tag_name);
            return -1;
        }
        return process_back_level(frame, element, name, back_level);
    }

    purc_set_error_with_info(PURC_ERROR_NOT_IMPLEMENTED,
            "<%s to = ...>", element->tag_name);
    return -1;
}

static int
process_attr_with(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_back *ctxt;
    ctxt = (struct ctxt_for_back*)frame->ctxt;
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

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, TO)) == name) {
        return process_attr_to(frame, element, name, val);
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

    int r = 0;

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    struct ctxt_for_back *ctxt = frame->ctxt;
    if (!ctxt) {
        ctxt = (struct ctxt_for_back*)calloc(1, sizeof(*ctxt));
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

    struct pcvdom_element *element = frame->pos;

    r = pcintr_walk_attrs(frame, element, stack, attr_found_val);
    if (r)
        return ctxt;

    if (!ctxt->with) {
        purc_variant_t caret = pcintr_get_symbol_var(frame,
                PURC_SYMBOL_VAR_CARET);
        if (caret && !purc_variant_is_undefined(caret)) {
            ctxt->with = caret;
            purc_variant_ref(ctxt->with);
        }
    }

    r = post_process(stack->co, frame);
    if (r)
        return ctxt;

    // NOTE: no element to process if succeeds
    return NULL;
}

static bool
on_popping(pcintr_stack_t stack, void* ud)
{
    UNUSED_PARAM(ud);

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    if (frame->ctxt == NULL)
        return true;

    struct ctxt_for_back *ctxt;
    ctxt = (struct ctxt_for_back*)frame->ctxt;
    if (ctxt) {
        ctxt_for_back_destroy(ctxt);
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

    return -1;
}

static int
on_content(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_content *content)
{
    UNUSED_PARAM(frame);
    UNUSED_PARAM(content);

    pcintr_stack_t stack = &co->stack;

    if (stack->except)
        return 0;

    return -1;
}

static int
on_comment(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_comment *comment)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);
    UNUSED_PARAM(comment);
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

    struct ctxt_for_back *ctxt;
    ctxt = (struct ctxt_for_back*)frame->ctxt;

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
            purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
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
            purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
            break;
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

struct pcintr_element_ops* pcintr_get_back_ops(void)
{
    return &ops;
}

