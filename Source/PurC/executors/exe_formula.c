/*
 * @file exe_formula.c
 * @author Xu Xiaohong
 * @date 2021/10/10
 * @brief The implementation of public part for FORMULA executor.
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

#include "exe_formula.h"

#include "private/executor.h"

#include "private/debug.h"
#include "private/errors.h"

#include <math.h>

struct pcexec_exe_formula_inst {
    struct purc_exec_inst       super;

    struct exe_formula_param        param;

    purc_variant_t              curr;
};

// clear internal data except `input`
static inline void
reset(struct pcexec_exe_formula_inst *exe_formula_inst)
{
    struct exe_formula_param *param = &exe_formula_inst->param;
    exe_formula_param_reset(param);
    pcexecutor_inst_reset(&exe_formula_inst->super);
    PCEXE_CLR_VAR(exe_formula_inst->curr);
}

static inline bool
parse_rule(struct pcexec_exe_formula_inst *exe_formula_inst,
        const char* rule)
{
    purc_exec_inst_t inst = &exe_formula_inst->super;

    struct exe_formula_param param = {0};
    int r = exe_formula_parse(rule, strlen(rule), &param);
    if (inst->err_msg) {
        free(inst->err_msg);
        inst->err_msg = NULL;
    }

    if (r) {
        inst->err_msg = param.err_msg;
        param.err_msg = NULL;
        return false;
    }

    exe_formula_param_reset(&exe_formula_inst->param);
    exe_formula_inst->param = param;

    return true;
}

static inline bool
iterate(struct pcexec_exe_formula_inst *exe_formula_inst)
{
    struct exe_formula_param *param = &exe_formula_inst->param;
    struct formula_rule *rule = &param->rule;
    purc_variant_t curr = exe_formula_inst->curr;
    purc_variant_t k = purc_variant_make_string_static("X", false);
    purc_variant_t v = purc_variant_object_get(curr, k);
    double d = purc_variant_numerify(v);

    bool ok = false;
    do {
        if (!isfinite(d)) {
            pcinst_set_error(PCEXECUTOR_ERROR_OUT_OF_RANGE);
            break;
        }

        double result;
        int r = iterative_formula_iterate(rule->ife, curr, &result);
        if (r) {
            pcinst_set_error(PCEXECUTOR_ERROR_OUT_OF_RANGE);
            break;
        }

        if (d == result) {
            ok = true;
            break;
        }
        if (pcexe_obj_set(curr, k, result))
            break;

        ok = true;
    } while (0);

    purc_variant_unref(k);
    return ok;
}

static inline bool
check_curr(struct pcexec_exe_formula_inst *exe_formula_inst)
{
    purc_exec_inst_t inst = &exe_formula_inst->super;
    struct exe_formula_param *param = &exe_formula_inst->param;
    struct formula_rule *rule = &param->rule;
    struct number_comparing_logical_expression *ncle = rule->ncle;
    purc_variant_t curr = exe_formula_inst->curr;
    purc_variant_t k = purc_variant_make_string_static("X", false);
    purc_variant_t v = purc_variant_object_get(curr, k);
    double d = purc_variant_numerify(v);

    bool ok = false;
    do {
        if (!isfinite(d)) {
            pcinst_set_error(PCEXECUTOR_ERROR_OUT_OF_RANGE);
            break;
        }

        bool match = false;
        int r = number_comparing_logical_expression_match(ncle, d, &match);
        if (r || !match)
            break;

        PCEXE_CLR_VAR(inst->value);
        inst->value = v;
        purc_variant_ref(v);

        ok = true;
    } while (0);

    purc_variant_unref(k);
    return ok;
}

static inline purc_exec_iter_t
fetch_begin(struct pcexec_exe_formula_inst *exe_formula_inst)
{
    purc_exec_inst_t inst = &exe_formula_inst->super;
    purc_exec_iter_t it = &inst->it;
    purc_variant_t input = inst->input;

    double d = purc_variant_numerify(input);

    purc_variant_t curr;
    curr = purc_variant_make_object(0, NULL, PURC_VARIANT_INVALID);
    PC_ASSERT(curr != PURC_VARIANT_INVALID); // FIXME: exception or not?

    bool ok = false;

    purc_variant_t k = purc_variant_make_string_static("X", false);
    purc_variant_t v = purc_variant_make_number(d);
    PC_ASSERT(v != PURC_VARIANT_INVALID); // FIXME: exception or not?
    ok = purc_variant_object_set(curr, k, v);
    purc_variant_unref(k);
    purc_variant_unref(v);

    if (!ok) {
        purc_variant_unref(curr);
    }
    if (!ok) {
        PC_ASSERT(0); // FIXME: exception or not?
    }

    PCEXE_CLR_VAR(exe_formula_inst->curr);
    exe_formula_inst->curr = curr;

    if (!check_curr(exe_formula_inst))
        return NULL;

    return it;
}

static inline purc_variant_t
fetch_value(struct pcexec_exe_formula_inst *exe_formula_inst)
{
    purc_exec_inst_t inst = &exe_formula_inst->super;
    return inst->value;
}

static inline purc_exec_iter_t
fetch_next(struct pcexec_exe_formula_inst *exe_formula_inst)
{
    purc_exec_inst_t inst = &exe_formula_inst->super;
    purc_exec_iter_t it = &inst->it;
    if (!iterate(exe_formula_inst))
        return NULL;

    if (!check_curr(exe_formula_inst))
        return NULL;

    return it;
}

static inline purc_exec_iter_t
it_begin(struct pcexec_exe_formula_inst *exe_formula_inst, const char *rule)
{
    if (!parse_rule(exe_formula_inst, rule))
        return NULL;

    return fetch_begin(exe_formula_inst);
}

static inline purc_variant_t
it_value(struct pcexec_exe_formula_inst *exe_formula_inst)
{
    return fetch_value(exe_formula_inst);
}

static inline purc_exec_iter_t
it_next(struct pcexec_exe_formula_inst *exe_formula_inst, const char *rule)
{
    if (rule) {
        if (!parse_rule(exe_formula_inst, rule))
            return NULL;
    }

    return fetch_next(exe_formula_inst);
}

static inline void
destroy(struct pcexec_exe_formula_inst *exe_formula_inst)
{
    purc_exec_inst_t inst = &exe_formula_inst->super;

    reset(exe_formula_inst);

    PCEXE_CLR_VAR(inst->input);
    PCEXE_CLR_VAR(inst->value);

    free(exe_formula_inst);
}

// 创建一个执行器实例
static purc_exec_inst_t
exe_formula_create(enum purc_exec_type type,
        purc_variant_t input, bool asc_desc)
{
    struct pcexec_exe_formula_inst *inst;
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
exe_formula_choose(purc_exec_inst_t inst, const char* rule)
{
    if (!inst || !rule) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return PURC_VARIANT_INVALID;
    }

    struct pcexec_exe_formula_inst *exe_formula_inst;
    exe_formula_inst = (struct pcexec_exe_formula_inst*)inst;

    purc_variant_t vals = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    if (vals == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    bool ok = true;

    purc_exec_iter_t it = it_begin(exe_formula_inst, rule);

    for(; it; it = it_next(exe_formula_inst, NULL)) {
        purc_variant_t v = it_value(exe_formula_inst);
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
exe_formula_it_begin(purc_exec_inst_t inst, const char* rule)
{
    if (!inst || !rule) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return NULL;
    }

    struct pcexec_exe_formula_inst *exe_formula_inst;
    exe_formula_inst = (struct pcexec_exe_formula_inst*)inst;

    return it_begin(exe_formula_inst, rule);
}

// 根据迭代子获得对应的变体值
static purc_variant_t
exe_formula_it_value(purc_exec_inst_t inst, purc_exec_iter_t it)
{
    if (!inst || !it) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return NULL;
    }

    PC_ASSERT(&inst->it == it);
    PC_ASSERT(inst->selected_keys != PURC_VARIANT_INVALID);
    PC_ASSERT(inst->input != PURC_VARIANT_INVALID);

    struct pcexec_exe_formula_inst *exe_formula_inst;
    exe_formula_inst = (struct pcexec_exe_formula_inst*)inst;

    return it_value(exe_formula_inst);
}

// 获得下一个迭代子
// 注意: 规则字符串可能在前后两次迭代中发生变化，比如在规则中引用了变量的情形下。
// 如果规则并没有发生变化，则对 `rule` 参数传递 NULL。
static purc_exec_iter_t
exe_formula_it_next(purc_exec_inst_t inst, purc_exec_iter_t it, const char* rule)
{
    if (!inst || !it) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return NULL;
    }

    PC_ASSERT(&inst->it == it);
    PC_ASSERT(inst->input != PURC_VARIANT_INVALID);

    struct pcexec_exe_formula_inst *exe_formula_inst;
    exe_formula_inst = (struct pcexec_exe_formula_inst*)inst;

    return it_next(exe_formula_inst, rule);
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
exe_formula_reduce(purc_exec_inst_t inst, const char* rule)
{
    if (!inst || !rule) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return PURC_VARIANT_INVALID;
    }

    struct pcexec_exe_formula_inst *exe_formula_inst;
    exe_formula_inst = (struct pcexec_exe_formula_inst*)inst;

    size_t count = 0;
    double sum   = 0;
    double avg   = 0;
    double max   = NAN;
    double min   = NAN;

    purc_exec_iter_t it = it_begin(exe_formula_inst, rule);

    for(; it; it = it_next(exe_formula_inst, NULL)) {
        purc_variant_t v = it_value(exe_formula_inst);
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
exe_formula_destroy(purc_exec_inst_t inst)
{
    if (!inst) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return false;
    }

    struct pcexec_exe_formula_inst *exe_formula_inst;
    exe_formula_inst = (struct pcexec_exe_formula_inst*)inst;
    destroy(exe_formula_inst);

    return true;
}

static struct purc_exec_ops exe_formula_ops = {
    exe_formula_create,
    exe_formula_choose,
    exe_formula_it_begin,
    exe_formula_it_value,
    exe_formula_it_next,
    exe_formula_reduce,
    exe_formula_destroy,
};

int pcexec_exe_formula_register(void)
{
    bool ok = purc_register_executor("FORMULA", &exe_formula_ops);
    return ok ? 0 : -1;
}

