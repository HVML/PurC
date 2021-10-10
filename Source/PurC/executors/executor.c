/*
 * @file executor.c
 * @author Xu Xiaohong
 * @date 2021/10/10
 * @brief The implementation of public part for executor.
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

#include "private/executor.h"
#include "private/instance.h"
#include "private/errors.h"

static const char* executor_err_msgs[] = {
    /* PCEXECUTOR_INVALID_TYPE */
    "Invalid executor type",
};

static struct err_msg_seg _executor_err_msgs_seg = {
    { NULL, NULL },
    PURC_ERROR_FIRST_EXECUTOR,
    PURC_ERROR_FIRST_EXECUTOR + PCA_TABLESIZE(executor_err_msgs) - 1,
    executor_err_msgs
};


void pcexecutor_init_once(void)
{
    // register error message
    pcinst_register_error_message_segment(&_executor_err_msgs_seg);

    // initialize others
}

void pcexecutor_init_instance(struct pcinst *inst)
{
    struct pcexecutor_heap *heap = &inst->executor_heap;
    UNUSED_PARAM(heap);
}

void pcexecutor_cleanup_instance(struct pcinst *inst)
{
    struct pcexecutor_heap *heap = &inst->executor_heap;
    UNUSED_PARAM(heap);
}

