/*
 * @file utils.h
 * @author Vincent Wei (https://github.com/VincentWei)
 * @date 2021/07/05
 * @brief The internal utility interfaces.
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

#ifndef PURC_PRIVATE_UTILS_H
#define PURC_PRIVATE_UTILS_H

#include "config.h"

#include <stddef.h>
#include <stdarg.h>

/*
 * calloc_a(size_t len, [void **addr, size_t len,...], NULL)
 *
 * allocate a block of memory big enough to hold multiple aligned objects.
 * the pointer to the full object (starting with the first chunk) is returned,
 * all other pointers are stored in the locations behind extra addr arguments.
 * the last argument needs to be a NULL pointer
 */
#define calloc_a(len, ...) pcutils_calloc_a(len, ##__VA_ARGS__, NULL)

#ifdef __cplusplus
extern "C" {
#endif

#if !HAVE(VASPRINTF)
int vasprintf(char **buf, const char *fmt, va_list ap)
    __attribute__ ((format (gnu_printf, 2, 0)));
#endif

void pcutils_init_atom(void) WTF_INTERNAL;

void *pcutils_calloc_a(size_t len, ...);

/* hex must be long enough to hold the heximal characters */
void pcutils_bin2hex (const unsigned char *bin, int len, char *hex);

/* bin must be long enough to hold the bytes.
   return the number of bytes converted, <= 0 for error */
int pcutils_hex2bin (const char *hex, unsigned char *bin);

size_t pcutils_get_cmdline_arg (int arg, char* buf, size_t sz_buf);

#ifdef __cplusplus
}
#endif

#endif /* not defined PURC_PRIVATE_UTILS_H */

