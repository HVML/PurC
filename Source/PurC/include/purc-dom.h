/**
 * @file purc-dom.h
 * @author Vincent Wei
 * @date 2022/01/02
 * @brief The API of DOM.
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

#ifndef PURC_PURC_DOM_H
#define PURC_PURC_DOM_H

#include "purc-macros.h"
#include "purc-rwstream.h"
#include "purc-utils.h"
#include "purc-errors.h"

#define PURC_ERROR_DOM PURC_ERROR_FIRST_DOM

typedef uintptr_t pchtml_ns_id_t;
typedef uintptr_t pchtml_tag_id_t;
typedef uintptr_t pcdom_attr_id_t;

struct pcdom_node;
typedef struct pcdom_node pcdom_node_t;
struct pcdom_document;
typedef struct pcdom_document pcdom_document_t;
struct pcdom_document_fragment;
typedef struct pcdom_document_fragment pcdom_document_fragment_t;
struct pcdom_attr;
typedef struct pcdom_attr pcdom_attr_t;
struct pcdom_document_type;
typedef struct pcdom_document_type pcdom_document_type_t;
struct pcdom_element;
typedef struct pcdom_element pcdom_element_t;
struct pcdom_character_data;
typedef struct pcdom_character_data pcdom_character_data_t;
struct pcdom_processing_instruction;
typedef struct pcdom_processing_instruction pcdom_processing_instruction_t;
struct pcdom_shadow_root;
typedef struct pcdom_shadow_root pcdom_shadow_root_t;
struct pcdom_event_target;
typedef struct pcdom_event_target pcdom_event_target_t;
struct pcdom_text;
typedef struct pcdom_text pcdom_text_t;
struct pcdom_cdata_section;
typedef struct pcdom_cdata_section pcdom_cdata_section_t;
struct pcdom_comment;
typedef struct pcdom_comment pcdom_comment_t;

#define pcdom_interface_cdata_section(obj) ((pcdom_cdata_section_t *) (obj))
#define pcdom_interface_character_data(obj) ((pcdom_character_data_t *) (obj))
#define pcdom_interface_comment(obj) ((pcdom_comment_t *) (obj))
#define pcdom_interface_document(obj) ((pcdom_document_t *) (obj))
#define pcdom_interface_document_fragment(obj) ((pcdom_document_fragment_t *) (obj))
#define pcdom_interface_document_type(obj) ((pcdom_document_type_t *) (obj))
#define pcdom_interface_element(obj) ((pcdom_element_t *) (obj))
#define pcdom_interface_attr(obj) ((pcdom_attr_t *) (obj))
#define pcdom_interface_event_target(obj) ((pcdom_event_target_t *) (obj))
#define pcdom_interface_node(obj) ((pcdom_node_t *) (obj))
#define pcdom_interface_processing_instruction(obj) ((pcdom_processing_instruction_t *) (obj))
#define pcdom_interface_shadow_root(obj) ((pcdom_shadow_root_t *) (obj))
#define pcdom_interface_text(obj) ((pcdom_text_t *) (obj))

PCA_EXTERN_C_BEGIN

// ============================= for interface ================================
typedef void pcdom_interface_t;

typedef void *
(*pcdom_interface_constructor_f)(void *document);

typedef void *
(*pcdom_interface_destructor_f)(void *intrfc);


typedef pcdom_interface_t *
(*pcdom_interface_create_f)(pcdom_document_t *document, pchtml_tag_id_t tag_id,
                              pchtml_ns_id_t ns);

typedef pcdom_interface_t *
(*pcdom_interface_destroy_f)(pcdom_interface_t *intrfc);


pcdom_interface_t *
pcdom_interface_create(pcdom_document_t *document, pchtml_tag_id_t tag_id,
                         pchtml_ns_id_t ns);

pcdom_interface_t *
pcdom_interface_destroy(pcdom_interface_t *intrfc);


// ============================= for event target =============================
struct pcdom_event_target {
    void *events;
};

pcdom_event_target_t *
pcdom_event_target_create(pcdom_document_t *document);

pcdom_event_target_t *
pcdom_event_target_destroy(pcdom_event_target_t *event_target,
                pcdom_document_t *document);


// ============================= for node =====================================
typedef enum {
    PCDOM_NODE_TYPE_UNDEF                  = 0x00,
    PCDOM_NODE_TYPE_ELEMENT                = 0x01,
    PCDOM_NODE_TYPE_ATTRIBUTE              = 0x02,
    PCDOM_NODE_TYPE_TEXT                   = 0x03,
    PCDOM_NODE_TYPE_CDATA_SECTION          = 0x04,
    PCDOM_NODE_TYPE_ENTITY_REFERENCE       = 0x05, // historical
    PCDOM_NODE_TYPE_ENTITY                 = 0x06, // historical
    PCDOM_NODE_TYPE_PROCESSING_INSTRUCTION = 0x07,
    PCDOM_NODE_TYPE_COMMENT                = 0x08,
    PCDOM_NODE_TYPE_DOCUMENT               = 0x09,
    PCDOM_NODE_TYPE_DOCUMENT_TYPE          = 0x0A,
    PCDOM_NODE_TYPE_DOCUMENT_FRAGMENT      = 0x0B,
    PCDOM_NODE_TYPE_NOTATION               = 0x0C, // historical
    PCDOM_NODE_TYPE_LAST_ENTRY             = 0x0D
} pcdom_node_type_t;

struct pcdom_node {
    pcdom_event_target_t event_target;

    /* For example: <LalAla:DiV Fix:Me="value"> */
    uintptr_t           local_name; /* , lowercase, without prefix: div */
    uintptr_t           prefix;     /* lowercase: lalala */
    uintptr_t           ns;         /* namespace */

    pcdom_document_t    *owner_document;

    pcdom_node_t        *next;
    pcdom_node_t        *prev;
    pcdom_node_t        *parent;
    pcdom_node_t        *first_child;
    pcdom_node_t        *last_child;

    pcdom_node_type_t   type;
    unsigned int        flags;      /* user-defined flags */
    void                *user;
};

typedef enum {
    PCHTML_ACTION_OK    = 0x00,
    PCHTML_ACTION_STOP  = 0x01,
    PCHTML_ACTION_NEXT  = 0x02
} pchtml_action_t;

typedef pchtml_action_t
(*pcdom_node_simple_walker_f)(pcdom_node_t *node, void *ctx);

pcdom_node_t *
pcdom_node_interface_create(pcdom_document_t *document);

pcdom_node_t *
pcdom_node_interface_destroy(pcdom_node_t *node);

pcdom_node_t *
pcdom_node_destroy(pcdom_node_t *node);

pcdom_node_t *
pcdom_node_destroy_deep(pcdom_node_t *root);

const unsigned char *
pcdom_node_name(pcdom_node_t *node,
                size_t *len);

void
pcdom_node_append_child(pcdom_node_t *to,
                pcdom_node_t *node);

void
pcdom_node_prepend_child(pcdom_node_t *to,
                pcdom_node_t *node);

void
pcdom_node_insert_before(pcdom_node_t *to,
                pcdom_node_t *node);

void
pcdom_node_insert_after(pcdom_node_t *to,
                pcdom_node_t *node);

void
pcdom_node_remove(pcdom_node_t *node);

unsigned int
pcdom_node_replace_all(pcdom_node_t *parent,
                pcdom_node_t *node);

void
pcdom_node_simple_walk(pcdom_node_t *root,
                pcdom_node_simple_walker_f walker_cb,
                void *ctx);

void
pcdom_displace_fragment(pcdom_node_t *parent,
        pcdom_node_t *fragment);

void
pcdom_merge_fragment_prepend(pcdom_node_t *parent,
        pcdom_node_t *fragment);

void
pcdom_merge_fragment_append(pcdom_node_t *parent,
        pcdom_node_t *fragment);

void
pcdom_merge_fragment_insert_before(pcdom_node_t *to,
        pcdom_node_t *fragment);

void
pcdom_merge_fragment_insert_after(pcdom_node_t *to,
        pcdom_node_t *fragment);

/*
 * Memory of returns value will be freed in document destroy moment.
 * If you need to release returned resource after use, then call the
 * pcdom_document_destroy_text(node->owner_document, text) function.
 */
unsigned char *
pcdom_node_text_content(pcdom_node_t *node, size_t *len);

unsigned int
pcdom_node_text_content_set(pcdom_node_t *node,
                const unsigned char *content, size_t len);

/*
 * Inline functions
 */
static inline pchtml_tag_id_t
pcdom_node_tag_id(pcdom_node_t *node)
{
    return node->local_name;
}

static inline pcdom_node_t *
pcdom_node_next(pcdom_node_t *node)
{
    return node->next;
}

static inline pcdom_node_t *
pcdom_node_prev(pcdom_node_t *node)
{
    return node->prev;
}

static inline pcdom_node_t *
pcdom_node_parent(pcdom_node_t *node)
{
    return node->parent;
}

static inline pcdom_node_t *
pcdom_node_first_child(pcdom_node_t *node)
{
    return node->first_child;
}

static inline pcdom_node_t *
pcdom_node_last_child(pcdom_node_t *node)
{
    return node->last_child;
}

// ============================= for character data ===========================
struct pcdom_character_data {
    pcdom_node_t  node;

    pcutils_str_t   data;
};


pcdom_character_data_t *
pcdom_character_data_interface_create(
                pcdom_document_t *document);

pcdom_character_data_t *
pcdom_character_data_interface_destroy(
                pcdom_character_data_t *character_data);

unsigned int
pcdom_character_data_replace(pcdom_character_data_t *ch_data,
                const unsigned char *data, size_t len,
                size_t offset, size_t count);


// ============================= for text node =================================
struct pcdom_text {
    pcdom_character_data_t char_data;
};

pcdom_text_t *
pcdom_text_interface_create(pcdom_document_t *document);

pcdom_text_t *
pcdom_text_interface_destroy(pcdom_text_t *text);


// ============================= for cdata_section ============================
struct pcdom_cdata_section {
    pcdom_text_t text;
};

pcdom_cdata_section_t *
pcdom_cdata_section_interface_create(
                pcdom_document_t *document);

pcdom_cdata_section_t *
pcdom_cdata_section_interface_destroy(
                pcdom_cdata_section_t *cdata_section);


// ============================= for comment ==================================
struct pcdom_comment {
    pcdom_character_data_t char_data;
};

pcdom_comment_t *
pcdom_comment_interface_create(
                pcdom_document_t *document);

pcdom_comment_t *
pcdom_comment_interface_destroy(
                pcdom_comment_t *comment);

// ============================= for document =================================
typedef enum {
    PCDOM_DOCUMENT_CMODE_NO_QUIRKS       = 0x00,
    PCDOM_DOCUMENT_CMODE_QUIRKS          = 0x01,
    PCDOM_DOCUMENT_CMODE_LIMITED_QUIRKS  = 0x02
} pcdom_document_cmode_t;

typedef enum {
    PCDOM_DOCUMENT_DTYPE_UNDEF = 0x00,
    PCDOM_DOCUMENT_DTYPE_HTML  = 0x01,
    PCDOM_DOCUMENT_DTYPE_XML   = 0x02
} pcdom_document_dtype_t;

struct pcdom_document {
    pcdom_node_t              node;

    pcdom_document_cmode_t    compat_mode;
    pcdom_document_dtype_t    type;

    pcdom_document_type_t     *doctype;
    pcdom_element_t           *element;

    pcdom_interface_create_f  create_interface;
    pcdom_interface_destroy_f destroy_interface;

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

pcdom_document_t *
pcdom_document_interface_create(
            pcdom_document_t *document);

pcdom_document_t *
pcdom_document_interface_destroy(
            pcdom_document_t *document);

pcdom_document_t *
pcdom_document_create(pcdom_document_t *owner);

static inline pcdom_document_t *
pcdom_document_owner(pcdom_document_t *document)
{
    return pcdom_interface_node(document)->owner_document;
}

static inline bool
pcdom_document_is_original(pcdom_document_t *document)
{
    return pcdom_interface_node(document)->owner_document == document;
}

unsigned int
pcdom_document_init(pcdom_document_t *document, pcdom_document_t *owner,
            pcdom_interface_create_f create_interface,
            pcdom_interface_destroy_f destroy_interface,
            pcdom_document_dtype_t type, unsigned int ns);

unsigned int
pcdom_document_clean(pcdom_document_t *document);

pcdom_document_t *
pcdom_document_destroy(pcdom_document_t *document);

void
pcdom_document_attach_doctype(pcdom_document_t *document,
            pcdom_document_type_t *doctype);

void
pcdom_document_attach_element(pcdom_document_t *document,
            pcdom_element_t *element);

pcdom_element_t *
pcdom_document_create_element(pcdom_document_t *document,
            const unsigned char *local_name, size_t lname_len,
            void *reserved_for_opt, bool self_close);

pcdom_element_t *
pcdom_document_destroy_element(
            pcdom_element_t *element);

pcdom_document_fragment_t *
pcdom_document_create_document_fragment(
            pcdom_document_t *document);

pcdom_text_t *
pcdom_document_create_text_node(pcdom_document_t *document,
            const unsigned char *data, size_t len);

pcdom_cdata_section_t *
pcdom_document_create_cdata_section(pcdom_document_t *document,
            const unsigned char *data, size_t len);

pcdom_processing_instruction_t *
pcdom_document_create_processing_instruction(pcdom_document_t *document,
            const unsigned char *target, size_t target_len,
            const unsigned char *data, size_t data_len);

pcdom_comment_t *
pcdom_document_create_comment(pcdom_document_t *document,
            const unsigned char *data, size_t len);


/*
 * Inline functions
 */
static inline pcdom_interface_t *
pcdom_document_create_interface(pcdom_document_t *document,
                                  pchtml_tag_id_t tag_id, pchtml_ns_id_t ns)
{
    return document->create_interface(document, tag_id, ns);
}

static inline pcdom_interface_t *
pcdom_document_destroy_interface(pcdom_interface_t *intrfc)
{
    return pcdom_interface_node(intrfc)->owner_document->destroy_interface(intrfc);
}

static inline void *
pcdom_document_create_struct(pcdom_document_t *document, size_t struct_size)
{
    return pcutils_mraw_calloc(document->mraw, struct_size);
}

static inline void *
pcdom_document_destroy_struct(pcdom_document_t *document, void *structure)
{
    return pcutils_mraw_free(document->mraw, structure);
}

static inline unsigned char *
pcdom_document_create_text(pcdom_document_t *document, size_t len)
{
    return (unsigned char *) pcutils_mraw_alloc(document->text,
                                            sizeof(unsigned char) * len);
}

static inline void *
pcdom_document_destroy_text(pcdom_document_t *document, unsigned char *text)
{
    return pcutils_mraw_free(document->text, text);
}

static inline pcdom_element_t *
pcdom_document_element(pcdom_document_t *document)
{
    return document->element;
}


// ============================= for document fragment ========================
struct pcdom_document_fragment {
    pcdom_node_t    node;

    pcdom_element_t *host;
};

pcdom_document_fragment_t *
pcdom_document_fragment_interface_create(
            pcdom_document_t *document);

pcdom_document_fragment_t *
pcdom_document_fragment_interface_destroy(
            pcdom_document_fragment_t *document_fragment);


// ============================= for attribute ================================
typedef struct {
    pcutils_hash_entry_t entry;
    pcdom_attr_id_t      attr_id;
    size_t               ref_count;
    bool                 read_only;
} pcdom_attr_data_t;

/* More memory to God of memory! */
struct pcdom_attr {
    pcdom_node_t     node;

    /* For example: <LalAla:DiV Fix:Me="value"> */

    pcdom_attr_id_t  upper_name;     /* uppercase, with prefix: FIX:ME */
    pcdom_attr_id_t  qualified_name; /* original, with prefix: Fix:Me */

    pcutils_str_t    *value;
    pcdom_element_t  *owner;

    pcdom_attr_t     *next;
    pcdom_attr_t     *prev;
};

pcdom_attr_t *
pcdom_attr_interface_create(pcdom_document_t *document);

pcdom_attr_t *
pcdom_attr_interface_destroy(pcdom_attr_t *attr);

unsigned int
pcdom_attr_set_name(pcdom_attr_t *attr, const unsigned char *local_name,
                      size_t local_name_len, bool to_lowercase);

unsigned int
pcdom_attr_set_value(pcdom_attr_t *attr,
                const unsigned char *value, size_t value_len);

unsigned int
pcdom_attr_set_value_wo_copy(pcdom_attr_t *attr,
                unsigned char *value, size_t value_len);

unsigned int
pcdom_attr_set_existing_value(pcdom_attr_t *attr,
                const unsigned char *value, size_t value_len);

unsigned int
pcdom_attr_clone_name_value(pcdom_attr_t *attr_from,
                pcdom_attr_t *attr_to);

bool
pcdom_attr_compare(pcdom_attr_t *first,
                pcdom_attr_t *second);

const pcdom_attr_data_t *
pcdom_attr_data_by_id(pcutils_hash_t *hash,
                pcdom_attr_id_t attr_id);

const pcdom_attr_data_t *
pcdom_attr_data_by_local_name(pcutils_hash_t *hash,
                const unsigned char *name, size_t length);

const pcdom_attr_data_t *
pcdom_attr_data_by_qualified_name(pcutils_hash_t *hash,
                                    const unsigned char *name, size_t length);

const unsigned char *
pcdom_attr_qualified_name(pcdom_attr_t *attr, size_t *len);

/*
 * Inline functions
 */
static inline const unsigned char *
pcdom_attr_local_name(pcdom_attr_t *attr, size_t *len)
{
    const pcdom_attr_data_t *data;

    data = pcdom_attr_data_by_id(attr->node.owner_document->attrs,
                                   attr->node.local_name);

    if (len != NULL) {
        *len = data->entry.length;
    }

    return pcutils_hash_entry_str(&data->entry);
}

static inline const unsigned char *
pcdom_attr_value(pcdom_attr_t *attr, size_t *len)
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
struct pcdom_document_type {
    pcdom_node_t    node;

    pcdom_attr_id_t name;
    pcutils_str_t      public_id;
    pcutils_str_t      system_id;
};

pcdom_document_type_t *
pcdom_document_type_interface_create(
                pcdom_document_t *document);

pcdom_document_type_t *
pcdom_document_type_interface_destroy(
                pcdom_document_type_t *document_type);


const unsigned char *
pcdom_document_type_name(pcdom_document_type_t *doc_type, size_t *len);

static inline const unsigned char *
pcdom_document_type_public_id(pcdom_document_type_t *doc_type, size_t *len)
{
    if (len != NULL) {
        *len = doc_type->public_id.length;
    }

    return doc_type->public_id.data;
}

static inline const unsigned char *
pcdom_document_type_system_id(pcdom_document_type_t *doc_type, size_t *len)
{
    if (len != NULL) {
        *len = doc_type->system_id.length;
    }

    return doc_type->system_id.data;
}


// ============================= for collection ===============================
typedef struct {
    pcutils_array_t     array;
    pcdom_document_t *document;
} pcdom_collection_t;

pcdom_collection_t *
pcdom_collection_create(pcdom_document_t *document);

unsigned int
pcdom_collection_init(pcdom_collection_t *col,
                size_t start_list_size);

pcdom_collection_t *
pcdom_collection_destroy(pcdom_collection_t *col,
                bool self_destroy);


/*
 * Inline functions
 */
static inline pcdom_collection_t *
pcdom_collection_make(pcdom_document_t *document, size_t start_list_size)
{
    unsigned int status;
    pcdom_collection_t *col;

    col = pcdom_collection_create(document);
    status = pcdom_collection_init(col, start_list_size);

    if(status != PURC_ERROR_OK) {
        return pcdom_collection_destroy(col, true);
    }

    return col;
}

static inline void
pcdom_collection_clean(pcdom_collection_t *col)
{
    pcutils_array_clean(&col->array);
}

static inline unsigned int
pcdom_collection_append(pcdom_collection_t *col, void *value)
{
    return pcutils_array_push(&col->array, value);
}

static inline pcdom_element_t *
pcdom_collection_element(pcdom_collection_t *col, size_t idx)
{
    return (pcdom_element_t *) pcutils_array_get(&col->array, idx);
}

static inline pcdom_node_t *
pcdom_collection_node(pcdom_collection_t *col, size_t idx)
{
    return (pcdom_node_t *) pcutils_array_get(&col->array, idx);
}

static inline size_t
pcdom_collection_length(pcdom_collection_t *col)
{
    return pcutils_array_length(&col->array);
}



// ============================= for element ==================================
typedef enum {
    PCDOM_ELEMENT_CUSTOM_STATE_UNDEFINED      = 0x00,
    PCDOM_ELEMENT_CUSTOM_STATE_FAILED         = 0x01,
    PCDOM_ELEMENT_CUSTOM_STATE_UNCUSTOMIZED   = 0x02,
    PCDOM_ELEMENT_CUSTOM_STATE_CUSTOM         = 0x03
} pcdom_element_custom_state_t;

struct pcdom_element {
    pcdom_node_t                 node;

    /* For example: <LalAla:DiV Fix:Me="value"> */

    /* uppercase, with prefix: LALALA:DIV */
    pcdom_attr_id_t              upper_name;

    /* original, with prefix: LalAla:DiV */
    pcdom_attr_id_t              qualified_name;

    pcutils_str_t                *is_value;

    pcdom_attr_t                 *first_attr;
    pcdom_attr_t                 *last_attr;

    pcdom_attr_t                 *attr_id;
    pcdom_attr_t                 *attr_class;

    pcdom_element_custom_state_t custom_state;
    bool                         self_close;
};

pcdom_element_t *
pcdom_element_interface_create(
                pcdom_document_t *document);

pcdom_element_t *
pcdom_element_interface_destroy(
                pcdom_element_t *element);

pcdom_element_t *
pcdom_element_create(pcdom_document_t *document,
                const unsigned char *local_name, size_t lname_len,
                const unsigned char *ns_name, size_t ns_len,
                const unsigned char *prefix, size_t prefix_len,
                const unsigned char *is, size_t is_len,
                bool sync_custom, bool self_close);

pcdom_element_t *
pcdom_element_destroy(pcdom_element_t *element);

bool
pcdom_element_has_attributes(pcdom_element_t *element);

pcdom_attr_t *
pcdom_element_set_attribute(pcdom_element_t *element,
                const unsigned char *qualified_name, size_t qn_len,
                const unsigned char *value, size_t value_len);

const unsigned char *
pcdom_element_get_attribute(pcdom_element_t *element,
                const unsigned char *qualified_name, size_t qn_len,
                size_t *value_len);

unsigned int
pcdom_element_remove_attribute(pcdom_element_t *element,
                const unsigned char *qualified_name, size_t qn_len);

bool
pcdom_element_has_attribute(pcdom_element_t *element,
                const unsigned char *qualified_name, size_t qn_len);

unsigned int
pcdom_element_attr_append(pcdom_element_t *element,
                pcdom_attr_t *attr);

unsigned int
pcdom_element_attr_remove(pcdom_element_t *element,
                pcdom_attr_t *attr);

pcdom_attr_t *
pcdom_element_attr_by_name(pcdom_element_t *element,
                const unsigned char *qualified_name, size_t length);

pcdom_attr_t *
pcdom_element_attr_by_local_name_data(pcdom_element_t *element,
                const pcdom_attr_data_t *data);

pcdom_attr_t *
pcdom_element_attr_by_id(pcdom_element_t *element,
                pcdom_attr_id_t attr_id);

pcdom_attr_t *
pcdom_element_attr_by_data(pcdom_element_t *element,
                const pcdom_attr_data_t *data);

bool
pcdom_element_compare(pcdom_element_t *first,
                pcdom_element_t *second);

pcdom_attr_t *
pcdom_element_attr_is_exist(pcdom_element_t *element,
                const unsigned char *qualified_name, size_t length);

unsigned int
pcdom_element_is_set(pcdom_element_t *element,
                const unsigned char *is, size_t is_len);

unsigned int
pcdom_elements_by_tag_name(pcdom_element_t *root,
                pcdom_collection_t *collection,
                const unsigned char *qualified_name, size_t len);

unsigned int
pcdom_elements_by_class_name(pcdom_element_t *root,
                pcdom_collection_t *collection,
                const unsigned char *class_name, size_t len);

unsigned int
pcdom_elements_by_attr(pcdom_element_t *root,
                pcdom_collection_t *collection,
                const unsigned char *qualified_name, size_t qname_len,
                const unsigned char *value, size_t value_len,
                bool case_insensitive);

unsigned int
pcdom_elements_by_attr_begin(pcdom_element_t *root,
                pcdom_collection_t *collection,
                const unsigned char *qualified_name, size_t qname_len,
                const unsigned char *value, size_t value_len,
                bool case_insensitive);

unsigned int
pcdom_elements_by_attr_end(pcdom_element_t *root,
                pcdom_collection_t *collection,
                const unsigned char *qualified_name, size_t qname_len,
                const unsigned char *value, size_t value_len,
                bool case_insensitive);

unsigned int
pcdom_elements_by_attr_contain(pcdom_element_t *root,
                pcdom_collection_t *collection,
                const unsigned char *qualified_name, size_t qname_len,
                const unsigned char *value, size_t value_len,
                bool case_insensitive);

const unsigned char *
pcdom_element_qualified_name(pcdom_element_t *element,
                size_t *len);

const unsigned char *
pcdom_element_qualified_name_upper(pcdom_element_t *element,
                size_t *len);

const unsigned char *
pcdom_element_local_name(pcdom_element_t *element,
                size_t *len);

const unsigned char *
pcdom_element_prefix(pcdom_element_t *element,
                size_t *len);

const unsigned char *
pcdom_element_tag_name(pcdom_element_t *element,
                size_t *len);


/*
 * Inline functions
 */
static inline const unsigned char *
pcdom_element_id(pcdom_element_t *element, size_t *len)
{
    if (element->attr_id == NULL) {
        if (len != NULL) {
            *len = 0;
        }

        return NULL;
    }

    return pcdom_attr_value(element->attr_id, len);
}

static inline const unsigned char *
pcdom_element_class(pcdom_element_t *element, size_t *len)
{
    if (element->attr_class == NULL) {
        if (len != NULL) {
            *len = 0;
        }

        return NULL;
    }

    return pcdom_attr_value(element->attr_class, len);
}

static inline bool
pcdom_element_is_custom(pcdom_element_t *element)
{
    return element->custom_state & PCDOM_ELEMENT_CUSTOM_STATE_CUSTOM;
}

static inline bool
pcdom_element_custom_is_defined(pcdom_element_t *element)
{
    return element->custom_state & PCDOM_ELEMENT_CUSTOM_STATE_CUSTOM
        || element->custom_state & PCDOM_ELEMENT_CUSTOM_STATE_UNCUSTOMIZED;
}

static inline pcdom_attr_t *
pcdom_element_first_attribute(pcdom_element_t *element)
{
    return element->first_attr;
}

static inline pcdom_attr_t *
pcdom_element_next_attribute(pcdom_attr_t *attr)
{
    return attr->next;
}

static inline pcdom_attr_t *
pcdom_element_prev_attribute(pcdom_attr_t *attr)
{
    return attr->prev;
}
static inline pcdom_attr_t *
pcdom_element_last_attribute(pcdom_element_t *element)
{
    return element->last_attr;
}

static inline pcdom_attr_t *
pcdom_element_id_attribute(pcdom_element_t *element)
{
    return element->attr_id;
}

static inline pcdom_attr_t *
pcdom_element_class_attribute(pcdom_element_t *element)
{
    return element->attr_class;
}

static inline pchtml_tag_id_t
pcdom_element_tag_id(pcdom_element_t *element)
{
    return pcdom_interface_node(element)->local_name;
}

static inline pchtml_ns_id_t
pcdom_element_ns_id(pcdom_element_t *element)
{
    return pcdom_interface_node(element)->ns;
}


// ============================= for processing instruction ===================
struct pcdom_processing_instruction {
    pcdom_character_data_t char_data;

    pcutils_str_t             target;
};


pcdom_processing_instruction_t *
pcdom_processing_instruction_interface_create(
        pcdom_document_t *document);

pcdom_processing_instruction_t *
pcdom_processing_instruction_interface_destroy(
        pcdom_processing_instruction_t *processing_instruction);


/*
 * Inline functions
 */
static inline const unsigned char *
pcdom_processing_instruction_target(pcdom_processing_instruction_t *pi,
                                      size_t *len)
{
    if (len != NULL) {
        *len = pi->target.length;
    }

    return pi->target.data;
}



// ============================= for element-variant ==========================
// .attr(<string: attributeName>)
int
pcdom_element_attr(pcdom_element_t *elem, const char *attr_name,
        const unsigned char **val, size_t *len);

// TODO:
// .prop(<string: propertyName>)
// int
// pcdom_element_prop(pcdom_element_t *elem, const char *attr_name,
//         struct pcdom_attr **attr);

// .style(<string: styleName>)
int
pcdom_element_style(pcdom_element_t *elem, const char *style_name,
        const unsigned char **style, size_t *len);

// .content()
int
pcdom_element_content(pcdom_element_t *elem,
        const unsigned char **content, size_t *len);

// .textContent()
int
pcdom_element_text_content(pcdom_element_t *elem,
        char **text, size_t *len);

// TODO:
// .jsonContent()
// FIXME: json in serialized form?
// static int
// pcdom_element_json_content(pcdom_element_t *elem, char **json);

// .val()
// TODO: what type for val?
// static int
// pcdom_element_val(pcdom_element_t *elem, what_type **val);

// .hasClass(<string: className>)
int
pcdom_element_has_class(pcdom_element_t *elem, const char *class_name,
        bool *has);



// .attr(! <string: attributeName>, <string: value>)
int
pcdom_element_set_attr(pcdom_element_t *elem, const char *attr_name,
        const char *attr_val);

// TODO:
// .prop(! <string: propertyName>, <any: value>)
// static int
// pcdom_element_set_prop(pcdom_element_t *elem, ...);

// .style(! <string: styleName>, <string: value>)
int
pcdom_element_set_style(pcdom_element_t *elem, const char *style_name,
        const char *style);

// FIXME: de-serialize and then replace?
// .content(! <string: content>)
int
pcdom_element_set_content(pcdom_element_t *elem, const char *content);

// .textContent(! <string: content>)
int
pcdom_element_set_text_content(pcdom_element_t *elem,
        const char *text);

// FIXME: json in serialized form?
// .jsonContent(! <string: content>)
int
pcdom_element_set_json_content(pcdom_element_t *elem, const char *json);

// .val(! <newValue>)
// TODO: what type for val?
// static int
// pcdom_element_set_val(pcdom_element_t *elem, const what_type *val);

// .addClass(! <string: className>)
int
pcdom_element_add_class(pcdom_element_t *elem, const char *class_name);

// .removeAttr(! <string: attributeName>)
int
pcdom_element_remove_attr(pcdom_element_t *elem, const char *attr_name);

// .removeClass(! <string: className>)
int
pcdom_element_remove_class_by_name(pcdom_element_t *elem,
        const char *class_name);

// .removeClass(! )
static inline int
pcdom_element_remove_class(pcdom_element_t *elem)
{
    return pcdom_element_remove_class_by_name(elem, NULL);
}

// ============================= for collection-variant =======================
// .count()
int
pcdom_collection_count(pcdom_collection_t *col, size_t *count);

// .at(<real: index>)
int
pcdom_collection_at(pcdom_collection_t *col, size_t idx,
        struct pcdom_element **element);

PCA_EXTERN_C_END

#endif  /* PURC_PURC_DOM_H */

