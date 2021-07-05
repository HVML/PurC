/*
 * @file errors.c
 * @author Vincent Wei (https://github.com/VincentWei)
 * @date 2021/07/02
 * @brief The get/set error code of PurC.
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

#include "purc-errors.h"

#include "private/errors.h"
#include "private/instance.h"

int purc_get_last_error(void)
{
    const struct pcinst* inst = pcinst_current();
    if (inst) {
        return inst->errcode;
    }

    return PURC_ERROR_NO_INSTANCE;
}

int pcinst_set_error(int errcode)
{
    struct pcinst* inst = pcinst_current();
    if (inst) {
        inst->errcode = errcode;
        return PURC_ERROR_OK;
    }

    return PURC_ERROR_NO_INSTANCE;
}

static LIST_HEAD(_err_msg_seg_list);

/* Error Messages */
#define UNKNOWN_ERR_CODE    "Unknown Error Code"

const char* purc_get_error_message(int errcode)
{
    struct list_head *p;

    list_for_each(p, &_err_msg_seg_list) {
        struct err_msg_seg *seg = (struct err_msg_seg *)p;
        if (errcode >= seg->first_errcode && errcode <= seg->last_errcode) {
            return seg->msgs[errcode - seg->first_errcode];
        }
    }

    return UNKNOWN_ERR_CODE;
}

void pcinst_register_error_message_segment(struct err_msg_seg* seg)
{
    list_add (&seg->list, &_err_msg_seg_list);
}

