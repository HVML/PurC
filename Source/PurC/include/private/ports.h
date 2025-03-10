/*
 * @file ports.h
 * @author Vincent Wei (https://github.com/VincentWei)
 * @date 2022/03/08
 * @brief The internal portability interfaces.
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

#ifndef PURC_PRIVATE_PORTS_H
#define PURC_PRIVATE_PORTS_H

#include "config.h"
#include "purc-ports.h"

#include <stddef.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t pcutils_get_cmdline_arg(int arg, char* buf, size_t sz_buf);

#if !HAVE(VASPRINTF)
WTF_ATTRIBUTE_PRINTF(2, 0)
int vasprintf(char **buf, const char *fmt, va_list ap);
#endif

/* Calculate the MD5 digest of a file by using the inode, size,
   last modification time, and so on. */
bool pcutils_file_md5(const char *pathname, unsigned char *md5_buf, size_t *sz);

#if !HAVE(STRERRORNAME_NP)
const char *strerrorname_np(int errnum);
#endif

#if !HAVE(STRNSTR)
char *strnstr(const char *haystack, const char *needle, size_t len);
#endif

#ifdef __cplusplus
}
#endif

#endif /* not defined PURC_PRIVATE_PORTS_H */

