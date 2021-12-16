/*
 * @file instance.c
 * @author Vincent Wei (https://github.com/VincentWei)
 * @date 2021/07/02
 * @brief The instance of PurC.
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

#include "purc.h"

#include "config.h"

#include "private/instance.h"
#include "private/errors.h"
#include "private/tls.h"
#include "private/utils.h"
#include "private/rwstream.h"
#include "private/ejson.h"
#include "private/html.h"
#include "private/vdom.h"
#include "private/edom.h"
#include "private/dvobjs.h"
#include "private/executor.h"

#include <stdlib.h>
#include <string.h>

static struct err_msg_info generic_err_msgs[] = {
    /* PURC_ERROR_OK */
    { "Ok", NULL, PURC_EXCEPT_FLAGS_NULL, 0},
    /* PURC_ERROR_BAD_SYSTEM_CALL */
    { "Bad system call", PURC_EXCEPT_OS_ERROR, PURC_EXCEPT_FLAGS_NULL, 0},
    /* PURC_ERROR_BAD_STDC_CALL */
    { "Bad STDC call", PURC_EXCEPT_OS_ERROR, PURC_EXCEPT_FLAGS_NULL, 0},
    /* PURC_ERROR_OUT_OF_MEMORY */
    { "Out of memory", PURC_EXCEPT_MEMORY_ERROR, PURC_EXCEPT_FLAGS_NULL, 0},
    /* PURC_ERROR_INVALID_VALUE */
    { "Invalid value", PURC_EXCEPT_BAD_VALUE, PURC_EXCEPT_FLAGS_NULL, 0},
    /* PURC_ERROR_DUPLICATED */
    { "Duplicated", PURC_EXCEPT_BAD_VALUE, PURC_EXCEPT_FLAGS_NULL, 0},
    /* PURC_ERROR_NOT_IMPLEMENTED */
    { "Not implemented", PURC_EXCEPT_NOT_IMPLEMENTED, PURC_EXCEPT_FLAGS_NULL, 0},
    /* PURC_ERROR_NO_INSTANCE */
    { "No instance", PURC_EXCEPT_NOT_READY, PURC_EXCEPT_FLAGS_NULL, 0},
    /* PURC_ERROR_TOO_LARGE_ENTITY */
    { "Tool large entity", PURC_EXCEPT_TOO_LONG, PURC_EXCEPT_FLAGS_NULL, 0},
    /* PURC_ERROR_BAD_ENCODING */
    { "Bad encoding", PURC_EXCEPT_BAD_ENCODING, PURC_EXCEPT_FLAGS_NULL, 0},
    /* PURC_ERROR_NOT_SUPPORTED */
    { "Not supported", PURC_EXCEPT_NOT_IMPLEMENTED, PURC_EXCEPT_FLAGS_NULL, 0},
    /* PURC_ERROR_OUTPUT */
    {
        "An output error is encountered",
        PURC_EXCEPT_IO_ERROR,
        PURC_EXCEPT_FLAGS_NULL,
        0
    },
    /* PURC_ERROR_TOO_SMALL_BUFF */
    { "Too small buffer", PURC_EXCEPT_BUFFER_ERROR, PURC_EXCEPT_FLAGS_NULL, 0},
    /* PURC_ERROR_NULL_OBJECT */
    { "Null object", PURC_EXCEPT_BAD_VALUE, PURC_EXCEPT_FLAGS_NULL, 0},
    /* PURC_ERROR_TOO_SMALL_SIZE */
    { "Too small size", PURC_EXCEPT_BAD_VALUE, PURC_EXCEPT_FLAGS_NULL, 0},
    /* PURC_ERROR_INCOMPLETE_OBJECT */
    { "Incomplete object", PURC_EXCEPT_BAD_VALUE, PURC_EXCEPT_FLAGS_NULL, 0},
    /* PURC_ERROR_NO_FREE_SLOT */
    { "No free slot", PURC_EXCEPT_BUFFER_ERROR, PURC_EXCEPT_FLAGS_NULL, 0},
    /* PURC_ERROR_NOT_EXISTS */
    { "Does not exist", PURC_EXCEPT_ENTITY_EXISTS, PURC_EXCEPT_FLAGS_NULL, 0},
    /* PURC_ERROR_WRONG_ARGS */
    { "Wrong arguments", PURC_EXCEPT_BAD_VALUE, PURC_EXCEPT_FLAGS_NULL, 0},
    /* PURC_ERROR_WRONG_STAGE */
    { "Wrong stage", PURC_EXCEPT_BAD_VALUE, PURC_EXCEPT_FLAGS_NULL, 0},
    /* PURC_ERROR_UNEXPECTED_RESULT */
    { "Unexpected result", PURC_EXCEPT_BAD_VALUE, PURC_EXCEPT_FLAGS_NULL, 0},
    /* PURC_ERROR_UNEXPECTED_DATA */
    { "Unexpected data", PURC_EXCEPT_BAD_VALUE, PURC_EXCEPT_FLAGS_NULL, 0},
    /* PURC_ERROR_OVERFLOW */
    { "Overflow", PURC_EXCEPT_OVERFLOW, PURC_EXCEPT_FLAGS_NULL, 0},
    /* PURC_ERROR_UNDERFLOW */
    { "Underflow", PURC_EXCEPT_OVERFLOW, PURC_EXCEPT_FLAGS_NULL, 0},
    /* PURC_ERROR_DIVBYZERO*/
    { "Divide by zero", PURC_EXCEPT_ZERO_DIVISION, PURC_EXCEPT_FLAGS_NULL, 0},
    /* PURC_ERROR_UNKNOWN */
    { "Unknown", PURC_EXCEPT_OS_ERROR, PURC_EXCEPT_FLAGS_NULL, 0},
    /* PURC_ERROR_BAD_LOCALE_CATEGORY */
    { "Bad locale category", PURC_EXCEPT_BAD_VALUE, PURC_EXCEPT_FLAGS_NULL, 0},
};

/* Make sure the number of error messages matches the number of error codes */
#define _COMPILE_TIME_ASSERT(name, x)               \
       typedef int _dummy_ ## name[(x) * 2 - 1]

_COMPILE_TIME_ASSERT(msgs,
        PCA_TABLESIZE(generic_err_msgs) == PURC_ERROR_NR);

#undef _COMPILE_TIME_ASSERT

static struct err_msg_seg _generic_err_msgs_seg = {
    { NULL, NULL },
    PURC_ERROR_OK, PURC_ERROR_OK + PCA_TABLESIZE(generic_err_msgs) - 1,
    generic_err_msgs,
};

static void init_modules(void)
{
    pcutils_atom_init_once();

    pcinst_register_error_message_segment(&_generic_err_msgs_seg);

    // TODO: init other modules here.
    pcrwstream_init_once();
    pcvariant_init_once();
    pcdvobjs_init_once();
    pcejson_init_once();
    pchtml_init_once();
    pcedom_init_once();
    pcexecutor_init_once();
    pcintr_stack_init_once();
}

#if USE(PTHREADS)
#include <pthread.h>

static pthread_once_t once = PTHREAD_ONCE_INIT;
static inline void init_once(void)
{
    pthread_once(&once, init_modules);
}

#else

static inline void init_once(void)
{
    static bool inited = false;
    if (inited)
        return;

    init_modules();
    inited = true;
}

#endif /* not USE_PTHREADS */

PURC_DEFINE_THREAD_LOCAL(struct pcinst, inst);

struct pcinst* pcinst_current(void)
{
    struct pcinst* curr_inst;
    curr_inst = PURC_GET_THREAD_LOCAL(inst);

    if (curr_inst == NULL || curr_inst->app_name == NULL) {
        return NULL;
    }

    return curr_inst;
}

static void cleanup_instance (struct pcinst *curr_inst)
{
    if (curr_inst->local_data_map) {
        pcutils_map_destroy(curr_inst->local_data_map);
        curr_inst->local_data_map = NULL;
    }

    if (curr_inst->variables) {
        pcvarmgr_list_destroy(curr_inst->variables);
        curr_inst->variables = NULL;
    }

    if (curr_inst->app_name) {
        free (curr_inst->app_name);
        curr_inst->app_name = NULL;
    }

    if (curr_inst->runner_name) {
        free (curr_inst->runner_name);
        curr_inst->runner_name = NULL;
    }
}

int purc_init(const char* app_name, const char* runner_name,
        const purc_instance_extra_info* extra_info)
{
    struct pcinst* curr_inst;

    init_once();

    curr_inst = PURC_GET_THREAD_LOCAL(inst);
    if (curr_inst == NULL)
        return PURC_ERROR_OUT_OF_MEMORY;

    if (curr_inst->app_name)
        return PURC_ERROR_DUPLICATED;

    curr_inst->errcode = PURC_ERROR_OK;
    if (app_name)
        curr_inst->app_name = strdup(app_name);
    else {
        char cmdline[128];
        size_t len;
        len = pcutils_get_cmdline_arg (0, cmdline, sizeof(cmdline));
        if (len > 0)
            curr_inst->app_name = strdup(cmdline);
        else
            curr_inst->app_name = strdup("unknown");
    }

    if (runner_name)
        curr_inst->runner_name = strdup(runner_name);
    else
        curr_inst->runner_name = strdup("unknown");

    // map for local data
    curr_inst->local_data_map =
        pcutils_map_create (copy_key_string,
                free_key_string, NULL, NULL, comp_key_string, false);


    curr_inst->variables = pcvarmgr_list_create();

    if (curr_inst->app_name == NULL ||
            curr_inst->runner_name == NULL ||
            curr_inst->local_data_map == NULL ||
            curr_inst->variables == NULL)
        goto failed;

    // TODO: init other fields
    pcvariant_init_instance(curr_inst);
    pcdvobjs_init_instance(curr_inst);
    pchtml_init_instance(curr_inst);
    pcedom_init_instance(curr_inst);
    pcexecutor_init_instance(curr_inst);
    pcintr_stack_init_instance(curr_inst);

    /* TODO: connnect to renderer */
    UNUSED_PARAM(extra_info);
    return PURC_ERROR_OK;

failed:
    cleanup_instance(curr_inst);

    return PURC_ERROR_OUT_OF_MEMORY;
}

bool purc_cleanup(void)
{
    struct pcinst* curr_inst;

    curr_inst = PURC_GET_THREAD_LOCAL(inst);
    if (curr_inst == NULL || curr_inst->app_name == NULL)
        return false;

    // TODO: clean up other fields in reverse order
    pcexecutor_cleanup_instance(curr_inst);
    pcedom_cleanup_instance(curr_inst);
    pchtml_cleanup_instance(curr_inst);
    pcdvobjs_cleanup_instance(curr_inst);
    pcvariant_cleanup_instance(curr_inst);

    cleanup_instance(curr_inst);
    return true;
}

bool
purc_set_local_data(const char* data_name, uintptr_t local_data,
        cb_free_local_data cb_free)
{
    struct pcinst* inst = pcinst_current();
    if (inst == NULL)
        return false;

    if (pcutils_map_find_replace_or_insert(inst->local_data_map,
                data_name, (void *)local_data, (free_val_fn)cb_free)) {
        inst->errcode = PURC_ERROR_OUT_OF_MEMORY;
        return false;
    }

    return true;
}

ssize_t
purc_remove_local_data(const char* data_name)
{
    struct pcinst* inst = pcinst_current();
    if (inst == NULL)
        return -1;

    if (data_name) {
        if (pcutils_map_erase (inst->local_data_map, (void*)data_name))
            return 1;
    }
    else {
        ssize_t sz = pcutils_map_get_size(inst->local_data_map);
        pcutils_map_clear(inst->local_data_map);
        return sz;
    }

    return 0;
}

int
purc_get_local_data(const char* data_name, uintptr_t *local_data,
        cb_free_local_data* cb_free)
{
    struct pcinst* inst;
    const pcutils_map_entry* entry = NULL;

    if ((inst = pcinst_current()) == NULL)
        return -1;

    if (data_name == NULL) {
        inst->errcode = PURC_ERROR_INVALID_VALUE;
        return -1;
    }

    if ((entry = pcutils_map_find(inst->local_data_map, data_name))) {
        if (local_data)
            *local_data = (uintptr_t)entry->val;

        if (cb_free)
            *cb_free = (cb_free_local_data)entry->free_val_alt;

        return 1;
    }

    return 0;
}

bool purc_bind_variable(const char* name, purc_variant_t variant)
{
    struct pcinst* inst = pcinst_current();
    if (inst == NULL)
        return false;

    return pcvarmgr_list_add(inst->variables, name, variant);
}

#if 0
bool purc_unbind_variable(const char* name)
{
    struct pcinst* inst = pcinst_current();
    if (inst == NULL)
        return false;

    return pcvarmgr_list_remove(inst->variables, name);
}
#endif

bool
purc_bind_document_variable(purc_vdom_t vdom, const char* name,
        purc_variant_t variant)
{
    if (vdom == NULL) {
        return false;
    }
    return pcvdom_document_bind_variable(vdom->document, name, variant);
}

#if 0
bool
purc_unbind_document_variable(purc_vdom_t vdom, const char* name)
{
    if (vdom == NULL) {
        return false;
    }
    return pcvdom_document_unbind_variable(vdom->document, name);
}
#endif
