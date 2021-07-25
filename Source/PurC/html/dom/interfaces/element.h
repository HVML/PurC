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
 */


#ifndef PCHTML_DOM_ELEMENT_H
#define PCHTML_DOM_ELEMENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/core/str.h"

#include "html/dom/interfaces/document.h"
#include "html/dom/interfaces/node.h"
#include "html/dom/collection.h"
#include "html/dom/interfaces/attr.h"

#include "html/tag/tag.h"


typedef enum {
    PCHTML_DOM_ELEMENT_CUSTOM_STATE_UNDEFINED      = 0x00,
    PCHTML_DOM_ELEMENT_CUSTOM_STATE_FAILED         = 0x01,
    PCHTML_DOM_ELEMENT_CUSTOM_STATE_UNCUSTOMIZED   = 0x02,
    PCHTML_DOM_ELEMENT_CUSTOM_STATE_CUSTOM         = 0x03
}
pchtml_dom_element_custom_state_t;

struct pchtml_dom_element {
    pchtml_dom_node_t                 node;

    /* For example: <LalAla:DiV Fix:Me="value"> */

    /* uppercase, with prefix: LALALA:DIV */
    pchtml_dom_attr_id_t              upper_name;

    /* original, with prefix: LalAla:DiV */
    pchtml_dom_attr_id_t              qualified_name;

    pchtml_str_t                   *is_value;

    pchtml_dom_attr_t                 *first_attr;
    pchtml_dom_attr_t                 *last_attr;

    pchtml_dom_attr_t                 *attr_id;
    pchtml_dom_attr_t                 *attr_class;

    pchtml_dom_element_custom_state_t custom_state;
};


pchtml_dom_element_t *
pchtml_dom_element_interface_create(
                pchtml_dom_document_t *document) WTF_INTERNAL;

pchtml_dom_element_t *
pchtml_dom_element_interface_destroy(
                pchtml_dom_element_t *element) WTF_INTERNAL;

pchtml_dom_element_t *
pchtml_dom_element_create(pchtml_dom_document_t *document,
                const unsigned char *local_name, size_t lname_len,
                const unsigned char *ns_name, size_t ns_len,
                const unsigned char *prefix, size_t prefix_len,
                const unsigned char *is, size_t is_len,
                bool sync_custom) WTF_INTERNAL;

pchtml_dom_element_t *
pchtml_dom_element_destroy(pchtml_dom_element_t *element) WTF_INTERNAL;

bool
pchtml_dom_element_has_attributes(pchtml_dom_element_t *element) WTF_INTERNAL;

pchtml_dom_attr_t *
pchtml_dom_element_set_attribute(pchtml_dom_element_t *element,
                const unsigned char *qualified_name, size_t qn_len,
                const unsigned char *value, size_t value_len) WTF_INTERNAL;

const unsigned char *
pchtml_dom_element_get_attribute(pchtml_dom_element_t *element,
                const unsigned char *qualified_name, size_t qn_len,
                size_t *value_len) WTF_INTERNAL;

unsigned int
pchtml_dom_element_remove_attribute(pchtml_dom_element_t *element,
                const unsigned char *qualified_name, size_t qn_len) WTF_INTERNAL;

bool
pchtml_dom_element_has_attribute(pchtml_dom_element_t *element,
                const unsigned char *qualified_name, size_t qn_len) WTF_INTERNAL;

unsigned int
pchtml_dom_element_attr_append(pchtml_dom_element_t *element, 
                pchtml_dom_attr_t *attr) WTF_INTERNAL;

unsigned int
pchtml_dom_element_attr_remove(pchtml_dom_element_t *element, 
                pchtml_dom_attr_t *attr) WTF_INTERNAL;

pchtml_dom_attr_t *
pchtml_dom_element_attr_by_name(pchtml_dom_element_t *element,
                const unsigned char *qualified_name, size_t length) WTF_INTERNAL;

pchtml_dom_attr_t *
pchtml_dom_element_attr_by_local_name_data(pchtml_dom_element_t *element,
                const pchtml_dom_attr_data_t *data) WTF_INTERNAL;

pchtml_dom_attr_t *
pchtml_dom_element_attr_by_id(pchtml_dom_element_t *element,
                pchtml_dom_attr_id_t attr_id) WTF_INTERNAL;

pchtml_dom_attr_t *
pchtml_dom_element_attr_by_data(pchtml_dom_element_t *element,
                const pchtml_dom_attr_data_t *data) WTF_INTERNAL;

bool
pchtml_dom_element_compare(pchtml_dom_element_t *first, 
                pchtml_dom_element_t *second) WTF_INTERNAL;

pchtml_dom_attr_t *
pchtml_dom_element_attr_is_exist(pchtml_dom_element_t *element,
                const unsigned char *qualified_name, size_t length) WTF_INTERNAL;

unsigned int
pchtml_dom_element_is_set(pchtml_dom_element_t *element,
                const unsigned char *is, size_t is_len) WTF_INTERNAL;

unsigned int
pchtml_dom_elements_by_tag_name(pchtml_dom_element_t *root,
                pchtml_dom_collection_t *collection,
                const unsigned char *qualified_name, size_t len) WTF_INTERNAL;

unsigned int
pchtml_dom_elements_by_class_name(pchtml_dom_element_t *root,
                pchtml_dom_collection_t *collection,
                const unsigned char *class_name, size_t len) WTF_INTERNAL;

unsigned int
pchtml_dom_elements_by_attr(pchtml_dom_element_t *root,
                pchtml_dom_collection_t *collection,
                const unsigned char *qualified_name, size_t qname_len,
                const unsigned char *value, size_t value_len,
                bool case_insensitive) WTF_INTERNAL;

unsigned int
pchtml_dom_elements_by_attr_begin(pchtml_dom_element_t *root,
                pchtml_dom_collection_t *collection,
                const unsigned char *qualified_name, size_t qname_len,
                const unsigned char *value, size_t value_len,
                bool case_insensitive) WTF_INTERNAL;

unsigned int
pchtml_dom_elements_by_attr_end(pchtml_dom_element_t *root,
                pchtml_dom_collection_t *collection,
                const unsigned char *qualified_name, size_t qname_len,
                const unsigned char *value, size_t value_len,
                bool case_insensitive) WTF_INTERNAL;

unsigned int
pchtml_dom_elements_by_attr_contain(pchtml_dom_element_t *root,
                pchtml_dom_collection_t *collection,
                const unsigned char *qualified_name, size_t qname_len,
                const unsigned char *value, size_t value_len,
                bool case_insensitive) WTF_INTERNAL;

const unsigned char *
pchtml_dom_element_qualified_name(pchtml_dom_element_t *element, 
                size_t *len) WTF_INTERNAL;

const unsigned char *
pchtml_dom_element_qualified_name_upper(pchtml_dom_element_t *element, 
                size_t *len) WTF_INTERNAL;

const unsigned char *
pchtml_dom_element_local_name(pchtml_dom_element_t *element, 
                size_t *len) WTF_INTERNAL;

const unsigned char *
pchtml_dom_element_prefix(pchtml_dom_element_t *element, 
                size_t *len) WTF_INTERNAL;

const unsigned char *
pchtml_dom_element_tag_name(pchtml_dom_element_t *element, 
                size_t *len) WTF_INTERNAL;


/*
 * Inline functions
 */
static inline const unsigned char *
pchtml_dom_element_id(pchtml_dom_element_t *element, size_t *len)
{
    if (element->attr_id == NULL) {
        if (len != NULL) {
            *len = 0;
        }

        return NULL;
    }

    return pchtml_dom_attr_value(element->attr_id, len);
}

static inline const unsigned char *
pchtml_dom_element_class(pchtml_dom_element_t *element, size_t *len)
{
    if (element->attr_class == NULL) {
        if (len != NULL) {
            *len = 0;
        }

        return NULL;
    }

    return pchtml_dom_attr_value(element->attr_class, len);
}

static inline bool
pchtml_dom_element_is_custom(pchtml_dom_element_t *element)
{
    return element->custom_state & PCHTML_DOM_ELEMENT_CUSTOM_STATE_CUSTOM;
}

static inline bool
pchtml_dom_element_custom_is_defined(pchtml_dom_element_t *element)
{
    return element->custom_state & PCHTML_DOM_ELEMENT_CUSTOM_STATE_CUSTOM
        || element->custom_state & PCHTML_DOM_ELEMENT_CUSTOM_STATE_UNCUSTOMIZED;
}

static inline pchtml_dom_attr_t *
pchtml_dom_element_first_attribute(pchtml_dom_element_t *element)
{
    return element->first_attr;
}

static inline pchtml_dom_attr_t *
pchtml_dom_element_next_attribute(pchtml_dom_attr_t *attr)
{
    return attr->next;
}

static inline pchtml_dom_attr_t *
pchtml_dom_element_prev_attribute(pchtml_dom_attr_t *attr)
{
    return attr->prev;
}

static inline pchtml_dom_attr_t *
pchtml_dom_element_last_attribute(pchtml_dom_element_t *element)
{
    return element->last_attr;
}

static inline pchtml_dom_attr_t *
pchtml_dom_element_id_attribute(pchtml_dom_element_t *element)
{
    return element->attr_id;
}

static inline pchtml_dom_attr_t *
pchtml_dom_element_class_attribute(pchtml_dom_element_t *element)
{
    return element->attr_class;
}

static inline pchtml_tag_id_t
pchtml_dom_element_tag_id(pchtml_dom_element_t *element)
{
    return pchtml_dom_interface_node(element)->local_name;
}

static inline pchtml_ns_id_t
pchtml_dom_element_ns_id(pchtml_dom_element_t *element)
{
    return pchtml_dom_interface_node(element)->ns;
}


/*
 * No inline functions for ABI.
 */
const unsigned char *
pchtml_dom_element_id_noi(pchtml_dom_element_t *element, size_t *len);

const unsigned char *
pchtml_dom_element_class_noi(pchtml_dom_element_t *element, size_t *len);

bool
pchtml_dom_element_is_custom_noi(pchtml_dom_element_t *element);

bool
pchtml_dom_element_custom_is_defined_noi(pchtml_dom_element_t *element);

pchtml_dom_attr_t *
pchtml_dom_element_first_attribute_noi(pchtml_dom_element_t *element);

pchtml_dom_attr_t *
pchtml_dom_element_next_attribute_noi(pchtml_dom_attr_t *attr);

pchtml_dom_attr_t *
pchtml_dom_element_prev_attribute_noi(pchtml_dom_attr_t *attr);

pchtml_dom_attr_t *
pchtml_dom_element_last_attribute_noi(pchtml_dom_element_t *element);

pchtml_dom_attr_t *
pchtml_dom_element_id_attribute_noi(pchtml_dom_element_t *element);

pchtml_dom_attr_t *
pchtml_dom_element_class_attribute_noi(pchtml_dom_element_t *element);


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_DOM_ELEMENT_H */
