/*
 * @file sleep.c
 * @author Vincent Wei
 * @date 2022/03/07
 * @brief The portable implementation for POSIX sleep() and usleep().
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

#if OS(LINUX) || OS(UNIX)
#include <unistd.h>

unsigned int pcutils_sleep(unsigned int seconds)
{
    return sleep(seconds);
}

int pcutils_usleep(unsigned long long usec)
{
    return usleep(usec);
}

#else
#error "Not implemented for this platform."
#endif

