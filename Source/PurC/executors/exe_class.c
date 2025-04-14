/*
 * @file exe_class.c
 * @author Xu Xiaohong
 * @date 2022/06/17
 * @brief The implementation of public part for CLASS executor.
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
#include "exe_class.h"

#include "private/executor.h"
#include "private/variant.h"

#include "private/debug.h"
#include "private/errors.h"
#include "private/stringbuilder.h"

#include "keywords.h"

#include "purc.h"

#include <dlfcn.h>
#include <limits.h>
#include <math.h>

static void*
_load_module(const char *module)
{
#define PRINT_DEBUG
#if OS(LINUX) || OS(UNIX) || OS(DARWIN)             /* { */
    const char *ext = ".so";
#   if OS(DARWIN)
    ext = ".dylib";
#   endif

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
    UNUSED_PARAM(exe_class_inst);
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
    struct exe_class_param param = {0};
#ifndef NDEBUG
    param.debug_flex = 1;
    param.debug_bison = 1;
#else
    param.debug_flex = 0;
    param.debug_bison = 0;
#endif

    int r = exe_class_parse(rule, strlen(rule), &param);
    if (r) {
        PC_DEBUGX(
                "failed parsing rule: %s\n"
                "err_msg: %s",
                rule, param.err_msg);
        exe_class_param_reset(&param);
        PC_ASSERT(purc_get_last_error());
        return -1;
    }

    const char *module = param.rule.module;

    struct pcutils_string name;
    pcutils_string_init(&name, 64);

    do {
        r = pcutils_string_append(&name, "%s_instantiate", param.rule.name);
        if (r) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            break;
        }

        void *library_handle = NULL;
        library_handle = _load_module(module);
        if (!library_handle) {
            PC_ASSERT(purc_get_last_error());
            break;
        }

        const char *s = pcutils_string_get(&name);
        void *p = dlsym(library_handle, s);
        if (dlerror() != NULL) {
            dlclose(library_handle);
            purc_set_error_with_info(PURC_ERROR_BAD_SYSTEM_CALL,
                    "failed to locate symbol `%s` from `%s`",
                    s, module);
            break;
        }
        PC_ASSERT(p);

        *handle = library_handle;
        *symbol = p;

        pcutils_string_reset(&name);
        exe_class_param_reset(&param);

        return 0;
    } while (0);

    pcutils_string_reset(&name);
    exe_class_param_reset(&param);
    return -1;
}

struct pcexec_class_iter {
    void                      *handle;
    struct purc_iterator_ops   ops;

    purc_variant_t             it;

    purc_variant_t             val;
};

static void
it_release(pcexec_class_iter_t it)
{
    if (it) {
        PURC_VARIANT_SAFE_CLEAR(it->val);
        PURC_VARIANT_SAFE_CLEAR(it->it);
        if (it->handle) {
            dlclose(it->handle);
            it->handle = NULL;
        }
    }
}

static void
it_destroy(pcexec_class_iter_t it)
{
    if (it) {
        it_release(it);
        free(it);
    }
}

static pcexec_class_iter_t
exe_class_it_begin(const char* rule, purc_variant_t on,
        purc_variant_t with)
{
    void *handle, *symbol;

    pcexec_class_iter_t it;
    it = (pcexec_class_iter_t)calloc(1, sizeof(*it));
    if (!it) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    do {
        int r;
        r = _get_symbol_by_rule(rule, &handle, &symbol);
        if (r)
            break;

        PC_ASSERT(handle);
        PC_ASSERT(symbol);

        it->handle = handle;

        struct purc_iterator_ops* (*instantiate)(void);
        instantiate = symbol;

        struct purc_iterator_ops  *ops;
        ops = instantiate();

        if (!ops) {
            PC_ASSERT(purc_get_last_error());
            break;
        }

        it->ops = *ops;

        if (!it->ops.begin || !it->ops.next) {
            purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                    "bad ops from external class executor");
            break;
        }

        it->it = it->ops.begin(on, with);
        if (it->it == PURC_VARIANT_INVALID) {
            break;
        }
        else {
            PURC_VARIANT_SAFE_CLEAR(it->val);
            it->val = it->ops.next(it->it);
            if (it->val == PURC_VARIANT_INVALID)
                break;
        }

        return it;
    } while (0);

    it_destroy(it);
    return NULL;
}

static purc_variant_t
exe_class_it_value(pcexec_class_iter_t it)
{
    PC_ASSERT(it);
    PC_ASSERT(it->val != PURC_VARIANT_INVALID);
    return it->val;
}

static pcexec_class_iter_t
exe_class_it_next(pcexec_class_iter_t it)
{
    PC_ASSERT(it);
    PC_ASSERT(it->ops.next);
    PURC_VARIANT_SAFE_CLEAR(it->val);
    it->val = it->ops.next(it->it);
    if (it->val == PURC_VARIANT_INVALID) {
        it_destroy(it);
        return NULL;
    }

    return it;
}

static void
exe_class_it_destroy(pcexec_class_iter_t it)
{
    it_destroy(it);
}

static struct pcexec_class_ops exe_class_ops = {
    exe_class_it_begin,
    exe_class_it_value,
    exe_class_it_next,
    exe_class_it_destroy,
};

static struct pcexec_ops ops = {
    .type                    = PCEXEC_TYPE_EXTERNAL_CLASS,
    .external_class_ops      = &exe_class_ops,
};

int pcexec_exe_class_register(void)
{
    const char *name = "CLASS";

    if (ops.atom == 0) {
        ops.atom = PCHVML_KEYWORD_ATOM(HVML, name);
        if (ops.atom == 0) {
            PC_ERROR("unknown atom: %s\n", name);
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

