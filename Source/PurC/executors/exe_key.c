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

#include "private/debug.h"
#include "private/errors.h"
#include "private/executor.h"
#include "private/variant.h"


struct pcexec_exe_key_inst {
    struct purc_exec_inst       super;

    struct exe_key_param        param;
};

// clear internal data except `input`
static inline void
exe_key_reset(struct pcexec_exe_key_inst *exe_key_inst)
{
    if (exe_key_inst->super.selected_keys) {
        purc_variant_unref(exe_key_inst->super.selected_keys);
        exe_key_inst->super.selected_keys = PURC_VARIANT_INVALID;
    }

    if (exe_key_inst->param.err_msg) {
        free(exe_key_inst->param.err_msg);
        exe_key_inst->param.err_msg = NULL;
    }

    if (exe_key_inst->param.rule.lexp) {
        logical_expression_destroy(exe_key_inst->param.rule.lexp);
        exe_key_inst->param.rule.lexp = NULL;
    }

    if (exe_key_inst->super.err_msg) {
        free(exe_key_inst->super.err_msg);
        exe_key_inst->super.err_msg = NULL;
    }
}

// 创建一个执行器实例
static purc_exec_inst_t
exe_key_create(enum purc_exec_type type, purc_variant_t input, bool asc_desc)
{
    if (!purc_variant_is_object(input))
        return NULL;

    struct pcexec_exe_key_inst *inst;
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
exe_key_parse_rule(purc_exec_inst_t inst, const char* rule)
{
    // parse and fill the internal fields from rule
    // for example, generating the `selected_keys` which contains all
    // selected exe_keys.

    // TODO: parse rule and eval to selected_keys
    struct pcexec_exe_key_inst *exe_key_inst;
    exe_key_inst = (struct pcexec_exe_key_inst*)inst;

    exe_key_reset(exe_key_inst);
    if (inst->err_msg) {
        free(inst->err_msg);
        inst->err_msg = NULL;
    }

    int r = exe_key_parse(rule, strlen(rule), &exe_key_inst->param);
    inst->err_msg = exe_key_inst->param.err_msg;
    exe_key_inst->param.err_msg = NULL;

    if (r)
        return false;

    PC_ASSERT(exe_key_inst->param.rule.lexp);

    inst->selected_keys = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    if (inst->selected_keys == PURC_VARIANT_INVALID) {
        return false;
    }

    purc_variant_t key, val;
    UNUSED_PARAM(val);
    foreach_key_value_in_variant_object(inst->input, key, val)
        const char *k = purc_variant_get_string_const(key);
        if (!k) {
            r = -1;
            break;
        }
        bool result = false;
        r = key_rule_eval(&exe_key_inst->param.rule, k, &result);
        if (r)
            break;

        if (result) {
            bool ok = false;
            ok = purc_variant_array_append(inst->selected_keys, key);
            if (!ok)
                r = -1;
        }

        if (r)
            break;
    end_foreach;

    return r ? false : true;
}

// 用于执行选择
static purc_variant_t
exe_key_choose(purc_exec_inst_t inst, const char* rule)
{
    if (!inst || !rule) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return PURC_VARIANT_INVALID;
    }

    if (inst->type != PURC_EXEC_TYPE_CHOOSE) {
        pcinst_set_error(PCEXECUTOR_ERROR_NOT_ALLOWED);
        return PURC_VARIANT_INVALID;
    }

    if (!exe_key_parse_rule(inst, rule))
        return PURC_VARIANT_INVALID;

    purc_variant_t vals = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    if (vals == PURC_VARIANT_INVALID) {
        return PURC_VARIANT_INVALID;
    }

    bool ok = true;

    struct pcexec_exe_key_inst *exe_key_inst;
    exe_key_inst = (struct pcexec_exe_key_inst*)inst;

    size_t sz = purc_variant_array_get_size(inst->selected_keys);
    for (size_t i=0; i<sz; ++i) {
        purc_variant_t k, v;
        foreach_value_in_variant_array(inst->selected_keys, k)
            switch (exe_key_inst->param.rule.for_clause)
            {
                case FOR_CLAUSE_VALUE:
                    v = purc_variant_object_get(inst->input, k);
                    ok = purc_variant_array_append(vals, v);
                    break;
                case FOR_CLAUSE_KEY:
                    ok = purc_variant_array_append(vals, k);
                    break;
                case FOR_CLAUSE_KV:
                {
                    purc_variant_t obj;
                    v = purc_variant_object_get(inst->input, k);
                    obj = purc_variant_make_object(1, k, v);
                    if (obj == PURC_VARIANT_INVALID) {
                        ok = false;
                        break;
                    }
                    ok = purc_variant_array_append(vals, obj);
                    purc_variant_unref(obj);
                } break;
            };
            if (!ok)
                break;
        end_foreach;
        if (!ok)
           break;
    }

    if (ok)
        return vals;

    purc_variant_unref(vals);
    return PURC_VARIANT_INVALID;
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

    inst->it.curr = 0;
    if (!exe_key_parse_rule(inst, rule))
        return NULL;

    size_t sz = purc_variant_array_get_size(inst->selected_keys);
    if (sz<=0) {
        pcinst_set_error(PCEXECUTOR_ERROR_NO_KEYS_SELECTED);
        return NULL;
    }

    return &inst->it;
}

// 根据迭代子获得对应的变体值
static purc_variant_t
exe_key_it_value(purc_exec_inst_t inst, purc_exec_iter_t it)
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
    PC_ASSERT(inst->selected_keys != PURC_VARIANT_INVALID);
    PC_ASSERT(inst->input != PURC_VARIANT_INVALID);

    purc_variant_t k;
    k = purc_variant_array_get(inst->selected_keys, it->curr);
    purc_variant_t v;
    v = purc_variant_object_get(inst->input, k);

    return v;
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

    if (inst->type != PURC_EXEC_TYPE_ITERATE) {
        pcinst_set_error(PCEXECUTOR_ERROR_NOT_ALLOWED);
        return NULL;
    }

    PC_ASSERT(&inst->it == it);

    if (rule) {
        // clear previously-selected-exe_keys
        if (inst->selected_keys) {
            purc_variant_unref(inst->selected_keys);
            inst->selected_keys = PURC_VARIANT_INVALID;
        }

        if (!exe_key_parse_rule(inst, rule))
            return NULL;
    }

    ++it->curr;

    size_t sz = purc_variant_array_get_size(inst->selected_keys);
    if (it->curr >= sz) {
        it->curr = sz;
        return NULL;
    }

    return it;
}

// 用于执行规约
static purc_variant_t
exe_key_reduce(purc_exec_inst_t inst, const char* rule)
{
    if (!inst || !rule) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return PURC_VARIANT_INVALID;
    }

    if (inst->type != PURC_EXEC_TYPE_REDUCE) {
        pcinst_set_error(PCEXECUTOR_ERROR_NOT_ALLOWED);
        return PURC_VARIANT_INVALID;
    }

    if (!exe_key_parse_rule(inst, rule))
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
exe_key_destroy(purc_exec_inst_t inst)
{
    if (!inst) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return false;
    }

    struct pcexec_exe_key_inst *exe_key_inst = (struct pcexec_exe_key_inst*)inst;
    exe_key_reset(exe_key_inst);

    if (inst->input) {
        purc_variant_unref(inst->input);
        inst->input = PURC_VARIANT_INVALID;
    }

    free(exe_key_inst);
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

