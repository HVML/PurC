/*
 * @file strnstr.c
 * @author Vincent Wei (https://github.com/VincentWei)
 * @date 2025/02/20
 * @brief The implementation of strnstr().
 *
 * Copyright (C) 2025 FMSoft <https://www.fmsoft.cn>
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

#if !HAVE(STRNSTR)

#include "private/ports.h"

#include <string.h>

/* compliant version of strnstr() */
char *strnstr(const char *haystack, const char *needle, size_t len)
{
    size_t left = len;
    size_t needle_len = strlen(needle);
    char *p = (char *)haystack;

    if (*p == 0) {
        return p;
    }

    while (*p != 0 && left >= needle_len) {
        if (memcmp(p, needle, needle_len) == 0) {
            return p;
        }

        p++;
        left--;
    }

    return NULL;
}

#endif /* !HAVE(STRNSTR) */

