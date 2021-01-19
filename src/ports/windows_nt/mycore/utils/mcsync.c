/*
** Copyright (C) 2021 FMSoft (https://www.fmsoft.cn)
** Copyright (C) 2015-2017 Alexander Borisov
**
** This file is a part of Purring Cat 2, a HVML parser and interpreter.
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
** Author: Vincent Wei (https://github.com/VincentWei)
** Author: lex.borisov@gmail.com (Alexander Borisov)
*/

#include "mycore/utils/mcsync.h"

#ifndef MyCORE_BUILD_WITHOUT_THREADS
#include <windows.h>

/* spinlock */
void * mcsync_spin_create(void)
{
    CRITICAL_SECTION *spinlock = mycore_calloc(1, sizeof(CRITICAL_SECTION));
    if(spinlock == NULL)
        return NULL;
    
    if(InitializeCriticalSectionAndSpinCount(spinlock, 0x00000400))
        return spinlock;
    
    return mycore_free(spinlock);
}

mcsync_status_t mcsync_spin_init(void* spinlock)
{
    if(spinlock)
        return MCSYNC_STATUS_OK;
    
    return MCSYNC_STATUS_NOT_OK;
}

void mcsync_spin_clean(void* spinlock)
{
}

void mcsync_spin_destroy(void* spinlock)
{
    DeleteCriticalSection(spinlock);
    mycore_free(spinlock);
}

mcsync_status_t mcsync_spin_lock(void* spinlock)
{
    EnterCriticalSection(spinlock);
    return MCSYNC_STATUS_OK;
}

mcsync_status_t mcsync_spin_unlock(void* spinlock)
{
    LeaveCriticalSection(spinlock);
    return MCSYNC_STATUS_OK;
}

/* mutex */
void * mcsync_mutex_create(void)
{
    return CreateSemaphore(NULL, 1, 1, NULL);
}

mcsync_status_t mcsync_mutex_init(void* mutex)
{
    if(mutex == NULL)
        return MCSYNC_STATUS_NOT_OK;
    
    return MCSYNC_STATUS_OK;
}

void mcsync_mutex_clean(void* mutex)
{
    /* clean function */
}

void mcsync_mutex_destroy(void* mutex)
{
    CloseHandle(mutex);
}

mcsync_status_t mcsync_mutex_lock(void* mutex)
{
    if(WaitForSingleObject(mutex, INFINITE) == WAIT_OBJECT_0)
        return MCSYNC_STATUS_OK;
    
    return MCSYNC_STATUS_NOT_OK;
}

mcsync_status_t mcsync_mutex_try_lock(void* mutex)
{
    if(WaitForSingleObject(mutex, 0) != WAIT_FAILED)
        return MCSYNC_STATUS_OK;
    
    return MCSYNC_STATUS_NOT_OK;
}

mcsync_status_t mcsync_mutex_unlock(void* mutex)
{
    if(ReleaseSemaphore(mutex, 1, NULL))
        return MCSYNC_STATUS_OK;
    
    return MCSYNC_STATUS_NOT_OK;
}

#endif
