/*
** Copyright (C) 2015-2017 Alexander Borisov
**
** This file is a part of PurC (short for Purring Cat), an HVML parser
** and interpreter.
** 
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
**
** Author: lex.borisov@gmail.com (Alexander Borisov)
*/

#ifndef PURC_PURC_VERSION_H
#define PURC_PURC_VERSION_H

#pragma once

/**
 * @file purc_version.h
 *
 * Version of Purring Cat 2.
 */

#define PURC_VERSION_MAJOR 0
#define PURC_VERSION_MINOR 0
#define PURC_VERSION_PATCH 1

#define PURC_VERSION_STRING MyCORE_STR(PURC_VERSION_MAJOR) MyCORE_STR(.) MyCORE_STR(PURC_VERSION_MINOR) MyCORE_STR(.) MyCORE_STR(PURC_VERSION_PATCH)

/***********************************************************************************
 *
 * PURC_VERSION
 *
 ***********************************************************************************/

/**
 * @struct purc_version_t
 */
struct purc_version {
    int major;
    int minor;
    int patch;
}
typedef purc_version_t;
    
/**
 * Get current version
 *
 * @return purc_version_t
 */
static inline purc_version_t purc_version(void)
{
    return (purc_version_t) {
        PURC_VERSION_MAJOR, PURC_VERSION_MINOR, PURC_VERSION_PATCH };
}

#endif /* PURC_PURC_VERSION_H */

