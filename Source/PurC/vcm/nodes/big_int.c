/*
 * @file big_int.c
 * @author XueShuming
 * @date 2025/08/19
 * @brief The impl of ops for bigint vcm node.
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
    return 0;
}

static purc_variant_t
eval(struct pcvcm_eval_ctxt *ctxt,
        struct pcvcm_eval_stack_frame *frame, const char **name)
{
    UNUSED_PARAM(ctxt);
    UNUSED_PARAM(frame);
    UNUSED_PARAM(name);

    struct pcvcm_node *node = frame->node;
    char *str = (char*)node->sz_ptr[1];
    return purc_variant_make_bigint_from_string(str, NULL, node->int_base);
}


static struct pcvcm_eval_stack_frame_ops ops = {
    .after_pushed = after_pushed,
    .select_param = select_param_default,
    .eval = eval
 };

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_big_int_ops() {
    return &ops;
}

