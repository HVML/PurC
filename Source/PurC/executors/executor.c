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

#include "executor_err_msgs.inc"

struct pcexec_record {
    char                      *name;
    struct purc_exec_ops       ops;
    enum pcexecutor_type       type;
};

static int comp_pcexec_key(const void *key1, const void *key2)
{
    return strcmp(key1, key2);
}

static void free_pcexec_val(void *val)
{
    struct pcexec_record *record = (struct pcexec_record*)val;
    if (record->name) {
        free(record->name);
    }
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


static int _init_once(void)
{
    // register error message
    pcinst_register_error_message_segment(&_executor_err_msgs_seg);

    // initialize others
    return 0;
}

static int _init_instance(struct pcinst *inst,
        const purc_instance_extra_info* extra_info)
{
    UNUSED_PARAM(extra_info);

    struct pcexecutor_heap *heap = inst->executor_heap;
    PC_ASSERT(heap == NULL);
    heap = (struct pcexecutor_heap*)calloc(1, sizeof(*heap));
    if (!heap)
        return PURC_ERROR_OUT_OF_MEMORY;

    inst->executor_heap = heap;

    heap->debug_flex = 0;
    heap->debug_bison = 0;

    pcexec_exe_key_register();
    pcexec_exe_range_register();
    pcexec_exe_filter_register();
    pcexec_exe_char_register();
    pcexec_exe_token_register();
    pcexec_exe_add_register();
    pcexec_exe_sub_register();
    pcexec_exe_mul_register();
    pcexec_exe_div_register();
    pcexec_exe_formula_register();
    pcexec_exe_objformula_register();
    pcexec_exe_sql_register();
    pcexec_exe_travel_register();
    pcexec_exe_func_register();

    return 0;
}

static void _cleanup_instance(struct pcinst *inst)
{
    struct pcexecutor_heap *heap = inst->executor_heap;

    if (!heap)
        return;

    if (heap->executors) {
        pcutils_map_destroy(heap->executors);
        heap->executors = NULL;
    }

    free(heap);
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

int pcexecutor_register(const char* name,
        enum pcexecutor_type type, purc_exec_ops_t ops)
{
    if (!name || !ops) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return -1;
    }

    pcutils_map_entry *entry = NULL;
    struct pcexec_record *record = NULL;
    int r = 0;

    struct pcexecutor_heap *heap;
    heap = pcinst_current()->executor_heap;
    if (!heap->executors) {
        heap->executors = pcutils_map_create(NULL, NULL,
            NULL, free_pcexec_val,
            comp_pcexec_key, false); // FIXME: thread-safe or NOT?
        if (!heap->executors) {
            pcinst_set_error(PCEXECUTOR_ERROR_OOM);
            goto failure;
        }
    }

    entry = pcutils_map_find(heap->executors, name);
    if (entry) {
        pcinst_set_error(PCEXECUTOR_ERROR_ALREAD_EXISTS);
        goto failure;
    }

    record = (struct pcexec_record*)calloc(1, sizeof(*record));
    if (!record) {
        pcinst_set_error(PCEXECUTOR_ERROR_OOM);
        goto failure;
    }

    record->name = strdup(name);
    if (!record->name) {
        pcinst_set_error(PCEXECUTOR_ERROR_OOM);
        goto failure;
    }
    record->ops  = *ops;
    record->type = type;

    r = pcutils_map_find_replace_or_insert(heap->executors, name, record, NULL);
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

bool purc_register_executor(const char* name, purc_exec_ops_t ops)
{
    int r;
    r = pcexecutor_register(name, PCEXECUTOR_INTERNAL, ops);
    return r ? false : true;
}

static inline bool
get_executor(const char* name, enum pcexecutor_type *type, purc_exec_ops_t ops)
{
    pcutils_map_entry *entry = NULL;

    struct pcexecutor_heap *heap;
    heap = pcinst_current()->executor_heap;
    if (!heap) {
        pcinst_set_error(PCEXECUTOR_ERROR_NOT_EXISTS);
        return false;
    }

    entry = pcutils_map_find(heap->executors, name);
    if (!entry) {
        pcinst_set_error(PCEXECUTOR_ERROR_NOT_EXISTS);
        return false;
    }

    struct pcexec_record *record = NULL;
    record = (struct pcexec_record*)entry->val;
    if (ops)
        *ops = record->ops;

    if (type)
        *type = record->type;

    return true;
}

bool purc_get_executor(const char* name, purc_exec_ops_t ops)
{
    int r;
    r = pcexecutor_get_by_rule(name, NULL, ops);
    return r ? false : true;
}

int pcexecutor_get_by_rule(const char *rule,
        enum pcexecutor_type *type, purc_exec_ops_t ops)
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

    bool ok = get_executor(s, type, ops);
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

