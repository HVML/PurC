/**
 * @file catch.c
 * @author Xue Shuming
 * @date 2022/01/14
 * @brief The ops for <catch>
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

#define KEY_NAME        "name"
#define KEY_INFO        "info"

struct ctxt_for_catch {
    struct pcvdom_node           *curr;
    purc_variant_t                for_var;
    struct pcintr_exception      *exception;
    bool                          match;
};

static void
ctxt_for_catch_destroy(struct ctxt_for_catch *ctxt)
{
    if (ctxt) {
        PURC_VARIANT_SAFE_CLEAR(ctxt->for_var);

        free(ctxt);
    }
}

static void
ctxt_destroy(void *ctxt)
{
    ctxt_for_catch_destroy((struct ctxt_for_catch*)ctxt);
}

static int
post_process_data(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(co);

    struct ctxt_for_catch *ctxt;
    ctxt = (struct ctxt_for_catch*)frame->ctxt;

    purc_variant_t for_var = ctxt->for_var;
    if (for_var != PURC_VARIANT_INVALID) {
        ctxt->match = pcintr_match_exception(ctxt->exception->error_except,
                for_var);
    }
    else {
        ctxt->match = true;
    }

    if (ctxt->match) {
        const char *s_except = purc_atom_to_string(ctxt->exception->error_except);
        purc_variant_t s = purc_variant_make_string(s_except, false);
        if (!s) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto out;
        }

        purc_variant_t obj;
        if (ctxt->exception->exinfo) {
            obj = purc_variant_make_object_by_static_ckey(2,
                KEY_NAME, s, KEY_INFO, ctxt->exception->exinfo);
        }
        else {
            obj = purc_variant_make_object_by_static_ckey(1,
                KEY_NAME, s, KEY_INFO, s);
        }
        if (!obj) {
            purc_variant_unref(s);
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto out;
        }

        pcintr_set_question_var(frame, obj);
        purc_variant_unref(s);
        purc_variant_unref(obj);
    }

out:

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

static int
process_attr_for(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_catch *ctxt;
    ctxt = (struct ctxt_for_catch*)frame->ctxt;
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
attr_found_val(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val,
        struct pcvdom_attr *attr,
        void *ud)
{
    UNUSED_PARAM(ud);
    UNUSED_PARAM(attr);

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, FOR)) == name) {
        return process_attr_for(frame, element, name, val);
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, SILENTLY)) == name) {
        return 0;
    }

    /* ignore other attr */
    return 0;
}

static void*
_after_pushed(pcintr_stack_t stack, pcvdom_element_t pos,
        struct pcintr_exception *exception)
{
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    struct ctxt_for_catch *ctxt = frame->ctxt;
    if (!ctxt) {
        ctxt = (struct ctxt_for_catch*)calloc(1, sizeof(*ctxt));
        if (!ctxt) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            return NULL;
        }

        ctxt->exception = exception;

        frame->ctxt = ctxt;
        frame->ctxt_destroy = ctxt_destroy;

        frame->pos = pos; // ATTENTION!!
    }

    if (0 != pcintr_stack_frame_eval_attr_and_content(stack, frame, false)) {
        return NULL;
    }

    struct pcvdom_element *element = frame->pos;

    int r;
    r = pcintr_walk_attrs(frame, element, stack, attr_found_val);
    if (r)
        return ctxt;

    r = post_process(stack->co, frame);
    if (r)
        return ctxt;

    return ctxt;
}

static void*
after_pushed(pcintr_stack_t stack, pcvdom_element_t pos)
{
    if (stack->except == 0)
        return NULL;

    pcintr_check_insertion_mode_for_normal_element(stack);

    struct pcintr_exception cache = {};
    pcintr_exception_move(&cache, &stack->exception);
    stack->except = 0;

    // keep last vcm_ctxt
    struct pcvcm_eval_ctxt *vcm_ctxt = stack->vcm_ctxt;
    stack->vcm_ctxt = NULL;


    struct ctxt_for_catch *ctxt;
    ctxt = (struct ctxt_for_catch*)_after_pushed(stack, pos, &cache);
    if (ctxt && ctxt->match) {
        if (vcm_ctxt) {
            pcvcm_eval_ctxt_destroy(vcm_ctxt);
        }
    }
    else {
        pcintr_exception_move(&stack->exception, &cache);
        stack->except = 1;
        // FIXME: eval failed on catch _after_pushed
        if (stack->vcm_ctxt) {
            if (vcm_ctxt) {
                pcvcm_eval_ctxt_destroy(vcm_ctxt);
            }
        }
        else {
            stack->vcm_ctxt = vcm_ctxt;
        }
    }

    pcintr_exception_clear(&cache);

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

    struct ctxt_for_catch *ctxt;
    ctxt = (struct ctxt_for_catch*)frame->ctxt;
    if (ctxt) {
        ctxt_for_catch_destroy(ctxt);
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

    struct ctxt_for_catch *ctxt;
    ctxt = (struct ctxt_for_catch*)frame->ctxt;

    if (!ctxt->match)
        return NULL;

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

    if (curr == NULL)
        return NULL;

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

struct pcintr_element_ops* pcintr_get_catch_ops(void)
{
    return &ops;
}

