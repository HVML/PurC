/*
 * @file basename.c
 * @author Vincent Wei
 * @date 2022/03/10
 * @brief The portable implementation for POSIX sleep() and usleep().
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

#include "config.h"

#include "purc-utils.h"
#include "private/utils.h"

#include <string.h>

const char *
pcutils_basename(const char *s)
{
    const char *path_sep;

    path_sep = strrchr(s, PATH_SEP);
    if (path_sep == NULL)
        return s;

    /* avoid trailing PATH_SEP, if present */
    if (!IS_PATH_SEP(s[strlen (s) - 1]))
        return path_sep + 1;

    while (--path_sep > s && !IS_PATH_SEP(*path_sep))
        ;
    return (path_sep != s) ? path_sep + 1 : s;
}

