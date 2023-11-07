/*
 * window-transition-styles.c -- The parser for window transition styles.
 *
 * Copyright (C) 2023 FMSoft (http://www.fmsoft.cn)
 *
 * Authors: XueShuming 2023
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

#include "config.h"
#include "purc/purc-helpers.h"
#include "private/utils.h"
#include "private/debug.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <math.h>

int
purc_evaluate_standalone_window_transition_from_styles(const char *styles,
      struct purc_window_transition *transition)
{
    UNUSED_PARAM(styles);
    UNUSED_PARAM(transition);
    return 0;
}

