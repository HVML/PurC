/**
 * @file html-document.c
 * @author Vincent Wei
 * @date 2022/07/12
 * @brief The implementation of html document.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
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

#undef NDEBUG

#include "purc-document.h"
#include "purc-errors.h"
#include "purc-html.h"

#include "private/document.h"

static purc_document_t create(const char *content, size_t length)
{
    pchtml_html_document_t *html_doc;
    html_doc = pchtml_html_document_create();
    if (!html_doc) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    if (content == NULL) {
        content = "<html></html>";
        length = 0;
    }

    if (length == 0) {
        length = strlen(content);
    }

    unsigned int r;
    r = pchtml_html_document_parse_with_buf(html_doc,
            (const unsigned char*)content, length);
    if (r) {
        pchtml_html_document_destroy(html_doc);
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    purc_document_t doc = calloc(1, sizeof(*doc));
    doc->data_content = 0;
    doc->have_head = 1;
    doc->have_body = 1;
    doc->ops = &_pcdoc_html_ops;
    doc->impl = html_doc;

    return doc;
}

static void destroy(purc_document_t doc)
{
    assert(doc->impl);
    pchtml_html_document_destroy(doc->impl);
    free(doc);
}

static void
dom_append_node_to_element(pcdom_element_t *element,
        pcdom_node_t *node)
{
    pcdom_node_t *parent = pcdom_interface_node(element);
    pcdom_node_append_child(parent, node);
}

static void
dom_prepend_node_to_element(pcdom_element_t *element,
        pcdom_node_t *node)
{
    pcdom_node_t *parent = pcdom_interface_node(element);
    pcdom_node_prepend_child(parent, node);
}

static void
dom_insert_node_before_element(pcdom_element_t *element,
        pcdom_node_t *node)
{
    pcdom_node_t *to = pcdom_interface_node(element);
    pcdom_node_insert_before(to, node);
}

static void
dom_insert_node_after_element(pcdom_element_t *element,
        pcdom_node_t *node)
{
    pcdom_node_t *to = pcdom_interface_node(element);
    pcdom_node_insert_after(to, node);
}

static void
dom_displace_content_by_node(pcdom_element_t *element,
        pcdom_node_t *node)
{
    pcdom_node_t *parent = pcdom_interface_node(element);

    while (parent->first_child != NULL) {
        pcdom_node_destroy_deep(parent->first_child);
    }
    pcdom_node_append_child(parent, node);
}

typedef void (*dom_node_op)(pcdom_element_t *element,
        pcdom_node_t *node);

static const dom_node_op dom_node_ops[] = {
    dom_append_node_to_element,
    dom_prepend_node_to_element,
    dom_insert_node_before_element,
    dom_insert_node_after_element,
    dom_displace_content_by_node,
};

static inline void
dom_erase_element(pcdom_element_t *element)
{
    pcdom_node_t *node = pcdom_interface_node(element);

    pcdom_node_destroy_deep(node);
}

static inline void
dom_clear_element(pcdom_element_t *element)
{
    pcdom_node_t *parent = pcdom_interface_node(element);
    while (parent->first_child != NULL) {
        pcdom_node_destroy_deep(parent->first_child);
    }
}

static pcdoc_element_t operate_element(purc_document_t doc,
            pcdoc_element_t elem, pcdoc_operation op,
            const char *tag, bool self_close)
{
    UNUSED_PARAM(self_close);

    if (op == PCDOC_OP_ERASE) {
        dom_erase_element(pcdom_interface_element(elem));
        return NULL;
    }
    else if (op == PCDOC_OP_CLEAR) {
        dom_clear_element(pcdom_interface_element(elem));
        return elem;
    }

    if (UNLIKELY(op >= PCA_TABLESIZE(dom_node_ops))) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    pcdom_element_t *dom_elem = pcdom_interface_element(elem);
    pcdom_document_t *dom_doc = pcdom_interface_document(doc->impl);
    pcdom_element_t *new_elem;
    new_elem = pcdom_document_create_element(dom_doc,
            (const unsigned char*)tag, strlen(tag), NULL);
    if (new_elem) {
        dom_node_ops[op](dom_elem, pcdom_interface_node(new_elem));
    }
    else {
        purc_set_error(PURC_ERROR_INVALID_VALUE); // TODO
    }

    return (pcdoc_element_t)new_elem;
}

static pcdoc_text_node_t new_text_content(purc_document_t doc,
            pcdoc_element_t elem, pcdoc_operation op,
            const char *text, size_t length)
{
    if (UNLIKELY(op >= PCA_TABLESIZE(dom_node_ops))) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    pcdom_document_t *dom_doc = pcdom_interface_document(doc->impl);
    pcdom_element_t *dom_elem = pcdom_interface_element(elem);
    pcdom_text_t *text_node;

    text_node = pcdom_document_create_text_node(dom_doc,
            (const unsigned char *)text, length ? length : strlen(text));
    if (text_node) {
        dom_node_ops[op](dom_elem, pcdom_interface_node(text_node));
    }
    else {
        purc_set_error(PURC_ERROR_INVALID_VALUE); // TODO
    }

    return (pcdoc_text_node_t)text_node;
}

static pcdom_node_t *
dom_parse_fragment(pcdom_document_t *dom_doc,
        pcdom_element_t *parent, const char *fragment, size_t length)
{
    unsigned int status;
    pchtml_html_document_t *html_doc;
    pcdom_node_t *root = NULL;

    html_doc = (pchtml_html_document_t *)dom_doc;
    status = pchtml_html_document_parse_fragment_chunk_begin(html_doc, parent);
    if (status)
        goto failed;

    status = pchtml_html_document_parse_fragment_chunk(html_doc,
            (const unsigned char*)"<div>", 5);
    if (status)
        goto failed;

    status = pchtml_html_document_parse_fragment_chunk(html_doc,
            (const unsigned char*)fragment, length);
    if (status)
        goto failed;

    status = pchtml_html_document_parse_fragment_chunk(html_doc,
            (const unsigned char*)"</div>", 6);
    if (status)
        goto failed;

    root = pchtml_html_document_parse_fragment_chunk_end(html_doc);

failed:
    return root;
}

static void
dom_append_subtree_to_element(pcdom_element_t *element,
        pcdom_node_t *subtree)
{
    pcdom_node_t *parent = pcdom_interface_node(element);

    if (subtree && subtree->first_child) {
        pcdom_node_t *div;
        div = subtree->first_child;

        while (div->first_child) {
            pcdom_node_t *child = div->first_child;
            pcdom_node_remove(child);
            pcdom_node_append_child(parent, child);
        }
    }

    if (subtree)
        pcdom_node_destroy_deep(subtree);
}

static void
dom_prepend_subtree_to_element(pcdom_element_t *element,
        pcdom_node_t *subtree)
{
    pcdom_node_t *parent = pcdom_interface_node(element);

    if (subtree && subtree->first_child) {
        pcdom_node_t *div;
        div = subtree->first_child;

        while (div->last_child) {
            pcdom_node_t *child = div->last_child;
            pcdom_node_remove(child);
            pcdom_node_prepend_child(parent, child);
        }
    }

    if (subtree)
        pcdom_node_destroy_deep(subtree);
}

static void
dom_insert_subtree_before_element(pcdom_element_t *element,
        pcdom_node_t *subtree)
{
    pcdom_node_t *to = pcdom_interface_node(element);

    if (subtree && subtree->first_child) {
        pcdom_node_t *div;
        div = subtree->first_child;

        while (div->last_child) {
            pcdom_node_t *child = div->last_child;
            pcdom_node_remove(child);
            pcdom_node_insert_before(to, child);
        }
    }

    if (subtree)
        pcdom_node_destroy_deep(subtree);
}

static void
dom_insert_subtree_after_element(pcdom_element_t *element,
        pcdom_node_t *subtree)
{
    pcdom_node_t *to = pcdom_interface_node(element);

    if (subtree && subtree->first_child) {
        pcdom_node_t *div;
        div = subtree->first_child;

        while (div->first_child) {
            pcdom_node_t *child = div->first_child;
            pcdom_node_remove(child);
            pcdom_node_insert_after(to, child);
        }
    }

    if (subtree)
        pcdom_node_destroy_deep(subtree);
}

static void
dom_displace_content_by_subtree(pcdom_element_t *element,
        pcdom_node_t *subtree)
{
    pcdom_node_t *parent = pcdom_interface_node(element);

    while (parent->first_child != NULL) {
        pcdom_node_destroy_deep(parent->first_child);
    }

    if (subtree && subtree->first_child) {
        pcdom_node_t *div;
        div = subtree->first_child;

        while (div->first_child) {
            pcdom_node_t *child = div->first_child;
            pcdom_node_remove(child);
            pcdom_node_append_child(parent, child);
        }
    }

    if (subtree)
        pcdom_node_destroy_deep(subtree);
}

typedef void (*dom_subtree_op)(pcdom_element_t *element,
        pcdom_node_t *subtree);

static const dom_subtree_op dom_subtree_ops[] = {
    dom_append_subtree_to_element,
    dom_prepend_subtree_to_element,
    dom_insert_subtree_before_element,
    dom_insert_subtree_after_element,
    dom_displace_content_by_subtree,
};

static pcdoc_node new_content(purc_document_t doc,
            pcdoc_element_t elem, pcdoc_operation op,
            const char *content, size_t length)
{
    pcdoc_node node;

    if (UNLIKELY(op >= PCA_TABLESIZE(dom_subtree_ops))) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        node.type = PCDOC_NODE_NULL;
        node.elem = NULL;
        goto done;
    }

    pcdom_document_t *dom_doc = pcdom_interface_document(doc->impl);
    pcdom_element_t *dom_elem = pcdom_interface_element(elem);
    pcdom_node_t *subtree = dom_parse_fragment(dom_doc, dom_elem,
            content, length ? length : strlen(content));

    if (subtree) {
        dom_subtree_ops[op](dom_elem, subtree);
    }
    else {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
    }

    node.type = PCDOC_NODE_ELEMENT;
    node.elem = (pcdoc_element_t)doc;

done:
    return node;
}

static inline bool
dom_set_element_attribute(pcdom_element_t *element,
        const char* name, const char* value, size_t length)
{
    pcdom_attr_t *attr;
    attr = pcdom_element_set_attribute(element,
            (const unsigned char*)name, strlen(name),
            (const unsigned char*)value, length);
    return attr ? true : false;
}

static inline bool
dom_remove_element_attr(pcdom_element_t *element, const char* name)
{
    bool retv = false;

    if (pcdom_element_remove_attribute(element,
                (const unsigned char*)name, strlen(name)) == PURC_ERROR_OK)
        retv = true;

    return retv;
}

static bool set_attribute(purc_document_t doc,
            pcdoc_element_t elem, pcdoc_operation op,
            const char *name, const char *val, size_t len)
{
    UNUSED_PARAM(doc);

    pcdom_element_t *dom_elem = pcdom_interface_element(elem);
    if (op == PCDOC_OP_ERASE) {
        return dom_remove_element_attr(dom_elem, name);
    }
    else if (op == PCDOC_OP_CLEAR) {
        return dom_set_element_attribute(dom_elem, name, "", 0);
    }
    else if (op == PCDOC_OP_DISPLACE) {
        return dom_set_element_attribute(dom_elem, name,
                val, len ? len : strlen(val));
    }
    else {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
    }

    return false;
}

static pcdoc_element_t special_elem(purc_document_t doc,
            pcdoc_special_elem which)
{
    pchtml_html_document_t *html_doc = (pchtml_html_document_t *)doc->impl;

    switch (which) {
    case PCDOC_SPECIAL_ELEM_ROOT:
        return (pcdoc_element_t)pchtml_doc_get_document(html_doc)->element;

    case PCDOC_SPECIAL_ELEM_HEAD:
        return (pcdoc_element_t)pchtml_doc_get_head(html_doc);

    case PCDOC_SPECIAL_ELEM_BODY:
        return (pcdoc_element_t)pchtml_doc_get_body(html_doc);
    }

    return NULL;
}

static pcdoc_element_t get_parent(purc_document_t doc, pcdoc_node node)
{
    UNUSED_PARAM(doc);

    assert(node.type != PCDOC_NODE_NULL && node.type != PCDOC_NODE_OTHERS);

    pcdom_node_t *dom_node = pcdom_interface_node(node.elem);

    assert(dom_node->parent->type == PCDOM_NODE_TYPE_ELEMENT);

    return (pcdoc_element_t)dom_node->parent;
}

static size_t children_count(purc_document_t doc,
        pcdoc_element_t elem, pcdoc_node_type type)
{
    UNUSED_PARAM(doc);

    size_t n = 0;
    pcdom_node_t *dom_node = pcdom_interface_node(elem);

    pcdom_node_t *child = dom_node->first_child;
    while (child) {
        // TODO: CDATA Section
        if (type == PCDOC_NODE_ANY ||
                (type == PCDOC_NODE_ELEMENT &&
                 child->type == PCDOM_NODE_TYPE_ELEMENT) ||
                (type == PCDOC_NODE_TEXT &&
                 child->type == PCDOM_NODE_TYPE_TEXT)) {
            n++;
        }

        child = child->next;
    }

    return n;
}

static inline pcdoc_node_type
node_type(pcdom_node_type_t type)
{
    switch (type) {
        case PCDOM_NODE_TYPE_ELEMENT:
            return PCDOC_NODE_ELEMENT;
        case PCDOM_NODE_TYPE_TEXT:
            return PCDOC_NODE_TEXT;
        case PCDOM_NODE_TYPE_CDATA_SECTION:
        case PCDOM_NODE_TYPE_COMMENT:
            break;
        default:
            assert(0);
            break;
    }

    return PCDOC_NODE_OTHERS;
}

static pcdoc_node get_child(purc_document_t doc,
            pcdoc_element_t elem, pcdoc_node_type type, size_t idx)
{
    UNUSED_PARAM(doc);

    pcdoc_node node;
    node.type = PCDOC_NODE_NULL;
    node.elem = (pcdoc_element_t)NULL;

    size_t i = 0;
    pcdom_node_t *dom_node = pcdom_interface_node(elem);

    pcdom_node_t *child = dom_node->first_child;
    while (child) {
        // TODO: CDATA Section
        if (type == PCDOC_NODE_ANY) {
            if (i == idx) {
                node.type = node_type(child->type);
                node.elem = (pcdoc_element_t)child;
                return node;
            }

            i++;
        }
        else if ((type == PCDOC_NODE_ELEMENT ||
                    type == PCDOC_NODE_TEXT) &&
                node_type(child->type) == type) {
            if (i == idx) {
                node.type = type;
                node.elem = (pcdoc_element_t)child;
                return node;
            }

            i++;
        }

        child = child->next;
    }

    return node;
}

static bool get_attribute(purc_document_t doc, pcdoc_element_t elem,
            const char *name, const char **val, size_t *len)
{
    UNUSED_PARAM(doc);

    pcdom_element_t *dom_elem = pcdom_interface_element(elem);
    pcdom_attr_t *attr = pcdom_element_first_attribute(dom_elem);

    while (attr) {
        const char *str;
        size_t sz;

        str = (const char *)pcdom_attr_local_name(attr, &sz);
        if (strcasecmp(name, str) == 0) {
            *val = (const char *)pcdom_attr_value(attr, &sz);
            if (len)
                *len = sz;
            return true;
        }

        attr = pcdom_element_next_attribute(attr);
    }

    return false;
}

static bool get_text(purc_document_t doc, pcdoc_text_node_t text_node,
            const char **text, size_t *len)
{
    UNUSED_PARAM(doc);

    pcdom_text_t *dom_text;
    dom_text = pcdom_interface_text(text_node);
    *text = (const char *)dom_text->char_data.data.data;
    if (len)
        *len = dom_text->char_data.data.length;

    return true;
}

struct purc_document_ops _pcdoc_html_ops = {
    .create = create,
    .destroy = destroy,
    .operate_element = operate_element,
    .new_text_content = new_text_content,
    .new_data_content = NULL,
    .new_content = new_content,
    .set_attribute = set_attribute,
    .special_elem = special_elem,
    .get_parent = get_parent,
    .children_count = children_count,
    .get_child = get_child,
    .get_attribute = get_attribute,
    .get_text = get_text,
    .get_data = NULL,
    .elem_coll_select = NULL,
    .elem_coll_filter = NULL,
};

