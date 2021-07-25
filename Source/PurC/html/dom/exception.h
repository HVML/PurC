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
 */


#ifndef PCHTML_DOM_EXCEPTION_H
#define PCHTML_DOM_EXCEPTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/core/base.h"


typedef enum {
    PCHTML_DOM_INDEX_SIZE_ERR = 0x00,
    PCHTML_DOM_DOMSTRING_SIZE_ERR,
    PCHTML_DOM_HIERARCHY_REQUEST_ERR,
    PCHTML_DOM_WRONG_DOCUMENT_ERR,
    PCHTML_DOM_INVALID_CHARACTER_ERR,
    PCHTML_DOM_NO_DATA_ALLOWED_ERR,
    PCHTML_DOM_NO_MODIFICATION_ALLOWED_ERR,
    PCHTML_DOM_NOT_FOUND_ERR,
    PCHTML_DOM_NOT_SUPPORTED_ERR,
    PCHTML_DOM_INUSE_ATTRIBUTE_ERR,
    PCHTML_DOM_INVALID_STATE_ERR,
    PCHTML_DOM_SYNTAX_ERR,
    PCHTML_DOM_INVALID_MODIFICATION_ERR,
    PCHTML_DOM_NAMESPACE_ERR,
    PCHTML_DOM_INVALID_ACCESS_ERR,
    PCHTML_DOM_VALIDATION_ERR,
    PCHTML_DOM_TYPE_MISMATCH_ERR,
    PCHTML_DOM_SECURITY_ERR,
    PCHTML_DOM_NETWORK_ERR,
    PCHTML_DOM_ABORT_ERR,
    PCHTML_DOM_URL_MISMATCH_ERR,
    PCHTML_DOM_QUOTA_EXCEEDED_ERR,
    PCHTML_DOM_TIMEOUT_ERR,
    PCHTML_DOM_INVALID_NODE_TYPE_ERR,
    PCHTML_DOM_DATA_CLONE_ERR
}
pchtml_dom_exception_code_t;


/*
 * Inline functions
 */
static inline void *
pchtml_dom_exception_code_ref_set(pchtml_dom_exception_code_t *var,
                               pchtml_dom_exception_code_t code)
{
    if (var != NULL) {
        *var = code;
    }

    return NULL;
}

/*
 * No inline functions for ABI.
 */
void *
pchtml_dom_exception_code_ref_set_noi(pchtml_dom_exception_code_t *var,
                                   pchtml_dom_exception_code_t code);


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_DOM_EXCEPTION_H */
