/*
 * @file misc.c
 * @author Vincent Wei (https://github.com/VincentWei)
 * @date 2021/07/02
 * @brief The misc portable functions.
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

#include "config.h"

#include "purc-errors.h"
#include "private/errors.h"
#include "private/utils.h"

#include <stdio.h>

#if OS(LINUX)

size_t pcutils_get_cmdline_arg(int arg, char* buf, size_t sz_buf)
{
    size_t i, n = 0;
    FILE *fp = fopen("/proc/self/cmdline", "rb");

    if (fp == NULL) {
        pcinst_set_error (PURC_ERROR_BAD_STDC_CALL);
        return 0;
    }

    if (arg > 0) {
        while (1) {
            int ch = fgetc(fp);
            if (ch == '\0') {
                n++;
            }
            else if (ch == EOF) {
                pcinst_set_error (PURC_ERROR_INVALID_VALUE);
                return 0;
            }

            if (n == (size_t)arg)
                break;
        }
    }

    n = 0;
    for (i = 0; i < sz_buf - 1; i++) {
        int ch = fgetc(fp);

        if (purc_isalnum(ch) || ch == '-' || ch == '_')
            buf[n++] = ch;
        else if (n > 0 && ch == '/')
            buf[n++] = '.';
    }

    buf[n] = '\0';
    fclose(fp);
    return n;
}

#else /* OS(LINUX) */

size_t pcutils_get_cmdline_arg(int arg, char* buf, size_t sz_buf)
{
    size_t i;
    const char* unknown = "unknown.cmdline";

    UNUSED_PARAM(arg);

    for (i = 0; i < sz_buf - 1; i++) {
        if (unknown[i])
            buf[i] = unknown[i];
        else
            break;
    }

    buf[i] = '\0';
    return i;
}
#endif /* not OS(LINUX) */
