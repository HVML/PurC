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

#ifndef PCAT2_PCAT2_VERSION_H
#define PCAT2_PCAT2_VERSION_H

#pragma once

/**
 * @file pcat2_version.h
 *
 * Version of Purring Cat 2.
 */

#define PCAT2_VERSION_MAJOR 0
#define PCAT2_VERSION_MINOR 0
#define PCAT2_VERSION_PATCH 1

#define PCAT2_VERSION_STRING MyCORE_STR(PCAT2_VERSION_MAJOR) MyCORE_STR(.) MyCORE_STR(PCAT2_VERSION_MINOR) MyCORE_STR(.) MyCORE_STR(PCAT2_VERSION_PATCH)

/***********************************************************************************
 *
 * PCAT2_VERSION
 *
 ***********************************************************************************/

/**
 * @struct pcat2_version_t
 */
struct pcat2_version {
    int major;
    int minor;
    int patch;
}
typedef pcat2_version_t;
    
/**
 * Get current version
 *
 * @return pcat2_version_t
 */
static inline pcat2_version_t pcat2_version(void)
{
    return (pcat2_version_t) {
        PCAT2_VERSION_MAJOR, PCAT2_VERSION_MINOR, PCAT2_VERSION_PATCH };
}

#endif /* PCAT2_PCAT2_VERSION_H */

