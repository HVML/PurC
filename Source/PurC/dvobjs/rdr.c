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

static purc_variant_t
start_session_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    bool ret = false;
    pcrdr_msg *msg = NULL;
    pcrdr_msg *response_msg = NULL;
    purc_variant_t session_data = PURC_VARIANT_INVALID;
    struct pcinst* inst = pcinst_current();
    struct pcrdr_conn *rdr = inst->conn_to_rdr;

    if (!rdr) {
        goto out;
    }

    msg = pcrdr_make_request_message(PCRDR_MSG_TARGET_SESSION, 0,
            PCRDR_OPERATION_STARTSESSION, NULL, NULL,
            PCRDR_MSG_ELEMENT_TYPE_VOID, NULL, NULL,
            PCRDR_MSG_DATA_TYPE_VOID, NULL, 0);
    if (msg == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }

    purc_variant_t vs[10] = { NULL };
    vs[0] = purc_variant_make_string_static("protocolName", false);
    vs[1] = purc_variant_make_string_static(PCRDR_PURCMC_PROTOCOL_NAME, false);
    vs[2] = purc_variant_make_string_static("protocolVersion", false);
    vs[3] = purc_variant_make_ulongint(PCRDR_PURCMC_PROTOCOL_VERSION);
    vs[4] = purc_variant_make_string_static("hostName", false);
    vs[5] = purc_variant_make_string_static(inst->conn_to_rdr->own_host_name,
            false);
    vs[6] = purc_variant_make_string_static("appName", false);
    vs[7] = purc_variant_make_string_static(inst->app_name, false);
    vs[8] = purc_variant_make_string_static("runnerName", false);
    vs[9] = purc_variant_make_string_static(inst->runner_name, false);

    session_data = purc_variant_make_object(0, NULL, NULL);
    if (session_data == PURC_VARIANT_INVALID || vs[9] == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }
    for (int i = 0; i < 5; i++) {
        purc_variant_object_set(session_data, vs[i * 2], vs[i * 2 + 1]);
        purc_variant_unref(vs[i * 2]);
        purc_variant_unref(vs[i * 2 + 1]);
    }

    msg->dataType = PCRDR_MSG_DATA_TYPE_JSON;
    msg->data = session_data;

    int retcode;
    if ((retcode = pcrdr_send_request_and_wait_response(inst->conn_to_rdr,
            msg, PCRDR_TIME_DEF_EXPECTED, &response_msg)) < 0) {
        goto out;
    }
    pcrdr_release_message(msg);
    msg = NULL;

    int ret_code = response_msg->retCode;
    if (ret_code == PCRDR_SC_OK) {
        inst->rdr_caps->session_handle = response_msg->resultValue;
    }

    pcrdr_release_message(response_msg);
    response_msg = NULL;

    if (ret_code != PCRDR_SC_OK) {
        purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
        goto out;
    }

    ret = true;
out:
    return purc_variant_make_boolean(ret);
}

uint64_t pcintr_rdr_create_workspace(struct pcrdr_conn *conn,
        uint64_t session, const char *name, const char *title);

static purc_variant_t
create_workspace_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    bool ret = false;
    const char *name = NULL;
    const char *title = NULL;
    struct pcinst* inst = pcinst_current();
    struct pcrdr_conn *rdr = inst->conn_to_rdr;

    if (!rdr || nr_args < 1 || !purc_variant_is_string(argv[0])) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }
    if (nr_args > 1) {
        if (!purc_variant_is_string(argv[1])) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto out;
        }
        else {
            title = purc_variant_get_string_const(argv[1]);
        }
    }

    name = purc_variant_get_string_const(argv[0]);
    uint64_t handle = pcintr_rdr_create_workspace(rdr,
            inst->rdr_caps->session_handle, name, title);
    if (handle != 0) {
        inst->rdr_caps->workspace_handle = handle;
    }

    ret = true;
out:
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
        { "startSession",       start_session_getter,   NULL },
        { "createWorkspace",    create_workspace_getter,   NULL },
    };

    retv = purc_dvobj_make_from_methods(method, PCA_TABLESIZE(method));
    if (retv == PURC_VARIANT_INVALID) {
        return PURC_VARIANT_INVALID;
    }

    return retv;
}

