/*
 * @file vasprintf.c
 * @author Vincent Wei (https://github.com/VincentWei)
 * @date 2021/07/02
 * @brief The instance of PurC.
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
 * Note that the code is derived from json-c:
 *
 * Copyright (c) 2004, 2005 Metaparadigm Pte. Ltd.
 * Michael Clark <michael@metaparadigm.com>
 * Copyright (c) 2009 Hewlett-Packard Development Company, L.P.
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See COPYING for details.
 */

#include "config.h"

#if !HAVE(VASPRINTF)

#include "private/utils.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

/* compliant version of vasprintf */
int vasprintf(char **buf, const char *fmt, va_list ap)
{
#ifndef WIN32
    static char _T_emptybuffer = '\0';
#endif /* !defined(WIN32) */
    int chars;
    char *b;

    if (!buf) {
        return -1;
    }

#ifdef WIN32
    chars = _vscprintf(fmt, ap) + 1;
#else  /* !defined(WIN32) */
    /* CAW: RAWR! We have to hope to god here that vsnprintf doesn't overwrite
     * our buffer like on some 64bit sun systems.... but hey, its time to move on
     */
    chars = vsnprintf(&_T_emptybuffer, 0, fmt, ap) + 1;
    if (chars < 0) {
        chars *= -1;
    } /* CAW: old glibc versions have this problem */
#endif /* defined(WIN32) */

    b = (char *)malloc(sizeof(char) * chars);
    if (!b) {
        return -1;
    }

    if ((chars = vsprintf(b, fmt, ap)) < 0) {
        free(b);
    }
    else {
        *buf = b;
    }

    return chars;
}
#endif /* !HAVE(VASPRINTF) */

