/*
 * @file rwlock.c
 * @author Vincent Wei
 * @date 2021/07/07
 * @brief The portable implementation of rwlock.
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

void purc_rwlock_init (purc_rwlock  *rw_lock)
{
    rw_lock->native_impl = malloc (sizeof (pthread_rwlock_t));
    if (rw_lock->native_impl) {
        if (pthread_rwlock_init (rw_lock->native_impl, NULL)) {
            pthread_rwlock_destroy (rw_lock->native_impl);
            free (rw_lock->native_impl);
            rw_lock->native_impl = NULL;
        }
    }
}

void purc_rwlock_clear (purc_rwlock *rw_lock)
{
    pthread_rwlock_destroy (rw_lock->native_impl);
    free (rw_lock->native_impl);
    rw_lock->native_impl = NULL;
}

void purc_rwlock_writer_lock (purc_rwlock *rw_lock)
{
    pthread_rwlock_wrlock (rw_lock->native_impl);
}

bool purc_rwlock_writer_trylock (purc_rwlock *rw_lock)
{
    if (pthread_rwlock_trywrlock (rw_lock->native_impl) == 0)
        return true;
    return false;
}

void purc_rwlock_writer_unlock (purc_rwlock *rw_lock)
{
    pthread_rwlock_unlock (rw_lock->native_impl);
}

void purc_rwlock_reader_lock (purc_rwlock *rw_lock)
{
    pthread_rwlock_rdlock (rw_lock->native_impl);
}

bool purc_rwlock_reader_trylock (purc_rwlock *rw_lock)
{
    if (pthread_rwlock_tryrdlock (rw_lock->native_impl) == 0)
        return true;
    return false;
}

void purc_rwlock_reader_unlock (purc_rwlock *rw_lock)
{
    pthread_rwlock_unlock (rw_lock->native_impl);
}

#endif /* HAVE(PTHREADS) */
