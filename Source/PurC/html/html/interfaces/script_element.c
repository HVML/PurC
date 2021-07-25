/*
 * Copyright (C) 2018 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

#include "html/html/interfaces/script_element.h"
#include "html/html/interfaces/document.h"


pchtml_html_script_element_t *
pchtml_html_script_element_interface_create(pchtml_html_document_t *document)
{
    pchtml_html_script_element_t *element;

    element = pchtml_mraw_calloc(document->dom_document.mraw,
                                 sizeof(pchtml_html_script_element_t));
    if (element == NULL) {
        return NULL;
    }

    pchtml_dom_node_t *node = pchtml_dom_interface_node(element);

    node->owner_document = pchtml_html_document_original_ref(document);
    node->type = PCHTML_DOM_NODE_TYPE_ELEMENT;

    return element;
}

pchtml_html_script_element_t *
pchtml_html_script_element_interface_destroy(pchtml_html_script_element_t *script_element)
{
    return pchtml_mraw_free(
        pchtml_dom_interface_node(script_element)->owner_document->mraw,
        script_element);
}
