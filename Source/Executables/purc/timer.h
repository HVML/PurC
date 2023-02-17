/*
** @file timer.h
** @author Vincent Wei
** @date 2023/02/09
** @brief The interface of timer.
**
** Copyright (C) 2023 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of purc, which is an HVML interpreter with
** a command line interface (CLI).
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef purc_foil_timer_h_
#define purc_foil_timer_h_

#include "purcmc-thread.h"

struct pcmcth_timer;
typedef struct pcmcth_timer *pcmcth_timer_t;

int foil_timer_module_init(pcmcth_renderer *rdr);
void foil_timer_module_cleanup(pcmcth_renderer *rdr);

/**
 * Prototype of the callback which will be called when a timer expired.
 *
 * Returns zero to keep the timer;
 * Returns a positive value to change the interval;
 * Returns a negative value to cancel the timer.
 */
typedef int (*on_timer_expired_f)(const char *name, void *ctxt);

/** Returns the current milliseconds since the renderer starts up */
int64_t foil_timer_current_milliseconds(pcmcth_renderer *rdr);

/**
 * Creates a new timer.
 *
 * Note that the name (must be a static string) and the callback handler
 * constitute a unique identifier for the new timer if @unique is %true.
 *
 * Returns the handle to the timer; NULL for failure.
 */
pcmcth_timer_t foil_timer_new(pcmcth_renderer *rdr, const char *name,
        on_timer_expired_f callback, int interval, void *ctxt, bool unique);

/**
 * Retrieves a timer based on the identifier and callback.
 * Returns the handle to the timer; NULL for not found.
 */
pcmcth_timer_t foil_timer_find(pcmcth_renderer *rdr, const char *name,
        on_timer_expired_f callback);

/** Returns the identifier of a timer. */
const char *foil_timer_id(pcmcth_renderer *rdr, pcmcth_timer_t timer);

/** Deletes a timer. */
int foil_timer_delete(pcmcth_renderer *rdr, pcmcth_timer_t timer);

/** Deletes all timer. Returns the number of timers deleted */
unsigned foil_timer_delete_all(pcmcth_renderer *rdr);

/** Calls the on_timer_expired_f callbacks for all expired timers. */
unsigned foil_timer_check_expired(pcmcth_renderer *rdr);

#endif /* !purc_foil_timer_h_ */

