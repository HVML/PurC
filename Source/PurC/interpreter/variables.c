/*
 * @file variables.c
 * @author XueShuming
 * @date 2022/07/05
 * @brief The runner-level predefined variables.
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

#include "internal.h"

#include "private/var-mgr.h"
#include "private/errors.h"
#include "private/instance.h"
#include "private/utils.h"
#include "private/variant.h"

#include <stdlib.h>
#include <string.h>

#define USER_OBJ                "myObj"
#define INNER_WRAP              "__inner_wrap"

struct runner_myobj_wrap {
    purc_variant_t object;
    struct pcvar_listener *listener;
};

static void
post_event(purc_variant_t source, purc_variant_t key, purc_variant_t value)
{
    struct pcinst* inst = pcinst_current();
    const char *k = purc_variant_get_string_const(key);

    purc_variant_t source_uri = purc_variant_make_string(
            inst->endpoint_name, false);
    pcintr_post_event_by_ctype(0, PURC_EVENT_TARGET_BROADCAST,
            PCRDR_MSG_EVENT_REDUCE_OPT_OVERLAY, source_uri,
            source, MSG_TYPE_CHANGE, k, value, PURC_VARIANT_INVALID);
    purc_variant_unref(source_uri);
}

static bool
myobj_grow_handler(purc_variant_t source, pcvar_op_t msg_type, void *ctxt,
        size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(source);
    UNUSED_PARAM(msg_type);
    UNUSED_PARAM(ctxt);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    post_event(source, argv[0], argv[1]);

    return true;
}

static bool
myobj_change_handler(purc_variant_t source, pcvar_op_t msg_type, void *ctxt,
        size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(source);
    UNUSED_PARAM(msg_type);
    UNUSED_PARAM(ctxt);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    post_event(source, argv[0], argv[1]);
    return true;
}

static bool
myobj_shrink_handler(purc_variant_t source, pcvar_op_t msg_type, void *ctxt,
        size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(source);
    UNUSED_PARAM(msg_type);
    UNUSED_PARAM(ctxt);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    post_event(source, argv[0], argv[1]);
    return true;
}

static bool
myobj_handler(purc_variant_t source, pcvar_op_t msg_type, void *ctxt,
        size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(source);
    UNUSED_PARAM(msg_type);
    UNUSED_PARAM(ctxt);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    switch (msg_type) {
    case PCVAR_OPERATION_GROW:
        return myobj_grow_handler(source, msg_type, ctxt, nr_args, argv);

    case PCVAR_OPERATION_SHRINK:
        return myobj_shrink_handler(source, msg_type, ctxt, nr_args, argv);

    case PCVAR_OPERATION_CHANGE:
        return myobj_change_handler(source, msg_type, ctxt, nr_args, argv);

    default:
        return true;
    }
    return true;
}

static void
on_runner_myobj_release(void *native_entity)
{
    struct runner_myobj_wrap *wrap = (struct runner_myobj_wrap*)native_entity;
    if (wrap->listener) {
        purc_variant_revoke_listener(wrap->object, wrap->listener);
    }
    free(wrap);
}

static bool
add_runner_myobj_listener(purc_variant_t runner)
{
    purc_variant_t my_obj = purc_variant_object_get_by_ckey(runner, USER_OBJ);
    int op = PCVAR_OPERATION_GROW | PCVAR_OPERATION_SHRINK |
        PCVAR_OPERATION_CHANGE;
    struct runner_myobj_wrap *wrap = (struct runner_myobj_wrap*)calloc(1,
            sizeof(*wrap));
    if (!wrap) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return false;
    }

    // do not need ref
    wrap->object = my_obj;

    wrap->listener = purc_variant_register_post_listener(my_obj,
        (pcvar_op_t)op, myobj_handler, wrap);

    static struct purc_native_ops ops = {
        .on_release                   = on_runner_myobj_release,
    };

    purc_variant_t v = purc_variant_make_native(wrap, &ops);
    if (v == PURC_VARIANT_INVALID) {
        on_runner_myobj_release(wrap);
        return false;
    }

    bool ret = purc_variant_object_set_by_static_ckey(runner, INNER_WRAP, v);
    purc_variant_unref(v);
    return ret;
}

bool
pcintr_bind_builtin_runner_variables(void)
{
    bool ret = false;
    purc_variant_t runner = PURC_VARIANT_INVALID;

    // $SYS
    purc_variant_t sys = purc_dvobj_system_new();
    if (!purc_bind_runner_variable(PURC_PREDEF_VARNAME_SYS, sys)) {
        goto out;
    }
    purc_variant_unref(sys);

    // $RUNNER
    runner = purc_dvobj_runner_new();
    if (!purc_bind_runner_variable(PURC_PREDEF_VARNAME_RUNNER, runner)) {
        goto out;
    }

#if ENABLE(CHINESE_NAMES) && defined(PURC_PREDEF_VARNAME_RUNNER_ZH)
    if (!purc_bind_runner_variable(PURC_PREDEF_VARNAME_RUNNER_ZH, runner)) {
        goto out;
    }
#endif

    /* $L, $STR, $URL, $DATA, $STREAM, $SOCKET, $DATETIME
     * are all runner-level variables */

    // $L
    purc_variant_t l = purc_dvobj_logical_new();
    if (!purc_bind_runner_variable(PURC_PREDEF_VARNAME_L, l)) {
        goto out;
    }
    purc_variant_unref(l);

    // $STR
    purc_variant_t str = purc_dvobj_string_new();
    if (!purc_bind_runner_variable(PURC_PREDEF_VARNAME_STR, str)) {
        goto out;
    }
    purc_variant_unref(str);

    // $URL
    purc_variant_t url = purc_dvobj_url_new();
    if (!purc_bind_runner_variable(PURC_PREDEF_VARNAME_URL, url)) {
        goto out;
    }
    purc_variant_unref(url);

    // $DATA
    purc_variant_t data = purc_dvobj_data_new();
    if (!purc_bind_runner_variable(PURC_PREDEF_VARNAME_DATA, data)) {
        goto out;
    }
    purc_variant_unref(data);

    // $STREAM
    purc_variant_t stream = purc_dvobj_stream_new();
    if (!purc_bind_runner_variable(PURC_PREDEF_VARNAME_STREAM, stream)) {
        goto out;
    }
    purc_variant_unref(stream);

    // $SOCKET
    purc_variant_t socket = purc_dvobj_socket_new();
    if (!purc_bind_runner_variable(PURC_PREDEF_VARNAME_SOCKET, socket)) {
        goto out;
    }
    purc_variant_unref(socket);

    // $DATETIME
    purc_variant_t dt = purc_dvobj_datetime_new();
    if (!purc_bind_runner_variable(PURC_PREDEF_VARNAME_DATETIME, dt)) {
        goto out;
    }
    purc_variant_unref(dt);

    // $RDR
    purc_variant_t rdr = purc_dvobj_rdr_new();
    if (!purc_bind_runner_variable(PURC_PREDEF_VARNAME_RDR, rdr)) {
        goto out;
    }
    purc_variant_unref(rdr);

    ret = add_runner_myobj_listener(runner);

out:
    if (runner) {
        purc_variant_unref(runner);
    }
    return ret;
}

