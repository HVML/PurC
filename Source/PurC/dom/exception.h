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


#ifndef PCDOM_EXCEPTION_H
#define PCDOM_EXCEPTION_H

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PCDOM_INDEX_SIZE_ERR = 0x00,
    PCDOM_DOMSTRING_SIZE_ERR,
    PCDOM_HIERARCHY_REQUEST_ERR,
    PCDOM_WRONG_DOCUMENT_ERR,
    PCDOM_INVALID_CHARACTER_ERR,
    PCDOM_NO_DATA_ALLOWED_ERR,
    PCDOM_NO_MODIFICATION_ALLOWED_ERR,
    PCDOM_NOT_FOUND_ERR,
    PCDOM_NOT_SUPPORTED_ERR,
    PCDOM_INUSE_ATTRIBUTE_ERR,
    PCDOM_INVALID_STATE_ERR,
    PCDOM_SYNTAX_ERR,
    PCDOM_INVALID_MODIFICATION_ERR,
    PCDOM_NAMESPACE_ERR,
    PCDOM_INVALID_ACCESS_ERR,
    PCDOM_VALIDATION_ERR,
    PCDOM_TYPE_MISMATCH_ERR,
    PCDOM_SECURITY_ERR,
    PCDOM_NETWORK_ERR,
    PCDOM_ABORT_ERR,
    PCDOM_URL_MISMATCH_ERR,
    PCDOM_QUOTA_EXCEEDED_ERR,
    PCDOM_TIMEOUT_ERR,
    PCDOM_INVALID_NODE_TYPE_ERR,
    PCDOM_DATA_CLONE_ERR
}
pcdom_exception_code_t;


/*
 * Inline functions
 */
static inline void *
pcdom_exception_code_ref_set(pcdom_exception_code_t *var,
                               pcdom_exception_code_t code)
{
    if (var != NULL) {
        *var = code;
    }

    return NULL;
}


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCDOM_EXCEPTION_H */
