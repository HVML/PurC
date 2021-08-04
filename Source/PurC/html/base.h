/**
 * @file base.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for html parser.
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
 * <https://github.com/lexbor/lexbor>, which is licensed under Apahce 2.0:
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */


#ifndef PCHTML_HTML_BASE_H
#define PCHTML_HTML_BASE_H

#include "config.h"

#include "purc-errors.h"

#include "html/def.h"
#include "html/types.h"
#include "html/pchtml.h"

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include <memory.h>
#include <limits.h>
#include <string.h>

#define pchtml_assert(val)

typedef enum {
    PCHTML_STATUS_OK                       = PURC_ERROR_OK,
    PCHTML_STATUS_ERROR                    = PURC_ERROR_UNKNOWN,
    PCHTML_STATUS_ERROR_MEMORY_ALLOCATION  = PURC_ERROR_OUT_OF_MEMORY,
    PCHTML_STATUS_ERROR_OBJECT_IS_NULL     = PURC_ERROR_NULL_OBJECT,
    PCHTML_STATUS_ERROR_SMALL_BUFFER       = PURC_ERROR_TOO_SMALL_BUFF,
    PCHTML_STATUS_ERROR_TOO_SMALL_SIZE     = PURC_ERROR_TOO_SMALL_SIZE,
    PCHTML_STATUS_ERROR_INCOMPLETE_OBJECT  = PURC_ERROR_INCOMPLETE_OBJECT,
    PCHTML_STATUS_ERROR_NO_FREE_SLOT       = PURC_ERROR_NO_FREE_SLOT,
    PCHTML_STATUS_ERROR_NOT_EXISTS         = PURC_ERROR_NOT_EXISTS,
    PCHTML_STATUS_ERROR_WRONG_ARGS         = PURC_ERROR_WRONG_ARGS,
    PCHTML_STATUS_ERROR_WRONG_STAGE        = PURC_ERROR_WRONG_STAGE,
    PCHTML_STATUS_ERROR_UNEXPECTED_RESULT  = PURC_ERROR_UNEXPECTED_RESULT,
    PCHTML_STATUS_ERROR_UNEXPECTED_DATA    = PURC_ERROR_UNEXPECTED_DATA,
    PCHTML_STATUS_ERROR_OVERFLOW           = PURC_ERROR_OVERFLOW,
    PCHTML_STATUS_CONTINUE                 = PURC_ERROR_FIRST_HTML,
    PCHTML_STATUS_SMALL_BUFFER,
    PCHTML_STATUS_ABORTED,
    PCHTML_STATUS_STOPPED,
    PCHTML_STATUS_NEXT,
    PCHTML_STATUS_STOP,
} pchtml_status_t;

typedef enum {
    PCHTML_ACTION_OK    = 0x00,
    PCHTML_ACTION_STOP  = 0x01,
    PCHTML_ACTION_NEXT  = 0x02
} pchtml_action_t;

typedef struct pchtml_html_tokenizer pchtml_html_tokenizer_t;
typedef unsigned int pchtml_html_tokenizer_opt_t;
typedef struct pchtml_html_tree pchtml_html_tree_t;

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_HTML_BASE_H */
