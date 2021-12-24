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
#include "purc-variant.h"
#include "helper.h"

#include <limits.h>

inline static struct pcvdom_dvobj_hvml * get_dvobj_html_in_vdom (
        purc_variant_t root)
{
    purc_variant_t var = purc_variant_object_get_by_ckey (
            root, DVOBJ_HVML_DATA_NAME);
    if (var) {
        uint64_t u64 = 0;
        purc_variant_cast_to_ulongint (var, &u64, false);
        return (struct pcvdom_dvobj_hvml *)u64;
    }
    else
        return NULL;
}

static purc_variant_t
base_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    if (root == PURC_VARIANT_INVALID) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }
    if (!purc_variant_is_object (root)) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    struct pcvdom_dvobj_hvml *dvobj_hvml = get_dvobj_html_in_vdom (root);

    if (dvobj_hvml) {
        ret_var = purc_variant_make_string (dvobj_hvml->url, false);
    }

    return ret_var;
}

static purc_variant_t
base_setter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
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

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    const char *url = purc_variant_get_string_const (argv[0]);
    size_t length = 0;
    purc_variant_string_bytes (argv[0], &length);

    // TODO: url is valid.
    // bool pcutils_url_valid (char *url, struct purc_broken_down_url *);

    struct pcvdom_dvobj_hvml *dvobj_hvml = get_dvobj_html_in_vdom (root);
    if (dvobj_hvml) {
        if (length <= (strlen (dvobj_hvml->url) + 1)) {
            strcpy (dvobj_hvml->url, url);
            ret_var = purc_variant_make_string (url, false);
        }
        else {
            // dvobj_hvml->url can not be NULL, so use malloc to test,
            // do not use realloc
            char * newbuf = malloc (length);

            if (newbuf) {
                strcpy (newbuf, url);
                free (dvobj_hvml->url);
                dvobj_hvml->url = newbuf;
                ret_var = purc_variant_make_string (url, false);
            }
            else {
                pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
                ret_var = purc_variant_make_string (dvobj_hvml->url, false);
            }
        }
    }

    return ret_var;
}


static purc_variant_t
maxIterationCount_getter (
        purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    if (root == PURC_VARIANT_INVALID) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }
    if (!purc_variant_is_object (root)) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    struct pcvdom_dvobj_hvml *dvobj_hvml = get_dvobj_html_in_vdom (root);
    if (dvobj_hvml) {
        ret_var = purc_variant_make_ulongint (dvobj_hvml->maxIterationCount);
    }

    return ret_var;
}


static purc_variant_t
maxIterationCount_setter (
        purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
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

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    uint64_t u64;
    purc_variant_cast_to_ulongint (argv[0], &u64, false);

    struct pcvdom_dvobj_hvml *dvobj_hvml = get_dvobj_html_in_vdom (root);
    if (dvobj_hvml) {
        dvobj_hvml->maxIterationCount = u64;
        ret_var = purc_variant_make_ulongint (u64);
    }

    return ret_var;
}


static purc_variant_t
maxRecursionDepth_getter (
        purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    if (root == PURC_VARIANT_INVALID) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }
    if (!purc_variant_is_object (root)) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    struct pcvdom_dvobj_hvml *dvobj_hvml = get_dvobj_html_in_vdom (root);
    if (dvobj_hvml) {
        ret_var = purc_variant_make_ulongint (dvobj_hvml->maxRecursionDepth);
    }

    return ret_var;
}


static purc_variant_t
maxRecursionDepth_setter (
        purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
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

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    uint64_t u64;
    purc_variant_cast_to_ulongint (argv[0], &u64, false);

    struct pcvdom_dvobj_hvml *dvobj_hvml = get_dvobj_html_in_vdom (root);
    if (dvobj_hvml) {
        dvobj_hvml->maxRecursionDepth = u64;
        ret_var = purc_variant_make_ulongint (u64);
    }

    return ret_var;
}

static purc_variant_t
timeout_getter (
        purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    if (root == PURC_VARIANT_INVALID) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }
    if (!purc_variant_is_object (root)) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    struct pcvdom_dvobj_hvml *dvobj_hvml = get_dvobj_html_in_vdom (root);
    if (dvobj_hvml) {
        double number = (double)dvobj_hvml->timeout.tv_sec +
                (double)dvobj_hvml->timeout.tv_usec / 1000000.0;
        ret_var = purc_variant_make_number (number);
    }

    return ret_var;
}


static purc_variant_t
timeout_setter (
        purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
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

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number = 0.0;
    purc_variant_cast_to_number (argv[0], &number, false);

    struct pcvdom_dvobj_hvml *dvobj_hvml = get_dvobj_html_in_vdom (root);
    if (dvobj_hvml) {
        if (number > 0) {
            dvobj_hvml->timeout.tv_sec = (long) number;
            dvobj_hvml->timeout.tv_usec = (long)
                ((number - dvobj_hvml->timeout.tv_sec) * 1000000);
        }
        else
            number = (double)dvobj_hvml->timeout.tv_sec +
                (double)dvobj_hvml->timeout.tv_usec / 1000000;

        ret_var = purc_variant_make_number (number);
    }

    return ret_var;
}

purc_variant_t pcdvobjs_get_hvml (void * param)
{
    UNUSED_PARAM(param);
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    static struct pcdvobjs_dvobjs method [] = {
        {"base",               base_getter,              base_setter},
        {"maxIterationCount",  maxIterationCount_getter, maxIterationCount_setter},
        {"maxRecursionDepth",  maxRecursionDepth_getter, maxRecursionDepth_setter},
        {"timeout",            timeout_getter,           timeout_setter},
    };

    ret_var = pcdvobjs_make_dvobjs (method, PCA_TABLESIZE(method));

    // initialize pcvdom_dvobj_hvml
    struct pcvdom_dvobj_hvml * dvobj_hvml = (struct pcvdom_dvobj_hvml *)param;
    int length = strlen (DEFAULT_HVML_BASE) + 1;
    dvobj_hvml->url = malloc (length);
    if (dvobj_hvml->url == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        purc_variant_unref (ret_var);
        return PURC_VARIANT_INVALID;
    }

    strcpy (dvobj_hvml->url, DEFAULT_HVML_BASE);
    dvobj_hvml->maxIterationCount = ULONG_MAX;
    dvobj_hvml->maxRecursionDepth = USHRT_MAX;
    dvobj_hvml->timeout.tv_sec = (long) DEFAULT_HVML_TIMEOUT;
    dvobj_hvml->timeout.tv_usec = (long) ((DEFAULT_HVML_TIMEOUT -
                dvobj_hvml->timeout.tv_sec) * 1000000);

    purc_variant_t val = purc_variant_make_ulongint ((uint64_t)param);
    purc_variant_object_set_by_static_ckey (ret_var, DVOBJ_HVML_DATA_NAME, val);
    purc_variant_unref (val);

    return pcdvobjs_make_dvobjs (method, PCA_TABLESIZE(method));
}
