/*
 * @file msg-queue.h
 * @author XueShuming
 * @date 2022/06/28
 * @brief The message queue.
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

#ifndef PURC_PRIVATE_MSG_QUEUE_H
#define PURC_PRIVATE_MSG_QUEUE_H

#include "purc.h"

#include "config.h"

#include "private/list.h"
#include "purc-pcrdr.h"

typedef struct pcrdr_msg pcinst_msg;
struct pcinst_msg_hdr {
    atomic_uint             owner;
    struct list_head        ln;
};

struct pcinst_msg_queue {
    struct purc_rwlock  lock;
    struct list_head    req_msgs;
    struct list_head    res_msgs;
    struct list_head    event_msgs;
    struct list_head    timer_msgs;
    struct list_head    msgs;

    uint64_t            state;
    size_t              nr_msgs;
};

/* Make sure the size of `struct list_head` is two times of sizeof(void *) */
#define _COMPILE_TIME_ASSERT(name, x)               \
       typedef int _dummy_ ## name[(x) * 2 - 1]
_COMPILE_TIME_ASSERT(onwer_atom,
        sizeof(atomic_uint) == sizeof(purc_atom_t));
_COMPILE_TIME_ASSERT(list_head,
        sizeof(struct list_head) == (sizeof(void *) * 2));
#undef _COMPILE_TIME_ASSERT

PCA_EXTERN_C_BEGIN

struct pcinst_msg_queue *
pcinst_msg_queue_create(void);

ssize_t
pcinst_msg_queue_destroy(struct pcinst_msg_queue *queue);

int
pcinst_msg_queue_append(struct pcinst_msg_queue *queue, pcinst_msg *msg);

int
pcinst_msg_queue_prepend(struct pcinst_msg_queue *queue, pcinst_msg *msg);

pcinst_msg *
pcinst_msg_get_msg(struct pcinst_msg_queue *queue);


PCA_EXTERN_C_END

#endif /* not defined PURC_PRIVATE_MSG_QUEUE_H */

