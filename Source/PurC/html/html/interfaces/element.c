/*
 * Copyright (C) 2018 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

#include "html/html/interfaces/element.h"
#include "html/html/interfaces/document.h"


pchtml_html_element_t *
pchtml_html_element_interface_create(pchtml_html_document_t *document)
{
    pchtml_html_element_t *element;

    element = pchtml_mraw_calloc(document->dom_document.mraw,
                                 sizeof(pchtml_html_element_t));
    if (element == NULL) {
        return NULL;
    }

    pchtml_dom_node_t *node = pchtml_dom_interface_node(element);

    node->owner_document = pchtml_html_document_original_ref(document);
    node->type = PCHTML_DOM_NODE_TYPE_ELEMENT;

    return element;
}

pchtml_html_element_t *
pchtml_html_element_interface_destroy(pchtml_html_element_t *element)
{
    return pchtml_mraw_free(
                pchtml_dom_interface_node(element)->owner_document->mraw, element);
}

pchtml_html_element_t *
pchtml_html_element_inner_html_set(pchtml_html_element_t *element,
                                const unsigned char *html, size_t size)
{
    pchtml_dom_node_t *node, *child;
    pchtml_dom_node_t *root = pchtml_dom_interface_node(element);
    pchtml_html_document_t *doc = pchtml_html_interface_document(root->owner_document);

    node = pchtml_html_document_parse_fragment(doc, &element->element, html, size);
    if (node == NULL) {
        return NULL;
    }

    while (root->first_child != NULL) {
        pchtml_dom_node_destroy_deep(root->first_child);
    }

    while (node->first_child != NULL) {
        child = node->first_child;

        pchtml_dom_node_remove(child);
        pchtml_dom_node_insert_child(root, child);
    }

    pchtml_dom_node_destroy(node);

    return pchtml_html_interface_element(root);
}
