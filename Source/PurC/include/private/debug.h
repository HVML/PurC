/*
 * @file debug.h
 * @author Vincent Wei
 * @date 2021/07/07
 * @brief The internal interfaces for debug.
 *
 * Copyright (C) 2021, 2022 FMSoft <https://www.fmsoft.cn>
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

#include "purc/purc-helpers.h"

#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#ifndef __STRING
#define __STRING(x) #x
#endif

#ifdef NDEBUG

#define PC_ASSERT(cond)                                 \
    do {                                                \
        if (0 && !(cond)) {                             \
            /* do nothing */                            \
        }                                               \
    } while (0)

#else /* define NDEBUG */

#define PC_ASSERT(cond)                                                 \
    do {                                                                \
        if (!(cond)) {                                                  \
            purc_log_error("PurC assertion failure %s:%d: condition ‘"  \
                    __STRING(cond) "’ failed\n",                        \
                    __FILE__, __LINE__);                                \
            assert(0);                                                  \
        }                                                               \
    } while (0)

#endif /* not defined NDEBUG */

#define PC_ENABLE_DEBUG(x)  purc_enable_log(x, true)
#define PC_ENABLE_SYSLOG(x) purc_enable_log(x?true:false, x)
#define PC_ERROR(x, ...)    purc_log_error(x, ##__VA_ARGS__)
#define PC_WARN(x, ...)     purc_log_warn(x, ##__VA_ARGS__)
#define PC_NOTICE(x, ...)   purc_log_notice(x, ##__VA_ARGS__)
#define PC_INFO(x, ...)     purc_log_info(x, ##__VA_ARGS__)

#define PC_TIMESTAMP(x, ...)                                                \
    do {                                                                    \
        FILE* fp = fopen("/tmp/purc_run.log", "a+");                        \
        fprintf(fp, "timestamp: %ld : %s[%d]:%s(): " x ,                     \
            pcutils_get_monotoic_time_ms(),                                 \
            pcutils_basename(__FILE__), __LINE__, __func__, ##__VA_ARGS__); \
        fclose(fp);    \
    } while (0)

#ifndef NDEBUG

#define PC_DEBUG(x, ...)    purc_log_debug(x, ##__VA_ARGS__)

#define PC_DEBUGX(x, ...)                                                  \
    purc_log_debug("%s[%d]:%s(): " x "\n",                                 \
            pcutils_basename(__FILE__), __LINE__, __func__, ##__VA_ARGS__)

#else /* not defined NDEBUG */

#define PC_DEBUG(x, ...)                \
    if (0)                              \
        purc_log_debug(x, ##__VA_ARGS__)

#define PC_DEBUGX(x, ...)                                                   \
    if (0)                                                                  \
        purc_log_debug("%s[%d]:%s(): " x "\n",                              \
                pcutils_basename(__FILE__), __LINE__, __func__, ##__VA_ARGS__)

#endif /* defined NDEBUG */

struct pcdebug_backtrace {
    int         refc;

    const char *file;
    int line;
    const char *func;

#ifndef NDEBUG                     /* { */
#if OS(LINUX)                      /* { */
    void       *c_stacks[64];
    int         nr_stacks;
#endif                             /* } */
#endif                             /* } */
};

struct pcdebug_backtrace*
pcdebug_backtrace_ref(struct pcdebug_backtrace *bt) WTF_INTERNAL;

void
pcdebug_backtrace_unref(struct pcdebug_backtrace *bt) WTF_INTERNAL;

void
pcdebug_backtrace_dump(struct pcdebug_backtrace *bt) WTF_INTERNAL;

#endif /* PURC_PRIVATE_DEBUG_H */
