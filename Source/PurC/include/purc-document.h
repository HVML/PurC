/**
 * @file purc-document.h
 * @author Vincent Wei
 * @date 2022/07/11
 * @brief The API of target document.
 *
 * Copyright (C) 2022, 2025 FMSoft <https://www.fmsoft.cn>
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

/**
 * SECTION: purc_document
 * @title: Abstract Document
 * @short_description: The abstract representation of a structured document.
 */

/**
 * purc_document_type_k:
 * Document types
 */
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
} purc_document_type_k;

#define PCDOC_NR_TYPES (PCDOC_K_TYPE_LAST - PCDOC_K_TYPE_FIRST + 1)

/**
 * purc_namespace_type_k:
 * Namespace types
 */
typedef enum {
    PCDOC_K_NAMESPACE_FIRST = 0,
    PCDOC_K_NAMESPACE__UNDEF = PCDOC_K_NAMESPACE_FIRST,
#define PCDOC_NSNAME__UNDEF   ""
    PCDOC_K_NAMESPACE_HTML,
#define PCDOC_NSNAME_HTML     "html"
    PCDOC_K_NAMESPACE_MATHML,
#define PCDOC_NSNAME_MATHML   "mathml"
    PCDOC_K_NAMESPACE_SVG,
#define PCDOC_NSNAME_SVG      "svg"
    PCDOC_K_NAMESPACE_XGML,
#define PCDOC_NSNAME_XGML     "xgml"
    PCDOC_K_NAMESPACE_XLINK,
#define PCDOC_NSNAME_XLINK    "xlink"
    PCDOC_K_NAMESPACE_XML,
#define PCDOC_NSNAME_XML      "xml"
    PCDOC_K_NAMESPACE_XMLNS,
#define PCDOC_NSNAME_XMLNS    "xmlns"

    /* XXX: change this when you append a new operation */
    PCDOC_K_NAMESPACE_LAST = PCDOC_K_NAMESPACE_XMLNS,
} purc_namespace_type_k;

#define PCDOC_NR_NAMESPACES     (PCDOC_K_NS_LAST - PCDOC_K_NS_FIRST + 1)

/* Special document type */
#define PCDOC_K_STYPE_INHERIT           "_inherit"

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

struct pcdoc_attr;
typedef struct pcdoc_attr pcdoc_attr;
typedef struct pcdoc_attr *pcdoc_attr_t;

/**
 * pcdoc_node_type_k:
 * Document node types.
 */
typedef enum {
    PCDOC_NODE_ELEMENT = 0,
    PCDOC_NODE_TEXT,
    PCDOC_NODE_DATA,
    PCDOC_NODE_CDATA_SECTION,
    PCDOC_NODE_OTHERS,  // DOCUMENT, DOCTYPE, COMMENT, ...
    PCDOC_NODE_VOID,    // NOTHING
} pcdoc_node_type_k;

typedef struct {
    pcdoc_node_type_k       type;
    union {
        void               *data;
        pcdoc_element_t     elem;
        pcdoc_text_node_t   text_node;
        pcdoc_data_node_t   data_node;
        pcdoc_node_others_t others;
    };
} pcdoc_node;

struct pcdoc_selector;
typedef struct pcdoc_selector pcdoc_selector;
typedef struct pcdoc_selector *pcdoc_selector_t;

struct pcdoc_elem_coll;
typedef struct pcdoc_elem_coll pcdoc_elem_coll;
typedef struct pcdoc_elem_coll *pcdoc_elem_coll_t;


PCA_EXTERN_C_BEGIN

/**
 * purc_document_retrieve_type:
 *
 * Gets the document type for a specific target name.
 *
 * @target_name: the target name.
 *
 * This function retrieves the document type for a specific target name
 * and returns it.
 *
 * Returns: A supported document type.
 *
 * Since: 0.2.0
 */
PCA_EXPORT purc_document_type_k
purc_document_retrieve_type(const char *target_name);

/**
 * purc_document_new:
 *
 * Creates a new empty document.
 *
 * @type: The type of the document.
 *
 * This function creates a new empty document in specific type.
 *
 * Returns: A pointer to the document.
 *
 * Since: 0.2.0
 */
PCA_EXPORT purc_document_t
purc_document_new(purc_document_type_k type);

/**
 * purc_document_type:
 *
 * Gets the document type of a given document.
 *
 * @doc: the document.
 *
 * This function returns the document type for a specific document.
 *
 * Returns: The document type of the given document.
 *
 * Since: 0.9.18
 */
PCA_EXPORT purc_document_type_k
purc_document_type(purc_document_t doc);

/**
 * purc_document_get_refc:
 *
 * Gets the reference count of an existing document.
 *
 * @doc: The pointer to a document.
 *
 * This function returns the current reference count of the given document.
 *
 * Returns: The current reference count.
 *
 * Since: 0.2.0
 */
PCA_EXPORT unsigned int
purc_document_get_refc(purc_document_t doc);

/**
 * purc_document_ref:
 *
 * References an existing document.
 *
 * @doc: The pointer to a document.
 *
 * This function increases the reference count of an existing document.
 *
 * Returns: the document.
 *
 * Since: 0.2.0
 */
PCA_EXPORT purc_document_t
purc_document_ref(purc_document_t doc);

/**
 * purc_document_unref:
 *
 * Un-references an existing document.
 *
 * @doc: The pointer to a document.
 *
 * This function decreases the reference count of an existing document,
 * when the reference count reaches zero, the function will delete
 * the document by calling `purc_document_delete()`.
 *
 * Returns: The new reference count. If it is zero, that means the document
 *  has been deleted.
 *
 * Since: 0.2.0
 */
PCA_EXPORT unsigned int
purc_document_unref(purc_document_t doc);

/**
 * purc_document_load:
 *
 * Creates a new document by loading a content.
 *
 * @type: A string contains the type of the document.
 * @content: A string contains the content to load.
 * @len: The length of the content, 0 for null-terminated string.
 *
 * This function creates a new empty document in specific type.
 *
 * Returns: A pointer to the document; %NULL on error.
 *
 * Since: 0.2.0
 */
PCA_EXPORT purc_document_t
purc_document_load(purc_document_type_k type, const char *content, size_t len);

/**
 * purc_document_impl_entity:
 *
 * Gets the underlying implementation entity of a document.
 *
 * @doc: The pointer to the document.
 * @type: A pointer to a purc_document_type_k buffer to receive
 *      the document type.
 *
 * This function returns the underlying implementation entity of
 * the given document; it is generally a pointer.
 *
 * Returns: A pointer to the underlying implementation which
 *      represents the document.
 *
 * Since: 0.9.0
 */
PCA_EXPORT void *
purc_document_impl_entity(purc_document_t doc, purc_document_type_k *type);

/**
 * purc_document_delete:
 *
 * Deletes a document.
 *
 * @doc: The pointer to a document.
 *
 * This function deletes a document whatever the reference count is.
 *
 * Returns: The reference count when deleting the document.
 *
 * Since: 0.2.0
 */
PCA_EXPORT unsigned int
purc_document_delete(purc_document_t doc);

typedef enum {
    PCDOC_SPECIAL_ELEM_ROOT = 0,
    PCDOC_SPECIAL_ELEM_HEAD,
    PCDOC_SPECIAL_ELEM_BODY,
} pcdoc_special_elem_k;

/**
 * purc_document_special_elem:
 *
 * Retrieves the special element of a doucment.
 *
 * @doc: The pointer to a document.
 * @elem: The identifier of the special element, one of the following values:
 *  - %PCDOC_SPECIAL_ELEM_ROOT: The root element.
 *  - %PCDOC_SPECIAL_ELEM_HEAD: The head element.
 *  - %PCDOC_SPECIAL_ELEM_BODY: The body element.
 *
 * This function retrieves the special element of a document. If the document
 * does not contain the specified element, it returns %NULL.
 *
 * Returns: The pointer to pcdoc_element structure or %NULL for no such element.
 *
 * Since: 0.2.0
 */
PCA_EXPORT pcdoc_element_t
purc_document_special_elem(purc_document_t doc, pcdoc_special_elem_k elem);

/**
 * purc_document_root:
 *
 * Retrieves the root element of a doucment.
 *
 * @doc: The pointer to a document.
 *
 * This function retrieves the root element of a document.
 *
 * Returns: The pointer to pcdoc_element structure or %NULL on error.
 *
 * Since: 0.2.0
 */
static inline pcdoc_element_t purc_document_root(purc_document_t doc)
{
    return purc_document_special_elem(doc, PCDOC_SPECIAL_ELEM_ROOT);
}

/**
 * purc_document_head:
 *
 * Retrieves the head element of a doucment.
 *
 * @doc: The pointer to a document.
 *
 * This function retrieves the head element of a document.
 *
 * Returns: The pointer to pcdoc_element structure or %NULL on error.
 *
 * Since: 0.2.0
 */
static inline pcdoc_element_t purc_document_head(purc_document_t doc)
{
    return purc_document_special_elem(doc, PCDOC_SPECIAL_ELEM_HEAD);
}

/**
 * purc_document_body:
 *
 * Retrieves the body element of a doucment.
 *
 * @doc: The pointer to a document.
 *
 * This function retrieves the body element of a document.
 *
 * Returns: The pointer to pcdoc_element structure or %NULL on error.
 *
 * Since: 0.2.0
 */
static inline pcdoc_element_t purc_document_body(purc_document_t doc)
{
    return purc_document_special_elem(doc, PCDOC_SPECIAL_ELEM_BODY);
}

/**
 * purc_document_set_global_selector:
 *
 * Sets the global selector of a document.
 *
 * @doc: The pointer to a document.
 * @selector: A string contains the global selector.
 *
 * This function sets the global selector of a document.
 *
 * Returns: The previous global selector of the document.
 *
 *  Since: 0.9.24
 */
const char *purc_document_set_global_selector(purc_document_t doc,
        const char *selector);

/**
* purc_document_get_global_selector:
*
* Gets the global selector of a document.
*
* @doc: The pointer to a document.
*
* This function gets the global selector of a document.
*
* Returns: The global selector of the document, or %NULL if not set.
*
* Since: 0.9.24
*/
const char *purc_document_get_global_selector(purc_document_t doc);

/**
 * pcdoc_document_lock_for_read:
 *
 * Locks a document for read access.
 *
 * @doc: The pointer to a document.
 *
 * This function locks a document for read access. Multiple threads can
 * lock a document for read access simultaneously.
 *
 * Returns: 0 on success, -1 on failure.
 *
 * Since: 0.9.26
 */
PCA_EXPORT int
pcdoc_document_lock_for_read(purc_document_t doc);

/**
 * pcdoc_document_lock_for_write:
 *
 * Locks a document for write access.
 *
 * @doc: The pointer to a document.
 *
 * This function locks a document for write access. Only one thread can
 * lock a document for write access at a time.
 *
 * Returns: 0 on success, -1 on failure.
 *
 * Since: 0.9.26
 */
PCA_EXPORT int
pcdoc_document_lock_for_write(purc_document_t doc);

/**
 * pcdoc_document_unlock:
 *
 * Unlocks a document.
 *
 * @doc: The pointer to a document.
 *
 * This function unlocks a document that was previously locked for
 * read or write access.
 *
 * Returns: 0 on success, -1 on failure.
 *
 * Since: 0.9.26
 */
PCA_EXPORT int
pcdoc_document_unlock(purc_document_t doc);

/**
 * pcdoc_document_update_count:
 *
 * Returns the number of update requests sent to renderer for a document.
 *
 * @doc: The pointer to a document.
 *
 * This function returns the number of update requests sent to renderer
 * for a document. If the document does not have a read-write lock initialized,
 * it always returns 0.
 *
 * Returns: The number of update requests.
 *
 * Since: 0.9.26
 */
PCA_EXPORT size_t
pcdoc_document_update_count(purc_document_t doc);

typedef enum {
    PCDOC_OP_APPEND = 0,
    PCDOC_OP_PREPEND,
    PCDOC_OP_INSERTBEFORE,
    PCDOC_OP_INSERTAFTER,
    PCDOC_OP_DISPLACE,
    PCDOC_OP_UPDATE,
    PCDOC_OP_ERASE,
    PCDOC_OP_CLEAR,
    PCDOC_OP_UNKNOWN,
} pcdoc_operation_k;

/**
 * pcdoc_element_new_element:
 *
 * Creates a new element in a document.
 *
 * @doc: The pointer to a document.
 * @elem: The pointer to an element.
 * @op: The operation.
 * @tag: A string contains the tag name in the target markup language.
 * @self_close: Indicate whether is a self-close element.
 *
 * This function creates a new element with specific tag and inserts it to
 * the specified position related to an existing element.
 *
 * Returns: The pointer to the new element.
 *
 * Since: 0.2.0
 */
PCA_EXPORT pcdoc_element_t
pcdoc_element_new_element(purc_document_t doc,
        pcdoc_element_t elem, pcdoc_operation_k op,
        const char *tag, bool self_close);

/**
 * pcdoc_element_clear:
 *
 * Clears the contents of an element.
 *
 * @doc: The pointer to a document.
 * @elem: the pointer to an element.
 *
 * This functions clears all contents of an element.
 *
 * Returns: None.
 *
 * Since: 0.2.0
 */
PCA_EXPORT void
pcdoc_element_clear(purc_document_t doc, pcdoc_element_t elem);

/**
 * pcdoc_element_erase:
 *
 * Removes an element.
 *
 * @doc: The pointer to a document.
 * @elem: the pointer to an element.
 *
 * This function removes an element and its contents.
 *
 * Since: 0.2.0
 */
PCA_EXPORT void
pcdoc_element_erase(purc_document_t doc, pcdoc_element_t elem);

/**
 * pcdoc_element_new_text_content:
 *
 * Creates new text content for an element.
 *
 * @doc: The pointer to a document.
 * @elem: The pointer to an element.
 * @op: The operation.
 * @content: A string contains the content in the target markup language.
 * @len: The length of the content, 0 for null-terminated string.
 *
 * This function creates a new text content and inserts it to the specified
 * position for an existing element.
 *
 * Returns: The pointer to the new text content.
 *
 * Since: 0.2.0
 */
PCA_EXPORT pcdoc_text_node_t
pcdoc_element_new_text_content(purc_document_t doc,
        pcdoc_element_t elem, pcdoc_operation_k op,
        const char *text, size_t len);

/**
 * pcdoc_element_set_data_content:
 *
 * Sets the data content of an element.
 *
 * @doc: The pointer to a document.
 * @elem: The pointer to an element.
 * @op: The operation.
 * @data: The data represented by #purc_variant.
 *
 * This function sets the data content of an existing element.
 * The old data content will be replaced.
 *
 * Note that only XGML supports the data content.
 *
 * Returns: The pointer to the new data content.
 *
 * Since: 0.2.0
 */
PCA_EXPORT pcdoc_data_node_t
pcdoc_element_set_data_content(purc_document_t doc,
        pcdoc_element_t elem, pcdoc_operation_k op,
        purc_variant_t data);

/**
 * pcdoc_element_new_content:
 *
 * Inserts or replaces the content of an element.
 *
 * @doc: The pointer to a document.
 * @elem: The pointer to an element.
 * @op: The operation.
 * @content: A string contains the content in the target markup language.
 * @len: The length of the content, 0 for null-terminated string.
 *
 * This function inserts or replaces the contents in target markup language
 * of the specific element.
 *
 * Returns: The root node of the content.
 *
 * Since: 0.2.0
 */
PCA_EXPORT pcdoc_node
pcdoc_element_new_content(purc_document_t doc,
        pcdoc_element_t elem, pcdoc_operation_k op,
        const char *content, size_t len);

/**
 * pcdoc_element_get_tag_name:
 *
 * Gets the tag name of a specific element.
 *
 * @doc: the pointer to the document.
 * @elem: the pointer to the element.
 * @local_name: the location to return the local name of the element.
 * @local_len: the location to return the length of the local name.
 * @prefix: the location to return the prefix of the tag name of the element.
 * @prefix_len: the location to return the length of the prefix.
 * @ns_name: the location to return the namespace name of the element.
 * @ns_len: the location to return the length of the namespace name.
 *
 * This function gets the tag name and the namespace of an element.
 *
 * Returns: 0 for success, -1 for failure.
 *
 * Since: 0.9.0
 */
PCA_EXPORT int
pcdoc_element_get_tag_name(purc_document_t doc, pcdoc_element_t elem,
        const char **local_name, size_t *local_len,
        const char **prefix, size_t *prefix_len,
        const char **ns_name, size_t *ns_len);

/**
 * pcdoc_element_set_attribute:
 *
 * Sets an attribute of an element.
 *
 * @doc: The pointer to a document.
 * @elem: The pointer to an element.
 * @op: The operation, can be one of the following values:
 *  - %PCDOC_OP_DISPLACE: set or update the attribute value.
 *  - %PCDOC_OP_ERASE: remove the attribute.
 *  - %PCDOC_OP_CLEAR: clear the attribute value.
 * @name: The name of the attribute, must be null-terminated.
 * @val (nullable): the value of the attribute.
 * @len: The length of the attribute value in bytes.
 *
 * This function sets an attribute of the given element.
 *
 * Returns: 0 for success, -1 for failure.
 *
 * Since: 0.2.0
 */
PCA_EXPORT int
pcdoc_element_set_attribute(purc_document_t doc,
        pcdoc_element_t elem, pcdoc_operation_k op,
        const char *name, const char *val, size_t len);

/**
 * pcdoc_element_remove_attribute:
 *
 * Removes an attribute of an element.
 *
 * @doc: The pointer to a document.
 * @elem: The pointer to an element.
 * @name: The name of the attribute, must be null-terminated.
 *
 * This function removes an attribute of the given element.
 *
 * Returns: 0 for success, -1 for failure.
 *
 * Since: 0.2.0
 */
static inline int pcdoc_element_remove_attribute(purc_document_t doc,
        pcdoc_element_t elem, const char *name)
{
    return pcdoc_element_set_attribute(doc, elem, PCDOC_OP_ERASE,
        name, NULL, 0);
}

/**
 * pcdoc_element_get_attribute:
 *
 * Gets the attribute value of the specific attribute of an element.
 *
 * @doc: the pointer to the document.
 * @elem: the pointer to the element.
 * @name: the name of the attribute.
 * @val: the buffer to return the value of the attribute.
 * @len (nullable): the buffer to return the length of the value.
 *
 * This function gets the attribute value of the specific attribute of
 * the given element.
 *
 * Returns: 0 for success, -1 for failure.
 *
 * Since: 0.2.0
 */
PCA_EXPORT int
pcdoc_element_get_attribute(purc_document_t doc, pcdoc_element_t elem,
        const char *name, const char **val, size_t *len);

typedef enum {
    PCDOC_ATTR_ID = 0,
    PCDOC_ATTR_CLASS,
} pcdoc_special_attr_k;

/**
 * pcdoc_element_get_special_attr:
 *
 * @doc: The pointer to the document.
 * @elem: The pointer to the element.
 * @which: An enum value to distinguish the special attribute, which can be
 *  one of the following values:
 *      - %PCDOC_ATTR_ID
 *      - %PCDOC_ATTR_CLASS
 * @val: The pointer to a const char* buffer to return the value
 *  of the attribute.
 * @len (nullable): The pointer to a size_t buffer to return the length
 *  (in bytes) of the value.
 *
 * Gets the value of a special attribute of the specified element.
 *
 * Returns: 0 for success, -1 for no specified attribute defined.
 *
 * Since: 0.2.0
 */
PCA_EXPORT int
pcdoc_element_get_special_attr(purc_document_t doc, pcdoc_element_t elem,
        pcdoc_special_attr_k which, const char **val, size_t *len);

/**
 * pcdoc_element_id:
 *
 * @doc: The pointer to the document.
 * @elem: The pointer to the element.
 * @len (nullable): The pointer to a size_t buffer to return the length
 *  of the value in bytes.
 *
 * Gets the value of `id` attribute of the specified element.
 *
 * Returns: The pointer to the value of `id` attribute on success,
 *  %NULL for no `id` attribute defined.
 *
 * Since: 0.2.0
 */
static inline const char *
pcdoc_element_id(purc_document_t doc, pcdoc_element_t elem, size_t *len)
{
    const char *val;

    if (pcdoc_element_get_special_attr(doc, elem, PCDOC_ATTR_ID,
                &val, len)) {
        if (len != NULL) {
            *len = 0;
        }

        return NULL;
    }

    return val;
}

/**
 * pcdoc_element_class:
 *
 * @doc: The pointer to the document.
 * @elem: The pointer to the element.
 * @len (nullable): The pointer to a size_t buffer to return the length
 *  of the value in bytes.
 *
 * Gets the value of `class` attribute of the specified element.
 *
 * Returns: The pointer to the value of `id` attribute on success,
 *  %NULL for no `id` attribute defined.
 *
 * Since: 0.2.0
 */
static inline const char *
pcdoc_element_class(purc_document_t doc, pcdoc_element_t elem, size_t *len)
{
    const char *val;

    if (pcdoc_element_get_special_attr(doc, elem, PCDOC_ATTR_CLASS,
                &val, len)) {
        if (len != NULL) {
            *len = 0;
        }

        return NULL;
    }

    return val;
}

/**
 * pcdoc_element_has_class:
 *
 * @doc: The pointer to the document.
 * @elem: The pointer to the element.
 * @klass: The pointer to the class name (null-terminated string) to check.
 * @found: The pointer to a bool buffer to return the result.
 *
 * Checks whether a class name given by @klass is defined for the specified
 * element.
 *
 * Returns: 0 for success (the reult is reliable), -1 for bad class name or
 *  other errors.
 *
 * Since: 0.2.0
 */
PCA_EXPORT int
pcdoc_element_has_class(purc_document_t doc, pcdoc_element_t elem,
        const char *klass, bool *found);

/**
 * pcdoc_attribute_cb:
 *
 * The callback for an attribute traveled. If the callback returns
 * %PCDOC_TRAVEL_GOON, the travel will go on; returns %PCDOC_TRAVEL_STOP
 * to stop the travel.
 */
typedef int (*pcdoc_attribute_cb)(purc_document_t doc, pcdoc_attr_t attr,
        const char *name, size_t name_len,
        const char *value, size_t value_len, void *ctxt);

/**
 * pcdoc_element_travel_attributes:
 *
 * @doc: The pointer to the document.
 * @elem: The pointer to the element.
 * @cb: The pointer to the callback for the attribute found.
 * @ctxt: The pointer to the context data will be passed to the callback.
 * @n: The pointer to a size_t buffer to returned the number of attributes
 *  travelled.
 *
 * Travels all attributes of the specified element.
 *
 * Returns: 0 for all attributes travelled, otherwise the traverse was broken
 * by the callback.
 *
 * Since: 0.9.0
 */
PCA_EXPORT int
pcdoc_element_travel_attributes(purc_document_t doc, pcdoc_element_t elem,
        pcdoc_attribute_cb cb, void *ctxt, size_t *n);

/**
 * pcdoc_element_first_attr:
 *
 * @doc: The pointer to the document.
 * @elem: The pointer to the element.
 *
 * Gets the first attribute of the specified element.
 *
 * Returns: The pointer to the desired attribute, %NULL for no such attribute.
 *
 * Since: 0.9.0
 */
PCA_EXPORT pcdoc_attr_t
pcdoc_element_first_attr(purc_document_t doc, pcdoc_element_t elem);

/**
 * pcdoc_element_last_attr:
 *
 * @doc: The pointer to the document.
 * @elem: The pointer to the element.
 *
 * Gets the last attribute of the specified element.
 *
 * Returns: The pointer to the desired attribute, %NULL for no such attribute.
 *
 * Since: 0.9.0
 */
PCA_EXPORT pcdoc_attr_t
pcdoc_element_last_attr(purc_document_t doc, pcdoc_element_t elem);

/**
 * pcdoc_attr_next_sibling:
 *
 * @doc: The pointer to the document.
 * @attr: The pointer to the current attribute.
 *
 * Gets the next sibling attribute of the current attribute.
 *
 * Returns: The pointer to the desired attribute, %NULL for no such attribute.
 *
 * Since: 0.9.0
 */
PCA_EXPORT pcdoc_attr_t
pcdoc_attr_next_sibling(purc_document_t doc, pcdoc_attr_t attr);

/**
 * pcdoc_attr_prev_sibling:
 *
 * @doc: The pointer to the document.
 * @attr: The pointer to the current attribute.
 *
 * Gets the previous sibling of the current attribute.
 *
 * Returns: The pointer to the desired attribute, %NULL for no such attribute.
 *
 * Since: 0.9.0
 */
PCA_EXPORT pcdoc_attr_t
pcdoc_attr_prev_sibling(purc_document_t doc, pcdoc_attr_t attr);

/**
 * pcdoc_attr_get_info:
 *
 * @doc: The pointer to the document.
 * @attr: The pointer to the attribute.
 * @local_name: The pointer to the location to return
 *      the local name of the attribute.
 * @local_len: The pointer to a size_t buffer to return
 *      the length of the local name.
 * @qualified_name (nullable): The pointer to the location to return
 *      the qualified name (with prefix) of the attribute.
 * @qualified_len (nullable): The pointer to a size_t buffer to return
 *      the length of the qualified name.
 * @value (nullable): The pointer to the location to return
 *      the value of the attribute.
 * @value_len (nullable): The pointer to a size_t buffer to return
 *      the length of the value.
 *
 * Get the name and value of a specific attribute.
 *
 * Returns: 0 for success, -1 for failure.
 *
 * Since: 0.9.0
 */
PCA_EXPORT int
pcdoc_attr_get_info(purc_document_t doc, pcdoc_attr_t attr,
        const char **local_name, size_t *local_len,
        const char **qualified_name, size_t *qualified_len,
        const char **value, size_t *value_len);

/**
 * pcdoc_text_content_get_text:
 *
 * @doc: The pointer to the document.
 * @text_ndoe: The pointer to the text node.
 * @text: The buffer to a location to return the pointer to the text content.
 * @len (nullable): The pointer to a size_t buffer to return the length (in
 *  bytes) of the text.
 *
 * Gets the text of a text node.
 *
 * Returns: 0 for success (the reult is reliable), -1 for errors.
 *
 * Since: 0.2.0
 */
PCA_EXPORT int
pcdoc_text_content_get_text(purc_document_t doc, pcdoc_text_node_t text_node,
        const char **text, size_t *len);

/**
 * pcdoc_data_content_get_data:
 *
 * @doc: The pointer to the document.
 * @data_node: The pointer to the data node.
 * @data: The pointer to a purc_variant_t buffer to return the variant.
 *
 * Gets the variant of a data node.
 *
 * Returns: 0 for success (the reult is reliable), -1 on failure.
 *
 * Since: 0.2.0
 */
PCA_EXPORT int
pcdoc_data_content_get_data(purc_document_t doc, pcdoc_data_node_t data_node,
        purc_variant_t *data);

/**
 * pcdoc_element_first_child:
 *
 * @doc: The pointer to the document.
 * @elem: The pointer to the element.
 *
 * Gets the first child node of the specified element.
 *
 * Returns: The desired node, %PCDOC_NODE_VOID type for no such node.
 *
 * Since: 0.9.0
 */
PCA_EXPORT pcdoc_node
pcdoc_element_first_child(purc_document_t doc, pcdoc_element_t elem);

/**
 * pcdoc_element_last_child:
 *
 * @doc: The pointer to the document.
 * @elem: The pointer to the element.
 *
 * Gets the last child node of the specified element.
 *
 * Returns: The desired node, %PCDOC_NODE_VOID type for no such node.
 *
 * Since: 0.9.0
 */
PCA_EXPORT pcdoc_node
pcdoc_element_last_child(purc_document_t doc, pcdoc_element_t elem);

/**
 * pcdoc_node_next_sibling:
 *
 * @doc: The pointer to the document.
 * @node: The current node.
 *
 * Gets the next sibling node of the current node.
 *
 * Returns: The desired node, %PCDOC_NODE_VOID type for no such node.
 *
 * Since: 0.9.0
 */
PCA_EXPORT pcdoc_node
pcdoc_node_next_sibling(purc_document_t doc, pcdoc_node node);

/**
 * pcdoc_node_prev_sibling:
 *
 * @doc: The pointer to the document.
 * @node: The current node.
 *
 * Gets the previous sibling node of the current node.
 *
 * Returns: The desired node, %PCDOC_NODE_VOID type for no such node.
 *
 * Since: 0.9.0
 */
PCA_EXPORT pcdoc_node
pcdoc_node_prev_sibling(purc_document_t doc, pcdoc_node node);

/**
 * pcdoc_node_get_user_data:
 *
 * @doc: The pointer to the document.
 * @node: The node.
 * @user_data The pointer to a location to receive the user data.
 *
 * Gets the user data of the specified node.
 *
 * Returns: 0 for success, -1 on failure.
 *
 * Since: 0.9.0
 */
PCA_EXPORT int
pcdoc_node_get_user_data(purc_document_t doc, pcdoc_node node,
        void **user_data);

/**
 * pcdoc_node_set_user_data:
 *
 * @doc: The pointer to the document.
 * @node: The node.
 * @user_data The user data to set.
 *
 * Sets the user data of the specified node.
 *
 * Returns: 0 for success, -1 on failure.
 *
 * Since: 0.9.0
 */
PCA_EXPORT int
pcdoc_node_set_user_data(purc_document_t doc, pcdoc_node node,
        void *user_data);

/**
 * pcdoc_element_children_count:
 *
 * @doc: The pointer to the document.
 * @elem: The pointer to the element.
 * @nr_elements (nullable):
 *      The pointer to a size_t buffer to return the number of children elements.
 * @nr_text_nodes (nullable):
 *      The pointer to a size_t buffer to return the number of text nodes.
 * @nr_data_nodes (nullable):
 *      The pointer to a size_t buffer to return the number of data nodes.
 *
 * Gets the number of different children nodes of the specified element.
 *
 * Returns: 0 for success, -1 on failure.
 *
 * Since: 0.2.0
 */
PCA_EXPORT int
pcdoc_element_children_count(purc_document_t doc, pcdoc_element_t elem,
        size_t *nr_elements, size_t *nr_text_nodes, size_t *nr_data_nodes);

/**
 * pcdoc_element_get_child_element:
 *
 * @doc: The pointer to the document.
 * @elem: The pointer to the element.
 * @idx: The index of the desired child element.
 *
 * Gets the specified child element of an element by index.
 *
 * Returns: The pointer to the child element; %NULL for the bad index value.
 *
 * Since: 0.2.0
 */
PCA_EXPORT pcdoc_element_t
pcdoc_element_get_child_element(purc_document_t doc, pcdoc_element_t elem,
        size_t idx);

/**
 * pcdoc_element_get_child_text_node:
 *
 * @doc: The pointer to the document.
 * @elem: The pointer to the element.
 * @idx: The index of the desired child text node.
 *
 * Gets the specified child text node of an element by index.
 *
 * Returns: The pointer to the child text node; %NULL for the bad index value.
 *
 * Since: 0.2.0
 */
PCA_EXPORT pcdoc_text_node_t
pcdoc_element_get_child_text_node(purc_document_t doc, pcdoc_element_t elem,
        size_t idx);

/**
 * pcdoc_element_get_child_data_node:
 *
 * @doc: The pointer to the document.
 * @elem: The pointer to the element.
 * @idx: The index of the desired child data node.
 *
 * Gets the specified child data node of an element by index.
 *
 * Returns: The pointer to the child data node; %NULL for the bad index value.
 *
 * Since: 0.2.0
 */
PCA_EXPORT pcdoc_data_node_t
pcdoc_element_get_child_data_node(purc_document_t doc, pcdoc_element_t elem,
        size_t idx);

/**
 * pcdoc_node_get_parent:
 *
 * @doc: The pointer to the document.
 * @node: The node.
 *
 * Gets the parent element of a document node.
 *
 * Returns: The pointer to the parent element or %NULL if the node is the root.
 *
 * Since: 0.2.0
 */
PCA_EXPORT pcdoc_element_t
pcdoc_node_get_parent(purc_document_t doc, pcdoc_node node);

#define PCDOC_TRAVEL_GOON       (0)
#define PCDOC_TRAVEL_STOP       (-1)
#define PCDOC_TRAVEL_SKIP       (1)

/**
 * pcdoc_element_cb:
 *
 * The callback for traveling descendant elements.
 * Returns
 *  - PCDOC_TRAVEL_GOON for continuing the travel,
 *  - PCDOC_TRAVEL_STOP for stopping the travel,
 *  - PCDOC_TRAVEL_SKIP for skipping the descendants.
 */
typedef int (*pcdoc_element_cb)(purc_document_t doc,
        pcdoc_element_t element, void *ctxt);

/**
 * pcdoc_travel_descendant_elements:
 *
 * @doc: The pointer to the document.
 * @ancestor (nullable): The pointer to the ancestor of the subtree.
 *      %NULL for the root element of the document.
 * @cb: The callback for the element travelled.
 * @ctxt: The pointer to the context data which will be passed to the callback.
 * @n: The pointer to a size_t buffer to return the number of elements
 *  travelled, i.e., the number of times the callback function was called.
 *
 * Travels all descendant elements in the subtree.
 *
 * Returns: 0 for all descendants travelled, otherwise the traverse was broken
 * by the callback.
 *
 * Since: 0.2.0
 */
PCA_EXPORT int
pcdoc_travel_descendant_elements(purc_document_t doc,
        pcdoc_element_t ancestor, pcdoc_element_cb cb, void *ctxt, size_t *n);

/**
 * pcdoc_text_node_cb:
 *
 * The callback for traveling descendant text nodes.
 * Returns
 *  - PCDOC_TRAVEL_GOON for continuing the travel,
 *  - PCDOC_TRAVEL_STOP for stopping the travel,
 *  - PCDOC_TRAVEL_SKIP for skipping the descendants.
 */
typedef int (*pcdoc_text_node_cb)(purc_document_t doc,
        pcdoc_text_node_t text_node, void *ctxt);

/**
 * pcdoc_travel_descendant_text_nodes:
 *
 * @doc: The pointer to the document.
 * @ancestor (nullable): The pointer to the ancestor of the subtree.
 *      %NULL for the root element of the document.
 * @cb: The callback for the text node travelled.
 * @ctxt: The pointer to the context data which will be passed to the callback.
 * @n: The pointer to a size_t buffer to return the number of text nodes
 *  travelled.
 *
 * Travels all descendant text nodes in the subtree.
 *
 * Returns: 0 for all descendant text nodes travelled, otherwise the traverse
 * was broken by the callback.
 *
 * Since: 0.2.0
 */
PCA_EXPORT int
pcdoc_travel_descendant_text_nodes(purc_document_t doc,
        pcdoc_element_t ancestor, pcdoc_text_node_cb cb, void *ctxt, size_t *n);

/**
 * The callback for traveling descendant data nodes.
 * Returns
 *  - PCDOC_TRAVEL_GOON for continuing the travel,
 *  - PCDOC_TRAVEL_STOP for stopping the travel,
 *  - PCDOC_TRAVEL_SKIP for skipping the descendants.
 */
typedef int (*pcdoc_data_node_cb)(purc_document_t doc,
        pcdoc_data_node_t data_node, void *ctxt);

/**
 * pcdoc_travel_descendant_data_nodes:
 *
 * @doc: The pointer to the document.
 * @ancestor (nullable): The pointer to the ancestor of the subtree.
 *      %NULL for the root element of the document.
 * @cb: The callback for the text node travelled.
 * @ctxt: The pointer to the context data which will be passed to the callback.
 * @n: The pointer to a size_t buffer to return the number of data nodes
 *  travelled.
 *
 * Travels all descendant data nodes in the subtree.
 *
 * Returns: 0 for all descendant data nodes travelled, otherwise the traverse
 * was broken by the callback.
 *
 * Since: 0.2.0
 */
PCA_EXPORT int
pcdoc_travel_descendant_data_nodes(purc_document_t doc,
        pcdoc_element_t ancestor, pcdoc_data_node_cb cb, void *ctxt, size_t *n);

/* XXX: keep sync with pchtml_html_serialize_opt in purc-html.h */
enum pcdoc_serialize_opt {
    PCDOC_SERIALIZE_OPT_UNDEF               = 0x0000,
    PCDOC_SERIALIZE_OPT_SKIP_WS_NODES       = 0x0001,
    PCDOC_SERIALIZE_OPT_SKIP_COMMENT        = 0x0002,
    PCDOC_SERIALIZE_OPT_RAW                 = 0x0004,
    PCDOC_SERIALIZE_OPT_WITHOUT_CLOSING     = 0x0008,
    PCDOC_SERIALIZE_OPT_TAG_WITH_NS         = 0x0010,
    PCDOC_SERIALIZE_OPT_WITHOUT_TEXT_INDENT = 0x0020,
    PCDOC_SERIALIZE_OPT_FULL_DOCTYPE        = 0x0040,
    PCDOC_SERIALIZE_OPT_WITH_HVML_HANDLE    = 0x0080,
    PCDOC_SERIALIZE_OPT_MASK_C0CTRLS        = 0x0F00,
    PCDOC_SERIALIZE_OPT_KEEP_C0CTRLS        = 0x0000,
    PCDOC_SERIALIZE_OPT_IGNORE_C0CTRLS      = 0x0100,
    PCDOC_SERIALIZE_OPT_READABLE_C0CTRLS    = 0x0200,
};

/**
 * pcdoc_serialize_text_contents_to_stream:
 *
 * @doc: The pointer to a document.
 * @ancestor (nullable): The pointer to the ancestor of the subtree.
 *      %NULL for the root element of the document.
 * @opts: The serialization options.
 * @out: The output stream.
 *
 * Serializes text contents (including contents of all descendants) in
 * the subtree of the specific document to a stream.
 *
 * Returns: 0 for succes, -1 for errors.
 *
 * Since: 0.2.0
 */
PCA_EXPORT int
pcdoc_serialize_text_contents_to_stream(purc_document_t doc,
        pcdoc_element_t ancestor, unsigned opts, purc_rwstream_t out);

/**
 * purc_document_serialize_text_contents_to_stream:
 *
 * @doc: The pointer to a document.
 * @opts: The serialization options.
 * @out: The output stream.
 *
 * Serializes all text contents in a document to a stream.
 *
 * Returns: 0 for succes, -1 for errors.
 *
 * Since: 0.2.0
 */
static inline int
purc_document_serialize_text_contents_to_stream(purc_document_t doc,
        unsigned opts, purc_rwstream_t out)
{
    return pcdoc_serialize_text_contents_to_stream(doc, NULL, opts, out);
}

/**
 * pcdoc_serialize_descendants_to_stream:
 *
 * @doc: The pointer to a document.
 * @ancestor (nullable): The pointer to the ancestor of the subtree.
 *      %NULL for the root element of the document.
 * @opts: The serialization options.
 * @out: The output stream.
 *
 * Serializes all descendant elements (as well as the contents) in the subtree
 * of the specific document to a stream.
 *
 * Returns: 0 for succes, -1 for errors.
 *
 * Since: 0.2.0
 */
PCA_EXPORT int
pcdoc_serialize_descendants_to_stream(purc_document_t doc,
        pcdoc_element_t ancestor, unsigned opts, purc_rwstream_t out);

/**
 * purc_document_serialize_descendants_to_stream:
 *
 * @doc: The pointer to a document.
 * @selector: The CSS selector to to match elements.
 * @opts: The serialization options.
 * @out: The output stream.
 *
 * Serializes all descendant elements (as well as the contents) in a document
 * to a stream.
 *
 * Returns: 0 for succes, -1 for errors.
 *
 * Since: 0.9.24
 *
 */
PCA_EXPORT int
pcdoc_serialize_fragment_to_stream(purc_document_t doc,
        const char *selector, unsigned opts, purc_rwstream_t out);

/**
 * purc_document_serialize_contents_to_stream:
 *
 * @doc: The pointer to a document.
 * @opts: The serialization options.
 * @out: The output stream.
 *
 * Serializes whole document to a stream.
 *
 * Returns: 0 for succes, -1 for errors.
 *
 * Since: 0.2.0
 */
PCA_EXPORT int
purc_document_serialize_contents_to_stream(purc_document_t doc,
        unsigned opts, purc_rwstream_t out);

/**
 * pcdoc_selector_new:
 *
 * @char: the css selector.
 *
 * Creates a new selector.
 *
 * Returns: the pointer to the selector or %NULL on failure.
 *
 */
PCA_EXPORT pcdoc_selector_t
pcdoc_selector_new(const char *selector);

/**
 * pcdoc_selector_delete:
 *
 * @selector: The pointer to a selector.
 *
 * This function deletes a selector.
 *
 * Returns: 0 for success, -1 for failure.
 */
PCA_EXPORT int
pcdoc_selector_delete(pcdoc_selector_t selector);


/**
 * pcdoc_get_element_by_id_in_descendants:
 *
 * @doc: The pointer to a document.
 * @ancestor: The pointer to the ancestor element.
 * @id: The target id.
 *
 * Gets the element matching the id from the descendants.
 *
 * Returns: the pointer to the matching element or %NULL if no such one.
 *
 */
PCA_EXPORT pcdoc_element_t
pcdoc_get_element_by_id_in_descendants(purc_document_t doc,
        pcdoc_element_t ancestor, const char *id);

/**
 * pcdoc_get_element_by_id_in_document:
 *
 * @doc: The pointer to a document.
 * @id: The target id.
 *
 * Gets the element matching the id from the document.
 *
 * Returns: the pointer to the matching element or %NULL if no such one.
 *
 */
static inline pcdoc_element_t
pcdoc_get_element_by_id_in_document(purc_document_t doc, const char *id)
{
    return pcdoc_get_element_by_id_in_descendants(doc, NULL, id);
}


/**
 * pcdoc_find_element_in_descendants:
 *
 * @doc: The pointer to a document.
 * @ancestor: The pointer to the ancestor element.
 * @selector: The pointer to the selector.
 *
 * Finds the first element matching the CSS selector from the descendants.
 *
 * Returns: the pointer to the matching element or %NULL if no such one.
 *
 * Note: Unimplemented.
 */
PCA_EXPORT pcdoc_element_t
pcdoc_find_element_in_descendants(purc_document_t doc,
        pcdoc_element_t ancestor, pcdoc_selector_t selector);

/**
 * pcdoc_find_element_in_document:
 *
 * @doc: The pointer to a document.
 * @selector: The pointer to the selector.
 *
 * Finds the first element matching the CSS selector in the document.
 *
 * Returns: the pointer to the matching element or %NULL if no such one.
 *
 * Note: Unimplemented.
 */
static inline pcdoc_element_t
pcdoc_find_element_in_document(purc_document_t doc, pcdoc_selector_t selector)
{
    return pcdoc_find_element_in_descendants(doc, NULL, selector);
}

/**
 * pcdoc_elem_coll_new_from_descendants:
 *
 * @doc: The pointer to a document.
 * @ancestor: The pointer to the ancestor element.
 * @selector: The pointer to the selector.
 *
 * Creates an element collection by selecting the elements from the descendants
 * of the specified element according to the CSS selector.
 *
 * Returns: A pointer to the element collection; %NULL on failure.
 *
 * Note: Unimplemented.
 */
PCA_EXPORT pcdoc_elem_coll_t
pcdoc_elem_coll_new_from_descendants(purc_document_t doc,
        pcdoc_element_t ancestor, pcdoc_selector_t selector);

/**
 * pcdoc_elem_coll_new_from_document:
 *
 * @doc: The pointer to a document.
 * @selector: The pointer to the selector.
 *
 * Creates an element collection by selecting the elements from
 * the whole document according to the CSS selector.
 *
 * Returns: A pointer to the element collection; %NULL on failure.
 *
 * Note: Unimplemented.
 */
static inline pcdoc_elem_coll_t
pcdoc_elem_coll_new_from_document(purc_document_t doc,
        pcdoc_selector_t selector)
{
    return pcdoc_elem_coll_new_from_descendants(doc, NULL, selector);
}

/**
 * pcdoc_elem_coll_select:
 *
 * @doc: The pointer to a document.
 * @elem_coll: The pointer to the element collection.
 * @selector: The pointer to the selector.
 *
 * Creates a new element collection by selecting a part of elements
 * in the specific element collection.
 *
 * Returns: A pointer to the new element collection; %NULL on failure.
 *
 * Note: Unimplemented.
 */
PCA_EXPORT pcdoc_elem_coll_t
pcdoc_elem_coll_select(purc_document_t doc,
        pcdoc_elem_coll_t elem_coll, pcdoc_selector_t selector);

/**
 * pcdoc_elem_coll_delete:
 *
 * @doc: The pointer to a document.
 * @elem_coll: The pointer to the element collection.
 *
 * Deletes the specified element collection.
 *
 * Note: Unimplemented.
 */
PCA_EXPORT void
pcdoc_elem_coll_delete(purc_document_t doc,
        pcdoc_elem_coll_t elem_coll);

/**
 * pcdoc_elem_coll_count:
 *
 * @doc: The pointer to a document.
 * @elem_coll: The pointer to the element collection.
 *
 * Gets the element count of element collection.
 *
 * Returns: The element count of the element collection.
 *
 */
PCA_EXPORT ssize_t
pcdoc_elem_coll_count(purc_document_t doc,
        pcdoc_elem_coll_t elem_coll);

/**
 * pcdoc_elem_coll_get:
 *
 * Gets the member from the element collection by index (@idx).
 *
 * @doc: The pointer to a document.
 * @elem_coll: The pointer to the element collection.
 * @idx: The index of the member.
 *
 * Returns: The element pointer on success, or %NULL on failure.
 *
 */
PCA_EXPORT pcdoc_element_t
pcdoc_elem_coll_get(purc_document_t doc,
    pcdoc_elem_coll_t elem_coll, size_t idx);


/**
 * pcdoc_elem_coll_sub:
 *
 * @doc: The pointer to a document.
 * @elem_coll: The pointer to the element collection.
 * @offset: The offset of the index.
 * @length: The size to be sub.
 *
 * Creates an element collection by sub the elements from
 * the element collection.
 *
 * Returns: A pointer to the element collection; %NULL on failure.
 *
 */
PCA_EXPORT pcdoc_elem_coll_t
pcdoc_elem_coll_sub(purc_document_t doc,
        pcdoc_elem_coll_t elem_coll, int offset, size_t length);

/**
 * pcdoc_elem_coll_travel:
 *
 * @doc: The pointer to the document.
 * @elem_coll : The pointer to the element collection.
 * @cb: The callback for the element travelled.
 * @ctxt: The pointer to the context data which will be passed to the callback.
 * @n: The pointer to a size_t buffer to return the number of elements
 *  travelled, i.e., the number of times the callback function was called.
 *
 * Travels all descendant elements in the element collection.
 *
 * Returns: 0 for all descendants travelled, otherwise the traverse was broken
 * by the callback.
 *
 */
PCA_EXPORT int
pcdoc_elem_coll_travel(purc_document_t doc, pcdoc_elem_coll_t elem_coll,
        pcdoc_element_cb cb, void *ctxt, size_t *n);


PCA_EXTERN_C_END

#endif  /* PURC_PURC_DOCUMENT_H */

