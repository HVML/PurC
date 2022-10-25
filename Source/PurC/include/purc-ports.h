/**
 * @file purc-ports.h
 * @author Vincent Wei
 * @date 2021/07/05
 * @brief The portability API.
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

#ifndef PURC_PURC_PORTS_H
#define PURC_PURC_PORTS_H

#include <stdbool.h>
#include <stddef.h>

#include "purc-macros.h"

typedef struct purc_mutex {
    void *native_impl;
} purc_mutex;

typedef struct purc_rwlock {
    void *native_impl;
} purc_rwlock;

PCA_EXTERN_C_BEGIN

PCA_EXPORT
void purc_mutex_init (purc_mutex *mutex);

PCA_EXPORT
void purc_mutex_clear (purc_mutex *mutex);

PCA_EXPORT
void purc_mutex_lock  (purc_mutex *mutex);

PCA_EXPORT
bool purc_mutex_trylock (purc_mutex *mutex);

PCA_EXPORT
void purc_mutex_unlock (purc_mutex *mutex);

PCA_EXPORT
void purc_rwlock_init (purc_rwlock  *rw_lock);

PCA_EXPORT
void purc_rwlock_clear (purc_rwlock *rw_lock);

PCA_EXPORT
void purc_rwlock_writer_lock (purc_rwlock *rw_lock);

PCA_EXPORT
bool purc_rwlock_writer_trylock (purc_rwlock *rw_lock);

PCA_EXPORT
void purc_rwlock_writer_unlock (purc_rwlock *rw_lock);

PCA_EXPORT
void purc_rwlock_reader_lock (purc_rwlock *rw_lock);

PCA_EXPORT
bool purc_rwlock_reader_trylock (purc_rwlock *rw_lock);

PCA_EXPORT
void purc_rwlock_reader_unlock (purc_rwlock *rw_lock);

unsigned int pcutils_sleep(unsigned int seconds);
int pcutils_usleep(unsigned long long usec);

PCA_EXTERN_C_END

#endif /* not defined PURC_PURC_PORTS_H */

