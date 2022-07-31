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

#include "keywords.h"

#include "purc.h"

#include <dlfcn.h>
#include <limits.h>
#include <math.h>

static void*
_load_module(const char *module)
{
#define PRINT_DEBUG
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
                else {
#ifdef PRINT_DEBUG        /* { */
                    PC_DEBUGX("Failed loading Executor from %s\n", so);
#endif                    /* } */
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
            else {
#ifdef PRINT_DEBUG        /* { */
                PC_DEBUGX("Failed loading Executor from %s\n", so);
#endif                    /* } */
            }
        }

    } while (0);

    if (!library_handle) {
        purc_set_error_with_info(PURC_ERROR_BAD_SYSTEM_CALL,
                "failed to load: %s", so);
        return NULL;
    }
#ifdef PRINT_DEBUG        /* { */
    PC_DEBUGX("loaded: %s", so);
#endif                    /* } */

    return library_handle;

#else                                                 /* }{ */
    UNUSED_PARAM(exe_func_inst);
    UNUSED_PARAM(module);

    // TODO: Add codes for other OS.
    pcinst_set_error (PURC_ERROR_NOT_SUPPORTED);
    PC_ASSERT(0); // Not implemented yet
#endif                                                /* } */
#undef PRINT_DEBUG
}

static int
_get_symbol_by_rule(const char *rule, void **handle, void **symbol)
{
    struct exe_func_param param = {0};
    param.debug_flex = 1;
    param.debug_bison = 0;

    int r = exe_func_parse(rule, strlen(rule), &param);
    if (r) {
        PC_DEBUGX(
                "failed parsing rule: %s\n"
                "err_msg: %s",
                rule, param.err_msg);
        exe_func_param_reset(&param);
        PC_ASSERT(purc_get_last_error());
        return -1;
    }

    const char *module = param.rule.module;
    const char *name   = param.rule.name;

    do {
        void *library_handle = NULL;
        library_handle = _load_module(module);
        if (!library_handle) {
            PC_ASSERT(purc_get_last_error());
            break;
        }

        void *p = dlsym(library_handle, name);
        if (dlerror() != NULL) {
            dlclose(library_handle);
            purc_set_error_with_info(PURC_ERROR_BAD_SYSTEM_CALL,
                    "failed to locate symbol `%s` from `%s`",
                    name, module);
            break;
        }
        PC_ASSERT(p);

        *handle = library_handle;
        *symbol = p;

        exe_func_param_reset(&param);

        return 0;
    } while (0);

    exe_func_param_reset(&param);
    return -1;
}

static purc_variant_t
exe_func_chooser(const char *rule,
        purc_variant_t on_value, purc_variant_t with_value)
{
    void *handle, *symbol;

    int r;
    r = _get_symbol_by_rule(rule, &handle, &symbol);
    if (r)
        return PURC_VARIANT_INVALID;

    PC_ASSERT(handle);
    PC_ASSERT(symbol);

    purc_variant_t (*chooser)(purc_variant_t on_value,
            purc_variant_t with_value);

    chooser = symbol;

    purc_variant_t v = chooser(on_value, with_value);

    dlclose(handle);

    return v;
}

// 迭代器，仅用于 `iterate` 动作元素。
static purc_variant_t
exe_func_iterator(const char *rule,
        purc_variant_t on_value, purc_variant_t with_value)
{
    void *handle, *symbol;

    int r;
    r = _get_symbol_by_rule(rule, &handle, &symbol);
    if (r)
        return PURC_VARIANT_INVALID;

    PC_ASSERT(handle);
    PC_ASSERT(symbol);

    purc_variant_t (*iterator)(purc_variant_t on_value,
            purc_variant_t with_value);

    iterator = symbol;

    purc_variant_t v = iterator(on_value, with_value);

    dlclose(handle);

    return v;
}

// 规约器，仅用于 `reduce` 动作元素。
static purc_variant_t
exe_func_reducer(const char *rule,
        purc_variant_t on_value, purc_variant_t with_value)
{
    void *handle, *symbol;

    int r;
    r = _get_symbol_by_rule(rule, &handle, &symbol);
    if (r)
        return PURC_VARIANT_INVALID;

    PC_ASSERT(handle);
    PC_ASSERT(symbol);

    purc_variant_t (*reducer)(purc_variant_t on_value,
            purc_variant_t with_value);

    reducer = symbol;

    purc_variant_t v = reducer(on_value, with_value);

    dlclose(handle);

    return v;
}

// 排序器，仅用于 `sort` 动作元素。
static purc_variant_t
exe_func_sorter(const char *rule,
        purc_variant_t on_value, purc_variant_t with_value,
        purc_variant_t against_value, bool desc, bool caseless)
{
    void *handle, *symbol;

    int r;
    r = _get_symbol_by_rule(rule, &handle, &symbol);
    if (r)
        return PURC_VARIANT_INVALID;

    PC_ASSERT(handle);
    PC_ASSERT(symbol);

    purc_variant_t (*sorter)(purc_variant_t on_value,
            purc_variant_t with_value, purc_variant_t against_value,
            bool desc, bool caseless);

    sorter = symbol;

    purc_variant_t v = sorter(on_value, with_value, against_value,
            desc, caseless);

    dlclose(handle);

    return v;
}

static struct pcexec_func_ops exe_func_ops = {
    exe_func_chooser,
    exe_func_iterator,
    exe_func_reducer,
    exe_func_sorter,
};

static struct pcexec_ops ops = {
    .type                    = PCEXEC_TYPE_EXTERNAL_FUNC,
    .external_func_ops       = &exe_func_ops,
};

int pcexec_exe_func_register(void)
{
    const char *name = "FUNC";

    if (ops.atom == 0) {
        ops.atom = PCHVML_KEYWORD_ATOM(HVML, name);
        if (ops.atom == 0) {
            fprintf(stderr, "unknown atom: %s\n", name);
            PC_ASSERT(0);
            return -1;
        }

        int r;
        r = pcexecutor_register(&ops);
        PC_ASSERT(r == 0);
        return r ? -1 : 0;
    }

    return -1;
}

