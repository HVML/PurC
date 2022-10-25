/*
 * @file get_variable.c
 * @author XueShuming
 * @date 2021/09/02
 * @brief The impl of ops for undefind vcm node.
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
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "config.h"
#include "purc-utils.h"
#include "purc-errors.h"
#include "purc-rwstream.h"

#include "private/errors.h"
#include "private/stack.h"
#include "private/interpreter.h"
#include "private/utils.h"
#include "private/vcm.h"

#include "../eval.h"
#include "../ops.h"

static int
after_pushed(struct pcvcm_eval_ctxt *ctxt,
        struct pcvcm_eval_stack_frame *frame)
{
    UNUSED_PARAM(ctxt);
    UNUSED_PARAM(frame);
    if (frame->nr_params != 1) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }
    return 0;
}

static purc_variant_t
find_from_frame(struct pcvcm_eval_ctxt *ctxt, const char *name)
{
    purc_variant_t ret = PURC_VARIANT_INVALID;
    struct list_head *stack = &ctxt->stack;
    struct pcvcm_eval_stack_frame *p, *n;
    list_for_each_entry_reverse_safe(p, n, stack, ln) {
        ret = pcvarmgr_get(p->variables, name);
        if (ret) {
            goto out;
        }
    }

out:
    purc_clr_error();
    return ret;
}

static purc_variant_t
eval(struct pcvcm_eval_ctxt *ctxt,
        struct pcvcm_eval_stack_frame *frame)
{
    purc_variant_t ret = PURC_VARIANT_INVALID;
    purc_variant_t name = pcutils_array_get(frame->params_result, 0);
    if (name == PURC_VARIANT_INVALID || !purc_variant_is_string(name)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    if(!ctxt->find_var) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    const char *sname = purc_variant_get_string_const(name);
    ret = find_from_frame(ctxt, sname);
    if (!ret) {
        ret = ctxt->find_var(ctxt->find_var_ctxt, sname);
    }

out:
    if (ret) {
        purc_variant_ref(ret);
    }
    return ret;
}


static struct pcvcm_eval_stack_frame_ops ops = {
    .after_pushed = after_pushed,
    .select_param = select_param_default,
    .eval = eval
};

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_get_variable_ops() {
    return &ops;
}

