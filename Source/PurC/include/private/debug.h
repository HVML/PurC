/*
 * @file debug.h
 * @author Vincent Wei
 * @date 2021/07/07
 * @brief The internal interfaces for debug.
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

#ifndef PURC_PRIVATE_DEBUG_H
#define PURC_PRIVATE_DEBUG_H

#include "config.h"

#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <libgen.h> /* for `basename` */

#ifdef __cplusplus
extern "C" {
#endif

void pcutils_enable_debug(bool debug) WTF_INTERNAL;
void pcutils_enable_syslog(bool syslog) WTF_INTERNAL;

void pcutils_debug(const char *msg, ...) WTF_INTERNAL
    __attribute__ ((format (printf, 1, 2)));

void pcutils_error(const char *msg, ...)
    __attribute__ ((format (printf, 1, 2)));

void pcutils_info(const char *msg, ...) WTF_INTERNAL
    __attribute__ ((format (printf, 1, 2)));


#ifdef __cplusplus
}
#endif

#ifndef __STRING
#define __STRING(x) #x
#endif

#ifdef NDEBUG

#define PC_ASSERT(cond)                                 \
    do {                                                \
        if (!(cond)) {                                  \
            pcutils_error("PurC assert failed.\n");     \
            abort();                                    \
        }                                               \
    } while (0)

#else /* define NDEBUG */

#define PC_ASSERT(cond)                                                                         \
    do {                                                                                        \
        if (!(cond)) {                                                                          \
            pcutils_error("PurC assert failure %s:%d: condition \"" __STRING(cond) " failed\n", \
                     __FILE__, __LINE__);                                                       \
            assert(0);                                                                          \
        }                                                                                       \
    } while (0)

#endif /* not defined NDEBUG */

#define PC_ERROR(x, ...) pcutils_error(x, ##__VA_ARGS__)

#ifndef NDEBUG

# define PC_ENABLE_DEBUG(x) pcutils_enable_debug(x)
# define PC_ENABLE_SYSLOG(x) pcutils_enable_syslog(x)
# define PC_DEBUG(x, ...) pcutils_debug(x, ##__VA_ARGS__)
# define PC_INFO(x, ...) pcutils_info(x, ##__VA_ARGS__)

#else /* not defined NDEBUG */

# define PC_ENABLE_DEBUG(x)             \
    if (0)                              \
        pcutils_enable_debug(x)

# define PC_ENABLE_SYSLOG(x)            \
    if (0)                              \
        pcutils_set_syslog(x)

#define PC_DEBUG(x, ...)                \
    if (0)                              \
        pcutils_debug(x, ##__VA_ARGS__)

#define PC_INFO(x, ...)                 \
    if (0)                              \
        pcutils_info(x, ##__VA_ARGS__)

#endif /* defined NDEBUG */

#ifndef _D            /* { */
/* for test-case to use, because of WTF_INTERNAL for pcutils_info/error/... */
#define _D(fmt, ...)                                           \
    if (TO_DEBUG) {                                           \
        fprintf(stderr, "%s[%d]:%s(): " fmt "\n",             \
            basename((char*)__FILE__), __LINE__, __func__,    \
            ##__VA_ARGS__);                                   \
    }
#endif                /* } */

#endif /* PURC_PRIVATE_DEBUG_H */
