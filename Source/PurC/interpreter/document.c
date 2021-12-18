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

#include "internal.h"

#include "private/debug.h"
#include "private/interpreter.h"

#include "ops.h"

static void
after_pushed(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    fprintf(stderr, "==%s[%d]==\n", __FILE__, __LINE__);
    pcintr_stack_t stack = co->stack;
    purc_vdom_t vdom = stack->vdom;
    struct pcvdom_document *document = vdom->document;
    struct pcvdom_doctype *doctype = &document->doctype;
    // TODO: load external libraries
    if (doctype) {
        const char *system_info = doctype->system_info;
        (void)system_info;
    }

    struct pcvdom_element *hvml = document->root;
    PC_ASSERT(hvml);
    PC_ASSERT(hvml->tag_id == PCHVML_TAG_HVML);

    struct pcintr_stack_frame *child_frame;
    child_frame = push_stack_frame(stack);
    if (!child_frame) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return;
    }
    child_frame->ops = pcintr_get_ops_by_element(hvml);
    child_frame->scope = hvml;

    frame->ctxt = hvml;
    frame->next_step = NEXT_STEP_ON_POPPING;
    co->state = CO_STATE_READY;
}

static void
on_popping(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    fprintf(stderr, "==%s[%d]==\n", __FILE__, __LINE__);
    UNUSED_PARAM(frame);
    pcintr_stack_t stack = co->stack;
    pop_stack_frame(stack);
}

static struct pcintr_element_ops
ops = {
    .after_pushed       = after_pushed,
    .on_popping         = on_popping,
    .rerun              = NULL,
    .select_child       = NULL,
};

struct pcintr_element_ops pcintr_get_document_ops(void)
{
    return ops;
}


