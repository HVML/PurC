/**
 * @file exception.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for dom parser exception.
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


#ifndef PCEDOM_EXCEPTION_H
#define PCEDOM_EXCEPTION_H

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PCEDOM_INDEX_SIZE_ERR = 0x00,
    PCEDOM_DOMSTRING_SIZE_ERR,
    PCEDOM_HIERARCHY_REQUEST_ERR,
    PCEDOM_WRONG_DOCUMENT_ERR,
    PCEDOM_INVALID_CHARACTER_ERR,
    PCEDOM_NO_DATA_ALLOWED_ERR,
    PCEDOM_NO_MODIFICATION_ALLOWED_ERR,
    PCEDOM_NOT_FOUND_ERR,
    PCEDOM_NOT_SUPPORTED_ERR,
    PCEDOM_INUSE_ATTRIBUTE_ERR,
    PCEDOM_INVALID_STATE_ERR,
    PCEDOM_SYNTAX_ERR,
    PCEDOM_INVALID_MODIFICATION_ERR,
    PCEDOM_NAMESPACE_ERR,
    PCEDOM_INVALID_ACCESS_ERR,
    PCEDOM_VALIDATION_ERR,
    PCEDOM_TYPE_MISMATCH_ERR,
    PCEDOM_SECURITY_ERR,
    PCEDOM_NETWORK_ERR,
    PCEDOM_ABORT_ERR,
    PCEDOM_URL_MISMATCH_ERR,
    PCEDOM_QUOTA_EXCEEDED_ERR,
    PCEDOM_TIMEOUT_ERR,
    PCEDOM_INVALID_NODE_TYPE_ERR,
    PCEDOM_DATA_CLONE_ERR
}
pcedom_exception_code_t;


/*
 * Inline functions
 */
static inline void *
pcedom_exception_code_ref_set(pcedom_exception_code_t *var,
                               pcedom_exception_code_t code)
{
    if (var != NULL) {
        *var = code;
    }

    return NULL;
}


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCEDOM_EXCEPTION_H */
