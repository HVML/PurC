/*
 * @file html.c
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

#define ATOM_HTML_BUCKET    1

#if 0
static struct atom_info {
    const char *string;
    purc_atom_t atom;
} my_atoms [] = {
    { "append", 0 },
    { "prepend", 0 },
    { "insertBefore", 0 },
    { "insertAfter", 0 },
};
#endif

purc_atom_t pcvariant_atom_append = 0;
purc_atom_t pcvariant_atom_prepend = 0;
purc_atom_t pcvariant_atom_insertBefore = 0;
purc_atom_t pcvariant_atom_insertAfter = 0;

void pchtml_init_once(void)
{
    // initialize others
    pcvariant_atom_append  = purc_atom_from_static_string_ex (
            ATOM_HTML_BUCKET, "append");
    pcvariant_atom_prepend = purc_atom_from_static_string_ex (
            ATOM_HTML_BUCKET, "prepend");
    pcvariant_atom_insertBefore  = purc_atom_from_static_string_ex (
            ATOM_HTML_BUCKET, "insertBefore");
    pcvariant_atom_insertAfter = purc_atom_from_static_string_ex (
            ATOM_HTML_BUCKET, "insertAfter");
}

void pchtml_init_instance(struct pcinst* inst)
{
    UNUSED_PARAM(inst);

    // initialize others
}

void pchtml_cleanup_instance(struct pcinst* inst)
{
    UNUSED_PARAM(inst);
}
