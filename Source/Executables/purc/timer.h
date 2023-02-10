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

struct foil_timer;
typedef struct foil_timer *foil_timer_t;

/**
 * Prototype of the callback which will be called when a timer expired.
 *
 * Returns zero to keep the timer;
 * Returns a positive value to change the interval;
 * Returns a negative value to cancel the timer.
 */
typedef int (*on_timer_expired_f)(foil_timer_t timer, int id, void *ctxt);

/** Compares two timers to sort them in AVL tree. */
int foil_timer_compare(const void *k1, const void *k2, void *ptr);

/** Returns the current milliseconds since the renderer starts up */
int64_t foil_timer_current_milliseconds(pcmcth_renderer* rdr);

/**
 * Creates a new timer.
 * Returns the handle to the timer; NULL for failure.
 */
foil_timer_t foil_timer_new(pcmcth_renderer* rdr, int id,
        int interval, on_timer_expired_f callback, void *ctxt);

/** Deletes a timer. */
int foil_timer_delete(pcmcth_renderer* rdr, foil_timer_t timer);

/** Deletes all timer. Returns the number of timers deleted */
unsigned foil_timer_delete_all(pcmcth_renderer* rdr);

/** Calls the on_timer_expired_f callbacks for all expired timers. */
unsigned foil_timer_check_expired(pcmcth_renderer *rdr);

#endif /* !purc_foil_timer_h_ */

