/*
 * @file executor.c
 * @author Xu Xiaohong
 * @date 2021/10/10
 * @brief The implementation of public part for executor.
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

#include "private/executor.h"
#include "private/debug.h"
#include "private/errors.h"
#include "private/instance.h"
#include "keywords.h"

#include "purc-utils.h"

#include "pcexe-helper.h"

#include "exe_key.h"
#include "exe_range.h"
#include "exe_filter.h"
#include "exe_char.h"
#include "exe_token.h"
#include "exe_add.h"
#include "exe_sub.h"
#include "exe_mul.h"
#include "exe_div.h"
#include "exe_formula.h"
#include "exe_objformula.h"
#include "exe_sql.h"
#include "exe_travel.h"
#include "exe_func.h"
#include "exe_class.h"

#include "executor_err_msgs.inc"

#include <pthread.h>

static int comp_pcexec_key(const void *key1, const void *key2)
{
    purc_atom_t la = (purc_atom_t)(uint64_t)key1;
    purc_atom_t ra = (purc_atom_t)(uint64_t)key2;
    return la - ra;
}

static void free_pcexec_val(void *val)
{
    pcexec_ops_t record = (pcexec_ops_t)val;
    free(record);
}

/* Make sure the number of error messages matches the number of error codes */
#define _COMPILE_TIME_ASSERT(name, x)               \
       typedef int _dummy_ ## name[(x) * 2 - 1]

_COMPILE_TIME_ASSERT(msgs,
        PCA_TABLESIZE(executor_err_msgs) == PCEXECUTOR_ERROR_NR);

#undef _COMPILE_TIME_ASSERT

static struct err_msg_seg _executor_err_msgs_seg = {
    { NULL, NULL },
    PURC_ERROR_FIRST_EXECUTOR,
    PURC_ERROR_FIRST_EXECUTOR + PCA_TABLESIZE(executor_err_msgs) - 1,
    executor_err_msgs
};


static struct pcutils_map *_executors = NULL;


static void executors_cleanup(void)
{
    if (_executors) {
        pcutils_map_destroy(_executors);
        _executors = NULL;
    }
}

typedef int (*register_f)(void);

static int
_do_registers(void)
{
    register_f regs[] = {
        pcexec_exe_key_register,
        pcexec_exe_range_register,
        pcexec_exe_filter_register,
        pcexec_exe_char_register,
        pcexec_exe_token_register,
        pcexec_exe_add_register,
        pcexec_exe_sub_register,
        pcexec_exe_mul_register,
        pcexec_exe_div_register,
        pcexec_exe_formula_register,
        pcexec_exe_objformula_register,
        pcexec_exe_sql_register,
        pcexec_exe_travel_register,
        pcexec_exe_func_register,
        pcexec_exe_class_register,
    };

    for (size_t i=0; i<PCA_TABLESIZE(regs); ++i) {
        int r;
        r = regs[i]();
        if (r)
            return -1;
    }

    return 0;
}

static int _init_once(void)
{
    // register error message
    pcinst_register_error_message_segment(&_executor_err_msgs_seg);

    atexit(executors_cleanup);

    _executors = pcutils_map_create(NULL, NULL, NULL, free_pcexec_val,
            comp_pcexec_key, false);
    if (!_executors)
        return -1;

    // initialize others
    return 0;
}

static void _init_registers_once(void)
{
    _do_registers();
}

static int _init_instance(struct pcinst *inst,
        const purc_instance_extra_info* extra_info)
{
    UNUSED_PARAM(extra_info);

    if (!_executors) {
        pcinst_set_error(PCEXECUTOR_ERROR_OOM);
        return -1;
    }

    // NOTE: although we can initialize in _init_once for the simplicity,
    // we can NOT use purc_set_error internally because of module-dependance
    // Thus, we prefer lazy-load with pthread_once
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, _init_registers_once);
    if (purc_get_last_error()) {
        pcinst_dump_err_info();
        struct pcdebug_backtrace *bt = inst->bt;
        if (bt)
            pcdebug_backtrace_dump(bt);
        return -1;
    }

    inst->executor_heap = (struct pcexecutor_heap*)calloc(1,
            sizeof(*inst->executor_heap));
    if (!inst->executor_heap) {
        pcinst_set_error(PCEXECUTOR_ERROR_OOM);
        return -1;
    }

    inst->executor_heap->debug_flex = 0;
    inst->executor_heap->debug_bison = 0;

    PC_ASSERT(purc_get_last_error() == 0);
    return 0;
}

static void _cleanup_instance(struct pcinst *inst)
{
    if (!inst->executor_heap)
        return;

    free(inst->executor_heap);
    inst->executor_heap = NULL;
}

struct pcmodule _module_executor = {
    .id              = PURC_HAVE_VARIANT | PURC_HAVE_HVML,
    .module_inited   = 0,

    .init_once                = _init_once,
    .init_instance            = _init_instance,
    .cleanup_instance         = _cleanup_instance,
};

void pcexecutor_set_debug(int debug_flex, int debug_bison)
{
    struct pcexecutor_heap *heap;
    heap = pcinst_current()->executor_heap;

    heap->debug_flex  = debug_flex;
    heap->debug_bison = debug_bison;
}

void pcexecutor_get_debug(int *debug_flex, int *debug_bison)
{
    struct pcexecutor_heap *heap;
    heap = pcinst_current()->executor_heap;

    if (debug_flex)
        *debug_flex  = heap->debug_flex;
    if (debug_bison)
        *debug_bison = heap->debug_bison;
}

int pcexecutor_register(pcexec_ops_t ops)
{
    if (!ops || !ops->atom) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return -1;
    }

    const char *name = purc_atom_to_string(ops->atom);
    PC_ASSERT(name);

    if (ops->atom != PCHVML_KEYWORD_ATOM(HVML, name)) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return -1;
    }

    pcutils_map_entry *entry = NULL;
    pcexec_ops_t record = NULL;
    void *key = (void*)(uint64_t)ops->atom;
    int r = 0;

    entry = pcutils_map_find(_executors, key);
    if (entry) {
        purc_set_error_with_info(PCEXECUTOR_ERROR_ALREAD_EXISTS,
                "executor `%s` already registered", name);
        goto failure;
    }

    record = (struct pcexec_ops*)calloc(1, sizeof(*record));
    if (!record) {
        pcinst_set_error(PCEXECUTOR_ERROR_OOM);
        goto failure;
    }

    *record = *ops;

    r = pcutils_map_replace_or_insert(_executors, key, record, NULL);
    if (r) {
        pcinst_set_error(PCEXECUTOR_ERROR_OOM);
        goto failure;
    }

    return 0;

failure:
    if (record) {
        free_pcexec_val(record);
    }

    return -1;
}

bool purc_register_executor(const char* name, const purc_exec_ops_t ops)
{
    pcexec_ops record = {
        .type         = PCEXEC_TYPE_INTERNAL,
        .internal_ops = ops,
        .atom         = PCHVML_KEYWORD_ATOM(HVML, name),
    };

    if (!record.atom) {
        purc_set_error_with_info(PCEXECUTOR_ERROR_BAD_ARG,
                "unknown name `%s`", name);
        return false;
    }

    int r;
    r = pcexecutor_register(&record);
    return r ? false : true;
}

static inline bool
get_executor(const char* name, pcexec_ops_t ops)
{
    pcutils_map_entry *entry = NULL;

    struct pcexecutor_heap *heap;
    heap = pcinst_current()->executor_heap;
    if (!heap) {
        pcinst_set_error(PCEXECUTOR_ERROR_NOT_EXISTS);
        return false;
    }

    purc_atom_t atom = PCHVML_KEYWORD_ATOM(HVML, name);
    if (atom == 0) {
        purc_set_error_with_info(PCEXECUTOR_ERROR_BAD_ARG,
                "unknown atom: %s", name);
    }

    void *key = (void*)(uint64_t)atom;

    entry = pcutils_map_find(_executors, key);
    if (!entry) {
        pcinst_set_error(PCEXECUTOR_ERROR_NOT_EXISTS);
        return false;
    }

    pcexec_ops_t record = NULL;
    record = (pcexec_ops_t)entry->val;
    if (ops)
        *ops = *record;

    return true;
}

bool purc_get_executor(const char* name, purc_exec_ops_t *ops)
{
    pcexec_ops record = {};

    int r;
    r = pcexecutor_get_by_rule(name, &record);
    if (r)
        return false;

    if (record.type != PCEXEC_TYPE_INTERNAL) {
        purc_set_error_with_info(PCEXECUTOR_ERROR_BAD_ARG,
                "`%s` is not internal executor",
                name);
        return false;
    }

    *ops = record.internal_ops;

    return true;
}

int pcexecutor_get_by_rule(const char *rule, pcexec_ops_t ops)
{
    if (!rule) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return -1;
    }

    const char *h = rule;
    while (*h && purc_isspace(*h))
        ++h;

    if (!*h)
        return -1;

    const char *t = h + 1;
    while (*t && !purc_isspace(*t) && *t != ':')
        ++t;

    char *s = strndup(h, t-h);
    PC_ASSERT(s);

    bool ok = get_executor(s, ops);
    free(s);

    return ok ? 0 : -1;
}

void
pcexecutor_inst_reset(struct purc_exec_inst *inst)
{
    if (inst->selected_keys != PURC_VARIANT_INVALID) {
        purc_variant_unref(inst->selected_keys);
        inst->selected_keys = PURC_VARIANT_INVALID;
    }
    if (inst->err_msg) {
        free(inst->err_msg);
        inst->err_msg = NULL;
    }
}

purc_atom_t
pcexecutor_get_rule_name(const char *rule)
{
    if (!rule) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return 0;
    }

    const char *h = rule;
    while (*h && purc_isspace(*h))
        ++h;

    if (!*h) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return 0;
    }

    const char *t = h + 1;
    while (*t && !purc_isspace(*t) && *t != ':')
        ++t;

    char buf[128];

    int n = snprintf(buf, sizeof(buf), "%.*s", (int)(t-h), h);
    if (n < 0 || (size_t)n >= sizeof(buf)) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return 0;
    }

    purc_atom_t atom = PCHVML_KEYWORD_ATOM(HVML, buf);
    if (atom == 0) {
        purc_set_error_with_info(PCEXECUTOR_ERROR_BAD_ARG,
                "unknown atom: %s", buf);
    }
    return atom;
}


