/**
 * @file attr.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for html element attribution.
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


#ifndef PCEDOM_PRIVATE_ATTR_H
#define PCEDOM_PRIVATE_ATTR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/core/hash.h"
#include "html/core/str.h"

#include "html/ns.h"

#include "private/edom/interface.h"
#include "private/edom/node.h"
#include "res/dom/attr_const.h"
#include "private/edom/document.h"


typedef struct {
    pchtml_hash_entry_t  entry;
    pcedom_attr_id_t    attr_id;
    size_t               ref_count;
    bool                 read_only;
}
pcedom_attr_data_t;

/* More memory to God of memory! */
struct pcedom_attr {
    pcedom_node_t     node;

    /* For example: <LalAla:DiV Fix:Me="value"> */

    pcedom_attr_id_t  upper_name;     /* uppercase, with prefix: FIX:ME */
    pcedom_attr_id_t  qualified_name; /* original, with prefix: Fix:Me */

    pchtml_str_t       *value;

    pcedom_element_t  *owner;

    pcedom_attr_t     *next;
    pcedom_attr_t     *prev;
};


pcedom_attr_t *
pcedom_attr_interface_create(pcedom_document_t *document) WTF_INTERNAL;

pcedom_attr_t *
pcedom_attr_interface_destroy(pcedom_attr_t *attr) WTF_INTERNAL;

unsigned int
pcedom_attr_set_name(pcedom_attr_t *attr, const unsigned char *local_name,
                      size_t local_name_len, bool to_lowercase) WTF_INTERNAL;

unsigned int
pcedom_attr_set_value(pcedom_attr_t *attr,
                const unsigned char *value, size_t value_len) WTF_INTERNAL;

unsigned int
pcedom_attr_set_value_wo_copy(pcedom_attr_t *attr,
                unsigned char *value, size_t value_len) WTF_INTERNAL;

unsigned int
pcedom_attr_set_existing_value(pcedom_attr_t *attr,
                const unsigned char *value, size_t value_len) WTF_INTERNAL;

unsigned int
pcedom_attr_clone_name_value(pcedom_attr_t *attr_from,
                pcedom_attr_t *attr_to) WTF_INTERNAL;

bool
pcedom_attr_compare(pcedom_attr_t *first, 
                pcedom_attr_t *second) WTF_INTERNAL;

const pcedom_attr_data_t *
pcedom_attr_data_by_id(pchtml_hash_t *hash, 
                pcedom_attr_id_t attr_id) WTF_INTERNAL;

const pcedom_attr_data_t *
pcedom_attr_data_by_local_name(pchtml_hash_t *hash,
                const unsigned char *name, size_t length) WTF_INTERNAL;

const pcedom_attr_data_t *
pcedom_attr_data_by_qualified_name(pchtml_hash_t *hash,
                                    const unsigned char *name, size_t length);

const unsigned char *
pcedom_attr_qualified_name(pcedom_attr_t *attr, size_t *len) WTF_INTERNAL;


/*
 * Inline functions
 */
static inline const unsigned char *
pcedom_attr_local_name(pcedom_attr_t *attr, size_t *len)
{
    const pcedom_attr_data_t *data;

    data = pcedom_attr_data_by_id(attr->node.owner_document->attrs,
                                   attr->node.local_name);

    if (len != NULL) {
        *len = data->entry.length;
    }

    return pchtml_hash_entry_str(&data->entry);
}

static inline const unsigned char *
pcedom_attr_value(pcedom_attr_t *attr, size_t *len)
{
    if (attr->value == NULL) {
        if (len != NULL) {
            *len = 0;
        }

        return NULL;
    }

    if (len != NULL) {
        *len = attr->value->length;
    }

    return attr->value->data;
}

/*
 * No inline functions for ABI.
 */
const unsigned char *
pcedom_attr_local_name_noi(pcedom_attr_t *attr, size_t *len);

const unsigned char *
pcedom_attr_value_noi(pcedom_attr_t *attr, size_t *len);


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCEDOM_PRIVATE_ATTR_H */
