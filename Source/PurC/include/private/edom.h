/**
 * @file edom.h
 * @author 
 * @date 2021/07/02
 * @brief The internal interfaces for edom.
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

#ifndef PURC_PRIVATE_EDOM_H
#define PURC_PRIVATE_EDOM_H

#include "config.h"
#include "purc-variant.h"

#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//struct pcedom_tag_id;
//typedef struct pcedom_tag_id  pcedom_tag_id;
//typedef struct pcedom_tag_id* pcedom_tag_id_t;
typedef uintptr_t pcedom_tag_id;
typedef pcedom_tag_id* pcedom_tag_id_t;


typedef enum purc_namespace
{
    PURC_NAMESPACE_HTML,
    PURC_NAMESPACE_MAX,
} purc_namespace;

// edom type
typedef enum {
    PCEDOM_NODE_TYPE_HTML_UNDEF                  = 0x00,
    PCEDOM_NODE_TYPE_HTML_ELEMENT                = 0x01,
    PCEDOM_NODE_TYPE_HTML_ATTRIBUTE              = 0x02,
    PCEDOM_NODE_TYPE_HTML_TEXT                   = 0x03,
    PCEDOM_NODE_TYPE_HTML_CDATA_SECTION          = 0x04,
    PCEDOM_NODE_TYPE_HTML_ENTITY_REFERENCE       = 0x05, // historical
    PCEDOM_NODE_TYPE_HTML_ENTITY                 = 0x06, // historical
    PCEDOM_NODE_TYPE_HTML_PROCESSING_INSTRUCTION = 0x07,
    PCEDOM_NODE_TYPE_HTML_COMMENT                = 0x08,
    PCEDOM_NODE_TYPE_HTML_DOCUMENT               = 0x09,
    PCEDOM_NODE_TYPE_HTML_DOCUMENT_TYPE          = 0x0A,
    PCEDOM_NODE_TYPE_HTML_DOCUMENT_FRAGMENT      = 0x0B,
    PCEDOM_NODE_TYPE_HTML_NOTATION               = 0x0C, // historical
    PCEDOM_NODE_TYPE_HTML_LAST_ENTRY             = 0x0D
}
pcedom_node_type;


// define edom node
struct pcedom_node {
//    pchtml_dom_event_target_t event_target;

    /* For example: <LalAla:DiV Fix:Me="value"> */

    uintptr_t              local_name;          // lowercase, without prefix: div
    uintptr_t              prefix;              // lowercase: lalala
    uintptr_t              ns;                  // namespace

    struct pcedom_document * owner_document;    // document

    struct pcedom_node     * next;
    struct pcedom_node     * prev;
    struct pcedom_node     * parent;
    struct pcedom_node     * first_child;
    struct pcedom_node     * last_child;
    void                   * user;

    pcedom_node_type       type;
};
typedef struct pcedom_node     pcedom_node;
typedef struct pcedom_node *   pcedom_node_t;

typedef uintptr_t pcedom_attr_id_t;

// define edom node with element type
struct pcedom_element {
    pcedom_node_t          node;

    /* For example: <LalAla:DiV Fix:Me="value"> */

    /* uppercase, with prefix: LALALA:DIV */
    pcedom_attr_id_t       upper_name;

    /* original, with prefix: LalAla:DiV */
    pcedom_attr_id_t       qualified_name;

    purc_variant_t         *is_value;

    pcedom_attr_t          *first_attr;
    pcedom_attr_t          *last_attr;

    pcedom_attr_t          *attr_id;
    pcedom_attr_t          *attr_class;

//    pchtml_dom_element_custom_state_t custom_state;
};
typedef struct pcedom_element  pcedom_element;
typedef struct pcedom_element *pcedom_element_t;

// define edom node with attribution type
struct pcedom_attr {
    pcedom_node_t         node;

    /* For example: <LalAla:DiV Fix:Me="value"> */

    pcedom_attr_id_t      upper_name;           // uppercase, with prefix: FIX:ME
    pcedom_attr_id_t      qualified_name;       // original, with prefix: Fix:Me

    purc_variant_t        value;

    pcedom_element_t      * owner;              // elemnet node

    struct pcedom_attr    * next;
    struct pcedom_attr    * prev;
};
typedef struct pcedom_attr  pcedom_attr;
typedef struct pcedom_attr* pcedom_attr_t;


// define
struct pcedom_character_data {
    pchtml_dom_node_t node;

    purc_variant_t    data;
};
typedef struct pcedom_character_data  pcedom_character_data;
typedef struct pcedom_character_data* pcedom_character_data_t;


// define edom node with comment type
struct pcedom_comment {
    pchtml_dom_character_data_t char_data;
};
typedef struct pcedom_comment  pcedom_comment;
typedef struct pcedom_comment* pcedom_comment_t;


// define edom node with text type
struct pcedom_text {
    pcedom_character_data_t char_data;
};
typedef struct pcedom_text  pcedom_text;
typedef struct pcedom_text* pcedom_text_t;


struct pchtml
{
    int type;
};

struct pcedom_tree
{
    int type;
};

//struct pchtml_t;
typedef struct pchtml  pchtml;
typedef struct pchtml* pchtml_t;

// edom tree type
//struct pcedom_tree;
typedef struct pcedom_tree  pcedom_tree;
typedef struct pcedom_tree* pcedom_tree_t;

// functions about edom node
// create a new empty edom node
pcedom_element_t pcedom_element_create (pcedom_tree_t tree,
                    pcedom_tag_id_t tag_id, const char* tag_name, 
                    enum purc_namespace ns) WTF_INTERNAL;

// set edom node attribution with string
pcedom_attr_t pcedom_element_set_attribute (pcedom_element_t element,
                     const char *attr_name, size_t name_len,
                     const char *attr_value, size_t value_len) WTF_INTERNAL;

// get edom node attribution
pcedom_attr_t pcedom_element_get_attribute (pcedom_element_t element,
                     const char *attr_name, size_t name_len) WTF_INTERNAL;

// remove indicated attribution
bool pcedom_element_remove_attribute (pcedom_element_t element,
                     pcedom_attr_t attr) WTF_INTERNAL;

// remove indicated attribution with name
bool pcedom_element_remove_attribute_by_name (pcedom_element_t element,
                     const char* attr_name, size_t name_len) WTF_INTERNAL;

// destroy attribution of an edom node
bool pcedom_attribute_destroy (pcedom_attr_t attr) WTF_INTERNAL;

// set edom node content
bool pcedom_element_set_content (pcedom_element_t element,
                     purc_variant_t variant) WTF_INTERNAL;

// destroy an edom node
bool pcedom_element_destroy(pcedom_element_t elem) WTF_INTERNAL;


// functions about edom tree
// create a new empty edom tree
pcedom_tree_t pcedom_tree_create(void) WTF_INTERNAL;

// initialize edom tree with html parser
bool pcedom_tree_init(pcedom_tree_t tree, pchtml_t parser) WTF_INTERNAL;

// initialize edom tree with XGML parser
//bool pcedom_tree_init(pcedom_tree_t tree, pcxgml_t parser) WTF_INTERNAL;

// initialize edom tree with XML parser
//bool pcedom_tree_init(pcedom_tree_t tree, pcxml_t parser) WTF_INTERNAL;

// append an edom element as child 
bool pcedom_tree_element_add_child(pcedom_element_t parent, 
                                pcedom_element_t elem) WTF_INTERNAL;

// insert an edom elment before indicated element 
bool pcedom_tree_element_insert_before(pcedom_element_t current, 
                                pcedom_element_t elem) WTF_INTERNAL;

// insert an edom elment after indicated element 
bool pcedom_tree_element_insert_after(pcedom_element_t current, 
                                pcedom_element_t elem) WTF_INTERNAL;

// remove an edom elment from tree 
bool pcedom_tree_remove_element(pcedom_tree_t tree, 
                                pcedom_element_t elem) WTF_INTERNAL;

// clean edom tree, the tree is existence
bool pcedom_tree_clean(pcedom_tree_t tree) WTF_INTERNAL;

// destroy edom tree, the tree is disappear
void pcedom_tree_destroy(pcedom_tree_t tree) WTF_INTERNAL;

// functions about traverse edom tree
// get the first element for an edom tree
pcedom_element_t pcedom_element_first(pcedom_tree_t tree) WTF_INTERNAL;

// get the last element for an edom tree
pcedom_element_t pcedom_element_last(pcedom_tree_t tree) WTF_INTERNAL;

// get the first child element from an indicated edom element
pcedom_element_t pcedom_element_child(pcedom_element_t elem) WTF_INTERNAL;

// get the last child element from an indicated edom element
pcedom_element_t pcedom_element_last_child(pcedom_element_t elem) WTF_INTERNAL;

// get the next brother edom element of an indicated edom element
pcedom_element_t pcedom_element_next(pcedom_element_t elem) WTF_INTERNAL;

// get the previous brother edom element of an indicated edom element
pcedom_element_t pcedom_element_prev(pcedom_element_t elem) WTF_INTERNAL;


// edom collection
typedef struct pcedom_collection {
    pcedom_element_t *list;
    size_t size;
    size_t length;
} pcedom_collection;
typedef struct pcedom_collection* pcedom_collection_t;

// create an empty edom element collection
pcedom_collection_t pcedom_collection_create(size_t size) WTF_INTERNAL;

// clear an empty edom element collection
bool pcedom_collection_clean(pcedom_collection_t collection) WTF_INTERNAL;

// destroy an empty edom element collection
bool pcedom_collection_destroy(pcedom_collection_t collection) WTF_INTERNAL;

// create collection by tag
pcedom_collection_t pcedom_get_elements_by_tag_id(pcedom_tree_t tree, 
                                pcedom_collection_t collection,
                                pcedom_tag_id_t tag_id) WTF_INTERNAL;

// create collection by name 
pcedom_collection_t pcedom_get_elements_by_name(pcedom_tree_t tree, 
                                pcedom_collection_t collection,
                                const char* name, size_t length) WTF_INTERNAL;

// create collection by attribution name 
pcedom_collection_t pcedom_get_elements_by_attribute_name(pcedom_tree_t tree,
                                pcedom_collection_t collection,
                                pcedom_element_t scope_node,
                                const char* key, size_t key_len) WTF_INTERNAL;

// create collection by attribution value 
pcedom_collection_t pcedom_get_elements_by_attribute_value(pcedom_tree_t tree,
                                pcedom_collection_t collection,
                                pcedom_element_t node,
                                bool case_insensitive,
                                const char* key, size_t key_len,
                                const char* value, 
                                size_t value_len) WTF_INTERNAL;

// create collection by attribution value 
pcedom_collection_t pcedom_get_elements_by_attribute_value_whitespace_separated(
                                pcedom_tree_t tree,
                                pcedom_collection_t collection,
                                pcedom_element_t node,
                                bool case_insensitive,
                                const char* key, size_t key_len,
                                const char* value, 
                                size_t value_len) WTF_INTERNAL;



#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* PURC_PRIVATE_EDOM_H*/
