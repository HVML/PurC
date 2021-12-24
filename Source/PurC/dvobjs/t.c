/*
 * @file t.c
 * @author Geng Yue
 * @date 2021/07/02
 * @brief The implementation of T dynamic variant object.
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

static purc_variant_t
get_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

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
    purc_variant_t var = purc_variant_object_get_by_ckey (root, "map");
    if (var) {
        ret_var = purc_variant_object_get (var, argv[0]);
        // ret_var is a reference of value
        if (ret_var)
            purc_variant_ref (ret_var);
    }

    return ret_var;
}

purc_variant_t pcdvobjs_get_t (void *param)
{
    UNUSED_PARAM(param);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    static struct pcdvobjs_dvobjs method [] = {
        {"get",          get_getter,          NULL},
    };

    ret_var = pcdvobjs_make_dvobjs (method, PCA_TABLESIZE(method));

    if (ret_var) {
        purc_variant_t dict = purc_variant_make_object (0,
                PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
        purc_variant_object_set_by_static_ckey (ret_var, "map", dict);
        purc_variant_unref (dict);
    }

    return ret_var;
}
