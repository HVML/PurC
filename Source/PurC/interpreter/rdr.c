/*
 * @file rdr.c
 * @author XueShuming
 * @date 2022/03/09
 * @brief The impl of the interaction between interpreter and renderer.
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
#include "internal.h"

#include "private/errors.h"
#include "private/instance.h"
#include "private/utils.h"
#include "private/variant.h"

#include <string.h>


bool
purc_attach_vdom_to_renderer(purc_vdom_t vdom,
        const char *target_workspace,
        const char *target_window,
        const char *target_tabpage,
        const char *target_level,
        purc_renderer_extra_info *extra_info)
{
    UNUSED_PARAM(vdom);
    UNUSED_PARAM(target_workspace);
    UNUSED_PARAM(target_window);
    UNUSED_PARAM(target_tabpage);
    UNUSED_PARAM(target_level);
    UNUSED_PARAM(extra_info);
    return false;
}
