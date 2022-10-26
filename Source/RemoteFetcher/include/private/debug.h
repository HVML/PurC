/*
 * @file debug.h
 * @author Vincent Wei
 * @date 2021/09/18
 * @brief The internal interfaces for debug.
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

#ifndef PURCFETCHER_PRIVATE_DEBUG_H
#define PURCFETCHER_PRIVATE_DEBUG_H

#include "config.h"

#include <stdlib.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

void fbutils_enable_debug(bool debug) WTF_INTERNAL;
void fbutils_enable_syslog(bool syslog) WTF_INTERNAL;

void fbutils_debug(const char *msg, ...) WTF_INTERNAL
    __attribute__ ((format (printf, 1, 2)));

void fbutils_error(const char *msg, ...) WTF_INTERNAL
    __attribute__ ((format (printf, 1, 2)));

void fbutils_info(const char *msg, ...) WTF_INTERNAL
    __attribute__ ((format (printf, 1, 2)));


#ifdef __cplusplus
}
#endif

#ifndef __STRING
#define __STRING(x) #x
#endif

#ifdef NDEBUG

#define FB_ASSERT(cond)                                 \
    do {                                                \
        if (!(cond)) {                                  \
            fbutils_error("PurCFetcher assert failed.\n");     \
            abort();                                    \
        }                                               \
    } while (0)

#else /* define NDEBUG */

#define FB_ASSERT(cond)                                                 \
    do {                                                                \
        if (!(cond)) {                                                  \
            fbutils_error("PurCFetcher assert failure %s:%d: condition \""   \
                    __STRING(cond) " failed\n",                         \
                     __FILE__, __LINE__);                               \
            assert(0);                                                  \
        }                                                               \
    } while (0)

#endif /* not defined NDEBUG */

#define FB_ERROR(x, ...) fbutils_error(x, ##__VA_ARGS__)

#ifndef NDEBUG

# define FB_ENABLE_DEBUG(x) fbutils_enable_debug(x)
# define FB_ENABLE_SYSLOG(x) fbutils_enable_syslog(x)
# define FB_DEBUG(x, ...) fbutils_debug(x, ##__VA_ARGS__)
# define FB_INFO(x, ...) fbutils_info(x, ##__VA_ARGS__)

#else /* not defined NDEBUG */

# define FB_ENABLE_DEBUG(x)             \
    if (0)                              \
        fbutils_enable_debug(x)

# define FB_ENABLE_SYSLOG(x)            \
    if (0)                              \
        fbutils_set_syslog(x)

#define FB_DEBUG(x, ...)                \
    if (0)                              \
        fbutils_debug(x, ##__VA_ARGS__)

#define FB_INFO(x, ...)                 \
    if (0)                              \
        fbutils_info(x, ##__VA_ARGS__)

#endif /* defined NDEBUG */

#endif /* PURCFETCHER_PRIVATE_DEBUG_H */
