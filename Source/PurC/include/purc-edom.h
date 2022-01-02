/**
 * @file purc-edom.h
 * @author Vincent Wei
 * @date 2022/01/02
 * @brief The API of eDOM.
 *
 * Copyright (C) 2021, 2022 FMSoft <https://www.fmsoft.cn>
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

#ifndef PURC_PURC_EDOM_H
#define PURC_PURC_EDOM_H

#include "purc-macros.h"
#include "purc-rwstream.h"
#include "purc-utils.h"
#include "purc-errors.h"

#define PURC_ERROR_EDOM PURC_ERROR_FIRST_EDOM

typedef uintptr_t pchtml_ns_id_t;
typedef uintptr_t pchtml_tag_id_t;
typedef uintptr_t pcedom_attr_id_t;

struct pcedom_node;
typedef struct pcedom_node pcedom_node_t;
struct pcedom_document;
typedef struct pcedom_document pcedom_document_t;
struct pcedom_document_fragment;
typedef struct pcedom_document_fragment pcedom_document_fragment_t;
struct pcedom_attr;
typedef struct pcedom_attr pcedom_attr_t;
struct pcedom_document_type;
typedef struct pcedom_document_type pcedom_document_type_t;
struct pcedom_element;
typedef struct pcedom_element pcedom_element_t;
struct pcedom_character_data;
typedef struct pcedom_character_data pcedom_character_data_t;
struct pcedom_processing_instruction;
typedef struct pcedom_processing_instruction pcedom_processing_instruction_t;
struct pcedom_shadow_root;
typedef struct pcedom_shadow_root pcedom_shadow_root_t;
struct pcedom_event_target;
typedef struct pcedom_event_target pcedom_event_target_t;
struct pcedom_text;
typedef struct pcedom_text pcedom_text_t;
struct pcedom_cdata_section;
typedef struct pcedom_cdata_section pcedom_cdata_section_t;
struct pcedom_comment;
typedef struct pcedom_comment pcedom_comment_t;

#define pcedom_interface_cdata_section(obj) ((pcedom_cdata_section_t *) (obj))
#define pcedom_interface_character_data(obj) ((pcedom_character_data_t *) (obj))
#define pcedom_interface_comment(obj) ((pcedom_comment_t *) (obj))
#define pcedom_interface_document(obj) ((pcedom_document_t *) (obj))
#define pcedom_interface_document_fragment(obj) ((pcedom_document_fragment_t *) (obj))
#define pcedom_interface_document_type(obj) ((pcedom_document_type_t *) (obj))
#define pcedom_interface_element(obj) ((pcedom_element_t *) (obj))
#define pcedom_interface_attr(obj) ((pcedom_attr_t *) (obj))
#define pcedom_interface_event_target(obj) ((pcedom_event_target_t *) (obj))
#define pcedom_interface_node(obj) ((pcedom_node_t *) (obj))
#define pcedom_interface_processing_instruction(obj) ((pcedom_processing_instruction_t *) (obj))
#define pcedom_interface_shadow_root(obj) ((pcedom_shadow_root_t *) (obj))
#define pcedom_interface_text(obj) ((pcedom_text_t *) (obj))

PCA_EXTERN_C_BEGIN

// ============================= for interface ================================
typedef void pcedom_interface_t;

typedef void *
(*pcedom_interface_constructor_f)(void *document);

typedef void *
(*pcedom_interface_destructor_f)(void *intrfc);


typedef pcedom_interface_t *
(*pcedom_interface_create_f)(pcedom_document_t *document, pchtml_tag_id_t tag_id,
                              pchtml_ns_id_t ns);

typedef pcedom_interface_t *
(*pcedom_interface_destroy_f)(pcedom_interface_t *intrfc);


pcedom_interface_t *
pcedom_interface_create(pcedom_document_t *document, pchtml_tag_id_t tag_id,
                         pchtml_ns_id_t ns);

pcedom_interface_t *
pcedom_interface_destroy(pcedom_interface_t *intrfc);


// ============================= for event target =============================
struct pcedom_event_target {
    void *events;
};

pcedom_event_target_t *
pcedom_event_target_create(pcedom_document_t *document);

pcedom_event_target_t *
pcedom_event_target_destroy(pcedom_event_target_t *event_target,
                pcedom_document_t *document);


// ============================= for node =====================================
typedef enum {
    PCEDOM_NODE_TYPE_UNDEF                  = 0x00,
    PCEDOM_NODE_TYPE_ELEMENT                = 0x01,
    PCEDOM_NODE_TYPE_ATTRIBUTE              = 0x02,
    PCEDOM_NODE_TYPE_TEXT                   = 0x03,
    PCEDOM_NODE_TYPE_CDATA_SECTION          = 0x04,
    PCEDOM_NODE_TYPE_ENTITY_REFERENCE       = 0x05, // historical
    PCEDOM_NODE_TYPE_ENTITY                 = 0x06, // historical
    PCEDOM_NODE_TYPE_PROCESSING_INSTRUCTION = 0x07,
    PCEDOM_NODE_TYPE_COMMENT                = 0x08,
    PCEDOM_NODE_TYPE_DOCUMENT               = 0x09,
    PCEDOM_NODE_TYPE_DOCUMENT_TYPE          = 0x0A,
    PCEDOM_NODE_TYPE_DOCUMENT_FRAGMENT      = 0x0B,
    PCEDOM_NODE_TYPE_NOTATION               = 0x0C, // historical
    PCEDOM_NODE_TYPE_LAST_ENTRY             = 0x0D
} pcedom_node_type_t;

struct pcedom_node {
    pcedom_event_target_t event_target;

    /* For example: <LalAla:DiV Fix:Me="value"> */
    uintptr_t              local_name; /* , lowercase, without prefix: div */
    uintptr_t              prefix;     /* lowercase: lalala */
    uintptr_t              ns;         /* namespace */

    pcedom_document_t     *owner_document;

    pcedom_node_t         *next;
    pcedom_node_t         *prev;
    pcedom_node_t         *parent;
    pcedom_node_t         *first_child;
    pcedom_node_t         *last_child;
    void                  *user;

    pcedom_node_type_t    type;

#ifdef PCEDOM_NODE_USER_VARIABLES
    PCEDOM_NODE_USER_VARIABLES
#endif /* PCEDOM_NODE_USER_VARIABLES */
};

typedef enum {
    PCHTML_ACTION_OK    = 0x00,
    PCHTML_ACTION_STOP  = 0x01,
    PCHTML_ACTION_NEXT  = 0x02
} pchtml_action_t;

typedef pchtml_action_t
(*pcedom_node_simple_walker_f)(pcedom_node_t *node, void *ctx);

pcedom_node_t *
pcedom_node_interface_create(pcedom_document_t *document);

pcedom_node_t *
pcedom_node_interface_destroy(pcedom_node_t *node);

pcedom_node_t *
pcedom_node_destroy(pcedom_node_t *node);

pcedom_node_t *
pcedom_node_destroy_deep(pcedom_node_t *root);

const unsigned char *
pcedom_node_name(pcedom_node_t *node,
                size_t *len);

void
pcedom_node_insert_child(pcedom_node_t *to,
                pcedom_node_t *node);

void
pcedom_node_insert_before(pcedom_node_t *to,
                pcedom_node_t *node);

void
pcedom_node_insert_after(pcedom_node_t *to,
                pcedom_node_t *node);

void
pcedom_node_remove(pcedom_node_t *node);

unsigned int
pcedom_node_replace_all(pcedom_node_t *parent,
                pcedom_node_t *node);

void
pcedom_node_simple_walk(pcedom_node_t *root,
                pcedom_node_simple_walker_f walker_cb,
                void *ctx);

void
pcedom_merge_fragment_prepend(pcedom_node_t *parent,
        pcedom_node_t *fragment);

void
pcedom_merge_fragment_append(pcedom_node_t *parent,
        pcedom_node_t *fragment);

void
pcedom_merge_fragment_insert_before(pcedom_node_t *to,
        pcedom_node_t *fragment);

void
pcedom_merge_fragment_insert_after(pcedom_node_t *to,
        pcedom_node_t *fragment);

/*
 * Memory of returns value will be freed in document destroy moment.
 * If you need to release returned resource after use, then call the
 * pcedom_document_destroy_text(node->owner_document, text) function.
 */
unsigned char *
pcedom_node_text_content(pcedom_node_t *node, size_t *len);

unsigned int
pcedom_node_text_content_set(pcedom_node_t *node,
                const unsigned char *content, size_t len);

/*
 * Inline functions
 */
static inline pchtml_tag_id_t
pcedom_node_tag_id(pcedom_node_t *node)
{
    return node->local_name;
}

static inline pcedom_node_t *
pcedom_node_next(pcedom_node_t *node)
{
    return node->next;
}

static inline pcedom_node_t *
pcedom_node_prev(pcedom_node_t *node)
{
    return node->prev;
}

static inline pcedom_node_t *
pcedom_node_parent(pcedom_node_t *node)
{
    return node->parent;
}

static inline pcedom_node_t *
pcedom_node_first_child(pcedom_node_t *node)
{
    return node->first_child;
}

static inline pcedom_node_t *
pcedom_node_last_child(pcedom_node_t *node)
{
    return node->last_child;
}

// ============================= for character data ===========================
struct pcedom_character_data {
    pcedom_node_t  node;

    pcutils_str_t   data;
};


pcedom_character_data_t *
pcedom_character_data_interface_create(
                pcedom_document_t *document);

pcedom_character_data_t *
pcedom_character_data_interface_destroy(
                pcedom_character_data_t *character_data);

unsigned int
pcedom_character_data_replace(pcedom_character_data_t *ch_data,
                const unsigned char *data, size_t len,
                size_t offset, size_t count);


// ============================= for text node =================================
struct pcedom_text {
    pcedom_character_data_t char_data;
};

pcedom_text_t *
pcedom_text_interface_create(pcedom_document_t *document);

pcedom_text_t *
pcedom_text_interface_destroy(pcedom_text_t *text);


// ============================= for cdata_section ============================
struct pcedom_cdata_section {
    pcedom_text_t text;
};

pcedom_cdata_section_t *
pcedom_cdata_section_interface_create(
                pcedom_document_t *document);

pcedom_cdata_section_t *
pcedom_cdata_section_interface_destroy(
                pcedom_cdata_section_t *cdata_section);


// ============================= for comment ==================================
struct pcedom_comment {
    pcedom_character_data_t char_data;
};

pcedom_comment_t *
pcedom_comment_interface_create(
                pcedom_document_t *document);

pcedom_comment_t *
pcedom_comment_interface_destroy(
                pcedom_comment_t *comment);



// ============================= for document =================================
typedef enum {
    PCEDOM_DOCUMENT_CMODE_NO_QUIRKS       = 0x00,
    PCEDOM_DOCUMENT_CMODE_QUIRKS          = 0x01,
    PCEDOM_DOCUMENT_CMODE_LIMITED_QUIRKS  = 0x02
}
pcedom_document_cmode_t;

typedef enum {
    PCEDOM_DOCUMENT_DTYPE_UNDEF = 0x00,
    PCEDOM_DOCUMENT_DTYPE_HTML  = 0x01,
    PCEDOM_DOCUMENT_DTYPE_XML   = 0x02
}
pcedom_document_dtype_t;

struct pcedom_document {
    pcedom_node_t              node;

    pcedom_document_cmode_t    compat_mode;
    pcedom_document_dtype_t    type;

    pcedom_document_type_t     *doctype;
    pcedom_element_t           *element;

    pcedom_interface_create_f  create_interface;
    pcedom_interface_destroy_f destroy_interface;

    pcutils_mraw_t             *mraw;
    pcutils_mraw_t             *text;
    pcutils_hash_t             *tags;
    pcutils_hash_t             *attrs;
    pcutils_hash_t             *prefix;
    pcutils_hash_t             *ns;
    void                       *parser;
    void                       *user;

    bool                       tags_inherited;
    bool                       ns_inherited;

    bool                       scripting;
};

pcedom_document_t *
pcedom_document_interface_create(
            pcedom_document_t *document);

pcedom_document_t *
pcedom_document_interface_destroy(
            pcedom_document_t *document);

pcedom_document_t *
pcedom_document_create(pcedom_document_t *owner);

unsigned int
pcedom_document_init(pcedom_document_t *document, pcedom_document_t *owner,
            pcedom_interface_create_f create_interface,
            pcedom_interface_destroy_f destroy_interface,
            pcedom_document_dtype_t type, unsigned int ns);

unsigned int
pcedom_document_clean(pcedom_document_t *document);

pcedom_document_t *
pcedom_document_destroy(pcedom_document_t *document);

void
pcedom_document_attach_doctype(pcedom_document_t *document,
            pcedom_document_type_t *doctype);

void
pcedom_document_attach_element(pcedom_document_t *document,
            pcedom_element_t *element);

pcedom_element_t *
pcedom_document_create_element(pcedom_document_t *document,
            const unsigned char *local_name, size_t lname_len,
            void *reserved_for_opt);

pcedom_element_t *
pcedom_document_destroy_element(
            pcedom_element_t *element);

pcedom_document_fragment_t *
pcedom_document_create_document_fragment(
            pcedom_document_t *document);

pcedom_text_t *
pcedom_document_create_text_node(pcedom_document_t *document,
            const unsigned char *data, size_t len);

pcedom_cdata_section_t *
pcedom_document_create_cdata_section(pcedom_document_t *document,
            const unsigned char *data, size_t len);

pcedom_processing_instruction_t *
pcedom_document_create_processing_instruction(pcedom_document_t *document,
            const unsigned char *target, size_t target_len,
            const unsigned char *data, size_t data_len);

pcedom_comment_t *
pcedom_document_create_comment(pcedom_document_t *document,
            const unsigned char *data, size_t len);


/*
 * Inline functions
 */
static inline pcedom_interface_t *
pcedom_document_create_interface(pcedom_document_t *document,
                                  pchtml_tag_id_t tag_id, pchtml_ns_id_t ns)
{
    return document->create_interface(document, tag_id, ns);
}

static inline pcedom_interface_t *
pcedom_document_destroy_interface(pcedom_interface_t *intrfc)
{
    return pcedom_interface_node(intrfc)->owner_document->destroy_interface(intrfc);
}

static inline void *
pcedom_document_create_struct(pcedom_document_t *document, size_t struct_size)
{
    return pcutils_mraw_calloc(document->mraw, struct_size);
}

static inline void *
pcedom_document_destroy_struct(pcedom_document_t *document, void *structure)
{
    return pcutils_mraw_free(document->mraw, structure);
}

static inline unsigned char *
pcedom_document_create_text(pcedom_document_t *document, size_t len)
{
    return (unsigned char *) pcutils_mraw_alloc(document->text,
                                            sizeof(unsigned char) * len);
}

static inline void *
pcedom_document_destroy_text(pcedom_document_t *document, unsigned char *text)
{
    return pcutils_mraw_free(document->text, text);
}

static inline pcedom_element_t *
pcedom_document_element(pcedom_document_t *document)
{
    return document->element;
}


// ============================= for document fragment ========================
struct pcedom_document_fragment {
    pcedom_node_t    node;

    pcedom_element_t *host;
};

pcedom_document_fragment_t *
pcedom_document_fragment_interface_create(
            pcedom_document_t *document);

pcedom_document_fragment_t *
pcedom_document_fragment_interface_destroy(
            pcedom_document_fragment_t *document_fragment);


// ============================= for attribute ================================
typedef struct {
    pcutils_hash_entry_t entry;
    pcedom_attr_id_t     attr_id;
    size_t               ref_count;
    bool                 read_only;
} pcedom_attr_data_t;

/* More memory to God of memory! */
struct pcedom_attr {
    pcedom_node_t     node;

    /* For example: <LalAla:DiV Fix:Me="value"> */

    pcedom_attr_id_t  upper_name;     /* uppercase, with prefix: FIX:ME */
    pcedom_attr_id_t  qualified_name; /* original, with prefix: Fix:Me */

    pcutils_str_t     *value;
    pcedom_element_t  *owner;

    pcedom_attr_t     *next;
    pcedom_attr_t     *prev;
};

pcedom_attr_t *
pcedom_attr_interface_create(pcedom_document_t *document);

pcedom_attr_t *
pcedom_attr_interface_destroy(pcedom_attr_t *attr);

unsigned int
pcedom_attr_set_name(pcedom_attr_t *attr, const unsigned char *local_name,
                      size_t local_name_len, bool to_lowercase);

unsigned int
pcedom_attr_set_value(pcedom_attr_t *attr,
                const unsigned char *value, size_t value_len);

unsigned int
pcedom_attr_set_value_wo_copy(pcedom_attr_t *attr,
                unsigned char *value, size_t value_len);

unsigned int
pcedom_attr_set_existing_value(pcedom_attr_t *attr,
                const unsigned char *value, size_t value_len);

unsigned int
pcedom_attr_clone_name_value(pcedom_attr_t *attr_from,
                pcedom_attr_t *attr_to);

bool
pcedom_attr_compare(pcedom_attr_t *first,
                pcedom_attr_t *second);

const pcedom_attr_data_t *
pcedom_attr_data_by_id(pcutils_hash_t *hash,
                pcedom_attr_id_t attr_id);

const pcedom_attr_data_t *
pcedom_attr_data_by_local_name(pcutils_hash_t *hash,
                const unsigned char *name, size_t length);

const pcedom_attr_data_t *
pcedom_attr_data_by_qualified_name(pcutils_hash_t *hash,
                                    const unsigned char *name, size_t length);

const unsigned char *
pcedom_attr_qualified_name(pcedom_attr_t *attr, size_t *len);

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

    return pcutils_hash_entry_str(&data->entry);
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


// ============================= for document type ============================
struct pcedom_document_type {
    pcedom_node_t    node;

    pcedom_attr_id_t name;
    pcutils_str_t      public_id;
    pcutils_str_t      system_id;
};

pcedom_document_type_t *
pcedom_document_type_interface_create(
                pcedom_document_t *document);

pcedom_document_type_t *
pcedom_document_type_interface_destroy(
                pcedom_document_type_t *document_type);


const unsigned char *
pcedom_document_type_name(pcedom_document_type_t *doc_type, size_t *len);

static inline const unsigned char *
pcedom_document_type_public_id(pcedom_document_type_t *doc_type, size_t *len)
{
    if (len != NULL) {
        *len = doc_type->public_id.length;
    }

    return doc_type->public_id.data;
}

static inline const unsigned char *
pcedom_document_type_system_id(pcedom_document_type_t *doc_type, size_t *len)
{
    if (len != NULL) {
        *len = doc_type->system_id.length;
    }

    return doc_type->system_id.data;
}


// ============================= for collection ===============================
typedef struct {
    pcutils_array_t     array;
    pcedom_document_t *document;
} pcedom_collection_t;

pcedom_collection_t *
pcedom_collection_create(pcedom_document_t *document);

unsigned int
pcedom_collection_init(pcedom_collection_t *col,
                size_t start_list_size);

pcedom_collection_t *
pcedom_collection_destroy(pcedom_collection_t *col,
                bool self_destroy);


/*
 * Inline functions
 */
static inline pcedom_collection_t *
pcedom_collection_make(pcedom_document_t *document, size_t start_list_size)
{
    unsigned int status;
    pcedom_collection_t *col;

    col = pcedom_collection_create(document);
    status = pcedom_collection_init(col, start_list_size);

    if(status != PURC_ERROR_OK) {
        return pcedom_collection_destroy(col, true);
    }

    return col;
}

static inline void
pcedom_collection_clean(pcedom_collection_t *col)
{
    pcutils_array_clean(&col->array);
}

static inline unsigned int
pcedom_collection_append(pcedom_collection_t *col, void *value)
{
    return pcutils_array_push(&col->array, value);
}

static inline pcedom_element_t *
pcedom_collection_element(pcedom_collection_t *col, size_t idx)
{
    return (pcedom_element_t *) pcutils_array_get(&col->array, idx);
}

static inline pcedom_node_t *
pcedom_collection_node(pcedom_collection_t *col, size_t idx)
{
    return (pcedom_node_t *) pcutils_array_get(&col->array, idx);
}

static inline size_t
pcedom_collection_length(pcedom_collection_t *col)
{
    return pcutils_array_length(&col->array);
}



// ============================= for element ==================================
typedef enum {
    PCEDOM_ELEMENT_CUSTOM_STATE_UNDEFINED      = 0x00,
    PCEDOM_ELEMENT_CUSTOM_STATE_FAILED         = 0x01,
    PCEDOM_ELEMENT_CUSTOM_STATE_UNCUSTOMIZED   = 0x02,
    PCEDOM_ELEMENT_CUSTOM_STATE_CUSTOM         = 0x03
} pcedom_element_custom_state_t;

struct pcedom_element {
    pcedom_node_t                 node;

    /* For example: <LalAla:DiV Fix:Me="value"> */

    /* uppercase, with prefix: LALALA:DIV */
    pcedom_attr_id_t              upper_name;

    /* original, with prefix: LalAla:DiV */
    pcedom_attr_id_t              qualified_name;

    pcutils_str_t                 *is_value;

    pcedom_attr_t                 *first_attr;
    pcedom_attr_t                 *last_attr;

    pcedom_attr_t                 *attr_id;
    pcedom_attr_t                 *attr_class;

    pcedom_element_custom_state_t custom_state;
};

pcedom_element_t *
pcedom_element_interface_create(
                pcedom_document_t *document);

pcedom_element_t *
pcedom_element_interface_destroy(
                pcedom_element_t *element);

pcedom_element_t *
pcedom_element_create(pcedom_document_t *document,
                const unsigned char *local_name, size_t lname_len,
                const unsigned char *ns_name, size_t ns_len,
                const unsigned char *prefix, size_t prefix_len,
                const unsigned char *is, size_t is_len,
                bool sync_custom);

pcedom_element_t *
pcedom_element_destroy(pcedom_element_t *element);

bool
pcedom_element_has_attributes(pcedom_element_t *element);

pcedom_attr_t *
pcedom_element_set_attribute(pcedom_element_t *element,
                const unsigned char *qualified_name, size_t qn_len,
                const unsigned char *value, size_t value_len);

const unsigned char *
pcedom_element_get_attribute(pcedom_element_t *element,
                const unsigned char *qualified_name, size_t qn_len,
                size_t *value_len);

unsigned int
pcedom_element_remove_attribute(pcedom_element_t *element,
                const unsigned char *qualified_name, size_t qn_len);

bool
pcedom_element_has_attribute(pcedom_element_t *element,
                const unsigned char *qualified_name, size_t qn_len);

unsigned int
pcedom_element_attr_append(pcedom_element_t *element,
                pcedom_attr_t *attr);

unsigned int
pcedom_element_attr_remove(pcedom_element_t *element,
                pcedom_attr_t *attr);

pcedom_attr_t *
pcedom_element_attr_by_name(pcedom_element_t *element,
                const unsigned char *qualified_name, size_t length);

pcedom_attr_t *
pcedom_element_attr_by_local_name_data(pcedom_element_t *element,
                const pcedom_attr_data_t *data);

pcedom_attr_t *
pcedom_element_attr_by_id(pcedom_element_t *element,
                pcedom_attr_id_t attr_id);

pcedom_attr_t *
pcedom_element_attr_by_data(pcedom_element_t *element,
                const pcedom_attr_data_t *data);

bool
pcedom_element_compare(pcedom_element_t *first,
                pcedom_element_t *second);

pcedom_attr_t *
pcedom_element_attr_is_exist(pcedom_element_t *element,
                const unsigned char *qualified_name, size_t length);

unsigned int
pcedom_element_is_set(pcedom_element_t *element,
                const unsigned char *is, size_t is_len);

unsigned int
pcedom_elements_by_tag_name(pcedom_element_t *root,
                pcedom_collection_t *collection,
                const unsigned char *qualified_name, size_t len);

unsigned int
pcedom_elements_by_class_name(pcedom_element_t *root,
                pcedom_collection_t *collection,
                const unsigned char *class_name, size_t len);

unsigned int
pcedom_elements_by_attr(pcedom_element_t *root,
                pcedom_collection_t *collection,
                const unsigned char *qualified_name, size_t qname_len,
                const unsigned char *value, size_t value_len,
                bool case_insensitive);

unsigned int
pcedom_elements_by_attr_begin(pcedom_element_t *root,
                pcedom_collection_t *collection,
                const unsigned char *qualified_name, size_t qname_len,
                const unsigned char *value, size_t value_len,
                bool case_insensitive);

unsigned int
pcedom_elements_by_attr_end(pcedom_element_t *root,
                pcedom_collection_t *collection,
                const unsigned char *qualified_name, size_t qname_len,
                const unsigned char *value, size_t value_len,
                bool case_insensitive);

unsigned int
pcedom_elements_by_attr_contain(pcedom_element_t *root,
                pcedom_collection_t *collection,
                const unsigned char *qualified_name, size_t qname_len,
                const unsigned char *value, size_t value_len,
                bool case_insensitive);

const unsigned char *
pcedom_element_qualified_name(pcedom_element_t *element,
                size_t *len);

const unsigned char *
pcedom_element_qualified_name_upper(pcedom_element_t *element,
                size_t *len);

const unsigned char *
pcedom_element_local_name(pcedom_element_t *element,
                size_t *len);

const unsigned char *
pcedom_element_prefix(pcedom_element_t *element,
                size_t *len);

const unsigned char *
pcedom_element_tag_name(pcedom_element_t *element,
                size_t *len);


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


// ============================= for processing instruction ===================
struct pcedom_processing_instruction {
    pcedom_character_data_t char_data;

    pcutils_str_t             target;
};


pcedom_processing_instruction_t *
pcedom_processing_instruction_interface_create(
        pcedom_document_t *document);

pcedom_processing_instruction_t *
pcedom_processing_instruction_interface_destroy(
        pcedom_processing_instruction_t *processing_instruction);


/*
 * Inline functions
 */
static inline const unsigned char *
pcedom_processing_instruction_target(pcedom_processing_instruction_t *pi,
                                      size_t *len)
{
    if (len != NULL) {
        *len = pi->target.length;
    }

    return pi->target.data;
}


PCA_EXTERN_C_END

#endif  /* PURC_PURC_EDOM_H */

