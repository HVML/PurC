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

#define COMM_NONE               "none"

#define KEY_COMM                "comm"
#define KEY_PROT                "prot"
#define KEY_PROT_VERSION        "prot-version"
#define KEY_PROT_VER_CODE       "prot-ver-code"
#define KEY_URI                 "uri"

static struct pcrdr_conn *
rdr_conn()
{
    struct pcinst* inst = pcinst_current();
    return inst->conn_to_rdr;
}

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

    case PURC_RDRCOMM_HIBUS:
        comm = PURC_RDRCOMM_NAME_HIBUS;
        break;
    }

out:
    return comm;
}

static const char *
rdr_uri(struct pcrdr_conn *rdr)
{
    UNUSED_PARAM(rdr);
    return "";
}

static purc_variant_t
status_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    purc_variant_t vs[10] = { NULL };
    struct pcrdr_conn *rdr = rdr_conn();
    const char *comm = rdr_comm(rdr);
    const char *uri = rdr_uri(rdr);

    purc_variant_t data = purc_variant_make_object(0, NULL, NULL);
    if (data == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out_clear_data;
    }

    vs[0] = purc_variant_make_string_static(KEY_PROT, false);
    vs[1] = purc_variant_make_string_static(PCRDR_PURCMC_PROTOCOL_NAME, false);
    vs[2] = purc_variant_make_string_static(KEY_PROT_VERSION, false);
    vs[3] = purc_variant_make_string_static(PCRDR_PURCMC_PROTOCOL_VERSION_STRING,
            false);
    vs[4] = purc_variant_make_string_static(KEY_PROT_VER_CODE, false);
    vs[5] = purc_variant_make_ulongint(PCRDR_PURCMC_PROTOCOL_VERSION);
    vs[6] = purc_variant_make_string_static(KEY_COMM, false);
    vs[7] = purc_variant_make_string_static(comm, false);

    vs[8] = purc_variant_make_string_static(KEY_URI, false);
    vs[9] = purc_variant_make_string_static(uri, false);

    if (!vs[9]) {
        goto out_clear_vs;
    }

    for (int i = 0; i < 5; i++) {
        purc_variant_object_set(data, vs[i * 2], vs[i * 2 + 1]);
        purc_variant_unref(vs[i * 2]);
        purc_variant_unref(vs[i * 2 + 1]);
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

out:
    return data;
}

static purc_variant_t
comm_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);
    struct pcrdr_conn *rdr = rdr_conn();
    const char *comm = rdr_comm(rdr);
    return purc_variant_make_string_static(comm, false);
}

static purc_variant_t
uri_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);
    struct pcrdr_conn *rdr = rdr_conn();
    const char *uri = rdr_uri(rdr);
    return purc_variant_make_string_static(uri, false);
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
        { "status",             status_getter,            NULL },
        { "comm",               comm_getter,            NULL },
        { "uri",                uri_getter,            NULL },
        { "connect",            connect_getter,         NULL },
        { "disconnect",         disconnect_getter,      NULL },
    };

    retv = purc_dvobj_make_from_methods(method, PCA_TABLESIZE(method));
    if (retv == PURC_VARIANT_INVALID) {
        return PURC_VARIANT_INVALID;
    }

    return retv;
}

