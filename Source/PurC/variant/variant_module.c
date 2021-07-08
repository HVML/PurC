/*
 * @file variant-module.c
 * @author gengyue
 * @date 2021/07/06
 * @brief The operation for variant module.
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

#include <stdlib.h>
#include <string.h>

#include "purc.h"
#include "config.h"
#include "private/instance.h"
#include "private/errors.h"
#include "private/tls.h"
#include "purc-variant.h"
#include "variant.h"

static struct purc_variant pcvariant_null = { PURC_VARIANT_TYPE_NULL, 0, 0, PCVARIANT_FLAG_NOFREE };
static struct purc_variant pcvariant_undefined = { PURC_VARIANT_TYPE_UNDEFINED, 0, 0, PCVARIANT_FLAG_NOFREE };
static struct purc_variant pcvariant_false = { PURC_VARIANT_TYPE_BOOLEAN, 0, 0, PCVARIANT_FLAG_NOFREE, { b:0 } };
static struct purc_variant pcvariant_true = { PURC_VARIANT_TYPE_BOOLEAN, 0, 0, PCVARIANT_FLAG_NOFREE, { b:1 } };
static struct purc_variant_const = {&pcvariant_null, &pcvariant_undefined, &pcvariant_false, &pcvariant_true};

static const char* variant_err_msgs[] = {
    /* PURC_ERROR_VARIANT_INVALID_TYPE */
    "Invalid variant type",
};

static struct err_msg_seg _variant_err_msgs_seg = {
    { NULL, NULL },
    PURC_ERROR_FIRST_VARIANT, PURC_ERROR_FIRST_VARIANT + PCA_TABLESIZE(variant_err_msgs) - 1,
    variant_err_msgs
};

bool pcvariant_init_module(void)
{
    struct pcinst * pcinstance = NULL;

    // register error message
    pcinst_register_error_message_segment(&_variant_err_msgs_seg);

    // register const value in instance
    pcinstance = pcinst_current();
    pcinstance->variant_const = &purc_variant_const;
}
