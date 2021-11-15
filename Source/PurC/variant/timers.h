/**
 * @file timers.h
 * @author Xu Xiaohong
 * @date 2021/11/15
 * @brief The hearder file for TIMERS
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


#ifndef _VARIANT_TIMERS_H_
#define _VARIANT_TIMERS_H_

#include "config.h"

#include "purc-macros.h"

#include "private/debug.h"
#include "private/rbtree.h"

struct pcvariant_timer {
    struct rb_node             node;
    char                      *id;
    int                        milli_secs;

    struct timespec            expire;

    unsigned int               activated:1;
    unsigned int               expired:1;
    unsigned int               zombie:1;
};

struct pcvariant_timers {
    struct rb_root             root;
};

PCA_EXTERN_C_BEGIN

void
pcvariant_timers_init(struct pcvariant_timers *timers) WTF_INTERNAL;

void
pcvariant_timers_release(struct pcvariant_timers *timers) WTF_INTERNAL;

int
pcvariant_timers_add_timer(struct pcvariant_timers *timers,
        const char *id, int milli_secs, int activate) WTF_INTERNAL;

int
pcvariant_timers_activate_timer(struct pcvariant_timers *timers,
        const char *id, int activate) WTF_INTERNAL;

int
pcvariant_timers_del_timer(struct pcvariant_timers *timers,
        const char *id) WTF_INTERNAL;

typedef void (*timer_expired_handler)(struct pcvariant_timers *timers,
        const char *id, void *ud);
void
pcvariant_timers_expired(struct pcvariant_timers *timers,
        void *ud, timer_expired_handler handler) WTF_INTERNAL;

PCA_EXTERN_C_END

#endif  /* _VARIANT_TIMERS_H_ */

