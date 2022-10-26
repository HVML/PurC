/**
 * @file purc_fetcher_macros.h
 * @author Vincent Wei (https://github.com/VincentWei)
 * @date 2021/09/18
 * @brief Some useful macros of PurCFetcher.
 *
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of PurCFetcher, which contains the examples of my course:
 * _the Best Practices of C Language_.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to
 * deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef PURCFETCHER_PURCFETCHER_MACROS_H
#define PURCFETCHER_PURCFETCHER_MACROS_H

#ifndef __has_declspec_attribute
#define __has_declspec_attribute(x) 0
#endif

#undef PURCFETCHER_EXPORT
#if defined(PURCFETCHER_NO_EXPORT)
#define PURCFETCHER_EXPORT
#elif defined(WIN32) || defined(_WIN32) || \
    defined(__CC_ARM) || defined(__ARMCC__) || \
    (__has_declspec_attribute(dllimport) && __has_declspec_attribute(dllexport))
#if BUILDING_PurCFetcher
#define PURCFETCHER_EXPORT __declspec(dllexport)
#else
#define PURCFETCHER_EXPORT __declspec(dllimport)
#endif /* BUILDING_PurCFetcher */
#elif defined(__GNUC__)
#define PURCFETCHER_EXPORT __attribute__((visibility("default")))
#else /* !defined(PURCFETCHER_NO_EXPORT) */
#define PURCFETCHER_EXPORT
#endif /* defined(PURCFETCHER_NO_EXPORT) */

#if !defined(PURCFETCHER_INLINE)
#if defined(__cplusplus)
#define PURCFETCHER_INLINE static inline
#elif defined(__GNUC__)
#define PURCFETCHER_INLINE static __inline__
#else
#define PURCFETCHER_INLINE static
#endif
#endif /* !defined(PURCFETCHER_INLINE) */

#ifndef __has_extension
#define __has_extension(x) 0
#endif

#if __has_extension(enumerator_attributes) && \
    __has_extension(attribute_unavailable_with_message)
#define PURCFETCHER_C_DEPRECATED(message) __attribute__((deprecated(message)))
#else
#define PURCFETCHER_C_DEPRECATED(message)
#endif

#ifndef __has_attribute
#define __has_attribute(x) 0
#endif

#if __has_attribute(unavailable)
#define PURCFETCHER_UNAVAILABLE __attribute__((unavailable))
#else
#define PURCFETCHER_UNAVAILABLE
#endif

#ifdef __cplusplus
#define PURCFETCHER_EXTERN_C_BEGIN extern "C" {
#define PURCFETCHER_EXTERN_C_END }
#else
#define PURCFETCHER_EXTERN_C_BEGIN
#define PURCFETCHER_EXTERN_C_END
#endif

#define PURCFETCHER_TABLESIZE(table)    (sizeof(table)/sizeof(table[0]))

#endif /*  PURCFETCHER_PURCFETCHER_MACROS_H */
