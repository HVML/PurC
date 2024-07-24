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
#include "private/runners.h"

#include "connect.h"

#include "pcrdr_err_msgs.inc"

/* Make sure the number of error messages matches the number of error codes */
#define _COMPILE_TIME_ASSERT(name, x)               \
       typedef int _dummy_ ## name[(x) * 2 - 1]

_COMPILE_TIME_ASSERT(msgs,
        PCA_TABLESIZE(pcrdr_err_msgs) == PCRDR_ERROR_NR);

#undef _COMPILE_TIME_ASSERT

#define DEFAULT_RENDERER_NAME       "xGUI Pro"

struct pcrdr_conn_extra_info {
    char            *workspace_name;
    char            *workspace_title;
    char            *workspace_layout;
    struct pcinst   *inst;

    unsigned int    allow_switching_rdr:1;
    unsigned int    allow_scaling_by_density:1;
};

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
    { PCRDR_OPERATION_LOADFROMURL,          0 }, // "loadFromURL" (0.9.18)
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

static purc_variant_t make_signature(struct pcinst *inst,
    struct renderer_capabilities *rdr_caps)
{
    unsigned char *sig = NULL;
    unsigned int sig_len;
    char *enc_sig = NULL;
    unsigned int enc_sig_len;

    int err_code = pcutils_sign_data(inst->app_name,
            (const unsigned char *)rdr_caps->challenge_code,
            strlen(rdr_caps->challenge_code),
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

static int set_session_args(struct pcinst *inst,
        purc_variant_t session_data, struct pcrdr_conn *conn_to_rdr,
        struct renderer_capabilities *rdr_caps)
{
    (void) rdr_caps;
    purc_variant_t vs[24] = { NULL };
    purc_variant_t tmp;
    int n = 0;

    vs[n++] = purc_variant_make_string_static("protocolName", false);
    vs[n++] = purc_variant_make_string_static(PCRDR_PURCMC_PROTOCOL_NAME,
            false);
    vs[n++] = purc_variant_make_string_static("protocolVersion", false);
    vs[n++] = purc_variant_make_ulongint(PCRDR_PURCMC_PROTOCOL_VERSION);
    vs[n++] = purc_variant_make_string_static("hostName", false);
    vs[n++] = purc_variant_make_string_static(conn_to_rdr->own_host_name,
            false);
    vs[n++] = purc_variant_make_string_static("appName", false);
    vs[n++] = purc_variant_make_string_static(inst->app_name, false);
    vs[n++] = purc_variant_make_string_static("runnerName", false);
    vs[n++] = purc_variant_make_string_static(inst->runner_name, false);
    vs[n++] = purc_variant_make_string_static("allowSwitchingRdr", false);
    vs[n++] = purc_variant_make_boolean(inst->allow_switching_rdr);
    vs[n++] = purc_variant_make_string_static("allowScalingByDensity", false);
    vs[n++] = purc_variant_make_boolean(inst->allow_scaling_by_density);

    vs[n++] = purc_variant_make_string_static("appLabel", false);
    tmp = purc_get_app_label(rdr_caps->locale);
    vs[n++] = tmp ? purc_variant_ref(tmp) : purc_variant_make_null();

    vs[n++] = purc_variant_make_string_static("appDesc", false);
    vs[n++] = purc_variant_ref(purc_get_app_description(rdr_caps->locale));

    vs[n++] = purc_variant_make_string_static("appIcon", false);
    tmp = purc_get_app_icon_url(rdr_caps->display_density, rdr_caps->locale);
    vs[n++] = tmp ? purc_variant_ref(tmp) : purc_variant_make_null();
    vs[n++] = purc_variant_make_string_static("runnerLabel", false);
    vs[n++] = purc_variant_ref(pcinst_get_runner_label(inst->runner_name,
                rdr_caps->locale));

    vs[n++] = purc_variant_make_string_static("duplicate", false);
    vs[n++] = purc_variant_make_boolean(inst->conn_to_rdr);

    if (vs[n - 1] == NULL) {
        goto failed;
    }

    for (int i = 0; i < n >> 1; i++) {
        bool success = purc_variant_object_set(session_data,
                vs[i * 2], vs[i * 2 + 1]);
        if (!success) {
            goto failed;
        }

        purc_variant_unref(vs[i * 2]);
        purc_variant_unref(vs[i * 2 + 1]);
        vs[i * 2] = NULL;
        vs[i * 2 + 1] = NULL;
    }

    return 0;

failed:
    for (int i = 0; i < n; i++) {
        if (vs[i])
            purc_variant_unref(vs[i]);
    }

    return -1;
}

static int append_authenticate_args(struct pcinst *inst,
        purc_variant_t session_data, struct renderer_capabilities *rdr_caps)
{
    purc_variant_t vs[6] = { NULL };
    int n = 0;

    if (inst->app_manifest == PURC_VARIANT_INVALID) {
        inst->app_manifest = pcinst_load_app_manifest(inst->app_name);
        if (inst->app_manifest == NULL) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }
    }

    vs[n++] = purc_variant_make_string_static("signature", false);
    vs[n++] = make_signature(inst, rdr_caps);
    vs[n++] = purc_variant_make_string_static("encodedIn", false);
    vs[n++] = purc_variant_make_string_static("base64", false);
    vs[n++] = purc_variant_make_string_static("timeoutSeconds", false);
    vs[n++] = purc_variant_make_ulongint(PCRDR_TIME_AUTH_EXPECTED);

    if (vs[n - 1] == NULL) {
        goto failed;
    }

    for (int i = 0; i < n >> 1; i++) {
        bool success = purc_variant_object_set(session_data,
                vs[i * 2], vs[i * 2 + 1]);
        if (!success) {
            goto failed;
        }

        purc_variant_unref(vs[i * 2]);
        purc_variant_unref(vs[i * 2 + 1]);
        vs[i * 2] = NULL;
        vs[i * 2 + 1] = NULL;
    }

    return 0;

failed:
    for (int i = 0; i < n; i++) {
        if (vs[i])
            purc_variant_unref(vs[i]);
    }

    return -1;
}

#if HAVE(STDATOMIC_H)

#include <stdatomic.h>

static char* generate_unique_rid(const char* name)
{
    static atomic_ullong atomic_accumulator;
    char *buf = malloc(strlen(name) + 5); // name-xxx

    unsigned long long accumulator = atomic_fetch_add(&atomic_accumulator, 1);
    sprintf(buf, "%s-%03lld", name, accumulator);
    return buf;
}

#else /* HAVE(STDATOMIC_H) */

static char* generate_unique_rid(const char* name)
{
    static atomic_ullong atomic_accumulator;
    char *buf = malloc(strlen(name) + 5); // name-xxx
    sprintf(buf, "%s-%03lld", name, accumulator);
    accumulator++;
    return buf;
}

#endif

static pcrdr_conn *
connect_to_renderer(struct pcinst *inst,
        const purc_instance_extra_info *extra_info)
{
    pcrdr_msg *msg = NULL, *response_msg = NULL;
    purc_variant_t session_data;
    struct pcrdr_conn *conn_to_rdr = NULL;
    // purc_rdrcomm_k rdr_comm;

    if (extra_info == NULL ||
            extra_info->renderer_comm == PURC_RDRCOMM_HEADLESS) {
        // rdr_comm = PURC_RDRCOMM_HEADLESS;
        msg = pcrdr_headless_connect(
            extra_info ? extra_info->renderer_uri : NULL,
            inst->app_name, inst->runner_name, &conn_to_rdr);
    }
    else if (extra_info->renderer_comm == PURC_RDRCOMM_SOCKET) {
        // rdr_comm = PURC_RDRCOMM_SOCKET;
        msg = pcrdr_socket_connect(extra_info->renderer_uri,
            inst->app_name, inst->runner_name, &conn_to_rdr);
    }
    else if (extra_info->renderer_comm == PURC_RDRCOMM_THREAD) {
        // rdr_comm = PURC_RDRCOMM_THREAD;
        msg = pcrdr_thread_connect(extra_info->renderer_uri,
            inst->app_name, inst->runner_name, &conn_to_rdr);
    }
    else if (extra_info->renderer_comm == PURC_RDRCOMM_WEBSOCKET) {
        // rdr_comm = PURC_RDRCOMM_WEBSOCKET;
        msg = pcrdr_websocket_connect(extra_info->renderer_uri,
            inst->app_name, inst->runner_name, &conn_to_rdr);
    }
    else {
        // TODO: other protocol
        purc_set_error(PURC_ERROR_NOT_SUPPORTED);
        goto failed;
    }

    if (msg == NULL) {
        conn_to_rdr = NULL;
        goto failed;
    }

    conn_to_rdr->stats.start_time = purc_get_monotoic_time();

    if (extra_info && extra_info->renderer_uri) {
        conn_to_rdr->uri = strdup(extra_info->renderer_uri);
    }
    else {
        conn_to_rdr->uri = NULL;
    }

    if (msg->type == PCRDR_MSG_TYPE_RESPONSE && msg->retCode == PCRDR_SC_OK) {
        conn_to_rdr->caps =
            pcrdr_parse_renderer_capabilities(
                    purc_variant_get_string_const(msg->data));
        if (conn_to_rdr->caps == NULL) {
            goto failed;
        }
    }
    pcrdr_release_message(msg);

    /* Since 0.9.18 */
    if (extra_info) {
        inst->allow_switching_rdr = extra_info->allow_switching_rdr;
        inst->allow_scaling_by_density = extra_info->allow_scaling_by_density;
    }
    else {
        inst->allow_switching_rdr = 1;
        inst->allow_scaling_by_density = 0;
    }

    conn_to_rdr->stats.start_time = purc_get_monotoic_time();
    /* send startSession request and wait for the response */
    msg = pcrdr_make_request_message(PCRDR_MSG_TARGET_SESSION, 0,
            PCRDR_OPERATION_STARTSESSION, NULL, NULL,
            PCRDR_MSG_ELEMENT_TYPE_VOID, NULL, NULL,
            PCRDR_MSG_DATA_TYPE_VOID, NULL, 0);
    if (msg == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    session_data = purc_variant_make_object(0, NULL, NULL);
    if (session_data == NULL) {
        goto failed;
    }

    if (set_session_args(inst, session_data, conn_to_rdr,
                conn_to_rdr->caps)) {
        goto failed;
    }

    /* Since v160, if the renderer needs authentication */
    if (conn_to_rdr->caps->challenge_code) {
        if (append_authenticate_args(inst, session_data, conn_to_rdr->caps)) {
            goto failed;
        }
    }

    msg->dataType = PCRDR_MSG_DATA_TYPE_JSON;
    msg->data = session_data;

    /* avoid send same request */
    purc_atom_t uri_atom = purc_atom_from_string_ex(ATOM_BUCKET_RDRID,
            conn_to_rdr->uri);

    int ret;
    /* startSession */
    int seconds_expected = PCRDR_TIME_AUTH_EXPECTED + 2;
    if ((ret = pcrdr_send_request_and_wait_response(conn_to_rdr, msg,
            seconds_expected, &response_msg)) < 0) {
        goto failed;
    }
    pcrdr_release_message(msg);
    msg = NULL;

    int ret_code = response_msg->retCode;
    if (ret_code == PCRDR_SC_OK) {
        conn_to_rdr->caps->session_handle = response_msg->resultValue;
        if (response_msg->data && purc_variant_is_object(response_msg->data)) {
            purc_variant_t name = purc_variant_object_get_by_ckey(
                    response_msg->data, "name");
            if (name && purc_variant_is_string(name)) {
                conn_to_rdr->name = strdup(purc_variant_get_string_const(name));
            }
            else {
                conn_to_rdr->name = strdup(DEFAULT_RENDERER_NAME);
            }
            purc_atom_t atom = purc_atom_try_string_ex(ATOM_BUCKET_RDRID,
                    conn_to_rdr->name);
            char *uid;
            if (atom) {
                uid = generate_unique_rid(conn_to_rdr->name);
            }
            else {
                uid = strdup(conn_to_rdr->name);
            }

            atom = purc_atom_from_string_ex(ATOM_BUCKET_RDRID, uid);
            conn_to_rdr->uid = uid;
            conn_to_rdr->id = atom;
        }

        conn_to_rdr->uri_atom = uri_atom;
    }
    else if (ret_code == PCRDR_SC_CONFLICT) {
        /* unix domain socket vs localhost websocket */
        if (!inst->conflict_uri) {
            inst->conflict_uri = conn_to_rdr->uri;
            inst->conflict_uri_atom = uri_atom;
            conn_to_rdr->uri = NULL;
        }
    }

    pcrdr_release_message(response_msg);
    response_msg = NULL;

    if (ret_code != PCRDR_SC_OK) {
        purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
        goto failed;
    }

    bool set_page_groups = (conn_to_rdr->caps->workspace == 0);
    if (extra_info && extra_info->workspace_name &&
            conn_to_rdr->caps->workspace != 0) {
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

        /* createWorkspace */
        if (pcrdr_send_request_and_wait_response(conn_to_rdr,
                msg, PCRDR_TIME_DEF_EXPECTED, &response_msg) < 0) {
            goto failed;
        }
        pcrdr_release_message(msg);
        msg = NULL;

        if (response_msg->retCode == PCRDR_SC_OK ||
                response_msg->retCode == PCRDR_SC_CONFLICT) {
            conn_to_rdr->caps->workspace_handle = response_msg->resultValue;
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
        conn_to_rdr->caps->workspace_handle = 0;   /* default workspace */
    }

    if (set_page_groups && extra_info && extra_info->workspace_layout) {
        /* send `setPageGroups` request */
        msg = pcrdr_make_request_message(PCRDR_MSG_TARGET_WORKSPACE,
                conn_to_rdr->caps->workspace_handle,
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

        /* setPageGroups */
        if (pcrdr_send_request_and_wait_response(conn_to_rdr,
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

    /* Add conns */
    list_add_tail(&conn_to_rdr->ln, &inst->conns);
    if (!inst->conn_to_rdr) {
        inst->conn_to_rdr = conn_to_rdr;
    }

    if (!inst->curr_conn) {
        inst->curr_conn = conn_to_rdr;
    }

#if 0
    /* update main conn to socket/websocket */
    if ((inst->conn_to_rdr != conn_to_rdr) &&
            (inst->conn_to_rdr->prot < PURC_RDRCOMM_SOCKET) &&
            (conn_to_rdr->prot >= PURC_RDRCOMM_SOCKET)) {
        inst->conn_to_rdr = conn_to_rdr;

        if ((inst->curr_conn != conn_to_rdr) &&
                (inst->curr_conn->prot < PURC_RDRCOMM_SOCKET)) {
            inst->curr_conn = conn_to_rdr;
        }
    }
#endif

    return conn_to_rdr;

failed:
    if (response_msg)
        pcrdr_release_message(response_msg);

    if (msg)
        pcrdr_release_message(msg);

    if (conn_to_rdr) {
        pcrdr_disconnect(conn_to_rdr);
        conn_to_rdr = NULL;
    }

    return NULL;
}

purc_variant_t pcintr_crtn_observed_create(purc_atom_t cid);

static void
broadcast_renderer_event(struct pcinst *inst, const char *event_type,
        const char *event_sub_type, purc_variant_t data)
{
    struct pcintr_heap *heap = inst->intr_heap;
    struct list_head *crtns = &heap->crtns;
    pcintr_coroutine_t p, q;
    list_for_each_entry_safe(p, q, crtns, ln) {
        pcintr_coroutine_t co = p;
        pcintr_stack_t stack = &co->stack;
        purc_variant_t hvml = pcintr_crtn_observed_create(co->cid);
        pcintr_coroutine_post_event(stack->co->cid,
                PCRDR_MSG_EVENT_REDUCE_OPT_OVERLAY,
                hvml, event_type, event_sub_type,
                data, PURC_VARIANT_INVALID);
        purc_variant_unref(hvml);
    }

    crtns = &heap->stopped_crtns;
    list_for_each_entry_safe(p, q, crtns, ln) {
        pcintr_coroutine_t co = p;
        pcintr_stack_t stack = &co->stack;
        purc_variant_t hvml = pcintr_crtn_observed_create(co->cid);
        pcintr_coroutine_post_event(stack->co->cid,
                PCRDR_MSG_EVENT_REDUCE_OPT_OVERLAY,
                hvml, event_type, event_sub_type,
                data, PURC_VARIANT_INVALID);
        purc_variant_unref(hvml);
    }
}

int pcrdr_switch_renderer(struct pcinst *inst, const char *comm,
        const char *uri)
{
    pcrdr_msg *msg = NULL, *response_msg = NULL;
    purc_variant_t session_data;
    struct pcrdr_conn *n_conn_to_rdr = NULL;
    struct renderer_capabilities *n_rdr_caps = NULL;

    if (inst->conn_to_rdr->uri && strcmp(uri, inst->conn_to_rdr->uri) == 0) {
        PC_WARN("switch renderer uri(%s) same as current uri, do nothing!\n",
                uri);
        return PURC_ERROR_OK;
    }

    /* TODO: get workspace, group info from keep info */
    purc_instance_extra_info info = {0};
    info.workspace_name = inst->workspace_name;
    info.workspace_title = inst->workspace_title;
    info.workspace_layout = inst->workspace_layout;

    purc_instance_extra_info* extra_info = &info;

    if (strcasecmp(comm, PURC_RDRCOMM_NAME_SOCKET) == 0) {
        // rdr_comm = PURC_RDRCOMM_SOCKET;
        msg = pcrdr_socket_connect(uri,
            inst->app_name, inst->runner_name, &n_conn_to_rdr);
    }
    else if (strcasecmp(comm, PURC_RDRCOMM_NAME_WEBSOCKET) == 0) {
        // rdr_comm = PURC_RDRCOMM_WEBSOCKET;
        msg = pcrdr_websocket_connect(uri,
            inst->app_name, inst->runner_name, &n_conn_to_rdr);
    }
    else {
        // TODO: other protocol
        purc_set_error(PURC_ERROR_NOT_SUPPORTED);
        return PURC_ERROR_NOT_SUPPORTED;
    }

    if (msg == NULL) {
        n_conn_to_rdr = NULL;
        goto failed;
    }

    n_conn_to_rdr->stats.start_time = purc_get_monotoic_time();

    if (uri) {
        n_conn_to_rdr->uri = strdup(uri);
    }
    else {
        n_conn_to_rdr->uri = NULL;
    }

    if (msg->type == PCRDR_MSG_TYPE_RESPONSE && msg->retCode == PCRDR_SC_OK) {
        n_rdr_caps = pcrdr_parse_renderer_capabilities(
                    purc_variant_get_string_const(msg->data));
        if (n_rdr_caps == NULL) {
            goto failed;
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

    session_data = purc_variant_make_object(0, NULL, NULL);
    if (session_data == NULL) {
        goto failed;
    }

    if (set_session_args(inst, session_data, n_conn_to_rdr, n_rdr_caps)) {
        goto failed;
    }

    /* Since v160, if the renderer needs authentication */
    if (n_rdr_caps->challenge_code) {
        if (append_authenticate_args(inst, session_data, n_rdr_caps)) {
            goto failed;
        }
    }

    msg->dataType = PCRDR_MSG_DATA_TYPE_JSON;
    msg->data = session_data;

    int ret;
    /* startSession */
    if ((ret = pcrdr_send_request_and_wait_response(n_conn_to_rdr, msg,
            n_rdr_caps->challenge_code ? (PCRDR_TIME_AUTH_EXPECTED + 2) :
            PCRDR_TIME_DEF_EXPECTED, &response_msg)) < 0) {
        goto failed;
    }
    pcrdr_release_message(msg);
    msg = NULL;

    int ret_code = response_msg->retCode;
    if (ret_code == PCRDR_SC_OK) {
        n_rdr_caps->session_handle = response_msg->resultValue;
        if (response_msg->data && purc_variant_is_object(response_msg->data)) {
            purc_variant_t name = purc_variant_object_get_by_ckey(
                    response_msg->data, "name");
            if (name && purc_variant_is_string(name)) {
                n_conn_to_rdr->name = strdup(purc_variant_get_string_const(name));
            }
            else {
                n_conn_to_rdr->name = strdup(DEFAULT_RENDERER_NAME);
            }
            purc_atom_t atom = purc_atom_try_string_ex(ATOM_BUCKET_RDRID,
                    n_conn_to_rdr->name);
            char *uid;
            if (atom) {
                uid = generate_unique_rid(n_conn_to_rdr->name);
            }
            else {
                uid = strdup(n_conn_to_rdr->name);
            }

            atom = purc_atom_from_string_ex(ATOM_BUCKET_RDRID, uid);
            n_conn_to_rdr->uid = uid;
            n_conn_to_rdr->id = atom;
        }

        if (n_conn_to_rdr->uri) {
            n_conn_to_rdr->uri_atom = purc_atom_from_string_ex(ATOM_BUCKET_RDRID,
                    n_conn_to_rdr->uri);
        }
    }

    pcrdr_release_message(response_msg);
    response_msg = NULL;

    if (ret_code != PCRDR_SC_OK) {
        purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
        goto failed;
    }

    /* end origin session */

    pcrdr_conn_set_event_handler(inst->conn_to_rdr, NULL);
    inst->conn_to_rdr_origin = inst->conn_to_rdr;
    list_del(&inst->conn_to_rdr_origin->ln);

    inst->conn_to_rdr = n_conn_to_rdr;
    inst->conn_to_rdr->caps = n_rdr_caps;

    list_add_tail(&inst->conn_to_rdr->ln, &inst->conns);
    if (inst->curr_conn == inst->conn_to_rdr_origin) {
        inst->curr_conn = inst->conn_to_rdr;
    }

    pcrdr_conn_set_extra_message_source(inst->conn_to_rdr, pcrun_extra_message_source,
            NULL, NULL);
    pcrdr_conn_set_request_handler(inst->conn_to_rdr, pcrun_request_handler);

    /* TODO: send origin workspace, pagegroup */
    bool set_page_groups = (n_rdr_caps->workspace == 0);
    if (extra_info && extra_info->workspace_name &&
            n_rdr_caps->workspace != 0) {
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

        /* createWorkspace */
        if (pcrdr_send_request_and_wait_response(n_conn_to_rdr,
                msg, PCRDR_TIME_DEF_EXPECTED, &response_msg) < 0) {
            goto failed;
        }
        pcrdr_release_message(msg);
        msg = NULL;

        if (response_msg->retCode == PCRDR_SC_OK ||
                response_msg->retCode == PCRDR_SC_CONFLICT) {
            n_rdr_caps->workspace_handle = response_msg->resultValue;
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
        n_rdr_caps->workspace_handle = 0;   /* default workspace */
    }

    if (set_page_groups && extra_info && extra_info->workspace_layout) {
        /* send `setPageGroups` request */
        msg = pcrdr_make_request_message(PCRDR_MSG_TARGET_WORKSPACE,
                n_rdr_caps->workspace_handle,
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

        /* setPageGroups */
        if (pcrdr_send_request_and_wait_response(n_conn_to_rdr,
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
    PC_TIMESTAMP("connect to new renderer, app: %s runner: %s\n", inst->app_name, inst->runner_name);

    pcintr_attach_renderer(inst, inst->conn_to_rdr, inst->conn_to_rdr_origin);

    /* broadcase event rdrState:newRenderer */
    broadcast_renderer_event(inst, MSG_TYPE_RDR_STATE,
            MSG_SUB_TYPE_NEW_RENDERER, PURC_VARIANT_INVALID);

    PC_WARN("switch renderer comm=%s|uri=%s success!\n", comm, uri);
    return PURC_ERROR_OK;

failed:
    PC_WARN("switch renderer comm=%s|uri=%s failed! errcode=%d\n", comm, uri,
            purc_get_last_error());
    if (response_msg)
        pcrdr_release_message(response_msg);

    if (msg)
        pcrdr_release_message(msg);

    if (n_conn_to_rdr) {
        pcrdr_disconnect(n_conn_to_rdr);
        n_conn_to_rdr = NULL;
    }

    return purc_get_last_error();
}

#define COMM_NONE               "none"

#define KEY_COMM                "comm"
#define KEY_PROT                "prot"
#define KEY_PROT_VERSION        "prot-version"
#define KEY_PROT_VER_CODE       "prot-ver-code"
#define KEY_URI                 "uri"
#define KEY_NAME                "name"
#define KEY_ID                  "id"

static const char *
rdr_comm(struct pcrdr_conn *rdr)
{
    const char *comm = COMM_NONE;
    if (!rdr) {
        goto out;
    }

    switch (rdr->prot) {
    case PURC_RDRCOMM_HEADLESS:
        comm = PURC_RDRCOMM_NAME_HEADLESS;
        break;

    case PURC_RDRCOMM_THREAD:
        comm = PURC_RDRCOMM_NAME_THREAD;
        break;

    case PURC_RDRCOMM_SOCKET:
        comm = PURC_RDRCOMM_NAME_SOCKET;
        break;

    case PURC_RDRCOMM_HBDBUS:
        comm = PURC_RDRCOMM_NAME_HBDBUS;
        break;

    case PURC_RDRCOMM_WEBSOCKET:
        comm = PURC_RDRCOMM_NAME_WEBSOCKET;
        break;
    }

out:
    return comm;
}

static const char *
rdr_uri(struct pcrdr_conn *rdr)
{
    UNUSED_PARAM(rdr);
    if (rdr && rdr->uri) {
        return rdr->uri;
    }
    return "";
}

purc_variant_t
pcrdr_data(pcrdr_conn *conn)
{
    purc_variant_t vs[14] = { NULL };
    struct pcinst* inst = pcinst_current();
    struct pcrdr_conn *rdr = conn;
    const char *comm = rdr_comm(rdr);
    const char *uri = rdr_uri(rdr);

    purc_variant_t data = purc_variant_make_object(0, NULL, NULL);
    if (data == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out_clear_data;
    }

    if (!rdr) {
        vs[0] = purc_variant_make_string_static(KEY_COMM, false);
        vs[1] = purc_variant_make_string_static(comm, false);
        if (!vs[1]) {
            goto out_clear_vs;
        }
        purc_variant_object_set(data, vs[0], vs[1]);
        purc_variant_unref(vs[0]);
        purc_variant_unref(vs[1]);
        goto out;
    }

    vs[0] = purc_variant_make_string_static(KEY_PROT, false);
    vs[2] = purc_variant_make_string_static(KEY_PROT_VERSION, false);
    vs[4] = purc_variant_make_string_static(KEY_PROT_VER_CODE, false);

    struct renderer_capabilities *rdr_caps = inst->conn_to_rdr->caps;
    if (rdr_caps) {
        char buf[21];
        snprintf(buf, 20, "%ld", rdr_caps->prot_version);
        vs[1] = purc_variant_make_string_static(rdr_caps->prot_name, false);
        vs[3] = purc_variant_make_string(buf, false);
        vs[5] = purc_variant_make_ulongint(rdr_caps->prot_version);
    }
    else {
        vs[1] = purc_variant_make_string_static(PCRDR_PURCMC_PROTOCOL_NAME, false);
        vs[3] = purc_variant_make_string_static(PCRDR_PURCMC_PROTOCOL_VERSION_STRING,
                false);
        vs[5] = purc_variant_make_ulongint(PCRDR_PURCMC_PROTOCOL_VERSION);
    }

    vs[6] = purc_variant_make_string_static(KEY_COMM, false);
    vs[7] = purc_variant_make_string_static(comm, false);

    vs[8] = purc_variant_make_string_static(KEY_URI, false);
    vs[9] = purc_variant_make_string_static(uri, false);

    vs[10] = purc_variant_make_string_static(KEY_NAME, false);
    vs[11] = purc_variant_make_string_static(rdr->name, false);

    vs[12] = purc_variant_make_string_static(KEY_ID, false);
    vs[13] = purc_variant_make_string_static(rdr->uid, false);

    if (!vs[13]) {
        goto out_clear_vs;
    }

    for (int i = 0; i < 7; i++) {
        if (vs[i * 2]) {
            purc_variant_object_set(data, vs[i * 2], vs[i * 2 + 1]);
            purc_variant_unref(vs[i * 2]);
            purc_variant_unref(vs[i * 2 + 1]);
        }
    }
    goto out;

out_clear_vs:
    for (int i = 0; i < 10; i++) {
        if (vs[i]) {
            purc_variant_unref(vs[i]);
        }
    }

out_clear_data:
    purc_variant_unref(data);
    data = PURC_VARIANT_INVALID;

out:
    return data;
}

int
connect_to_renderer_async(struct pcinst *inst,
        const purc_instance_extra_info *extra_info,
        void *context, pcrdr_response_handler response_handler)
{
    pcrdr_msg *msg = NULL, *response_msg = NULL;
    purc_variant_t session_data;
    struct pcrdr_conn *conn_to_rdr = NULL;
    // purc_rdrcomm_k rdr_comm;

    if (extra_info == NULL ||
            extra_info->renderer_comm == PURC_RDRCOMM_HEADLESS) {
        // rdr_comm = PURC_RDRCOMM_HEADLESS;
        msg = pcrdr_headless_connect(
            extra_info ? extra_info->renderer_uri : NULL,
            inst->app_name, inst->runner_name, &conn_to_rdr);
    }
    else if (extra_info->renderer_comm == PURC_RDRCOMM_SOCKET) {
        // rdr_comm = PURC_RDRCOMM_SOCKET;
        msg = pcrdr_socket_connect(extra_info->renderer_uri,
            inst->app_name, inst->runner_name, &conn_to_rdr);
    }
    else if (extra_info->renderer_comm == PURC_RDRCOMM_THREAD) {
        // rdr_comm = PURC_RDRCOMM_THREAD;
        msg = pcrdr_thread_connect(extra_info->renderer_uri,
            inst->app_name, inst->runner_name, &conn_to_rdr);
    }
    else if (extra_info->renderer_comm == PURC_RDRCOMM_WEBSOCKET) {
        // rdr_comm = PURC_RDRCOMM_WEBSOCKET;
        msg = pcrdr_websocket_connect(extra_info->renderer_uri,
            inst->app_name, inst->runner_name, &conn_to_rdr);
    }
    else {
        // TODO: other protocol
        purc_set_error(PURC_ERROR_NOT_SUPPORTED);
        goto failed;
    }

    if (msg == NULL) {
        conn_to_rdr = NULL;
        goto failed;
    }

    conn_to_rdr->stats.start_time = purc_get_monotoic_time();

    if (extra_info && extra_info->renderer_uri) {
        conn_to_rdr->uri = strdup(extra_info->renderer_uri);
    }
    else {
        conn_to_rdr->uri = NULL;
    }

    if (msg->type == PCRDR_MSG_TYPE_RESPONSE && msg->retCode == PCRDR_SC_OK) {
        conn_to_rdr->caps =
            pcrdr_parse_renderer_capabilities(
                    purc_variant_get_string_const(msg->data));
        if (conn_to_rdr->caps == NULL) {
            goto failed;
        }
    }
    pcrdr_release_message(msg);

    /* Since 0.9.18 */
    if (extra_info) {
        inst->allow_switching_rdr = extra_info->allow_switching_rdr;
        inst->allow_scaling_by_density = extra_info->allow_scaling_by_density;
    }
    else {
        inst->allow_switching_rdr = 1;
        inst->allow_scaling_by_density = 0;
    }

    conn_to_rdr->stats.start_time = purc_get_monotoic_time();
    /* send startSession request and wait for the response */
    msg = pcrdr_make_request_message(PCRDR_MSG_TARGET_SESSION, 0,
            PCRDR_OPERATION_STARTSESSION, NULL, NULL,
            PCRDR_MSG_ELEMENT_TYPE_VOID, NULL, NULL,
            PCRDR_MSG_DATA_TYPE_VOID, NULL, 0);
    if (msg == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    session_data = purc_variant_make_object(0, NULL, NULL);
    if (session_data == NULL) {
        goto failed;
    }

    if (set_session_args(inst, session_data, conn_to_rdr,
                conn_to_rdr->caps)) {
        goto failed;
    }

    /* Since v160, if the renderer needs authentication */
    if (conn_to_rdr->caps->challenge_code) {
        if (append_authenticate_args(inst, session_data, conn_to_rdr->caps)) {
            goto failed;
        }
    }

    msg->dataType = PCRDR_MSG_DATA_TYPE_JSON;
    msg->data = session_data;

    /* avoid send same request */
    conn_to_rdr->uri_atom = purc_atom_from_string_ex(ATOM_BUCKET_RDRID,
            conn_to_rdr->uri);

    /* startSession */
    int seconds_expected = PCRDR_TIME_AUTH_EXPECTED + 2;
    int ret = pcrdr_send_request(conn_to_rdr, msg, seconds_expected, context,
            response_handler);
    pcrdr_release_message(msg);
    msg = NULL;

    if (ret != 0) {
        goto failed;
    }

    /* Add conns */
    list_add_tail(&conn_to_rdr->ln, &inst->pending_conns);

    return ret;

failed:
    if (response_msg) {
        pcrdr_release_message(response_msg);
    }

    if (msg) {
        pcrdr_release_message(msg);
    }

    if (conn_to_rdr) {
        pcrdr_disconnect(conn_to_rdr);
        conn_to_rdr = NULL;
    }

    return -1;
}

int connect_to_renderer_response_handler(pcrdr_conn *conn_to_rdr,
        const char *request_id, int state,
        void *context, const pcrdr_msg *response_msg)
{
    UNUSED_PARAM(request_id);
    UNUSED_PARAM(state);

    int ret = -1;
    pcrdr_msg *msg = NULL;
    pcrdr_msg *res_msg = NULL;

    struct pcrdr_conn_extra_info *extra_info;
    extra_info = (struct pcrdr_conn_extra_info *) context;

    struct pcinst *inst = extra_info->inst;

    if (state == PCRDR_RESPONSE_CANCELLED) {
        goto failed;
    }

    if (state != PCRDR_RESPONSE_RESULT) {
        list_del(&conn_to_rdr->ln);
        list_add_tail(&conn_to_rdr->ln, &inst->ready_to_close_conns);
        conn_to_rdr->async_close_expected = purc_get_monotoic_time() + PCRDR_TIME_DEF_EXPECTED;
        goto failed;
    }

    int ret_code = response_msg->retCode;
    if (ret_code == PCRDR_SC_OK) {
        conn_to_rdr->caps->session_handle = response_msg->resultValue;
        if (response_msg->data && purc_variant_is_object(response_msg->data)) {
            purc_variant_t name = purc_variant_object_get_by_ckey(
                    response_msg->data, "name");
            if (name && purc_variant_is_string(name)) {
                conn_to_rdr->name = strdup(purc_variant_get_string_const(name));
            }
            else {
                conn_to_rdr->name = strdup(DEFAULT_RENDERER_NAME);
            }
            purc_atom_t atom = purc_atom_try_string_ex(ATOM_BUCKET_RDRID,
                    conn_to_rdr->name);
            char *uid;
            if (atom) {
                uid = generate_unique_rid(conn_to_rdr->name);
            }
            else {
                uid = strdup(conn_to_rdr->name);
            }

            atom = purc_atom_from_string_ex(ATOM_BUCKET_RDRID, uid);
            conn_to_rdr->uid = uid;
            conn_to_rdr->id = atom;
        }
    }
    else if (ret_code == PCRDR_SC_CONFLICT) {
        /* unix domain socket vs localhost websocket */
        if (!inst->conflict_uri) {
            inst->conflict_uri = conn_to_rdr->uri;
            inst->conflict_uri_atom = conn_to_rdr->uri_atom;
            conn_to_rdr->uri = NULL;
        }
        list_del(&conn_to_rdr->ln);
        list_add_tail(&conn_to_rdr->ln, &inst->ready_to_close_conns);
        conn_to_rdr->async_close_expected = purc_get_monotoic_time() + PCRDR_TIME_DEF_EXPECTED;
    }

    if (ret_code != PCRDR_SC_OK) {
        purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
        goto failed;
    }

    bool set_page_groups = (conn_to_rdr->caps->workspace == 0);
    if (extra_info && extra_info->workspace_name &&
            conn_to_rdr->caps->workspace != 0) {
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

        /* createWorkspace */
        if (pcrdr_send_request_and_wait_response(conn_to_rdr,
                msg, PCRDR_TIME_DEF_EXPECTED, &res_msg) < 0) {
            goto failed;
        }
        pcrdr_release_message(msg);
        msg = NULL;

        if (res_msg->retCode == PCRDR_SC_OK ||
                res_msg->retCode == PCRDR_SC_CONFLICT) {
            conn_to_rdr->caps->workspace_handle = res_msg->resultValue;
            if (res_msg->retCode == PCRDR_SC_OK) {
                set_page_groups = true;
            }
        }
        else {
            purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
            goto failed;
        }
        pcrdr_release_message(res_msg);
        res_msg = NULL;
    }
    else {
        conn_to_rdr->caps->workspace_handle = 0;   /* default workspace */
    }

    if (set_page_groups && extra_info && extra_info->workspace_layout) {
        /* send `setPageGroups` request */
        msg = pcrdr_make_request_message(PCRDR_MSG_TARGET_WORKSPACE,
                conn_to_rdr->caps->workspace_handle,
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

        /* setPageGroups */
        if (pcrdr_send_request_and_wait_response(conn_to_rdr,
                msg, PCRDR_TIME_DEF_EXPECTED, &res_msg) < 0) {
            goto failed;
        }
        pcrdr_release_message(msg);
        msg = NULL;

        if (res_msg->retCode != PCRDR_SC_OK &&
                res_msg->retCode != PCRDR_SC_CONFLICT) {
            purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
            goto failed;
        }
        pcrdr_release_message(res_msg);
        res_msg = NULL;
    }

    /* del from pending conns */
    list_del(&conn_to_rdr->ln);

    /* Add conns */
    list_add_tail(&conn_to_rdr->ln, &inst->conns);
    if (!inst->conn_to_rdr) {
        inst->conn_to_rdr = conn_to_rdr;
    }

    if (!inst->curr_conn) {
        inst->curr_conn = conn_to_rdr;
    }

    pcrdr_conn_set_request_handler(conn_to_rdr, pcrun_request_handler);

    pcintr_attach_renderer(inst, conn_to_rdr, NULL);

    /* broadcase event rdrState:newDuplicate */
    purc_variant_t data = pcrdr_data(conn_to_rdr);
    broadcast_renderer_event(inst, MSG_TYPE_RDR_STATE,
            MSG_SUB_TYPE_NEW_DUPLICATE, data);
    if (data) {
        purc_variant_unref(data);
    }

    ret = 0;

failed:
    if (extra_info->workspace_name) {
        free(extra_info->workspace_name);
        extra_info->workspace_name = NULL;
    }

    if (extra_info->workspace_title) {
        free(extra_info->workspace_title);
        extra_info->workspace_title = NULL;
    }

    if (extra_info->workspace_layout) {
        free(extra_info->workspace_layout);
        extra_info->workspace_layout = NULL;
    }

    free(extra_info);

    if (res_msg) {
        pcrdr_release_message(res_msg);
    }

    if (msg) {
        pcrdr_release_message(msg);
    }

    return ret;
}

const char *
purc_connect_to_renderer(purc_instance_extra_info *extra_info)
{
    struct pcinst *inst = pcinst_current();
    if (inst == NULL) {
        purc_set_error(PURC_ERROR_NO_INSTANCE);
        return NULL;
    }

    purc_atom_t rdr_uri_atom = purc_atom_try_string_ex(ATOM_BUCKET_RDRID,
            extra_info->renderer_uri);
    if (rdr_uri_atom) {
        struct list_head *conns = &inst->conns;
        struct pcrdr_conn *pconn, *qconn;
        list_for_each_entry_safe(pconn, qconn, conns, ln) {
            if (pconn->uri_atom == rdr_uri_atom) {
                return pconn->uid;
            }
        }
    }

    if (extra_info) {
        if (!extra_info->workspace_name) {
            extra_info->workspace_name = inst->workspace_name;
        }
        if (!extra_info->workspace_title) {
            extra_info->workspace_title = inst->workspace_title;
        }
        if (!extra_info->workspace_layout) {
            extra_info->workspace_layout = inst->workspace_layout;
        }
    }


    pcrdr_conn *conn = connect_to_renderer(inst, extra_info);
    if (!conn) {
        /* PCRDR_ERROR_SERVER_REFUSED */
        purc_clr_error();
        goto out;
    }

#if 0
    /* inst->conn_to_rdr do this */
    pcrdr_conn_set_extra_message_source(conn, pcrun_extra_message_source,
            NULL, NULL);
#endif
    pcrdr_conn_set_request_handler(conn, pcrun_request_handler);

    pcintr_attach_renderer(inst, conn, NULL);

    /* broadcase event rdrState:newDuplicate */
    purc_variant_t data = pcrdr_data(conn);
    broadcast_renderer_event(inst, MSG_TYPE_RDR_STATE,
            MSG_SUB_TYPE_NEW_DUPLICATE, data);
    if (data) {
        purc_variant_unref(data);
    }

out:
    return conn ? conn->uid : NULL;
}

int
purc_connect_to_renderer_async(purc_instance_extra_info *extra_info)
{
    int ret = -1;
    struct pcrdr_conn_extra_info *conn_extra_info = NULL;
    struct pcinst *inst = pcinst_current();
    if (inst == NULL) {
        purc_set_error(PURC_ERROR_NO_INSTANCE);
        goto out;
    }

    purc_atom_t rdr_uri_atom = purc_atom_try_string_ex(ATOM_BUCKET_RDRID,
            extra_info->renderer_uri);
    if (rdr_uri_atom) {
        struct list_head *conns = &inst->conns;
        struct pcrdr_conn *pconn, *qconn;
        list_for_each_entry_safe(pconn, qconn, conns, ln) {
            if (pconn->uri_atom == rdr_uri_atom) {
                ret = 0;
                goto out;
            }
        }
    }

    if (extra_info) {
        if (!extra_info->workspace_name) {
            extra_info->workspace_name = inst->workspace_name;
        }
        if (!extra_info->workspace_title) {
            extra_info->workspace_title = inst->workspace_title;
        }
        if (!extra_info->workspace_layout) {
            extra_info->workspace_layout = inst->workspace_layout;
        }
    }

    conn_extra_info = (struct pcrdr_conn_extra_info *)calloc(1,
            sizeof(*conn_extra_info));

    if (extra_info->workspace_name) {
        conn_extra_info->workspace_name = strdup(extra_info->workspace_name);
    }

    if (extra_info->workspace_title) {
        conn_extra_info->workspace_title = strdup(extra_info->workspace_title);
    }

    if (extra_info->workspace_layout) {
        conn_extra_info->workspace_layout = strdup(extra_info->workspace_layout);
    }

    conn_extra_info->inst = inst;

    ret = connect_to_renderer_async(inst, extra_info, conn_extra_info,
            connect_to_renderer_response_handler);

out:
    return ret;
}

int
purc_disconnect_from_renderer(const char *id)
{
    if (!id) {
        goto failed;
    }

    struct pcinst *inst = pcinst_current();
    struct list_head *conns = &inst->conns;
    struct pcrdr_conn *pconn, *qconn;
    list_for_each_entry_safe(pconn, qconn, conns, ln) {
        if (strcmp(pconn->uid, id) == 0) {
            pcintr_detach_renderer(inst, pconn);

            /* broadcase event rdrState:lostDuplicate */
            purc_variant_t data = pcrdr_data(pconn);
            broadcast_renderer_event(inst, MSG_TYPE_RDR_STATE,
                    MSG_SUB_TYPE_LOST_DUPLICATE, data);
            if (data) {
                purc_variant_unref(data);
            }

            list_del(&pconn->ln);
            pcrdr_disconnect(pconn);

            if (inst->conn_to_rdr == pconn) {
                inst->conn_to_rdr = NULL;
            }
            if (inst->curr_conn == pconn) {
                inst->curr_conn = NULL;
            }

            /* choose main conn */
            if (inst->conn_to_rdr == NULL) {
                if (inst->curr_conn) {
                    inst->conn_to_rdr = inst->curr_conn;
                }
                else {
                    struct list_head *conns = &inst->conns;
                    inst->conn_to_rdr = list_first_entry(conns, struct pcrdr_conn, ln);
                    inst->curr_conn = inst->conn_to_rdr;
                }
            }
            return 0;
        }
    }


failed:
    return -1;
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
    list_head_init(&inst->conns);
    list_head_init(&inst->pending_conns);
    list_head_init(&inst->ready_to_close_conns);

    /* keep workspace info */
    if (extra_info) {
        if (extra_info->workspace_name) {
            inst->workspace_name = strdup(extra_info->workspace_name);
        }

        if (extra_info->workspace_title) {
            inst->workspace_title = strdup(extra_info->workspace_title);
        }

        if (extra_info->workspace_layout) {
            inst->workspace_layout = strdup(extra_info->workspace_layout);
        }
    }

    pcrdr_conn *conn = connect_to_renderer(inst, extra_info);
    return conn ? 0 :  purc_get_last_error();
}

static void _cleanup_instance(struct pcinst *inst)
{
    struct list_head *conns = &inst->conns;
    struct pcrdr_conn *pconn, *qconn;
    list_for_each_entry_safe(pconn, qconn, conns, ln) {
        list_del(&pconn->ln);
        pcrdr_disconnect(pconn);

        if (inst->conn_to_rdr == pconn) {
            inst->conn_to_rdr = NULL;
        }
    }

    conns = &inst->pending_conns;
    list_for_each_entry_safe(pconn, qconn, conns, ln) {
        list_del(&pconn->ln);
        pcrdr_disconnect(pconn);
    }

    conns = &inst->ready_to_close_conns;
    list_for_each_entry_safe(pconn, qconn, conns, ln) {
        list_del(&pconn->ln);
        pcrdr_disconnect(pconn);
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


