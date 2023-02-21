/*
 * @file object.c
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
    if (frame->nr_params % 2 != 0) {
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
    purc_variant_t object = purc_variant_make_object(0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    if (object == PURC_VARIANT_INVALID) {
        goto out;
    }

    for (size_t i = 0; i < frame->nr_params; i += 2) {
        purc_variant_t key = pcvcm_get_frame_result(ctxt, frame->idx, i);
        purc_variant_t value = pcvcm_get_frame_result(ctxt, frame->idx, i + 1);
        if (!purc_variant_object_set(object, key, value)) {
            goto out;
        }
    }

out:
    return object;
}


static struct pcvcm_eval_stack_frame_ops ops = {
    .after_pushed = after_pushed,
    .select_param = select_param_default,
    .eval = eval
};

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_object_ops() {
    return &ops;
}

