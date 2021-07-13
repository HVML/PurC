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

#include <stdlib.h>
#include <string.h>

static const char* generic_err_msgs[] = {
    /* PURC_ERROR_OK (0) */
    "Ok",
    /* PURC_ERROR_BAD_SYSTEM_CALL (1) */
    "Bad system call",
    /* PURC_ERROR_BAD_STDC_CALL (2) */
    "Bad STDC call",
    /* PURC_ERROR_OUT_OF_MEMORY (3) */
    "Out of memory",
    /* PURC_ERROR_INVALID_VALUE (4) */
    "Invalid value",
    /* PURC_ERROR_DUPLICATED (5) */
    "Duplicated",
    /* PURC_ERROR_NOT_IMPLEMENTED (6) */
    "Not implemented",
    /* PURC_ERROR_NO_INSTANCE (7) */
    "No instance",
    /* PURC_ERROR_TOO_LARGE_ENTITY (8) */
    "Tool large entity",
    /* PURC_ERROR_BAD_ENCODING (9) */
    "Bad encoding",
    /* PURC_ERROR_NOT_SUPPORTED (10) */
    "Not supported",
    /* PURC_ERROR_OUTPUT (11) */
    "An output error is encountered",
};

static struct err_msg_seg _generic_err_msgs_seg = {
    { NULL, NULL },
    PURC_ERROR_OK, PURC_ERROR_OK + PCA_TABLESIZE(generic_err_msgs) - 1,
    generic_err_msgs
};

static void init_modules(void)
{
    pcinst_register_error_message_segment(&_generic_err_msgs_seg);

    // TODO: init other modules here.
    pcutils_atom_init_once();
    pcrwstream_init_once();
    pcvariant_init_once();
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

    UNUSED_PARAM(extra_info);

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

    if (curr_inst->app_name == NULL || curr_inst->runner_name == NULL)
        goto failed;

    // TODO: init other fields
    pcvariant_init_instance(curr_inst);
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

    // TODO: clean up other fields
    pcvariant_cleanup_instance(curr_inst);

    cleanup_instance(curr_inst);
    return true;
}

