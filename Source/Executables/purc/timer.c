/*
** @file timer.c
** @author Vincent Wei
** @date 2023/02/09
** @brief The timer management.
**
** Copyright (c) 2023 FMSoft (http://www.fmsoft.cn)
**
** Author: Vincent Wei (https://github.com/VincentWei)
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

#include "config.h"
#include "timer.h"

#include <assert.h>

struct foil_timer {
    int     id;
    int     interval;
    int64_t expired_ms;
    void   *ctxt;

    on_timer_expired_f on_expired;

    /* AVL node for the AVL tree sorted by living time */
    struct avl_node avl;
};

int foil_timer_compare(const void *k1, const void *k2, void *ptr)
{
    (void)ptr;
    const struct foil_timer *timer1 = k1;
    const struct foil_timer *timer2 = k2;

    if (timer1->expired_ms > timer2->expired_ms)
        return 1;
    else if (timer1->expired_ms == timer2->expired_ms)
        return 0;
    return -1;
}

int64_t foil_timer_current_milliseconds(pcmcth_renderer* rdr)
{
    return purc_get_elapsed_milliseconds_alt(rdr->t_start, NULL);
}

foil_timer_t foil_timer_new(pcmcth_renderer* rdr, int id,
        int interval, on_timer_expired_f on_expired, void *ctxt)
{
    assert(interval > 0);

    struct foil_timer *timer;
    timer = (struct foil_timer *)calloc(1, sizeof(struct foil_timer));
    if (timer == NULL) {
        goto failed;
    }

    timer->id = id;
    timer->interval = interval;
    timer->expired_ms = foil_timer_current_milliseconds(rdr) + interval;
    timer->ctxt = ctxt;
    timer->on_expired = on_expired;

    timer->avl.key = timer;

    if (avl_insert(&rdr->timer_avl, &timer->avl)) {
        goto failed;
    }

    rdr->nr_timers++;
    return timer;

failed:
    if (timer) {
        free(timer);
    }
    return NULL;
}

int foil_timer_delete(pcmcth_renderer* rdr, foil_timer_t timer)
{
    avl_delete(&rdr->timer_avl, &timer->avl);
    free(timer);
    return 0;
}

unsigned foil_timer_delete_all(pcmcth_renderer* rdr)
{
    unsigned n = 0;
    struct foil_timer *timer, *tmp;

    avl_remove_all_elements(&rdr->timer_avl, timer, avl, tmp) {
        free(timer);
        n++;
    }

    return n;
}

unsigned foil_timer_check_expired(pcmcth_renderer *rdr)
{
    unsigned n = 0;
    int64_t curr_ms = foil_timer_current_milliseconds(rdr);
    struct foil_timer *timer, *tmp;

    avl_for_each_element_safe(&rdr->timer_avl, timer, avl, tmp) {
        if (curr_ms >= timer->expired_ms) {

            int interval = timer->on_expired(timer, timer->id, timer->ctxt);
            if (interval < 0) {
                foil_timer_delete(rdr, timer);
            }
            else {
                /* update interval and expired_ms and reinstall it */
                if (interval > 0)
                    timer->interval = interval;
                timer->expired_ms = curr_ms + timer->interval;
                avl_delete(&rdr->timer_avl, &timer->avl);
                avl_insert(&rdr->timer_avl, &timer->avl);
            }

            n++;
        }
        else {
            /* Skip left timers */
            break;
        }
    }

    return n;
}

