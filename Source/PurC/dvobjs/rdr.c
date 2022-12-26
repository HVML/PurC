/*
 * @file rdr.c
 * @author Xue Shuming
 * @date 2022/12/23
 * @brief The implementation of $RDR dynamic variant object.
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

#include "private/instance.h"
#include "private/errors.h"
#include "private/vdom.h"
#include "private/dvobjs.h"
#include "private/url.h"
#include "private/channel.h"
#include "private/pcrdr.h"
#include "pcrdr/connect.h"
#include "purc-variant.h"
#include "helper.h"

#include <limits.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

static struct pcrdr_conn *
rdr_conn()
{
    struct pcinst* inst = pcinst_current();
    return inst->conn_to_rdr;
}

static purc_variant_t
type_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);
    struct pcrdr_conn *rdr = rdr_conn();
    if (rdr) {
        return purc_variant_make_ulongint(rdr->type);
    }
    return purc_variant_make_undefined();
}

static purc_variant_t
connect_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);
    pcrdr_msg *msg = NULL;
    bool ret = false;
    const char *s_type;
    const char *s_uri;

    struct pcinst* inst = pcinst_current();
    struct pcrdr_conn *rdr = inst->conn_to_rdr;

    if (nr_args < 2 || !purc_variant_is_string(argv[0])
            || !purc_variant_is_string(argv[1])) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    s_type = purc_variant_get_string_const(argv[0]);
    s_uri = purc_variant_get_string_const(argv[1]);

    if (rdr) {
        pcrdr_disconnect(inst->conn_to_rdr);
        inst->conn_to_rdr = NULL;
        rdr = NULL;
        if (inst->rdr_caps) {
            pcrdr_release_renderer_capabilities(inst->rdr_caps);
            inst->rdr_caps = NULL;
        }
    }

    if (strcmp(s_type, PURC_RDRCOMM_NAME_HEADLESS) == 0) {
        msg = pcrdr_headless_connect(
            s_uri,
            inst->app_name, inst->runner_name, &inst->conn_to_rdr);
    }
    else if (strcmp(s_type, PURC_RDRCOMM_NAME_SOCKET) == 0) {
        msg = pcrdr_socket_connect(s_uri,
            inst->app_name, inst->runner_name, &inst->conn_to_rdr);
    }
    else if (strcmp(s_type, PURC_RDRCOMM_NAME_THREAD) == 0) {
        msg = pcrdr_thread_connect(s_uri,
            inst->app_name, inst->runner_name, &inst->conn_to_rdr);
    }
    else {
        // TODO: other protocol
        purc_set_error(PURC_ERROR_NOT_SUPPORTED);
        goto out;
    }

    if (msg == NULL) {
        inst->conn_to_rdr = NULL;
        goto out;
    }

    if (msg->type == PCRDR_MSG_TYPE_RESPONSE && msg->retCode == PCRDR_SC_OK) {
        inst->rdr_caps =
            pcrdr_parse_renderer_capabilities(
                    purc_variant_get_string_const(msg->data));
        if (inst->rdr_caps == NULL) {
            goto out;
        }
    }
    pcrdr_release_message(msg);

    ret = true;
out:
    return purc_variant_make_boolean(ret);
}

static purc_variant_t
disconnect_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    bool ret = true;
    struct pcinst* inst = pcinst_current();
    struct pcrdr_conn *rdr = inst->conn_to_rdr;

    if (rdr) {
        pcrdr_disconnect(inst->conn_to_rdr);
        inst->conn_to_rdr = NULL;

        if (inst->rdr_caps) {
            pcrdr_release_renderer_capabilities(inst->rdr_caps);
            inst->rdr_caps = NULL;
        }
    }

    return purc_variant_make_boolean(ret);
}

purc_variant_t
purc_dvobj_rdr_new(void)
{
    purc_variant_t retv = PURC_VARIANT_INVALID;

    static struct purc_dvobj_method method [] = {
        { "type",               type_getter,            NULL },
        { "connect",            connect_getter,         NULL },
        { "disconnect",         disconnect_getter,      NULL },
    };

    retv = purc_dvobj_make_from_methods(method, PCA_TABLESIZE(method));
    if (retv == PURC_VARIANT_INVALID) {
        return PURC_VARIANT_INVALID;
    }

    return retv;
}

