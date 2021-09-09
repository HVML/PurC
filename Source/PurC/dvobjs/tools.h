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
#include "purc-variant.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

struct pcdvobjs_math_param {
    double result;
    purc_variant_t v;
};

struct pcdvobjs_mathld_param {
    long double result;
    purc_variant_t v;
};

struct pcdvobjs_logical_param {
    int result;
    purc_variant_t v;
};

bool wildcard_cmp (const char *str1, const char *pattern);

const char * pcdvobjs_remove_space (char * buffer);

const char* pcdvobjs_get_next_option (const char* data, const char* delims,
                                            size_t* length) WTF_INTERNAL;
const char* pcdvobjs_get_prev_option (const char* data, size_t str_len, 
                            const char* delims, size_t* length) WTF_INTERNAL;
const char* pcdvobjs_file_get_next_option (const char* data, const char* delims,
                                            size_t* length) WTF_INTERNAL;
const char* pcdvobjs_file_get_prev_option (const char* data, size_t str_len, 
                            const char* delims, size_t* length) WTF_INTERNAL;

extern int
math_parse(const char *input, struct pcdvobjs_math_param *param);

extern int
mathld_parse(const char *input, struct pcdvobjs_mathld_param *param);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  // _DVOJBS_TOOLS_H_
