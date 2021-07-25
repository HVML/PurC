/*
 * Copyright (C) 2018 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

#ifndef PCHTML_HTML_ELEMENT_H
#define PCHTML_HTML_ELEMENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "html/html/interface.h"
#include "html/dom/interfaces/element.h"


struct pchtml_html_element {
    pchtml_dom_element_t element;
};


GENGYUE_API pchtml_html_element_t *
pchtml_html_element_interface_create(pchtml_html_document_t *document);

GENGYUE_API pchtml_html_element_t *
pchtml_html_element_interface_destroy(pchtml_html_element_t *element);


GENGYUE_API pchtml_html_element_t *
pchtml_html_element_inner_html_set(pchtml_html_element_t *element,
                                const unsigned char *html, size_t size);

/*
 * Inline functions
 */
static inline pchtml_tag_id_t
pchtml_html_element_tag_id(pchtml_html_element_t *element)
{
    return pchtml_dom_interface_node(element)->local_name;
}

static inline pchtml_ns_id_t
pchtml_html_element_ns_id(pchtml_html_element_t *element)
{
    return pchtml_dom_interface_node(element)->ns;
}


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PCHTML_HTML_ELEMENT_H */
