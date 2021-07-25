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
typedef uint32_t      uint32_t;
typedef unsigned char unsigned char;
typedef unsigned int  unsigned int;
#endif

/* Callbacks */
typedef pchtml_status_t (*pchtml_callback_f)(const unsigned char *buffer,
                                          size_t size, void *ctx);


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif /* PCHTML_TYPES_H */
