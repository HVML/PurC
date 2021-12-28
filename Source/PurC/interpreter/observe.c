/**
 * @file observe.c
 * @author Xue Shuming
 * @date 2021/12/28
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

#define TO_DEBUG 0

struct ctxt_for_observe {
    struct pcvdom_node           *curr;
};

static void
ctxt_for_observe_destroy(struct ctxt_for_observe *ctxt)
{
    if (ctxt) {
        free(ctxt);
    }
}

static void
ctxt_destroy(void *ctxt)
{
    ctxt_for_observe_destroy((struct ctxt_for_observe*)ctxt);
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

    purc_variant_t on;
    on = purc_variant_object_get_by_ckey(frame->attr_vars, "on");
    if (on == PURC_VARIANT_INVALID)
        return NULL;

    purc_variant_t for_var;
    for_var = purc_variant_object_get_by_ckey(frame->attr_vars, "for");
    if (for_var == PURC_VARIANT_INVALID)
        return NULL;

    struct pcintr_observer* observer;
    observer = pcintr_register_observer(on, for_var, pos);
    if (observer == NULL) {
        return NULL;
    }

    struct ctxt_for_observe *ctxt;
    ctxt = (struct ctxt_for_observe*)calloc(1, sizeof(*ctxt));
    if (!ctxt) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    frame->ctxt = ctxt;
    frame->ctxt_destroy = ctxt_destroy;
    purc_clr_error();

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

    struct ctxt_for_observe *ctxt;
    ctxt = (struct ctxt_for_observe*)frame->ctxt;
    if (ctxt) {
        ctxt_for_observe_destroy(ctxt);
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

struct pcintr_element_ops* pcintr_get_observe_ops(void)
{
    return &ops;
}

