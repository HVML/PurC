/**
 * @file rdr_msc
 * @author Xue Shuming
 * @date 2022/07/01
 * @brief The message handler for instance
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
 *
 */

#include "config.h"

#include "internal.h"
#include "private/instance.h"
#include "private/msg-queue.h"
#include "private/interpreter.h"
#include "private/regex.h"

#define PLOG(...) do {                                                        \
    FILE *fp = fopen("/tmp/plog.log", "a+");                                  \
    fprintf(fp, ##__VA_ARGS__);                                               \
    fclose(fp);                                                               \
} while (0)


int
dispatch_move_buffer_msg(struct pcinst *inst, pcrdr_msg *msg)
{
    UNUSED_PARAM(inst);
    struct pcintr_heap *heap = inst->intr_heap;
    if (!heap) {
        return 0;
    }

    switch (msg->type) {
    case PCRDR_MSG_TYPE_EVENT:
    {
        // add msg to coroutine message queue
        struct rb_root *coroutines = &heap->coroutines;
        struct rb_node *p, *n;
        struct rb_node *first = pcutils_rbtree_first(coroutines);
        pcutils_rbtree_for_each_safe(first, p, n) {
            pcintr_coroutine_t co = container_of(p, struct pcintr_coroutine,
                    node);
            if (co->ident == msg->targetValue) {
                return pcinst_msg_queue_append(co->mq, msg);
            }
        }
    }
        break;

    case PCRDR_MSG_TYPE_VOID:
        // NOTE: not implemented yet
        PC_ASSERT(0);
        break;

    case PCRDR_MSG_TYPE_REQUEST:
        // NOTE: not implemented yet
        PC_ASSERT(0);
        break;

    case PCRDR_MSG_TYPE_RESPONSE:
        // NOTE: not implemented yet
        PC_ASSERT(0);
        break;

    default:
        // NOTE: shouldn't happen, no way to recover gracefully, fail-fast
        PC_ASSERT(0);
    }
    return 0;
}

void
handle_move_buffer_msg(void)
{
    size_t n;
    int r = purc_inst_holding_messages_count(&n);
    PC_ASSERT(r == 0);
    if (n <= 0) {
        return;
    }

    struct pcinst *inst = pcinst_current();

    for (size_t i = 0; i < n; i++) {
        pcrdr_msg *msg = purc_inst_take_away_message(0);
        if (msg == NULL) {
            PC_ASSERT(purc_get_last_error() == 0);
            return;
        }

        // TODO: how to handle dispatch failed
        dispatch_move_buffer_msg(inst, msg);
        pcinst_put_message(msg);
    }
}


void
handle_coroutine_msg(pcintr_coroutine_t co)
{
    UNUSED_PARAM(co);
}

void
pcintr_dispatch_msg(void)
{
    // handle msg from move buffer
    handle_move_buffer_msg();

    // handle msg from message queue of the current co
    handle_coroutine_msg(pcintr_get_coroutine());
}

