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

    unsigned int       debug_flex:1;
    unsigned int       debug_bison:1;
};

// 用于迭代的迭代器
struct purc_exec_iter {
    size_t                     curr;
    unsigned int               valid:1;
};

// 执行器实例
struct purc_exec_inst {
    enum purc_exec_type         type;
    struct purc_exec_iter       it; // FIXME: one `it` for one `exec_inst`

    purc_variant_t              input;
    bool                        asc_desc;

    purc_variant_t              selected_keys;

    char                       *err_msg;

    purc_variant_t              value;
};

struct pcinst;

void pcexecutor_set_debug(int debug_flex, int debug_bison);
void pcexecutor_get_debug(int *debug_flex, int *debug_bison);

void pcexecutor_inst_reset(struct purc_exec_inst *inst);


PCA_EXTERN_C_END

#endif // PURC_PRIVATE_EXECUTOR_H

