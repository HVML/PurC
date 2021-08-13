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
    purc_variant_t sys = purc_variant_make_object_c (4,
            "uname_all", purc_variant_make_dynamic (get_uname_all, NULL),
            "uname",     purc_variant_make_dynamic (get_uname, NULL),
            "locale",    purc_variant_make_dynamic (get_locale, set_locale),
            "random",    purc_variant_make_dynamic (get_random, NULL),
            "time",      purc_variant_make_dynamic (get_time, set_time)
       );
    return sys;
}
