/*
 * Copyright (C) 2018 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

#ifndef PCHTML_HTML_DOCUMENT_H
#define PCHTML_HTML_DOCUMENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "html/core/mraw.h"

#include "html/tag/tag.h"
#include "html/ns/ns.h"
#include "html/html/interface.h"
#include "html/dom/interfaces/attr.h"
#include "html/dom/interfaces/document.h"


typedef unsigned int pchtml_html_document_opt_t;


typedef enum {
    PCHTML_HTML_DOCUMENT_READY_STATE_UNDEF       = 0x00,
    PCHTML_HTML_DOCUMENT_READY_STATE_LOADING     = 0x01,
    PCHTML_HTML_DOCUMENT_READY_STATE_INTERACTIVE = 0x02,
    PCHTML_HTML_DOCUMENT_READY_STATE_COMPLETE    = 0x03,
}
pchtml_html_document_ready_state_t;

enum pchtml_html_document_opt {
    PCHTML_HTML_DOCUMENT_OPT_UNDEF     = 0x00,
    PCHTML_HTML_DOCUMENT_PARSE_WO_COPY = 0x01
};

struct pchtml_html_document {
    pchtml_dom_document_t              dom_document;

    void                            *iframe_srcdoc;

    pchtml_html_head_element_t         *head;
    pchtml_html_body_element_t         *body;

    pchtml_html_document_ready_state_t ready_state;

    pchtml_html_document_opt_t         opt;
};

GENGYUE_API pchtml_html_document_t *
pchtml_html_document_interface_create(pchtml_html_document_t *document);

GENGYUE_API pchtml_html_document_t *
pchtml_html_document_interface_destroy(pchtml_html_document_t *document);


GENGYUE_API pchtml_html_document_t *
pchtml_html_document_create(void);

GENGYUE_API void
pchtml_html_document_clean(pchtml_html_document_t *document);

GENGYUE_API pchtml_html_document_t *
pchtml_html_document_destroy(pchtml_html_document_t *document);


GENGYUE_API unsigned int
pchtml_html_document_parse(pchtml_html_document_t *document,
                        const unsigned char *html, size_t size);

GENGYUE_API unsigned int
pchtml_html_document_parse_chunk_begin(pchtml_html_document_t *document);

GENGYUE_API unsigned int
pchtml_html_document_parse_chunk(pchtml_html_document_t *document,
                              const unsigned char *html, size_t size);

GENGYUE_API unsigned int
pchtml_html_document_parse_chunk_end(pchtml_html_document_t *document);

GENGYUE_API pchtml_dom_node_t *
pchtml_html_document_parse_fragment(pchtml_html_document_t *document,
                                 pchtml_dom_element_t *element,
                                 const unsigned char *html, size_t size);

GENGYUE_API unsigned int
pchtml_html_document_parse_fragment_chunk_begin(pchtml_html_document_t *document,
                                             pchtml_dom_element_t *element);

GENGYUE_API unsigned int
pchtml_html_document_parse_fragment_chunk(pchtml_html_document_t *document,
                                       const unsigned char *html, size_t size);

GENGYUE_API pchtml_dom_node_t *
pchtml_html_document_parse_fragment_chunk_end(pchtml_html_document_t *document);

GENGYUE_API const unsigned char *
pchtml_html_document_title(pchtml_html_document_t *document, size_t *len);

GENGYUE_API unsigned int
pchtml_html_document_title_set(pchtml_html_document_t *document,
                            const unsigned char *title, size_t len);

GENGYUE_API const unsigned char *
pchtml_html_document_title_raw(pchtml_html_document_t *document, size_t *len);


/*
 * Inline functions
 */
static inline pchtml_html_head_element_t *
pchtml_html_document_head_element(pchtml_html_document_t *document)
{
    return document->head;
}

static inline pchtml_html_body_element_t *
pchtml_html_document_body_element(pchtml_html_document_t *document)
{
    return document->body;
}

static inline pchtml_dom_document_t *
pchtml_html_document_original_ref(pchtml_html_document_t *document)
{
    if (pchtml_dom_interface_node(document)->owner_document
        != &document->dom_document)
    {
        return pchtml_dom_interface_node(document)->owner_document;
    }

    return pchtml_dom_interface_document(document);
}

static inline bool
pchtml_html_document_is_original(pchtml_html_document_t *document)
{
    return pchtml_dom_interface_node(document)->owner_document
        == &document->dom_document;
}

static inline pchtml_mraw_t *
pchtml_html_document_mraw(pchtml_html_document_t *document)
{
    return (pchtml_mraw_t *) pchtml_dom_interface_document(document)->mraw;
}

static inline pchtml_mraw_t *
pchtml_html_document_mraw_text(pchtml_html_document_t *document)
{
    return (pchtml_mraw_t *) pchtml_dom_interface_document(document)->text;
}

static inline void
pchtml_html_document_opt_set(pchtml_html_document_t *document,
                          pchtml_html_document_opt_t opt)
{
    document->opt = opt;
}

static inline pchtml_html_document_opt_t
pchtml_html_document_opt(pchtml_html_document_t *document)
{
    return document->opt;
}

static inline pchtml_hash_t *
pchtml_html_document_tags(pchtml_html_document_t *document)
{
    return document->dom_document.tags;
}

static inline void *
pchtml_html_document_create_struct(pchtml_html_document_t *document,
                                size_t struct_size)
{
    return pchtml_mraw_calloc(pchtml_dom_interface_document(document)->mraw,
                              struct_size);
}

static inline void *
pchtml_html_document_destroy_struct(pchtml_html_document_t *document, void *data)
{
    return pchtml_mraw_free(pchtml_dom_interface_document(document)->mraw, data);
}

static inline pchtml_html_element_t *
pchtml_html_document_create_element(pchtml_html_document_t *document,
                                 const unsigned char *local_name, size_t lname_len,
                                 void *reserved_for_opt)
{
    return (pchtml_html_element_t *) pchtml_dom_document_create_element(&document->dom_document,
                                                                  local_name, lname_len,
                                                                  reserved_for_opt);
}

static inline pchtml_dom_element_t *
pchtml_html_document_destroy_element(pchtml_dom_element_t *element)
{
    return pchtml_dom_document_destroy_element(element);
}

/*
 * No inline functions for ABI.
 */
pchtml_html_head_element_t *
pchtml_html_document_head_element_noi(pchtml_html_document_t *document);

pchtml_html_body_element_t *
pchtml_html_document_body_element_noi(pchtml_html_document_t *document);

pchtml_dom_document_t *
pchtml_html_document_original_ref_noi(pchtml_html_document_t *document);

bool
pchtml_html_document_is_original_noi(pchtml_html_document_t *document);

pchtml_mraw_t *
pchtml_html_document_mraw_noi(pchtml_html_document_t *document);

pchtml_mraw_t *
pchtml_html_document_mraw_text_noi(pchtml_html_document_t *document);

void
pchtml_html_document_opt_set_noi(pchtml_html_document_t *document,
                              pchtml_html_document_opt_t opt);

pchtml_html_document_opt_t
pchtml_html_document_opt_noi(pchtml_html_document_t *document);

void *
pchtml_html_document_create_struct_noi(pchtml_html_document_t *document,
                                    size_t struct_size);

void *
pchtml_html_document_destroy_struct_noi(pchtml_html_document_t *document, void *data);

pchtml_html_element_t *
pchtml_html_document_create_element_noi(pchtml_html_document_t *document,
                                     const unsigned char *local_name,
                                     size_t lname_len, void *reserved_for_opt);

pchtml_dom_element_t *
pchtml_html_document_destroy_element_noi(pchtml_dom_element_t *element);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PCHTML_HTML_DOCUMENT_H */
