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
#define DVOBJ_HVML_DATA_NAME        "__handle_dvobj_hvml"

#define DEFAULT_HVML_TIMEOUT_SEC    (time_t)DEFAULT_HVML_TIMEOUT
#define DEFAULT_HVML_TIMEOUT_NSEC   (long)((DEFAULT_HVML_TIMEOUT -  \
            DEFAULT_HVML_TIMEOUT_SEC) * 1000000000)

//extern char * pcdvobjs_get_url (struct purc_broken_down_url * url);
//extern bool pcdvobjs_set_url (struct purc_broken_down_url *url,
//        const char *url_string);

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
    struct pcvdom_dvobj_hvml *dvobj_hvml = (struct pcvdom_dvobj_hvml *)
        get_dvobj_internal_pointer(root, DVOBJ_HVML_DATA_NAME);

    if (dvobj_hvml) {
        char *url = pcdvobjs_get_url (&(dvobj_hvml->url));
        if (url)
            ret_var = purc_variant_make_string_reuse_buff (
                    url, strlen (url), false);
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

    struct pcvdom_dvobj_hvml *dvobj_hvml = (struct pcvdom_dvobj_hvml *)
        get_dvobj_internal_pointer(root, DVOBJ_HVML_DATA_NAME);
    if (dvobj_hvml) {
        /* If the url is invlid, dvobj_hvml->url will not be changed.
         If the url is valid, perhaps the string which pcdvobjs_get_url() returned
         is not identical to input string. For example:
         input string to pcdvobjs_set_url is:  http://www.minigui.org
         output string of pcdvobjs_get_url is: http://www.minigui.org/
         */
        pcdvobjs_set_url (&(dvobj_hvml->url), url);

        char *url = pcdvobjs_get_url (&(dvobj_hvml->url));
        if (url) {
            ret_var = purc_variant_make_string_reuse_buff (
                    url, strlen (url), false);
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

    struct pcvdom_dvobj_hvml *dvobj_hvml = (struct pcvdom_dvobj_hvml *)
        get_dvobj_internal_pointer(root, DVOBJ_HVML_DATA_NAME);
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

    struct pcvdom_dvobj_hvml *dvobj_hvml = (struct pcvdom_dvobj_hvml *)
        get_dvobj_internal_pointer(root, DVOBJ_HVML_DATA_NAME);
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
    struct pcvdom_dvobj_hvml *dvobj_hvml = (struct pcvdom_dvobj_hvml *)
        get_dvobj_internal_pointer(root, DVOBJ_HVML_DATA_NAME);
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

    struct pcvdom_dvobj_hvml *dvobj_hvml = (struct pcvdom_dvobj_hvml *)
        get_dvobj_internal_pointer(root, DVOBJ_HVML_DATA_NAME);
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

    struct pcvdom_dvobj_hvml *dvobj_hvml = (struct pcvdom_dvobj_hvml *)
        get_dvobj_internal_pointer(root, DVOBJ_HVML_DATA_NAME);
    if (dvobj_hvml) {
        double number = (double)dvobj_hvml->timeout.tv_sec +
                (double)dvobj_hvml->timeout.tv_nsec / 1000000000.0;
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
    if (!purc_variant_is_number (argv[0])) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number = 0.0;
    purc_variant_cast_to_number (argv[0], &number, false);

    struct pcvdom_dvobj_hvml *dvobj_hvml = (struct pcvdom_dvobj_hvml *)
        get_dvobj_internal_pointer(root, DVOBJ_HVML_DATA_NAME);
    if (dvobj_hvml) {
        if (number > 0) {
            dvobj_hvml->timeout.tv_sec = (long) number;
            dvobj_hvml->timeout.tv_nsec = (long)
                ((number - dvobj_hvml->timeout.tv_sec) * 1000000000);
        }
        else
            number = (double)dvobj_hvml->timeout.tv_sec +
                (double)dvobj_hvml->timeout.tv_nsec / 1000000000;

        ret_var = purc_variant_make_number (number);
    }

    return ret_var;
}

purc_variant_t pcdvobjs_get_hvml (struct pcvdom_dvobj_hvml *dvobj_hvml)
{
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    static struct pcdvobjs_dvobjs method [] = {
        {"base",               base_getter,              base_setter},
        {"maxIterationCount",  maxIterationCount_getter, maxIterationCount_setter},
        {"maxRecursionDepth",  maxRecursionDepth_getter, maxRecursionDepth_setter},
        {"timeout",            timeout_getter,           timeout_setter},
    };

    ret_var = pcdvobjs_make_dvobjs (method, PCA_TABLESIZE(method));

    // initialize pcvdom_dvobj_hvml
    // set default URL
    memset (&(dvobj_hvml->url), 0, sizeof(struct purc_broken_down_url));
    pcdvobjs_set_url (&(dvobj_hvml->url), DEFAULT_HVML_BASE);

    dvobj_hvml->maxIterationCount = ULONG_MAX;
    dvobj_hvml->maxRecursionDepth = USHRT_MAX;
    dvobj_hvml->timeout.tv_sec = DEFAULT_HVML_TIMEOUT_SEC;
    dvobj_hvml->timeout.tv_nsec = DEFAULT_HVML_TIMEOUT_NSEC;

    purc_variant_t val = purc_variant_make_native ((void *)dvobj_hvml, NULL);
    purc_variant_object_set_by_static_ckey (ret_var, DVOBJ_HVML_DATA_NAME, val);
    purc_variant_unref (val);

    return ret_var;
}
