/*
 * Copyright (C) 2018 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

#include "html/html/interfaces/hr_element.h"
#include "html/html/interfaces/document.h"


pchtml_html_hr_element_t *
pchtml_html_hr_element_interface_create(pchtml_html_document_t *document)
{
    pchtml_html_hr_element_t *element;

    element = pchtml_mraw_calloc(document->dom_document.mraw,
                                 sizeof(pchtml_html_hr_element_t));
    if (element == NULL) {
        return NULL;
    }

    pchtml_dom_node_t *node = pchtml_dom_interface_node(element);

    node->owner_document = pchtml_html_document_original_ref(document);
    node->type = PCHTML_DOM_NODE_TYPE_ELEMENT;

    return element;
}

pchtml_html_hr_element_t *
pchtml_html_hr_element_interface_destroy(pchtml_html_hr_element_t *hr_element)
{
    return pchtml_mraw_free(
        pchtml_dom_interface_node(hr_element)->owner_document->mraw,
        hr_element);
}
