/*
 * @file tools.h
 * @author Geng Yue
 * @date 2021/07/02
 * @brief The header file of tools used by all files in this directory.
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

#ifndef _DVOJBS_TOOLS_H_
#define _DVOJBS_TOOLS_H_

#include "config.h"
#include "private/debug.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

const char* get_next_option (const char* data, const char* delims,
                                            size_t* length) WTF_INTERNAL;
const char* get_prev_option (const char* data, size_t str_len, 
                            const char* delims, size_t* length) WTF_INTERNAL;

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  // _DVOJBS_TOOLS_H_
