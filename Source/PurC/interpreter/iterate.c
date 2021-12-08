/**
 * @file iterate.c
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

#include "iterate.h"

struct ctxt_for_iterate {
    // the instance of the current executor.
    purc_exec_inst_t exec_inst;

    // the iterator if the current element is `iterate`.
    purc_exec_iter_t it;
};

static inline void *
iterate_after_pushed(pcintr_stack_t stack, pcvdom_element_t pos,
        struct ctxt_for_iterate *ctxt)
{
    UNUSED_PARAM(stack);
    UNUSED_PARAM(pos);
    UNUSED_PARAM(ctxt);
    PC_ASSERT(0); // Not implemented yet
    return NULL;
}

// called after pushed
static inline void *
after_pushed(pcintr_stack_t stack, pcvdom_element_t pos)
{
    struct ctxt_for_iterate *ctxt;
    ctxt = (struct ctxt_for_iterate*)calloc(1, sizeof(*ctxt));
    if (!ctxt)
        return NULL;

    return iterate_after_pushed(stack, pos, ctxt);
}

// called on popping
static inline bool
on_popping(pcintr_stack_t stack, void* ctxt)
{
    UNUSED_PARAM(stack);
    UNUSED_PARAM(ctxt);
    PC_ASSERT(0); // Not implemented yet
    return false;
}

// called to rerun
static inline bool
rerun(pcintr_stack_t stack, void* ctxt)
{
    UNUSED_PARAM(stack);
    UNUSED_PARAM(ctxt);
    PC_ASSERT(0); // Not implemented yet
    return false;
}

// called after executed
static inline pcvdom_element_t
select_child(pcintr_stack_t stack, void* ctxt)
{
    UNUSED_PARAM(stack);
    UNUSED_PARAM(ctxt);
    PC_ASSERT(0); // Not implemented yet
    return NULL;
}

static struct pcintr_element_ops
ops = {
    .after_pushed       = after_pushed,
    .on_popping         = on_popping,
    .rerun              = rerun,
    .select_child       = select_child,
};

struct pcintr_element_ops* pcintr_iterate_get_ops(void)
{
    return &ops;
}

