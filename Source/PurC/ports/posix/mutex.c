/*
 * @file mutex.c
 * @author Vincent Wei
 * @date 2021/07/07
 * @brief The portable implementation of mutext.
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

#include "config.h"

#if USE(PTHREADS)

#include "purc-ports.h"

#include <stdlib.h>
#include <pthread.h>

void purc_mutex_init (purc_mutex  *mutex)
{
    mutex->native_impl = malloc (sizeof (pthread_mutex_t));
    if (mutex->native_impl) {
        if (pthread_mutex_init (mutex->native_impl, NULL) != 0) {
            pthread_mutex_destroy (mutex->native_impl);
            free (mutex->native_impl);
            mutex->native_impl = NULL;
        }
    }
}

void purc_mutex_clear (purc_mutex *mutex)
{
    pthread_mutex_destroy (mutex->native_impl);
    free (mutex->native_impl);
    mutex->native_impl = NULL;
}

void purc_mutex_lock (purc_mutex *mutex)
{
    pthread_mutex_lock (mutex->native_impl);
}

bool purc_mutex_trylock (purc_mutex *mutex)
{
    if (pthread_mutex_trylock (mutex->native_impl) == 0)
        return true;
    return false;
}

void purc_mutex_unlock (purc_mutex *mutex)
{
    pthread_mutex_unlock (mutex->native_impl);
}

#endif /* HAVE(PTHREADS) */
