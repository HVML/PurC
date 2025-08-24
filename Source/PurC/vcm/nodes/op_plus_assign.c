/*
 * @file op_plus_assign.c
 * @author XueShuming
 * @date 2025/08/21
 * @brief The impl of ops for plus assign operator vcm node.
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
    UNUSED_PARAM(ctxt);
    UNUSED_PARAM(frame);
    UNUSED_PARAM(name);

    purc_variant_t left = pcvcm_get_frame_result(ctxt, frame->idx, 0, NULL);
    purc_variant_t right = pcvcm_get_frame_result(ctxt, frame->idx, 0, NULL);

    if (left == PURC_VARIANT_INVALID || right == PURC_VARIANT_INVALID) {
        return PURC_VARIANT_INVALID;
    }

    enum purc_variant_type ltype = purc_variant_get_type(left);
    enum purc_variant_type rtype = purc_variant_get_type(right);

    if ((ltype == PURC_VARIANT_TYPE_STRING ||
         ltype == PURC_VARIANT_TYPE_BSEQUENCE) &&
        (rtype == PURC_VARIANT_TYPE_STRING ||
         rtype == PURC_VARIANT_TYPE_BSEQUENCE)) {
        purc_variant_operator_iconcat(left, right);
        goto out;
    }

    if ((ltype == PURC_VARIANT_TYPE_ARRAY || ltype == PURC_VARIANT_TYPE_TUPLE) &&
        (rtype == PURC_VARIANT_TYPE_ARRAY || rtype == PURC_VARIANT_TYPE_TUPLE ||
         rtype == PURC_VARIANT_TYPE_SET)) {
        purc_variant_operator_iconcat(left, right);
        goto out;
    }

    if (ltype == PURC_VARIANT_TYPE_OBJECT && rtype == PURC_VARIANT_TYPE_OBJECT) {
        purc_variant_object_unite(left,right,PCVRNT_CR_METHOD_OVERWRITE);
        goto out;
    }

    if ((ltype == PURC_VARIANT_TYPE_SET) && (rtype == PURC_VARIANT_TYPE_ARRAY ||
        rtype == PURC_VARIANT_TYPE_SET || rtype == PURC_VARIANT_TYPE_TUPLE)) {
        purc_variant_set_unite(left, right, PCVRNT_CR_METHOD_OVERWRITE);
        goto out;
    }

    purc_variant_operator_iadd(left, right);

out:
    return left ? purc_variant_ref(left) : PURC_VARIANT_INVALID;
}

static struct pcvcm_eval_stack_frame_ops ops = {
    .after_pushed = after_pushed,
    .select_param = select_param_default,
    .eval = eval
};

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_plus_assign_ops() {
    return &ops;
}
