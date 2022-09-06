/**
 * @file purc-macros.h
 * @author Vincent Wei (https://github.com/VincentWei)
 * @date 2021/07/03
 * @brief Some macros of PurC.
 *
 * Copyright (C) 2021, 2022 FMSoft <https://www.fmsoft.cn>
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

#ifndef PURC_PURC_MACROS_H
#define PURC_PURC_MACROS_H

#ifndef __has_declspec_attribute
#define __has_declspec_attribute(x) 0
#endif

#undef PCA_EXPORT
#if defined(PCA_NO_EXPORT)
#define PCA_EXPORT
#elif defined(WIN32) || defined(_WIN32) || defined(__CC_ARM) || defined(__ARMCC__) || (__has_declspec_attribute(dllimport) && __has_declspec_attribute(dllexport))
#if BUILDING_PurC
#define PCA_EXPORT __declspec(dllexport)
#else
#define PCA_EXPORT __declspec(dllimport)
#endif /* BUILDING_WebKit */
#elif defined(__GNUC__)
#define PCA_EXPORT __attribute__((visibility("default")))
#else /* !defined(PCA_NO_EXPORT) */
#define PCA_EXPORT
#endif /* defined(PCA_NO_EXPORT) */

#if !defined(PCA_INLINE)
#if defined(__cplusplus)
#define PCA_INLINE static inline
#elif defined(__GNUC__)
#define PCA_INLINE static __inline__
#else
#define PCA_INLINE static
#endif
#endif /* !defined(PCA_INLINE) */

#ifndef __has_extension
#define __has_extension(x) 0
#endif

#if __has_extension(enumerator_attributes) && __has_extension(attribute_unavailable_with_message)
#define PCA_C_DEPRECATED(message) __attribute__((deprecated(message)))
#else
#define PCA_C_DEPRECATED(message)
#endif

#ifndef __has_attribute
#define __has_attribute(x) 0
#endif

#if __has_attribute(unavailable)
#define PCA_UNAVAILABLE __attribute__((unavailable))
#else
#define PCA_UNAVAILABLE
#endif

#if __has_attribute(format)
#define PCA_ATTRIBUTE_PRINTF(formatStringArgument, extraArguments) __attribute__((format(printf, formatStringArgument, extraArguments)))
#else
#define PCA_ATTRIBUTE_PRINTF
#endif

#ifdef __cplusplus
#define PCA_EXTERN_C_BEGIN extern "C" {
#define PCA_EXTERN_C_END }
#else
#define PCA_EXTERN_C_BEGIN
#define PCA_EXTERN_C_END
#endif

#define PCA_TABLESIZE(table)    (sizeof(table)/sizeof(table[0]))

#endif /*  PURC_PURC_MACROS_H */
