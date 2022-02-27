/*
 * @file pcrdr.c
 * @date 2022/02/21
 * @brief The initializer of the PCRDR module.
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
    pcrdr_msg *msg = NULL;

    // TODO: only PurCMC protocol and UNIX domain socket supported so far */
    if (extra_info->renderer_prot != PURC_RDRPROT_PURCMC ||
            strncasecmp (SCHEMA_UNIX_SOCKET, extra_info->renderer_uri,
                sizeof(SCHEMA_UNIX_SOCKET) - 1)) {
        return PURC_ERROR_NOT_SUPPORTED;
    }

    if (pcrdr_purcmc_connect_via_unix_socket(
            extra_info->renderer_uri + sizeof(SCHEMA_UNIX_SOCKET) - 1,
            inst->app_name, inst->runner_name, &inst->conn_to_rdr) < 0)
        goto failed;

    /* read the initial response from the server */
    char buff[PCRDR_DEF_PACKET_BUFF_SIZE];
    size_t len = sizeof(buff);

    if (pcrdr_purcmc_read_packet(inst->conn_to_rdr, buff, &len) < 0)
        goto failed;

    if (pcrdr_parse_packet(buff, len, &msg) < 0)
        goto failed;

    if (msg->type == PCRDR_MSG_TYPE_RESPONSE && msg->retCode == PCRDR_SC_OK) {
        inst->rdr_caps =
            pcrdr_parse_renderer_capabilities(
                    purc_variant_get_string_const(msg->data));
        if (inst->rdr_caps == NULL)
            goto failed;
    }

    pcrdr_release_message(msg);
    return PURC_ERROR_OK;

failed:
    if (msg)
        pcrdr_release_message(msg);

    if (inst->conn_to_rdr) {
        pcrdr_disconnect(inst->conn_to_rdr);
        inst->conn_to_rdr = NULL;
    }

    return purc_get_last_error();
}

void pcrdr_cleanup_instance(struct pcinst* inst)
{
    if (inst->rdr_caps) {
        pcrdr_release_renderer_capabilities(inst->rdr_caps);
    }

    if (inst->conn_to_rdr)
        pcrdr_disconnect (inst->conn_to_rdr);
}

