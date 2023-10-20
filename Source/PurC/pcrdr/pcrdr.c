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

#include "connect.h"

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

static struct pcrdr_opatom {
    const char *op;
    purc_atom_t atom;
} pcrdr_opatoms[] = {
    { PCRDR_OPERATION_STARTSESSION,         0 }, // "startSession"
    { PCRDR_OPERATION_ENDSESSION,           0 }, // "endSession"
    { PCRDR_OPERATION_AUTHENTICATE,         0 }, // "authenticate"
    { PCRDR_OPERATION_CREATEWORKSPACE,      0 }, // "createWorkspace"
    { PCRDR_OPERATION_UPDATEWORKSPACE,      0 }, // "updateWorkspace"
    { PCRDR_OPERATION_DESTROYWORKSPACE,     0 }, // "destroyWorkspace"
    { PCRDR_OPERATION_CREATEPLAINWINDOW,    0 }, // "createPlainWindow"
    { PCRDR_OPERATION_UPDATEPLAINWINDOW,    0 }, // "updatePlainWindow"
    { PCRDR_OPERATION_DESTROYPLAINWINDOW,   0 }, // "destroyPlainWindow"
    { PCRDR_OPERATION_SETPAGEGROUPS,        0 }, // "setPageGroups"
    { PCRDR_OPERATION_ADDPAGEGROUPS,        0 }, // "addPageGroups"
    { PCRDR_OPERATION_REMOVEPAGEGROUP,      0 }, // "removePageGroup"
    { PCRDR_OPERATION_CREATEWIDGET,         0 }, // "createWidget"
    { PCRDR_OPERATION_UPDATEWIDGET,         0 }, // "updateWidget"
    { PCRDR_OPERATION_DESTROYWIDGET,        0 }, // "destroyWidget"
    { PCRDR_OPERATION_LOAD,                 0 }, // "load"
    { PCRDR_OPERATION_WRITEBEGIN,           0 }, // "writeBegin"
    { PCRDR_OPERATION_WRITEMORE,            0 }, // "writeMore"
    { PCRDR_OPERATION_WRITEEND,             0 }, // "writeEnd"
    { PCRDR_OPERATION_REGISTER,             0 }, // "register"
    { PCRDR_OPERATION_REVOKE,               0 }, // "revoke"
    { PCRDR_OPERATION_APPEND,               0 }, // "append"
    { PCRDR_OPERATION_PREPEND,              0 }, // "prepend"
    { PCRDR_OPERATION_INSERTBEFORE,         0 }, // "insertBefore"
    { PCRDR_OPERATION_INSERTAFTER,          0 }, // "insertAfter"
    { PCRDR_OPERATION_DISPLACE,             0 }, // "displace"
    { PCRDR_OPERATION_UPDATE,               0 }, // "update"
    { PCRDR_OPERATION_ERASE,                0 }, // "erase"
    { PCRDR_OPERATION_CLEAR,                0 }, // "clear"
    { PCRDR_OPERATION_CALLMETHOD,           0 }, // "callMethod"
    { PCRDR_OPERATION_GETPROPERTY,          0 }, // "getProperty"
    { PCRDR_OPERATION_SETPROPERTY,          0 }, // "setProperty"
};

/* make sure the number of operations matches the enumulators */
#define _COMPILE_TIME_ASSERT(name, x)           \
       typedef int _dummy_ ## name[(x) * 2 - 1]
_COMPILE_TIME_ASSERT(ops,
        PCA_TABLESIZE(pcrdr_opatoms) == PCRDR_NR_OPERATIONS);
#undef _COMPILE_TIME_ASSERT

const char *pcrdr_operation_from_atom(purc_atom_t op_atom, unsigned int *id)
{
    if (op_atom >= pcrdr_opatoms[0].atom &&
            op_atom <= pcrdr_opatoms[PCRDR_K_OPERATION_LAST].atom) {
        *id = op_atom - pcrdr_opatoms[0].atom;
        return pcrdr_opatoms[*id].op;
    }

    return NULL;
}

purc_atom_t pcrdr_try_operation_atom(const char *op)
{
    return purc_atom_try_string_ex(ATOM_BUCKET_RDROP, op);
}

#if 0   /* deprecated code */
static const char *comm_names[] = {
    PURC_RDRCOMM_NAME_HEADLESS,
    PURC_RDRCOMM_NAME_THREAD,
    PURC_RDRCOMM_NAME_SOCKET,
    PURC_RDRCOMM_NAME_HBDBUS,
};

static const int comm_vers[] = {
    PURC_RDRCOMM_VERSION_HEADLESS,
    PURC_RDRCOMM_VERSION_THREAD,
    PURC_RDRCOMM_VERSION_SOCKET,
    PURC_RDRCOMM_VERSION_HBDBUS,
};
#endif

static int _init_once(void)
{
    pcinst_register_error_message_segment(&_pcrdr_err_msgs_seg);

    // put the operations into ATOM_BUCKET_RDROP bucket
    for (size_t i = 0; i < PCA_TABLESIZE(pcrdr_opatoms); i++) {
        pcrdr_opatoms[i].atom =
            purc_atom_from_static_string_ex(ATOM_BUCKET_RDROP,
                    pcrdr_opatoms[i].op);

        if (!pcrdr_opatoms[i].atom)
            return -1;
    }

    return 0;
}

static purc_variant_t make_signature(struct pcinst *inst)
{
    unsigned char *sig = NULL;
    unsigned int sig_len;
    char *enc_sig = NULL;
    unsigned int enc_sig_len;

    int err_code = pcutils_sign_data(inst->app_name,
            (const unsigned char *)inst->rdr_caps->challenge_code,
            strlen(inst->rdr_caps->challenge_code),
            &sig, &sig_len);
    if (err_code) {
        purc_set_error(err_code);
        goto failed;
    }

    enc_sig_len = pcutils_b64_encoded_length(sig_len);
    enc_sig = malloc(enc_sig_len);
    if (enc_sig == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    // When encode the signature in base64 or exadecimal notation,
    // there will be no any '"' and '\' charecters.
    pcutils_b64_encode(sig, sig_len, enc_sig, enc_sig_len);
    free(sig);

    return purc_variant_make_string_reuse_buff(enc_sig, enc_sig_len, false);

failed:
    if (sig)
        free(sig);
    return PURC_VARIANT_INVALID;
}

static int authenticate_app(struct pcinst *inst)
{
    if (inst->app_manifest == PURC_VARIANT_INVALID) {
        inst->app_manifest = pcinst_load_app_manifest(inst->app_name);
        if (inst->app_manifest == NULL) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }
    }

    pcrdr_msg *msg = NULL, *response_msg = NULL;
    /* send authenticate request and wait for the response */
    msg = pcrdr_make_request_message(PCRDR_MSG_TARGET_SESSION, 0,
            PCRDR_OPERATION_AUTHENTICATE, NULL, NULL,
            PCRDR_MSG_ELEMENT_TYPE_VOID, NULL, NULL,
            PCRDR_MSG_DATA_TYPE_VOID, NULL, 0);
    if (msg == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    int n = 0;
    purc_variant_t vs[16] = { NULL };
    vs[n++] = purc_variant_make_string_static("hostName", false);
    vs[n++] = purc_variant_make_string_static(inst->conn_to_rdr->own_host_name,
            false);
    vs[n++] = purc_variant_make_string_static("appName", false);
    vs[n++] = purc_variant_make_string_static(inst->app_name, false);

    vs[n++] = purc_variant_make_string_static("appLabel", false);
    vs[n++] = purc_variant_ref(purc_get_app_label(inst->rdr_caps->locale));
    vs[n++] = purc_variant_make_string_static("appDesc", false);
    vs[n++] = purc_variant_ref(purc_get_app_description(inst->rdr_caps->locale));
#if 0
    /* ICON too large (PCRDR_MAX_INMEM_PAYLOAD_SIZE) */
    vs[n++] = purc_variant_make_string_static("appIcon", false);
    vs[n++] = purc_get_app_icon_content(inst->rdr_caps->display_density,
            inst->rdr_caps->locale);
#endif

    vs[n++] = purc_variant_make_string_static("signature", false);
    vs[n++] = make_signature(inst);
    vs[n++] = purc_variant_make_string_static("encodedIn", false);
    vs[n++] = purc_variant_make_string_static("base64", false);
    vs[n++] = purc_variant_make_string_static("timeoutSeconds", false);
    vs[n++] = purc_variant_make_ulongint(PCRDR_TIME_AUTH_EXPECTED);

    purc_variant_t auth_data = purc_variant_make_object_0();
    if (auth_data == PURC_VARIANT_INVALID || vs[n - 1] == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }
    for (int i = 0; i < n >> 1; i++) {
        purc_variant_object_set(auth_data, vs[i * 2], vs[i * 2 + 1]);
        purc_variant_unref(vs[i * 2]);
        purc_variant_unref(vs[i * 2 + 1]);
    }

    msg->dataType = PCRDR_MSG_DATA_TYPE_JSON;
    msg->data = auth_data;

    int ret;
    if ((ret = pcrdr_send_request_and_wait_response(inst->conn_to_rdr,
            msg, PCRDR_TIME_AUTH_EXPECTED + 2, &response_msg)) < 0) {
        purc_set_error(PURC_ERROR_TIMEOUT);
        goto failed;
    }
    pcrdr_release_message(msg);
    msg = NULL;

    int ret_code = response_msg->retCode;
    pcrdr_release_message(response_msg);
    response_msg = NULL;

    if (ret_code != PCRDR_SC_OK) {
        purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
        goto failed;
    }

    return 0;

failed:
    if (response_msg)
        pcrdr_release_message(response_msg);

    if (msg)
        pcrdr_release_message(msg);

    return -1;
}

static int connect_to_renderer(struct pcinst *inst,
        const purc_instance_extra_info* extra_info)
{
    pcrdr_msg *msg = NULL, *response_msg = NULL;
    purc_variant_t session_data;
    // purc_rdrcomm_k rdr_comm;

    if (extra_info == NULL ||
            extra_info->renderer_comm == PURC_RDRCOMM_HEADLESS) {
        // rdr_comm = PURC_RDRCOMM_HEADLESS;
        msg = pcrdr_headless_connect(
            extra_info ? extra_info->renderer_uri : NULL,
            inst->app_name, inst->runner_name, &inst->conn_to_rdr);
    }
    else if (extra_info->renderer_comm == PURC_RDRCOMM_SOCKET) {
        // rdr_comm = PURC_RDRCOMM_SOCKET;
        msg = pcrdr_socket_connect(extra_info->renderer_uri,
            inst->app_name, inst->runner_name, &inst->conn_to_rdr);
    }
    else if (extra_info->renderer_comm == PURC_RDRCOMM_THREAD) {
        // rdr_comm = PURC_RDRCOMM_THREAD;
        msg = pcrdr_thread_connect(extra_info->renderer_uri,
            inst->app_name, inst->runner_name, &inst->conn_to_rdr);
    }
    else if (extra_info->renderer_comm == PURC_RDRCOMM_WEBSOCKET) {
        // rdr_comm = PURC_RDRCOMM_WEBSOCKET;
        msg = pcrdr_websocket_connect(extra_info->renderer_uri,
            inst->app_name, inst->runner_name, &inst->conn_to_rdr);
    }
    else {
        // TODO: other protocol
        purc_set_error(PURC_ERROR_NOT_SUPPORTED);
        return PURC_ERROR_NOT_SUPPORTED;
    }

    if (msg == NULL) {
        inst->conn_to_rdr = NULL;
        goto failed;
    }

    inst->conn_to_rdr->stats.start_time = purc_get_monotoic_time();

    if (extra_info && extra_info->renderer_uri) {
        inst->conn_to_rdr->uri = strdup(extra_info->renderer_uri);
    }
    else {
        inst->conn_to_rdr->uri = NULL;
    }

    if (msg->type == PCRDR_MSG_TYPE_RESPONSE && msg->retCode == PCRDR_SC_OK) {
        inst->rdr_caps =
            pcrdr_parse_renderer_capabilities(
                    purc_variant_get_string_const(msg->data));
        if (inst->rdr_caps == NULL) {
            goto failed;
        }

        /* Since v160, if the renderer needs authentication */
        if (inst->rdr_caps->challenge_code) {
            if (authenticate_app(inst)) {
                goto failed;
            }
        }
    }
    pcrdr_release_message(msg);

    /* send startSession request and wait for the response */
    msg = pcrdr_make_request_message(PCRDR_MSG_TARGET_SESSION, 0,
            PCRDR_OPERATION_STARTSESSION, NULL, NULL,
            PCRDR_MSG_ELEMENT_TYPE_VOID, NULL, NULL,
            PCRDR_MSG_DATA_TYPE_VOID, NULL, 0);
    if (msg == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
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
        goto failed;
    }
    for (int i = 0; i < 5; i++) {
        purc_variant_object_set(session_data, vs[i * 2], vs[i * 2 + 1]);
        purc_variant_unref(vs[i * 2]);
        purc_variant_unref(vs[i * 2 + 1]);
    }

    msg->dataType = PCRDR_MSG_DATA_TYPE_JSON;
    msg->data = session_data;

    int ret;
    if ((ret = pcrdr_send_request_and_wait_response(inst->conn_to_rdr,
            msg, PCRDR_TIME_DEF_EXPECTED, &response_msg)) < 0) {
        goto failed;
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
        goto failed;
    }

    bool set_page_groups = (inst->rdr_caps->workspace == 0);
    if (extra_info && extra_info->workspace_name &&
            inst->rdr_caps->workspace != 0) {
        /* send `createWorkspace` */
        msg = pcrdr_make_request_message(PCRDR_MSG_TARGET_SESSION, 0,
                PCRDR_OPERATION_CREATEWORKSPACE, NULL, NULL,
                PCRDR_MSG_ELEMENT_TYPE_VOID, NULL, NULL,
                PCRDR_MSG_DATA_TYPE_VOID, NULL, 0);
        if (msg == NULL) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }

        msg->data = purc_variant_make_object_0();
        purc_variant_t value = purc_variant_make_string_static(
                extra_info->workspace_name, true);
        if (value == PURC_VARIANT_INVALID) {
            goto failed;
        }
        purc_variant_object_set_by_static_ckey(msg->data, "name", value);
        purc_variant_unref(value);

        if (extra_info->workspace_title) {
            value = purc_variant_make_string_static(
                    extra_info->workspace_title, true);
            if (value == PURC_VARIANT_INVALID) {
                goto failed;
            }
            purc_variant_object_set_by_static_ckey(msg->data, "title", value);
            purc_variant_unref(value);
        }

        msg->dataType = PCRDR_MSG_DATA_TYPE_JSON;

        if (pcrdr_send_request_and_wait_response(inst->conn_to_rdr,
                msg, PCRDR_TIME_DEF_EXPECTED, &response_msg) < 0) {
            goto failed;
        }
        pcrdr_release_message(msg);
        msg = NULL;

        if (response_msg->retCode == PCRDR_SC_OK ||
                response_msg->retCode == PCRDR_SC_CONFLICT) {
            inst->rdr_caps->workspace_handle = response_msg->resultValue;
            if (response_msg->retCode == PCRDR_SC_OK)
                set_page_groups = true;
        }
        else {
            purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
            goto failed;
        }
        pcrdr_release_message(response_msg);
        response_msg = NULL;
    }
    else {
        inst->rdr_caps->workspace_handle = 0;   /* default workspace */
    }

    if (set_page_groups && extra_info && extra_info->workspace_layout) {
        /* send `setPageGroups` request */
        msg = pcrdr_make_request_message(PCRDR_MSG_TARGET_WORKSPACE,
                inst->rdr_caps->workspace_handle,
                PCRDR_OPERATION_SETPAGEGROUPS, NULL, NULL,
                PCRDR_MSG_ELEMENT_TYPE_VOID, NULL, NULL,
                PCRDR_MSG_DATA_TYPE_VOID, NULL, 0);
        if (msg == NULL) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }

        msg->data = purc_variant_make_string_static(
                extra_info->workspace_layout, true);
        if (msg->data == PURC_VARIANT_INVALID) {
            goto failed;
        }
        msg->dataType = PCRDR_MSG_DATA_TYPE_HTML;

        if (pcrdr_send_request_and_wait_response(inst->conn_to_rdr,
                msg, PCRDR_TIME_DEF_EXPECTED, &response_msg) < 0) {
            goto failed;
        }
        pcrdr_release_message(msg);
        msg = NULL;

        if (response_msg->retCode != PCRDR_SC_OK &&
                response_msg->retCode != PCRDR_SC_CONFLICT) {
            purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
            goto failed;
        }
        pcrdr_release_message(response_msg);
        response_msg = NULL;
    }

    return PURC_ERROR_OK;

failed:
    if (response_msg)
        pcrdr_release_message(response_msg);

    if (msg)
        pcrdr_release_message(msg);

    if (inst->conn_to_rdr) {
        pcrdr_disconnect(inst->conn_to_rdr);
        inst->conn_to_rdr = NULL;
    }

    return purc_get_last_error();
}

pcrdr_conn *
pcrdr_connect(const struct purc_instance_extra_info *extra_info)
{
    struct pcinst *inst = pcinst_current();
    if (inst == NULL) {
        purc_set_error(PURC_ERROR_NO_INSTANCE);
        return NULL;
    }

    if (inst->conn_to_rdr == NULL) {
        connect_to_renderer(inst, extra_info);
    }

    return inst->conn_to_rdr;
}

static int _init_instance(struct pcinst *inst,
        const purc_instance_extra_info* extra_info)
{
    return connect_to_renderer(inst, extra_info);
}

static void _cleanup_instance(struct pcinst *inst)
{
    if (inst->rdr_caps) {
        pcrdr_release_renderer_capabilities(inst->rdr_caps);
        inst->rdr_caps = NULL;
    }

    if (inst->conn_to_rdr) {
        pcrdr_disconnect(inst->conn_to_rdr);
        inst->conn_to_rdr = NULL;
    }
}

struct pcmodule _module_renderer = {
    .id              = PURC_HAVE_PCRDR,
    .module_inited   = 0,

    .init_once                   = _init_once,
    .init_instance               = _init_instance,
    .cleanup_instance            = _cleanup_instance,
};


