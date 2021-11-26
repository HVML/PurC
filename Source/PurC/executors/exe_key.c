/*
 * @file exe_key.c
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

#include "exe_key.h"

#include "pcexe-helper.h"

#include "private/executor.h"
#include "private/variant.h"

#include "private/debug.h"
#include "private/errors.h"

#include <math.h>

struct pcexec_exe_key_inst {
    struct purc_exec_inst       super;
    struct purc_variant_object_iterator *curr;

    struct exe_key_param     param;
};

static inline void
exe_key_param_reset(struct exe_key_param *param)
{
    if (param->err_msg) {
        free(param->err_msg);
        param->err_msg = NULL;
    }

    if (param->rule_valid) {
        if (param->rule.lexp) {
            logical_expression_destroy(param->rule.lexp);
            param->rule.lexp = NULL;
        }
        param->rule_valid = 0;
    }
}

// clear internal data except `input`
static inline void
reset(struct pcexec_exe_key_inst *exe_key_inst)
{
    struct exe_key_param *param = &exe_key_inst->param;
    exe_key_param_reset(param);
    if (exe_key_inst->curr) {
        purc_variant_object_release_iterator(exe_key_inst->curr);
        exe_key_inst->curr = NULL; 
    }
    pcexecutor_inst_reset(&exe_key_inst->super);
}

static inline bool
parse_rule(struct pcexec_exe_key_inst *exe_key_inst,
        const char* rule)
{
    purc_exec_inst_t inst = &exe_key_inst->super;

    reset(exe_key_inst);
    PCEXE_CLR_VAR(inst->value);

    struct exe_key_param *param = &exe_key_inst->param;
    param->rule_valid = 0;
    int r = exe_key_parse(rule, strlen(rule), &exe_key_inst->param);
    inst->err_msg = exe_key_inst->param.err_msg;
    exe_key_inst->param.err_msg = NULL;

    if (r)
        return false;

    param->rule_valid = 1;
    return true;
}

static inline purc_exec_iter_t
fetch_and_cache(struct pcexec_exe_key_inst *exe_key_inst)
{
    purc_exec_inst_t inst = &exe_key_inst->super;
    purc_exec_iter_t it = &inst->it;
    struct key_rule *rule = &exe_key_inst->param.rule;

    PC_ASSERT(exe_key_inst->param.rule_valid);
    PC_ASSERT(exe_key_inst->curr);

    purc_variant_t val = PURC_VARIANT_INVALID;

    struct purc_variant_object_iterator *curr;
    curr = exe_key_inst->curr;

    switch (rule->for_clause)
    {
        case FOR_CLAUSE_VALUE:
            {
                val = purc_variant_object_iterator_get_value(curr);
                purc_variant_ref(val);
            } break;
        case FOR_CLAUSE_KEY:
            {
                val = purc_variant_object_iterator_get_key(curr);
                purc_variant_ref(val);
            } break;
        case FOR_CLAUSE_KV:
            {
                purc_variant_t k, v;
                k = purc_variant_object_iterator_get_key(curr);
                v = purc_variant_object_iterator_get_value(curr);
                val = purc_variant_make_object(1, k, v);
                if (val == PURC_VARIANT_INVALID)
                    return NULL;
            } break;
        default:
            {
                PC_ASSERT(0);
            } break;
    }

    PC_ASSERT(val != PURC_VARIANT_INVALID);

    PCEXE_CLR_VAR(inst->value);

    inst->value = val;

    return it;
}

static inline purc_exec_iter_t
fetch_object_begin(struct pcexec_exe_key_inst *exe_key_inst)
{
    purc_exec_inst_t inst = &exe_key_inst->super;
    purc_variant_t cache = inst->cache;
    struct key_rule *rule = &exe_key_inst->param.rule;

    PC_ASSERT(exe_key_inst->param.rule_valid);
    PC_ASSERT(exe_key_inst->curr == NULL);

    bool ok = true;

    struct purc_variant_object_iterator *curr;
    curr = purc_variant_object_make_iterator_begin(cache);
    if (!curr) {
        pcinst_set_error(PCEXECUTOR_ERROR_NOT_EXISTS);
        return NULL;
    }

    PC_ASSERT(curr);

    bool result = false;
    do {
        purc_variant_t k = purc_variant_object_iterator_get_key(curr);

        int r = key_rule_eval(rule, k, &result);
        if (r) {
            ok = false;
            break;
        }
        if (result)
            break;
    } while (purc_variant_object_iterator_next(curr));

    if (!ok) {
        return NULL;
    }

    if (!result) {
        pcinst_set_error(PCEXECUTOR_ERROR_NOT_EXISTS);
        return NULL;
    }

    exe_key_inst->curr = curr;
    return fetch_and_cache(exe_key_inst);
}

static inline purc_exec_iter_t
fetch_begin(struct pcexec_exe_key_inst *exe_key_inst)
{
    purc_exec_inst_t inst = &exe_key_inst->super;
    purc_variant_t cache = inst->cache;
    switch (purc_variant_get_type(cache))
    {
        case PURC_VARIANT_TYPE_OBJECT:
            return fetch_object_begin(exe_key_inst);
        default:
            pcinst_set_error(PCEXECUTOR_ERROR_NOT_IMPLEMENTED);
            return NULL;
    }
}

static inline purc_exec_iter_t
fetch_object_next(struct pcexec_exe_key_inst *exe_key_inst)
{
    purc_exec_inst_t inst = &exe_key_inst->super;
    struct key_rule *rule = &exe_key_inst->param.rule;

    PC_ASSERT(exe_key_inst->param.rule_valid);

    PCEXE_CLR_VAR(inst->value);

    bool ok = true;

    struct purc_variant_object_iterator *curr = exe_key_inst->curr;
    PC_ASSERT(curr);
    if (!purc_variant_object_iterator_next(curr)) {
        return NULL;
    }

    PC_ASSERT(curr);

    bool result = false;
    do {
        purc_variant_t k = purc_variant_object_iterator_get_key(curr);

        int r = key_rule_eval(rule, k, &result);
        if (r) {
            ok = false;
            break;
        }
        if (result)
            break;
    } while (purc_variant_object_iterator_next(curr));

    if (!ok) {
        return NULL;
    }

    if (!result) {
        pcinst_set_error(PCEXECUTOR_ERROR_NOT_EXISTS);
        return NULL;
    }

    exe_key_inst->curr = curr;
    return fetch_and_cache(exe_key_inst);
}

static inline purc_exec_iter_t
fetch_next(struct pcexec_exe_key_inst *exe_key_inst)
{
    purc_exec_inst_t inst = &exe_key_inst->super;
    purc_variant_t cache = inst->cache;
    switch (purc_variant_get_type(cache))
    {
        case PURC_VARIANT_TYPE_OBJECT:
            return fetch_object_next(exe_key_inst);
        default:
            pcinst_set_error(PCEXECUTOR_ERROR_NOT_IMPLEMENTED);
            return NULL;
    }
}

static inline purc_variant_t
fetch_value(struct pcexec_exe_key_inst *exe_key_inst)
{
    purc_exec_inst_t inst = &exe_key_inst->super;

    return inst->value;
}

static inline void
destroy(struct pcexec_exe_key_inst *exe_key_inst)
{
    purc_exec_inst_t inst = &exe_key_inst->super;

    reset(exe_key_inst);

    PCEXE_CLR_VAR(inst->input);
    PCEXE_CLR_VAR(inst->cache);
    PCEXE_CLR_VAR(inst->value);

    free(exe_key_inst);
}

// 创建一个执行器实例
static purc_exec_inst_t
exe_key_create(enum purc_exec_type type,
        purc_variant_t input, bool asc_desc)
{
    struct pcexec_exe_key_inst *exe_key_inst;
    exe_key_inst = calloc(1, sizeof(*exe_key_inst));
    if (!exe_key_inst) {
        pcinst_set_error(PCEXECUTOR_ERROR_OOM);
        return NULL;
    }

    purc_exec_inst_t inst = &exe_key_inst->super;

    inst->type        = type;
    inst->asc_desc    = asc_desc;

    int debug_flex, debug_bison;
    pcexecutor_get_debug(&debug_flex, &debug_bison);
    exe_key_inst->param.debug_flex  = debug_flex;
    exe_key_inst->param.debug_bison = debug_bison;

    enum purc_variant_type vt = purc_variant_get_type(input);
    if (vt == PURC_VARIANT_TYPE_OBJECT)
    {
        purc_variant_t cache = pcexe_make_cache(input, asc_desc);
        if (cache != PURC_VARIANT_INVALID) {
            inst->cache = cache;
            inst->input = input;
            purc_variant_ref(input);

            return inst;
        }
    }

    destroy(exe_key_inst);
    return NULL;
}

// 用于执行选择
static purc_variant_t
exe_key_choose(purc_exec_inst_t inst, const char* rule)
{
    if (!inst || !rule) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return PURC_VARIANT_INVALID;
    }

    struct pcexec_exe_key_inst *exe_key_inst;
    exe_key_inst = (struct pcexec_exe_key_inst*)inst;

    if (!parse_rule(exe_key_inst, rule))
        return PURC_VARIANT_INVALID;

    purc_variant_t vals = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    if (vals == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    bool ok = true;

    purc_exec_iter_t it = fetch_begin(exe_key_inst);

    for(; it; it = fetch_next(exe_key_inst)) {
        purc_variant_t v = fetch_value(exe_key_inst);
        ok = purc_variant_array_append(vals, v);
        if (!ok)
            break;
    }

    if (!ok) {
        purc_variant_unref(vals);
        return PURC_VARIANT_INVALID;
    }

    return vals;
}

// 获得用于迭代的初始迭代子
static purc_exec_iter_t
exe_key_it_begin(purc_exec_inst_t inst, const char* rule)
{
    if (!inst || !rule) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return NULL;
    }

    if (inst->type != PURC_EXEC_TYPE_ITERATE) {
        pcinst_set_error(PCEXECUTOR_ERROR_NOT_ALLOWED);
        return NULL;
    }

    PC_ASSERT(inst->input != PURC_VARIANT_INVALID);

    struct pcexec_exe_key_inst *exe_key_inst;
    exe_key_inst = (struct pcexec_exe_key_inst*)inst;

    if (!parse_rule(exe_key_inst, rule))
        return NULL;

    PC_ASSERT(inst->cache != PURC_VARIANT_INVALID);

    return fetch_begin(exe_key_inst);
}

// 根据迭代子获得对应的变体值
static purc_variant_t
exe_key_it_value(purc_exec_inst_t inst, purc_exec_iter_t it)
{
    if (!inst || !it) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return NULL;
    }

    PC_ASSERT(&inst->it == it);
    PC_ASSERT(inst->input != PURC_VARIANT_INVALID);
    PC_ASSERT(inst->cache != PURC_VARIANT_INVALID);
    PC_ASSERT(inst->value != PURC_VARIANT_INVALID);

    struct pcexec_exe_key_inst *exe_key_inst;
    exe_key_inst = (struct pcexec_exe_key_inst*)inst;

    return fetch_value(exe_key_inst);
}

// 获得下一个迭代子
// 注意: 规则字符串可能在前后两次迭代中发生变化，比如在规则中引用了变量的情形下。
// 如果规则并没有发生变化，则对 `rule` 参数传递 NULL。
static purc_exec_iter_t
exe_key_it_next(purc_exec_inst_t inst, purc_exec_iter_t it, const char* rule)
{
    if (!inst || !it) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return NULL;
    }

    PC_ASSERT(&inst->it == it);
    PC_ASSERT(inst->input != PURC_VARIANT_INVALID);
    PC_ASSERT(inst->cache != PURC_VARIANT_INVALID);

    struct pcexec_exe_key_inst *exe_key_inst;
    exe_key_inst = (struct pcexec_exe_key_inst*)inst;

    if (rule) {
        if (!parse_rule(exe_key_inst, rule))
            return NULL;
    }

    PC_ASSERT(inst->cache != PURC_VARIANT_INVALID);

    return fetch_next(exe_key_inst);
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
exe_key_reduce(purc_exec_inst_t inst, const char* rule)
{
    if (!inst || !rule) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return PURC_VARIANT_INVALID;
    }

    struct pcexec_exe_key_inst *exe_key_inst;
    exe_key_inst = (struct pcexec_exe_key_inst*)inst;

    if (!parse_rule(exe_key_inst, rule))
        return PURC_VARIANT_INVALID;

    size_t count = 0;
    double sum   = 0;
    double avg   = 0;
    double max   = NAN;
    double min   = NAN;

    purc_exec_iter_t it = fetch_begin(exe_key_inst);

    for(; it; it = fetch_next(exe_key_inst)) {
        purc_variant_t v = fetch_value(exe_key_inst);
        double d = purc_variant_numberify(v);
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
exe_key_destroy(purc_exec_inst_t inst)
{
    if (!inst) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return false;
    }

    struct pcexec_exe_key_inst *exe_key_inst;
    exe_key_inst = (struct pcexec_exe_key_inst*)inst;
    destroy(exe_key_inst);

    return true;
}

static struct purc_exec_ops exe_key_ops = {
    exe_key_create,
    exe_key_choose,
    exe_key_it_begin,
    exe_key_it_value,
    exe_key_it_next,
    exe_key_reduce,
    exe_key_destroy,
};

int pcexec_exe_key_register(void)
{
    bool ok = purc_register_executor("KEY", &exe_key_ops);
    return ok ? 0 : -1;
}

