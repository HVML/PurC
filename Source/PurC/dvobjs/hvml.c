/*
 * @file string.c
 * @author Geng Yue
 * @date 2021/07/02
 * @brief The implementation of string dynamic variant object.
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
#include "private/dvobjs.h"
#include "purc-variant.h"
#include "helper.h"

#include <limits.h>

struct hvml_interpret_param {
    char                *url;
    unsigned long int   maxIterationCount;
    unsigned short      maxRecursionDepth;
};

static struct hvml_interpret_param *hvml_param = NULL;

static purc_variant_t
base_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (hvml_param) {
        if (hvml_param->url)
            // do no use purc_variant_make_string_reuse_buff
            // if change url, the old url will be freed, and create new buffer.
            // it is too dangerous.
            ret_var = purc_variant_make_string (hvml_param->url, false);
    }

    return ret_var;
}

static purc_variant_t
base_setter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args < 1)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    const char *url = purc_variant_get_string_const (argv[0]);
    size_t length = 0;
    purc_variant_string_bytes (argv[0], &length);

    if (hvml_param) {
        if (hvml_param->url)
            hvml_param->url = realloc (hvml_param->url, length);
        else
            hvml_param->url = malloc (length);

        if (hvml_param->url) {
            strcpy (hvml_param->url, url);
            ret_var = purc_variant_make_string (hvml_param->url, false);
        }
    }

    return ret_var;
}


static purc_variant_t
maxIterationCount_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    uint64_t u64 = ULONG_MAX;

    if (hvml_param) {
        u64 = hvml_param->maxIterationCount;
        ret_var = purc_variant_make_ulongint (u64);
    }

    return ret_var;
}


static purc_variant_t
maxIterationCount_setter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args < 1)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_ulongint (argv[0]))) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    uint64_t u64;
    purc_variant_cast_to_ulongint (argv[0], &u64, false);

    if (hvml_param) {
        hvml_param->maxIterationCount = u64;
        ret_var = purc_variant_make_ulongint (u64);
    }

    return ret_var;
}


static purc_variant_t
maxRecursionDepth_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    uint64_t u64 = USHRT_MAX;

    if (hvml_param) {
        u64 = hvml_param->maxRecursionDepth;
        ret_var = purc_variant_make_ulongint (u64);
    }

    return ret_var;
}


static purc_variant_t
maxRecursionDepth_setter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args < 1)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_ulongint (argv[0]))) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    uint64_t u64;
    purc_variant_cast_to_ulongint (argv[0], &u64, false);

    if (u64 > USHRT_MAX)
        u64 = USHRT_MAX;

    if (hvml_param) {
        hvml_param->maxRecursionDepth = u64;
        ret_var = purc_variant_make_ulongint (u64);
    }

    return ret_var;
}

purc_variant_t pcdvobjs_get_hvml (void)
{
    static struct pcdvobjs_dvobjs method [] = {
        {"base",               base_getter,  base_setter},
        {"maxIterationCount",  maxIterationCount_getter, maxIterationCount_setter},
        {"maxRecursionDepth",  maxRecursionDepth_getter, maxRecursionDepth_setter},
    };

    if (hvml_param == NULL) {
        hvml_param = malloc (sizeof(struct hvml_interpret_param));
        if (hvml_param) {
            hvml_param->url = NULL;
            hvml_param->maxIterationCount = ULONG_MAX;
            hvml_param->maxRecursionDepth = USHRT_MAX;
        }
    }
    return pcdvobjs_make_dvobjs (method, PCA_TABLESIZE(method));
}
