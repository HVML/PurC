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

    struct pcrdr_conn *curr_conn = inst->curr_conn;
    if (curr_conn && curr_conn->caps) {
        locale = curr_conn->caps->locale;
    }
    else if (inst->conn_to_rdr && inst->conn_to_rdr->caps) {
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

    struct pcrdr_conn *curr_conn = inst->curr_conn;
    if (curr_conn && curr_conn->caps) {
        locale = curr_conn->caps->locale;
    }
    else if (inst->conn_to_rdr && inst->conn_to_rdr->caps) {
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

#if OS(UNIX)
#   define TEMP_CHAN_PATH           "/tmp/"
#   define TEMP_CHAN_PREEFIX        "_htc"
#   define TEMP_CHAN_TEMPLATE_FILE  TEMP_CHAN_PREEFIX "XXXXXX"
#   define TEMP_CHAN_TEMPLATE_PATH  TEMP_CHAN_PATH TEMP_CHAN_TEMPLATE_FILE
#else
#endif

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

        if (cap == 0 && strncmp(chan_name, TEMP_CHAN_PREEFIX,
                    sizeof(TEMP_CHAN_PREEFIX) - 1) == 0 &&
                strlen(chan_name) == sizeof(TEMP_CHAN_TEMPLATE_FILE) - 1) {
            /* this is a temporary channel */
            char temp_chan_path[] = TEMP_CHAN_TEMPLATE_PATH;
            strcpy(temp_chan_path + sizeof(TEMP_CHAN_PATH) - 1,
                    chan_name);
            if (access(temp_chan_path, F_OK) == 0) {
                remove(temp_chan_path);
            }
            else {
                PC_WARN("The corresponding file for temporary channel dose not"
                        "exist: %s\n", strerror(errno));
            }
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

static purc_variant_t
mktempchan_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    uint32_t cap = 1;
    if (nr_args > 0) {
        if (!purc_variant_cast_to_uint32(argv[0], &cap, false)) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        if (cap == 0) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }
    }

    char temp_chan[] = TEMP_CHAN_TEMPLATE_PATH;
    int fd;
    if ((fd = mkstemp(temp_chan)) < 0) {
        purc_set_error(purc_error_from_errno(errno));
        goto failed;
    }

    close(fd);

    const char *chan_name = temp_chan + sizeof(TEMP_CHAN_PATH) - 1;
    pcchan_t chan = pcchan_retrieve(chan_name);

    PC_DEBUG("%s: %s, %s\n", __func__, temp_chan, chan_name);

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

    return purc_variant_make_string(chan_name, false);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
duplicate_renderers_getter(purc_variant_t root, size_t nr_args,
        purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    purc_variant_t arr = purc_variant_make_array_0();
    if (!arr) {
        goto failed;
    }

    struct pcinst *inst = pcinst_current();
    struct list_head *conns = &inst->conns;
    struct pcrdr_conn *pconn, *qconn;
    list_for_each_entry_safe(pconn, qconn, conns, ln) {
        purc_variant_t v = pcrdr_data(pconn);
        if (v) {
            purc_variant_array_append(arr, v);
            purc_variant_unref(v);
        }
    }

    size_t nr_conn = purc_variant_array_get_size(arr);
    purc_variant_t tup = purc_variant_make_tuple(nr_conn, NULL);
    if (!tup) {
        goto failed;
    }

    for (size_t i = 0; i < nr_conn; i++) {
        purc_variant_tuple_set(tup, i, purc_variant_array_get(arr, i));
    }

    purc_variant_unref(arr);

    return tup;
failed:
    if (arr) {
        purc_variant_unref(arr);
    }

    if (call_flags & PCVRT_CALL_FLAG_SILENTLY) {
        return purc_variant_make_undefined();
    }

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
conn_renderer_getter(purc_variant_t root, size_t nr_args,
        purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(root);

    purc_instance_extra_info extra_info = {0};
    const char *id;
    const char *s_comm;
    const char *s_uri;

    if (nr_args < 2 || !purc_variant_is_string(argv[0])
            || !purc_variant_is_string(argv[1])) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    s_comm = purc_variant_get_string_const(argv[0]);
    if (strcasecmp(s_comm, PURC_RDRCOMM_NAME_HEADLESS) == 0) {
        extra_info.renderer_comm = PURC_RDRCOMM_HEADLESS;
    }
    else if (strcasecmp(s_comm, PURC_RDRCOMM_NAME_SOCKET) == 0) {
        extra_info.renderer_comm = PURC_RDRCOMM_SOCKET;
    }
    else if (strcasecmp(s_comm, PURC_RDRCOMM_NAME_THREAD) == 0) {
        extra_info.renderer_comm = PURC_RDRCOMM_THREAD;
    }
    /* XXX: Removed since 0.9.22
    else if (strcasecmp(s_comm, PURC_RDRCOMM_NAME_WEBSOCKET) == 0) {
        extra_info.renderer_comm = PURC_RDRCOMM_WEBSOCKET;
    } */
    else {
        // TODO: other protocol
        purc_set_error(PURC_ERROR_NOT_SUPPORTED);
        goto failed;
    }

    s_uri = purc_variant_get_string_const(argv[1]);
    extra_info.renderer_uri = s_uri;

    id = purc_connect_to_renderer(&extra_info);
    if (id) {
        return purc_variant_make_string_static(id, false);
    }
    return purc_variant_make_undefined();

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY) {
        return purc_variant_make_undefined();
    }

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
disconn_renderer_getter(purc_variant_t root, size_t nr_args,
        purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(call_flags);

    bool ret = false;

    if (nr_args < 1) {
        pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    const char *id;
    id = purc_variant_get_string_const(argv[0]);
    if (id == NULL) {
        pcinst_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    int r = purc_disconnect_from_renderer(id);
    ret = (r == 0);

failed:
    return purc_variant_make_boolean(ret);
}

static struct pcdvobjs_option_to_atom enablelog_levels_skws[] = {
    { "all",        0,  PURC_LOG_MASK_ALL },
    { "default",    0,  PURC_LOG_MASK_DEFAULT },
};

static struct pcdvobjs_option_to_atom enablelog_levels_ckws[] = {
    { "emerg",      0, PURC_LOG_MASK_EMERG },
    { "alert",      0, PURC_LOG_MASK_ALERT },
    { "crit",       0, PURC_LOG_MASK_CRIT },
    { "error",      0, PURC_LOG_MASK_ERR},
    { "warning",    0, PURC_LOG_MASK_WARNING },
    { "notice",     0, PURC_LOG_MASK_NOTICE },
    { "info",       0, PURC_LOG_MASK_INFO },
    { "debug",      0, PURC_LOG_MASK_DEBUG },
};

static struct pcdvobjs_option_to_atom enablelog_level_skws[] = {
    { "emerg",      0, PURC_LOG_EMERG },
    { "alert",      0, PURC_LOG_ALERT },
    { "crit",       0, PURC_LOG_CRIT },
    { "error",      0, PURC_LOG_ERR},
    { "warning",    0, PURC_LOG_WARNING },
    { "notice",     0, PURC_LOG_NOTICE },
    { "info",       0, PURC_LOG_INFO },
    { "debug",      0, PURC_LOG_DEBUG },
};

static struct pcdvobjs_option_to_atom enablelog_facility_skws[] = {
    { "stdout",     0, PURC_LOG_FACILITY_STDOUT },
    { "stderr",     0, PURC_LOG_FACILITY_STDERR },
    { "syslog",     0, PURC_LOG_FACILITY_SYSLOG },
    { "file",       0, PURC_LOG_FACILITY_FILE },
};

static purc_variant_t
enablelog_getter(purc_variant_t root, size_t nr_args,
        purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(call_flags);

    if (nr_args < 1) {
        pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    int levels = pcdvobjs_parse_options(argv[0],
            enablelog_levels_skws, PCA_TABLESIZE(enablelog_levels_skws),
            enablelog_levels_ckws, PCA_TABLESIZE(enablelog_levels_ckws),
            0, 0);
    if (levels == 0) {
        /* error will be set by pcdvobjs_parse_options() */
        goto failed;
    }

    int facility = pcdvobjs_parse_options(
            (nr_args > 1) ? argv[1] : PURC_VARIANT_INVALID,
            enablelog_facility_skws, PCA_TABLESIZE(enablelog_facility_skws),
            NULL, 0, PURC_LOG_FACILITY_STDOUT, -1);
    if (facility == -1) {
        /* error will be set by pcdvobjs_parse_options() */
        goto failed;
    }

    return purc_variant_make_boolean(purc_enable_log_ex(levels, facility));

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY) {
        return purc_variant_make_boolean(false);
    }

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
logmsg_getter(purc_variant_t root, size_t nr_args,
        purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(call_flags);

    if (nr_args < 1) {
        pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    const char *msg;
    if ((msg = purc_variant_get_string_const(argv[0])) == NULL) {
        pcinst_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    int level = pcdvobjs_parse_options(
            (nr_args > 1) ? argv[1] : PURC_VARIANT_INVALID,
            enablelog_level_skws, PCA_TABLESIZE(enablelog_level_skws),
            NULL, 0, PURC_LOG_INFO, -1);
    if (level == -1) {
        /* error will be set by pcdvobjs_parse_options() */
        goto failed;
    }

    const char *tag = NULL;
    if (nr_args > 2) {
        tag = purc_variant_get_string_const(argv[2]);
    }

    purc_log_with_tag_f(level, tag, "%s\n", msg);
    return purc_variant_make_boolean(true);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY) {
        return purc_variant_make_boolean(false);
    }

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
        { "mktempchan",         mktempchan_getter,      NULL },
        { "duplicateRenderers", duplicate_renderers_getter, NULL },
        { "connRenderer",       conn_renderer_getter,   NULL },
        { "disconnRenderer",    disconn_renderer_getter,NULL },
        { "enablelog",          enablelog_getter,       NULL },
        { "logmsg",             logmsg_getter,          NULL },
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
        { "复制渲染器",         duplicate_renderers_getter, NULL },
        { "连接渲染器",         conn_renderer_getter,   NULL },
        { "断开渲染器",         disconn_renderer_getter, NULL },
#endif
    };

    static struct dvobjs_option_set {
        struct pcdvobjs_option_to_atom *opts;
        size_t sz;
    } opts_set[] = {
        { enablelog_levels_skws,    PCA_TABLESIZE(enablelog_levels_skws) },
        { enablelog_levels_ckws,    PCA_TABLESIZE(enablelog_levels_ckws) },
        { enablelog_level_skws,     PCA_TABLESIZE(enablelog_level_skws) },
        { enablelog_facility_skws,  PCA_TABLESIZE(enablelog_facility_skws) },
    };

    for (size_t i = 0; i < PCA_TABLESIZE(opts_set); i++) {
        struct pcdvobjs_option_to_atom *opts = opts_set[i].opts;
        if (opts[0].atom == 0) {
            for (size_t j = 0; j < opts_set[i].sz; j++) {
                opts[j].atom = purc_atom_from_static_string_ex(
                        ATOM_BUCKET_DVOBJ, opts[j].option);
            }
        }
    }

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

