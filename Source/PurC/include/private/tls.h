/*
 * @file tls.h
 * @author Vincent Wei (https://github.com/VincentWei)
 * @date 2021/07/04
 * @brief The portable implementation of Thread Local Storage.
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

#ifndef PURC_PRIVATE_TLS_H
#define PURC_PRIVATE_TLS_H

#include "config.h"

#include <wtf/TLSKeyword.h>

#if HAVE(TLS_KEYWORD)

#if HAVE_TLS_KEYWORD == TLS_KEYWORD_TYPE_STORAGE_CLASS
#   define PURC_DEFINE_THREAD_LOCAL(type, name)                         \
    static __thread type name
#elif HAVE_TLS_KEYWORD == TLS_KEYWORD_TYPE_DECL_SPEC 
#   define PURC_DEFINE_THREAD_LOCAL(type, name)                         \
    static type __declspec(thread) name
#else
#   error "SHOULD NEVER BE REACHED"
#endif

#   define PURC_GET_THREAD_LOCAL(name)                                  \
    (&name)

#elif COMPILER(MINGW)

#   define _NO_W32_PSEUDO_MODIFIERS
#   include <windows.h>

#   define PURC_DEFINE_THREAD_LOCAL(type, name)                         \
    static volatile int tls_ ## name ## _initialized = 0;               \
    static void *tls_ ## name ## _mutex = NULL;                         \
    static unsigned tls_ ## name ## _index;                             \
                                                                        \
    static type *                                                       \
    tls_ ## name ## _alloc (void)                                       \
    {                                                                   \
        type *value = calloc (1, sizeof (type));                        \
        if (value)                                                      \
            TlsSetValue (tls_ ## name ## _index, value);                \
        return value;                                                   \
    }                                                                   \
                                                                        \
    static ALWAYS_INLINE type *                                         \
    tls_ ## name ## _get (void)                                         \
    {                                                                   \
        type *value;                                                    \
        if (!tls_ ## name ## _initialized)                              \
        {                                                               \
            if (!tls_ ## name ## _mutex)                                \
            {                                                           \
                void *mutex = CreateMutexA (NULL, 0, NULL);             \
                if (InterlockedCompareExchangePointer (                 \
                        &tls_ ## name ## _mutex, mutex, NULL) != NULL)  \
                {                                                       \
                    CloseHandle (mutex);                                \
                }                                                       \
            }                                                           \
            WaitForSingleObject (tls_ ## name ## _mutex, 0xFFFFFFFF);   \
            if (!tls_ ## name ## _initialized)                          \
            {                                                           \
                tls_ ## name ## _index = TlsAlloc ();                   \
                tls_ ## name ## _initialized = 1;                       \
            }                                                           \
            ReleaseMutex (tls_ ## name ## _mutex);                      \
        }                                                               \
        if (tls_ ## name ## _index == 0xFFFFFFFF)                       \
            return NULL;                                                \
        value = TlsGetValue (tls_ ## name ## _index);                   \
        if (!value)                                                     \
            value = tls_ ## name ## _alloc ();                          \
        return value;                                                   \
    }

#   define PURC_GET_THREAD_LOCAL(name)                                  \
    tls_ ## name ## _get ()

#elif COMPILER(MSVC)

#   define PURC_DEFINE_THREAD_LOCAL(type, name)                         \
    static __declspec(thread) type name
#   define PURC_GET_THREAD_LOCAL(name)                                  \
    (&name)

#elif defined(HAVE_PTHREADS)

#include <pthread.h>

#  define PURC_DEFINE_THREAD_LOCAL(type, name)                          \
    static pthread_once_t tls_ ## name ## _once_control = PTHREAD_ONCE_INIT; \
    static pthread_key_t tls_ ## name ## _key;                          \
                                                                        \
    static void                                                         \
    tls_ ## name ## _destroy_value (void *value)                        \
    {                                                                   \
        free (value);                                                   \
    }                                                                   \
                                                                        \
    static void                                                         \
    tls_ ## name ## _make_key (void)                                    \
    {                                                                   \
        pthread_key_create (&tls_ ## name ## _key,                      \
                            tls_ ## name ## _destroy_value);            \
    }                                                                   \
                                                                        \
    static type *                                                       \
    tls_ ## name ## _alloc (void)                                       \
    {                                                                   \
        type *value = calloc (1, sizeof (type));                        \
        if (value)                                                      \
            pthread_setspecific (tls_ ## name ## _key, value);          \
        return value;                                                   \
    }                                                                   \
                                                                        \
    static ALWAYS_INLINE type *                                         \
    tls_ ## name ## _get (void)                                         \
    {                                                                   \
        type *value = NULL;                                             \
        if (pthread_once (&tls_ ## name ## _once_control,               \
                          tls_ ## name ## _make_key) == 0)              \
        {                                                               \
            value = pthread_getspecific (tls_ ## name ## _key);         \
            if (!value)                                                 \
                value = tls_ ## name ## _alloc ();                      \
        }                                                               \
        return value;                                                   \
    }

#   define PURC_GET_THREAD_LOCAL(name)                                  \
    tls_ ## name ## _get ()

#else

#   define PURC_DEFINE_THREAD_LOCAL(type, name)                         \
    static type name
#   define PURC_GET_THREAD_LOCAL(name)                                  \
    (&name)

#    warn "No TLS (thread local storage) support for this system. PurC will work without multiple threads."

#endif

#endif /* not defined PURC_PRIVATE_TLS_H */

