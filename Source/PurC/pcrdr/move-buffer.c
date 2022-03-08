/*
 * move-buffer.c -- The implementation of move buffer.
 *      Created on 8 Mar 2022
 *
 * Copyright (C) 2022 FMSoft (http://www.fmsoft.cn)
 *
 * Authors:
 *  Vincent Wei (https://github.com/VincentWei), 2022
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

#include "config.h"
#include "purc-pcrdr.h"
#include "private/sorted-array.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/ports.h"

#include <assert.h>

size_t
pcrdr_thread_move_msg(purc_atom_t endpoint_to, pcrdr_msg *msg)
{
    UNUSED_PARAM(endpoint_to);
    UNUSED_PARAM(msg);
    return 0;
}


size_t
pcrdr_thread_nr_moving_msgs(void)
{
    return 0;
}

const pcrdr_msg *
pcrdr_thread_retrieve_msg(size_t index)
{
    UNUSED_PARAM(index);
    return NULL;
}

pcrdr_msg *
pcrdr_thread_take_away_msg(size_t index)
{
    UNUSED_PARAM(index);
    return NULL;
}

bool
pcrdr_thread_create_move_buffer(size_t max_moving_msgs)
{
    UNUSED_PARAM(max_moving_msgs);
    return false;
}

ssize_t
pcrdr_thread_destroy_move_buffer(void)
{
    return -1;
}
