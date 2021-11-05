/*
 * @file exe_range.c
 * @author Xu Xiaohong
 * @date 2021/10/10
 * @brief The implementation of public part for RANGE executor.
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

#include "exe_range.h"

#include "private/debug.h"
#include "private/errors.h"
#include "private/executor.h"
#include "private/variant.h"


struct pcexec_exe_range_inst {
    struct purc_exec_inst       super;

    struct exe_range_param        param;

    purc_variant_t        cache;

    int64_t       from;
    int64_t       to;
    int64_t       advance;

    int64_t       curr;
};

// clear internal data except `input`
static inline void
exe_range_reset(struct pcexec_exe_range_inst *exe_range_inst)
{
    if (exe_range_inst->param.err_msg) {
        free(exe_range_inst->param.err_msg);
        exe_range_inst->param.err_msg = NULL;
    }

    if (exe_range_inst->super.err_msg) {
        free(exe_range_inst->super.err_msg);
        exe_range_inst->super.err_msg = NULL;
    }
}

// 创建一个执行器实例
static purc_exec_inst_t
exe_range_create(enum purc_exec_type type, purc_variant_t input, bool asc_desc)
{
    purc_variant_t cache;
    enum purc_variant_type vt = purc_variant_get_type(input);
    switch (vt)
    {
        case PURC_VARIANT_TYPE_OBJECT:
        {
            cache = pcexe_cache_object(input, asc_desc);
        } break;
        case PURC_VARIANT_TYPE_ARRAY:
        {
            cache = pcexe_cache_array(input, asc_desc);
        } break;
        case PURC_VARIANT_TYPE_SET:
        {
            cache = pcexe_cache_set(input, asc_desc);
        } break;
        default:
        {
            pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
            return NULL;
        }
    }

    if (cache == PURC_VARIANT_INVALID) {
        return NULL;
    }

    struct pcexec_exe_range_inst *inst;
    inst = calloc(1, sizeof(*inst));
    if (!inst) {
        pcinst_set_error(PCEXECUTOR_ERROR_OOM);
        purc_variant_unref(cache);
        return NULL;
    }

    inst->super.type        = type;
    inst->super.input       = input;
    inst->super.asc_desc    = asc_desc;

    int debug_flex, debug_bison;
    pcexecutor_get_debug(&debug_flex, &debug_bison);
    inst->param.debug_flex  = debug_flex;
    inst->param.debug_bison = debug_bison;

    inst->cache = cache;

    purc_variant_ref(input);

    return &inst->super;
}

static inline bool
exe_range_parse_rule(purc_exec_inst_t inst, const char* rule)
{
    struct pcexec_exe_range_inst *exe_range_inst;
    exe_range_inst = (struct pcexec_exe_range_inst*)inst;

    exe_range_reset(exe_range_inst);
    if (inst->err_msg) {
        free(inst->err_msg);
        inst->err_msg = NULL;
    }

    struct exe_range_param *param = &exe_range_inst->param;
    param->rule_valid = 0;
    int r = exe_range_parse(rule, strlen(rule), &exe_range_inst->param);
    inst->err_msg = exe_range_inst->param.err_msg;
    exe_range_inst->param.err_msg = NULL;

    if (r)
        return false;

    struct range_rule *rr = &param->rule;
    int64_t from, to, advance;
    from = rr->from;

    if (rr->has_advance) {
        advance = rr->advance;
    } else {
        advance = 1;
    }
    if (advance == 0) {
        pcinst_set_error(PCEXECUTOR_ERROR_NOT_ALLOWED);
        return false;
    }

    if (rr->has_to) {
        to = rr->to;
    } else {
        purc_variant_t cache = exe_range_inst->cache;
        size_t sz = purc_variant_array_get_size(cache);
        if (advance > 0) {
            to = sz - 1;
        } else {
            to = 0;
        }
    }

    exe_range_inst->from             = from;
    exe_range_inst->to               = to;
    exe_range_inst->advance          = advance;

    param->rule_valid = 1;
    return true;
}

// 用于执行选择
static purc_variant_t
exe_range_choose(purc_exec_inst_t inst, const char* rule)
{
    if (!inst || !rule) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return PURC_VARIANT_INVALID;
    }

    if (inst->type != PURC_EXEC_TYPE_CHOOSE) {
        pcinst_set_error(PCEXECUTOR_ERROR_NOT_ALLOWED);
        return PURC_VARIANT_INVALID;
    }

    struct pcexec_exe_range_inst *exe_range_inst;
    exe_range_inst = (struct pcexec_exe_range_inst*)inst;

    purc_variant_t cache = exe_range_inst->cache;
    PC_ASSERT(cache != PURC_VARIANT_INVALID);
    PC_ASSERT(purc_variant_is_array(cache));

    if (!exe_range_parse_rule(inst, rule))
        return PURC_VARIANT_INVALID;

    purc_variant_t vals = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    if (vals == PURC_VARIANT_INVALID) {
        return PURC_VARIANT_INVALID;
    }

    int64_t from    = exe_range_inst->from;
    int64_t to      = exe_range_inst->to;
    int64_t advance = exe_range_inst->advance;
    size_t sz = purc_variant_array_get_size(cache);
    bool ok = false;
    for (int64_t i=from; i>=0 && (size_t)i<sz; i += advance) {
        if (advance > 0) {
            if (i > to)
                break;
        } else {
            if (i < to)
                break;
        }
        purc_variant_t v = purc_variant_array_get(cache, i);
        PC_ASSERT(v != PURC_VARIANT_INVALID);
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
exe_range_it_begin(purc_exec_inst_t inst, const char* rule)
{
    if (!inst || !rule) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return NULL;
    }

    if (inst->type != PURC_EXEC_TYPE_ITERATE) {
        pcinst_set_error(PCEXECUTOR_ERROR_NOT_ALLOWED);
        return NULL;
    }

    struct pcexec_exe_range_inst *exe_range_inst;
    exe_range_inst = (struct pcexec_exe_range_inst*)inst;

    purc_variant_t cache = exe_range_inst->cache;
    PC_ASSERT(cache != PURC_VARIANT_INVALID);
    PC_ASSERT(purc_variant_is_array(cache));

    if (!exe_range_parse_rule(inst, rule))
        return NULL;

    size_t sz = purc_variant_array_get_size(cache);
    int64_t from = exe_range_inst->from;
    if (from < 0 || (size_t)from >= sz) {
        pcinst_set_error(PCEXECUTOR_ERROR_OUT_OF_RANGE);
        exe_range_inst->curr = -1;
        return NULL;
    }

    int64_t to = exe_range_inst->to;
    int64_t advance = exe_range_inst->advance;
    if (advance > 0) {
        if (from <= to) {
            pcinst_set_error(PCEXECUTOR_ERROR_OUT_OF_RANGE);
            exe_range_inst->curr = -1;
            return NULL;
        }
    } else {
        if (from >= to) {
            pcinst_set_error(PCEXECUTOR_ERROR_OUT_OF_RANGE);
            exe_range_inst->curr = -1;
            return NULL;
        }
    }

    exe_range_inst->curr = exe_range_inst->param.rule.from;

    return &exe_range_inst->super.it;
}

// 根据迭代子获得对应的变体值
static purc_variant_t
exe_range_it_value(purc_exec_inst_t inst, purc_exec_iter_t it)
{
    PC_ASSERT(inst && it);
    PC_ASSERT(inst->type == PURC_EXEC_TYPE_ITERATE);
    PC_ASSERT(&inst->it == it);

    struct pcexec_exe_range_inst *exe_range_inst;
    exe_range_inst = (struct pcexec_exe_range_inst*)inst;

    purc_variant_t cache = exe_range_inst->cache;
    PC_ASSERT(cache != PURC_VARIANT_INVALID);
    PC_ASSERT(purc_variant_is_array(cache));

    int64_t curr = exe_range_inst->curr;
    PC_ASSERT(curr >= 0 && (size_t)curr < purc_variant_array_get_size(cache));

    return purc_variant_array_get(cache, curr);
}

// 获得下一个迭代子
// 注意: 规则字符串可能在前后两次迭代中发生变化，
// 比如在规则中引用了变量的情形下。
// 如果规则并没有发生变化，则对 `rule` 参数传递 NULL。
static purc_exec_iter_t
exe_range_it_next(purc_exec_inst_t inst, purc_exec_iter_t it, const char* rule)
{
    PC_ASSERT(inst && it);
    PC_ASSERT(inst->type == PURC_EXEC_TYPE_ITERATE);
    PC_ASSERT(&inst->it == it);

    struct pcexec_exe_range_inst *exe_range_inst;
    exe_range_inst = (struct pcexec_exe_range_inst*)inst;

    purc_variant_t cache = exe_range_inst->cache;
    PC_ASSERT(cache != PURC_VARIANT_INVALID);
    PC_ASSERT(purc_variant_is_array(cache));

    struct exe_range_param *param = &exe_range_inst->param;
    PC_ASSERT(param->rule_valid);

    if (rule) {
        if (!exe_range_parse_rule(inst, rule))
            return NULL;
    }

    size_t sz = purc_variant_array_get_size(cache);
    if (exe_range_inst->curr<0 || (size_t)exe_range_inst->curr>=sz) {
        pcinst_set_error(PCEXECUTOR_ERROR_OUT_OF_RANGE);
        exe_range_inst->curr = -1;
        return NULL;
    }

    int64_t to = exe_range_inst->to;
    int64_t advance = exe_range_inst->advance;

    int64_t next = exe_range_inst->curr + advance;
    if (next<0 || (size_t)next>=sz) {
        pcinst_set_error(PCEXECUTOR_ERROR_OUT_OF_RANGE);
        exe_range_inst->curr = -1;
        return NULL;
    }
    if (advance > 0) {
        if (next > to) {
            pcinst_set_error(PCEXECUTOR_ERROR_OUT_OF_RANGE);
            exe_range_inst->curr = -1;
            return NULL;
        }
    } else {
        if (next < to) {
            pcinst_set_error(PCEXECUTOR_ERROR_OUT_OF_RANGE);
            exe_range_inst->curr = -1;
            return NULL;
        }
    }

    exe_range_inst->curr = next;

    return it;
}

// 用于执行规约
static purc_variant_t
exe_range_reduce(purc_exec_inst_t inst, const char* rule)
{
    if (!inst || !rule) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return PURC_VARIANT_INVALID;
    }

    if (inst->type != PURC_EXEC_TYPE_REDUCE) {
        pcinst_set_error(PCEXECUTOR_ERROR_NOT_ALLOWED);
        return PURC_VARIANT_INVALID;
    }

    if (!exe_range_parse_rule(inst, rule))
        return PURC_VARIANT_INVALID;

    size_t sz = purc_variant_array_get_size(inst->selected_keys);

    purc_variant_t objs;
    objs = purc_variant_make_object(0,
                PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    if (objs == PURC_VARIANT_INVALID) {
        return objs;
    }

    bool ok = false;

    for (size_t i=0; i<sz; ++i) {
        purc_variant_t k;
        k = purc_variant_array_get(inst->selected_keys, i);
        purc_variant_t v;
        v = purc_variant_object_get(inst->input, k);
        if (v==PURC_VARIANT_INVALID)
            continue;
        ok = purc_variant_object_set(objs, k, v);
        if (!ok)
            break;
    }

    if (ok)
        return objs;

    purc_variant_unref(objs);
    return PURC_VARIANT_INVALID;
}

// 销毁一个执行器实例
static bool
exe_range_destroy(purc_exec_inst_t inst)
{
    if (!inst) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return false;
    }

    struct pcexec_exe_range_inst *exe_range_inst;
    exe_range_inst = (struct pcexec_exe_range_inst*)inst;
    exe_range_reset(exe_range_inst);

    if (inst->input) {
        purc_variant_unref(inst->input);
        inst->input = PURC_VARIANT_INVALID;
    }
    PCEXE_CLR_VAR(exe_range_inst->cache);

    free(exe_range_inst);
    return true;
}

static struct purc_exec_ops exe_range_ops = {
    exe_range_create,
    exe_range_choose,
    exe_range_it_begin,
    exe_range_it_value,
    exe_range_it_next,
    exe_range_reduce,
    exe_range_destroy,
};

int pcexec_exe_range_register(void)
{
    bool ok = purc_register_executor("RANGE", &exe_range_ops);
    return ok ? 0 : -1;
}

