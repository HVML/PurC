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
#include "private/ports.h"
#include "private/rwstream.h"
#include "private/ejson.h"
#include "private/html.h"
#include "private/vdom.h"
#include "private/dom.h"
#include "private/dvobjs.h"
#include "private/executor.h"
#include "private/atom-buckets.h"
#include "private/fetcher.h"
#include "private/pcrdr.h"
#include "private/runloop.h"

#include <stdio.h>  // fclose on inst->fp_log
#include <stdlib.h>
#include <string.h>

#include "generic_err_msgs.inc"

#define FETCHER_MAX_CONNS        100
#define FETCHER_CACHE_QUOTA      10240

static struct const_str_atom _except_names[] = {
    { "BadEncoding", 0 },
    { "BadHVMLTag", 0 },
    { "BadHVMLAttrName", 0 },
    { "BadHVMLAttrValue", 0 },
    { "BadHVMLContent", 0 },
    { "BadTargetHTML", 0 },
    { "BadTargetXGML", 0 },
    { "BadTargetXML", 0 },
    { "BadExpression", 0 },
    { "BadExecutor", 0 },
    { "BadName", 0 },
    { "NoData", 0 },
    { "NotIterable", 0 },
    { "BadIndex", 0 },
    { "NoSuchKey", 0 },
    { "DuplicateKey", 0 },
    { "ArgumentMissed", 0 },
    { "WrongDataType", 0 },
    { "InvalidValue", 0 },
    { "MaxIterationCount", 0 },
    { "MaxRecursionDepth", 0 },
    { "Unauthorized", 0 },
    { "Timeout", 0 },
    { "eDOMFailure", 0 },
    { "LostRenderer", 0 },
    { "MemoryFailure", 0 },
    { "InternalFailure", 0 },
    { "ZeroDivision", 0 },
    { "Overflow", 0 },
    { "Underflow", 0 },
    { "InvalidFloat", 0 },
    { "AccessDenied", 0 },
    { "IOFailure", 0 },
    { "TooSmall", 0 },
    { "TooMany", 0 },
    { "TooLong", 0 },
    { "TooLarge", 0 },
    { "NotDesiredEntity", 0 },
    { "InvalidOperand", 0 },
    { "EntityNotFound", 0 },
    { "EntityExists", 0 },
    { "NoStorageSpace", 0 },
    { "BrokenPipe", 0 },
    { "ConnectionAborted", 0 },
    { "ConnectionRefused", 0 },
    { "ConnectionReset", 0 },
    { "NameResolutionFailed", 0 },
    { "RequestFailed", 0 },
    { "OSFailure", 0 },
    { "NotReady", 0 },
    { "NotImplemented", 0 },
};

/* Make sure the number of error messages matches the number of error codes */
#define _COMPILE_TIME_ASSERT(name, x)               \
       typedef int _dummy_ ## name[(x) * 2 - 1]

_COMPILE_TIME_ASSERT(msgs,
        PCA_TABLESIZE(generic_err_msgs) == PURC_ERROR_NR);

_COMPILE_TIME_ASSERT(excepts,
        PCA_TABLESIZE(_except_names) == PURC_EXCEPT_NR);

#undef _COMPILE_TIME_ASSERT

static struct err_msg_seg _generic_err_msgs_seg = {
    { NULL, NULL },
    PURC_ERROR_OK, PURC_ERROR_OK + PCA_TABLESIZE(generic_err_msgs) - 1,
    generic_err_msgs,
};

bool purc_is_except_atom (purc_atom_t atom)
{
    if (atom < _except_names[0].atom ||
            atom > _except_names[PURC_EXCEPT_NR - 1].atom)
        return false;

    return true;
}

purc_atom_t purc_get_except_atom_by_id (int id)
{
    if (id < PURC_EXCEPT_NR)
        return _except_names[id].atom;

    return 0;
}

void pcexcept_init_once(void)
{
    for (size_t n = 0; n < PURC_EXCEPT_NR; n++) {
        _except_names[n].atom =
            purc_atom_from_static_string_ex(ATOM_BUCKET_EXCEPT,
                _except_names[n].str);
    }
}

static unsigned int _modules;

// FIXME: where to put declaration
void pchvml_keywords_init(void);

static void init_modules_once(void)
{
    // TODO: init modules working without instance here.
    pcutils_atom_init_once();
    atexit(pcutils_atom_cleanup_once);

    pcexcept_init_once();
    pchvml_keywords_init();

    pcinst_register_error_message_segment(&_generic_err_msgs_seg);

    pcrwstream_init_once();

    if (_modules & PURC_HAVE_DOM) {
        pcdom_init_once();
    }

    if (_modules & PURC_HAVE_HTML) {
        pchtml_init_once();
    }

    // TODO: init modules working with instance here.
    if (_modules & PURC_HAVE_VARIANT) {
        pcvariant_init_once();

        pcinst_move_buffer_init_once();
        atexit(pcinst_move_buffer_cleanup_once);

        if (_modules & PURC_HAVE_EJSON) {
            pcejson_init_once();
        }

        if (_modules & PURC_HAVE_HVML) {
            pcdvobjs_init_once();
            pcexecutor_init_once();
            pcintr_stack_init_once();
        }

        if (_modules & PURC_HAVE_PCRDR) {
            pcrdr_init_once();
        }
    }
}

#if USE(PTHREADS)
#include <pthread.h>

static pthread_once_t once = PTHREAD_ONCE_INIT;
static inline void init_once(void)
{
    pthread_once(&once, init_modules_once);
}

#else

static inline void init_once(void)
{
    static bool inited = false;
    if (inited)
        return;

    init_modules_once();
    inited = true;
}

#endif /* not USE_PTHREADS */

PURC_DEFINE_THREAD_LOCAL(struct pcinst, inst);

struct pcinst* pcinst_current(void)
{
    if (_modules & PURC_HAVE_VARIANT) {
        struct pcinst* curr_inst;
        curr_inst = PURC_GET_THREAD_LOCAL(inst);

        if (curr_inst == NULL || curr_inst->app_name == NULL) {
            return NULL;
        }

        return curr_inst;
    }

    return NULL;
}

static void cleanup_instance(struct pcinst *curr_inst)
{
    if (curr_inst->local_data_map) {
        pcutils_map_destroy(curr_inst->local_data_map);
        curr_inst->local_data_map = NULL;
    }

    if (curr_inst->app_name) {
        free(curr_inst->app_name);
        curr_inst->app_name = NULL;
    }

    if (curr_inst->runner_name) {
        free(curr_inst->runner_name);
        curr_inst->runner_name = NULL;
    }

    if (curr_inst->fp_log && curr_inst->fp_log != LOG_FILE_SYSLOG) {
        fclose(curr_inst->fp_log);
        curr_inst->fp_log = NULL;
    }
}

static void enable_log_on_demand(void)
{
    const char *env_value;

    env_value = getenv(PURC_ENVV_LOG_ENABLE);
    if (env_value == NULL)
        return;

    bool enable = (*env_value == '1' ||
            strcasecmp(env_value, "true") == 0);
    if (!enable)
        return;

    bool use_syslog = false;
    if ((env_value = getenv(PURC_ENVV_LOG_SYSLOG))) {
        use_syslog = (*env_value == '1' ||
                strcasecmp(env_value, "true") == 0);
    }

    purc_enable_log(true, use_syslog);
}

int purc_init_ex(unsigned int modules,
        const char* app_name, const char* runner_name,
        const purc_instance_extra_info* extra_info)
{
    struct pcinst* curr_inst;
    int ret;

    // FIXME:
    pcrunloop_init_main();

    _modules = modules;
    init_once();

    if (!(modules & PURC_HAVE_VARIANT))
        return PURC_ERROR_OK;

    curr_inst = PURC_GET_THREAD_LOCAL(inst);
    if (curr_inst == NULL)
        return PURC_ERROR_OUT_OF_MEMORY;

    if (curr_inst->app_name)
        return PURC_ERROR_DUPLICATED;

    ret = PURC_ERROR_OK;
    curr_inst->errcode = PURC_ERROR_OK;
    if (app_name)
        curr_inst->app_name = strdup(app_name);
    else {
        char cmdline[128];
        size_t len;
        len = pcutils_get_cmdline_arg(0, cmdline, sizeof(cmdline));
        if (len > 0)
            curr_inst->app_name = strdup(cmdline);
        else
            curr_inst->app_name = strdup("unknown");
    }

    if (runner_name)
        curr_inst->runner_name = strdup(runner_name);
    else
        curr_inst->runner_name = strdup("unknown");

    // endpoint_atom
    if (curr_inst->app_name && curr_inst->runner_name) {
        char endpoint_name [PURC_LEN_ENDPOINT_NAME + 1];
        purc_atom_t endpoint_atom;

        if (purc_assemble_endpoint_name(PCRDR_LOCALHOST,
                curr_inst->app_name, curr_inst->runner_name,
                endpoint_name) == 0) {
            ret = PURC_ERROR_INVALID_VALUE;
            goto failed;
        }

        endpoint_atom = purc_atom_try_string_ex(PURC_ATOM_BUCKET_USER,
                endpoint_name);
        if (curr_inst->endpoint_atom == 0 && endpoint_atom) {
            ret = PURC_ERROR_DUPLICATED;
            goto failed;
        }

        /* check whether app_name or runner_name changed */
        if (curr_inst->endpoint_atom &&
                curr_inst->endpoint_atom != endpoint_atom) {
            ret = PURC_ERROR_INVALID_VALUE;
            goto failed;
        }

        curr_inst->endpoint_atom =
            purc_atom_from_string_ex(PURC_ATOM_BUCKET_USER, endpoint_name);
        assert(curr_inst->endpoint_atom);
    }

    curr_inst->max_embedded_levels = MAX_EMBEDDED_LEVELS;

    enable_log_on_demand();

    // map for local data
    curr_inst->local_data_map =
        pcutils_map_create(copy_key_string,
                free_key_string, NULL, NULL, comp_key_string, false);

    if (curr_inst->endpoint_atom == 0) {
        ret = PURC_ERROR_OUT_OF_MEMORY;
        goto failed;
    }

    /* VW NOTE: eDOM and HTML modules should work without instance
    pcdom_init_instance(curr_inst);
    pchtml_init_instance(curr_inst); */

    if (modules & PURC_HAVE_VARIANT)
        pcvariant_init_instance(curr_inst);
    if (curr_inst->variant_heap == NULL) {
        ret = PURC_ERROR_OUT_OF_MEMORY;
        goto failed;
    }

    // TODO: init XML modules here

    // TODO: init XGML modules here

    if (modules & PURC_HAVE_HVML) {
        pcdvobjs_init_instance(curr_inst);

        curr_inst->executor_heap = NULL;
        pcexecutor_init_instance(curr_inst);
        if (curr_inst->executor_heap == NULL)
            goto failed;

        curr_inst->intr_heap = NULL;
        pcintr_stack_init_instance(curr_inst);
        if (curr_inst->intr_heap == NULL)
            goto failed;
    }

    if (modules & PURC_HAVE_FETCHER) {
        pcfetcher_init(FETCHER_MAX_CONNS, FETCHER_CACHE_QUOTA,
            (modules & PURC_HAVE_FETCHER_R));
    }

    /* connnect to renderer */
    curr_inst->conn_to_rdr = NULL;
    if ((modules & PURC_HAVE_PCRDR)) {
        if ((ret = pcrdr_init_instance(curr_inst, extra_info))) {
            goto failed;
        }
    }

    return PURC_ERROR_OK;

failed:
    purc_cleanup();

    return ret;
}

bool purc_cleanup(void)
{
    if (_modules & PURC_HAVE_VARIANT) {
        struct pcinst* curr_inst;

        curr_inst = PURC_GET_THREAD_LOCAL(inst);
        if (curr_inst == NULL || curr_inst->app_name == NULL)
            return false;

        /* disconnnect from the renderer */
        if (_modules & PURC_HAVE_PCRDR && curr_inst->conn_to_rdr) {
            pcrdr_cleanup_instance(curr_inst);
        }

        // TODO: clean up other fields in reverse order
        if (_modules & PURC_HAVE_HVML) {
            pcintr_stack_cleanup_instance(curr_inst);
            pcexecutor_cleanup_instance(curr_inst);
            pcdvobjs_cleanup_instance(curr_inst);
        }
        pcvariant_cleanup_instance(curr_inst);
        /* VW NOTE: eDOM and HTML modules should work without instance
        pchtml_cleanup_instance(curr_inst);
        pcdom_cleanup_instance(curr_inst); */

        if (_modules & PURC_HAVE_FETCHER) {
            pcfetcher_term();
        }

        cleanup_instance(curr_inst);
    }

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
    pcvarmgr_t varmgr = pcinst_get_variables();
    PC_ASSERT(varmgr);

    return pcvarmgr_add(varmgr, name, variant);
}

pcvarmgr_t pcinst_get_variables(void)
{
    struct pcinst* inst = pcinst_current();
    if (UNLIKELY(inst == NULL))
        return NULL;

    if (UNLIKELY(inst->variables == NULL)) {
        inst->variables = pcvarmgr_create();
    }

    return inst->variables;
}

purc_variant_t purc_get_variable(const char* name)
{
    pcvarmgr_t varmgr = pcinst_get_variables();
    PC_ASSERT(varmgr);

    return pcvarmgr_get(varmgr, name);
}

bool
purc_bind_document_variable(purc_vdom_t vdom, const char* name,
        purc_variant_t variant)
{
    return pcvdom_document_bind_variable(vdom, name, variant);
}

struct pcrdr_conn *
purc_get_conn_to_renderer(void)
{
    struct pcinst* inst = pcinst_current();
    if (inst == NULL)
        return NULL;

    return inst->conn_to_rdr;
}

#if 0
bool purc_unbind_variable(const char* name)
{
    struct pcinst* inst = pcinst_current();
    if (inst == NULL)
        return false;

    return pcvarmgr_remove(inst->variables, name);
}

bool
purc_unbind_document_variable(purc_vdom_t vdom, const char* name)
{
    return pcvdom_document_unbind_variable(vdom, name);
}
#endif
