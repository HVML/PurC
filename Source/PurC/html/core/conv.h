/**
 * @file conv.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for data convertion.
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

#ifndef PCHTML_CONV_H
#define PCHTML_CONV_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/core/base.h"


size_t
pchtml_conv_float_to_data(double num, unsigned char *buf, size_t len) WTF_INTERNAL;

double
pchtml_conv_data_to_double(const unsigned char **start, size_t len) WTF_INTERNAL;

unsigned long
pchtml_conv_data_to_ulong(const unsigned char **data, size_t length) WTF_INTERNAL;

unsigned
pchtml_conv_data_to_uint(const unsigned char **data, size_t length) WTF_INTERNAL;


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_CONV_H */
