/*
 * @file pcrdr.c
 * @date 2022/02/21
 * @brief The implementation of PURCRDR protocol.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
 *
 * Authors:
 *  Vincent Wei (https://github.com/VincentWei), 2021, 2022
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

#include "config.h"

#include "private/instance.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/pcrdr.h"

#include "pcrdr_err_msgs.inc"

/* Make sure the number of error messages matches the number of error codes */
#define _COMPILE_TIME_ASSERT(name, x)               \
       typedef int _dummy_ ## name[(x) * 2 - 1]

_COMPILE_TIME_ASSERT(msgs,
        PCA_TABLESIZE(pcrdr_err_msgs) == PCRDR_ERROR_NR);

#undef _COMPILE_TIME_ASSERT

static struct err_msg_seg _pcrdr_err_msgs_seg = {
    { NULL, NULL },
    PURC_ERROR_FIRST_PCRDR,
    PURC_ERROR_FIRST_PCRDR + PCA_TABLESIZE(pcrdr_err_msgs) - 1,
    pcrdr_err_msgs
};

void pcrdr_init_once(void)
{
    pcinst_register_error_message_segment(&_pcrdr_err_msgs_seg);
}

#define SCHEMA_UNIX_SOCKET  "unix://"

int pcrdr_init_instance(struct pcinst* inst,
        const purc_instance_extra_info *extra_info)
{
    // TODO: only UNIX domain socket supported so far */
    if (strncasecmp (SCHEMA_UNIX_SOCKET, extra_info->renderer_uri,
                sizeof(SCHEMA_UNIX_SOCKET) - 1)) {
        return PURC_ERROR_NOT_SUPPORTED;
    }

    int cnnfd = pcrdr_connect_via_unix_socket(
            extra_info->renderer_uri + sizeof(SCHEMA_UNIX_SOCKET) - 1,
            inst->app_name, inst->runner_name, &inst->conn_to_rdr);

    if (cnnfd < 0)
        return purc_get_last_error();

    // TODO: send the initial request to the renderer

    return PURC_ERROR_OK;
}

void pcrdr_cleanup_instance(struct pcinst* inst)
{
    if (inst->conn_to_rdr)
        pcrdr_disconnect (inst->conn_to_rdr);
}

