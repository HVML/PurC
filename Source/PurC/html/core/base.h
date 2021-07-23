/**
 * @file base.h
 * @author 
 * @date 2021/07/02
 * @brief The basic hearder file for html core.
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

#ifndef PCHTML_BASE_H
#define PCHTML_BASE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include <memory.h>
#include <limits.h>
#include <string.h>

#include "config.h"
#include "html/core/def.h"
#include "html/core/types.h"
#include "html/core/pchtml.h"

#define PCHTML_VERSION_MAJOR 1
#define PCHTML_VERSION_MINOR 3
#define PCHTML_VERSION_PATCH 2

#define PCHTML_VERSION_STRING PCHTML_STRINGIZE(PCHTML_VERSION_MAJOR) "."       \
                              PCHTML_STRINGIZE(PCHTML_VERSION_MINOR) "."       \
                              PCHTML_STRINGIZE(PCHTML_VERSION_PATCH)

#define pchtml_assert(val)


/*
 * Very important!!!
 *
 * for 0..00AFFF; PCHTML_STATUS_OK == 0x000000
 */
typedef enum {
    PCHTML_STATUS_OK                       = 0x0000,
    PCHTML_STATUS_ERROR                    = 0x0001,
    PCHTML_STATUS_ERROR_MEMORY_ALLOCATION,
    PCHTML_STATUS_ERROR_OBJECT_IS_NULL,
    PCHTML_STATUS_ERROR_SMALL_BUFFER,
    PCHTML_STATUS_ERROR_INCOMPLETE_OBJECT,
    PCHTML_STATUS_ERROR_NO_FREE_SLOT,
    PCHTML_STATUS_ERROR_TOO_SMALL_SIZE,
    PCHTML_STATUS_ERROR_NOT_EXISTS,
    PCHTML_STATUS_ERROR_WRONG_ARGS,
    PCHTML_STATUS_ERROR_WRONG_STAGE,
    PCHTML_STATUS_ERROR_UNEXPECTED_RESULT,
    PCHTML_STATUS_ERROR_UNEXPECTED_DATA,
    PCHTML_STATUS_ERROR_OVERFLOW,
    PCHTML_STATUS_CONTINUE,
    PCHTML_STATUS_SMALL_BUFFER,
    PCHTML_STATUS_ABORTED,
    PCHTML_STATUS_STOPPED,
    PCHTML_STATUS_NEXT,
    PCHTML_STATUS_STOP,
}
pchtml_status_t;

typedef enum {
    PCHTML_ACTION_OK    = 0x00,
    PCHTML_ACTION_STOP  = 0x01,
    PCHTML_ACTION_NEXT  = 0x02
}
pchtml_action_t;


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_BASE_H */

