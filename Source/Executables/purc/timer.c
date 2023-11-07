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

struct pcmcth_timer {
    const char         *name;
    on_timer_expired_f  on_expired;
    void               *ctxt;

    int                 interval;
    int64_t             expired_ms;
    const char         *id; /* returned by kvlist_set_ex() */

    /* AVL node for the AVL tree sorted by living time */
    struct avl_node     avl;
};

/* Compares two timers to sort them in AVL tree. */
static int compare_timers(const void *k1, const void *k2, void *ptr)
{
    (void)ptr;
    const struct pcmcth_timer *timer1 = k1;
    const struct pcmcth_timer *timer2 = k2;

    if (timer1->expired_ms > timer2->expired_ms)
        return 1;
    else if (timer1->expired_ms == timer2->expired_ms)
        return 0;
    return -1;
}

int pcmcth_timer_module_init(pcmcth_renderer *rdr)
{
    avl_init(&rdr->timer_avl, compare_timers, true, NULL);
    kvlist_init(&rdr->timer_list, NULL);
    return 0;
}

void pcmcth_timer_module_cleanup(pcmcth_renderer *rdr)
{
    kvlist_free(&rdr->timer_list);
    pcmcth_timer_delete_all(rdr);
}

int64_t pcmcth_timer_current_milliseconds(pcmcth_renderer* rdr)
{
    return purc_get_elapsed_milliseconds_alt(rdr->t_start, NULL);
}

static inline size_t get_timer_key_len(const char *name)
{
    /* pattern: regular-0x1321f0-null */
    return strlen(name) + sizeof(intptr_t) * 4 + 4 /* 0x */ + 2 /* - */;
}

static inline int get_timer_key(char *buf, size_t buf_len,
        const char *name, on_timer_expired_f on_expired, void *ctxt)
{
    int n = snprintf(buf, buf_len, "%s-%p-%p", name, on_expired, ctxt);
    if (n < 0 || (size_t)n > buf_len) {
        return -1;
    }

    return 0;
}

pcmcth_timer_t pcmcth_timer_find(pcmcth_renderer* rdr, const char *name,
        on_timer_expired_f on_expired, void *ctxt)
{
    size_t buf_len = get_timer_key_len(name) + 1;
    char id[buf_len];
    if (get_timer_key(id, buf_len, name, on_expired, ctxt))
        return NULL;

    void *data;
    data = kvlist_get(&rdr->timer_list, id);
    if (data == NULL)
        return NULL;

    return *(pcmcth_timer_t *)data;
}

pcmcth_timer_t pcmcth_timer_new(pcmcth_renderer* rdr, const char *name,
        on_timer_expired_f on_expired, int interval, void *ctxt)
{
    assert(interval > 0);
    struct pcmcth_timer *timer = NULL;

    size_t buf_len = get_timer_key_len(name) + 1;
    char id[buf_len];
    if (get_timer_key(id, buf_len, name, on_expired, ctxt))
        goto failed;

    void *data;
    data = kvlist_get(&rdr->timer_list, id);
    if (data)   /* duplicated */
        goto failed;

    timer = (struct pcmcth_timer *)calloc(1, sizeof(struct pcmcth_timer));
    if (timer == NULL) {
        goto failed;
    }

    timer->name = name;
    timer->interval = interval;
    timer->expired_ms = pcmcth_timer_current_milliseconds(rdr) + interval;
    timer->ctxt = ctxt;
    timer->on_expired = on_expired;
    timer->avl.key = timer;

    if (!(timer->id = kvlist_set_ex(&rdr->timer_list, id, &timer))) {
        goto failed;
    }

    if (avl_insert(&rdr->timer_avl, &timer->avl)) {
        goto failed_avl;
    }

    rdr->nr_timers++;
    return timer;

failed_avl:
    if (timer->id)
        kvlist_delete(&rdr->timer_list, timer->id);

failed:
    if (timer) {
        free(timer);
    }
    return NULL;
}

const char *pcmcth_timer_id(pcmcth_renderer* rdr, pcmcth_timer_t timer)
{
    (void)rdr;
    return timer->id;
}

int pcmcth_timer_delete(pcmcth_renderer* rdr, pcmcth_timer_t timer)
{
    avl_delete(&rdr->timer_avl, &timer->avl);
    if (timer->id)
        kvlist_delete(&rdr->timer_list, timer->id);
    free(timer);
    return 0;
}

unsigned pcmcth_timer_delete_all(pcmcth_renderer* rdr)
{
    unsigned n = 0;
    struct pcmcth_timer *timer, *tmp;

    avl_remove_all_elements(&rdr->timer_avl, timer, avl, tmp) {
        free(timer);
        n++;
    }

    return n;
}

unsigned pcmcth_timer_check_expired(pcmcth_renderer *rdr)
{
    unsigned n = 0;
    int64_t curr_ms = pcmcth_timer_current_milliseconds(rdr);
    struct pcmcth_timer *timer, *tmp;

    avl_for_each_element_safe(&rdr->timer_avl, timer, avl, tmp) {
        if (curr_ms >= timer->expired_ms) {

            int interval = timer->on_expired(timer->name, timer->ctxt);
            if (interval < 0) {
                pcmcth_timer_delete(rdr, timer);
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

