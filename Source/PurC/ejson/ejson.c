/*
 * @file ejson.c
 * @author XueShuming
 * @date 2021/07/19
 * @brief The impl for eJSON init
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

#include "private/ejson.h"
#include "private/errors.h"
#include "private/instance.h"

#include "purc-utils.h"
#include "purc-errors.h"
#include "config.h"

#include <math.h>
#include <string.h>

#if HAVE(GLIB)
#include <gmodule.h>
#else
#include <stdlib.h>
#endif

#include "ejson_err_msgs.inc"

/* Make sure the number of error messages matches the number of error codes */
#define _COMPILE_TIME_ASSERT(name, x)               \
       typedef int _dummy_ ## name[(x) * 2 - 1]

_COMPILE_TIME_ASSERT(msgs,
        PCA_TABLESIZE(ejson_err_msgs) == PCEJSON_ERROR_NR);

#undef _COMPILE_TIME_ASSERT

static struct err_msg_seg _ejson_err_msgs_seg = {
    { NULL, NULL },
    PURC_ERROR_FIRST_EJSON,
    PURC_ERROR_FIRST_EJSON + PCA_TABLESIZE(ejson_err_msgs) - 1,
    ejson_err_msgs
};


static int ejson_init_once (void)
{
    pcinst_register_error_message_segment(&_ejson_err_msgs_seg);
    return 0;
}

struct pcmodule _module_ejson = {
    .id              = PURC_HAVE_VARIANT | PURC_HAVE_EJSON,
    .module_inited   = 0,

    .init_once       = ejson_init_once,
    .init_instance   = NULL,
};


