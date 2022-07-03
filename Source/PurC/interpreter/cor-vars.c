/*
 * @file cor-vars.c
 * @author Vincent Wei
 * @date 2022/07/03
 * @brief The management of coroutine-level variables.
 *  Moved from ../vdom/vdom.c authored by Xue Shuming.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
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

#include "private/errors.h"
#include "private/debug.h"
#include "private/interpreter.h"
#include "private/vdom.h"

bool
purc_coroutine_bind_variable(purc_coroutine_t cor, const char *name,
        purc_variant_t variant)
{
    if (!cor || !cor->vdom || !name || !variant) {
        PC_ASSERT(0);
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return false;
    }

    struct pcvdom_node *node = pcvdom_doc_cast_to_node(cor->vdom);
    pcvarmgr_t scoped_variables = pcintr_create_scoped_variables(node);
    if (!scoped_variables)
        return false;

    bool b = pcvarmgr_add(scoped_variables, name, variant);
    return b;
}

static pcvarmgr_t
purc_coroutine_get_varmgr(purc_coroutine_t cor)
{
    if (!cor || !cor->vdom) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    struct pcvdom_node *node = pcvdom_doc_cast_to_node(cor->vdom);
    return pcintr_get_scoped_variables(cor, node);
}

bool
purc_coroutine_unbind_variable(purc_coroutine_t cor, const char *name)
{
    if (!cor || !cor->vdom || !name) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return false;
    }

    pcvarmgr_t scoped_variables = purc_coroutine_get_varmgr(cor);
    if (!scoped_variables)
        return false;

    return pcvarmgr_remove(scoped_variables, name);
}

purc_variant_t
purc_coroutine_get_variable(purc_coroutine_t cor, const char *name)
{
    if (!cor || !cor->vdom || !name) {
        PC_ASSERT(0);
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    pcvarmgr_t scoped_variables = purc_coroutine_get_varmgr(cor);
    if (!scoped_variables)
        return false;

    return pcvarmgr_get(scoped_variables, name);
}

