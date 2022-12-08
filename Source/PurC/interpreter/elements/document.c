/**
 * @file document.c
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

#include "private/stringbuilder.h"

#include "../ops.h"

#include <pthread.h>
#include <unistd.h>
#include <limits.h>

struct ctxt_for_document {
    struct pcvdom_node           *curr;
};

static void
ctxt_for_document_destroy(struct ctxt_for_document *ctxt)
{
    if (ctxt) {
        free(ctxt);
    }
}

static void
ctxt_destroy(void *ctxt)
{
    ctxt_for_document_destroy((struct ctxt_for_document*)ctxt);
}

static int
token_found(const char *start, const char *end, void *ud)
{
    pcintr_stack_t stack = (pcintr_stack_t)ud;
    (void)stack;

    if (start == end)
        return 0;

    bool ok;
    ok = pcintr_load_dynamic_variant(stack->co, start, end-start);

    return ok ? 0 : -1;
}

static void*
after_pushed(pcintr_stack_t stack, pcvdom_element_t pos)
{
    if (stack->except)
        return NULL;

    int r;
    // TODO: catch/except/error if failed???
    r = pcintr_init_vdom_under_stack(stack);
    if (r)
        return NULL;

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    struct ctxt_for_document *ctxt = frame->ctxt;
    if (!ctxt) {
        ctxt = (struct ctxt_for_document*)calloc(1, sizeof(*ctxt));
        if (!ctxt) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            return NULL;
        }

        frame->ctxt = ctxt;
        frame->ctxt_destroy = ctxt_destroy;

        frame->pos = pos; // ATTENTION!!
    }

    frame->edom_element = NULL;

    struct pcvdom_document *document;
    document = stack->vdom;
    struct pcvdom_doctype  *doctype = &document->doctype;
    const char *system_info = doctype->system_info;
    if (system_info) {
        const char *p = strchr(system_info, ':');
        if (p) {
            size_t nr = p - system_info;
            if (nr) {
                if (stack->tag_prefix) {
                    free(stack->tag_prefix);
                }
                stack->tag_prefix = strndup(system_info, nr);
            }
            ++p;
            r = pcutils_token_by_delim(p, p + strlen(p),
                    ' ', stack, token_found);
            if (r) {
                return ctxt;
            }
        }
    }

    purc_clr_error();

    return ctxt;
}

static bool
on_popping(pcintr_stack_t stack, void* ud)
{
    UNUSED_PARAM(ud);
    switch (stack->mode) {
        case STACK_VDOM_BEFORE_HVML:
            stack->mode = STACK_VDOM_AFTER_HVML;
            break;
        case STACK_VDOM_BEFORE_HEAD:
            stack->mode = STACK_VDOM_AFTER_HVML;
            break;
        case STACK_VDOM_IN_HEAD:
            break;
        case STACK_VDOM_AFTER_HEAD:
            stack->mode = STACK_VDOM_AFTER_HVML;
            break;
        case STACK_VDOM_IN_BODY:
            break;
        case STACK_VDOM_AFTER_BODY:
            stack->mode = STACK_VDOM_AFTER_HVML;
            break;
        case STACK_VDOM_AFTER_HVML:
            break;
        default:
            break;
    }

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    if (frame->ctxt == NULL)
        return true;

    struct ctxt_for_document *ctxt;
    ctxt = (struct ctxt_for_document*)frame->ctxt;
    if (ctxt) {
        ctxt_for_document_destroy(ctxt);
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

    struct ctxt_for_document *ctxt;
    ctxt = (struct ctxt_for_document*)frame->ctxt;

    struct pcvdom_node *curr;

again:
    curr = ctxt->curr;

    if (curr == NULL) {
        struct pcvdom_document *document = stack->vdom;
        curr = pcvdom_node_first_child(&document->node);
        purc_clr_error();
    }
    else {
        curr = pcvdom_node_next_sibling(curr);
        purc_clr_error();
    }

    ctxt->curr = curr;

    if (curr == NULL) {
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
                if (element->tag_id == PCHVML_TAG_HVML)
                    return element;
                goto again;
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

struct pcintr_element_ops* pcintr_get_document_ops(void)
{
    return &ops;
}

