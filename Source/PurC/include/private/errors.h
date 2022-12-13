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

PCA_EXTERN_C_BEGIN

struct err_msg_info {
    const char* msg;
    int except_id;
    unsigned int flags;
    purc_atom_t except_atom;
};

struct err_msg_seg {
    struct list_head list;
    int first_errcode, last_errcode;
    struct err_msg_info* info;
};

/* registers the messages for a segment of error codes */
void pcinst_register_error_message_segment(struct err_msg_seg* seg) WTF_INTERNAL;

/* sets the the last error code */
#define pcinst_set_error(x)                 purc_set_error(x)
#define pcinst_set_error_exinfo(x, exinfo)  purc_set_error_exinfo(x, exinfo)

bool
pcinst_is_ignorable_error(int err);

PCA_EXTERN_C_END

#endif /* not defined PURC_PRIVATE_ERRORS_H */

