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

static purc_variant_t
state_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    purc_variant_t vs[10] = { NULL };
    struct pcinst* inst = pcinst_current();
    struct pcrdr_conn *rdr = inst ? inst->conn_to_rdr : NULL;
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

    if (!vs[9]) {
        goto out_clear_vs;
    }

    for (int i = 0; i < 5; i++) {
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
    if (data && nr_args > 0) {
        purc_variant_t v = purc_variant_object_get(data, argv[0]);
        if (v) {
            purc_variant_ref(v);
        }
        purc_variant_unref(data);
        return v;
    }

    return data;
}

static purc_variant_t
stats_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    purc_variant_t ret = PURC_VARIANT_INVALID;
    struct pcinst* inst = pcinst_current();
    if (inst == NULL) {
        purc_set_error(PURC_ERROR_NO_INSTANCE);
        goto failed;
    }

    struct pcrdr_conn *rdr = inst->conn_to_rdr;
    if (rdr == NULL) {
        purc_set_error(PURC_ERROR_NOT_DESIRED_ENTITY);
        goto failed;
    }

    static const char * keys[] = {
        "nrRequestsSent",
        "nrRequestsRecv",
        "nrResponsesSent",
        "nrResponsesRecv",
        "nrEventsSent",
        "nrEventsRecv",
        "bytesSent",
        "bytesRecv",
        "durationSeconds",
    };

    const struct pcrdr_conn_stats *stats = pcrdr_conn_stats(rdr);
    const uint64_t *items = &stats->nr_requests_sent;
    if (nr_args > 0) {
        const char *key = purc_variant_get_string_const(argv[0]);
        if (key == NULL) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        for (size_t i = 0; i < PCA_TABLESIZE(keys); i++) {
            if (strcmp(key, keys[i]) == 0) {
                ret = purc_variant_make_ulongint(items[i]);
                if (ret == PURC_VARIANT_INVALID) {
                    purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
                    goto failed;
                }
                goto done;
            }
        }
    }

    ret = purc_variant_make_object_0();
    if (ret == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    for (size_t i = 0; i < PCA_TABLESIZE(keys); i++) {
        purc_variant_t val = purc_variant_make_ulongint(items[i]);
        if (val) {
            purc_variant_object_set_by_static_ckey(ret, keys[i], val);
            purc_variant_unref(val);
        }
        else {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }
    }

done:
    return ret;

failed:
    if (ret)
        purc_variant_unref(ret);

    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_null();
    return PURC_VARIANT_INVALID;
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
    const char *s_comm;
    const char *s_uri;

    struct pcinst* inst = pcinst_current();
    struct pcrdr_conn *rdr = inst->conn_to_rdr;

    if (nr_args < 2 || !purc_variant_is_string(argv[0])
            || !purc_variant_is_string(argv[1])) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    s_comm = purc_variant_get_string_const(argv[0]);
    s_uri = purc_variant_get_string_const(argv[1]);

    if (rdr) {
        list_del(&inst->conn_to_rdr->ln);
        if (inst->main_conn == inst->conn_to_rdr) {
            inst->main_conn = NULL;
        }
        if (inst->curr_conn == inst->conn_to_rdr) {
            inst->curr_conn = NULL;
        }

        pcrdr_disconnect(inst->conn_to_rdr);
        inst->conn_to_rdr = NULL;
        rdr = NULL;
    }

    if (strcasecmp(s_comm, PURC_RDRCOMM_NAME_HEADLESS) == 0) {
        msg = pcrdr_headless_connect(
            s_uri,
            inst->app_name, inst->runner_name, &inst->conn_to_rdr);
    }
    else if (strcasecmp(s_comm, PURC_RDRCOMM_NAME_SOCKET) == 0) {
        msg = pcrdr_socket_connect(s_uri,
            inst->app_name, inst->runner_name, &inst->conn_to_rdr);
    }
    else if (strcasecmp(s_comm, PURC_RDRCOMM_NAME_THREAD) == 0) {
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

    if (s_uri){
        inst->conn_to_rdr->uri = strdup(s_uri);
    }
    else {
        inst->conn_to_rdr->uri = NULL;
    }

    if (msg->type == PCRDR_MSG_TYPE_RESPONSE && msg->retCode == PCRDR_SC_OK) {
        inst->conn_to_rdr->caps =
            pcrdr_parse_renderer_capabilities(
                    purc_variant_get_string_const(msg->data));
        if (inst->conn_to_rdr->caps == NULL) {
            goto out;
        }
    }
    pcrdr_release_message(msg);

    list_add_tail(&inst->conn_to_rdr->ln, &inst->conns);
    if (!inst->main_conn) {
        inst->main_conn = inst->conn_to_rdr;
    }

    if (!inst->curr_conn) {
        inst->curr_conn = inst->conn_to_rdr;
    }

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
        list_del(&inst->conn_to_rdr->ln);
        if (inst->main_conn == inst->conn_to_rdr) {
            inst->main_conn = NULL;
        }
        if (inst->curr_conn == inst->conn_to_rdr) {
            inst->curr_conn = NULL;
        }
        pcrdr_disconnect(inst->conn_to_rdr);
        inst->conn_to_rdr = NULL;
    }

    return purc_variant_make_boolean(ret);
}

purc_variant_t
purc_dvobj_rdr_new(void)
{
    purc_variant_t retv = PURC_VARIANT_INVALID;

    static struct purc_dvobj_method method [] = {
        { "state",          state_getter,       NULL },
        { "stats",          stats_getter,       NULL },
        { "connect",        connect_getter,     NULL },
        { "disconnect",     disconnect_getter,  NULL },
    };

    retv = purc_dvobj_make_from_methods(method, PCA_TABLESIZE(method));
    if (retv == PURC_VARIANT_INVALID) {
        return PURC_VARIANT_INVALID;
    }

    return retv;
}

