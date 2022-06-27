/**
 * @file rdr_msc
 * @author Xu Xiaohong
 * @date 2022/06/08
 * @brief The internal interfaces for interpreter
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

static void
process_rdr_msg_by_event(pcrdr_msg *msg)
{
    switch (msg->target) {
        case PCRDR_MSG_TARGET_SESSION:
            // NOTE: not implemented yet
            PC_ASSERT(0);
            break;
        case PCRDR_MSG_TARGET_WORKSPACE:
            // NOTE: not implemented yet
            PC_ASSERT(0);
            break;
        case PCRDR_MSG_TARGET_PLAINWINDOW:
            // NOTE: not implemented yet
            PC_ASSERT(0);
            break;
        case PCRDR_MSG_TARGET_WIDGET:
            // NOTE: not implemented yet
            PC_ASSERT(0);
            break;
        case PCRDR_MSG_TARGET_DOM:
            // NOTE: not implemented yet
            PC_ASSERT(0);
            break;
        case PCRDR_MSG_TARGET_INSTANCE:
            // NOTE: not implemented yet
            PC_ASSERT(0);
            break;
        case PCRDR_MSG_TARGET_COROUTINE:
            // NOTE: not implemented yet
            PC_ASSERT(0);
            break;
        case PCRDR_MSG_TARGET_USER:
            // NOTE: not implemented yet
            PC_ASSERT(0);
            break;
        default:
            // NOTE: shouldn't happen, no way to recover gracefully, fail-fast
            PC_ASSERT(0);
    }
}

void
pcintr_check_and_dispatch_msg(void)
{
    PC_ASSERT(pcintr_get_coroutine());

    int r;
    size_t n;
    r = purc_inst_holding_messages_count(&n);
    PC_ASSERT(r == 0);
    if (n <= 0)
        return;

    pcrdr_msg *msg;
    msg = purc_inst_take_away_message(0);
    if (msg == NULL) {
        PC_ASSERT(purc_get_last_error() == 0);
        return;
    }

    switch (msg->type) {
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
        case PCRDR_MSG_TYPE_EVENT:
            return process_rdr_msg_by_event(msg);
            break;
        default:
            // NOTE: shouldn't happen, no way to recover gracefully, fail-fast
            PC_ASSERT(0);
    }

    PC_ASSERT(0);
}

