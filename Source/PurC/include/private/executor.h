/**
 * @file executor.h
 * @author Xu Xiaohong
 * @date 2021/10/10
 * @brief The internal interfaces for executor.
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

#ifndef PURC_PRIVATE_EXECUTOR_H
#define PURC_PRIVATE_EXECUTOR_H

#include "config.h"

#include "purc-macros.h"
#include "purc-errors.h"
#include "purc-executor.h"

#include "private/map.h"

PCA_EXTERN_C_BEGIN

struct pcexecutor_heap {
    struct pcutils_map *executors;
};

// 执行器实例
struct purc_exec_inst {
   enum purc_exec_type         type;
   purc_variant_t              input;
   bool                        asc_desc;
};

// 用于迭代的迭代器
struct purc_exec_iter {
    struct purc_exec_inst     *inst;
};


// initialize executor module (once)
void pcexecutor_init_once(void) WTF_INTERNAL;

struct pcinst;

// initialize the executor module for a PurC instance.
void pcexecutor_init_instance(struct pcinst* inst) WTF_INTERNAL;
// clean up the executor module for a PurC instance.
void pcexecutor_cleanup_instance(struct pcinst* inst) WTF_INTERNAL;

PCA_EXTERN_C_END

#endif // PURC_PRIVATE_EXECUTOR_H

