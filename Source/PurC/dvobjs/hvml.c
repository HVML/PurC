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
#include "private/interpreter.h"
#include "private/dvobjs.h"
#include "purc-variant.h"
#include "helper.h"

#include <limits.h>

static purc_variant_t
base_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    pcintr_stack_t stack = purc_get_stack();
    if (stack) {
        if (stack->dvobjs.hvml.url)
            ret_var = purc_variant_make_string_reuse_buff (
                    stack->dvobjs.hvml.url,
                    strlen (stack->dvobjs.hvml.url), false);
    }

    return ret_var;
}

static purc_variant_t
base_setter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args < 1)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    const char *url = purc_variant_get_string_const (argv[0]);
    size_t length = 0;
    purc_variant_string_bytes (argv[0], &length);

    pcintr_stack_t stack = purc_get_stack();
    if (stack) {
        if (stack->dvobjs.hvml.url)
            stack->dvobjs.hvml.url = realloc (stack->dvobjs.hvml.url, length);
        else
            stack->dvobjs.hvml.url = malloc (length);

        if (stack->dvobjs.hvml.url) {
            strcpy (stack->dvobjs.hvml.url, url);
            ret_var = purc_variant_make_string_reuse_buff (
                    stack->dvobjs.hvml.url,
                    strlen (stack->dvobjs.hvml.url), false);
        }
    }

    return ret_var;
}


static purc_variant_t
maxIterationCount_getter (
        purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    uint64_t u64 = ULONG_MAX;

    pcintr_stack_t stack = purc_get_stack();
    if (stack) {
        u64 = stack->dvobjs.hvml.maxIterationCount;
        ret_var = purc_variant_make_ulongint (u64);
    }

    return ret_var;
}


static purc_variant_t
maxIterationCount_setter (
        purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args < 1)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_ulongint (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    uint64_t u64;
    purc_variant_cast_to_ulongint (argv[0], &u64, false);

    pcintr_stack_t stack = purc_get_stack();
    if (stack) {
        stack->dvobjs.hvml.maxIterationCount = u64;
        ret_var = purc_variant_make_ulongint (u64);
    }

    return ret_var;
}


static purc_variant_t
maxRecursionDepth_getter (
        purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    uint64_t u64 = USHRT_MAX;

    pcintr_stack_t stack = purc_get_stack();
    if (stack) {
        u64 = stack->dvobjs.hvml.maxRecursionDepth;
        ret_var = purc_variant_make_ulongint (u64);
    }

    return ret_var;
}


static purc_variant_t
maxRecursionDepth_setter (
        purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args < 1)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_ulongint (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    uint64_t u64;
    purc_variant_cast_to_ulongint (argv[0], &u64, false);

    if (u64 > USHRT_MAX) {
        // send an exception?
        u64 = USHRT_MAX;
    }

    pcintr_stack_t stack = purc_get_stack();
    if (stack) {
        stack->dvobjs.hvml.maxRecursionDepth = u64;
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

    return pcdvobjs_make_dvobjs (method, PCA_TABLESIZE(method));
}
