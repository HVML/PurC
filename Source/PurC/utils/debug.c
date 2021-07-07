/*
 * @file debug.c
 * @date 2021/07/07
 * @brief The implementation of debug functions.
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
 */

#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#if HAVE(SYSLOG_H)
#include <syslog.h>
#endif /* HAVE_SYSLOG_H */

#include "private/debug.h"

static bool _syslog = false;
#ifdef NDEBUG
static bool _debug = false;
#else
static bool _debug = true;
#endif

void pcutils_enable_debug(bool debug)
{
    _debug = debug;
}

void pcutils_enable_syslog(bool syslog)
{
    _syslog = syslog;
}

void pcutils_debug(const char *msg, ...)
{
    if (_debug) {
        va_list ap;

        va_start(ap, msg);
#if HAVE(VSYSLOG)
        if (_syslog) {
            vsyslog(LOG_DEBUG, msg, ap);
        }
        else
#endif
            vprintf(msg, ap);
        va_end(ap);
    }
}

void pcutils_error(const char *msg, ...)
{
    va_list ap;

    va_start(ap, msg);
#if HAVE(VSYSLOG)
    if (_syslog) {
        vsyslog(LOG_ERR, msg, ap);
    }
    else
#endif
        vfprintf(stderr, msg, ap);
    va_end(ap);
}

void pcutils_info(const char *msg, ...)
{
    va_list ap;

    va_start(ap, msg);
#if HAVE(VSYSLOG)
    if (_syslog) {
        vsyslog(LOG_INFO, msg, ap);
    }
    else
#endif
        vfprintf(stderr, msg, ap);
    va_end(ap);
}
