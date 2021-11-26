/*
 * @file exe_char.c
 * @author Xu Xiaohong
 * @date 2021/10/10
 * @brief The implementation of public part for CHAR executor.
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

#include "exe_char.h"

#include "pcexe-helper.h"

#include "private/executor.h"
#include "private/variant.h"

#include "private/debug.h"
#include "private/errors.h"

#include <math.h>

struct pcexec_exe_char_inst {
    struct purc_exec_inst       super;

    int                       it_pos;

    struct exe_char_param     param;
};

// clear internal data except `input`
static inline void
reset(struct pcexec_exe_char_inst *exe_char_inst)
{
    struct exe_char_param *param = &exe_char_inst->param;
    exe_char_param_reset(param);
    exe_char_inst->it_pos = 0;
    pcexecutor_inst_reset(&exe_char_inst->super);
}

static inline bool
parse_rule(struct pcexec_exe_char_inst *exe_char_inst,
        const char* rule)
{
    purc_exec_inst_t inst = &exe_char_inst->super;

    reset(exe_char_inst);
    PCEXE_CLR_VAR(inst->value);

    struct exe_char_param *param = &exe_char_inst->param;
    param->rule_valid = 0;
    int r = exe_char_parse(rule, strlen(rule), &exe_char_inst->param);
    inst->err_msg = exe_char_inst->param.err_msg;
    exe_char_inst->param.err_msg = NULL;

    if (r)
        return false;

    param->rule_valid = 1;
    return true;
}

static inline purc_exec_iter_t
fetch_and_cache(struct pcexec_exe_char_inst *exe_char_inst)
{
    purc_exec_inst_t inst = &exe_char_inst->super;
    purc_exec_iter_t it = &inst->it;

    PC_ASSERT(0);
    return it;
}

int
char_rule_eval(struct char_rule *rule, purc_variant_t val, bool *result)
{
    if (rule->until) {
        const char *str = purc_variant_get_string_const(val);
        if (strcmp(str, rule->until) == 0) {
            *result = false;
            return -1;
        }
    }
    *result = true;
    return 0;
}

static inline purc_exec_iter_t
char_string_until_match(struct pcexec_exe_char_inst *exe_char_inst)
{
    purc_exec_inst_t inst = &exe_char_inst->super;
    purc_exec_iter_t it = &inst->it;
    purc_variant_t cache = inst->cache;
    const char *str = purc_variant_get_string_const(cache);
    size_t len = strlen(str);
    struct char_rule *rule = &exe_char_inst->param.rule;

    PC_ASSERT(exe_char_inst->param.rule_valid);

    int it_pos = exe_char_inst->it_pos;

    purc_variant_t found = PURC_VARIANT_INVALID;
    bool result = false;
    char buf[2];
    buf[1] = '\0';
    if (rule->advance)
        len -= *rule->advance;

    while (true) {
        if (it_pos < 0 || (size_t)it_pos >= len) {
            pcinst_set_error(PCEXECUTOR_ERROR_NOT_EXISTS);
            break;
        }
        if (rule->to && it_pos >= *(rule->to)) {
            pcinst_set_error(PCEXECUTOR_ERROR_NOT_EXISTS);
            break;
        }
        buf[0]  = str[it_pos];
        // TODO: better with wchar
        found = purc_variant_make_string(buf, false);
        int r = char_rule_eval(rule, found, &result);
        if (r)
            break;

        if (result)
            break;

        purc_variant_unref(found);
        ++it_pos;
    };

    if (!result) {
        if (found != PURC_VARIANT_INVALID) {
            purc_variant_unref(found);
            found = PURC_VARIANT_INVALID;
        }
        return NULL;
    }

    exe_char_inst->it_pos = it_pos;
    PCEXE_CLR_VAR(inst->value);
    inst->value = found;
    return it;
}

static inline purc_exec_iter_t
fetch_begin(struct pcexec_exe_char_inst *exe_char_inst)
{
    purc_exec_inst_t inst = &exe_char_inst->super;
    purc_variant_t cache = inst->cache;
    struct char_rule *rule = &exe_char_inst->param.rule;
    switch (purc_variant_get_type(cache))
    {
        case PURC_VARIANT_TYPE_STRING:
            exe_char_inst->it_pos = rule->from;
            return char_string_until_match(exe_char_inst);
        default:
            pcinst_set_error(PCEXECUTOR_ERROR_NOT_IMPLEMENTED);
            return NULL;
    }
}

static inline purc_exec_iter_t
fetch_next(struct pcexec_exe_char_inst *exe_char_inst)
{
    purc_exec_inst_t inst = &exe_char_inst->super;
    purc_variant_t cache = inst->cache;
    struct char_rule *rule = &exe_char_inst->param.rule;
    switch (purc_variant_get_type(cache))
    {
        case PURC_VARIANT_TYPE_STRING:
            if (rule->advance)
                exe_char_inst->it_pos += *(rule->advance);
            return char_string_until_match(exe_char_inst);
        default:
            pcinst_set_error(PCEXECUTOR_ERROR_NOT_IMPLEMENTED);
            return NULL;
    }
}

static inline purc_variant_t
fetch_value(struct pcexec_exe_char_inst *exe_char_inst)
{
    purc_exec_inst_t inst = &exe_char_inst->super;

    return inst->value;
}

static inline void
destroy(struct pcexec_exe_char_inst *exe_char_inst)
{
    purc_exec_inst_t inst = &exe_char_inst->super;

    reset(exe_char_inst);

    PCEXE_CLR_VAR(inst->input);
    PCEXE_CLR_VAR(inst->cache);
    PCEXE_CLR_VAR(inst->value);

    free(exe_char_inst);
}

// 创建一个执行器实例
static purc_exec_inst_t
exe_char_create(enum purc_exec_type type,
        purc_variant_t input, bool asc_desc)
{
    struct pcexec_exe_char_inst *exe_char_inst;
    exe_char_inst = calloc(1, sizeof(*exe_char_inst));
    if (!exe_char_inst) {
        pcinst_set_error(PCEXECUTOR_ERROR_OOM);
        return NULL;
    }

    purc_exec_inst_t inst = &exe_char_inst->super;

    inst->type        = type;
    inst->asc_desc    = asc_desc;

    int debug_flex, debug_bison;
    pcexecutor_get_debug(&debug_flex, &debug_bison);
    exe_char_inst->param.debug_flex  = debug_flex;
    exe_char_inst->param.debug_bison = debug_bison;

    enum purc_variant_type vt = purc_variant_get_type(input);
    if (vt == PURC_VARIANT_TYPE_STRING) {
        purc_variant_t cache = pcexe_make_cache(input, asc_desc);
        if (cache != PURC_VARIANT_INVALID) {
            inst->cache = cache;
            inst->input = input;
            purc_variant_ref(input);

            return inst;
        }
    }

    destroy(exe_char_inst);
    return NULL;
}

// 用于执行选择
static purc_variant_t
exe_char_choose(purc_exec_inst_t inst, const char* rule)
{
    if (!inst || !rule) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return PURC_VARIANT_INVALID;
    }

    struct pcexec_exe_char_inst *exe_char_inst;
    exe_char_inst = (struct pcexec_exe_char_inst*)inst;

    if (!parse_rule(exe_char_inst, rule))
        return PURC_VARIANT_INVALID;

    purc_variant_t vals = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    if (vals == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    bool ok = false;

    purc_exec_iter_t it = fetch_begin(exe_char_inst);

    for(; it; it = fetch_next(exe_char_inst)) {
        purc_variant_t v = fetch_value(exe_char_inst);
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
exe_char_it_begin(purc_exec_inst_t inst, const char* rule)
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

    struct pcexec_exe_char_inst *exe_char_inst;
    exe_char_inst = (struct pcexec_exe_char_inst*)inst;

    if (!parse_rule(exe_char_inst, rule))
        return NULL;

    PC_ASSERT(inst->cache != PURC_VARIANT_INVALID);

    return fetch_begin(exe_char_inst);
}

// 根据迭代子获得对应的变体值
static purc_variant_t
exe_char_it_value(purc_exec_inst_t inst, purc_exec_iter_t it)
{
    if (!inst || !it) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return NULL;
    }

    PC_ASSERT(&inst->it == it);
    PC_ASSERT(inst->input != PURC_VARIANT_INVALID);
    PC_ASSERT(inst->cache != PURC_VARIANT_INVALID);
    PC_ASSERT(inst->value != PURC_VARIANT_INVALID);

    struct pcexec_exe_char_inst *exe_char_inst;
    exe_char_inst = (struct pcexec_exe_char_inst*)inst;

    return fetch_value(exe_char_inst);
}

// 获得下一个迭代子
// 注意: 规则字符串可能在前后两次迭代中发生变化，比如在规则中引用了变量的情形下。
// 如果规则并没有发生变化，则对 `rule` 参数传递 NULL。
static purc_exec_iter_t
exe_char_it_next(purc_exec_inst_t inst, purc_exec_iter_t it, const char* rule)
{
    if (!inst || !it) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return NULL;
    }

    PC_ASSERT(&inst->it == it);
    PC_ASSERT(inst->input != PURC_VARIANT_INVALID);
    PC_ASSERT(inst->cache != PURC_VARIANT_INVALID);

    struct pcexec_exe_char_inst *exe_char_inst;
    exe_char_inst = (struct pcexec_exe_char_inst*)inst;

    if (rule) {
        if (!parse_rule(exe_char_inst, rule))
            return NULL;
    }

    PC_ASSERT(inst->cache != PURC_VARIANT_INVALID);

    return fetch_next(exe_char_inst);
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
exe_char_reduce(purc_exec_inst_t inst, const char* rule)
{
    if (!inst || !rule) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return PURC_VARIANT_INVALID;
    }

    struct pcexec_exe_char_inst *exe_char_inst;
    exe_char_inst = (struct pcexec_exe_char_inst*)inst;

    if (!parse_rule(exe_char_inst, rule))
        return PURC_VARIANT_INVALID;

    size_t count = 0;
    double sum   = 0;
    double avg   = 0;
    double max   = NAN;
    double min   = NAN;

    purc_exec_iter_t it = fetch_begin(exe_char_inst);

    for(; it; it = fetch_next(exe_char_inst)) {
        purc_variant_t v = fetch_value(exe_char_inst);
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
exe_char_destroy(purc_exec_inst_t inst)
{
    if (!inst) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return false;
    }

    struct pcexec_exe_char_inst *exe_char_inst;
    exe_char_inst = (struct pcexec_exe_char_inst*)inst;
    destroy(exe_char_inst);

    return true;
}

static struct purc_exec_ops exe_char_ops = {
    exe_char_create,
    exe_char_choose,
    exe_char_it_begin,
    exe_char_it_value,
    exe_char_it_next,
    exe_char_reduce,
    exe_char_destroy,
};

int pcexec_exe_char_register(void)
{
    bool ok = purc_register_executor("CHAR", &exe_char_ops);
    return ok ? 0 : -1;
}

