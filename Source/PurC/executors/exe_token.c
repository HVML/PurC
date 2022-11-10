/*
 * @file exe_token.c
 * @author Xu Xiaohong
 * @date 2021/10/10
 * @brief The implementation of public part for TOKEN executor.
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

#include "exe_token.h"

#include "pcexe-helper.h"

#include "private/executor.h"
#include "private/variant.h"

#include "private/debug.h"
#include "private/errors.h"

#include <math.h>

struct pcexec_exe_token_inst {
    struct purc_exec_inst       super;

    struct exe_token_param      param;

    purc_variant_t              result_set;
};

// clear internal data except `input`
static inline void
reset(struct pcexec_exe_token_inst *exe_token_inst)
{
    struct exe_token_param *param = &exe_token_inst->param;
    exe_token_param_reset(param);
    pcexecutor_inst_reset(&exe_token_inst->super);
    PCEXE_CLR_VAR(exe_token_inst->result_set);
}

static inline const char* 
get_next_token_w(const char *s, const wchar_t *delimiters,
        size_t *len, size_t *next)
{
    PC_ASSERT(s);
    PC_ASSERT(delimiters);
    PC_ASSERT(len);
    PC_ASSERT(next);
    PC_ASSERT(*s);

    const char *head = s;

    const char *p = s;
    while (*p) {
        wchar_t wc;
        int n = pcexe_utf8_to_wchar(p, &wc);
        PC_ASSERT(n > 0);
        if (wcschr(delimiters, wc)) {
            *len = p - head;
            *next = *len + n;
            return head;
        }
        p += n;
    }

    *len = p - head;
    *next = *len;

    return head;
}

static inline bool
init_result_set_w(struct pcexec_exe_token_inst *exe_token_inst,
        purc_variant_t result_set, const wchar_t *delimiters_w)
{
    purc_exec_inst_t inst = &exe_token_inst->super;
    purc_variant_t input = inst->input;

    const char *src = purc_variant_get_string_const(input);

    const char *p = src;
    while (*p) {
        size_t len, next;
        const char *token = get_next_token_w(p, delimiters_w, &len, &next);
        PC_ASSERT(token);
        purc_variant_t v = purc_variant_make_string_ex(token, len, true);
        if (v == PURC_VARIANT_INVALID)
            return false;
        bool ok = purc_variant_array_append(result_set, v);
        purc_variant_unref(v);
        if (!ok)
            return false;

        p += next;
    }

    return true;
}

static inline bool
init_result_set(struct pcexec_exe_token_inst *exe_token_inst,
        purc_variant_t result_set)
{
    struct token_rule *rule = &exe_token_inst->param.rule;

    const char *delimiters = " ";
    if (rule->delimiters && *rule->delimiters) {
        delimiters = rule->delimiters;
    }
    size_t bytes, chars;
    wchar_t *delimiters_w;
    delimiters_w = pcexe_wchar_from_utf8(delimiters, &bytes, &chars);
    if (!delimiters_w) {
        pcinst_set_error(PCEXECUTOR_ERROR_OOM);
        return false;
    }

    bool ok = init_result_set_w(exe_token_inst, result_set, delimiters_w);

    free(delimiters_w);

    if (ok) {
        PCEXE_CLR_VAR(exe_token_inst->result_set);
        exe_token_inst->result_set = result_set;
        purc_variant_ref(result_set);
    }

    return ok;
}

static inline bool
prepare_result_set(struct pcexec_exe_token_inst *exe_token_inst)
{
    purc_variant_t result_set;
    result_set = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    if (result_set == PURC_VARIANT_INVALID) {
        return false;
    }

    bool ok = init_result_set(exe_token_inst, result_set);
    purc_variant_unref(result_set);

    return ok;
}

static inline bool
parse_rule(struct pcexec_exe_token_inst *exe_token_inst,
        const char* rule)
{
    purc_exec_inst_t inst = &exe_token_inst->super;

    struct exe_token_param param = {0};
    int r = exe_token_parse(rule, strlen(rule), &param);
    if (inst->err_msg) {
        free(inst->err_msg);
        inst->err_msg = NULL;
    }

    if (r) {
        inst->err_msg = param.err_msg;
        param.err_msg = NULL;
        return false;
    }

    exe_token_param_reset(&exe_token_inst->param);
    exe_token_inst->param = param;

    return prepare_result_set(exe_token_inst);
}

int
token_rule_eval(struct token_rule *rule, purc_variant_t val, bool *result)
{
    struct string_matching_logical_expression *until = rule->until;
    *result = false;
    if (!until) {
        return 0;
    }

    return string_matching_logical_expression_match(until, val, result);
}

static inline bool
check_curr(struct pcexec_exe_token_inst *exe_token_inst)
{
    purc_exec_inst_t inst = &exe_token_inst->super;
    purc_exec_iter_t it = &inst->it;
    struct token_rule *rule = &exe_token_inst->param.rule;

    int curr = (int)it->curr;

    if (curr < 0) {
        pcinst_set_error(PCEXECUTOR_ERROR_NOT_EXISTS);
        return false;
    }

    purc_variant_t result_set = exe_token_inst->result_set;
    size_t nr;
    if (!purc_variant_array_size(result_set, &nr)) {
        pcinst_set_error(PCEXECUTOR_ERROR_NOT_EXISTS);
        return false;
    }

    if ((size_t)curr >= nr) {
        pcinst_set_error(PCEXECUTOR_ERROR_NOT_EXISTS);
        return false;
    }

    if (!isnan(rule->to)) {
        int to = rule->to;

        int advance = 1;
        if (!isnan(rule->advance)) {
            advance = rule->advance;
            PC_ASSERT(advance);
        }

        if (((advance > 0) && (curr > to)) ||
            ((advance < 0) && (curr < to)))
        {
            pcinst_set_error(PCEXECUTOR_ERROR_NOT_EXISTS);
            return false;
        }
    }

    purc_variant_t val = purc_variant_array_get(result_set, curr);
    bool result = false;
    if (token_rule_eval(rule, val, &result)) {
        // TODO: exception
        PC_ASSERT(0);
        return false;
    }
    if (result) {
        pcinst_set_error(PCEXECUTOR_ERROR_NOT_EXISTS);
        return false;
    }

    PCEXE_CLR_VAR(inst->value);
    inst->value = val;
    purc_variant_ref(val);

    return true;
}

static inline purc_exec_iter_t
fetch_begin(struct pcexec_exe_token_inst *exe_token_inst)
{
    purc_exec_inst_t inst = &exe_token_inst->super;
    purc_exec_iter_t it = &inst->it;
    struct token_rule *rule = &exe_token_inst->param.rule;
    it->curr = rule->from;
    if (check_curr(exe_token_inst)) {
        return it;
    }
    return NULL;
}

static inline purc_exec_iter_t
fetch_next(struct pcexec_exe_token_inst *exe_token_inst)
{
    purc_exec_inst_t inst = &exe_token_inst->super;
    purc_exec_iter_t it = &inst->it;
    struct token_rule *rule = &exe_token_inst->param.rule;
    if (isnan(rule->advance)) {
        it->curr += 1;
    } else {
        it->curr += rule->advance;
    }
    if (check_curr(exe_token_inst)) {
        return it;
    }
    return NULL;
}

static inline purc_variant_t
fetch_value(struct pcexec_exe_token_inst *exe_token_inst)
{
    purc_exec_inst_t inst = &exe_token_inst->super;

    return inst->value;
}

static inline void
destroy(struct pcexec_exe_token_inst *exe_token_inst)
{
    purc_exec_inst_t inst = &exe_token_inst->super;

    reset(exe_token_inst);

    PCEXE_CLR_VAR(inst->input);
    PCEXE_CLR_VAR(inst->value);

    free(exe_token_inst);
}

// 创建一个执行器实例
static purc_exec_inst_t
exe_token_create(enum purc_exec_type type,
        purc_variant_t input, bool asc_desc)
{
    struct pcexec_exe_token_inst *exe_token_inst;
    exe_token_inst = calloc(1, sizeof(*exe_token_inst));
    if (!exe_token_inst) {
        pcinst_set_error(PCEXECUTOR_ERROR_OOM);
        return NULL;
    }

    purc_exec_inst_t inst = &exe_token_inst->super;

    inst->type        = type;
    inst->asc_desc    = asc_desc;

    int debug_flex, debug_bison;
    pcexecutor_get_debug(&debug_flex, &debug_bison);
    exe_token_inst->param.debug_flex  = debug_flex;
    exe_token_inst->param.debug_bison = debug_bison;

    enum purc_variant_type vt = purc_variant_get_type(input);
    if (vt == PURC_VARIANT_TYPE_STRING) {
        inst->input = input;
        purc_variant_ref(input);
        return inst;
    }

    destroy(exe_token_inst);
    return NULL;
}

static inline purc_exec_iter_t
it_begin(struct pcexec_exe_token_inst *exe_token_inst, const char *rule)
{
    if (!parse_rule(exe_token_inst, rule))
        return NULL;

    return fetch_begin(exe_token_inst);
}

static inline purc_variant_t
it_value(struct pcexec_exe_token_inst *exe_token_inst)
{
    return fetch_value(exe_token_inst);
}

static inline purc_exec_iter_t
it_next(struct pcexec_exe_token_inst *exe_token_inst, const char *rule)
{
    if (rule) {
        if (!parse_rule(exe_token_inst, rule))
            return NULL;
    }

    return fetch_next(exe_token_inst);
}

// 用于执行选择
static purc_variant_t
exe_token_choose(purc_exec_inst_t inst, const char* rule)
{
    if (!inst || !rule) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return PURC_VARIANT_INVALID;
    }

    struct pcexec_exe_token_inst *exe_token_inst;
    exe_token_inst = (struct pcexec_exe_token_inst*)inst;

    purc_variant_t vals = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    if (vals == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    bool ok = true;

    purc_exec_iter_t it = it_begin(exe_token_inst, rule);
    if (!it && inst->err_msg) {
        purc_variant_unref(vals);
        return false;
    }

    for(; it; it = it_next(exe_token_inst, NULL)) {
        purc_variant_t v = it_value(exe_token_inst);
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
exe_token_it_begin(purc_exec_inst_t inst, const char* rule)
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

    struct pcexec_exe_token_inst *exe_token_inst;
    exe_token_inst = (struct pcexec_exe_token_inst*)inst;

    return it_begin(exe_token_inst, rule);
}

// 根据迭代子获得对应的变体值
static purc_variant_t
exe_token_it_value(purc_exec_inst_t inst, purc_exec_iter_t it)
{
    if (!inst || !it) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return NULL;
    }

    PC_ASSERT(&inst->it == it);
    PC_ASSERT(inst->input != PURC_VARIANT_INVALID);
    PC_ASSERT(inst->value != PURC_VARIANT_INVALID);

    struct pcexec_exe_token_inst *exe_token_inst;
    exe_token_inst = (struct pcexec_exe_token_inst*)inst;

    return it_value(exe_token_inst);
}

// 获得下一个迭代子
// 注意: 规则字符串可能在前后两次迭代中发生变化，比如在规则中引用了变量的情形下。
// 如果规则并没有发生变化，则对 `rule` 参数传递 NULL。
static purc_exec_iter_t
exe_token_it_next(purc_exec_inst_t inst, purc_exec_iter_t it, const char* rule)
{
    if (!inst || !it) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return NULL;
    }

    PC_ASSERT(&inst->it == it);
    PC_ASSERT(inst->input != PURC_VARIANT_INVALID);

    struct pcexec_exe_token_inst *exe_token_inst;
    exe_token_inst = (struct pcexec_exe_token_inst*)inst;

    return it_next(exe_token_inst, rule);
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
exe_token_reduce(purc_exec_inst_t inst, const char* rule)
{
    if (!inst || !rule) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return PURC_VARIANT_INVALID;
    }

    struct pcexec_exe_token_inst *exe_token_inst;
    exe_token_inst = (struct pcexec_exe_token_inst*)inst;

    size_t count = 0;
    double sum   = 0;
    double avg   = 0;
    double max   = NAN;
    double min   = NAN;

    purc_exec_iter_t it = it_begin(exe_token_inst, rule);
    if (!it && inst->err_msg) {
        return false;
    }

    for(; it; it = it_next(exe_token_inst, NULL)) {
        purc_variant_t v = it_value(exe_token_inst);
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
exe_token_destroy(purc_exec_inst_t inst)
{
    if (!inst) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return false;
    }

    struct pcexec_exe_token_inst *exe_token_inst;
    exe_token_inst = (struct pcexec_exe_token_inst*)inst;
    destroy(exe_token_inst);

    return true;
}

static struct purc_exec_ops exe_token_ops = {
    exe_token_create,
    exe_token_choose,
    exe_token_it_begin,
    exe_token_it_value,
    exe_token_it_next,
    exe_token_reduce,
    exe_token_destroy,
};

int pcexec_exe_token_register(void)
{
    bool ok = purc_register_executor("TOKEN", &exe_token_ops);
    return ok ? 0 : -1;
}

