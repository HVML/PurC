/*
 * Copyright (C) 2018 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

#ifndef PCHTML_HTML_TITLE_ELEMENT_H
#define PCHTML_HTML_TITLE_ELEMENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "html/html/interface.h"
#include "html/html/interfaces/element.h"


struct pchtml_html_title_element {
    pchtml_html_element_t element;

    pchtml_str_t       *strict_text;
};


GENGYUE_API pchtml_html_title_element_t *
pchtml_html_title_element_interface_create(pchtml_html_document_t *document);

GENGYUE_API pchtml_html_title_element_t *
pchtml_html_title_element_interface_destroy(pchtml_html_title_element_t *title_element);

GENGYUE_API const unsigned char *
pchtml_html_title_element_text(pchtml_html_title_element_t *title, size_t *len);

GENGYUE_API const unsigned char *
pchtml_html_title_element_strict_text(pchtml_html_title_element_t *title, size_t *len);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PCHTML_HTML_TITLE_ELEMENT_H */
