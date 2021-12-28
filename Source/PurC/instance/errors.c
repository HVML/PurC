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

#include "private/interpreter.h" // FIXME:

#include <stdarg.h>

static const struct err_msg_info* get_error_info(int errcode);

int purc_get_last_error(void)
{
    const struct pcinst* inst = pcinst_current();
    if (inst) {
        return inst->errcode;
    }

    return PURC_ERROR_NO_INSTANCE;
}

purc_variant_t purc_get_last_error_ex(void)
{
    const struct pcinst* inst = pcinst_current();
    if (inst) {
        return inst->err_exinfo;
    }

    return PURC_VARIANT_INVALID;
}

int purc_set_error_with_location(int errcode, purc_variant_t exinfo,
        const char *file, int lineno, const char *func)
{
    struct pcinst* inst = pcinst_current();
    if (inst == NULL) {
        return PURC_ERROR_NO_INSTANCE;
    }

    inst->errcode = errcode;
    PURC_VARIANT_SAFE_CLEAR(inst->err_exinfo);
    inst->err_exinfo = exinfo;
    inst->file       = file;
    inst->lineno     = lineno;
    inst->func       = func;

    // set the exception info into stack
    pcintr_stack_t stack = purc_get_stack();
    if (stack) {
        const struct err_msg_info* info = get_error_info(errcode);
        if (info == NULL ||
                ((info->flags & PURC_EXCEPT_FLAGS_REQUIRED) && !exinfo)) {
            return PURC_ERROR_INVALID_VALUE;
        }
        stack->error_except = info->except_atom;
        PURC_VARIANT_SAFE_CLEAR(stack->err_except_info);
        stack->err_except_info = exinfo;
        if (exinfo != PURC_VARIANT_INVALID) {
            purc_variant_ref(exinfo);
        }
        stack->file = file;
        stack->lineno = lineno;
        stack->func = func;
        stack->except = errcode ? 1 : 0; // FIXME: when to set stack->error???
    }
    return PURC_ERROR_OK;
}

int
purc_set_error_exinfo_printf(int err_code,
        const char *file, int lineno, const char *func,
        const char *fmt, ...)
{
    char buf[1024];

    va_list ap;
    va_start(ap, fmt);
    int r = vsnprintf(buf, sizeof(buf), fmt, ap);
    // TODO: remove below 2 lines
    PC_ASSERT(r >= 0);
    PC_ASSERT((size_t)r < sizeof(buf));
    (void)r;
    va_end(ap);

    purc_variant_t v;
    v = purc_variant_make_string(buf, true);
    PC_ASSERT(v != PURC_VARIANT_INVALID);

    r = purc_set_error_with_location(err_code, v,
            file, lineno, func);

    return r;
}

static LIST_HEAD(_err_msg_seg_list);

/* Error Messages */
#define UNKNOWN_ERR_CODE    "Unknown Error Code"

const struct err_msg_info* get_error_info(int errcode)
{
    struct list_head *p;

    list_for_each(p, &_err_msg_seg_list) {
        struct err_msg_seg *seg = container_of (p, struct err_msg_seg, list);
        if (errcode >= seg->first_errcode && errcode <= seg->last_errcode) {
            return &seg->info[errcode - seg->first_errcode];
        }
    }

    return NULL;
}


const char* purc_get_error_message(int errcode)
{
    const struct err_msg_info* info = get_error_info(errcode);
    return info ? info->msg : UNKNOWN_ERR_CODE;
}

purc_atom_t purc_get_error_exception(int errcode)
{
    const struct err_msg_info* info = get_error_info(errcode);
    return info ? info->except_atom : 0;
}

void pcinst_register_error_message_segment(struct err_msg_seg* seg)
{
    list_add(&seg->list, &_err_msg_seg_list);
    if (seg->info == NULL) {
        return;
    }

    int count = seg->last_errcode - seg->first_errcode + 1;
    for (int i = 0; i < count; i++) {
        seg->info[i].except_atom = purc_get_except_atom_by_id(
                seg->info[i].except_id);
    }
}

