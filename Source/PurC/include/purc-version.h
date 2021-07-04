/**
 * @file purc-version.h
 * @author Vincent Wei (https://github.com/VincentWei)
 * @date 2021/07/02
 * @brief The version of PurC.
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

#pragma once

#define PURC_VERSION_MAJOR 0
#define PURC_VERSION_MINOR 0
#define PURC_VERSION_MICRO 1

#define PURC_VERSION_STRING "0.0.1"

PURC_EXTERN_C_BEGIN

static inline void
purc_version (unsigned int *major, unsigned int *minor, unsigned int *micro) {
    if (major) *major = PURC_VERSION_MAJOR;
    if (minor) *minor = PURC_VERSION_MINOR;
    if (micro) *micro = PURC_VERSION_MICRO;
}

static inline const char *
purc_version_string (void) {
    return PURC_VERSION_STRING;
}

PURC_EXTERN_C_END

