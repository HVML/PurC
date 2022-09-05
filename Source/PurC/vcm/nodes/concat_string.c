/*
 * @file concat_string.c
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

#define MIN_BUF_SIZE         32
#define MAX_BUF_SIZE         SIZE_MAX

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
        struct pcvcm_eval_stack_frame *frame)
{
    UNUSED_PARAM(ctxt);
    UNUSED_PARAM(frame);
    purc_variant_t ret = PURC_VARIANT_INVALID;
    purc_rwstream_t rws = purc_rwstream_new_buffer(MIN_BUF_SIZE, MAX_BUF_SIZE);
    if (!rws) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }

    for (size_t i = 0; i < frame->nr_params; i++) {
        purc_variant_t v = pcutils_array_get(frame->params_result, i);

        // FIXME: stringify or serialize
        char *buf = NULL;
        int total = purc_variant_stringify_alloc(&buf, v);
        if (total) {
            purc_rwstream_write(rws, buf, total);
        }
        free(buf);
    }

    // do not forget tailing-null-terminator
    purc_rwstream_write(rws, "", 1);

    size_t rw_size = 0;
    size_t content_size = 0;
    char *rw_string = purc_rwstream_get_mem_buffer_ex(rws,
            &content_size, &rw_size, true);

    if (rw_size && rw_string) {
        ret = purc_variant_make_string_reuse_buff(rw_string,
                content_size, false);
        if (ret == PURC_VARIANT_INVALID) {
            pcinst_set_error(PURC_ERROR_INVALID_VALUE);
            ret = PURC_VARIANT_INVALID;
        }
    }

out:
    if (rws) {
        purc_rwstream_destroy(rws);
    }
    return ret;
}


static struct pcvcm_eval_stack_frame_ops ops = {
     after_pushed,
     eval
 };

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_concat_string_ops() {
    return &ops;
}

