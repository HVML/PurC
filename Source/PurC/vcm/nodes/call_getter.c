/*
 * @file call_getter.c
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
    if (frame->nr_params < 1) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }
    return 0;
}

static purc_variant_t
eval(struct pcvcm_eval_ctxt *ctxt,
        struct pcvcm_eval_stack_frame *frame)
{
    UNUSED_PARAM(ctxt);
    UNUSED_PARAM(frame);
    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    size_t nr_params = frame->nr_params - 1;
    purc_variant_t params[nr_params];

    struct pcvcm_eval_node *enode = frame->ops->select_param(ctxt, frame, 0);
    struct pcvcm_node *caller_node = enode->node;
    purc_variant_t caller_var = pcvcm_get_frame_result(ctxt, frame, 0);

    if (!purc_variant_is_dynamic(caller_var)
            && !pcvcm_eval_is_native_wrapper(caller_var)) {
        goto out;
    }

    unsigned call_flags = pcvcm_eval_ctxt_get_call_flags(ctxt);

    if (nr_params > 0) {
        for (size_t i = 1, j = 0; i < frame->nr_params; i++, j++) {
            params[j] = pcvcm_get_frame_result(ctxt, frame, i);
        }
    }

    if (purc_variant_is_dynamic(caller_var)) {
        ret_var = pcvcm_eval_call_dvariant_method(
                pcvcm_eval_get_attach_variant(
                    pcvcm_node_first_child(caller_node)),
                caller_var, nr_params, params, GETTER_METHOD, call_flags);
    }
    else if (pcvcm_eval_is_native_wrapper(caller_var)) {
        purc_variant_t nv = pcvcm_eval_native_wrapper_get_caller(caller_var);
        if (purc_variant_is_native(nv)) {
            purc_variant_t name = pcvcm_eval_native_wrapper_get_param(caller_var);
            if (name) {
                ret_var = pcvcm_eval_call_nvariant_method(nv,
                        purc_variant_get_string_const(name), nr_params,
                        params, GETTER_METHOD, call_flags);
            }
        }
    }

out:
    return ret_var;
}

static struct pcvcm_eval_stack_frame_ops ops = {
    .after_pushed = after_pushed,
    .select_param = select_param_default,
    .eval = eval
 };

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_call_getter_ops() {
    return &ops;
}

