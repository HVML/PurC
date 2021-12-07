/*
 * @file var-mgr.c
 * @author XueShuming
 * @date 2021/12/06
 * @brief The impl of PurC variable manager.
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

#include "purc.h"

#include "config.h"

#include "private/var-mgr.h"
#include "private/errors.h"
#include "private/utils.h"
#include "private/instance.h"

#include <stdlib.h>
#include <string.h>

static void* pcvarmgr_list_copy_val(const void* val)
{
    purc_variant_t var = (purc_variant_t)val;
    purc_variant_ref(var);
    return var;
}

static void pcvarmgr_list_free_val(void* val)
{
    purc_variant_t var = (purc_variant_t)val;
    purc_variant_unref(var);
}

pcvarmgr_list_t pcvarmgr_list_create(void)
{
    return pcutils_map_create (copy_key_string, free_key_string,
            pcvarmgr_list_copy_val, pcvarmgr_list_free_val,
            comp_key_string, false);
}

int pcvarmgr_list_destroy(pcvarmgr_list_t list)
{
    return pcutils_map_destroy(list);
}

bool pcvarmgr_list_add(pcvarmgr_list_t list, const char* name,
        purc_variant_t variant)
{
    if (pcutils_map_find_replace_or_insert(list, name,
                (void *)variant, NULL)) {
        return false;
    }

    return true;
}

purc_variant_t pcvarmgr_list_get(pcvarmgr_list_t list, const char* name)
{
    const pcutils_map_entry* entry = NULL;

    if (name == NULL) {
        return PURC_VARIANT_INVALID;
    }

    if ((entry = pcutils_map_find(list, name))) {
        return (purc_variant_t) entry->val;
    }

    return PURC_VARIANT_INVALID;
}

bool pcvarmgr_list_remove(pcvarmgr_list_t list, const char* name)
{
    if (name) {
        if (pcutils_map_erase (list, (void*)name))
            return true;
    }
    return false;
}

