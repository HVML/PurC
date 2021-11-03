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


    purc_variant_t        vals;

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

    if (exe_range_inst->vals != PURC_VARIANT_INVALID) {
        purc_variant_unref(exe_range_inst->vals);
        exe_range_inst->vals = PURC_VARIANT_INVALID;
    }
}

// 创建一个执行器实例
static purc_exec_inst_t
exe_range_create(enum purc_exec_type type, purc_variant_t input, bool asc_desc)
{
    if (!purc_variant_is_array(input) && !purc_variant_is_set(input))
        return NULL;

    struct pcexec_exe_range_inst *inst;
    inst = calloc(1, sizeof(*inst));
    if (!inst) {
        pcinst_set_error(PCEXECUTOR_ERROR_OOM);
        return NULL;
    }

    inst->super.type        = type;
    inst->super.input       = input;
    inst->super.asc_desc    = asc_desc;

    int debug_flex, debug_bison;
    pcexecutor_get_debug(&debug_flex, &debug_bison);
    inst->param.debug_flex  = debug_flex;
    inst->param.debug_bison = debug_bison;

    purc_variant_ref(input);

    return &inst->super;
}

static inline bool
exe_range_parse_rule(purc_exec_inst_t inst, const char* rule)
{
    // parse and fill the internal fields from rule
    // for example, generating the `selected_keys` which contains all
    // selected exe_ranges.

    // TODO: parse rule and eval to selected_keys
    struct pcexec_exe_range_inst *exe_range_inst;
    exe_range_inst = (struct pcexec_exe_range_inst*)inst;

    exe_range_reset(exe_range_inst);
    if (inst->err_msg) {
        free(inst->err_msg);
        inst->err_msg = NULL;
    }

    int r = exe_range_parse(rule, strlen(rule), &exe_range_inst->param);
    inst->err_msg = exe_range_inst->param.err_msg;
    exe_range_inst->param.err_msg = NULL;

    if (r)
        return false;

    if (purc_variant_is_array(inst->input)) {
        exe_range_inst->vals = inst->input;
        purc_variant_ref(exe_range_inst->vals);
    } else {
        purc_variant_t vals = purc_variant_make_array(0, PURC_VARIANT_INVALID);
        if (vals != PURC_VARIANT_INVALID) {
            purc_variant_t val;
            foreach_value_in_variant_set(inst->input, val);
            if (!purc_variant_array_append(vals, val)) {
                purc_variant_unref(vals);
                break;
            }
            end_foreach;
        }
        exe_range_inst->vals = vals;
    }

    if (exe_range_inst->vals == PURC_VARIANT_INVALID)
        return false;

    return true;
}

static inline bool
exe_range_refresh_from_to_advance(purc_exec_inst_t inst)
{
    struct pcexec_exe_range_inst *exe_range_inst;
    exe_range_inst = (struct pcexec_exe_range_inst*)inst;

    int64_t from = exe_range_inst->param.rule.from.i64;
    int64_t to = INT64_MAX;
    int64_t advance = 1;

    if (from < 0) {
        return false; // out of range
    }

    if (exe_range_inst->param.rule.has_advance) {
        advance = exe_range_inst->param.rule.has_advance;
        if (advance == 0) {
            return false; // potentially infinite loop
        }
    }

    if (exe_range_inst->param.rule.has_to) {
        to = exe_range_inst->param.rule.to.i64;
        if (advance > 0) {
            if (to == INT64_MAX) {
                return false; // out of range
            }
            ++to;
        } else {
            if (to == INT64_MIN) {
                return false; // out of range
            }
            --to;
        }
    } else {
        if (advance < 0) {
            to = 0;
        }
    }

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

    if (!exe_range_parse_rule(inst, rule))
        return PURC_VARIANT_INVALID;

    struct pcexec_exe_range_inst *exe_range_inst;
    exe_range_inst = (struct pcexec_exe_range_inst*)inst;

    purc_variant_ref(exe_range_inst->vals);

    return exe_range_inst->vals;
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
    exe_range_inst->from = 0;
    exe_range_inst->to = 0;
    exe_range_inst->advance = 0;
    if (!exe_range_parse_rule(inst, rule))
        return NULL;

    if (!exe_range_refresh_from_to_advance(inst)) {
        return NULL;
    }

    size_t sz = purc_variant_array_get_size(exe_range_inst->vals);
    if (exe_range_inst->from <0 || (size_t)exe_range_inst->from>=sz) {
        return NULL; // out of range
    }
    if (exe_range_inst->advance > 0) {
        if (exe_range_inst->from >= exe_range_inst->to) {
            return NULL; // out of range
        }
    } else {
        if (exe_range_inst->from <= exe_range_inst->to) {
            return NULL; // out of range
        }
    }
    exe_range_inst->curr = exe_range_inst->from;
    return &inst->it;
}

// 根据迭代子获得对应的变体值
static purc_variant_t
exe_range_it_value(purc_exec_inst_t inst, purc_exec_iter_t it)
{
    if (!inst || !it) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return PURC_VARIANT_INVALID;
    }

    if (inst->type != PURC_EXEC_TYPE_ITERATE) {
        pcinst_set_error(PCEXECUTOR_ERROR_NOT_ALLOWED);
        return PURC_VARIANT_INVALID;
    }

    PC_ASSERT(&inst->it == it);
    PC_ASSERT(inst->input != PURC_VARIANT_INVALID);

    struct pcexec_exe_range_inst *exe_range_inst;
    exe_range_inst = (struct pcexec_exe_range_inst*)inst;
    PC_ASSERT(exe_range_inst->vals != PURC_VARIANT_INVALID);

    purc_variant_t vals = exe_range_inst->vals;
    PC_ASSERT(vals != PURC_VARIANT_INVALID);
    PC_ASSERT(exe_range_inst->curr >=0);
    PC_ASSERT((size_t)exe_range_inst->curr < purc_variant_array_get_size(vals));

    return purc_variant_array_get(vals, exe_range_inst->curr);
}

// 获得下一个迭代子
// 注意: 规则字符串可能在前后两次迭代中发生变化，比如在规则中引用了变量的情形下。
// 如果规则并没有发生变化，则对 `rule` 参数传递 NULL。
static purc_exec_iter_t
exe_range_it_next(purc_exec_inst_t inst, purc_exec_iter_t it, const char* rule)
{
    if (!inst || !it) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return NULL;
    }

    if (inst->type != PURC_EXEC_TYPE_ITERATE) {
        pcinst_set_error(PCEXECUTOR_ERROR_NOT_ALLOWED);
        return NULL;
    }

    PC_ASSERT(&inst->it == it);

    struct pcexec_exe_range_inst *exe_range_inst;
    exe_range_inst = (struct pcexec_exe_range_inst*)inst;

    if (rule) {
        int64_t curr = exe_range_inst->curr;
        purc_exec_iter_t it = exe_range_it_begin(inst, rule);
        if (!it) {
            exe_range_inst->curr = -1;
            return NULL;
        }
        PC_ASSERT(it == &inst->it);
        exe_range_inst->curr = curr;
    }

    PC_ASSERT(&inst->it == it);
    PC_ASSERT(inst->input != PURC_VARIANT_INVALID);

    PC_ASSERT(exe_range_inst->vals != PURC_VARIANT_INVALID);

    purc_variant_t vals = exe_range_inst->vals;
    PC_ASSERT(vals != PURC_VARIANT_INVALID);
    size_t sz = purc_variant_array_get_size(vals);
    if (exe_range_inst->curr<0 || (size_t)exe_range_inst->curr>=sz)
        return NULL;

    exe_range_inst->curr += exe_range_inst->advance;
    if (exe_range_inst->curr<0 || (size_t)exe_range_inst->curr>=sz)
        return NULL;

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

    struct pcexec_exe_range_inst *exe_range_inst = (struct pcexec_exe_range_inst*)inst;
    exe_range_reset(exe_range_inst);

    if (inst->input) {
        purc_variant_unref(inst->input);
        inst->input = PURC_VARIANT_INVALID;
    }

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

