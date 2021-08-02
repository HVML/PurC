/**
 * @file element.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for html element.
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
 * This implementation of HTML parser is derived from Lexbor <http://lexbor.com/>.
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */


#ifndef PCEDOM_ELEMENT_H
#define PCEDOM_ELEMENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/str.h"

#include "private/edom/document.h"
#include "private/edom/node.h"
#include "private/edom/collection.h"
#include "private/edom/attr.h"

#include "html/tag_tag.h"


typedef enum {
    PCEDOM_ELEMENT_CUSTOM_STATE_UNDEFINED      = 0x00,
    PCEDOM_ELEMENT_CUSTOM_STATE_FAILED         = 0x01,
    PCEDOM_ELEMENT_CUSTOM_STATE_UNCUSTOMIZED   = 0x02,
    PCEDOM_ELEMENT_CUSTOM_STATE_CUSTOM         = 0x03
}
pcedom_element_custom_state_t;

struct pcedom_element {
    pcedom_node_t                 node;

    /* For example: <LalAla:DiV Fix:Me="value"> */

    /* uppercase, with prefix: LALALA:DIV */
    pcedom_attr_id_t              upper_name;

    /* original, with prefix: LalAla:DiV */
    pcedom_attr_id_t              qualified_name;

    pchtml_str_t                   *is_value;

    pcedom_attr_t                 *first_attr;
    pcedom_attr_t                 *last_attr;

    pcedom_attr_t                 *attr_id;
    pcedom_attr_t                 *attr_class;

    pcedom_element_custom_state_t custom_state;
};


pcedom_element_t *
pcedom_element_interface_create(
                pcedom_document_t *document) WTF_INTERNAL;

pcedom_element_t *
pcedom_element_interface_destroy(
                pcedom_element_t *element) WTF_INTERNAL;

pcedom_element_t *
pcedom_element_create(pcedom_document_t *document,
                const unsigned char *local_name, size_t lname_len,
                const unsigned char *ns_name, size_t ns_len,
                const unsigned char *prefix, size_t prefix_len,
                const unsigned char *is, size_t is_len,
                bool sync_custom) WTF_INTERNAL;

pcedom_element_t *
pcedom_element_destroy(pcedom_element_t *element) WTF_INTERNAL;

bool
pcedom_element_has_attributes(pcedom_element_t *element) WTF_INTERNAL;

pcedom_attr_t *
pcedom_element_set_attribute(pcedom_element_t *element,
                const unsigned char *qualified_name, size_t qn_len,
                const unsigned char *value, size_t value_len) WTF_INTERNAL;

const unsigned char *
pcedom_element_get_attribute(pcedom_element_t *element,
                const unsigned char *qualified_name, size_t qn_len,
                size_t *value_len) WTF_INTERNAL;

unsigned int
pcedom_element_remove_attribute(pcedom_element_t *element,
                const unsigned char *qualified_name, size_t qn_len) WTF_INTERNAL;

bool
pcedom_element_has_attribute(pcedom_element_t *element,
                const unsigned char *qualified_name, size_t qn_len) WTF_INTERNAL;

unsigned int
pcedom_element_attr_append(pcedom_element_t *element, 
                pcedom_attr_t *attr) WTF_INTERNAL;

unsigned int
pcedom_element_attr_remove(pcedom_element_t *element, 
                pcedom_attr_t *attr) WTF_INTERNAL;

pcedom_attr_t *
pcedom_element_attr_by_name(pcedom_element_t *element,
                const unsigned char *qualified_name, size_t length) WTF_INTERNAL;

pcedom_attr_t *
pcedom_element_attr_by_local_name_data(pcedom_element_t *element,
                const pcedom_attr_data_t *data) WTF_INTERNAL;

pcedom_attr_t *
pcedom_element_attr_by_id(pcedom_element_t *element,
                pcedom_attr_id_t attr_id) WTF_INTERNAL;

pcedom_attr_t *
pcedom_element_attr_by_data(pcedom_element_t *element,
                const pcedom_attr_data_t *data) WTF_INTERNAL;

bool
pcedom_element_compare(pcedom_element_t *first, 
                pcedom_element_t *second) WTF_INTERNAL;

pcedom_attr_t *
pcedom_element_attr_is_exist(pcedom_element_t *element,
                const unsigned char *qualified_name, size_t length) WTF_INTERNAL;

unsigned int
pcedom_element_is_set(pcedom_element_t *element,
                const unsigned char *is, size_t is_len) WTF_INTERNAL;

unsigned int
pcedom_elements_by_tag_name(pcedom_element_t *root,
                pcedom_collection_t *collection,
                const unsigned char *qualified_name, size_t len) WTF_INTERNAL;

unsigned int
pcedom_elements_by_class_name(pcedom_element_t *root,
                pcedom_collection_t *collection,
                const unsigned char *class_name, size_t len) WTF_INTERNAL;

unsigned int
pcedom_elements_by_attr(pcedom_element_t *root,
                pcedom_collection_t *collection,
                const unsigned char *qualified_name, size_t qname_len,
                const unsigned char *value, size_t value_len,
                bool case_insensitive) WTF_INTERNAL;

unsigned int
pcedom_elements_by_attr_begin(pcedom_element_t *root,
                pcedom_collection_t *collection,
                const unsigned char *qualified_name, size_t qname_len,
                const unsigned char *value, size_t value_len,
                bool case_insensitive) WTF_INTERNAL;

unsigned int
pcedom_elements_by_attr_end(pcedom_element_t *root,
                pcedom_collection_t *collection,
                const unsigned char *qualified_name, size_t qname_len,
                const unsigned char *value, size_t value_len,
                bool case_insensitive) WTF_INTERNAL;

unsigned int
pcedom_elements_by_attr_contain(pcedom_element_t *root,
                pcedom_collection_t *collection,
                const unsigned char *qualified_name, size_t qname_len,
                const unsigned char *value, size_t value_len,
                bool case_insensitive) WTF_INTERNAL;

const unsigned char *
pcedom_element_qualified_name(pcedom_element_t *element, 
                size_t *len) WTF_INTERNAL;

const unsigned char *
pcedom_element_qualified_name_upper(pcedom_element_t *element, 
                size_t *len) WTF_INTERNAL;

const unsigned char *
pcedom_element_local_name(pcedom_element_t *element, 
                size_t *len) WTF_INTERNAL;

const unsigned char *
pcedom_element_prefix(pcedom_element_t *element, 
                size_t *len) WTF_INTERNAL;

const unsigned char *
pcedom_element_tag_name(pcedom_element_t *element, 
                size_t *len) WTF_INTERNAL;


/*
 * Inline functions
 */
static inline const unsigned char *
pcedom_element_id(pcedom_element_t *element, size_t *len)
{
    if (element->attr_id == NULL) {
        if (len != NULL) {
            *len = 0;
        }

        return NULL;
    }

    return pcedom_attr_value(element->attr_id, len);
}

static inline const unsigned char *
pcedom_element_class(pcedom_element_t *element, size_t *len)
{
    if (element->attr_class == NULL) {
        if (len != NULL) {
            *len = 0;
        }

        return NULL;
    }

    return pcedom_attr_value(element->attr_class, len);
}

static inline bool
pcedom_element_is_custom(pcedom_element_t *element)
{
    return element->custom_state & PCEDOM_ELEMENT_CUSTOM_STATE_CUSTOM;
}

static inline bool
pcedom_element_custom_is_defined(pcedom_element_t *element)
{
    return element->custom_state & PCEDOM_ELEMENT_CUSTOM_STATE_CUSTOM
        || element->custom_state & PCEDOM_ELEMENT_CUSTOM_STATE_UNCUSTOMIZED;
}

static inline pcedom_attr_t *
pcedom_element_first_attribute(pcedom_element_t *element)
{
    return element->first_attr;
}

static inline pcedom_attr_t *
pcedom_element_next_attribute(pcedom_attr_t *attr)
{
    return attr->next;
}

static inline pcedom_attr_t *
pcedom_element_prev_attribute(pcedom_attr_t *attr)
{
    return attr->prev;
}

static inline pcedom_attr_t *
pcedom_element_last_attribute(pcedom_element_t *element)
{
    return element->last_attr;
}

static inline pcedom_attr_t *
pcedom_element_id_attribute(pcedom_element_t *element)
{
    return element->attr_id;
}

static inline pcedom_attr_t *
pcedom_element_class_attribute(pcedom_element_t *element)
{
    return element->attr_class;
}

static inline pchtml_tag_id_t
pcedom_element_tag_id(pcedom_element_t *element)
{
    return pcedom_interface_node(element)->local_name;
}

static inline pchtml_ns_id_t
pcedom_element_ns_id(pcedom_element_t *element)
{
    return pcedom_interface_node(element)->ns;
}


/*
 * No inline functions for ABI.
 */
const unsigned char *
pcedom_element_id_noi(pcedom_element_t *element, size_t *len);

const unsigned char *
pcedom_element_class_noi(pcedom_element_t *element, size_t *len);

bool
pcedom_element_is_custom_noi(pcedom_element_t *element);

bool
pcedom_element_custom_is_defined_noi(pcedom_element_t *element);

pcedom_attr_t *
pcedom_element_first_attribute_noi(pcedom_element_t *element);

pcedom_attr_t *
pcedom_element_next_attribute_noi(pcedom_attr_t *attr);

pcedom_attr_t *
pcedom_element_prev_attribute_noi(pcedom_attr_t *attr);

pcedom_attr_t *
pcedom_element_last_attribute_noi(pcedom_element_t *element);

pcedom_attr_t *
pcedom_element_id_attribute_noi(pcedom_element_t *element);

pcedom_attr_t *
pcedom_element_class_attribute_noi(pcedom_element_t *element);


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCEDOM_ELEMENT_H */
