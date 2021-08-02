/**
 * @file utils.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for html parser.
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
 *
 * This implementation of HTML parser is derived from Lexbor <http://lexbor.com/>.
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

#ifndef PCHTML_UTILS_H
#define PCHTML_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/core_base.h"


#define pchtml_utils_whitespace(onechar, action, logic)                        \
    (onechar action ' '  logic                                                 \
     onechar action '\t' logic                                                 \
     onechar action '\n' logic                                                 \
     onechar action '\f' logic                                                 \
     onechar action '\r')


size_t pchtml_utils_power(size_t t, size_t k) WTF_INTERNAL;

size_t 
pchtml_utils_hash_hash(const unsigned char *key, size_t key_size) WTF_INTERNAL;


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_UTILS_H */
