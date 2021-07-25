/*
 * Copyright (C) 2018 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

#ifndef PCHTML_HTML_TEMPLATE_ELEMENT_H
#define PCHTML_HTML_TEMPLATE_ELEMENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "html/dom/interfaces/document_fragment.h"

#include "html/html/interface.h"
#include "html/html/interfaces/element.h"


struct pchtml_html_template_element {
    pchtml_html_element_t          element;

    pchtml_dom_document_fragment_t *content;
};


GENGYUE_API pchtml_html_template_element_t *
pchtml_html_template_element_interface_create(pchtml_html_document_t *document);

GENGYUE_API pchtml_html_template_element_t *
pchtml_html_template_element_interface_destroy(pchtml_html_template_element_t *template_element);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PCHTML_HTML_TEMPLATE_ELEMENT_H */
