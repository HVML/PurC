/*
 * @file purc-runloop.h
 * @author XueShuming
 * @date 2021/12/14
 * @brief The C api for RunLoop.
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

#ifndef PURC_RUNLOOP_H
#define PURC_RUNLOOP_H

#include "purc-macros.h"

#include <stdint.h>

typedef void* purc_runloop_t;

#define     PCRUNLOOP_IO_IN     0x01
#define     PCRUNLOOP_IO_PRI    0x02
#define     PCRUNLOOP_IO_OUT    0x04
#define     PCRUNLOOP_IO_ERR    0x08
#define     PCRUNLOOP_IO_HUP    0x10
#define     PCRUNLOOP_IO_NVAL   0x20

PCA_EXTERN_C_BEGIN

/**
 * Get the runloop of current thread
 *
 * Returns: The runloop of current thread
 *
 * Since: 0.1.1
 */
PCA_EXPORT
purc_runloop_t purc_runloop_get_current(void);

/**
 * Start the current runloop
 *
 * Returns: void
 *
 * Since: 0.1.1
 */
PCA_EXPORT
void purc_runloop_run(void);

/**
 * Stop the runloop
 *
 * @param runloop: the runloop.
 *
 * Returns: void
 *
 * Since: 0.1.1
 */
PCA_EXPORT
void purc_runloop_stop(purc_runloop_t runloop);

/**
 * Warkup the runloop
 *
 * @param runloop: the runloop.
 *
 * Returns: void
 *
 * Since: 0.1.1
 */
PCA_EXPORT
void purc_runloop_wakeup(purc_runloop_t runloop);

typedef void (*purc_runloop_func)(void *ctxt);

/**
 * Dispatch function on the runloop
 *
 * @param runloop: the runloop.
 * @param func: the function to dispatch.
 * @param ctxt: the data to pass to the function
 *
 * Returns: void
 *
 * Since: 0.1.1
 */
PCA_EXPORT
void purc_runloop_dispatch(purc_runloop_t runloop, purc_runloop_func func,
        void *ctxt);

/**
 * Dispatch function on the runloop after specified time
 *
 * @param runloop: the runloop.
 * @param func: the function to dispatch.
 * @param ctxt: the data to pass to the function
 * @param time_ms: time in millisecond
 *
 * Returns: void
 *
 * Since: 0.2.0
 */
PCA_EXPORT
void purc_runloop_dispatch_after(purc_runloop_t runloop, long time_ms,
        purc_runloop_func func, void *ctxt);

/**
 * Set the idle function on the runloop which will be called on idle.
 *
 * @param runloop: the runloop.
 * @param func: the function.
 * @param ctxt: the data to pass to the function
 *
 * Returns: void
 *
 * Since: 0.1.1
 */
PCA_EXPORT
void purc_runloop_set_idle_func(purc_runloop_t runloop, purc_runloop_func func,
        void *ctxt);

typedef bool (*purc_runloop_io_callback)(int fd,
        uint32_t event, void *ctxt);

/**
 * Add file descriptors monitor on the runloop
 *
 * @param runloop: the runloop
 * @param fd: the file descriptors
 * @param event: the io event
 * @param callback: the callback function
 * @param ctxt: the data to pass to the function
 *
 * Returns: the handle of the monitor
 *
 * Since: 0.1.1
 */
PCA_EXPORT
uintptr_t purc_runloop_add_fd_monitor(purc_runloop_t runloop, int fd,
        uint32_t event, purc_runloop_io_callback callback,
        void *ctxt);

/**
 * Remove file descriptors monitor on the runloop
 *
 * @param runloop: the runloop
 * @param handle: the monitor handle
 *
 * Returns: void
 *
 * Since: 0.1.1
 */
PCA_EXPORT
void purc_runloop_remove_fd_monitor(purc_runloop_t runloop, uintptr_t handle);

typedef bool (*purc_runloop_timeout_callback)(void *ctxt);

/**
 * Set a timer which executes the callback by interval. When the callback
 * returns false it will be deleted.
 *
 * @param runloop: the runloop
 * @param callback: the timeout callback
 * @param interval: the timeout interval in milliseconds
 *
 * Returns: void
 *
 * Since: 0.1.1
 */
PCA_EXPORT
void purc_runloop_set_timeout(purc_runloop_t runloop,
        purc_runloop_timeout_callback callback, void *ctxt, uint32_t interval);

PCA_EXTERN_C_END

#endif /* not defined PURC_RUNLOOP_H */

