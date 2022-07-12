/**
 * @file purc-document.h
 * @author Vincent Wei
 * @date 2022/07/11
 * @brief The API of target document.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
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

#ifndef PURC_PURC_DOCUMENT_H
#define PURC_PURC_DOCUMENT_H

#include "purc-macros.h"
#include "purc-variant.h"

/* operations */
typedef enum {
    PCDOC_K_TYPE_FIRST = 0,
    PCDOC_K_TYPE_VOID = PCDOC_K_TYPE_FIRST,
#define PCDOC_TYPE_VOID                 "void"
    PCDOC_K_TYPE_PLAIN,
#define PCDOC_TYPE_PLAIN                "plain"
    PCDOC_K_TYPE_HTML,
#define PCDOC_TYPE_HTML                 "html"
    PCDOC_K_TYPE_XML,
#define PCDOC_TYPE_XML                  "xml"
    PCDOC_K_TYPE_XGML,
#define PCDOC_TYPE_XGML                 "xgml"

    /* XXX: change this when you append a new operation */
    PCDOC_K_TYPE_LAST = PCDOC_K_TYPE_XGML,
} purc_document_type;

#define PCDOC_NR_TYPES (PCDOC_K_TYPE_LAST - PCDOC_K_TYPE_FIRST + 1)

struct purc_document;
typedef struct purc_document purc_document;
typedef struct purc_document *purc_document_t;

struct pcdoc_element;
typedef struct pcdoc_element pcdoc_element;
typedef struct pcdoc_element *pcdoc_element_t;

struct pcdoc_text_node;
typedef struct pcdoc_text_node pcdoc_text_node;
typedef struct pcdoc_text_node *pcdoc_text_node_t;

struct pcdoc_data_node;
typedef struct pcdoc_data_node pcdoc_data_node;
typedef struct pcdoc_data_node *pcdoc_data_node_t;

struct pcdoc_node_others;
typedef struct pcdoc_node_others pcdoc_node_others;
typedef struct pcdoc_node_others *pcdoc_node_others_t;

typedef union {
    pcdoc_element_t     elem;
    pcdoc_text_node_t   text_node;
    pcdoc_data_node_t   data_node;
    pcdoc_node_others_t others;
} pcdoc_node_t;

struct pcdoc_elem_coll;
typedef struct pcdoc_elem_coll pcdoc_elem_coll;
typedef struct pcdoc_elem_coll *pcdoc_elem_coll_t;

PCA_EXTERN_C_BEGIN

/**
 * Create a new empty document.
 *
 * @param type: the type of the document.
 *
 * This function creates a new empty document in specific type.
 *
 * Returns: a pointer to the document.
 *
 * Since: 0.2.0
 */
PCA_EXPORT purc_document_t
purc_document_new(purc_document_type type);

/**
 * Create a new document by loading a content.
 *
 * @param type: a string contains the type of the document.
 * @param content: a string contains the content to load.
 * @param len: the len of the content, 0 for null-terminated string.
 *
 * This function creates a new empty document in specific type.
 *
 * Returns: a pointer to the document.
 *
 * Since: 0.2.0
 */
PCA_EXPORT purc_document_t
purc_document_load(purc_document_type type, const char *content, size_t len);

/**
 * Destroy a document.
 *
 * @param doc: The pointer to the document.
 *
 * This function deletes a document.
 *
 * Returns: a pointer to the document.
 *
 * Since: 0.2.0
 */
PCA_EXPORT void
purc_document_delete(purc_document_t doc);

typedef enum {
    PCDOC_SPECIAL_ELEM_ROOT = 0,
    PCDOC_SPECIAL_ELEM_HEAD,
    PCDOC_SPECIAL_ELEM_BODY,
} pcdoc_special_elem;

PCA_EXPORT pcdoc_element_t
purc_document_special_elem(purc_document_t doc, pcdoc_special_elem elem);

typedef enum {
    PCDOC_OP_APPEND = 1,
    PCDOC_OP_PREPEND,
    PCDOC_OP_INSERTBEFORE,
    PCDOC_OP_INSERTAFTER,
    PCDOC_OP_DISPLACE,
    PCDOC_OP_UPDATE,
    PCDOC_OP_ERASE,
    PCDOC_OP_CLEAR,
} pcdoc_operation;

typedef enum {
    PCDOC_NODE_ELEMENT = 0,
    PCDOC_NODE_TEXT_CONTENT,
    PCDOC_NODE_DATA_CONTENT,
    PCDOC_NODE_OTHERS,
} pcdoc_node_type;

/**
 * Create a new element with specific tag and insert it to
 * the specified position related to the specific element.
 *
 * @param elem: the pointer to an element.
 * @param op: The operation.
 * @param content: a string contains the content in the target markup language.
 * @param len: the len of the content, 0 for null-terminated string.
 *
 * Returns: The pointer to the new element.
 *
 * Since: 0.2.0
 */
PCA_EXPORT pcdoc_element_t
pcdoc_element_new_element(purc_document_t doc,
        pcdoc_element_t elem, pcdoc_operation op,
        const char *tag, bool self_close);

/**
 * Create a new text content and insert it to the specified position related
 * to the specific element.
 *
 * @param elem: the pointer to an element.
 * @param op: The operation.
 * @param content: a string contains the content in the target markup language.
 * @param len: the len of the content, 0 for null-terminated string.
 *
 * Returns: The pointer to the new text content.
 *
 * Since: 0.2.0
 */
PCA_EXPORT pcdoc_text_node_t
pcdoc_element_new_text_content(purc_document_t doc,
        pcdoc_element_t elem, pcdoc_operation op,
        const char *text, size_t len);

/**
 * Set the data content of an element.
 *
 * @param elem: the pointer to an element.
 * @param vrt: the data in PurC variant.
 *
 * Returns: The pointer to the new data content.
 *
 * Note that only XGML supports the data content.
 *
 * Since: 0.2.0
 */
PCA_EXPORT pcdoc_data_node_t
pcdoc_element_set_data_content(purc_document_t doc,
        pcdoc_element_t elem, pcdoc_operation op,
        purc_variant_t data);

/**
 * Insert or replace the content in target markup language of the specific
 * element.
 *
 * @param elem: the pointer to an element.
 * @param op: The operation.
 * @param content: a string contains the content in the target markup language.
 * @param len: the len of the content, 0 for null-terminated string.
 *
 * Returns: the root node of the content.
 *
 * Since: 0.2.0
 */
PCA_EXPORT pcdoc_node_t
pcdoc_element_new_content(purc_document_t doc,
        pcdoc_element_t elem, pcdoc_operation op,
        const char *content, size_t len, pcdoc_node_type *type);

/**
 * Set an attribute of the specified element.
 *
 * @param elem: the pointer to the element.
 * @param op: The operation, can be one of the following values:
 *  - PCDOC_OP_UPDATE: change the attribute value.
 *  - PCDOC_OP_ERASE: remove the attribute.
 *  - PCDOC_OP_CLEAR: clear the attribute value.
 * @param name: the name of the attribute.
 * @param value: the value of the attribute (nullable).
 *
 * Since: 0.2.0
 */
PCA_EXPORT bool
pcdoc_element_set_attribute(purc_document_t doc,
        pcdoc_element_t elem, pcdoc_operation op,
        const char *name, const char *val, size_t len);

/**
 * Get the attribute value of the specific attribute of the specified element.
 *
 * @param elem: the pointer to the element.
 * @param name: the name of the attribute.
 * @param value: the buffer to return the value of the attribute.
 *
 * Returns: @true for success, otherwise @false.
 *
 * Since: 0.2.0
 */
PCA_EXPORT bool
pcdoc_element_get_attribute(purc_document_t doc, pcdoc_element_t elem,
        const char *name, const char **val, size_t *len);

PCA_EXPORT bool
pcdoc_text_content_get_text(purc_document_t doc, pcdoc_text_node_t text_node,
        const char **text, size_t *len);

PCA_EXPORT bool
pcdoc_data_content_get_data(purc_document_t doc, pcdoc_data_node_t data_node,
        purc_variant_t *data);

PCA_EXPORT size_t
pcdoc_element_children_count(purc_document_t doc, pcdoc_element_t elem);

PCA_EXPORT pcdoc_node_t
pcdoc_element_get_child(purc_document_t doc, pcdoc_element_t elem,
        size_t idx, pcdoc_node_type *type);

/**
 * Get the parent element of a document node.
 *
 * Returns: the pointer to the parent element or @NULL if the node is the root.
 */
PCA_EXPORT pcdoc_element_t
pcdoc_node_get_parent(purc_document_t doc, pcdoc_node_t node);

/**
 * Find the first element matching the CSS selector from the descendants.
 *
 * Returns: the pointer to the matching element or @NULL if no such one.
 */
PCA_EXPORT pcdoc_element_t
pcdoc_find_element_in_descendants(purc_document_t doc,
        pcdoc_element_t ancestor, const char *selector);

/**
 * Find the first element matching the CSS selector in the document.
 *
 * Returns: the pointer to the matching element or @NULL if no such one.
 */
static inline pcdoc_element_t
pcdoc_find_element_in_document(purc_document_t doc, const char *selector)
{
    return pcdoc_find_element_in_descendants(doc, NULL, selector);
}

/**
 * Create an element collection by selecting the elements from the descendants
 * of the specified element according to the CSS selector.
 *
 * Returns: A pointer to the element collection; @NULL on failure.
 */
PCA_EXPORT pcdoc_elem_coll_t
pcdoc_elem_coll_new_from_descendants(purc_document_t doc,
        pcdoc_element_t ancestor, const char *selector);

/**
 * Create an element collection by selecting the elements from
 * the whole document according to the CSS selector.
 *
 * Returns: A pointer to the element collection; @NULL on failure.
 */
static inline pcdoc_elem_coll_t
pcdoc_elem_coll_new_from_document(purc_document_t doc,
        const char *selector)
{
    return pcdoc_elem_coll_new_from_descendants(doc, NULL, selector);
}

/**
 * Create a new element collection by selecting a part of elements
 * in the specific element collection.
 *
 * Returns: A pointer to the new element collection; @NULL on failure.
 */
PCA_EXPORT pcdoc_elem_coll_t
pcdoc_elem_coll_select(purc_document_t doc,
        pcdoc_elem_coll_t elem_coll, const char *selector);

/**
 * Delete the speicified element collection.
 */
PCA_EXPORT void
pcdoc_elem_coll_delete(purc_document_t doc,
        pcdoc_elem_coll_t elem_coll);

PCA_EXTERN_C_END

#endif  /* PURC_PURC_DOCUMENT_H */

