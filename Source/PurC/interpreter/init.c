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

static int
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

    return ok ? 0 : -1;
}

static int
post_process_array(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        purc_variant_t name)
{
    purc_variant_t via;
    via = purc_variant_object_get_by_ckey(frame->attr_vars, "via");
    purc_clr_error();
    purc_variant_t set;
    set = purc_variant_make_set(0, via, PURC_VARIANT_INVALID);
    if (set == PURC_VARIANT_INVALID)
        return -1;

    purc_variant_t val;
    foreach_value_in_variant_array(frame->ctnt_var, val)
        bool ok = purc_variant_is_type(val, PURC_VARIANT_TYPE_OBJECT);
        if (ok) {
            ok = purc_variant_set_add(set, val, true);
        }
        if (!ok) {
            purc_variant_unref(set);
            return -1;
        }
    end_foreach;

    return post_process_bind_scope_var(co, frame, name, set);
}

static int
post_process_object(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        purc_variant_t name)
{
    purc_variant_t val = frame->ctnt_var;

    purc_variant_ref(val);
    return post_process_bind_scope_var(co, frame, name, val);
}

static int
post_process(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    purc_variant_t name;
    name = purc_variant_object_get_by_ckey(frame->attr_vars, "as");
    if (name == PURC_VARIANT_INVALID)
        return -1;

    if (purc_variant_is_type(frame->ctnt_var, PURC_VARIANT_TYPE_ARRAY)) {
        return post_process_array(co, frame, name);
    }
    if (purc_variant_is_type(frame->ctnt_var, PURC_VARIANT_TYPE_OBJECT)) {
        return post_process_object(co, frame, name);
    }

    purc_set_error(PURC_ERROR_NOT_EXISTS);
    return -1;
}

static void*
after_pushed(pcintr_stack_t stack, pcvdom_element_t pos)
{
    PC_ASSERT(stack && pos);
    PC_ASSERT(stack == purc_get_stack());

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    frame->pos = pos; // ATTENTION!!

    struct pcvdom_element *element = frame->pos;
    PC_ASSERT(element);
    D("<%s>", element->tag_name);

    int r;
    r = pcintr_element_eval_attrs(frame, element);
    if (r)
        return NULL;

    struct pcvcm_node *vcm_content = element->vcm_content;
    PC_ASSERT(vcm_content);

    purc_variant_t v = pcvcm_eval(vcm_content, stack);
    if (v == PURC_VARIANT_INVALID)
        return NULL;

    PURC_VARIANT_SAFE_CLEAR(frame->ctnt_var);
    frame->ctnt_var = v;

    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)calloc(1, sizeof(*ctxt));
    if (!ctxt) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    frame->ctxt = ctxt;
    frame->ctxt_destroy = ctxt_destroy;

    while ((element=pcvdom_element_parent(element))) {
        if (element->tag_id == PCHVML_TAG_HEAD) {
            ctxt->under_head = 1;
        }
    }

    r = post_process(&stack->co, frame);
    if (r)
        return NULL;

    return ctxt;
}

static bool
on_popping(pcintr_stack_t stack, void* ctxt)
{
    PC_ASSERT(stack);
    PC_ASSERT(stack == purc_get_stack());

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame);
    PC_ASSERT(ctxt == frame->ctxt);

    struct pcvdom_element *element = frame->pos;
    PC_ASSERT(element);

    struct ctxt_for_init *init_ctxt;
    init_ctxt = (struct ctxt_for_init*)frame->ctxt;
    if (init_ctxt) {
        ctxt_for_init_destroy(init_ctxt);
        frame->ctxt = NULL;
    }

    D("</%s>", element->tag_name);
    return true;
}

static struct pcintr_element_ops
ops = {
    .after_pushed       = after_pushed,
    .on_popping         = on_popping,
    .rerun              = NULL,
    .select_child       = NULL,
};

struct pcintr_element_ops* pcintr_get_init_ops(void)
{
    return &ops;
}

