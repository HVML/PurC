/*
 * @file debug.c
 * @date 2021/09/18
 * @brief The implementation of debug functions.
 *
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of PurCFetcher, which contains the examples of my course:
 * _the Best Practices of C Language_.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to
 * deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
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

void fbutils_enable_debug(bool debug)
{
    _debug = debug;
}

void fbutils_enable_syslog(bool syslog)
{
    _syslog = syslog;
}

void fbutils_debug(const char *msg, ...)
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

void fbutils_error(const char *msg, ...)
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

void fbutils_info(const char *msg, ...)
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
