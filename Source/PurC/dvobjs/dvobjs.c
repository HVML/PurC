/*
 * @file dvobjs.c
 * @author Geng Yue
 * @date 2021/07/02
 * @brief The interface of dynamic variant objects.
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
#include "private/debug.h"
#include "private/utils.h"
#include "private/edom.h"
#include "private/html.h"

#include "purc-variant.h"
#include "system.h"
#include "string.h"
#include "math.h"
#include "logical.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void pcdvobjs_init_once(void)
{
    // initialize others
}

void pcdvobjs_init_instance(struct pcinst* inst)
{
    UNUSED_PARAM(inst);

}

void pcdvobjs_cleanup_instance(struct pcinst* inst)
{
    UNUSED_PARAM(inst);
}

// only for test now.
purc_variant_t pcdvojbs_get_system (void)
{
    purc_variant_t sys = purc_variant_make_object_c (6,
            "uname",        purc_variant_make_dynamic (get_uname, NULL),
            "uname_prt",    purc_variant_make_dynamic (get_uname_prt, NULL),
            "locale",       purc_variant_make_dynamic (get_locale, set_locale),
            "random",       purc_variant_make_dynamic (get_random, NULL),
            "time",         purc_variant_make_dynamic (get_time, set_time),
            "time_iso8601", purc_variant_make_dynamic (get_time_iso8601, NULL)
       );
    return sys;
}

// only for test now.
purc_variant_t pcdvojbs_get_string (void)
{
    purc_variant_t string = purc_variant_make_object_c (7,
            "conatins",     purc_variant_make_dynamic (string_contains, NULL),
            "ends_with",    purc_variant_make_dynamic (string_ends_with, NULL),
            "explode",      purc_variant_make_dynamic (string_explode, NULL),
            "shuffle",      purc_variant_make_dynamic (string_shuffle, NULL),
            "replace",      purc_variant_make_dynamic (string_replace, NULL),
            "format_c",     purc_variant_make_dynamic (string_format_c, NULL),
            "format_p",     purc_variant_make_dynamic (string_format_p, NULL)
       );
    return string;
}

// only for test now.
purc_variant_t pcdvojbs_get_math (void)
{
    purc_variant_t math = purc_variant_make_object_c (7,
            "pi",     purc_variant_make_dynamic (get_pi, NULL),
            "eval",  purc_variant_make_dynamic (math_eval, NULL),
            "sin",   purc_variant_make_dynamic (math_sin, NULL),
            "cos",   purc_variant_make_dynamic (math_cos, NULL),
            "sqrt",  purc_variant_make_dynamic (math_sqrt, NULL)
       );
    return math;
}

// only for test now.
purc_variant_t pcdvojbs_get_logical (void)
{
    purc_variant_t logical = purc_variant_make_object_c (7,
            "not",    purc_variant_make_dynamic (logical_not, NULL),
            "and",    purc_variant_make_dynamic (logical_and, NULL),
            "or",     purc_variant_make_dynamic (logical_or, NULL),
            "xor",    purc_variant_make_dynamic (logical_xor, NULL),
            "eq",     purc_variant_make_dynamic (logical_eq, NULL),
            "ne",     purc_variant_make_dynamic (logical_ne, NULL),
            "gt",     purc_variant_make_dynamic (logical_gt, NULL),
            "ge",     purc_variant_make_dynamic (logical_ge, NULL),
            "lt",     purc_variant_make_dynamic (logical_lt, NULL),
            "le",     purc_variant_make_dynamic (logical_le, NULL),
            "streq",  purc_variant_make_dynamic (logical_streq, NULL),
            "strne",  purc_variant_make_dynamic (logical_strne, NULL),
            "strgt",  purc_variant_make_dynamic (logical_strgt, NULL),
            "strge",  purc_variant_make_dynamic (logical_strge, NULL),
            "strlt",  purc_variant_make_dynamic (logical_strlt, NULL),
            "strle",  purc_variant_make_dynamic (logical_strle, NULL),
            "eval",   purc_variant_make_dynamic (logical_eval, NULL)
       );
    return logical;
}
