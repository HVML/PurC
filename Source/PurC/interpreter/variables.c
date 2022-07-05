/*
 * @file variables.c
 * @author XueShuming
 * @date 2022/07/05
 * @brief The session predefined variables.
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

#include "private/var-mgr.h"
#include "private/errors.h"
#include "private/instance.h"
#include "private/utils.h"
#include "private/variant.h"

#include <stdlib.h>
#include <string.h>

#define BUILDIN_VAR_SYSTEM      "SYSTEM"
#define BUILDIN_VAR_SESSION     "SESSION"

bool
purc_bind_session_variables(void)
{
    // $SYSTEM
    if(!purc_bind_variable(BUILDIN_VAR_SYSTEM,
                purc_dvobj_system_new())) {
        return false;
    }

    // $SESSION
    if(!purc_bind_variable(BUILDIN_VAR_SESSION,
                purc_dvobj_session_new())) {
        return false;
    }

    return true;
}
