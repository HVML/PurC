/**
 * @file purc-errors.h
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

#ifndef PURC_PURC_ERRORS_H
#define PURC_PURC_ERRORS_H

#include <stdbool.h>
#include <stddef.h>

#include "purc-macros.h"

#define PURC_ERROR_OK                   0
#define PURC_ERROR_BAD_SYSTEM_CALL      1
#define PURC_ERROR_BAD_STDC             2
#define PURC_ERROR_OUT_OF_MEMORY        3
#define PURC_ERROR_INVALID_VALUE        4
#define PURC_ERROR_DUPLICATED           5
#define PURC_ERROR_NOT_IMPLEMENTED      6

// the first error codes for various modules:
#define PURC_ERROR_FIRST_VARIANT        100
#define PURC_ERROR_FIRST_RWSTREAM       200

#define PURC_ERROR_FIRST_EJSON          1100
#define PURC_ERROR_FIRST_HVML           1200
#define PURC_ERROR_FIRST_HTML           1300
#define PURC_ERROR_FIRST_XGML           1400
#define PURC_ERROR_FIRST_XML            1500

#define PURC_ERROR_FIRST_VDOM           2100
#define PURC_ERROR_FIRST_EDOM           2200
#define PURC_ERROR_FIRST_VCM            2300

// TODO: error codes for variant go here
enum pcvariant_error
{
    PCVARIANT_SUCCESS = PURC_ERROR_OK,
    PCVARIANT_BAD_ENCODING = PURC_ERROR_FIRST_VARIANT,
};

// TODO: error codes for variant go here
enum pcrwstream_error
{
    PCRWSTREAM_SUCCESS = PURC_ERROR_OK,
    // PCRWSTREAM_BAD_ENCODING = PURC_ERROR_FIRST_RWSTREAM,
};

PCA_EXTERN_C_BEGIN

PCA_EXPORT int
purc_get_last_error (void);

PCA_EXPORT const char*
purc_get_error_message (int err_code);

bool
purc_set_error_messages (int first, const char* msgs[], size_t nr_msgs);

void
purc_set_error (int err_code);

PCA_EXTERN_C_END

#endif /* not defined PURC_PURC_ERRORS_H */

