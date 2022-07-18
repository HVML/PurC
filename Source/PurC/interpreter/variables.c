/*
 * @file variables.c
 * @author XueShuming
 * @date 2022/07/05
 * @brief The session predefined variables.
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

#define BUILTIN_VAR_SYS         PURC_PREDEF_VARNAME_SYS
#define BUILTIN_VAR_SESSION     "SESSION"

#define USER_OBJ                "myObj"
#define INNER_WRAP              "__inner_wrap"
#define MSG_TYPE_CHANGE         "change"

struct session_myobj_wrap {
    purc_variant_t object;
    struct pcvar_listener *listener;
};

void
post_event(purc_variant_t source, purc_variant_t key, purc_variant_t value)
{
    struct pcinst* inst = pcinst_current();
    const char *k = purc_variant_get_string_const(key);

    purc_variant_t source_uri = purc_variant_make_string(
            inst->endpoint_name, false);
    pcintr_post_event_by_ctype(PURC_EVENT_TARGET_BROADCAST,
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

void
on_session_myobj_release(void *native_entity)
{
    struct session_myobj_wrap *wrap = (struct session_myobj_wrap*)native_entity;
    if (wrap->listener) {
        purc_variant_revoke_listener(wrap->object, wrap->listener);
    }
    free(wrap);
}

bool
add_session_myobj_listener(purc_variant_t session)
{
    purc_variant_t my_obj = purc_variant_object_get_by_ckey(session, USER_OBJ);
    int op = PCVAR_OPERATION_GROW | PCVAR_OPERATION_SHRINK |
        PCVAR_OPERATION_CHANGE;
    struct session_myobj_wrap *wrap = (struct session_myobj_wrap*)calloc(1,
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
        .on_release                   = on_session_myobj_release,
    };

    purc_variant_t v = purc_variant_make_native(wrap, &ops);
    if (v == PURC_VARIANT_INVALID) {
        on_session_myobj_release(wrap);
        return false;
    }

    bool ret = purc_variant_object_set_by_static_ckey(session, INNER_WRAP, v);
    purc_variant_unref(v);
    return ret;
}

bool
purc_bind_session_variables(void)
{
    // $SYS
    purc_variant_t sys = purc_dvobj_system_new();
    if(!purc_bind_variable(BUILTIN_VAR_SYS, sys)) {
        return false;
    }
    purc_variant_unref(sys);

    // $SESSION
    purc_variant_t session = purc_dvobj_session_new();
    if(!purc_bind_variable(BUILTIN_VAR_SESSION, session)) {
        return false;
    }

    bool ret = add_session_myobj_listener(session);
    purc_variant_unref(session);
    return ret;
}
