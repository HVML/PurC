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

#include "config.h"

#include "private/debug.h"
#include "private/executor.h"

#include "archetype.h"

struct ctxt_for_archetype {
    struct purc_exec_ops     ops;

    // the instance of the current executor.
    purc_exec_inst_t exec_inst;

    // the iterator if the current element is `archetype`.
    purc_exec_iter_t it;

    struct pcvdom_element       *curr;

    unsigned int                uniquely:1;
    unsigned int                case_insensitive:1;
};

static inline void
ctxt_release(struct ctxt_for_archetype *ctxt)
{
    if (!ctxt)
        return;

    if (ctxt->exec_inst) {
        struct purc_exec_ops *ops = &ctxt->ops;

        if (ops->destroy) {
            ops->destroy(ctxt->exec_inst);
        }

        ctxt->exec_inst = NULL;
    }
}

static inline void
ctxt_destroy(struct ctxt_for_archetype *ctxt)
{
    if (ctxt) {
        ctxt_release(ctxt);
        free(ctxt);
    }
}

static inline int
archetype_after_pushed_name(pcintr_stack_t stack, pcvdom_element_t pos,
        struct ctxt_for_archetype *ctxt,
        purc_variant_t name)
{
    PC_ASSERT(name && purc_variant_is_type(name, PURC_VARIANT_TYPE_STRING));

    const char *s_name  = purc_variant_get_string_const(name);

    // FIXME: children shall be vcm
    PC_ASSERT(0); // Not implemented yet
    UNUSED_PARAM(s_name);
    UNUSED_PARAM(stack);
    UNUSED_PARAM(pos);
    UNUSED_PARAM(ctxt);
    return -1;
}

static inline int
archetype_after_pushed(pcintr_stack_t stack, pcvdom_element_t pos,
        struct ctxt_for_archetype *ctxt)
{
    struct pcvdom_attr *src = pcvdom_element_find_attr(pos, "src");
    PC_ASSERT(src == NULL); // Not implemented yet

    purc_variant_t name  = pcvdom_element_eval_attr_val(pos, "name");

    int r = archetype_after_pushed_name(stack, pos, ctxt, name);

    purc_variant_unref(name);

    return r ? -1 : 0;
}

// called after pushed
static inline void *
after_pushed(pcintr_stack_t stack, pcvdom_element_t pos)
{
    struct ctxt_for_archetype *ctxt;
    ctxt = (struct ctxt_for_archetype*)calloc(1, sizeof(*ctxt));
    if (!ctxt)
        return NULL;

    int r = archetype_after_pushed(stack, pos, ctxt);
    if (r) {
        ctxt_destroy(ctxt);
        return NULL;
    }

    return ctxt;
}

// called on popping
static inline bool
on_popping(pcintr_stack_t stack, void* ctxt)
{
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame->ctxt == ctxt);

    struct ctxt_for_archetype *archetype_ctxt;
    archetype_ctxt = (struct ctxt_for_archetype*)ctxt;

    ctxt_destroy(archetype_ctxt);
    frame->ctxt = NULL;

    return false;
}

static struct pcintr_element_ops
ops = {
    .after_pushed       = after_pushed,
    .on_popping         = on_popping,
    .rerun              = NULL,
    .select_child       = NULL,
};

struct pcintr_element_ops* pcintr_archetype_get_ops(void)
{
    return &ops;
}

