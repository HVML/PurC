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
};

static struct err_msg_seg _generic_err_msgs_seg = {
    { NULL, NULL },
    PURC_ERROR_OK, PURC_ERROR_OK + PCA_TABLESIZE(generic_err_msgs),
    generic_err_msgs
};

static void init_modules(void)
{
    pcinst_register_error_message_segment(&_generic_err_msgs_seg);

    // TODO: init other modules here.
}

#if USE(PTHREADS)
#include <pthread.h>

static pthread_once_t once = PTHREAD_ONCE_INIT;
static inline void init_once(void) {
    pthread_once(&once, init_modules);
}

#else

static inline void init_once(void) {
    init_modules();
}

#endif

struct pcinst* pcinst_current(void)
{
    return NULL;
}

int purc_init(const char* app_name, const char* runner_name,
        const purc_instance_extra_info* extra_info)
{
    UNUSED_PARAM(app_name);
    UNUSED_PARAM(runner_name);
    UNUSED_PARAM(extra_info);

    init_once();

    return PURC_ERROR_OK;
}

bool purc_cleanup(void)
{
    return true;
}

