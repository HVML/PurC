/*
 * @file cjsonee.c
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

static bool
is_cjsonee_op(struct pcvcm_node *node)
{
    if (!node) {
        return false;
    }
    switch (node->type) {
    case PCVCM_NODE_TYPE_CJSONEE_OP_AND:
    case PCVCM_NODE_TYPE_CJSONEE_OP_OR:
    case PCVCM_NODE_TYPE_CJSONEE_OP_SEMICOLON:
        return true;
    default:
        return false;
    }
    return false;
}

static int
after_pushed(struct pcvcm_eval_ctxt *ctxt,
        struct pcvcm_eval_stack_frame *frame)
{
   UNUSED_PARAM(ctxt);
   UNUSED_PARAM(frame);
   return 0;
}


struct pcvcm_node *
select_param(struct pcvcm_eval_ctxt *ctxt,
        struct pcvcm_eval_stack_frame *frame, size_t pos)
{
    UNUSED_PARAM(ctxt);
    purc_variant_t curr_val = PURC_VARIANT_INVALID;
    struct pcvcm_node *param = pcutils_array_get(frame->params, pos);
    bool is_op = is_cjsonee_op(param);
    if (!is_op) {
        goto out;
    }

    if ((pos % 2 == 0) && is_op) {
        pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
        param = NULL;
        goto out;
    }

    for (int i = pos -1; i >= 0; i -= 2) {
        curr_val = pcutils_array_get(frame->params_result, i);
        if (curr_val) {
            break;
        }
    }

    switch (param->type) {
    case PCVCM_NODE_TYPE_CJSONEE_OP_SEMICOLON:
        param = NULL;
        break;

    case PCVCM_NODE_TYPE_CJSONEE_OP_AND:
        param = NULL;
        if (!purc_variant_booleanize(curr_val)) {
            frame->pos++;
            goto out;
        }
        break;

    case PCVCM_NODE_TYPE_CJSONEE_OP_OR:
        param = NULL;
        if (purc_variant_booleanize(curr_val)) {
            frame->pos++;
            goto out;
        }
        break;

    default:
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        param = NULL;
        goto out;
    }

out:
    return param;
}

static purc_variant_t
eval(struct pcvcm_eval_ctxt *ctxt,
        struct pcvcm_eval_stack_frame *frame)
{
    UNUSED_PARAM(ctxt);
    UNUSED_PARAM(frame);
    purc_variant_t curr_val = PURC_VARIANT_INVALID;
    for (int i = frame->nr_params - 1; i >= 0; i--) {
        curr_val = pcutils_array_get(frame->params_result, i);
        if (curr_val && (i % 2 == 0)) {
            break;
        }
    }
    purc_variant_ref(curr_val);
    return curr_val;
}


static struct pcvcm_eval_stack_frame_ops ops = {
    .after_pushed = after_pushed,
    .select_param = select_param,
    .eval = eval
 };

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_cjsonee_ops() {
    return &ops;
}

