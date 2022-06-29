/*
 * @file exe_func.c
 * @author Xu Xiaohong
 * @date 2022/06/17
 * @brief The implementation of public part for FUNC executor.
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

#define _GNU_SOURCE
#include "exe_func.h"

#include "private/executor.h"
#include "private/variant.h"

#include "private/debug.h"
#include "private/errors.h"
#include "purc.h"

#include <dlfcn.h>
#include <limits.h>
#include <math.h>

struct pcexec_exe_func_inst {
    struct purc_exec_inst       super;

    struct exe_func_param        param;

    purc_variant_t              results;
    size_t                      curr;

    void                       *handle;
    void                       *symbol;

    // // 选择器，可用于 `choose` 和 `test` 动作元素
    // purc_variant_t (*chooser)(purc_variant_t on_value,
    //         purc_variant_t with_value);

    // // 迭代器，仅用于 `iterate` 动作元素。
    // purc_variant_t (*iterator)(purc_variant_t on_value,
    //         purc_variant_t with_value);

    // // 规约器，仅用于 `reduce` 动作元素。
    // purc_variant_t (*reducer)(purc_variant_t on_value,
    //         purc_variant_t with_value);

    // // 排序器，仅用于 `sort` 动作元素。
    // purc_variant_t (*sorter)(purc_variant_t on_value,
    //         purc_variant_t with_value,
    //         purc_variant_t against_value, bool desc, bool caseless);

    unsigned int                loaded:1;
};

// clear internal data except `input`
static inline void
reset(struct pcexec_exe_func_inst *exe_func_inst)
{
    if (exe_func_inst->handle) {
        dlclose(exe_func_inst->handle);
        exe_func_inst->handle = NULL;
        exe_func_inst->loaded = 0;
    }
    PURC_VARIANT_SAFE_CLEAR(exe_func_inst->results);

    struct exe_func_param *param = &exe_func_inst->param;
    exe_func_param_reset(param);
    pcexecutor_inst_reset(&exe_func_inst->super);
}

static inline int
load_module(struct pcexec_exe_func_inst *exe_func_inst, const char *module)
{
#define PRINT_DEBUG
    PC_ASSERT(exe_func_inst->loaded == 0);

#if OS(LINUX) || OS(UNIX) || OS(MAC_OS_X)             /* { */
    const char *ext = ".so";
#if OS(MAC_OS_X)
    ext = ".dylib";
#endif

    void *library_handle = NULL;

    char so[PATH_MAX+1];
    int n;
    do {
        /* XXX: the order of searching directories:
         *
         * 1. the valid directories contains in the environment variable:
         *      PURC_EXECUTOR_PATH
         * 2. /usr/local/lib/purc-<purc-api-version>/
         * 3. /usr/lib/purc-<purc-api-version>/
         * 4. /lib/purc-<purc-api-version>/
         */

        // step1: search in directories defined by the env var
        const char *env = getenv(PURC_ENVV_EXECUTOR_PATH);
        if (env) {
            char *path = strdup(env);
            char *str1;
            char *saveptr1;
            char *dir;

            for (str1 = path; ; str1 = NULL) {
                dir = strtok_r(str1, ":;", &saveptr1);
                if (dir == NULL || dir[0] != '/') {
                    break;
                }

                n = snprintf(so, sizeof(so),
                        "%s/libpurc-executor-%s%s", dir, module, ext);
                PC_ASSERT(n>0 && (size_t)n<sizeof(so));
                library_handle = dlopen(so, RTLD_LAZY);
                if (library_handle) {
#ifdef PRINT_DEBUG        /* { */
                    PC_DEBUGX("Loaded Executor from %s\n", so);
#endif                    /* } */
                    break;
                }
            }

            free(path);

            if (library_handle)
                break;
        }

        static const char *ver = PURC_API_VERSION_STRING;

        // try in system directories.
        static const char *other_tries[] = {
            "/usr/local/lib/purc-%s/libpurc-executor-%s%s",
            "/usr/lib/purc-%s/libpurc-executor-%s%s",
            "/lib/purc-%s/libpurc-executor-%s%s",
        };

        for (size_t i = 0; i < PCA_TABLESIZE(other_tries); i++) {
            n = snprintf(so, sizeof(so), other_tries[i], ver, module, ext);
            PC_ASSERT(n>0 && (size_t)n<sizeof(so));
            library_handle = dlopen(so, RTLD_LAZY);
            if (library_handle) {
#ifdef PRINT_DEBUG        /* { */
                PC_DEBUGX("Loaded Executor from %s\n", so);
#endif                    /* } */
                break;
            }
        }

    } while (0);

    if (!library_handle) {
        purc_set_error_with_info(PURC_ERROR_BAD_SYSTEM_CALL,
                "failed to load: %s", so);
        return -1;
    }
#ifdef PRINT_DEBUG        /* { */
    PC_DEBUGX("loaded: %s", so);
#endif                    /* } */

    exe_func_inst->handle = library_handle;
    exe_func_inst->loaded = 1;
    return 0;

#else                                                 /* }{ */
    UNUSED_PARAM(exe_func_inst);
    UNUSED_PARAM(module);

    // TODO: Add codes for other OS.
    pcinst_set_error (PURC_ERROR_NOT_SUPPORTED);
    PC_ASSERT(0); // Not implemented yet
#endif                                                /* } */
#undef PRINT_DEBUG
}

static inline bool
parse_rule(struct pcexec_exe_func_inst *exe_func_inst,
        const char* rule)
{
    PC_ASSERT(exe_func_inst->loaded == 0);

    purc_exec_inst_t inst = &exe_func_inst->super;

    struct exe_func_param param = {0};
    param.debug_flex = 1;
    param.debug_bison = 0;

    int r = exe_func_parse(rule, strlen(rule), &param);
    if (inst->err_msg) {
        free(inst->err_msg);
        inst->err_msg = NULL;
    }

    if (r) {
        inst->err_msg = param.err_msg;
        param.err_msg = NULL;
        return false;
    }

    exe_func_param_reset(&exe_func_inst->param);
    exe_func_inst->param = param;

    const char *name = exe_func_inst->param.rule.name;
    const char *module = exe_func_inst->param.rule.module;
    if (module) {
        if (load_module(exe_func_inst, module))
            return false;
    }
    void *symbol = dlsym(exe_func_inst->handle, name);
    if (dlerror() != NULL) {
        pcinst_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        return false;
    }
    exe_func_inst->symbol = symbol;

    return true;
}

static inline bool
check_curr(struct pcexec_exe_func_inst *exe_func_inst, const size_t curr)
{
    purc_exec_inst_t inst = &exe_func_inst->super;

    purc_variant_t results = exe_func_inst->results;

    size_t sz;
    if (!purc_variant_array_size(results, &sz) ||
        curr >= sz)
    {
        return false;
    }

    PURC_VARIANT_SAFE_CLEAR(inst->value);
    inst->value = purc_variant_ref(purc_variant_array_get(results, curr));
    return true;
}

static inline purc_exec_iter_t
fetch_begin(struct pcexec_exe_func_inst *exe_func_inst)
{
    purc_exec_inst_t inst = &exe_func_inst->super;
    purc_exec_iter_t it = &inst->it;
    purc_variant_t input = inst->input;
    purc_variant_t with = inst->with;

    if (exe_func_inst->symbol == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    purc_variant_t (*iterator)(purc_variant_t on_value,
            purc_variant_t with_value) = exe_func_inst->symbol;

    purc_variant_t results;
    results = iterator(input, with);
    PC_ASSERT(results != PURC_VARIANT_INVALID);
    // TODO: linear api
    PC_ASSERT(purc_variant_is_array(results));

    PURC_VARIANT_SAFE_CLEAR(exe_func_inst->results);
    exe_func_inst->results = results;

    size_t curr = 0;
    if (!check_curr(exe_func_inst, curr))
        return NULL;

    return it;
}

static inline purc_variant_t
fetch_value(struct pcexec_exe_func_inst *exe_func_inst)
{
    purc_exec_inst_t inst = &exe_func_inst->super;
    return inst->value;
}

static inline purc_exec_iter_t
fetch_next(struct pcexec_exe_func_inst *exe_func_inst)
{
    purc_exec_inst_t inst = &exe_func_inst->super;
    purc_exec_iter_t it = &inst->it;

    exe_func_inst->curr += 1;

    if (!check_curr(exe_func_inst, exe_func_inst->curr))
        return NULL;

    return it;
}

static inline purc_exec_iter_t
it_begin(struct pcexec_exe_func_inst *exe_func_inst, const char *rule)
{
    if (!parse_rule(exe_func_inst, rule))
        return NULL;

    return fetch_begin(exe_func_inst);
}

static inline purc_variant_t
it_value(struct pcexec_exe_func_inst *exe_func_inst)
{
    return fetch_value(exe_func_inst);
}

static inline purc_exec_iter_t
it_next(struct pcexec_exe_func_inst *exe_func_inst, const char *rule)
{
    UNUSED_PARAM(rule);

    return fetch_next(exe_func_inst);
}

static inline void
destroy(struct pcexec_exe_func_inst *exe_func_inst)
{
    purc_exec_inst_t inst = &exe_func_inst->super;

    reset(exe_func_inst);

    PCEXE_CLR_VAR(inst->input);
    PCEXE_CLR_VAR(inst->value);

    free(exe_func_inst);
}

// 创建一个执行器实例
static purc_exec_inst_t
exe_func_create(enum purc_exec_type type,
        purc_variant_t input, bool asc_desc)
{
    struct pcexec_exe_func_inst *inst;
    inst = calloc(1, sizeof(*inst));
    if (!inst) {
        pcinst_set_error(PCEXECUTOR_ERROR_OOM);
        return NULL;
    }

    inst->super.type        = type;
    inst->super.input       = input;
    inst->super.asc_desc    = asc_desc;
    inst->super.with        = PURC_VARIANT_INVALID;

    purc_variant_ref(input);

    return &inst->super;
}

// 用于执行选择
static purc_variant_t
exe_func_choose(purc_exec_inst_t inst, const char* rule)
{
    if (!inst || !rule) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return PURC_VARIANT_INVALID;
    }

    PC_ASSERT(inst->type == PURC_EXEC_TYPE_CHOOSE);

    struct pcexec_exe_func_inst *exe_func_inst;
    exe_func_inst = (struct pcexec_exe_func_inst*)inst;

    bool ok = true;

    ok = parse_rule(exe_func_inst, rule);
    if (!ok) {
        PC_ASSERT(purc_get_last_error());
        return PURC_VARIANT_INVALID;
    }

    if (exe_func_inst->symbol == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    purc_variant_t input = inst->input;
    purc_variant_t with = inst->with;

    purc_variant_t (*chooser)(purc_variant_t on_value,
            purc_variant_t with_value) = exe_func_inst->symbol;

    return chooser(input, with);
}

// 获得用于迭代的初始迭代子
static purc_exec_iter_t
exe_func_it_begin(purc_exec_inst_t inst, const char* rule)
{
    if (!inst || !rule) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return NULL;
    }

    PC_ASSERT(inst->type == PURC_EXEC_TYPE_ITERATE);

    struct pcexec_exe_func_inst *exe_func_inst;
    exe_func_inst = (struct pcexec_exe_func_inst*)inst;

    return it_begin(exe_func_inst, rule);
}

// 根据迭代子获得对应的变体值
static purc_variant_t
exe_func_it_value(purc_exec_inst_t inst, purc_exec_iter_t it)
{
    if (!inst || !it) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return NULL;
    }

    PC_ASSERT(&inst->it == it);
    // PC_ASSERT(inst->selected_keys != PURC_VARIANT_INVALID);
    PC_ASSERT(inst->input != PURC_VARIANT_INVALID);

    struct pcexec_exe_func_inst *exe_func_inst;
    exe_func_inst = (struct pcexec_exe_func_inst*)inst;

    return it_value(exe_func_inst);
}

// 获得下一个迭代子
// 注意: 规则字符串可能在前后两次迭代中发生变化，比如在规则中引用了变量的情形下。
// 如果规则并没有发生变化，则对 `rule` 参数传递 NULL。
static purc_exec_iter_t
exe_func_it_next(purc_exec_inst_t inst, purc_exec_iter_t it, const char* rule)
{
    if (!inst || !it) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return NULL;
    }

    PC_ASSERT(&inst->it == it);
    PC_ASSERT(inst->input != PURC_VARIANT_INVALID);

    struct pcexec_exe_func_inst *exe_func_inst;
    exe_func_inst = (struct pcexec_exe_func_inst*)inst;

    return it_next(exe_func_inst, rule);
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
exe_func_reduce(purc_exec_inst_t inst, const char* rule)
{
    if (!inst || !rule) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return PURC_VARIANT_INVALID;
    }

    struct pcexec_exe_func_inst *exe_func_inst;
    exe_func_inst = (struct pcexec_exe_func_inst*)inst;

    bool ok = true;
    ok = parse_rule(exe_func_inst, rule);
    if (!ok) {
        PC_ASSERT(purc_get_last_error());
        return PURC_VARIANT_INVALID;
    }

    if (exe_func_inst->symbol == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t input = inst->input;
    purc_variant_t with = inst->with;
    purc_variant_t (*iterator)(purc_variant_t on_value,
            purc_variant_t with_value) = exe_func_inst->symbol;

    purc_variant_t result = iterator(input, with);
    return result;
}

// 销毁一个执行器实例
static bool
exe_func_destroy(purc_exec_inst_t inst)
{
    if (!inst) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return false;
    }

    struct pcexec_exe_func_inst *exe_func_inst;
    exe_func_inst = (struct pcexec_exe_func_inst*)inst;
    destroy(exe_func_inst);

    return true;
}

static struct purc_exec_ops exe_func_ops = {
    exe_func_create,
    exe_func_choose,
    exe_func_it_begin,
    exe_func_it_value,
    exe_func_it_next,
    exe_func_reduce,
    exe_func_destroy,
};

int pcexec_exe_func_register(void)
{
    bool ok = pcexecutor_register("FUNC", PCEXECUTOR_EXTERNAL, &exe_func_ops);
    return ok ? 0 : -1;
}

