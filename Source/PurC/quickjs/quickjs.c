/*
 * @file quickjs.c
 * @date 2025/08/05
 * @brief The initializer of the QuickJS module.
 *
 * Copyright (C) 2025 FMSoft <https://www.fmsoft.cn>
 *
 * Authors:
 *  Vincent Wei (https://github.com/VincentWei)
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
 *
 */
#define _GNU_SOURCE
#include "config.h"

#include "private/instance.h"
#include "private/debug.h"

#include "purc-macros.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wcast-function-type"
#include "quickjs/quickjs.h"
#include "quickjs/quickjs-libc.h"
#pragma GCC diagnostic pop

#include <stdlib.h>

static size_t get_suffixed_size(const char *str)
{
    char *p;
    size_t v;
    v = (size_t)strtod(str, &p);
    switch(*p) {
    case 'G':
        v <<= 30;
        break;
    case 'M':
        v <<= 20;
        break;
    case 'k':
    case 'K':
        v <<= 10;
        break;
    default:
        if (*p != '\0') {
            PC_WARN("PurC/QuickJS: invalid suffix: %s\n", p);
            v = 0;;
        }
        break;
    }

    return v;
}

/* also used to initialize the worker context */
JSContext *JS_NewCustomContext(JSRuntime *rt)
{
    JSContext *ctx;
    ctx = JS_NewContext(rt);
    if (!ctx)
        return NULL;
    /* system modules */
    js_init_module_std(ctx, "std");
    js_init_module_os(ctx, "os");
    return ctx;
}

static int _init_instance(struct pcinst *inst,
        const purc_instance_extra_info* extra_info)
{
    (void)extra_info;
    inst->js_rt = JS_NewRuntime();

    if (!inst->js_rt) {
        PC_WARN("Cannot allocate JS runtime\n");
        return -1;
    }

    inst->js_memory_limit = -1;
    inst->js_max_stack_size = 0;
    inst->js_gc_threshold = 256 * 1024;
    inst->js_promise_rejection_tracker = NULL;

    const char *envv;
    envv = getenv(PURC_ENVV_JSRT_MEM_LIMIT);
    if (envv) {
        size_t sz = get_suffixed_size(envv);
        if (sz)
            JS_SetMemoryLimit(inst->js_rt, sz);
        inst->js_memory_limit = sz;
    }

    envv = getenv(PURC_ENVV_JSRT_STACK_SIZE);
    if (envv) {
        size_t sz = get_suffixed_size(envv);
        if (sz)
            JS_SetMaxStackSize(inst->js_rt, sz);
        inst->js_max_stack_size = sz;
    }

    envv = getenv(PURC_ENVV_JSRT_GC_THRESHOLD);
    if (envv) {
        size_t sz = get_suffixed_size(envv);
        if (sz)
            JS_SetGCThreshold(inst->js_rt, sz);
        inst->js_gc_threshold = sz;
    }

    envv = getenv(PURC_ENVV_JSRT_STRIP_OPTS);
    if (envv) {
        int strip_flags = 0;
        if (strcasestr(envv, "debug"))
            strip_flags = JS_STRIP_DEBUG;
        if (strcasestr(envv, "source"))
            strip_flags = JS_STRIP_SOURCE;
        JS_SetStripInfo(inst->js_rt, strip_flags);
    }

    js_std_set_worker_new_context_func(JS_NewCustomContext);
    js_std_init_handlers(inst->js_rt);

    /* loader for ES6 modules */
    JS_SetModuleLoaderFunc2(inst->js_rt, NULL, js_module_loader,
            js_module_check_attributes, NULL);

    envv = getenv(PURC_ENVV_JSRT_UNHANDLED_REJECTION);
    if (envv && strcasecmp(envv, "dump") == 0) {
        JS_SetHostPromiseRejectionTracker(inst->js_rt,
                js_std_promise_rejection_tracker, NULL);
        inst->js_promise_rejection_tracker = js_std_promise_rejection_tracker;
    }

    return 0;
}

static void _cleanup_instance(struct pcinst *inst)
{
    if (inst->js_rt) {
        js_std_free_handlers(inst->js_rt);
        JS_FreeRuntime(inst->js_rt);
        inst->js_rt = NULL;
    }
}

struct pcmodule _module_quickjs = {
    .id                 = PURC_HAVE_QUICKJS,
    .module_inited      = 0,

    .init_once          = NULL,
    .init_instance      = _init_instance,
    .cleanup_instance   = _cleanup_instance,
};


