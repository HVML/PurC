/**
 * @file update.c
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

#include "internal.h"

#include "private/debug.h"
#include "private/runloop.h"

#include "ops.h"

#include <pthread.h>
#include <unistd.h>
#include <libgen.h>

#define TO_DEBUG 1

struct ctxt_for_update {
    struct pcvdom_node           *curr;

    purc_variant_t                on;
    purc_variant_t                to;
    purc_variant_t                src;
};

static void
ctxt_for_update_destroy(struct ctxt_for_update *ctxt)
{
    if (ctxt) {
        PURC_VARIANT_SAFE_CLEAR(ctxt->on);
        PURC_VARIANT_SAFE_CLEAR(ctxt->to);
        PURC_VARIANT_SAFE_CLEAR(ctxt->src);
        free(ctxt);
    }
}

static void
ctxt_destroy(void *ctxt)
{
    ctxt_for_update_destroy((struct ctxt_for_update*)ctxt);
}

static purc_variant_t
get_source_by_with(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
    purc_variant_t with)
{
    UNUSED_PARAM(frame);
    PC_ASSERT(purc_variant_is_type(with, PURC_VARIANT_TYPE_ULONGINT));
    bool ok;
    uint64_t u64;
    ok = purc_variant_cast_to_ulongint(with, &u64, false);
    PC_ASSERT(ok);
    struct pcvcm_node *vcm_content;
    vcm_content = (struct pcvcm_node*)u64;
    PC_ASSERT(vcm_content);

    pcintr_stack_t stack = co->stack;
    PC_ASSERT(stack);

    return pcvcm_eval(vcm_content, stack);
}

static purc_variant_t
get_source_by_from(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
    purc_variant_t from)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);
    const char* uri = purc_variant_get_string_const(from);
    return pcintr_load_from_uri(uri);
}

static purc_variant_t
get_source(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    purc_variant_t with;
    with = purc_variant_object_get_by_ckey(frame->attr_vars, "with");
    if (with != PURC_VARIANT_INVALID)
        return get_source_by_with(co, frame, with);

    purc_variant_t from;
    from = purc_variant_object_get_by_ckey(frame->attr_vars, "from");
    if (from != PURC_VARIANT_INVALID)
        return get_source_by_from(co, frame, from);

    return PURC_VARIANT_INVALID;
}

static int
process(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(co);
    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
    PC_ASSERT(ctxt);
    purc_variant_t on  = ctxt->on;
    purc_variant_t to  = ctxt->to;
    purc_variant_t src = ctxt->src;
    PC_ASSERT(on != PURC_VARIANT_INVALID);
    PC_ASSERT(to != PURC_VARIANT_INVALID);
    PC_ASSERT(src != PURC_VARIANT_INVALID);

    /* FIXME: what if array of elements? */
    enum purc_variant_type type = purc_variant_get_type(on);
    if (type == PURC_VARIANT_TYPE_NATIVE) {
        const char *s = purc_variant_get_string_const(src);
        // fprintf(stderr, "[%s]\n", s);
        // pcintr_printf_to_edom(stack, "%s", s);
        pcintr_printf_to_fragment(co->stack, on, to, "%s", s);
        return 0;
    }
    PC_ASSERT(0); // Not implemented yet
    return -1;
}

static int
post_process(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
    PC_ASSERT(ctxt);

    // TODO: '$@'
    purc_variant_t on;
    on = purc_variant_object_get_by_ckey(frame->attr_vars, "on");
    if (on == PURC_VARIANT_INVALID)
        return -1;
    PURC_VARIANT_SAFE_CLEAR(ctxt->on);
    ctxt->on = on;
    purc_variant_ref(on);

    purc_variant_t to;
    to = purc_variant_object_get_by_ckey(frame->attr_vars, "to");
    if (to == PURC_VARIANT_INVALID)
        return -1;
    PURC_VARIANT_SAFE_CLEAR(ctxt->to);
    ctxt->to = to;
    purc_variant_ref(to);

    purc_variant_t src = get_source(co, frame);
    if (src == PURC_VARIANT_INVALID)
        return -1;
    PURC_VARIANT_SAFE_CLEAR(ctxt->src);
    ctxt->src = src;

    return process(co, frame);
}

static void*
after_pushed(pcintr_stack_t stack, pcvdom_element_t pos)
{
    PC_ASSERT(stack && pos);
    PC_ASSERT(stack == purc_get_stack());

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    frame->pos = pos; // ATTENTION!!

    if (pcintr_set_symbol_var_at_sign())
        return NULL;

    struct pcvdom_element *element = frame->pos;
    PC_ASSERT(element);
    D("<%s>", element->tag_name);

    int r;
    r = pcintr_element_eval_attrs(frame, element);
    if (r)
        return NULL;

    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)calloc(1, sizeof(*ctxt));
    if (!ctxt) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    frame->ctxt = ctxt;
    frame->ctxt_destroy = ctxt_destroy;
    purc_clr_error();

    r = post_process(&stack->co, frame);
    if (r)
        return NULL;

    return ctxt;
}

static bool
on_popping(pcintr_stack_t stack, void* ud)
{
    PC_ASSERT(stack);
    PC_ASSERT(stack == purc_get_stack());

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame);
    PC_ASSERT(ud == frame->ctxt);

    struct pcvdom_element *element = frame->pos;
    PC_ASSERT(element);

    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
    if (ctxt) {
        ctxt_for_update_destroy(ctxt);
        frame->ctxt = NULL;
    }

    D("</%s>", element->tag_name);
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
    char *text = content->text;
    D("content: [%s]", text);
}

static void
on_comment(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_comment *comment)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);
    PC_ASSERT(comment);
    char *text = comment->text;
    D("comment: [%s]", text);
}

static pcvdom_element_t
select_child(pcintr_stack_t stack, void* ud)
{
    PC_ASSERT(stack);
    PC_ASSERT(stack == purc_get_stack());

    pcintr_coroutine_t co = &stack->co;
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(ud == frame->ctxt);

    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;

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
                D("");
                pcvdom_element_t element = PCVDOM_ELEMENT_FROM_NODE(curr);
                on_element(co, frame, element);
                PC_ASSERT(stack->except == 0);
                return element;
            }
        case PCVDOM_NODE_CONTENT:
            D("");
            on_content(co, frame, PCVDOM_CONTENT_FROM_NODE(curr));
            goto again;
        case PCVDOM_NODE_COMMENT:
            D("");
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

struct pcintr_element_ops* pcintr_get_update_ops(void)
{
    return &ops;
}

