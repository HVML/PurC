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
 *
 * This implementation of HTML parser is derived from Lexbor <http://lexbor.com/>.
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */


#ifndef PCEDOM_DOCUMENT_H
#define PCEDOM_DOCUMENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/core/mraw.h"
#include "html/core/hash.h"

#include "private/edom/interface.h"
#include "private/edom/node.h"


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


pcedom_document_t *
pcedom_document_interface_create(
            pcedom_document_t *document) WTF_INTERNAL;

pcedom_document_t *
pcedom_document_interface_destroy(
            pcedom_document_t *document) WTF_INTERNAL;

pcedom_document_t *
pcedom_document_create(pcedom_document_t *owner) WTF_INTERNAL;

unsigned int
pcedom_document_init(pcedom_document_t *document, pcedom_document_t *owner,
            pcedom_interface_create_f create_interface,
            pcedom_interface_destroy_f destroy_interface,
            pcedom_document_dtype_t type, unsigned int ns) WTF_INTERNAL;

unsigned int
pcedom_document_clean(pcedom_document_t *document) WTF_INTERNAL;

pcedom_document_t *
pcedom_document_destroy(pcedom_document_t *document) WTF_INTERNAL;

void
pcedom_document_attach_doctype(pcedom_document_t *document,
            pcedom_document_type_t *doctype) WTF_INTERNAL;

void
pcedom_document_attach_element(pcedom_document_t *document,
            pcedom_element_t *element) WTF_INTERNAL;

pcedom_element_t *
pcedom_document_create_element(pcedom_document_t *document,
            const unsigned char *local_name, size_t lname_len,
            void *reserved_for_opt) WTF_INTERNAL;

pcedom_element_t *
pcedom_document_destroy_element(
            pcedom_element_t *element) WTF_INTERNAL;

pcedom_document_fragment_t *
pcedom_document_create_document_fragment(
            pcedom_document_t *document) WTF_INTERNAL;

pcedom_text_t *
pcedom_document_create_text_node(pcedom_document_t *document,
            const unsigned char *data, size_t len) WTF_INTERNAL;

pcedom_cdata_section_t *
pcedom_document_create_cdata_section(pcedom_document_t *document,
            const unsigned char *data, size_t len) WTF_INTERNAL;

pcedom_processing_instruction_t *
pcedom_document_create_processing_instruction(pcedom_document_t *document,
            const unsigned char *target, size_t target_len,
            const unsigned char *data, size_t data_len) WTF_INTERNAL;

pcedom_comment_t *
pcedom_document_create_comment(pcedom_document_t *document,
            const unsigned char *data, size_t len) WTF_INTERNAL;


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
    return pchtml_mraw_calloc(document->mraw, struct_size);
}

static inline void *
pcedom_document_destroy_struct(pcedom_document_t *document, void *structure)
{
    return pchtml_mraw_free(document->mraw, structure);
}

static inline unsigned char *
pcedom_document_create_text(pcedom_document_t *document, size_t len)
{
    return (unsigned char *) pchtml_mraw_alloc(document->text,
                                            sizeof(unsigned char) * len);
}

static inline void *
pcedom_document_destroy_text(pcedom_document_t *document, unsigned char *text)
{
    return pchtml_mraw_free(document->text, text);
}

static inline pcedom_element_t *
pcedom_document_element(pcedom_document_t *document)
{
    return document->element;
}

/*
 * No inline functions for ABI.
 */
pcedom_interface_t *
pcedom_document_create_interface_noi(pcedom_document_t *document,
                                      pchtml_tag_id_t tag_id, pchtml_ns_id_t ns);

pcedom_interface_t *
pcedom_document_destroy_interface_noi(pcedom_interface_t *intrfc);

void *
pcedom_document_create_struct_noi(pcedom_document_t *document,
                                   size_t struct_size);

void *
pcedom_document_destroy_struct_noi(pcedom_document_t *document,
                                    void *structure);

unsigned char *
pcedom_document_create_text_noi(pcedom_document_t *document, size_t len);

void *
pcedom_document_destroy_text_noi(pcedom_document_t *document,
                                  unsigned char *text);

pcedom_element_t *
pcedom_document_element_noi(pcedom_document_t *document);


#ifdef __cplusplus
}       /* __cplusplus */
   
#endif

#endif  /* PCEDOM_DOCUMENT_H */
