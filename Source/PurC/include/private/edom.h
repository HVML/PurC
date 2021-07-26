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

typedef enum purc_namespace
{
    PURC_NAMESPACE_HTML,
    PURC_NAMESPACE_MAX,
} purc_namespace;


// edom type
struct pcxgml
{
    int type;
};

struct pcxml
{
    int type;
};

struct pchtml
{
    int type;
};

struct pcedom_tag_id
{
    int type;
};

struct pcedom_element
{
    int type;
};

struct pcedom_attr
{
    int type;
};

struct pcedom_tree
{
    int type;
};

//struct pcxgml_t;
typedef struct pcxgml  pcxgml;
typedef struct pcxgml* pcxgml_t;

//struct pcxml_t;
typedef struct pcxml  pcxml;
typedef struct pcxml* pcxml_t;

//struct pchtml_t;
typedef struct pchtml  pchtml;
typedef struct pchtml* pchtml_t;

//struct pcedom_tag_id;
typedef struct pcedom_tag_id  pcedom_tag_id;
typedef struct pcedom_tag_id* pcedom_tag_id_t;

//struct pcedom_element;
typedef struct pcedom_element  pcedom_element;
typedef struct pcedom_element* pcedom_element_t;

// attribute type
//struct pcedom_attr;
typedef struct pcedom_attr  pcedom_attr;
typedef struct pcedom_attr* pcedom_attr_t;

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
