/*
 * @file op_conditional.c
 * @author XueShuming
 * @date 2025/08/21
 * @brief The impl of ops for conditional operator vcm node.
 *
 * Copyright (C) 2025 FMSoft <https://www.fmsoft.cn>
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
    return 0;
}

static purc_variant_t
eval(struct pcvcm_eval_ctxt *ctxt,
        struct pcvcm_eval_stack_frame *frame, const char **name)
{
    UNUSED_PARAM(name);

    // Get condition, true_expr, and false_expr operands
    purc_variant_t condition = pcvcm_get_frame_result(ctxt, frame->idx, 0, NULL);
    purc_variant_t true_expr = pcvcm_get_frame_result(ctxt, frame->idx, 1, NULL);
    purc_variant_t false_expr = pcvcm_get_frame_result(ctxt, frame->idx, 2, NULL);

    if (condition == PURC_VARIANT_INVALID ||
        true_expr == PURC_VARIANT_INVALID ||
        false_expr == PURC_VARIANT_INVALID) {
        return PURC_VARIANT_INVALID;
    }

    bool cond_val = purc_variant_operator_truth(condition);

    return cond_val ? purc_variant_ref(true_expr) : purc_variant_ref(false_expr);
}

static struct pcvcm_eval_stack_frame_ops ops = {
    .after_pushed = after_pushed,
    .select_param = select_param_default,
    .eval = eval
};

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_conditional_ops() {
    return &ops;
}
