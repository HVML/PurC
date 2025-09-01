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
#include "purc-variant.h"

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
    if (strcmp(name, PCVCM_VARIABLE_ARGS_NAME) != 0) {
        goto out;
    }

    for (int i = ctxt->frame_idx; i >= 0; i--) {
        struct pcvcm_eval_stack_frame *p = ctxt->frames + i;
        if (p->args) {
            ret = p->args;
            break;
        }
    }

out:
    purc_clr_error();
    return ret;
}

static purc_variant_t
eval(struct pcvcm_eval_ctxt *ctxt,
        struct pcvcm_eval_stack_frame *frame, const char **name_out)
{
    UNUSED_PARAM(name_out);
    purc_variant_t ret = PURC_VARIANT_INVALID;
    purc_variant_t name = pcvcm_get_frame_result(ctxt, frame->idx, 0, NULL);
    if (name == PURC_VARIANT_INVALID || !purc_variant_is_string(name)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    if(!ctxt->find_var) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    const char *sname = purc_variant_get_string_const(name);
#ifdef PCVCM_KEEP_NAME
    if (name_out) {
        *name_out = sname;
    }
#endif
    ret = find_from_frame(ctxt, sname);
    if (!ret) {
        ret = ctxt->find_var(ctxt->find_var_ctxt, sname);
    }

out:
    if (ret) {
        purc_variant_ref(ret);
    }
    else if (frame->node->extra & EXTRA_ASSIGN_FLAG) {
        ret = purc_variant_make_undefined();
        pcutils_map_replace_or_insert(ctxt->node_var_name_map, frame->node,
                                      sname, NULL);
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

