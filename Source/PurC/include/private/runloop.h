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

PCA_EXTERN_C_BEGIN

// Must be called from the main thread
void pcrunloop_init_main(void);

// the RunLoop of main thread
pcrunloop_t pcrunloop_get_main(void);

// the RunLoop of current thread
pcrunloop_t pcrunloop_get_current(void);

// check if current is in main thread
bool pcrunloop_is_on_main(void);

// start the current runloop
void pcrunloop_run(void);

// stop the runloop
void pcrunloop_stop(pcrunloop_t runloop);

// warkup the runloop
void pcrunloop_warkup(pcrunloop_t runloop);

// dispatch function
typedef int (*pcrunloop_func)(void* ctxt);
void pcrunloop_dispatch(pcrunloop_t runloop, pcrunloop_func func, void* ctxt);

PCA_EXTERN_C_END

#endif /* not defined PURC_PRIVATE_RUNLOOP_H */

