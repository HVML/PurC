/**
 * @file archetype.c
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

#define TO_DEBUG 0

struct ctxt_for_archetype {
    struct pcvdom_node           *curr;
};

static void
ctxt_for_archetype_destroy(struct ctxt_for_archetype *ctxt)
{
    if (ctxt) {
        free(ctxt);
    }
}

static void
ctxt_destroy(void *ctxt)
{
    ctxt_for_archetype_destroy((struct ctxt_for_archetype*)ctxt);
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

    r = pcintr_element_eval_vcm_content(frame, element);
    if (r)
        return NULL;

    purc_variant_t name;
    name = purc_variant_object_get_by_ckey(frame->attr_vars, "name", false);
    if (name == PURC_VARIANT_INVALID)
        return NULL;

    const char *s_name = purc_variant_get_string_const(name);
    if (s_name == NULL)
        return NULL;

    struct pcvdom_element *scope = frame->scope;
    PC_ASSERT(scope);

    bool ok;
    ok = pcintr_bind_scope_variable(scope, s_name, frame->ctnt_var);
    if (!ok)
        return NULL;

    struct ctxt_for_archetype *ctxt;
    ctxt = (struct ctxt_for_archetype*)calloc(1, sizeof(*ctxt));
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

    struct ctxt_for_archetype *ctxt;
    ctxt = (struct ctxt_for_archetype*)frame->ctxt;
    if (ctxt) {
        ctxt_for_archetype_destroy(ctxt);
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

struct pcintr_element_ops* pcintr_get_archetype_ops(void)
{
    return &ops;
}

