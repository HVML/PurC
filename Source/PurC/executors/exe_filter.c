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

    struct exe_filter_param        param;

    purc_variant_t              result_set;
};

// clear internal data except `input`
static inline void
reset(struct pcexec_exe_filter_inst *exe_filter_inst)
{
    struct exe_filter_param *param = &exe_filter_inst->param;
    exe_filter_param_reset(param);
    pcexecutor_inst_reset(&exe_filter_inst->super);
    PCEXE_CLR_VAR(exe_filter_inst->result_set);
}

static inline bool
init_result_set_with_object(struct pcexec_exe_filter_inst *exe_filter_inst,
        purc_variant_t result_set)
{
    purc_exec_inst_t inst = &exe_filter_inst->super;
    purc_variant_t input = inst->input;

    bool ok = true;
    purc_variant_t k, v;
    foreach_key_value_in_variant_object(input, k, v)
        purc_variant_t arr = purc_variant_make_array(2, k, v);
        if (arr == PURC_VARIANT_INVALID) {
            ok = false;
            break;
        }
        if (!purc_variant_array_append(result_set, arr)) {
            ok = false;
            break;
        }
    end_foreach;

    if (ok) {
        PCEXE_CLR_VAR(exe_filter_inst->result_set);
        exe_filter_inst->result_set = result_set;
        purc_variant_ref(result_set);
    }

    return ok;
}

static inline bool
init_result_set_with_array(struct pcexec_exe_filter_inst *exe_filter_inst,
        purc_variant_t result_set)
{
    purc_exec_inst_t inst = &exe_filter_inst->super;
    purc_variant_t input = inst->input;

    bool ok = true;
    purc_variant_t v;
    size_t idx;
    foreach_value_in_variant_array(input, v, idx)
        (void)idx;
        if (!purc_variant_array_append(result_set, v)) {
            ok = false;
            break;
        }
    end_foreach;

    if (ok) {
        PCEXE_CLR_VAR(exe_filter_inst->result_set);
        exe_filter_inst->result_set = result_set;
        purc_variant_ref(result_set);
    }

    return ok;
}

static inline bool
init_result_set_with_set(struct pcexec_exe_filter_inst *exe_filter_inst,
        purc_variant_t result_set)
{
    purc_exec_inst_t inst = &exe_filter_inst->super;
    purc_variant_t input = inst->input;

    bool ok = true;
    purc_variant_t v;
    // FIXME: document-order or content-order?
    foreach_value_in_variant_set(input, v)
        if (!purc_variant_array_append(result_set, v)) {
            ok = false;
            break;
        }
    end_foreach;

    if (ok) {
        PCEXE_CLR_VAR(exe_filter_inst->result_set);
        exe_filter_inst->result_set = result_set;
        purc_variant_ref(result_set);
    }

    return ok;
}

static inline bool
init_result_set(struct pcexec_exe_filter_inst *exe_filter_inst,
        purc_variant_t result_set)
{
    purc_exec_inst_t inst = &exe_filter_inst->super;
    purc_variant_t input = inst->input;

    switch (purc_variant_get_type(input)) {
        case PURC_VARIANT_TYPE_OBJECT:
            return init_result_set_with_object(exe_filter_inst, result_set);
        case PURC_VARIANT_TYPE_ARRAY:
            return init_result_set_with_array(exe_filter_inst, result_set);
        case PURC_VARIANT_TYPE_SET:
            return init_result_set_with_set(exe_filter_inst, result_set);
        default:
            PC_ASSERT(0);
            return false;
    }

    bool ok = true;
    purc_variant_t k, v;
    foreach_key_value_in_variant_object(input, k, v)
        if (!purc_variant_array_append(result_set, k)) {
            ok = false;
            break;
        }
        if (!purc_variant_array_append(result_set, v)) {
            ok = false;
            break;
        }
    end_foreach;

    if (ok) {
        PCEXE_CLR_VAR(exe_filter_inst->result_set);
        exe_filter_inst->result_set = result_set;
        purc_variant_ref(result_set);
    }

    return ok;
}

static inline bool
prepare_result_set(struct pcexec_exe_filter_inst *exe_filter_inst)
{
    purc_variant_t result_set;
    result_set = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    if (result_set == PURC_VARIANT_INVALID) {
        return false;
    }

    bool ok = init_result_set(exe_filter_inst, result_set);
    purc_variant_unref(result_set);

    return ok;
}

static inline bool
parse_rule(struct pcexec_exe_filter_inst *exe_filter_inst,
        const char* rule)
{
    purc_exec_inst_t inst = &exe_filter_inst->super;

    struct exe_filter_param param = {0};
    int r = exe_filter_parse(rule, strlen(rule), &param);
    if (inst->err_msg) {
        free(inst->err_msg);
        inst->err_msg = NULL;
    }

    if (r) {
        inst->err_msg = param.err_msg;
        param.err_msg = NULL;
        return false;
    }

    exe_filter_param_reset(&exe_filter_inst->param);
    exe_filter_inst->param = param;

    return prepare_result_set(exe_filter_inst);
}

int
filter_rule_eval(struct filter_rule *rule, purc_variant_t val, bool *result)
{
    struct number_comparing_logical_expression *ncle = rule->ncle;
    struct string_matching_logical_expression  *smle = rule->smle;
    if (!ncle && !smle) {
        *result = true;
        return 0;
    }

    *result = false;
    if (smle) {
        PC_ASSERT(!ncle);
        return string_matching_logical_expression_match(smle, val, result);
    }

    double curr = purc_variant_numerify(val);
    return number_comparing_logical_expression_match(ncle, curr, result);
}

static inline bool
check_item_with_object(struct pcexec_exe_filter_inst *exe_filter_inst,
    const int curr, purc_variant_t item, bool *result)
{
    purc_exec_inst_t inst = &exe_filter_inst->super;
    purc_exec_iter_t it = &inst->it;
    struct filter_rule *rule = &exe_filter_inst->param.rule;

    purc_variant_t v = purc_variant_array_get(item, 1);
    PC_ASSERT(v != PURC_VARIANT_INVALID);

    if (filter_rule_eval(rule, v, result)) {
        // TODO: exception
        PC_ASSERT(0);
        return false;
    }
    if (!result)
        return false;

    purc_variant_t k = purc_variant_array_get(item, 0);
    PC_ASSERT(k != PURC_VARIANT_INVALID);

    purc_variant_t val = PURC_VARIANT_INVALID;

    switch (rule->for_clause) {
        case FOR_CLAUSE_VALUE:
            val = v;
            purc_variant_ref(val);
            break;
        case FOR_CLAUSE_KEY:
            val = k;
            purc_variant_ref(val);
            break;
        case FOR_CLAUSE_KV:
            val = purc_variant_make_object_by_static_ckey(2,
                    "k", k, "v", v);
            break;
    }

    PCEXE_CLR_VAR(inst->value);
    inst->value = val;
    it->curr = curr;

    return true;
}

static inline bool
check_item_with_array(struct pcexec_exe_filter_inst *exe_filter_inst,
    const int curr, purc_variant_t item, bool *result)
{
    purc_exec_inst_t inst = &exe_filter_inst->super;
    purc_exec_iter_t it = &inst->it;
    struct filter_rule *rule = &exe_filter_inst->param.rule;

    if (filter_rule_eval(rule, item, result)) {
        // TODO: exception
        PC_ASSERT(0);
        return false;
    }
    if (!result)
        return true;

    PCEXE_CLR_VAR(inst->value);
    inst->value = item;
    purc_variant_ref(item);
    it->curr = curr;

    return true;
}

static inline bool
check_item_with_set(struct pcexec_exe_filter_inst *exe_filter_inst,
    const int curr, purc_variant_t item, bool *result)
{
    purc_exec_inst_t inst = &exe_filter_inst->super;
    purc_exec_iter_t it = &inst->it;
    struct filter_rule *rule = &exe_filter_inst->param.rule;

    purc_variant_t input = inst->input;
    const char *unique_key = NULL;

    /* FIXME: no unique key  */
    bool r = purc_variant_set_unique_keys(input, &unique_key);
    if (!r) {
        *result = true;
    }

    purc_variant_t v = purc_variant_object_get_by_ckey_ex(item, unique_key,
            true);
    if (!v) {
        *result = false;
    }
    else if (filter_rule_eval(rule, v, result)) {
        // TODO: exception
        PC_ASSERT(0);
        return false;
    }

    if (!result)
        return false;

    PCEXE_CLR_VAR(inst->value);
    inst->value = item;
    purc_variant_ref(item);
    it->curr = curr;

    return true;
}

static inline bool
check_item(struct pcexec_exe_filter_inst *exe_filter_inst,
    const int curr, purc_variant_t item, bool *result)
{
    purc_exec_inst_t inst = &exe_filter_inst->super;
    purc_variant_t input = inst->input;

    switch (purc_variant_get_type(input)) {
        case PURC_VARIANT_TYPE_OBJECT:
            return check_item_with_object(exe_filter_inst, curr, item, result);
        case PURC_VARIANT_TYPE_ARRAY:
            return check_item_with_array(exe_filter_inst, curr, item, result);
        case PURC_VARIANT_TYPE_SET:
            return check_item_with_set(exe_filter_inst, curr, item, result);
        default:
            PC_ASSERT(0);
            return false;
    }
}

static inline bool
check_curr(struct pcexec_exe_filter_inst *exe_filter_inst)
{
    purc_exec_inst_t inst = &exe_filter_inst->super;
    purc_exec_iter_t it = &inst->it;

    int curr = (int)it->curr;

    if (curr < 0) {
        pcinst_set_error(PCEXECUTOR_ERROR_NOT_EXISTS);
        return false;
    }

    purc_variant_t result_set = exe_filter_inst->result_set;
    size_t nr;
    if (!purc_variant_array_size(result_set, &nr)) {
        pcinst_set_error(PCEXECUTOR_ERROR_NOT_EXISTS);
        return false;
    }

    bool result = false;
    while (!result) {
        if ((size_t)curr >= nr) {
            /* End the loop normally without set error code */
            // pcinst_set_error(PCEXECUTOR_ERROR_NOT_EXISTS);
            return false;
        }

        purc_variant_t item = purc_variant_array_get(result_set, curr);
        if (!check_item(exe_filter_inst, curr, item, &result)) {
            // TODO: exception
            PC_ASSERT(0);
            return false;
        }
        if (!result) {
            curr += 1;
            continue;
        }

        return true;
    }

    pcinst_set_error(PCEXECUTOR_ERROR_NOT_EXISTS);
    return false;
}

static inline purc_exec_iter_t
fetch_begin(struct pcexec_exe_filter_inst *exe_filter_inst)
{
    purc_exec_inst_t inst = &exe_filter_inst->super;
    purc_exec_iter_t it = &inst->it;
    it->curr = 0;
    if (check_curr(exe_filter_inst)) {
        return it;
    }
    return NULL;
}

static inline purc_exec_iter_t
fetch_next(struct pcexec_exe_filter_inst *exe_filter_inst)
{
    purc_exec_inst_t inst = &exe_filter_inst->super;
    purc_exec_iter_t it = &inst->it;
    it->curr += 1;
    if (check_curr(exe_filter_inst)) {
        return it;
    }
    return NULL;
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
        inst->input = input;
        purc_variant_ref(input);
        return inst;
    }

    destroy(exe_filter_inst);
    return NULL;
}

static inline purc_exec_iter_t
it_begin(struct pcexec_exe_filter_inst *exe_filter_inst, const char *rule)
{
    if (!parse_rule(exe_filter_inst, rule))
        return NULL;

    return fetch_begin(exe_filter_inst);
}

static inline purc_variant_t
it_value(struct pcexec_exe_filter_inst *exe_filter_inst)
{
    return fetch_value(exe_filter_inst);
}

static inline purc_exec_iter_t
it_next(struct pcexec_exe_filter_inst *exe_filter_inst, const char *rule)
{
    if (rule) {
        if (!parse_rule(exe_filter_inst, rule))
            return NULL;
    }

    return fetch_next(exe_filter_inst);
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

    purc_variant_t vals = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    if (vals == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    bool ok = true;

    purc_exec_iter_t it = it_begin(exe_filter_inst, rule);
    if (!it && inst->err_msg) {
        purc_variant_unref(vals);
        return false;
    }

    for(; it; it = it_next(exe_filter_inst, NULL)) {
        purc_variant_t v = it_value(exe_filter_inst);
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

    return it_begin(exe_filter_inst, rule);
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
    PC_ASSERT(inst->value != PURC_VARIANT_INVALID);

    struct pcexec_exe_filter_inst *exe_filter_inst;
    exe_filter_inst = (struct pcexec_exe_filter_inst*)inst;

    return it_value(exe_filter_inst);
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

    struct pcexec_exe_filter_inst *exe_filter_inst;
    exe_filter_inst = (struct pcexec_exe_filter_inst*)inst;

    return it_next(exe_filter_inst, rule);
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

    size_t count = 0;
    double sum   = 0;
    double avg   = 0;
    double max   = NAN;
    double min   = NAN;

    purc_exec_iter_t it = it_begin(exe_filter_inst, rule);
    if (!it && inst->err_msg) {
        return false;
    }

    for(; it; it = it_next(exe_filter_inst, NULL)) {
        purc_variant_t v = it_value(exe_filter_inst);
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

