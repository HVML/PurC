/*
 * @file text.c
 * @author Geng Yue, Vincent Wei
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

#define T_MAP_NAME          "map"

static purc_variant_t
get_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    if (nr_args < 1) {
        pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if (!purc_variant_is_string(argv[0])) {
        pcinst_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_variant_t var = purc_variant_object_get_by_ckey(root,
            T_MAP_NAME);

    if (var) {
        ret_var = purc_variant_object_get(var, argv[0]);
        // ret_var is a reference of value
        if (ret_var == PURC_VARIANT_INVALID)
            ret_var = argv[0];
    }
    else
        ret_var = argv[0];

    return purc_variant_ref(ret_var);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_string_static("", false);

    return PURC_VARIANT_INVALID;
}

purc_variant_t purc_dvobj_text_new(void)
{
    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_variant_t dict = PURC_VARIANT_INVALID;

    static struct purc_dvobj_method method [] = {
        { "get", get_getter, NULL },
    };

    ret_var = purc_dvobj_make_from_methods(method, PCA_TABLESIZE(method));
    if (ret_var == PURC_VARIANT_INVALID)
        goto fatal;

    dict = purc_variant_make_object(0,
                PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    if (dict == PURC_VARIANT_INVALID)
        goto fatal;
    if (!purc_variant_object_set_by_static_ckey(ret_var, T_MAP_NAME, dict))
        goto fatal;
    purc_variant_unref(dict);

    return ret_var;

fatal:
    if (dict)
        purc_variant_unref(dict);
    if (ret_var)
        purc_variant_unref(ret_var);

    return PURC_VARIANT_INVALID;
}
