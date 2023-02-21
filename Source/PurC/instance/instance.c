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

#include "config.h"

#include "purc.h"

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
#include "private/msg-queue.h"
#include "private/runners.h"
#include "purc-runloop.h"

#include "../interpreter/internal.h"

#include <locale.h>
#if USE(PTHREADS)          /* { */
#include <pthread.h>
#endif                     /* } */
#include <stdio.h>  // fclose on inst->fp_log
#include <stdlib.h>
#include <string.h>
#include <time.h>


#include "generic_err_msgs.inc"

#define FETCHER_MAX_CONNS        100
#define FETCHER_CACHE_QUOTA      10240

static struct const_str_atom _except_names[] = {
    { "OK", 0 },
    { "ANY", 0 },
    { "Again", 0 },
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
    { "EntityGone", 0 },
    { "NoStorageSpace", 0 },
    { "BrokenPipe", 0 },
    { "ConnectionAborted", 0 },
    { "ConnectionRefused", 0 },
    { "ConnectionReset", 0 },
    { "NameResolutionFailed", 0 },
    { "RequestFailed", 0 },
    { "SysFault", 0 },
    { "OSFailure", 0 },
    { "NotReady", 0 },
    { "NotImplemented", 0 },
    { "Unsupported", 0 },
    { "Incompleted", 0 },
    { "DuplicateName", 0 },
    { "ChildTerminated", 0 },
    { "Conflict", 0 },
    { "Gone", 0 },
    { "MismatchedVersion", 0 },
    { "NotAcceptable", 0 },
    { "NotAllowed", 0 },
    { "NotFound", 0 },
    { "TooEarly", 0 },
    { "UnavailableLegally", 0 },
    { "UnmetPrecondition", 0 },
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

static int
except_init_once(void)
{
    for (size_t n = 0; n < PURC_EXCEPT_NR; n++) {
        _except_names[n].atom =
            purc_atom_from_static_string_ex(ATOM_BUCKET_EXCEPT,
                _except_names[n].str);

        if (!_except_names[n].atom)
            return -1;
    }

    return 0;
}

struct pcmodule _module_except = {
    .id              = PURC_HAVE_UTILS,
    .module_inited   = 0,

    .init_once       = except_init_once,
    .init_instance   = NULL,
};

#if 0
locale_t __purc_locale_c;
static void free_locale_c(void)
{
    if (__purc_locale_c)
        freelocale(__purc_locale_c);
    __purc_locale_c = NULL;
}
#endif

static int
locale_init_once(void)
{
    tzset();
    setlocale(LC_ALL, "");
    return 0;
}

struct pcmodule _module_locale = {
    .id              = PURC_HAVE_UTILS,
    .module_inited   = 0,

    .init_once       = locale_init_once,
    .init_instance   = NULL,
};

static int
errmsg_init_once(void)
{
    pcinst_register_error_message_segment(&_generic_err_msgs_seg);
    return 0;
}

struct pcmodule _module_errmsg = {
    .id              = PURC_HAVE_UTILS,
    .module_inited   = 0,

    .init_once       = errmsg_init_once,
    .init_instance   = NULL,
};

extern struct pcmodule _module_atom;
extern struct pcmodule _module_keywords;
extern struct pcmodule _module_runloop;
extern struct pcmodule _module_rwstream;
extern struct pcmodule _module_dom;
extern struct pcmodule _module_html;
extern struct pcmodule _module_variant;
extern struct pcmodule _module_mvheap;
extern struct pcmodule _module_mvbuf;
extern struct pcmodule _module_ejson;
extern struct pcmodule _module_dvobjs;
extern struct pcmodule _module_hvml;
extern struct pcmodule _module_executor;
extern struct pcmodule _module_interpreter;
extern struct pcmodule _module_fetcher_local;
extern struct pcmodule _module_fetcher_remote;
extern struct pcmodule _module_renderer;

struct pcmodule* _pc_modules[] = {
    &_module_locale,
    &_module_atom,

    &_module_except,
    &_module_keywords,

    &_module_errmsg,

    &_module_rwstream,
    &_module_dom,
    &_module_html,

    &_module_variant,
    &_module_mvheap,
    &_module_mvbuf,

    &_module_ejson,
    &_module_dvobjs,
    &_module_hvml,

    &_module_runloop,

    &_module_executor,
    &_module_interpreter,

    &_module_fetcher_local,
    &_module_fetcher_remote,

    &_module_renderer,
};

static bool _init_ok = false;
static void _init_once(void)
{
#if 0
     __purc_locale_c = newlocale(LC_ALL_MASK, "C", (locale_t)0);
    atexit(free_locale_c);
#endif

    /* call once initializers of modules */
    for (size_t i = 0; i < PCA_TABLESIZE(_pc_modules); ++i) {
        struct pcmodule *m = _pc_modules[i];
        if (!m->init_once)
            continue;

        if (m->init_once())
            return;

        m->module_inited = 1;
    }

    _init_ok = true;
}

static inline void init_once(void)
{
    static int inited = false;
    if (inited)
        return;

#if USE(PTHREADS)          /* { */
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, _init_once);
#else                      /* }{ */
    _init_once();
#endif                     /* } */

    inited = true;
}

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

static void enable_log_on_demand(void)
{
    const char *env_value;

    env_value = getenv(PURC_ENVV_LOG_ENABLE);
    if (env_value == NULL)
        return;

    bool enable = (*env_value == '1' ||
            pcutils_strcasecmp(env_value, "true") == 0);
    if (!enable)
        return;

    bool use_syslog = false;
    if ((env_value = getenv(PURC_ENVV_LOG_SYSLOG))) {
        use_syslog = (*env_value == '1' ||
                pcutils_strcasecmp(env_value, "true") == 0);
    }

    purc_enable_log(true, use_syslog);
}

static int init_modules(struct pcinst *curr_inst,
        unsigned int modules, const purc_instance_extra_info* extra_info)
{
    curr_inst->modules = modules;
    curr_inst->modules_inited = 0;
    int ret;

    curr_inst->max_conns                  = FETCHER_MAX_CONNS;
    curr_inst->cache_quota                = FETCHER_CACHE_QUOTA;
    if (modules & PURC_HAVE_FETCHER_R) {
        curr_inst->enable_remote_fetcher      =  1;
    }

    // call mdule initializers
    for (size_t i = 0; i < PCA_TABLESIZE(_pc_modules); ++i) {
        struct pcmodule *m = _pc_modules[i];
        if ((m->id & modules) != m->id)
            continue;
        if (m->init_instance == NULL)
            continue;
        if ((ret = m->init_instance(curr_inst, extra_info)) != 0) {
            return ret;
        }

        curr_inst->modules_inited |= m->id;
    }

    return PURC_ERROR_OK;
}

static void cleanup_modules(struct pcinst *curr_inst)
{
    PURC_VARIANT_SAFE_CLEAR(curr_inst->err_exinfo);

    // cleanup modules
    for (size_t i = PCA_TABLESIZE(_pc_modules); i > 0; ) {
        struct pcmodule *m = _pc_modules[--i];
        if (m->cleanup_instance &&
                (m->id & curr_inst->modules_inited) == m->id) {
            m->cleanup_instance(curr_inst);
        }
    }
}

static void cleanup_instance(struct pcinst *curr_inst)
{
    if (curr_inst->local_data_map) {
        pcutils_uomap_destroy(curr_inst->local_data_map);
        curr_inst->local_data_map = NULL;
    }

    if (curr_inst->fp_log && curr_inst->fp_log != LOG_FILE_SYSLOG) {
        fclose(curr_inst->fp_log);
        curr_inst->fp_log = NULL;
    }

    if (curr_inst->bt) {
        pcdebug_backtrace_unref(curr_inst->bt);
        curr_inst->bt = NULL;
    }

    purc_atom_remove_string_ex(PURC_ATOM_BUCKET_DEF,
            curr_inst->endpoint_name);

    if (curr_inst->app_name) {
        free(curr_inst->app_name);
        curr_inst->app_name = NULL;
    }

    if (curr_inst->runner_name) {
        free(curr_inst->runner_name);
        curr_inst->runner_name = NULL;
    }

    curr_inst->modules = 0;
    curr_inst->modules_inited = 0;
}

purc_atom_t
pcinst_endpoint_get(char *endpoint_name, size_t sz,
        const char *app_name, const char *runner_name)
{
    PC_ASSERT(app_name && runner_name);

    if (!purc_is_valid_app_name(app_name) ||
            !purc_is_valid_runner_name(runner_name)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return 0;
    }

    int n;
    n = purc_assemble_endpoint_name_ex(PCRDR_LOCALHOST,
            app_name, runner_name, endpoint_name, sz);
    if (n == 0) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return 0;
    }
    PC_ASSERT(n > 0);
    if ((size_t)n >= sz) {
        purc_set_error(PURC_ERROR_TOO_SMALL_BUFF);
        return 0;
    }

    bool is_mine;
    purc_atom_t atom;
    atom = purc_atom_from_string_ex2(PURC_ATOM_BUCKET_DEF,
        endpoint_name, &is_mine);
    if (!is_mine) {
        purc_set_error(PURC_ERROR_DUPLICATED);
        return 0;
    }

    return atom;
}

int purc_init_ex(unsigned int modules,
        const char* app_name, const char* runner_name,
        const purc_instance_extra_info* extra_info)
{
    if (modules == 0) {
        modules = PURC_MODULE_ALL;
        if (modules == 0)
            return PURC_ERROR_NO_INSTANCE;
    }

    char cmdline[128];
    cmdline[0] = '\0';

    if (!app_name) {
        size_t len;
        len = pcutils_get_cmdline_arg(0, cmdline, sizeof(cmdline));
        if (len > 0)
            app_name = cmdline;
        else
            app_name = "unknown";
    }

    if (!runner_name)
        runner_name = "unknown";

    init_once();
    if (!_init_ok)
        return PURC_ERROR_NO_INSTANCE;

    struct pcinst *curr_inst = PURC_GET_THREAD_LOCAL(inst);
    if (curr_inst == NULL) {
        return PURC_ERROR_OUT_OF_MEMORY;
    }

    if (curr_inst->modules || curr_inst->app_name ||
            curr_inst->runner_name) {
        return PURC_ERROR_DUPLICATED;
    }

    if (!purc_is_valid_app_name(app_name) ||
            !purc_is_valid_runner_name(runner_name)) {
        purc_log_info("invalid app or runner name: %s/%s\n", app_name,
                runner_name);
        return PURC_ERROR_INVALID_VALUE;
    }

    int n;
    n = purc_assemble_endpoint_name_ex(PCRDR_LOCALHOST, app_name, runner_name,
            curr_inst->endpoint_name, sizeof(curr_inst->endpoint_name));
    if (n == 0) {
        return PURC_ERROR_INVALID_VALUE;
    }

    if ((size_t)n >= sizeof(curr_inst->endpoint_name)) {
        return PURC_ERROR_TOO_SMALL_BUFF;
    }

    bool is_mine;
    purc_atom_t atom;
    atom = purc_atom_from_string_ex2(PURC_ATOM_BUCKET_DEF,
        curr_inst->endpoint_name, &is_mine);
    if (!is_mine) {
        return PURC_ERROR_DUPLICATED;
    }

    curr_inst->app_name = strdup(app_name);
    curr_inst->runner_name = strdup(runner_name);
    curr_inst->endpoint_atom = atom;

    enable_log_on_demand();

    // map for local data
    curr_inst->local_data_map =
        pcutils_uomap_create(copy_key_string,
                free_key_string, NULL, NULL, pchash_fnv1a_str_hash,
                comp_key_string, false);

    int ret = init_modules(curr_inst, modules, extra_info);
    if (ret) {
        cleanup_modules(curr_inst);
        cleanup_instance(curr_inst);
        return ret;
    }

    struct pcrdr_conn *conn = purc_get_conn_to_renderer();
    if (conn) {
        pcrdr_conn_set_extra_message_source(conn, pcrun_extra_message_source,
                NULL, NULL);
        pcrdr_conn_set_request_handler(conn, pcrun_request_handler);
    }

    /* it is ready now */
    curr_inst->errcode = PURC_ERROR_OK;

    return PURC_ERROR_OK;
}

bool purc_cleanup(void)
{
    struct pcinst* curr_inst;

    curr_inst = PURC_GET_THREAD_LOCAL(inst);
    if (curr_inst == NULL || curr_inst->app_name == NULL)
        return false;

    // FIXME: shall we clear error here???
    purc_clr_error();

    cleanup_modules(curr_inst);
    cleanup_instance(curr_inst);
    return true;
}

const char *purc_get_endpoint(purc_atom_t *atom)
{
    struct pcinst* curr_inst;

    curr_inst = PURC_GET_THREAD_LOCAL(inst);
    if (curr_inst == NULL || curr_inst->app_name == NULL
            || curr_inst->endpoint_atom == 0)
        return NULL;

    if (atom)
        *atom = curr_inst->endpoint_atom;
    return curr_inst->endpoint_name;
}

bool
purc_set_local_data(const char* data_name, uintptr_t local_data,
        cb_free_local_data cb_free)
{
    struct pcinst* inst = pcinst_current();
    if (inst == NULL)
        return false;

    if (pcutils_uomap_replace_or_insert(inst->local_data_map,
                data_name, (void *)local_data, (free_kv_fn)cb_free)) {
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
        if (pcutils_uomap_erase(inst->local_data_map, (void*)data_name) == 0)
            return 1;
    }
    else {
        ssize_t sz = pcutils_uomap_get_size(inst->local_data_map);
        pcutils_uomap_clear(inst->local_data_map);
        return sz;
    }

    return 0;
}

int
purc_get_local_data(const char* data_name, uintptr_t *local_data,
        cb_free_local_data* cb_free)
{
    struct pcinst* inst;
    const pcutils_uomap_entry* entry = NULL;

    if ((inst = pcinst_current()) == NULL)
        return -1;

    if (data_name == NULL) {
        inst->errcode = PURC_ERROR_INVALID_VALUE;
        return -1;
    }

    if ((entry = pcutils_uomap_find(inst->local_data_map, data_name))) {
        if (local_data)
            *local_data = (uintptr_t)pcutils_uomap_entry_field(entry, val);

        if (cb_free)
            *cb_free = (cb_free_local_data)pcutils_uomap_entry_field(entry,
                    free_kv_alt);

        return 1;
    }

    return 0;
}

bool purc_bind_runner_variable(const char* name, purc_variant_t variant)
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

purc_variant_t pcinst_get_variable(const char* name)
{
    pcvarmgr_t varmgr = pcinst_get_variables();
    PC_ASSERT(varmgr);

    return pcvarmgr_get(varmgr, name);
}

struct pcrdr_conn *
purc_get_conn_to_renderer(void)
{
    struct pcinst* inst = pcinst_current();
    if (inst == NULL)
        return NULL;

    return inst->conn_to_rdr;
}

void pcinst_clear_error(struct pcinst *inst)
{
    if (!inst)
        return;

    inst->errcode = 0;
    PURC_VARIANT_SAFE_CLEAR(inst->err_exinfo);

    if (inst->bt) {
        pcdebug_backtrace_unref(inst->bt);
        inst->bt = NULL;
    }
}

