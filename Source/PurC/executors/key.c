/*
 * @file key.c
 * @author Xu Xiaohong
 * @date 2021/10/10
 * @brief The implementation of public part for KEY executor.
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

#include "key.h"

#include "private/executor.h"
#include "private/errors.h"

// 创建一个执行器实例
static purc_exec_inst_t
key_create(enum purc_exec_type type, purc_variant_t input, bool asc_desc)
{
    UNUSED_PARAM(type);
    UNUSED_PARAM(input);
    UNUSED_PARAM(asc_desc);
    return NULL;
}

// 用于执行选择
static purc_variant_t
key_choose(purc_exec_inst_t inst, const char* rule)
{
    UNUSED_PARAM(inst);
    UNUSED_PARAM(rule);
    return PURC_VARIANT_INVALID;
}

// 获得用于迭代的初始迭代子
static purc_exec_iter_t
key_it_begin(purc_exec_inst_t inst, const char* rule)
{
    UNUSED_PARAM(inst);
    UNUSED_PARAM(rule);
    return NULL;
}

// 根据迭代子获得对应的变体值
static purc_variant_t
key_it_value(purc_exec_inst_t inst, purc_exec_iter_t it)
{
    UNUSED_PARAM(inst);
    UNUSED_PARAM(it);
    return PURC_VARIANT_INVALID;
}

// 获得下一个迭代子
// 注意: 规则字符串可能在前后两次迭代中发生变化，比如在规则中引用了变量的情形下。
// 如果规则并没有发生变化，则对 `rule` 参数传递 NULL。
static purc_exec_iter_t
key_it_next(purc_exec_inst_t inst, purc_exec_iter_t it, const char* rule)
{
    UNUSED_PARAM(inst);
    UNUSED_PARAM(it);
    UNUSED_PARAM(rule);
    return NULL;
}

// 销毁当前迭代子
bool key_it_destroy(purc_exec_inst_t inst, purc_exec_iter_t it)
{
    UNUSED_PARAM(inst);
    UNUSED_PARAM(it);
    return true;
}

// 用于执行规约
static purc_variant_t
key_reduce(purc_exec_inst_t inst, const char* rule)
{
    UNUSED_PARAM(inst);
    UNUSED_PARAM(rule);
    return PURC_VARIANT_INVALID;
}

// 销毁一个执行器实例
static bool
key_destroy(purc_exec_inst_t inst)
{
    UNUSED_PARAM(inst);
    return true;
}

static struct purc_exec_ops key_ops = {
    key_create,
    key_choose,
    key_it_begin,
    key_it_value,
    key_it_next,
    key_it_destroy,
    key_reduce,
    key_destroy,
};

int pcexec_key_register(void)
{
    bool ok = purc_register_executor("KEY", &key_ops);
    return ok ? 0 : -1;
}

