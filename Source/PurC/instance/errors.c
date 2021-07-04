/*
 * @file errors.c
 * @author Vincent Wei (https://github.com/VincentWei)
 * @date 2021/07/02
 * @brief The error codes of PurC.
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

#include "purc-errors.h"

#include "config.h"

static int my_last_error = PURC_ERROR_OK;

int purc_get_last_error (void)
{
    return my_last_error;
}

void purc_set_error (int err_code)
{
    my_last_error = err_code;
}

const char* purc_get_error_message (int err_code)
{
    UNUSED_PARAM(err_code);
    return "Unknown";
}

bool purc_set_error_messages (int first, const char* msgs[], size_t nr_msgs)
{
    UNUSED_PARAM(first);
    UNUSED_PARAM(msgs);
    UNUSED_PARAM(nr_msgs);
    return true;
}

