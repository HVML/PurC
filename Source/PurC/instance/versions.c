/*
 * @file versions.c
 * @author Vincent Wei (https://github.com/VincentWei)
 * @date 2023/10/25
 * @brief The versions of HVML and PurC.
 *
 * Copyright (C) 2023 FMSoft <https://www.fmsoft.cn>
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

#define _GNU_SOURCE
#include "config.h"

#include "purc/purc-version.h"

int
purc_get_major_version(void)
{
    return PURC_VERSION_MAJOR;
}

int
purc_get_minor_version(void)
{
    return PURC_VERSION_MINOR;
}

int
purc_get_micro_version(void)
{
    return PURC_VERSION_MICRO;
}

void
purc_get_versions(int *major, int *minor, int *micro) {
    if (major) *major = PURC_VERSION_MAJOR;
    if (minor) *minor = PURC_VERSION_MINOR;
    if (micro) *micro = PURC_VERSION_MICRO;
}

const char *
purc_get_version_string(void) {
    return PURC_VERSION_STRING;
}

const char *
purc_get_api_version_string(void) {
    return PURC_API_VERSION_STRING;
}

int
purc_get_hvml_platform_version(void) {
    return HVML_PLATFORM_VERSION;
}

