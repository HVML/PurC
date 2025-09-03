/*
 * @file op_comma.c
 * @author XueShuming
 * @date 2025/08/21
 * @brief The impl of ops for comma operator vcm node.
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
#include "private/variant.h"
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
    UNUSED_PARAM(ctxt);
    UNUSED_PARAM(frame);
    UNUSED_PARAM(name);

    size_t nr_params = frame->nr_params;
    purc_variant_t arr = purc_variant_make_array_0();
    if (arr == PURC_VARIANT_INVALID) {
        return PURC_VARIANT_INVALID;
    }

    for (size_t i = 0; i < nr_params; i++) {
        struct pcvcm_eval_node *eval_node = select_param_default(ctxt, frame, i);
        if (eval_node->node->type == PCVCM_NODE_TYPE_OP_LP
            || eval_node->node->type == PCVCM_NODE_TYPE_OP_RP) {
            continue;
        }
        purc_variant_t val = pcvcm_get_frame_result(ctxt, frame->idx, i, NULL);
        purc_variant_array_append(arr, val);
    }

    size_t nr_size = purc_variant_array_get_size(arr);
    purc_variant_t tuple = purc_variant_make_tuple(nr_size, NULL);
    if (tuple == PURC_VARIANT_INVALID) {
        purc_variant_unref(arr);
        return PURC_VARIANT_INVALID;
    }

    for (size_t i = 0; i < nr_size; i++) {
        purc_variant_tuple_set(tuple, i, purc_variant_array_get(arr, i));
    }
    purc_variant_unref(arr);

    return tuple;
}

static struct pcvcm_eval_stack_frame_ops ops = {
    .after_pushed = after_pushed,
    .select_param = select_param_default,
    .eval = eval
};

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_comma_ops() {
    return &ops;
}
