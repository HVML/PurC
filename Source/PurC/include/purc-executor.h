/**
 * @file purc-executor.h
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

#ifndef PURC_PURC_EXECUTOR_H
#define PURC_PURC_EXECUTOR_H

#include "config.h"

#include "purc-macros.h"
#include "purc-errors.h"
#include "purc-variant.h"

PCA_EXTERN_C_BEGIN

// 执行器实例
struct purc_exec_inst;
typedef struct purc_exec_inst purc_exec_inst;
typedef struct purc_exec_inst* purc_exec_inst_t;

// 用于迭代的迭代器
struct purc_exec_iter;
typedef struct purc_exec_iter purc_exec_iter;
typedef struct purc_exec_iter* purc_exec_iter_t;

enum purc_exec_type {
    PURC_EXEC_TYPE_CHOOSE = 0,
    PURC_EXEC_TYPE_ITERATE,
    PURC_EXEC_TYPE_REDUCE,
};

// 内置执行器操作集
typedef struct purc_exec_ops {
    // 创建一个执行器实例
    purc_exec_inst_t (*create) (enum purc_exec_type type,
            purc_variant_t input, bool asc_desc);

    // 用于执行选择
    purc_variant_t (*choose) (purc_exec_inst_t inst, const char* rule);

    // 获得用于迭代的初始迭代子
    purc_exec_iter_t (*it_begin) (purc_exec_inst_t inst, const char* rule);

    // 根据迭代子获得对应的变体值
    purc_variant_t (*it_value) (purc_exec_inst_t inst, purc_exec_iter_t it);

    // 获得下一个迭代子
    // 注意: 规则字符串可能在前后两次迭代中发生变化，比如在规则中引用了变量的情形下。
    // 如果规则并没有发生变化，则对 `rule` 参数传递 NULL。
    purc_exec_iter_t (*it_next) (purc_exec_inst_t inst, purc_exec_iter_t it,
             const char* rule);

    // FIXME:
    // 销毁当前迭代子
    bool (*it_destroy) (purc_exec_inst_t inst, purc_exec_iter_t it);

    // 用于执行规约
    purc_variant_t (*reduce) (purc_exec_inst_t inst, const char* rule);

    // 销毁一个执行器实例
    bool (*destroy) (purc_exec_inst_t inst);
} purc_exec_ops;
typedef struct purc_exec_ops* purc_exec_ops_t;

// 注册一个执行器的操作集
bool purc_register_executor(const char* name, purc_exec_ops_t ops);


PCA_EXTERN_C_END

#endif // PURC_PURC_EXECUTOR_H

