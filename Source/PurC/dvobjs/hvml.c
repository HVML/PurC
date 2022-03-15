/*
 * @file hvml.c
 * @author Geng Yue
 * @date 2021/07/02
 * @brief The implementation of HVML dynamic variant object.
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
#include "purc-variant.h"
#include "helper.h"

#include <limits.h>

#define DEFAULT_HVML_BASE           ""
#define DEFAULT_HVML_TIMEOUT        10.0
#define DVOBJ_HVML_DATA_NAME        "__handle_ctrl_props"

#define DEFAULT_HVML_TIMEOUT_SEC    (time_t)DEFAULT_HVML_TIMEOUT
#define DEFAULT_HVML_TIMEOUT_NSEC   (long)((DEFAULT_HVML_TIMEOUT -  \
            DEFAULT_HVML_TIMEOUT_SEC) * 1000000000)

static inline struct purc_hvml_ctrl_props *
hvml_ctrl_props(purc_variant_t root)
{
    purc_variant_t var;

    var = purc_variant_object_get_by_ckey(root, DVOBJ_HVML_DATA_NAME, false);
    assert(var && purc_variant_is_native(var));

    return (struct purc_hvml_ctrl_props *)purc_variant_native_get_entity(var);
}

static purc_variant_t
base_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(silently);

    if (root == PURC_VARIANT_INVALID) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }
    if (!purc_variant_is_object (root)) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t retv = PURC_VARIANT_INVALID;

    struct purc_hvml_ctrl_props *ctrl_props = hvml_ctrl_props(root);

    if (ctrl_props) {
        char *url = pcdvobjs_get_url (&(ctrl_props->base_url));
        if (url)
            retv = purc_variant_make_string_reuse_buff (
                    url, strlen (url), false);
    }

    return retv;
}

static purc_variant_t
base_setter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(silently);

    if ((root == PURC_VARIANT_INVALID) || (argv == NULL) || (nr_args < 1)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }
    if (!purc_variant_is_object (root)) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
    if (!purc_variant_is_string (argv[0])) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t retv = PURC_VARIANT_INVALID;

    const char *url = purc_variant_get_string_const (argv[0]);

    struct purc_hvml_ctrl_props *ctrl_props = hvml_ctrl_props(root);
    if (ctrl_props) {
        /* If the url is invlid, ctrl_props->base_url will not be changed.
         If the url is valid, perhaps the string which pcdvobjs_get_url() returned
         is not identical to input string. For example:
         input string to pcdvobjs_set_url is:  http://www.minigui.org
         output string of pcdvobjs_get_url is: http://www.minigui.org/
         */
        pcdvobjs_set_url (&(ctrl_props->base_url), url);

        char *url = pcdvobjs_get_url (&(ctrl_props->base_url));
        if (url) {
            retv = purc_variant_make_string_reuse_buff (
                    url, strlen (url), false);
        }
    }
    return retv;
}

static purc_variant_t
max_iteration_count_getter (
        purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(silently);

    if (root == PURC_VARIANT_INVALID) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }
    if (!purc_variant_is_object (root)) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t retv = PURC_VARIANT_INVALID;

    struct purc_hvml_ctrl_props *ctrl_props = hvml_ctrl_props(root);
    if (ctrl_props) {
        retv = purc_variant_make_ulongint (ctrl_props->max_iteration_count);
    }

    return retv;
}

static purc_variant_t
max_iteration_count_setter (
        purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(silently);

    if ((root == PURC_VARIANT_INVALID) || (argv == NULL) || (nr_args < 1)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }
    if (!purc_variant_is_object (root)) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
    if (!purc_variant_is_ulongint (argv[0])) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t retv = PURC_VARIANT_INVALID;
    uint64_t u64;
    purc_variant_cast_to_ulongint (argv[0], &u64, false);

    struct purc_hvml_ctrl_props *ctrl_props = hvml_ctrl_props(root);
    if (ctrl_props) {
        ctrl_props->max_iteration_count = u64;
        retv = purc_variant_make_ulongint (u64);
    }

    return retv;
}

static purc_variant_t
max_recursion_depth_getter (
        purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(silently);

    if (root == PURC_VARIANT_INVALID) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }
    if (!purc_variant_is_object (root)) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t retv = PURC_VARIANT_INVALID;
    struct purc_hvml_ctrl_props *ctrl_props = hvml_ctrl_props(root);
    if (ctrl_props) {
        retv = purc_variant_make_ulongint (ctrl_props->max_recursion_depth);
    }

    return retv;
}

static purc_variant_t
max_recursion_depth_setter (
        purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(silently);

    if ((root == PURC_VARIANT_INVALID) || (argv == NULL) || (nr_args < 1)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }
    if (!purc_variant_is_object (root)) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
    if (!purc_variant_is_ulongint (argv[0])) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t retv = PURC_VARIANT_INVALID;
    uint64_t u64;
    purc_variant_cast_to_ulongint (argv[0], &u64, false);

    struct purc_hvml_ctrl_props *ctrl_props = hvml_ctrl_props(root);
    if (ctrl_props) {
        ctrl_props->max_recursion_depth = u64;
        retv = purc_variant_make_ulongint (u64);
    }

    return retv;
}

static purc_variant_t
timeout_getter (
        purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(silently);

    if (root == PURC_VARIANT_INVALID) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }
    if (!purc_variant_is_object (root)) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t retv = PURC_VARIANT_INVALID;

    struct purc_hvml_ctrl_props *ctrl_props = hvml_ctrl_props(root);
    if (ctrl_props) {
        double number = (double)ctrl_props->timeout.tv_sec +
                (double)ctrl_props->timeout.tv_nsec / 1000000000.0;
        retv = purc_variant_make_number (number);
    }

    return retv;
}

static purc_variant_t
timeout_setter (
        purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(silently);

    if ((root == PURC_VARIANT_INVALID) || (argv == NULL) || (nr_args < 1)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }
    if (!purc_variant_is_object (root)) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
    if (!purc_variant_is_number (argv[0])) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t retv = PURC_VARIANT_INVALID;
    double number = 0.0;
    purc_variant_cast_to_number (argv[0], &number, false);

    struct purc_hvml_ctrl_props *ctrl_props = hvml_ctrl_props(root);
    if (ctrl_props) {
        if (number > 0) {
            ctrl_props->timeout.tv_sec = (long) number;
            ctrl_props->timeout.tv_nsec = (long)
                ((number - ctrl_props->timeout.tv_sec) * 1000000000);
        }
        else
            number = (double)ctrl_props->timeout.tv_sec +
                (double)ctrl_props->timeout.tv_nsec / 1000000000;

        retv = purc_variant_make_number (number);
    }

    return retv;
}

static void
on_release(void* native_entity)
{
    PC_ASSERT(native_entity);

    struct purc_hvml_ctrl_props *hvml = (struct purc_hvml_ctrl_props*)native_entity;
    struct purc_broken_down_url *url = &hvml->base_url;

    if (url->schema)
        free (url->schema);

    if (url->user)
        free (url->user);

    if (url->passwd)
        free (url->passwd);

    if (url->host)
        free (url->host);

    if (url->path)
        free (url->path);

    if (url->query)
        free (url->query);

    if (url->fragment)
        free (url->fragment);

    free (native_entity);
}

purc_variant_t
purc_dvobj_hvml_new(struct purc_hvml_ctrl_props **ctrl_props)
{
    purc_variant_t retv = PURC_VARIANT_INVALID;

    struct purc_hvml_ctrl_props *my_props = NULL;
    static struct purc_native_ops ops = {
        .property_getter        = NULL,
        .property_setter        = NULL,
        .property_eraser        = NULL,
        .property_cleaner       = NULL,

        .updater                = NULL,
        .cleaner                = NULL,
        .eraser                 = NULL,

        .on_observe            = NULL,
        .on_release            = on_release,
    };

    static struct purc_dvobj_method method [] = {
        { "base", base_getter, base_setter },
        { "max_iteration_count",
            max_iteration_count_getter, max_iteration_count_setter },
        { "max_recursion_depth",
            max_recursion_depth_getter, max_recursion_depth_setter },
        { "timeout", timeout_getter, timeout_setter },
    };

    retv = purc_dvobj_make_from_methods(method, PCA_TABLESIZE(method));
    if (retv == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    // initialize purc_hvml_ctrl_props
    my_props = calloc(1, sizeof(struct purc_hvml_ctrl_props));
    if (my_props == NULL) {
        purc_variant_unref (retv);
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    pcdvobjs_set_url (&(my_props->base_url), DEFAULT_HVML_BASE);

    my_props->max_iteration_count = ULONG_MAX;
    my_props->max_recursion_depth = USHRT_MAX;
    my_props->max_embedded_levels = MAX_EMBEDDED_LEVELS;
    my_props->timeout.tv_sec = DEFAULT_HVML_TIMEOUT_SEC;
    my_props->timeout.tv_nsec = DEFAULT_HVML_TIMEOUT_NSEC;

    purc_variant_t val = purc_variant_make_native((void *)my_props, &ops);
    if (val == PURC_VARIANT_INVALID) {
        purc_variant_unref (val);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_object_set_by_static_ckey(retv, DVOBJ_HVML_DATA_NAME, val);
    purc_variant_unref(val);

    if (ctrl_props)
        *ctrl_props = my_props;
    return retv;
}
