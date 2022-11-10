/*
 * @file exe_add.c
 * @author Xu Xiaohong
 * @date 2021/10/10
 * @brief The implementation of public part for ADD executor.
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

#include "exe_add.h"

#include "private/executor.h"

#include "private/debug.h"
#include "private/errors.h"

#include <math.h>

struct pcexec_exe_add_inst {
    struct purc_exec_inst       super;

    struct exe_add_param        param;

    double                      curr;
};

// clear internal data except `input`
static inline void
reset(struct pcexec_exe_add_inst *exe_add_inst)
{
    struct exe_add_param *param = &exe_add_inst->param;
    exe_add_param_reset(param);
    pcexecutor_inst_reset(&exe_add_inst->super);
}

static inline bool
parse_rule(struct pcexec_exe_add_inst *exe_add_inst,
        const char* rule)
{
    purc_exec_inst_t inst = &exe_add_inst->super;

    struct exe_add_param param = {0};
    int r = exe_add_parse(rule, strlen(rule), &param);
    if (inst->err_msg) {
        free(inst->err_msg);
        inst->err_msg = NULL;
    }

    if (r) {
        inst->err_msg = param.err_msg;
        param.err_msg = NULL;
        return false;
    }

    exe_add_param_reset(&exe_add_inst->param);
    exe_add_inst->param = param;

    return true;
}

static inline bool
check_curr(struct pcexec_exe_add_inst *exe_add_inst, const double curr)
{
    purc_exec_inst_t inst = &exe_add_inst->super;
    struct exe_add_param *param = &exe_add_inst->param;
    struct add_rule *rule = &param->rule;
    struct number_comparing_logical_expression *ncle = rule->ncle;

    if (!isfinite(curr)) {
        pcinst_set_error(PCEXECUTOR_ERROR_OUT_OF_RANGE);
        return false;
    }

    bool match = false;
    int r = number_comparing_logical_expression_match(ncle, curr, &match);
    if (r || !match)
        return false;

    purc_variant_t v = purc_variant_make_number(curr);
    if (v == PURC_VARIANT_INVALID)
        return false;

    exe_add_inst->curr = curr;
    PCEXE_CLR_VAR(inst->value);
    inst->value = v;
    return true;
}

static inline purc_exec_iter_t
fetch_begin(struct pcexec_exe_add_inst *exe_add_inst)
{
    purc_exec_inst_t inst = &exe_add_inst->super;
    purc_exec_iter_t it = &inst->it;
    purc_variant_t input = inst->input;

    double curr = purc_variant_numerify(input);

    if (!check_curr(exe_add_inst, curr))
        return NULL;

    return it;
}

static inline purc_variant_t
fetch_value(struct pcexec_exe_add_inst *exe_add_inst)
{
    purc_exec_inst_t inst = &exe_add_inst->super;
    return inst->value;
}

static inline purc_exec_iter_t
fetch_next(struct pcexec_exe_add_inst *exe_add_inst)
{
    purc_exec_inst_t inst = &exe_add_inst->super;
    purc_exec_iter_t it = &inst->it;
    struct exe_add_param *param = &exe_add_inst->param;
    struct add_rule *rule = &param->rule;
    double curr = exe_add_inst->curr;
    if (!isnan(rule->nexp)) {
        curr += rule->nexp;
    }
    if (!check_curr(exe_add_inst, curr))
        return NULL;

    return it;
}

static inline purc_exec_iter_t
it_begin(struct pcexec_exe_add_inst *exe_add_inst, const char *rule)
{
    if (!parse_rule(exe_add_inst, rule))
        return NULL;

    return fetch_begin(exe_add_inst);
}

static inline purc_variant_t
it_value(struct pcexec_exe_add_inst *exe_add_inst)
{
    return fetch_value(exe_add_inst);
}

static inline purc_exec_iter_t
it_next(struct pcexec_exe_add_inst *exe_add_inst, const char *rule)
{
    if (rule) {
        if (!parse_rule(exe_add_inst, rule))
            return NULL;
    }

    return fetch_next(exe_add_inst);
}

static inline void
destroy(struct pcexec_exe_add_inst *exe_add_inst)
{
    purc_exec_inst_t inst = &exe_add_inst->super;

    reset(exe_add_inst);

    PCEXE_CLR_VAR(inst->input);
    PCEXE_CLR_VAR(inst->value);

    free(exe_add_inst);
}

// 创建一个执行器实例
static purc_exec_inst_t
exe_add_create(enum purc_exec_type type,
        purc_variant_t input, bool asc_desc)
{
    struct pcexec_exe_add_inst *inst;
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
exe_add_choose(purc_exec_inst_t inst, const char* rule)
{
    if (!inst || !rule) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return PURC_VARIANT_INVALID;
    }

    struct pcexec_exe_add_inst *exe_add_inst;
    exe_add_inst = (struct pcexec_exe_add_inst*)inst;

    purc_variant_t vals = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    if (vals == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    bool ok = true;

    purc_exec_iter_t it = it_begin(exe_add_inst, rule);
    if (!it && inst->err_msg) {
        purc_variant_unref(vals);
        return false;
    }

    for(; it; it = it_next(exe_add_inst, NULL)) {
        purc_variant_t v = it_value(exe_add_inst);
        ok = purc_variant_array_append(vals, v);
        if (!ok)
            break;
    }

    if (ok) {
        size_t n;
        purc_variant_array_size(vals, &n);
        if (n == 1) {
            purc_variant_t v = purc_variant_array_get(vals, 0);
            purc_variant_ref(v);
            purc_variant_unref(vals);
            vals = v;
        }
    }

    if (!ok) {
        purc_variant_unref(vals);
        return PURC_VARIANT_INVALID;
    }

    return vals;
}

// 获得用于迭代的初始迭代子
static purc_exec_iter_t
exe_add_it_begin(purc_exec_inst_t inst, const char* rule)
{
    if (!inst || !rule) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return NULL;
    }

    struct pcexec_exe_add_inst *exe_add_inst;
    exe_add_inst = (struct pcexec_exe_add_inst*)inst;

    return it_begin(exe_add_inst, rule);
}

// 根据迭代子获得对应的变体值
static purc_variant_t
exe_add_it_value(purc_exec_inst_t inst, purc_exec_iter_t it)
{
    if (!inst || !it) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return NULL;
    }

    PC_ASSERT(&inst->it == it);
    // PC_ASSERT(inst->selected_keys != PURC_VARIANT_INVALID);
    PC_ASSERT(inst->input != PURC_VARIANT_INVALID);

    struct pcexec_exe_add_inst *exe_add_inst;
    exe_add_inst = (struct pcexec_exe_add_inst*)inst;

    return it_value(exe_add_inst);
}

// 获得下一个迭代子
// 注意: 规则字符串可能在前后两次迭代中发生变化，比如在规则中引用了变量的情形下。
// 如果规则并没有发生变化，则对 `rule` 参数传递 NULL。
static purc_exec_iter_t
exe_add_it_next(purc_exec_inst_t inst, purc_exec_iter_t it, const char* rule)
{
    if (!inst || !it) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return NULL;
    }

    PC_ASSERT(&inst->it == it);
    PC_ASSERT(inst->input != PURC_VARIANT_INVALID);

    struct pcexec_exe_add_inst *exe_add_inst;
    exe_add_inst = (struct pcexec_exe_add_inst*)inst;

    return it_next(exe_add_inst, rule);
}

#define SET_KEY_AND_NUM(_o, _k, _d) {                        \
    purc_variant_t v;                                        \
    bool ok;                                                 \
    v = purc_variant_make_number(_d);                        \
    if (v == PURC_VARIANT_INVALID) {                         \
        ok = false;                                          \
        break;                                               \
    }                                                        \
    ok = purc_variant_object_set_by_static_ckey(obj,         \
            _k, v);                                          \
    purc_variant_unref(v);                                   \
    if (!ok)                                                 \
        break;                                               \
}

// 用于执行规约
static purc_variant_t
exe_add_reduce(purc_exec_inst_t inst, const char* rule)
{
    if (!inst || !rule) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return PURC_VARIANT_INVALID;
    }

    struct pcexec_exe_add_inst *exe_add_inst;
    exe_add_inst = (struct pcexec_exe_add_inst*)inst;

    size_t count = 0;
    double sum   = 0;
    double avg   = 0;
    double max   = NAN;
    double min   = NAN;

    purc_exec_iter_t it = it_begin(exe_add_inst, rule);
    if (!it && inst->err_msg) {
        return false;
    }

    for(; it; it = it_next(exe_add_inst, NULL)) {
        purc_variant_t v = it_value(exe_add_inst);
        double d = purc_variant_numerify(v);
        ++count;
        if (isnan(d))
            continue;
        sum += d;
        if (isnan(max)) {
            max = d;
        }
        else if (d > max) {
            max = d;
        }
        if (isnan(min)) {
            min = d;
        }
        else if (d < min) {
            min = d;
        }
    }

    if (count > 0) {
        avg = sum / count;
    }

    purc_variant_t obj = purc_variant_make_object(0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);

    if (obj == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    do {
        SET_KEY_AND_NUM(obj, "count", count);
        SET_KEY_AND_NUM(obj, "sum", sum);
        SET_KEY_AND_NUM(obj, "avg", avg);
        SET_KEY_AND_NUM(obj, "max", max);
        SET_KEY_AND_NUM(obj, "min", min);

        return obj;
    } while (0);

    purc_variant_unref(obj);
    return PURC_VARIANT_INVALID;
}

// 销毁一个执行器实例
static bool
exe_add_destroy(purc_exec_inst_t inst)
{
    if (!inst) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return false;
    }

    struct pcexec_exe_add_inst *exe_add_inst;
    exe_add_inst = (struct pcexec_exe_add_inst*)inst;
    destroy(exe_add_inst);

    return true;
}

static struct purc_exec_ops exe_add_ops = {
    exe_add_create,
    exe_add_choose,
    exe_add_it_begin,
    exe_add_it_value,
    exe_add_it_next,
    exe_add_reduce,
    exe_add_destroy,
};

int pcexec_exe_add_register(void)
{
    bool ok = purc_register_executor("ADD", &exe_add_ops);
    return ok ? 0 : -1;
}

