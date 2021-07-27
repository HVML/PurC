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


#ifndef PCHTML_DOM_ATTR_H
#define PCHTML_DOM_ATTR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/core/hash.h"
#include "html/core/str.h"

#include "html/ns/ns.h"

#include "private/edom/interface.h"
#include "private/edom/node.h"
#include "private/edom/attr_const.h"
#include "private/edom/document.h"


typedef struct {
    pchtml_hash_entry_t  entry;
    pchtml_dom_attr_id_t    attr_id;
    size_t               ref_count;
    bool                 read_only;
}
pchtml_dom_attr_data_t;

/* More memory to God of memory! */
struct pchtml_dom_attr {
    pchtml_dom_node_t     node;

    /* For example: <LalAla:DiV Fix:Me="value"> */

    pchtml_dom_attr_id_t  upper_name;     /* uppercase, with prefix: FIX:ME */
    pchtml_dom_attr_id_t  qualified_name; /* original, with prefix: Fix:Me */

    pchtml_str_t       *value;

    pchtml_dom_element_t  *owner;

    pchtml_dom_attr_t     *next;
    pchtml_dom_attr_t     *prev;
};


pchtml_dom_attr_t *
pchtml_dom_attr_interface_create(pchtml_dom_document_t *document) WTF_INTERNAL;

pchtml_dom_attr_t *
pchtml_dom_attr_interface_destroy(pchtml_dom_attr_t *attr) WTF_INTERNAL;

unsigned int
pchtml_dom_attr_set_name(pchtml_dom_attr_t *attr, const unsigned char *local_name,
                      size_t local_name_len, bool to_lowercase) WTF_INTERNAL;

unsigned int
pchtml_dom_attr_set_value(pchtml_dom_attr_t *attr,
                const unsigned char *value, size_t value_len) WTF_INTERNAL;

unsigned int
pchtml_dom_attr_set_value_wo_copy(pchtml_dom_attr_t *attr,
                unsigned char *value, size_t value_len) WTF_INTERNAL;

unsigned int
pchtml_dom_attr_set_existing_value(pchtml_dom_attr_t *attr,
                const unsigned char *value, size_t value_len) WTF_INTERNAL;

unsigned int
pchtml_dom_attr_clone_name_value(pchtml_dom_attr_t *attr_from,
                pchtml_dom_attr_t *attr_to) WTF_INTERNAL;

bool
pchtml_dom_attr_compare(pchtml_dom_attr_t *first, 
                pchtml_dom_attr_t *second) WTF_INTERNAL;

const pchtml_dom_attr_data_t *
pchtml_dom_attr_data_by_id(pchtml_hash_t *hash, 
                pchtml_dom_attr_id_t attr_id) WTF_INTERNAL;

const pchtml_dom_attr_data_t *
pchtml_dom_attr_data_by_local_name(pchtml_hash_t *hash,
                const unsigned char *name, size_t length) WTF_INTERNAL;

const pchtml_dom_attr_data_t *
pchtml_dom_attr_data_by_qualified_name(pchtml_hash_t *hash,
                                    const unsigned char *name, size_t length);

const unsigned char *
pchtml_dom_attr_qualified_name(pchtml_dom_attr_t *attr, size_t *len) WTF_INTERNAL;


/*
 * Inline functions
 */
static inline const unsigned char *
pchtml_dom_attr_local_name(pchtml_dom_attr_t *attr, size_t *len)
{
    const pchtml_dom_attr_data_t *data;

    data = pchtml_dom_attr_data_by_id(attr->node.owner_document->attrs,
                                   attr->node.local_name);

    if (len != NULL) {
        *len = data->entry.length;
    }

    return pchtml_hash_entry_str(&data->entry);
}

static inline const unsigned char *
pchtml_dom_attr_value(pchtml_dom_attr_t *attr, size_t *len)
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
pchtml_dom_attr_local_name_noi(pchtml_dom_attr_t *attr, size_t *len);

const unsigned char *
pchtml_dom_attr_value_noi(pchtml_dom_attr_t *attr, size_t *len);


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_DOM_ATTR_H */
