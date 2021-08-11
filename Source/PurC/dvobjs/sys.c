/*
 * @file dvobjs.c
 * @author Geng Yue
 * @date 2021/07/02
 * @brief The implementation of public part for html parser.
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
 *
 * This implementation of HTML parser is derived from Lexbor
 * <https://github.com/lexbor/lexbor>, which is licensed under the Apache
 * License, Version 2.0:
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

#include "private/instance.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/edom.h"
#include "private/html.h"

#include "purc-variant.h"
#include "dvobjs/parser.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/utsname.h>


// <object: the set of result, name: value> $_SYSTEM.uname (<string : items>)
// for example: 
// $_SYSTEM.uname('[kernel-name || kernel-release || kernel-version || nodename
//               || machine || processor || hardware-platform 
//               || operating-system] | all | default')
static UNUSED_FUNCTION purc_variant_t
get_uname (purc_variant_t root, unsigned int nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    struct utsname name;

    if (uname (&name) < 0) {
        return purc_variant_make_undefined ();
    }

#if 0
    if (nr_args) {
        if (purc_variant_is_string (argv[0])) {
            const char* option = purc_variant_get_string_const (argv[0]);

        }
    }
#endif

    return purc_variant_make_string (name.sysname, true);
}


static UNUSED_FUNCTION purc_variant_t
get_locale (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    struct utsname name;

    return purc_variant_make_string (name.sysname, true);
}


static UNUSED_FUNCTION purc_variant_t
set_locale (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    struct utsname name;

    return purc_variant_make_string (name.sysname, true);
}


static UNUSED_FUNCTION purc_variant_t
get_random (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    struct utsname name;

    return purc_variant_make_string (name.sysname, true);
}

static UNUSED_FUNCTION purc_variant_t
get_time (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    struct utsname name;

    return purc_variant_make_string (name.sysname, true);
}
