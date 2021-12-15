/*
 * @file errors.h
 * @author Vincent Wei (https://github.com/VincentWei)
 * @date 2021/07/05
 * @brief The internal interfaces for error code.
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

#ifndef PURC_PRIVATE_ERRORS_H
#define PURC_PRIVATE_ERRORS_H

#include "config.h"

#include "purc-utils.h"
#include "private/list.h"

struct err_msg_seg {
    struct list_head list;
    int first_errcode, last_errcode;
    const char **msgs;
};

/* registers the messages for a segment of error codes */
void pcinst_register_error_message_segment(struct err_msg_seg* seg) WTF_INTERNAL;

/* sets the the last error code */
#define pcinst_set_error(x)     purc_set_error(x)
#define pcinst_set_error_ex(x, exinfo)     purc_set_error_ex(x, exinfo)
// pcinst_set_error(int err_code) WTF_INTERNAL;

void pcinst_add_error_except_map(int error, purc_atom_t except);

void purc_error_init_once(void);

#endif /* not defined PURC_PRIVATE_ERRORS_H */

