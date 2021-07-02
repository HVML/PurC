/**
 * @file purc_macros.h
 * @author Vincent Wei (https://github.com/VincentWei)
 * @date 2021/07/02
 * @brief Some macros related to compiler features.
 *
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of PurC (short for Purring Cat), an HVML parser
 * and interpreter.
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

#pragma once

/* COMPILER() - the compiler being used to build the project */
#define COMPILER(PURC_FEATURE) (defined PURC_COMPILER_##PURC_FEATURE  && PURC_COMPILER_##PURC_FEATURE)

/* COMPILER_SUPPORTS() - whether the compiler being used to build the project supports the given feature. */
#define COMPILER_SUPPORTS(PURC_COMPILER_FEATURE) (defined PURC_COMPILER_SUPPORTS_##PURC_COMPILER_FEATURE  && PURC_COMPILER_SUPPORTS_##PURC_COMPILER_FEATURE)

/* COMPILER_QUIRK() - whether the compiler being used to build the project requires a given quirk. */
#define COMPILER_QUIRK(PURC_COMPILER_QUIRK) (defined PURC_COMPILER_QUIRK_##PURC_COMPILER_QUIRK  && PURC_COMPILER_QUIRK_##PURC_COMPILER_QUIRK)

/* COMPILER_HAS_CLANG_BUILTIN() - whether the compiler supports a particular clang builtin. */
#ifdef __has_builtin
#define COMPILER_HAS_CLANG_BUILTIN(x) __has_builtin(x)
#else
#define COMPILER_HAS_CLANG_BUILTIN(x) 0
#endif

/* COMPILER_HAS_CLANG_FEATURE() - whether the compiler supports a particular language or library feature. */
/* http://clang.llvm.org/docs/LanguageExtensions.html#has-feature-and-has-extension */
#ifdef __has_feature
#define COMPILER_HAS_CLANG_FEATURE(x) __has_feature(x)
#else
#define COMPILER_HAS_CLANG_FEATURE(x) 0
#endif

/* COMPILER_HAS_CLANG_DECLSPEC() - whether the compiler supports a Microsoft style __declspec attribute. */
/* https://clang.llvm.org/docs/LanguageExtensions.html#has-declspec-attribute */
#ifdef __has_declspec_attribute
#define COMPILER_HAS_CLANG_DECLSPEC(x) __has_declspec_attribute(x)
#else
#define COMPILER_HAS_CLANG_DECLSPEC(x) 0
#endif

/* ==== COMPILER() - primary detection of the compiler being used to build the project, in alphabetical order ==== */

/* COMPILER(CLANG) - Clang  */

#if defined(__clang__)
#define PURC_COMPILER_CLANG 1
#define PURC_COMPILER_SUPPORTS_BLOCKS COMPILER_HAS_CLANG_FEATURE(blocks)
#define PURC_COMPILER_SUPPORTS_C_STATIC_ASSERT COMPILER_HAS_CLANG_FEATURE(c_static_assert)
#define PURC_COMPILER_SUPPORTS_CXX_EXCEPTIONS COMPILER_HAS_CLANG_FEATURE(cxx_exceptions)
#define PURC_COMPILER_SUPPORTS_BUILTIN_IS_TRIVIALLY_COPYABLE COMPILER_HAS_CLANG_FEATURE(is_trivially_copyable)

#ifdef __cplusplus
#if __cplusplus <= 201103L
#define PURC_CPP_STD_VER 11
#elif __cplusplus <= 201402L
#define PURC_CPP_STD_VER 14
#elif __cplusplus <= 201703L
#define PURC_CPP_STD_VER 17
#endif
#endif

#endif // defined(__clang__)

/* COMPILER(GCC_COMPATIBLE) - GNU Compiler Collection or compatibles */
#if defined(__GNUC__)
#define PURC_COMPILER_GCC_COMPATIBLE 1
#endif

/* COMPILER(GCC) - GNU Compiler Collection */
/* Note: This section must come after the Clang section since we check !COMPILER(CLANG) here. */
#if COMPILER(GCC_COMPATIBLE) && !COMPILER(CLANG)
#define PURC_COMPILER_GCC 1

#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#define GCC_VERSION_AT_LEAST(major, minor, patch) (GCC_VERSION >= (major * 10000 + minor * 100 + patch))

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define PURC_COMPILER_SUPPORTS_C_STATIC_ASSERT 1
#endif

#endif /* COMPILER(GCC) */

#if COMPILER(GCC_COMPATIBLE) && defined(NDEBUG) && !defined(__OPTIMIZE__) && !defined(RELEASE_WITHOUT_OPTIMIZATIONS)
#error "Building release without compiler optimizations: xGUI will be slow. Set -DRELEASE_WITHOUT_OPTIMIZATIONS if this is intended."
#endif

/* COMPILER(MINGW) - MinGW GCC */

#if defined(__MINGW32__)
#define PURC_COMPILER_MINGW 1
#include <_mingw.h>
#endif

/* COMPILER(MINGW64) - mingw-w64 GCC - used as additional check to exclude mingw.org specific functions */

/* Note: This section must come after the MinGW section since we check COMPILER(MINGW) here. */

#if COMPILER(MINGW) && defined(__MINGW64_VERSION_MAJOR) /* best way to check for mingw-w64 vs mingw.org */
#define PURC_COMPILER_MINGW64 1
#endif

/* COMPILER(MSVC) - Microsoft Visual C++ */

#if defined(_MSC_VER)

#define PURC_COMPILER_MSVC 1

#if _MSC_VER < 1910
#error "Please use a newer version of Visual Studio. xGUI requires VS2017 or newer to compile."
#endif

#endif

#if !COMPILER(CLANG) && !COMPILER(MSVC)
#define PURC_COMPILER_QUIRK_CONSIDERS_UNREACHABLE_CODE 1
#endif

/* ==== COMPILER_SUPPORTS - additional compiler feature detection, in alphabetical order ==== */

/* COMPILER_SUPPORTS(EABI) */

#if defined(__ARM_EABI__) || defined(__EABI__)
#define PURC_COMPILER_SUPPORTS_EABI 1
#endif

/* ASAN_ENABLED and SUPPRESS_ASAN */

#ifdef __SANITIZE_ADDRESS__
#define ASAN_ENABLED 1
#else
#define ASAN_ENABLED COMPILER_HAS_CLANG_FEATURE(address_sanitizer)
#endif

#if ASAN_ENABLED
#define SUPPRESS_ASAN __attribute__((no_sanitize_address))
#else
#define SUPPRESS_ASAN
#endif

/* ==== Compiler-independent macros for various compiler features, in alphabetical order ==== */

/* ALWAYS_INLINE */

/* In GCC functions marked with no_sanitize_address cannot call functions that are marked with always_inline and not marked with no_sanitize_address.
 * Therefore we need to give up on the enforcement of ALWAYS_INLINE when bulding with ASAN. https://gcc.gnu.org/bugzilla/show_bug.cgi?id=67368 */
#if !defined(ALWAYS_INLINE) && COMPILER(GCC_COMPATIBLE) && defined(NDEBUG) && !COMPILER(MINGW) && !(COMPILER(GCC) && ASAN_ENABLED)
#define ALWAYS_INLINE inline __attribute__((__always_inline__))
#endif

#if !defined(ALWAYS_INLINE) && COMPILER(MSVC) && defined(NDEBUG)
#define ALWAYS_INLINE __forceinline
#endif

#if !defined(ALWAYS_INLINE)
#define ALWAYS_INLINE inline
#endif

#if COMPILER(MSVC)
#define ALWAYS_INLINE_EXCEPT_MSVC inline
#else
#define ALWAYS_INLINE_EXCEPT_MSVC ALWAYS_INLINE
#endif

/* PURC_EXTERN_C_{BEGIN, END} */

#ifdef __cplusplus
#define PURC_EXTERN_C_BEGIN extern "C" {
#define PURC_EXTERN_C_END }
#else
#define PURC_EXTERN_C_BEGIN
#define PURC_EXTERN_C_END
#endif

/* FALLTHROUGH */

#if !defined(FALLTHROUGH) && defined(__cplusplus) && defined(__has_cpp_attribute)

#if __has_cpp_attribute(fallthrough)
#define FALLTHROUGH [[fallthrough]]
#elif __has_cpp_attribute(clang::fallthrough)
#define FALLTHROUGH [[clang::fallthrough]]
#elif __has_cpp_attribute(gnu::fallthrough)
#define FALLTHROUGH [[gnu::fallthrough]]
#endif

#elif !defined(FALLTHROUGH) && !defined(__cplusplus)

#if COMPILER(GCC_COMPATIBLE) && defined(__has_attribute)
// Break out this #if to satisy some versions Windows compilers.
#if __has_attribute(fallthrough)
#define FALLTHROUGH __attribute__ ((fallthrough))
#endif
#endif

#endif // !defined(FALLTHROUGH) && defined(__cplusplus) && defined(__has_cpp_attribute)

#if !defined(FALLTHROUGH)
#define FALLTHROUGH
#endif

/* LIKELY */

#if !defined(LIKELY) && COMPILER(GCC_COMPATIBLE)
#define LIKELY(x) __builtin_expect(!!(x), 1)
#endif

#if !defined(LIKELY)
#define LIKELY(x) (x)
#endif

/* NEVER_INLINE */

#if !defined(NEVER_INLINE) && COMPILER(GCC_COMPATIBLE)
#define NEVER_INLINE __attribute__((__noinline__))
#endif

#if !defined(NEVER_INLINE) && COMPILER(MSVC)
#define NEVER_INLINE __declspec(noinline)
#endif

#if !defined(NEVER_INLINE)
#define NEVER_INLINE
#endif

/* NO_RETURN */

#if !defined(NO_RETURN) && COMPILER(GCC_COMPATIBLE)
#define NO_RETURN __attribute((__noreturn__))
#endif

#if !defined(NO_RETURN) && COMPILER(MSVC)
#define NO_RETURN __declspec(noreturn)
#endif

#if !defined(NO_RETURN)
#define NO_RETURN
#endif

/* NOT_TAIL_CALLED */

#if !defined(NOT_TAIL_CALLED) && defined(__has_attribute)
#if __has_attribute(not_tail_called)
#define NOT_TAIL_CALLED __attribute__((not_tail_called))
#endif
#endif

#if !defined(NOT_TAIL_CALLED)
#define NOT_TAIL_CALLED
#endif

/* RETURNS_NONNULL */
#if !defined(RETURNS_NONNULL) && COMPILER(GCC_COMPATIBLE)
#define RETURNS_NONNULL __attribute__((returns_nonnull))
#endif

#if !defined(RETURNS_NONNULL)
#define RETURNS_NONNULL
#endif

/* NO_RETURN_WITH_VALUE */

#if !defined(NO_RETURN_WITH_VALUE) && !COMPILER(MSVC)
#define NO_RETURN_WITH_VALUE NO_RETURN
#endif

#if !defined(NO_RETURN_WITH_VALUE)
#define NO_RETURN_WITH_VALUE
#endif

/* PURE_FUNCTION */

#if !defined(PURE_FUNCTION) && COMPILER(GCC_COMPATIBLE)
#define PURE_FUNCTION __attribute__((__pure__))
#endif

#if !defined(PURE_FUNCTION)
#define PURE_FUNCTION
#endif

/* MAYBE_UNUSED */

#if !defined(MAYBE_UNUSED) && COMPILER(GCC_COMPATIBLE)
#define MAYBE_UNUSED __attribute__((unused))
#endif

#if !defined(MAYBE_UNUSED)
#define MAYBE_UNUSED
#endif

/* UNUSED_FUNCTION */

#if !defined(UNUSED_FUNCTION) && COMPILER(GCC_COMPATIBLE)
#define UNUSED_FUNCTION __attribute__((unused))
#endif

#if !defined(UNUSED_FUNCTION)
#define UNUSED_FUNCTION
#endif

/* REFERENCED_FROM_ASM */

#if !defined(REFERENCED_FROM_ASM) && COMPILER(GCC_COMPATIBLE)
#define REFERENCED_FROM_ASM __attribute__((__used__))
#endif

#if !defined(REFERENCED_FROM_ASM)
#define REFERENCED_FROM_ASM
#endif

/* UNLIKELY */

#if !defined(UNLIKELY) && COMPILER(GCC_COMPATIBLE)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#endif

#if !defined(UNLIKELY)
#define UNLIKELY(x) (x)
#endif

/* UNUSED_LABEL */

/* Keep the compiler from complaining for a local label that is defined but not referenced. */
/* Helpful when mixing hand-written and autogenerated code. */

#if !defined(UNUSED_LABEL) && COMPILER(MSVC)
#define UNUSED_LABEL(label) if (false) goto label
#endif

#if !defined(UNUSED_LABEL)
#define UNUSED_LABEL(label) UNUSED_PARAM(&& label)
#endif

/* UNUSED_PARAM */

#if !defined(UNUSED_PARAM)
#define UNUSED_PARAM(variable) (void)variable
#endif

/* UNUSED_VARIABLE */
#if !defined(UNUSED_VARIABLE)
#define UNUSED_VARIABLE(variable) UNUSED_PARAM(variable)
#endif

/* WARN_UNUSED_RETURN */

#if !defined(WARN_UNUSED_RETURN) && COMPILER(GCC_COMPATIBLE)
#define WARN_UNUSED_RETURN __attribute__((__warn_unused_result__))
#endif

#if !defined(WARN_UNUSED_RETURN)
#define WARN_UNUSED_RETURN
#endif

/* DEBUGGER_ANNOTATION_MARKER */

#if !defined(DEBUGGER_ANNOTATION_MARKER) && COMPILER(GCC)
#define DEBUGGER_ANNOTATION_MARKER(name) \
    __attribute__((__no_reorder__)) void name(void) { __asm__(""); }
#endif

#if !defined(DEBUGGER_ANNOTATION_MARKER)
#define DEBUGGER_ANNOTATION_MARKER(name)
#endif

#if !defined(__has_include) && COMPILER(MSVC)
#define __has_include(path) 0
#endif

/* IGNORE_WARNINGS */

/* Can't use PURC_CONCAT() and STRINGIZE() because they are defined in
 * StdLibExtras.h, which includes this file. */
#define _COMPILER_CONCAT_I(a, b) a ## b
#define _COMPILER_CONCAT(a, b) _COMPILER_CONCAT_I(a, b)

#define _COMPILER_STRINGIZE(exp) #exp

#define _COMPILER_WARNING_NAME(warning) "-W" warning

#if COMPILER(GCC) || COMPILER(CLANG)
#define IGNORE_WARNINGS_BEGIN_COND(cond, compiler, warning) \
    _Pragma(_COMPILER_STRINGIZE(compiler diagnostic push)) \
    _COMPILER_CONCAT(IGNORE_WARNINGS_BEGIN_IMPL_, cond)(compiler, warning)

#define IGNORE_WARNINGS_BEGIN_IMPL_1(compiler, warning) \
    _Pragma(_COMPILER_STRINGIZE(compiler diagnostic ignored warning))
#define IGNORE_WARNINGS_BEGIN_IMPL_0(compiler, warning)
#define IGNORE_WARNINGS_BEGIN_IMPL_(compiler, warning)


#define IGNORE_WARNINGS_END_IMPL(compiler) _Pragma(_COMPILER_STRINGIZE(compiler diagnostic pop))

#if defined(__has_warning)
#define _IGNORE_WARNINGS_BEGIN_IMPL(compiler, warning) \
    IGNORE_WARNINGS_BEGIN_COND(__has_warning(warning), compiler, warning)
#else
#define _IGNORE_WARNINGS_BEGIN_IMPL(compiler, warning) IGNORE_WARNINGS_BEGIN_COND(1, compiler, warning)
#endif

#define IGNORE_WARNINGS_BEGIN_IMPL(compiler, warning) \
    _IGNORE_WARNINGS_BEGIN_IMPL(compiler, _COMPILER_WARNING_NAME(warning))

#endif // COMPILER(GCC) || COMPILER(CLANG)


#if COMPILER(GCC)
#define IGNORE_GCC_WARNINGS_BEGIN(warning) IGNORE_WARNINGS_BEGIN_IMPL(GCC, warning)
#define IGNORE_GCC_WARNINGS_END IGNORE_WARNINGS_END_IMPL(GCC)
#else
#define IGNORE_GCC_WARNINGS_BEGIN(warning)
#define IGNORE_GCC_WARNINGS_END
#endif

#if COMPILER(CLANG)
#define IGNORE_CLANG_WARNINGS_BEGIN(warning) IGNORE_WARNINGS_BEGIN_IMPL(clang, warning)
#define IGNORE_CLANG_WARNINGS_END IGNORE_WARNINGS_END_IMPL(clang)
#else
#define IGNORE_CLANG_WARNINGS_BEGIN(warning)
#define IGNORE_CLANG_WARNINGS_END
#endif

#if COMPILER(GCC) || COMPILER(CLANG)
#define IGNORE_WARNINGS_BEGIN(warning) IGNORE_WARNINGS_BEGIN_IMPL(GCC, warning)
#define IGNORE_WARNINGS_END IGNORE_WARNINGS_END_IMPL(GCC)
#else
#define IGNORE_WARNINGS_BEGIN(warning)
#define IGNORE_WARNINGS_END
#endif

#define ALLOW_DEPRECATED_DECLARATIONS_BEGIN IGNORE_WARNINGS_BEGIN("deprecated-declarations")
#define ALLOW_DEPRECATED_DECLARATIONS_END IGNORE_WARNINGS_END

#define ALLOW_DEPRECATED_IMPLEMENTATIONS_BEGIN IGNORE_WARNINGS_BEGIN("deprecated-implementations")
#define ALLOW_DEPRECATED_IMPLEMENTATIONS_END IGNORE_WARNINGS_END

#define ALLOW_NEW_API_WITHOUT_GUARDS_BEGIN IGNORE_CLANG_WARNINGS_BEGIN("unguarded-availability-new")
#define ALLOW_NEW_API_WITHOUT_GUARDS_END IGNORE_CLANG_WARNINGS_END

#define ALLOW_UNUSED_PARAMETERS_BEGIN IGNORE_WARNINGS_BEGIN("unused-parameter")
#define ALLOW_UNUSED_PARAMETERS_END IGNORE_WARNINGS_END

#define ALLOW_NONLITERAL_FORMAT_BEGIN IGNORE_WARNINGS_BEGIN("format-nonliteral")
#define ALLOW_NONLITERAL_FORMAT_END IGNORE_WARNINGS_END

#define IGNORE_RETURN_TYPE_WARNINGS_BEGIN IGNORE_WARNINGS_BEGIN("return-type")
#define IGNORE_RETURN_TYPE_WARNINGS_END IGNORE_WARNINGS_END

#define IGNORE_NULL_CHECK_WARNINGS_BEGIN IGNORE_WARNINGS_BEGIN("nonnull")
#define IGNORE_NULL_CHECK_WARNINGS_END IGNORE_WARNINGS_END
#if defined(_MSC_VER)
#  define PURC_DEPRECATED(func) __declspec(deprecated) func
#elif defined(__GNUC__) || defined(__INTEL_COMPILER)
#  define PURC_DEPRECATED(func) func __attribute__((deprecated))
#else
#  define PURC_DEPRECATED(func) func
#endif

#if defined(_WIN64)
#   define SIZEOF_PTR   8
#elif defined(__LP64__)
#   define SIZEOF_PTR   8
#else
#   define SIZEOF_PTR   4
#endif

#endif /* PURC_PURC_MACROS_H */

