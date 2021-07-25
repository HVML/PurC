/*
 * Copyright (C) 2018 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

#ifndef PCHTML_HTML_MAP_ELEMENT_H
#define PCHTML_HTML_MAP_ELEMENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "html/html/interface.h"
#include "html/html/interfaces/element.h"


struct pchtml_html_map_element {
    pchtml_html_element_t element;
};


GENGYUE_API pchtml_html_map_element_t *
pchtml_html_map_element_interface_create(pchtml_html_document_t *document);

GENGYUE_API pchtml_html_map_element_t *
pchtml_html_map_element_interface_destroy(pchtml_html_map_element_t *map_element);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PCHTML_HTML_MAP_ELEMENT_H */
