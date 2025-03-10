/*
 * @file file.c
 * @author Vincent Wei
 * @date 2022/07/04
 * @brief The portable implementation about file.
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

#include "config.h"

#include "purc-ports.h"
#include "private/ports.h"
#include "private/utils.h"

#if OS(LINUX) || OS(UNIX)
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

bool pcutils_file_md5(const char *pathname, unsigned char *md5_buf, size_t *sz)
{
    struct stat statbuf;

    if (stat(pathname, &statbuf))
        return false;

    char buff[128];
    snprintf(buff, sizeof(buff), "%llx-%llx-%llx-%llx",
            (unsigned long long)statbuf.st_dev,
            (unsigned long long)statbuf.st_ino,
            (unsigned long long)statbuf.st_size,
            (unsigned long long)statbuf.st_mtime);
    pcutils_md5digest(buff, md5_buf);
    if (sz)
        *sz = statbuf.st_size;
    return true;
}

#else
#error "Not implemented for this platform."
#endif

