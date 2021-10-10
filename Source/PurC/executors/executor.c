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
#include "private/instance.h"
#include "private/errors.h"

struct pcexec_record {
    char               *name;
    purc_exec_ops_t     ops;
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


static const char* executor_err_msgs[] = {
    /* PCEXECUTOR_ERROR_NOT_IMPLEMENTED */
    "Executor: NOT IMPLEMENTED",
};

static struct err_msg_seg _executor_err_msgs_seg = {
    { NULL, NULL },
    PURC_ERROR_FIRST_EXECUTOR,
    PURC_ERROR_FIRST_EXECUTOR + PCA_TABLESIZE(executor_err_msgs) - 1,
    executor_err_msgs
};


void pcexecutor_init_once(void)
{
    // register error message
    pcinst_register_error_message_segment(&_executor_err_msgs_seg);

    // initialize others
}

void pcexecutor_init_instance(struct pcinst *inst)
{
    struct pcexecutor_heap *heap = &inst->executor_heap;
    UNUSED_PARAM(heap);
}

void pcexecutor_cleanup_instance(struct pcinst *inst)
{
    struct pcexecutor_heap *heap = &inst->executor_heap;
    UNUSED_PARAM(heap);
    if (heap->executors) {
        pcutils_map_destroy(heap->executors);
        heap->executors = NULL;
    }
}

bool purc_register_executor(const char* name, purc_exec_ops_t ops)
{
    if (!name) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return false;
    }
    
    pcutils_map_entry *entry = NULL;
    struct pcexec_record *record = NULL;
    int r = 0;

    struct pcexecutor_heap *heap;
    heap = &pcinst_current()->executor_heap;
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
    record->ops  = ops;

    r = pcutils_map_insert(heap->executors, name, record);
    if (r) {
        pcinst_set_error(PCEXECUTOR_ERROR_OOM);
        goto failure;
    }

    return true;

failure:
    if (record) {
        free_pcexec_val(record);
    }

    return false;
}

