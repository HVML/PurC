/*
 * @file purc-runloop.h
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

#ifndef PURC_RUNLOOP_H
#define PURC_RUNLOOP_H

#include <stdint.h>

typedef void* purc_runloop_t;
typedef enum purc_runloop_io_state
{
    PCRUNLOOP_IO_IN,
    PCRUNLOOP_IO_OUT,
    PCRUNLOOP_IO_PRI,
    PCRUNLOOP_IO_ERR,
    PCRUNLOOP_IO_HUP,
    PCRUNLOOP_IO_NVAL,
} purc_runloop_io_state;

PCA_EXTERN_C_BEGIN

void purc_runloop_init_main(void);

void purc_runloop_stop_main(void);

bool purc_runloop_is_main_initialized(void);

// the RunLoop of current thread
purc_runloop_t purc_runloop_get_current(void);

// check if current is in main thread
bool purc_runloop_is_on_main(void);

// start the current runloop
void purc_runloop_run(void);

// stop the runloop
void purc_runloop_stop(purc_runloop_t runloop);

// warkup the runloop
void purc_runloop_wakeup(purc_runloop_t runloop);

// dispatch function
typedef int (*purc_runloop_func)(void* ctxt);
void purc_runloop_dispatch(purc_runloop_t runloop, purc_runloop_func func,
        void* ctxt);

void purc_runloop_set_idle_func(purc_runloop_t runloop, purc_runloop_func func,
        void* ctxt);

typedef bool (*purc_runloop_io_callback)(int fd,
        purc_runloop_io_state state, void *ctxt);

uintptr_t purc_runloop_add_fd_monitor(purc_runloop_t runloop, int fd,
        purc_runloop_io_state state, purc_runloop_io_callback callback,
        void *ctxt);

void purc_runloop_remove_fd_monitor(purc_runloop_t runloop, uintptr_t handle);

PCA_EXTERN_C_END

#endif /* not defined PURC_RUNLOOP_H */

