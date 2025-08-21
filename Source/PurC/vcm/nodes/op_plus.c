/*
 * @file op_plus.c
 * @author XueShuming
 * @date 2025/08/21
 * @brief The impl of ops for plus operator vcm node.
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
    
    // Get left and right operands
    purc_variant_t left = pcvcm_get_frame_result(ctxt, frame->idx, 0, NULL);
    purc_variant_t right = pcvcm_get_frame_result(ctxt, frame->idx, 1, NULL);
    
    if (left == PURC_VARIANT_INVALID || right == PURC_VARIANT_INVALID) {
        return PURC_VARIANT_INVALID;
    }
    
    // Handle number + number
    if (purc_variant_is_number(left) && purc_variant_is_number(right)) {
        double left_val = purc_variant_numerify(left);
        double right_val = purc_variant_numerify(right);
        return purc_variant_make_number(left_val + right_val);
    }
    
    // Handle string concatenation
    if (purc_variant_is_string(left) || purc_variant_is_string(right)) {
        const char *left_str = purc_variant_get_string_const(left);
        const char *right_str = purc_variant_get_string_const(right);
        
        if (!left_str || !right_str) {
            return PURC_VARIANT_INVALID;
        }
        
        size_t left_len = strlen(left_str);
        size_t right_len = strlen(right_str);
        char *result = malloc(left_len + right_len + 1);
        
        if (!result) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            return PURC_VARIANT_INVALID;
        }
        
        strcpy(result, left_str);
        strcat(result, right_str);
        
        purc_variant_t ret = purc_variant_make_string(result, true);
        free(result);
        return ret;
    }
    
    // For other types, try to convert to numbers
    double left_val = purc_variant_numerify(left);
    double right_val = purc_variant_numerify(right);
    
    if (isnan(left_val) || isnan(right_val)) {
        return PURC_VARIANT_INVALID;
    }
    
    return purc_variant_make_number(left_val + right_val);
}

static struct pcvcm_eval_stack_frame_ops ops = {
    .after_pushed = after_pushed,
    .select_param = select_param_default,
    .eval = eval
};

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_plus_ops() {
    return &ops;
}
