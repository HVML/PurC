/**
 * @file init.c
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

struct ctxt_for_init {
    struct pcvdom_node           *curr;

    unsigned int                  under_head:1;
};

static void
ctxt_for_init_destroy(struct ctxt_for_init *ctxt)
{
    if (ctxt) {
        free(ctxt);
    }
}

static void
ctxt_destroy(void *ctxt)
{
    ctxt_for_init_destroy((struct ctxt_for_init*)ctxt);
}

static void
post_process_bind_scope_var(pcintr_coroutine_t co,
        struct pcintr_stack_frame *frame,
        purc_variant_t name, purc_variant_t val)
{
    struct pcvdom_element *element = frame->scope;
    PC_ASSERT(element);

    const char *s_name = purc_variant_get_string_const(name);
    PC_ASSERT(s_name);

    bool ok;
    struct ctxt_for_init *ctxt = (struct ctxt_for_init*)frame->ctxt;
    if (ctxt->under_head) {
        ok = purc_bind_document_variable(co->stack->vdom, s_name, val);
    } else {
        element = pcvdom_element_parent(element);
        PC_ASSERT(element);
        ok = pcintr_bind_scope_variable(element, s_name, val);
    }
    purc_variant_unref(val);
    if (!ok) {
        frame->next_step = -1;
        co->state = CO_STATE_TERMINATED;
        return;
    }
}

static void
post_process_array(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        purc_variant_t name)
{
    purc_variant_t via;
    via = purc_variant_object_get_by_ckey(frame->attr_vars, "via");
    purc_clr_error();
    purc_variant_t set;
    set = purc_variant_make_set(0, via, PURC_VARIANT_INVALID);
    if (set == PURC_VARIANT_INVALID) {
        frame->next_step = -1;
        co->state = CO_STATE_TERMINATED;
        return;
    }

    purc_variant_t val;
    foreach_value_in_variant_array(frame->ctnt_var, val)
        bool ok = purc_variant_is_type(val, PURC_VARIANT_TYPE_OBJECT);
        if (ok) {
            ok = purc_variant_set_add(set, val, true);
        }
        if (!ok) {
            purc_variant_unref(set);
            purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
            frame->next_step = -1;
            co->state = CO_STATE_TERMINATED;
            return;
        }
    end_foreach;

    post_process_bind_scope_var(co, frame, name, set);
}

static void
post_process_object(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        purc_variant_t name)
{
    purc_variant_t val = frame->ctnt_var;

    purc_variant_ref(val);
    post_process_bind_scope_var(co, frame, name, val);
}

static void
post_process(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    purc_variant_t name;
    name = purc_variant_object_get_by_ckey(frame->attr_vars, "as");
    if (name == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_NOT_EXISTS);
        frame->next_step = -1;
        co->state = CO_STATE_TERMINATED;
        return;
    }

    if (purc_variant_is_type(frame->ctnt_var, PURC_VARIANT_TYPE_ARRAY)) {
        post_process_array(co, frame, name);
        return;
    }
    if (purc_variant_is_type(frame->ctnt_var, PURC_VARIANT_TYPE_OBJECT)) {
        post_process_object(co, frame, name);
        return;
    }

    purc_set_error(PURC_ERROR_NOT_EXISTS);
    frame->next_step = -1;
    co->state = CO_STATE_TERMINATED;
}

static void
after_pushed(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    struct pcvdom_element *element = frame->scope;
    PC_ASSERT(element);

    int r;
    r = pcintr_element_eval_attrs(frame, element);
    if (r) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        frame->next_step = -1;
        co->state = CO_STATE_TERMINATED;
        return;
    }

    struct pcvcm_node *vcm_content = element->vcm_content;
    PC_ASSERT(vcm_content);

    pcintr_stack_t stack = co->stack;
    PC_ASSERT(stack);

    purc_variant_t v = pcvcm_eval(vcm_content, stack);
    if (v == PURC_VARIANT_INVALID) {
        frame->next_step = -1;
        co->state = CO_STATE_TERMINATED;
        return;
    }
    PURC_VARIANT_SAFE_CLEAR(frame->ctnt_var);
    frame->ctnt_var = v;

    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)calloc(1, sizeof(*ctxt));
    if (!ctxt) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        frame->next_step = -1;
        co->state = CO_STATE_TERMINATED;
        return;
    }

    frame->ctxt = ctxt;
    frame->next_step = NEXT_STEP_ON_POPPING;
    frame->ctxt_destroy = ctxt_destroy;
    co->state = CO_STATE_READY;

    while ((element=pcvdom_element_parent(element))) {
        if (element->tag_id == PCHVML_TAG_HEAD) {
            ctxt->under_head = 1;
        }
    }

    post_process(co, frame);
}

static void
on_popping(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    pcintr_stack_t stack = co->stack;
    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;
    if (ctxt) {
        ctxt_for_init_destroy(ctxt);
        frame->ctxt = NULL;
    }
    pcintr_pop_stack_frame(stack);
    co->state = CO_STATE_READY;
}

static void
on_element(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_element *element)
{
    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;

    pcintr_stack_t stack = co->stack;
    struct pcintr_stack_frame *child_frame;
    child_frame = pcintr_push_stack_frame(stack);
    if (!child_frame) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return;
    }
    child_frame->ops = pcintr_get_ops_by_element(element);
    child_frame->scope = element;
    child_frame->next_step = NEXT_STEP_AFTER_PUSHED;

    ctxt->curr = &element->node;
    frame->next_step = NEXT_STEP_SELECT_CHILD;
    co->state = CO_STATE_READY;
}

static void
on_content(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_content *content)
{
    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;

    ctxt->curr = &content->node;
    frame->next_step = NEXT_STEP_SELECT_CHILD;
    co->state = CO_STATE_READY;
}

static void
on_comment(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_comment *comment)
{
    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;

    ctxt->curr = &comment->node;
    frame->next_step = NEXT_STEP_SELECT_CHILD;
    co->state = CO_STATE_READY;
}

static void
select_child(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;

    if (ctxt->curr == NULL) {
        struct pcvdom_element *element = frame->scope;
        struct pcvdom_node *node = &element->node;
        node = pcvdom_node_first_child(node);
        ctxt->curr = node;
    }
    else {
        ctxt->curr = pcvdom_node_next_sibling(ctxt->curr);
    }

    if (ctxt->curr == NULL) {
        purc_clr_error();
        frame->next_step = NEXT_STEP_ON_POPPING;
        co->state = CO_STATE_READY;
        return;
    }

    switch (ctxt->curr->type) {
        case PCVDOM_NODE_DOCUMENT:
            PC_ASSERT(0); // Not implemented yet
            break;
        case PCVDOM_NODE_ELEMENT:
            on_element(co, frame, PCVDOM_ELEMENT_FROM_NODE(ctxt->curr));
            return;
        case PCVDOM_NODE_CONTENT:
            on_content(co, frame, PCVDOM_CONTENT_FROM_NODE(ctxt->curr));
            return;
        case PCVDOM_NODE_COMMENT:
            on_comment(co, frame, PCVDOM_COMMENT_FROM_NODE(ctxt->curr));
            return;
        default:
            PC_ASSERT(0); // Not implemented yet
    }

    PC_ASSERT(0);
}

static struct pcintr_element_ops
ops = {
    .after_pushed       = after_pushed,
    .on_popping         = on_popping,
    .rerun              = NULL,
    .select_child       = select_child,
};

struct pcintr_element_ops* pcintr_get_init_ops(void)
{
    return &ops;
}

