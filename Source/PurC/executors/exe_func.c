/*
 * @file exe_func.c
 * @author Xu Xiaohong
 * @date 2022/06/16
 * @brief The implementation of public part for FUNC executor.
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

#include "exe_func.h"

#include "private/executor.h"

#include "private/debug.h"
#include "private/errors.h"

#include <math.h>

struct pcexec_exe_func_inst {
    struct purc_exec_inst       super;

    struct exe_func_param       param;
};

// clear internal data except `input`
static inline void
reset(struct pcexec_exe_func_inst *exe_func_inst)
{
    struct exe_func_param *param = &exe_func_inst->param;
    exe_func_param_reset(param);
    pcexecutor_inst_reset(&exe_func_inst->super);
}

static inline bool
parse_rule(struct pcexec_exe_func_inst *exe_func_inst,
        const char* rule)
{
    purc_exec_inst_t inst = &exe_func_inst->super;

    struct exe_func_param param = {0};
    int r = exe_func_parse(rule, strlen(rule), &param);
    if (inst->err_msg) {
        free(inst->err_msg);
        inst->err_msg = NULL;
    }

    if (r) {
        inst->err_msg = param.err_msg;
        param.err_msg = NULL;
        return false;
    }

    exe_func_param_reset(&exe_func_inst->param);
    exe_func_inst->param = param;

    return true;
}

static inline void
destroy(struct pcexec_exe_func_inst *exe_func_inst)
{
    purc_exec_inst_t inst = &exe_func_inst->super;

    reset(exe_func_inst);

    PCEXE_CLR_VAR(inst->input);
    PCEXE_CLR_VAR(inst->value);

    free(exe_func_inst);
}

// 创建一个执行器实例
static purc_exec_inst_t
exe_func_create(enum purc_exec_type type, purc_variant_t input, bool asc_desc)
{
    struct pcexec_exe_func_inst *inst;
    inst = calloc(1, sizeof(*inst));
    if (!inst) {
        pcinst_set_error(PCEXECUTOR_ERROR_OOM);
        return NULL;
    }

    inst->super.type        = type;
    inst->super.input       = input;
    inst->super.asc_desc    = asc_desc;

    purc_variant_ref(input);

    return &inst->super;
}

// 用于执行选择
static purc_variant_t
exe_func_choose(purc_exec_inst_t inst, const char* rule)
{
    if (!inst || !rule) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return PURC_VARIANT_INVALID;
    }

    struct pcexec_exe_func_inst *exe_func_inst;
    exe_func_inst = (struct pcexec_exe_func_inst*)inst;

    PC_ASSERT(exe_func_inst);
    PC_ASSERT(0);
}

// 获得用于迭代的初始迭代子
static purc_exec_iter_t
exe_func_it_begin(purc_exec_inst_t inst, const char* rule)
{
    if (!inst || !rule) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return NULL;
    }

    struct pcexec_exe_func_inst *exe_func_inst;
    exe_func_inst = (struct pcexec_exe_func_inst*)inst;

    PC_ASSERT(exe_func_inst);
    PC_ASSERT(0);
}

// 根据迭代子获得对应的变体值
static purc_variant_t
exe_func_it_value(purc_exec_inst_t inst, purc_exec_iter_t it)
{
    if (!inst || !it) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return NULL;
    }

    PC_ASSERT(&inst->it == it);
    PC_ASSERT(inst->input != PURC_VARIANT_INVALID);

    struct pcexec_exe_func_inst *exe_func_inst;
    exe_func_inst = (struct pcexec_exe_func_inst*)inst;

    PC_ASSERT(exe_func_inst);
    PC_ASSERT(0);
}

// 获得下一个迭代子
// 注意: 规则字符串可能在前后两次迭代中发生变化，比如在规则中引用了变量的情形下。
// 如果规则并没有发生变化，则对 `rule` 参数传递 NULL。
static purc_exec_iter_t
exe_func_it_next(purc_exec_inst_t inst, purc_exec_iter_t it, const char* rule)
{
    if (!inst || !it) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return NULL;
    }

    PC_ASSERT(&inst->it == it);
    PC_ASSERT(inst->input != PURC_VARIANT_INVALID);

    struct pcexec_exe_func_inst *exe_func_inst;
    exe_func_inst = (struct pcexec_exe_func_inst*)inst;

    UNUSED_PARAM(rule);
    PC_ASSERT(exe_func_inst);
    PC_ASSERT(0);
}

// 用于执行规约
static purc_variant_t
exe_func_reduce(purc_exec_inst_t inst, const char* rule)
{
    if (!inst || !rule) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return PURC_VARIANT_INVALID;
    }

    struct pcexec_exe_func_inst *exe_func_inst;
    exe_func_inst = (struct pcexec_exe_func_inst*)inst;

    PC_ASSERT(exe_func_inst);
    PC_ASSERT(0);
}

// 销毁一个执行器实例
static bool
exe_func_destroy(purc_exec_inst_t inst)
{
    if (!inst) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return false;
    }

    struct pcexec_exe_func_inst *exe_func_inst;
    exe_func_inst = (struct pcexec_exe_func_inst*)inst;
    destroy(exe_func_inst);

    PC_ASSERT(exe_func_inst);
    PC_ASSERT(0);
}

static struct purc_exec_ops exe_func_ops = {
    exe_func_create,
    exe_func_choose,
    exe_func_it_begin,
    exe_func_it_value,
    exe_func_it_next,
    exe_func_reduce,
    exe_func_destroy,
};

int pcexec_exe_func_register(void)
{
    bool ok = purc_register_executor("FUNC", &exe_func_ops);
    return ok ? 0 : -1;
}

