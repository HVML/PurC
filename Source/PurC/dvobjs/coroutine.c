/*
 * @file coroutine.c
 * @author Geng Yue, Vincent Wei
 * @date 2021/07/02
 * @brief The implementation of CRTN dynamic variant object.
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
#define DEFAULT_HVML_TARGET         "void"
#define DEFAULT_HVML_TIMEOUT        10.0
#define DVOBJ_HVML_DATA_NAME        "__handle_ctrl_props"

#define DEFAULT_HVML_TIMEOUT_SEC    (time_t)DEFAULT_HVML_TIMEOUT
#define DEFAULT_HVML_TIMEOUT_NSEC   (long)((DEFAULT_HVML_TIMEOUT -  \
            DEFAULT_HVML_TIMEOUT_SEC) * 1000000000)

static inline pcintr_coroutine_t
hvml_ctrl_coroutine(purc_variant_t root)
{
    purc_variant_t var;

    var = purc_variant_object_get_by_ckey(root, DVOBJ_HVML_DATA_NAME);
    assert(var && purc_variant_is_native(var));

    return (pcintr_coroutine_t)purc_variant_native_get_entity(var);
}

static purc_variant_t
target_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(silently);

    pcintr_coroutine_t cor = hvml_ctrl_coroutine(root);
    assert(cor);

    return purc_variant_make_string_static(cor->target, false);
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

    pcintr_coroutine_t cor = hvml_ctrl_coroutine(root);
    assert(cor);

    return purc_variant_make_string_static(cor->base_url_string, false);
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

    pcintr_coroutine_t cor = hvml_ctrl_coroutine(root);
    assert(cor);

    /*
     * If the url is invalid, cor->base_url_broken_down should not
     * be changed. If the url is valid, perhaps the string returned
     * by pcutils_url_assemble() is not identical to the input string.
     *
     * For example:
     *
     * The input string is `http://www.minigui.org`, but the the output string
     * of pcutils_url_assemble() is `http://www.minigui.org/`
     */
    if (pcutils_url_break_down(&(cor->base_url_broken_down), url)) {
        char *url = pcutils_url_assemble(&cor->base_url_broken_down);
        if (url) {
            free(cor->base_url_string);
            cor->base_url_string = url;
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

    pcintr_coroutine_t cor = hvml_ctrl_coroutine(root);
    assert(cor);

    return purc_variant_make_ulongint(cor->max_iteration_count);
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
        pcintr_coroutine_t cor = hvml_ctrl_coroutine(root);
        assert(cor);

        cor->max_iteration_count = u64;
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

    pcintr_coroutine_t cor = hvml_ctrl_coroutine(root);
    assert(cor);

    return purc_variant_make_ulongint(cor->max_recursion_depth);
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
        pcintr_coroutine_t cor = hvml_ctrl_coroutine(root);
        assert(cor);

        cor->max_recursion_depth = u64;
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

    pcintr_coroutine_t cor = hvml_ctrl_coroutine(root);
    assert(cor);

    return purc_variant_make_ulongint(cor->max_embedded_levels);
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
        pcintr_coroutine_t cor = hvml_ctrl_coroutine(root);
        assert(cor);

        cor->max_embedded_levels = u64;
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

    pcintr_coroutine_t cor = hvml_ctrl_coroutine(root);
    assert(cor);

    double number = (double)cor->timeout.tv_sec +
                (double)cor->timeout.tv_nsec / 1000000000.0;
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
        pcintr_coroutine_t cor = hvml_ctrl_coroutine(root);
        assert(cor);

        if (number > 0.0) {
            cor->timeout.tv_sec = (long)number;
            cor->timeout.tv_nsec = (long)
                ((number - cor->timeout.tv_sec) * 1000000000);

            return purc_variant_make_number(number);
        }
    }

    purc_set_error(PURC_ERROR_INVALID_VALUE);

failed:
    if (silently)
        return purc_variant_make_boolean(false);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
cid_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(silently);

    pcintr_coroutine_t cor = hvml_ctrl_coroutine(root);
    return purc_variant_make_ulongint(cor->cid);
}

static purc_variant_t
uri_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(silently);

    pcintr_coroutine_t cor = hvml_ctrl_coroutine(root);
    const char *uri = pcintr_coroutine_get_uri(cor);
    return purc_variant_make_string(uri, false);
}

static purc_variant_t
token_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(silently);

    pcintr_coroutine_t cor = hvml_ctrl_coroutine(root);
    const char *uri = pcintr_coroutine_get_uri(cor);
    const char *token = pcutils_basename(uri);
    if (token) {
        return purc_variant_make_string(token, false);
    }
    return purc_variant_make_string(uri, false);
}

static purc_variant_t
curator_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(silently);

    pcintr_coroutine_t cor = hvml_ctrl_coroutine(root);
    return purc_variant_make_ulongint(cor->curator);
}

purc_variant_t
purc_dvobj_coroutine_new(pcintr_coroutine_t cor)
{
    purc_variant_t retv = PURC_VARIANT_INVALID;
    purc_variant_t val = PURC_VARIANT_INVALID;

    static const struct purc_dvobj_method method [] = {
        { "target", target_getter, NULL },
        { "base", base_getter, base_setter },
        { "max_iteration_count",
            max_iteration_count_getter, max_iteration_count_setter },
        { "max_recursion_depth",
            max_recursion_depth_getter, max_recursion_depth_setter },
        { "max_embedded_levels",
            max_embedded_levels_getter, max_embedded_levels_setter },
        { "timeout", timeout_getter, timeout_setter },
        { "cid",     cid_getter,     NULL },
        { "uri",     uri_getter,     NULL },
        { "token",   token_getter,   NULL },
        { "curator", curator_getter, NULL },
    };

    retv = purc_dvobj_make_from_methods(method, PCA_TABLESIZE(method));
    if (retv == PURC_VARIANT_INVALID) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    cor->target = strdup(DEFAULT_HVML_TARGET);
    if (cor->target == NULL) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    cor->base_url_string = strdup(DEFAULT_HVML_BASE);
    if (cor->base_url_string == NULL ||
            !pcutils_url_break_down(&cor->base_url_broken_down,
                DEFAULT_HVML_BASE)) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    cor->max_iteration_count = UINT64_MAX;
    cor->max_recursion_depth = UINT16_MAX;
    cor->max_embedded_levels = DEF_EMBEDDED_LEVELS;
    cor->timeout.tv_sec = DEFAULT_HVML_TIMEOUT_SEC;
    cor->timeout.tv_nsec = DEFAULT_HVML_TIMEOUT_NSEC;

    val = purc_variant_make_native((void *)cor, NULL);
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

    return retv;

failed:
    if (val)
        purc_variant_unref(val);
    if (retv)
        purc_variant_unref(retv);

    return PURC_VARIANT_INVALID;
}
