/*
 * @file get_element.c
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
    if (frame->nr_params != 2) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }
    return 0;
}

static purc_variant_t
eval(struct pcvcm_eval_ctxt *ctxt, struct pcvcm_eval_stack_frame *frame)
{
    bool has_index = true;
    int64_t index = -1;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_variant_t inner_ret = PURC_VARIANT_INVALID;

    struct pcvcm_node *caller_node = frame->param_nodes[0];
    purc_variant_t caller_var = pcutils_array_get(frame->params_result, 0);

    struct pcvcm_node *param_node = frame->param_nodes[1];
    purc_variant_t param_var = pcutils_array_get(frame->params_result, 1);

    if (param_node->type == PCVCM_NODE_TYPE_STRING) {
        if (pcutils_parse_int64((const char*)param_node->sz_ptr[1],
                    param_node->sz_ptr[0], &index) != 0) {
            has_index = false;
        }
    }
    else if (!purc_variant_cast_to_longint(param_var, &index, true)) {
        has_index = false;
    }

    unsigned call_flags = pcvcm_eval_ctxt_get_call_flags(ctxt);
    if (pcvcm_eval_is_native_wrapper(caller_var)) {
        purc_variant_t inner_caller = pcvcm_eval_native_wrapper_get_caller(caller_var);
        purc_variant_t inner_param = pcvcm_eval_native_wrapper_get_param(caller_var);
        inner_ret = pcvcm_eval_call_nvariant_method(inner_caller,
                purc_variant_get_string_const(inner_param), 0, NULL,
                GETTER_METHOD, call_flags);
        if (inner_ret) {
            caller_var = inner_ret;
        }
    }

    if (purc_variant_is_object(caller_var)) {
        purc_variant_t val = purc_variant_object_get(caller_var, param_var);
        if (val == PURC_VARIANT_INVALID) {
            goto out;
        }

        if (!purc_variant_is_dynamic(val)) {
            ret_var = val;
            purc_variant_ref(ret_var);
            goto out;
        }

        if (!pcvcm_eval_is_handle_as_getter(frame->node)) {
            ret_var = val;
            purc_variant_ref(ret_var);
            goto out;
        }

        ret_var = pcvcm_eval_call_dvariant_method(caller_var, val, 0, NULL,
                GETTER_METHOD, call_flags);
    }
    else if (purc_variant_is_array(caller_var)) {
        if (!has_index) {
            goto out;
        }
        if (index < 0) {
            size_t len = purc_variant_array_get_size(caller_var);
            index += len;
        }
        if (index < 0) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto out;
        }

        purc_variant_t val = purc_variant_array_get(caller_var, index);
        if (val == PURC_VARIANT_INVALID) {
            goto out;
        }

        if (!purc_variant_is_dynamic(val)) {
            ret_var = val;
            purc_variant_ref(ret_var);
            goto out;
        }

        if (!pcvcm_eval_is_handle_as_getter(frame->node)) {
            ret_var = val;
            purc_variant_ref(ret_var);
            goto out;
        }
        ret_var = pcvcm_eval_call_dvariant_method(caller_var, val, 0, NULL,
                GETTER_METHOD, call_flags);
    }
    else if (purc_variant_is_tuple(caller_var)) {
        if (!has_index) {
            goto out;
        }
        if (index < 0) {
            size_t len = purc_variant_tuple_get_size(caller_var);
            index += len;
        }
        if (index < 0) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto out;
        }

        purc_variant_t val = purc_variant_tuple_get(caller_var, index);
        if (val == PURC_VARIANT_INVALID) {
            goto out;
        }

        if (!purc_variant_is_dynamic(val)) {
            ret_var = val;
            purc_variant_ref(ret_var);
            goto out;
        }

        if (!pcvcm_eval_is_handle_as_getter(frame->node)) {
            ret_var = val;
            purc_variant_ref(ret_var);
            goto out;
        }
        ret_var = pcvcm_eval_call_dvariant_method(caller_var, val, 0, NULL,
                GETTER_METHOD, call_flags);
    }
    else if (purc_variant_is_set(caller_var)) {
        if (!has_index) {
            goto out;
        }
        if (index < 0) {
            size_t len = purc_variant_set_get_size(caller_var);
            index += len;
        }
        if (index < 0) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto out;
        }

        purc_variant_t val = purc_variant_set_get_by_index(caller_var, index);
        if (val == PURC_VARIANT_INVALID) {
            goto out;
        }

        if (!purc_variant_is_dynamic(val)) {
            ret_var = val;
            purc_variant_ref(ret_var);
            goto out;
        }

        if (!pcvcm_eval_is_handle_as_getter(frame->node)) {
            ret_var = val;
            purc_variant_ref(ret_var);
            goto out;
        }
        ret_var = pcvcm_eval_call_dvariant_method(caller_var, val, 0, NULL,
                GETTER_METHOD, call_flags);
    }
    else if (purc_variant_is_dynamic(caller_var)) {
        ret_var = pcvcm_eval_call_dvariant_method(
                pcvcm_eval_get_attach_variant(
                    pcvcm_node_first_child(caller_node)),
                caller_var, 1, &param_var, GETTER_METHOD,
                call_flags);
        goto out;
    }
    else if (purc_variant_is_native(caller_var)) {
        if (!pcvcm_eval_is_handle_as_getter(frame->node)) {
            ret_var = pcvcm_eval_native_wrapper_create(caller_var, param_var);
            goto out;
        }
        ret_var = pcvcm_eval_call_nvariant_method(caller_var,
                purc_variant_get_string_const(param_var), 0, NULL,
                GETTER_METHOD, call_flags);
        goto out;
    }

out:
    if (inner_ret) {
        purc_variant_unref(inner_ret);
    }
    return ret_var;
}


static struct pcvcm_eval_stack_frame_ops ops = {
    .after_pushed = after_pushed,
    .select_param = select_param_default,
    .eval = eval
 };

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_get_element_ops() {
    return &ops;
}

