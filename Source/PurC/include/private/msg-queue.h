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

#endif /* not defined PURC_PRIVATE_MSG_QUEUE_H */

