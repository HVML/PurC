/*
 * @file hvml.c
 * @author Geng Yue, Vincent Wei
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

#define DEFAULT_HVML_BASE           "file:///"
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
base_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, bool silently)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(silently);

#if 0 // VWNOTE: redundant check
    if (root == PURC_VARIANT_INVALID) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }
    if (!purc_variant_is_object (root)) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
#endif

    struct purc_hvml_ctrl_props *ctrl_props = hvml_ctrl_props(root);
    assert(ctrl_props);

    return purc_variant_make_string_static(ctrl_props->base_url_string, false);
}

static purc_variant_t
base_setter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, bool silently)
{
#if 0 // VWNOTE: redundant check
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
#endif

    const char *url;
    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if ((url = purc_variant_get_string_const(argv[0])) == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    struct purc_hvml_ctrl_props *ctrl_props = hvml_ctrl_props(root);
    assert(ctrl_props);

    /*
     * If the url is invalid, ctrl_props->base_url_broken_down should not
     * be changed. If the url is valid, perhaps the string returned
     * by pcutils_url_assemble() is not identical to the input string.
     *
     * For example:
     *
     * The input string is `http://www.minigui.org`, but the the output string
     * of pcutils_url_assemble() is `http://www.minigui.org/`
     */
    if (pcutils_url_break_down(&(ctrl_props->base_url_broken_down), url)) {
        char *url = pcutils_url_assemble(&ctrl_props->base_url_broken_down);
        if (url) {
            free(ctrl_props->base_url_string);
            ctrl_props->base_url_string = url;
            return purc_variant_make_string_static(url, false);
        }
        else {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }
    }
    else {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

failed:
    if (silently)
        return purc_variant_make_boolean(false);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
max_iteration_count_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, bool silently)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(silently);

#if 0 // VWNOTE: redundant check
    if (root == PURC_VARIANT_INVALID) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }
    if (!purc_variant_is_object (root)) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
#endif

    struct purc_hvml_ctrl_props *ctrl_props = hvml_ctrl_props(root);
    assert(ctrl_props);

    return purc_variant_make_ulongint(ctrl_props->max_iteration_count);
}

static purc_variant_t
max_iteration_count_setter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, bool silently)
{
    UNUSED_PARAM(silently);

#if 0 // VWNOTE: redundant check
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
#endif

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    uint64_t u64;
    if (purc_variant_cast_to_ulongint(argv[0], &u64, false) && u64 > 0) {
        struct purc_hvml_ctrl_props *ctrl_props = hvml_ctrl_props(root);
        assert(ctrl_props);

        ctrl_props->max_iteration_count = u64;
        return purc_variant_make_ulongint(u64);
    }
    else {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
    }

failed:
    if (silently)
        return purc_variant_make_boolean(false);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
max_recursion_depth_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, bool silently)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(silently);

#if 0 // VWNOTE: redundant check
    if (root == PURC_VARIANT_INVALID) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }
    if (!purc_variant_is_object (root)) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
#endif

    struct purc_hvml_ctrl_props *ctrl_props = hvml_ctrl_props(root);
    assert(ctrl_props);

    return purc_variant_make_ulongint(ctrl_props->max_recursion_depth);
}

static purc_variant_t
max_recursion_depth_setter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, bool silently)
{
#if 0 // VWNOTE: redundant check
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
#endif

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    uint64_t u64;
    if (purc_variant_cast_to_ulongint(argv[0], &u64, false) &&
            u64 > 0 && u64 <= USHRT_MAX) {
        struct purc_hvml_ctrl_props *ctrl_props = hvml_ctrl_props(root);
        assert(ctrl_props);

        ctrl_props->max_recursion_depth = u64;
        return purc_variant_make_ulongint(u64);
    }

    purc_set_error(PURC_ERROR_INVALID_VALUE);

failed:
    if (silently)
        return purc_variant_make_boolean(false);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
max_embedded_levels_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, bool silently)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(silently);

    struct purc_hvml_ctrl_props *ctrl_props = hvml_ctrl_props(root);
    assert(ctrl_props);

    return purc_variant_make_ulongint(ctrl_props->max_embedded_levels);
}

static purc_variant_t
max_embedded_levels_setter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, bool silently)
{
    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    uint64_t u64;
    if (purc_variant_cast_to_ulongint(argv[0], &u64, false) &&
            u64 > 0 && u64 <= MAX_EMBEDDED_LEVELS) {
        struct purc_hvml_ctrl_props *ctrl_props = hvml_ctrl_props(root);
        assert(ctrl_props);

        ctrl_props->max_embedded_levels = u64;
        return purc_variant_make_ulongint(u64);
    }
    else {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
    }

failed:
    if (silently)
        return purc_variant_make_boolean(false);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
timeout_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, bool silently)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(silently);

#if 0 // VWNOTE: redundant check
    if (root == PURC_VARIANT_INVALID) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }
    if (!purc_variant_is_object (root)) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
#endif

    struct purc_hvml_ctrl_props *ctrl_props = hvml_ctrl_props(root);
    assert(ctrl_props);

    double number = (double)ctrl_props->timeout.tv_sec +
                (double)ctrl_props->timeout.tv_nsec / 1000000000.0;
    return purc_variant_make_number(number);
}

static purc_variant_t
timeout_setter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, bool silently)
{
    UNUSED_PARAM(silently);

#if 0 // VWNOTE: redundant check
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
#endif

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    double number = 0.0;
    if (purc_variant_cast_to_number(argv[0], &number, false)) {
        struct purc_hvml_ctrl_props *ctrl_props = hvml_ctrl_props(root);
        assert(ctrl_props);

        if (number > 0.0) {
            ctrl_props->timeout.tv_sec = (long)number;
            ctrl_props->timeout.tv_nsec = (long)
                ((number - ctrl_props->timeout.tv_sec) * 1000000000);

            return purc_variant_make_number(number);
        }
    }

    purc_set_error(PURC_ERROR_INVALID_VALUE);

failed:
    if (silently)
        return purc_variant_make_boolean(false);
    return PURC_VARIANT_INVALID;
}

static void
on_release(void* native_entity)
{
    struct purc_hvml_ctrl_props *ctrl_props;

    assert(native_entity);

    ctrl_props = (struct purc_hvml_ctrl_props*)native_entity;
    struct purc_broken_down_url *url = &ctrl_props->base_url_broken_down;

    if (url->schema)
        free(url->schema);

    if (url->user)
        free(url->user);

    if (url->passwd)
        free(url->passwd);

    if (url->host)
        free(url->host);

    if (url->path)
        free(url->path);

    if (url->query)
        free(url->query);

    if (url->fragment)
        free(url->fragment);

    if (ctrl_props->base_url_string)
        free(ctrl_props->base_url_string);

    free(native_entity);
}

purc_variant_t
purc_dvobj_hvml_new(const struct purc_hvml_ctrl_props **ctrl_props)
{
    purc_variant_t retv = PURC_VARIANT_INVALID;
    purc_variant_t val = PURC_VARIANT_INVALID;
    struct purc_hvml_ctrl_props *my_props = NULL;

    static const struct purc_native_ops ops = {
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

    static const struct purc_dvobj_method method [] = {
        { "base", base_getter, base_setter },
        { "max_iteration_count",
            max_iteration_count_getter, max_iteration_count_setter },
        { "max_recursion_depth",
            max_recursion_depth_getter, max_recursion_depth_setter },
        { "max_embedded_levels",
            max_embedded_levels_getter, max_embedded_levels_setter },
        { "timeout", timeout_getter, timeout_setter },
    };

    retv = purc_dvobj_make_from_methods(method, PCA_TABLESIZE(method));
    if (retv == PURC_VARIANT_INVALID) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    my_props = calloc(1, sizeof(struct purc_hvml_ctrl_props));
    if (my_props == NULL) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    my_props->base_url_string = strdup(DEFAULT_HVML_BASE);
    if (my_props->base_url_string == NULL ||
            !pcutils_url_break_down(&my_props->base_url_broken_down,
                DEFAULT_HVML_BASE)) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    my_props->max_iteration_count = UINT64_MAX;
    my_props->max_recursion_depth = UINT16_MAX;
    my_props->max_embedded_levels = DEF_EMBEDDED_LEVELS;
    my_props->timeout.tv_sec = DEFAULT_HVML_TIMEOUT_SEC;
    my_props->timeout.tv_nsec = DEFAULT_HVML_TIMEOUT_NSEC;

    val = purc_variant_make_native((void *)my_props, &ops);
    if (val == PURC_VARIANT_INVALID) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    if (!purc_variant_object_set_by_static_ckey(retv,
                DVOBJ_HVML_DATA_NAME, val)) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }
    purc_variant_unref(val);

    if (ctrl_props)
        *ctrl_props = my_props;
    return retv;

failed:
    if (val)
        purc_variant_unref(val);
    if (retv)
        purc_variant_unref(retv);
    if (my_props)
        on_release(my_props);

    return PURC_VARIANT_INVALID;
}
