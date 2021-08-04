/**
 * @file def.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for whole html core.
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
 * This implementation of HTML parser is derived from Lexbor
 * <https://github.com/lexbor/lexbor>, which is licensed under the Apache 
 * License Version 2.0:
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

#ifndef PCHTML_DEF_H
#define PCHTML_DEF_H

#define PCHTML_STRINGIZE_HELPER(x) #x
#define PCHTML_STRINGIZE(x) PCHTML_STRINGIZE_HELPER(x)

/* Format */
#ifdef _WIN32
    #define PCHTML_FORMAT_Z "%Iu"
#else
    #define PCHTML_FORMAT_Z "%zu"
#endif

/* Deprecated */
#ifdef _MSC_VER
    #define PCHTML_DEPRECATED(func) __declspec(deprecated) func
#elif defined(__GNUC__) || defined(__INTEL_COMPILER)
    #define PCHTML_DEPRECATED(func) func __attribute__((deprecated))
#else
    #define PCHTML_DEPRECATED(func) func
#endif

/* Debug */
//#define PCHTML_DEBUG(...) do {} while (0)
//#define PCHTML_DEBUG_ERROR(...) do {} while (0)

#define PCHTML_MEM_ALIGN_STEP sizeof(void *)

#ifndef PCHTML_STATIC
    #ifdef _WIN32
        #ifdef PCHTML_SHARED
            #define PCHTML_API __declspec(dllexport)
        #else
            #define PCHTML_API __declspec(dllimport)
        #endif
    #elif (defined(__SUNPRO_C)  || defined(__SUNPRO_CC))
        #define PCHTML_API __global
    #else
        #if (defined(__GNUC__) && __GNUC__ >= 4) || defined(__INTEL_COMPILER)
            #define PCHTML_API __attribute__ ((visibility("default")))
        #else
            #define PCHTML_API
        #endif
    #endif
#else
    #define PCHTML_API
#endif

#ifdef _WIN32
    #define PCHTML_EXTERN extern __declspec(dllimport)
#else
    #define PCHTML_EXTERN extern
#endif

#endif  /* PCHTML_DEF_H */
