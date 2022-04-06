/*
 * @file runloop.h
 * @author XueShuming
 * @date 2021/12/14
 * @brief The C api for RunLoop.
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

#ifndef PURC_PRIVATE_RUNLOOP_H
#define PURC_PRIVATE_RUNLOOP_H

#include "purc.h"

#include "config.h"

#include "private/variant.h"
#include "private/map.h"

typedef void* pcrunloop_t;
enum pcrunloop_io_condition
{
    PCRUNLOOP_IO_IN,
    PCRUNLOOP_IO_OUT,
    PCRUNLOOP_IO_PRI,
    PCRUNLOOP_IO_ERR,
    PCRUNLOOP_IO_HUP,
    PCRUNLOOP_IO_NVAL,
};

PCA_EXTERN_C_BEGIN

void pcrunloop_init_main(void);

void pcrunloop_stop_main(void);

bool pcrunloop_is_main_initialized(void);

// the RunLoop of current thread
pcrunloop_t pcrunloop_get_current(void);

// check if current is in main thread
bool pcrunloop_is_on_main(void);

// start the current runloop
void pcrunloop_run(void);

// stop the runloop
void pcrunloop_stop(pcrunloop_t runloop);

// warkup the runloop
void pcrunloop_wakeup(pcrunloop_t runloop);

// dispatch function
typedef int (*pcrunloop_func)(void* ctxt);
void pcrunloop_dispatch(pcrunloop_t runloop, pcrunloop_func func, void* ctxt);

void pcrunloop_set_idle_func(pcrunloop_t runloop, pcrunloop_func func, void* ctxt);

typedef bool (*pcrunloop_io_callback)(int fd,
        enum pcrunloop_io_condition condition, void *ctxt);

uintptr_t pcrunloop_add_fd_monitor(pcrunloop_t runloop, int fd,
        enum pcrunloop_io_condition condition, pcrunloop_io_callback callback,
        void *ctxt);

void pcrunloop_remove_fd_monitor(pcrunloop_t runloop, uintptr_t handle);

PCA_EXTERN_C_END

#endif /* not defined PURC_PRIVATE_RUNLOOP_H */

