/*
 * @file instance.h
 * @author Vincent Wei (https://github.com/VincentWei)
 * @date 2021/07/05
 * @brief The structures for PurC instance.
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

#ifndef PURC_PRIVATE_INSTANCE_H
#define PURC_PRIVATE_INSTANCE_H

#include "purc.h"

#include "config.h"

#include "private/variant.h"
#include "private/map.h"
#include "private/executor.h"
#include "private/interpreter.h"

#include <stdio.h>

struct pcinst;
typedef struct pcinst pcinst;
typedef struct pcinst *pcinst_t;

struct pcmodule;
typedef struct pcmodule pcmodule;
typedef struct pcmodule *pcmodule_t;

struct pcinst_msg_queue;

typedef int (*module_init_once_f)(void);
typedef int (*module_init_instance_f)(struct pcinst *curr_inst,
        const purc_instance_extra_info* extra_info);
typedef void (*module_cleanup_instance_f)(struct pcinst *curr_inst);

struct pcmodule {
    // PURC_HAVE_XXXX if !always
    unsigned int               id;
    unsigned int               module_inited;

    module_init_once_f         init_once;
    module_init_instance_f     init_instance;
    module_cleanup_instance_f  cleanup_instance;
};

struct pcinst {
    int                     errcode;
    purc_atom_t             error_except;
    purc_variant_t          err_exinfo;
    struct pcvdom_element  *err_element;

    unsigned int            modules;
    unsigned int            modules_inited;
    unsigned int            log_levels;

    // flags go here
    unsigned int            enable_remote_fetcher:1;
    unsigned int            is_instmgr:1;
    unsigned int            allow_switching_rdr:1;

    char                   *app_name;
    char                   *runner_name;
    char                    endpoint_name[PURC_LEN_ENDPOINT_NAME + 1];
    purc_atom_t             endpoint_atom;

    // fetcher related
    size_t                  max_conns;
    size_t                  cache_quota;

#define LOG_FILE_SYSLOG     ((FILE *)-1)
    /* the FILE object for logging (-1: use syslog; NULL: disabled) */
    FILE                   *fp_log;

    /* data bound to the current session, e.g, the statbuf of the random
       number generator (using unordered map) */
    pcutils_uomap          *local_data_map;

    struct pcvariant_heap  *variant_heap;
    struct pcvariant_heap  *org_vrt_heap;

    struct pcvarmgr        *variables;
    // for loaded dynamic variants
    pcutils_array_t        *dvobjs;

    struct pcrdr_conn      *conn_to_rdr;
    struct renderer_capabilities *rdr_caps;
    struct pcrdr_conn      *conn_to_rdr_origin;

    struct pcexecutor_heap *executor_heap;
    struct pcintr_heap     *intr_heap;
    purc_runloop_t          running_loop;

    /* FIXME: enable the fields ONLY when NDEBUG is undefined */
    struct pcdebug_backtrace  *bt;

    /* Since 0.9.17 */
    purc_variant_t         app_manifest;
};

PCA_EXTERN_C_BEGIN

/* gets the current instance */
struct pcinst* pcinst_current(void) WTF_INTERNAL;
pcvarmgr_t pcinst_get_variables(void) WTF_INTERNAL;
purc_variant_t pcinst_get_variable(const char* name) WTF_INTERNAL;

static inline purc_variant_t
pcinst_get_session_variables(const char* name)
{
    return pcinst_get_variable(name);
}

struct pcrdr_msg *pcinst_get_message(void) WTF_INTERNAL;
void pcinst_put_message(struct pcrdr_msg *msg) WTF_INTERNAL;

int
pcinst_broadcast_event(pcrdr_msg_event_reduce_opt reduce_op,
        purc_variant_t source_uri, purc_variant_t observed,
        const char *event_type, const char *event_sub_type,
        purc_variant_t data) WTF_INTERNAL;

void pcinst_clear_error(struct pcinst *inst) WTF_INTERNAL;

purc_atom_t
pcinst_endpoint_get(char *endpoint_name, size_t sz,
        const char *app_name, const char *runner_name) WTF_INTERNAL;

void
pcinst_dump_err_except_info(purc_variant_t err_except_info) WTF_INTERNAL;

void
pcinst_dump_err_info(void) WTF_INTERNAL;

purc_variant_t
pcinst_load_app_manifest(const char *app_name) WTF_INTERNAL;

PCA_EXTERN_C_END

#endif /* not defined PURC_PRIVATE_INSTANCE_H */

