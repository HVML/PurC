/*
 * @file exe_filter.c
 * @author Xu Xiaohong
 * @date 2021/10/10
 * @brief The implementation of public part for FILTER executor.
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

#include "exe_filter.h"

#include "pcexe-helper.h"

#include "private/executor.h"
#include "private/variant.h"

#include "private/debug.h"
#include "private/errors.h"

#include <math.h>

struct pcexec_exe_filter_inst {
    struct purc_exec_inst       super;
    struct purc_variant_object_iterator *it_obj;
    size_t                               it_arr;

    struct exe_filter_param     param;
};

void
exe_filter_param_reset(struct exe_filter_param *param)
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
reset(struct pcexec_exe_filter_inst *exe_filter_inst)
{
    struct exe_filter_param *param = &exe_filter_inst->param;
    exe_filter_param_reset(param);
    if (exe_filter_inst->it_obj) {
        purc_variant_object_release_iterator(exe_filter_inst->it_obj);
        exe_filter_inst->it_obj = NULL; 
    }
    pcexecutor_inst_reset(&exe_filter_inst->super);
}

static inline bool
parse_rule(struct pcexec_exe_filter_inst *exe_filter_inst,
        const char* rule)
{
    purc_exec_inst_t inst = &exe_filter_inst->super;

    reset(exe_filter_inst);
    PCEXE_CLR_VAR(inst->value);

    struct exe_filter_param *param = &exe_filter_inst->param;
    param->rule_valid = 0;
    int r = exe_filter_parse(rule, strlen(rule), &exe_filter_inst->param);
    inst->err_msg = exe_filter_inst->param.err_msg;
    exe_filter_inst->param.err_msg = NULL;

    if (r)
        return false;

    param->rule_valid = 1;
    return true;
}

static inline purc_exec_iter_t
fetch_and_cache(struct pcexec_exe_filter_inst *exe_filter_inst)
{
    purc_exec_inst_t inst = &exe_filter_inst->super;
    purc_exec_iter_t it = &inst->it;

    PC_ASSERT(0);
    return it;
}

static inline purc_exec_iter_t
filter_object_until_match(struct pcexec_exe_filter_inst *exe_filter_inst)
{
    purc_exec_inst_t inst = &exe_filter_inst->super;
    purc_exec_iter_t it = &inst->it;
    struct filter_rule *rule = &exe_filter_inst->param.rule;

    PC_ASSERT(exe_filter_inst->param.rule_valid);
    PC_ASSERT(exe_filter_inst->it_obj == NULL);

    bool ok = true;

    struct purc_variant_object_iterator *it_obj;
    it_obj = exe_filter_inst->it_obj;
    if (!it_obj) {
        pcinst_set_error(PCEXECUTOR_ERROR_NOT_EXISTS);
        return NULL;
    }

    PC_ASSERT(it_obj);

    bool result = false;
    purc_variant_t found = PURC_VARIANT_INVALID;
    do {
        purc_variant_t k = purc_variant_object_iterator_get_key(it_obj);

        int r = filter_rule_eval(rule, k, &result);
        if (r) {
            ok = false;
            break;
        }
        if (result) {
            purc_variant_t val = PURC_VARIANT_INVALID;
            switch (rule->for_clause)
            {
                case FOR_CLAUSE_VALUE:
                    {
                        val = purc_variant_object_iterator_get_value(it_obj);
                        purc_variant_ref(val);
                    } break;
                case FOR_CLAUSE_KEY:
                    {
                        val = purc_variant_object_iterator_get_key(it_obj);
                        purc_variant_ref(val);
                    } break;
                case FOR_CLAUSE_KV:
                    {
                        purc_variant_t v;
                        v = purc_variant_object_iterator_get_value(it_obj);
                        val = purc_variant_make_object(1, k, v);
                        if (val == PURC_VARIANT_INVALID)
                            return NULL;
                    } break;
                default:
                    {
                        PC_ASSERT(0);
                    } break;
            }
            found = val;
            break;
        }
    } while (purc_variant_object_iterator_next(it_obj));

    if (!ok) {
        return NULL;
    }

    if (!result) {
        pcinst_set_error(PCEXECUTOR_ERROR_NOT_EXISTS);
        return NULL;
    }

    exe_filter_inst->it_obj = it_obj;
    PCEXE_CLR_VAR(inst->value);
    inst->value = found;
    return it;
}

static inline purc_exec_iter_t
filter_array_until_match(struct pcexec_exe_filter_inst *exe_filter_inst)
{
    purc_exec_inst_t inst = &exe_filter_inst->super;
    purc_exec_iter_t it = &inst->it;
    purc_variant_t cache = inst->cache;
    struct filter_rule *rule = &exe_filter_inst->param.rule;

    PC_ASSERT(exe_filter_inst->param.rule_valid);
    PC_ASSERT(exe_filter_inst->it_obj == NULL);

    bool ok = true;

    size_t nr;
    if (!purc_variant_array_size(cache, &nr)) {
        return NULL;
    }

    size_t it_arr = exe_filter_inst->it_arr;

    bool result = false;
    purc_variant_t found = PURC_VARIANT_INVALID;
    do {
        if (it_arr >= nr) {
            pcinst_set_error(PCEXECUTOR_ERROR_NOT_EXISTS);
            break;
        }

        purc_variant_t v = purc_variant_array_get(cache, it_arr);

        int r = filter_rule_eval(rule, v, &result);
        if (r) {
            ok = false;
            break;
        }
        if (result) {
            found = v;
            purc_variant_ref(found);
            break;
        }
    } while (++it_arr);

    if (!ok) {
        return NULL;
    }

    if (!result) {
        pcinst_set_error(PCEXECUTOR_ERROR_NOT_EXISTS);
        return NULL;
    }

    exe_filter_inst->it_arr = it_arr;
    PCEXE_CLR_VAR(inst->value);
    inst->value = found;
    return it;
}

static inline purc_exec_iter_t
filter_until_match(struct pcexec_exe_filter_inst *exe_filter_inst)
{
    purc_exec_inst_t inst = &exe_filter_inst->super;
    purc_variant_t cache = inst->cache;
    switch (purc_variant_get_type(cache))
    {
        case PURC_VARIANT_TYPE_OBJECT:
            return filter_object_until_match(exe_filter_inst);
        case PURC_VARIANT_TYPE_ARRAY:
            return filter_array_until_match(exe_filter_inst);
        default:
            pcinst_set_error(PCEXECUTOR_ERROR_NOT_IMPLEMENTED);
            return NULL;
    }
}

static inline purc_exec_iter_t
fetch_begin(struct pcexec_exe_filter_inst *exe_filter_inst)
{
    purc_exec_inst_t inst = &exe_filter_inst->super;
    purc_variant_t cache = inst->cache;
    switch (purc_variant_get_type(cache))
    {
        case PURC_VARIANT_TYPE_OBJECT:
            exe_filter_inst->it_obj = purc_variant_object_make_iterator_begin(cache);
            return filter_until_match(exe_filter_inst);
        case PURC_VARIANT_TYPE_ARRAY:
            exe_filter_inst->it_arr = 0;
            return filter_until_match(exe_filter_inst);
        default:
            pcinst_set_error(PCEXECUTOR_ERROR_NOT_IMPLEMENTED);
            return NULL;
    }
}

static inline purc_exec_iter_t
fetch_object_next(struct pcexec_exe_filter_inst *exe_filter_inst)
{
    purc_exec_inst_t inst = &exe_filter_inst->super;
    struct filter_rule *rule = &exe_filter_inst->param.rule;

    PC_ASSERT(exe_filter_inst->param.rule_valid);

    PCEXE_CLR_VAR(inst->value);

    bool ok = true;

    struct purc_variant_object_iterator *it_obj = exe_filter_inst->it_obj;
    PC_ASSERT(it_obj);
    if (!purc_variant_object_iterator_next(it_obj)) {
        return NULL;
    }

    PC_ASSERT(it_obj);

    bool result = false;
    do {
        purc_variant_t k = purc_variant_object_iterator_get_key(it_obj);

        int r = filter_rule_eval(rule, k, &result);
        if (r) {
            ok = false;
            break;
        }
        if (result)
            break;
    } while (purc_variant_object_iterator_next(it_obj));

    if (!ok) {
        return NULL;
    }

    if (!result) {
        pcinst_set_error(PCEXECUTOR_ERROR_NOT_EXISTS);
        return NULL;
    }

    exe_filter_inst->it_obj = it_obj;
    return fetch_and_cache(exe_filter_inst);
}

static inline purc_exec_iter_t
fetch_next(struct pcexec_exe_filter_inst *exe_filter_inst)
{
    purc_exec_inst_t inst = &exe_filter_inst->super;
    purc_variant_t cache = inst->cache;
    switch (purc_variant_get_type(cache))
    {
        case PURC_VARIANT_TYPE_OBJECT:
            if (!purc_variant_object_iterator_next(exe_filter_inst->it_obj))
                return NULL;
            break;
        case PURC_VARIANT_TYPE_ARRAY:
            ++exe_filter_inst->it_arr;
            break;
        default:
            pcinst_set_error(PCEXECUTOR_ERROR_NOT_IMPLEMENTED);
            return NULL;
    }
    return filter_until_match(exe_filter_inst);
}

static inline purc_variant_t
fetch_value(struct pcexec_exe_filter_inst *exe_filter_inst)
{
    purc_exec_inst_t inst = &exe_filter_inst->super;

    return inst->value;
}

static inline void
destroy(struct pcexec_exe_filter_inst *exe_filter_inst)
{
    purc_exec_inst_t inst = &exe_filter_inst->super;

    reset(exe_filter_inst);

    PCEXE_CLR_VAR(inst->input);
    PCEXE_CLR_VAR(inst->cache);
    PCEXE_CLR_VAR(inst->value);

    free(exe_filter_inst);
}

// 创建一个执行器实例
static purc_exec_inst_t
exe_filter_create(enum purc_exec_type type,
        purc_variant_t input, bool asc_desc)
{
    struct pcexec_exe_filter_inst *exe_filter_inst;
    exe_filter_inst = calloc(1, sizeof(*exe_filter_inst));
    if (!exe_filter_inst) {
        pcinst_set_error(PCEXECUTOR_ERROR_OOM);
        return NULL;
    }

    purc_exec_inst_t inst = &exe_filter_inst->super;

    inst->type        = type;
    inst->asc_desc    = asc_desc;

    int debug_flex, debug_bison;
    pcexecutor_get_debug(&debug_flex, &debug_bison);
    exe_filter_inst->param.debug_flex  = debug_flex;
    exe_filter_inst->param.debug_bison = debug_bison;

    enum purc_variant_type vt = purc_variant_get_type(input);
    if (vt == PURC_VARIANT_TYPE_OBJECT ||
        vt == PURC_VARIANT_TYPE_ARRAY ||
        vt == PURC_VARIANT_TYPE_SET)
    {
        purc_variant_t cache = pcexe_make_cache(input, asc_desc);
        if (cache != PURC_VARIANT_INVALID) {
            inst->cache = cache;
            inst->input = input;
            purc_variant_ref(input);

            return inst;
        }
    }

    destroy(exe_filter_inst);
    return NULL;
}

// 用于执行选择
static purc_variant_t
exe_filter_choose(purc_exec_inst_t inst, const char* rule)
{
    if (!inst || !rule) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return PURC_VARIANT_INVALID;
    }

    struct pcexec_exe_filter_inst *exe_filter_inst;
    exe_filter_inst = (struct pcexec_exe_filter_inst*)inst;

    if (!parse_rule(exe_filter_inst, rule))
        return PURC_VARIANT_INVALID;

    purc_variant_t vals = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    if (vals == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    bool ok = false;

    purc_exec_iter_t it = fetch_begin(exe_filter_inst);

    for(; it; it = fetch_next(exe_filter_inst)) {
        purc_variant_t v = fetch_value(exe_filter_inst);
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
exe_filter_it_begin(purc_exec_inst_t inst, const char* rule)
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

    struct pcexec_exe_filter_inst *exe_filter_inst;
    exe_filter_inst = (struct pcexec_exe_filter_inst*)inst;

    if (!parse_rule(exe_filter_inst, rule))
        return NULL;

    PC_ASSERT(inst->cache != PURC_VARIANT_INVALID);

    return fetch_begin(exe_filter_inst);
}

// 根据迭代子获得对应的变体值
static purc_variant_t
exe_filter_it_value(purc_exec_inst_t inst, purc_exec_iter_t it)
{
    if (!inst || !it) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return NULL;
    }

    PC_ASSERT(&inst->it == it);
    PC_ASSERT(inst->input != PURC_VARIANT_INVALID);
    PC_ASSERT(inst->cache != PURC_VARIANT_INVALID);
    PC_ASSERT(inst->value != PURC_VARIANT_INVALID);

    struct pcexec_exe_filter_inst *exe_filter_inst;
    exe_filter_inst = (struct pcexec_exe_filter_inst*)inst;

    return fetch_value(exe_filter_inst);
}

// 获得下一个迭代子
// 注意: 规则字符串可能在前后两次迭代中发生变化，比如在规则中引用了变量的情形下。
// 如果规则并没有发生变化，则对 `rule` 参数传递 NULL。
static purc_exec_iter_t
exe_filter_it_next(purc_exec_inst_t inst, purc_exec_iter_t it, const char* rule)
{
    if (!inst || !it) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return NULL;
    }

    PC_ASSERT(&inst->it == it);
    PC_ASSERT(inst->input != PURC_VARIANT_INVALID);
    PC_ASSERT(inst->cache != PURC_VARIANT_INVALID);

    struct pcexec_exe_filter_inst *exe_filter_inst;
    exe_filter_inst = (struct pcexec_exe_filter_inst*)inst;

    if (rule) {
        if (!parse_rule(exe_filter_inst, rule))
            return NULL;
    }

    PC_ASSERT(inst->cache != PURC_VARIANT_INVALID);

    return fetch_next(exe_filter_inst);
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
exe_filter_reduce(purc_exec_inst_t inst, const char* rule)
{
    if (!inst || !rule) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return PURC_VARIANT_INVALID;
    }

    struct pcexec_exe_filter_inst *exe_filter_inst;
    exe_filter_inst = (struct pcexec_exe_filter_inst*)inst;

    if (!parse_rule(exe_filter_inst, rule))
        return PURC_VARIANT_INVALID;

    size_t count = 0;
    double sum   = 0;
    double avg   = 0;
    double max   = NAN;
    double min   = NAN;

    purc_exec_iter_t it = fetch_begin(exe_filter_inst);

    for(; it; it = fetch_next(exe_filter_inst)) {
        purc_variant_t v = fetch_value(exe_filter_inst);
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
exe_filter_destroy(purc_exec_inst_t inst)
{
    if (!inst) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return false;
    }

    struct pcexec_exe_filter_inst *exe_filter_inst;
    exe_filter_inst = (struct pcexec_exe_filter_inst*)inst;
    destroy(exe_filter_inst);

    return true;
}

static struct purc_exec_ops exe_filter_ops = {
    exe_filter_create,
    exe_filter_choose,
    exe_filter_it_begin,
    exe_filter_it_value,
    exe_filter_it_next,
    exe_filter_reduce,
    exe_filter_destroy,
};

int pcexec_exe_filter_register(void)
{
    bool ok = purc_register_executor("FILTER", &exe_filter_ops);
    return ok ? 0 : -1;
}

