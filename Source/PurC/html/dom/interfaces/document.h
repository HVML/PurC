/**
 * @file document.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for document.
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


#ifndef PCHTML_DOM_DOCUMENT_H
#define PCHTML_DOM_DOCUMENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/core/mraw.h"
#include "html/core/hash.h"

#include "html/dom/interface.h"
#include "html/dom/interfaces/node.h"


typedef enum {
    PCHTML_DOM_DOCUMENT_CMODE_NO_QUIRKS       = 0x00,
    PCHTML_DOM_DOCUMENT_CMODE_QUIRKS          = 0x01,
    PCHTML_DOM_DOCUMENT_CMODE_LIMITED_QUIRKS  = 0x02
}
pchtml_dom_document_cmode_t;

typedef enum {
    PCHTML_DOM_DOCUMENT_DTYPE_UNDEF = 0x00,
    PCHTML_DOM_DOCUMENT_DTYPE_HTML  = 0x01,
    PCHTML_DOM_DOCUMENT_DTYPE_XML   = 0x02
}
pchtml_dom_document_dtype_t;

struct pchtml_dom_document {
    pchtml_dom_node_t              node;

    pchtml_dom_document_cmode_t    compat_mode;
    pchtml_dom_document_dtype_t    type;

    pchtml_dom_document_type_t     *doctype;
    pchtml_dom_element_t           *element;

    pchtml_dom_interface_create_f  create_interface;
    pchtml_dom_interface_destroy_f destroy_interface;

    pchtml_mraw_t               *mraw;
    pchtml_mraw_t               *text;
    pchtml_hash_t               *tags;
    pchtml_hash_t               *attrs;
    pchtml_hash_t               *prefix;
    pchtml_hash_t               *ns;
    void                        *parser;
    void                        *user;

    bool                        tags_inherited;
    bool                        ns_inherited;

    bool                        scripting;
};


pchtml_dom_document_t *
pchtml_dom_document_interface_create(
            pchtml_dom_document_t *document) WTF_INTERNAL;

pchtml_dom_document_t *
pchtml_dom_document_interface_destroy(
            pchtml_dom_document_t *document) WTF_INTERNAL;

pchtml_dom_document_t *
pchtml_dom_document_create(pchtml_dom_document_t *owner) WTF_INTERNAL;

unsigned int
pchtml_dom_document_init(pchtml_dom_document_t *document, pchtml_dom_document_t *owner,
            pchtml_dom_interface_create_f create_interface,
            pchtml_dom_interface_destroy_f destroy_interface,
            pchtml_dom_document_dtype_t type, unsigned int ns) WTF_INTERNAL;

unsigned int
pchtml_dom_document_clean(pchtml_dom_document_t *document) WTF_INTERNAL;

pchtml_dom_document_t *
pchtml_dom_document_destroy(pchtml_dom_document_t *document) WTF_INTERNAL;

void
pchtml_dom_document_attach_doctype(pchtml_dom_document_t *document,
            pchtml_dom_document_type_t *doctype) WTF_INTERNAL;

void
pchtml_dom_document_attach_element(pchtml_dom_document_t *document,
            pchtml_dom_element_t *element) WTF_INTERNAL;

pchtml_dom_element_t *
pchtml_dom_document_create_element(pchtml_dom_document_t *document,
            const unsigned char *local_name, size_t lname_len,
            void *reserved_for_opt) WTF_INTERNAL;

pchtml_dom_element_t *
pchtml_dom_document_destroy_element(
            pchtml_dom_element_t *element) WTF_INTERNAL;

pchtml_dom_document_fragment_t *
pchtml_dom_document_create_document_fragment(
            pchtml_dom_document_t *document) WTF_INTERNAL;

pchtml_dom_text_t *
pchtml_dom_document_create_text_node(pchtml_dom_document_t *document,
            const unsigned char *data, size_t len) WTF_INTERNAL;

pchtml_dom_cdata_section_t *
pchtml_dom_document_create_cdata_section(pchtml_dom_document_t *document,
            const unsigned char *data, size_t len) WTF_INTERNAL;

pchtml_dom_processing_instruction_t *
pchtml_dom_document_create_processing_instruction(pchtml_dom_document_t *document,
            const unsigned char *target, size_t target_len,
            const unsigned char *data, size_t data_len) WTF_INTERNAL;

pchtml_dom_comment_t *
pchtml_dom_document_create_comment(pchtml_dom_document_t *document,
            const unsigned char *data, size_t len) WTF_INTERNAL;


/*
 * Inline functions
 */
static inline pchtml_dom_interface_t *
pchtml_dom_document_create_interface(pchtml_dom_document_t *document,
                                  pchtml_tag_id_t tag_id, pchtml_ns_id_t ns)
{
    return document->create_interface(document, tag_id, ns);
}

static inline pchtml_dom_interface_t *
pchtml_dom_document_destroy_interface(pchtml_dom_interface_t *intrfc)
{
    return pchtml_dom_interface_node(intrfc)->owner_document->destroy_interface(intrfc);
}

static inline void *
pchtml_dom_document_create_struct(pchtml_dom_document_t *document, size_t struct_size)
{
    return pchtml_mraw_calloc(document->mraw, struct_size);
}

static inline void *
pchtml_dom_document_destroy_struct(pchtml_dom_document_t *document, void *structure)
{
    return pchtml_mraw_free(document->mraw, structure);
}

static inline unsigned char *
pchtml_dom_document_create_text(pchtml_dom_document_t *document, size_t len)
{
    return (unsigned char *) pchtml_mraw_alloc(document->text,
                                            sizeof(unsigned char) * len);
}

static inline void *
pchtml_dom_document_destroy_text(pchtml_dom_document_t *document, unsigned char *text)
{
    return pchtml_mraw_free(document->text, text);
}

static inline pchtml_dom_element_t *
pchtml_dom_document_element(pchtml_dom_document_t *document)
{
    return document->element;
}

/*
 * No inline functions for ABI.
 */
pchtml_dom_interface_t *
pchtml_dom_document_create_interface_noi(pchtml_dom_document_t *document,
                                      pchtml_tag_id_t tag_id, pchtml_ns_id_t ns);

pchtml_dom_interface_t *
pchtml_dom_document_destroy_interface_noi(pchtml_dom_interface_t *intrfc);

void *
pchtml_dom_document_create_struct_noi(pchtml_dom_document_t *document,
                                   size_t struct_size);

void *
pchtml_dom_document_destroy_struct_noi(pchtml_dom_document_t *document,
                                    void *structure);

unsigned char *
pchtml_dom_document_create_text_noi(pchtml_dom_document_t *document, size_t len);

void *
pchtml_dom_document_destroy_text_noi(pchtml_dom_document_t *document,
                                  unsigned char *text);

pchtml_dom_element_t *
pchtml_dom_document_element_noi(pchtml_dom_document_t *document);


#ifdef __cplusplus
}       /* __cplusplus */
   
#endif

#endif  /* PCHTML_DOM_DOCUMENT_H */
