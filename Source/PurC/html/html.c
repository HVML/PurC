/*
 * @file html.c
 * @author Geng Yue
 * @date 2021/07/02
 * @brief The implementation of public part for html parser.
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
 * This implementation of HTML parser is derived from Lexbor
 * <https://github.com/lexbor/lexbor>, which is licensed under the Apache
 * License, Version 2.0:
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

#include "private/instance.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/edom.h"
#include "private/html.h"

static const char* html_err_msgs[] = {
    /* PCHTML_ERROR */
    "Error in parsing html document.",
    /* PCHTML_OBJECT_IS_NULL */
    "Html object is null.",
    /* PCHTML_TOO_SMALL_SIZE */
    "The size of array is too small.",
    /* PCHTML_WRONG_STAGE */
    "In wrong parsing stage.",
    /* PCHTML_OVERFLOW */
    "The tag stack is overflow.",
};

static struct err_msg_seg _html_err_msgs_seg = {
    { NULL, NULL },
    PURC_ERROR_FIRST_VARIANT,
    PURC_ERROR_FIRST_VARIANT + PCA_TABLESIZE(html_err_msgs) - 1,
    html_err_msgs
};

void pchtml_init_once(void)
{
    // register error message
    pcinst_register_error_message_segment(&_html_err_msgs_seg);

    // initialize others
}

void pchtml_init_instance(struct pcinst* inst)
{
    UNUSED_PARAM(inst);

    // initialize others
}

void pchtml_cleanup_instance(struct pcinst* inst)
{
    UNUSED_PARAM(inst);
}
