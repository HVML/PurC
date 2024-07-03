/*
 * @file runner.c
 * @author Xue Shuming
 * @date 2022/01/04
 * @brief The implementation of $RUNNER dynamic variant object.
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

#define KN_USER_OBJ     "myObj"

static purc_variant_t
user_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    purc_variant_t user_obj = purc_variant_object_get_by_ckey(root,
            KN_USER_OBJ);
    if (user_obj == PURC_VARIANT_INVALID) {
        pcinst_set_error(PURC_ERROR_NOT_DESIRED_ENTITY);
        goto failed;
    }

    if (nr_args < 1) {
        return purc_variant_ref(user_obj);
    }

    const char *keyname;
    keyname = purc_variant_get_string_const(argv[0]);
    if (keyname == NULL) {
        pcinst_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    purc_variant_t var = purc_variant_object_get(user_obj, argv[0]);
    if (var != PURC_VARIANT_INVALID) {
        return purc_variant_ref(var);
    }

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
user_setter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    purc_variant_t user_obj = purc_variant_object_get_by_ckey(root,
            KN_USER_OBJ);
    if (user_obj == PURC_VARIANT_INVALID) {
        pcinst_set_error(PURC_ERROR_NOT_DESIRED_ENTITY);
        goto failed;
    }

    if (nr_args < 2) {
        pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if (!purc_variant_is_string(argv[0])) {
        pcinst_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (purc_variant_is_undefined(argv[1])) {
        if (!purc_variant_object_remove(user_obj, argv[0], false))
            goto failed;
    }
    else {
        if (!purc_variant_object_set(user_obj, argv[0], argv[1]))
            goto failed;
    }

    return purc_variant_make_boolean(true);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
app_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    struct pcinst* inst = pcinst_current();
    return purc_variant_make_string(inst->app_name, false);
}

static purc_variant_t
app_label_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    struct pcinst* inst = pcinst_current();
    purc_variant_t v;
    const char *locale = NULL;
    if (inst->conn_to_rdr && inst->conn_to_rdr->caps) {
        locale = inst->conn_to_rdr->caps->locale;
    }
    v = purc_get_app_label(locale);
    return v ? purc_variant_ref(v) : purc_variant_make_null();
}

static purc_variant_t
runner_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    struct pcinst* inst = pcinst_current();
    return purc_variant_make_string(inst->runner_name, false);
}

static purc_variant_t
runner_label_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    struct pcinst* inst = pcinst_current();
    purc_variant_t v;
    const char *locale = NULL;
    if (inst->conn_to_rdr && inst->conn_to_rdr->caps) {
        locale = inst->conn_to_rdr->caps->locale;
    }
    v = pcinst_get_runner_label(inst->runner_name, locale);
    return v ? purc_variant_ref(v) : purc_variant_make_null();
}

static purc_variant_t
rid_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    struct pcinst* inst = pcinst_current();
    return purc_variant_make_ulongint(inst->endpoint_atom);
}

static purc_variant_t
uri_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    struct pcinst* inst = pcinst_current();
    return purc_variant_make_string(inst->endpoint_name, false);
}

static purc_variant_t
auto_switching_rdr_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    struct pcinst* inst = pcinst_current();
    return purc_variant_make_boolean(inst->auto_switching_rdr);
}

static purc_variant_t
auto_switching_rdr_setter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    struct pcinst* inst = pcinst_current();
    assert(inst);

    if (nr_args < 1) {
        pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if (purc_variant_is_boolean(argv[0])) {
        if (purc_variant_is_true(argv[0])) {
            inst->auto_switching_rdr = 1;
        }
        else {
            inst->auto_switching_rdr = 0;
        }
    }
    else {
        pcinst_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    return purc_variant_make_boolean(inst->auto_switching_rdr);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(inst->auto_switching_rdr);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
chan_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    if (nr_args < 1) {
        pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    const char *chan_name;
    chan_name = purc_variant_get_string_const(argv[0]);
    if (chan_name == NULL) {
        pcinst_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    pcchan_t chan = pcchan_retrieve(chan_name);
    if (chan) {
        return pcchan_make_entity(chan);
    }

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
chan_setter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    uint32_t cap = 1;
    if (nr_args < 1) {
        pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    const char *chan_name;
    chan_name = purc_variant_get_string_const(argv[0]);
    if (chan_name == NULL) {
        pcinst_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (nr_args > 1) {
        if (!purc_variant_cast_to_uint32(argv[1], &cap, true)) {
            pcinst_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }
    }

    PC_DEBUG("chan_setter(%s, %u)\n", chan_name, cap);

    pcchan_t chan = pcchan_retrieve(chan_name);
    if (chan) {
        if (!pcchan_ctrl(chan, cap)) {
            // error set by pcchan_ctrl()
            goto failed;
        }
    }
    else {
        chan = pcchan_open(chan_name, cap);
        if (chan == NULL) {
            // error set by pcchan_open()
            goto failed;
        }
    }

    return purc_variant_make_boolean(true);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

purc_variant_t
purc_dvobj_runner_new(void)
{
    purc_variant_t retv = PURC_VARIANT_INVALID;

    static struct purc_dvobj_method method [] = {
        { "user",               user_getter,            user_setter },
        { "app_name",           app_getter,             NULL },  // TODO: remove
        { "run_name",           runner_getter,          NULL },  // TODO: remove
        { "appName",            app_getter,             NULL },
        { "appLabel",           app_label_getter,       NULL },
        { "runName",            runner_getter,          NULL },
        { "runLabel",           runner_label_getter,    NULL },
        { "rid",                rid_getter,             NULL },
        { "uri",                uri_getter,             NULL },
        { "autoSwitchingRdr",
            auto_switching_rdr_getter, auto_switching_rdr_setter },
        { "chan",               chan_getter,            chan_setter },
#if ENABLE(CHINESE_NAMES)
        { "用户",               user_getter,            user_setter },
        { "应用名",             app_getter,             NULL },
        { "应用标签",           app_label_getter,       NULL },
        { "行者名",             runner_getter,          NULL },
        { "行者标签",           runner_label_getter,    NULL },
        { "行者标识符",         rid_getter,             NULL },
        { "统一资源标识符",     uri_getter,             NULL },
        { "自动切换渲染器",
            auto_switching_rdr_getter, auto_switching_rdr_setter },
        { "通道",               chan_getter,            chan_setter },
#endif
    };

    retv = purc_dvobj_make_from_methods(method, PCA_TABLESIZE(method));
    if (retv == PURC_VARIANT_INVALID) {
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t user_obj = purc_variant_make_object(0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    if (user_obj == PURC_VARIANT_INVALID) {
        purc_variant_unref(retv);
        return PURC_VARIANT_INVALID;
    }

    // TODO: set a pre-listener to avoid remove the user_obj property.
    purc_variant_object_set_by_static_ckey(retv, KN_USER_OBJ, user_obj);
    purc_variant_unref(user_obj);

    return retv;
}

