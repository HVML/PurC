/*
** Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
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
** Author: Vincent Wei <https://github.com/VincentWei>
*/

#ifndef PURC_PURC_MACROS_H
#define PURC_PURC_MACROS_H

#pragma once

/**
 * @file purc_macros.h
 *
 * Global macros.
 */

#if defined(_MSC_VER)
#  define PURC_DEPRECATED(func) __declspec(deprecated) func
#elif defined(__GNUC__) || defined(__INTEL_COMPILER)
#  define PURC_DEPRECATED(func) func __attribute__((deprecated))
#else
#  define PURC_DEPRECATED(func) func
#endif

/*
 * The PURC_LIKELY and PURC_UNLIKELY macros let the programmer give hints to
 * the compiler about the expected result of an expression. Some compilers
 * can use this information for optimizations.
 */
#if defined(__GNUC__) && (__GNUC__ > 2) && defined(__OPTIMIZE__)
#define _PURC_BOOLEAN_EXPR(expr)                  \
 PURC_GNUC_EXTENSION ({                           \
   int _g_boolean_var_;                         \
   if (expr)                                    \
      _g_boolean_var_ = 1;                      \
   else                                         \
      _g_boolean_var_ = 0;                      \
   _g_boolean_var_;                             \
})
#define PURC_LIKELY(expr) (__builtin_expect (_PURC_BOOLEAN_EXPR(expr), 1))
#define PURC_UNLIKELY(expr) (__builtin_expect (_PURC_BOOLEAN_EXPR(expr), 0))
#else
#define PURC_LIKELY(expr) (expr)
#define PURC_UNLIKELY(expr) (expr)
#endif

#if defined(_WIN64)
#   define SIZEOF_PTR   8
#elif defined(__LP64__)
#   define SIZEOF_PTR   8
#else
#   define SIZEOF_PTR   4
#endif

#endif /* PURC_PURC_MACROS_H */

