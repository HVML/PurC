/**
 * @file type.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for data type.
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


#ifndef PCHTML_TYPES_H
#define PCHTML_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* Inline */
#ifdef _MSC_VER
    #define static inline static __forceinline
#else
//    #define static inline static inline
#endif


#if 0
/* Simple types */
typedef uint32_t      pchtml_codepoint_t;
typedef unsigned char pchtml_char_t;
typedef unsigned int  pchtml_status_t;
#endif

/* Callbacks */
typedef unsigned int (*pchtml_callback_f)(const unsigned char *buffer,
                                          size_t size, void *ctx);


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif /* PCHTML_TYPES_H */
