/*
 * @file string.c
 * @author Vincent Wei (https://github.com/VincentWei)
 * @date 2022/09/07
 * @brief Some string utilities for portability.
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

#if !HAVE(STRNDUP)

#include "private/ports.h"

#include <stdlib.h>
#include <string.h>

char *strndup(const char *s, size_t n)
{
    char *d = malloc(n + 1);
    strncpy(d, s, n);
    d[n] = '\0';
    return d;
}

#if !HAVE(STRNDUP)
