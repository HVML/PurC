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
#include "private/debug.h"

#include "ns_const.h"

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
        PC_WARN("bad content\n");
    }

    purc_document_t doc = calloc(1, sizeof(*doc));
    doc->type = PCDOC_K_TYPE_HTML;
    doc->def_text_type = PCRDR_MSG_DATA_TYPE_HTML;
    doc->need_rdr = 1;
    doc->data_content = 0;
    doc->have_head = 1;
    doc->have_body = 1;

    doc->refc = 1;

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
            (const unsigned char*)tag, strlen(tag), NULL, self_close);
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
        PC_DEBUG("invalid op: %d\n", op);
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
        node.type = PCDOC_NODE_VOID;
        node.elem = NULL;
        goto done;
    }

    pcdom_document_t *dom_doc = pcdom_interface_document(doc->impl);
    pcdom_element_t *dom_elem = pcdom_interface_element(elem);
    pcdom_node_t *subtree = dom_parse_fragment(dom_doc, dom_elem,
            content, length ? length : strlen(content));

    pcdom_node_t *dom_node = subtree->first_child->first_child;

    if (subtree) {
        dom_subtree_ops[op](dom_elem, subtree);
    }
    else {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
    }

    node.type = PCDOC_NODE_ELEMENT;
    node.elem = (pcdoc_element_t)dom_node;

done:
    return node;
}

static inline int
dom_set_element_attribute(pcdom_element_t *element,
        const char* name, const char* value, size_t length)
{
    pcdom_attr_t *attr;
    attr = pcdom_element_set_attribute(element,
            (const unsigned char*)name, strlen(name),
            (const unsigned char*)value, length);
    return attr ? 0 : -1;
}

static inline int
dom_remove_element_attr(pcdom_element_t *element, const char* name)
{
    int retv = -1;

    if (pcdom_element_remove_attribute(element,
                (const unsigned char*)name, strlen(name)) == PURC_ERROR_OK)
        retv = 0;

    return retv;
}

static int set_attribute(purc_document_t doc,
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

    return -1;
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

static int
get_tag_name(purc_document_t doc, pcdoc_element_t elem,
        const char **local_name, size_t *local_len,
        const char **prefix, size_t *prefix_len,
        const char **ns_name, size_t *ns_len)
{
    UNUSED_PARAM(doc);

    pcdom_element_t *dom_elem = pcdom_interface_element(elem);
    *local_name = (const char *)pcdom_element_local_name(dom_elem, local_len);

    if (prefix)
        *prefix = (const char *)pcdom_element_prefix(dom_elem, prefix_len);

    if (ns_name) {
        switch (dom_elem->node.ns) {
        case PCHTML_NS_HTML:
        default:
            *ns_name = PCDOC_NSNAME_HTML;
            if (ns_len) *ns_len = sizeof(PCDOC_NSNAME_HTML) - 1;
            break;

        case PCHTML_NS_MATH:
            *ns_name = PCDOC_NSNAME_MATHML;
            if (ns_len) *ns_len = sizeof(PCDOC_NSNAME_MATHML) - 1;
            break;

        case PCHTML_NS_SVG:
            *ns_name = PCDOC_NSNAME_SVG;
            if (ns_len) *ns_len = sizeof(PCDOC_NSNAME_SVG) - 1;
            break;

        case PCHTML_NS_XLINK:
            *ns_name = PCDOC_NSNAME_XLINK;
            if (ns_len) *ns_len = sizeof(PCDOC_NSNAME_XLINK) - 1;
            break;

        case PCHTML_NS_XML:
            *ns_name = PCDOC_NSNAME_XLINK;
            if (ns_len) *ns_len = sizeof(PCDOC_NSNAME_XLINK) - 1;
            break;

        case PCHTML_NS_XMLNS:
            *ns_name = PCDOC_NSNAME_XMLNS;
            if (ns_len) *ns_len = sizeof(PCDOC_NSNAME_XMLNS) - 1;
            break;
        }
    }

    return 0;
}

static pcdoc_element_t get_parent(purc_document_t doc, pcdoc_node node)
{
    UNUSED_PARAM(doc);

    assert(node.type != PCDOC_NODE_VOID && node.type != PCDOC_NODE_OTHERS);

    pcdom_node_t *dom_node = pcdom_interface_node(node.elem);

    assert(dom_node->parent->type == PCDOM_NODE_TYPE_ELEMENT);

    return (pcdoc_element_t)dom_node->parent;
}

static pcdoc_node first_child(purc_document_t doc, pcdoc_element_t elem)
{
    UNUSED_PARAM(doc);

    pcdoc_node first = { PCDOC_NODE_VOID, { NULL } };

    pcdom_node_t *dom_node = pcdom_interface_node(elem);
    pcdom_node_t *child = dom_node->first_child;
    if (child) {
        if (child->type == PCDOM_NODE_TYPE_ELEMENT) {
            first.type = PCDOC_NODE_ELEMENT;
        }
        else if (child->type == PCDOM_NODE_TYPE_TEXT) {
            first.type = PCDOC_NODE_TEXT;
        }
        else if (child->type == PCDOM_NODE_TYPE_CDATA_SECTION) {
            first.type = PCDOC_NODE_CDATA_SECTION;
        }
        else {
            first.type = PCDOC_NODE_OTHERS;
        }

        first.data = child;
    }

    return first;
}

static pcdoc_node last_child(purc_document_t doc, pcdoc_element_t elem)
{
    UNUSED_PARAM(doc);

    pcdoc_node last = { PCDOC_NODE_VOID, { NULL } };

    pcdom_node_t *dom_node = pcdom_interface_node(elem);
    pcdom_node_t *child = dom_node->last_child;
    if (child) {
        if (child->type == PCDOM_NODE_TYPE_ELEMENT) {
            last.type = PCDOC_NODE_ELEMENT;
        }
        else if (child->type == PCDOM_NODE_TYPE_TEXT) {
            last.type = PCDOC_NODE_TEXT;
        }
        else if (child->type == PCDOM_NODE_TYPE_CDATA_SECTION) {
            last.type = PCDOC_NODE_CDATA_SECTION;
        }
        else {
            last.type = PCDOC_NODE_OTHERS;
        }

        last.data = child;
    }

    return last;
}

static pcdoc_node next_sibling(purc_document_t doc, pcdoc_node node)
{
    UNUSED_PARAM(doc);

    pcdoc_node next = { PCDOC_NODE_VOID, { NULL } };

    pcdom_node_t *dom_node = pcdom_interface_node(node.data);
    pcdom_node_t *sibling = dom_node->next;
    if (sibling) {
        if (sibling->type == PCDOM_NODE_TYPE_ELEMENT) {
            next.type = PCDOC_NODE_ELEMENT;
        }
        else if (sibling->type == PCDOM_NODE_TYPE_TEXT) {
            next.type = PCDOC_NODE_TEXT;
        }
        else if (sibling->type == PCDOM_NODE_TYPE_CDATA_SECTION) {
            next.type = PCDOC_NODE_CDATA_SECTION;
        }
        else {
            next.type = PCDOC_NODE_OTHERS;
        }

        next.data = sibling;
    }

    return next;
}

static pcdoc_node prev_sibling(purc_document_t doc, pcdoc_node node)
{
    UNUSED_PARAM(doc);

    pcdoc_node prev = { PCDOC_NODE_VOID, { NULL } };

    pcdom_node_t *dom_node = pcdom_interface_node(node.data);
    pcdom_node_t *sibling = dom_node->prev;
    if (sibling) {
        if (sibling->type == PCDOM_NODE_TYPE_ELEMENT) {
            prev.type = PCDOC_NODE_ELEMENT;
        }
        else if (sibling->type == PCDOM_NODE_TYPE_TEXT) {
            prev.type = PCDOC_NODE_TEXT;
        }
        else if (sibling->type == PCDOM_NODE_TYPE_CDATA_SECTION) {
            prev.type = PCDOC_NODE_CDATA_SECTION;
        }
        else {
            prev.type = PCDOC_NODE_OTHERS;
        }

        prev.data = sibling;
    }

    return prev;
}

static int children_count(purc_document_t doc, pcdoc_element_t elem,
        size_t *nrs)
{
    UNUSED_PARAM(doc);

    pcdom_node_t *dom_node = pcdom_interface_node(elem);
    pcdom_node_t *child = dom_node->first_child;
    while (child) {
        if (child->type == PCDOM_NODE_TYPE_ELEMENT) {
            nrs[PCDOC_NODE_ELEMENT]++;
        }
        else if (child->type == PCDOM_NODE_TYPE_TEXT) {
            nrs[PCDOC_NODE_TEXT]++;
        }
        else if (child->type == PCDOM_NODE_TYPE_CDATA_SECTION) {
            nrs[PCDOC_NODE_CDATA_SECTION]++;
        }
        else {
            nrs[PCDOC_NODE_OTHERS]++;
        }

        child = child->next;
    }

    return 0;
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
            return PCDOC_NODE_CDATA_SECTION;
        case PCDOM_NODE_TYPE_COMMENT:
        default:
            break;
    }

    return PCDOC_NODE_OTHERS;
}

static pcdoc_node
get_child(purc_document_t doc,
            pcdoc_element_t elem, pcdoc_node_type type, size_t idx)
{
    UNUSED_PARAM(doc);

    pcdoc_node node;
    node.type = PCDOC_NODE_VOID;
    node.elem = NULL;

    size_t i = 0;
    pcdom_node_t *dom_node = pcdom_interface_node(elem);
    pcdom_node_t *child = dom_node->first_child;
    while (child) {
        if (node_type(child->type) == type) {
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

static int get_attribute(purc_document_t doc, pcdoc_element_t elem,
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
            return 0;
        }

        attr = pcdom_element_next_attribute(attr);
    }

    return -1;
}

static int get_special_attr(purc_document_t doc, pcdoc_element_t elem,
            pcdoc_special_attr which, const char **val, size_t *len)
{
    UNUSED_PARAM(doc);
    pcdom_element_t *dom_elem = pcdom_interface_element(elem);

    if (which == PCDOC_ATTR_ID) {
        if (dom_elem->attr_id == NULL) {
            return -1;
        }

        size_t sz;
        *val = (const char *)pcdom_attr_value(dom_elem->attr_id, &sz);
        if (len)
            *len = sz;
    }
    else if (which == PCDOC_ATTR_CLASS) {
        if (dom_elem->attr_class == NULL) {
            return -1;
        }

        size_t sz;
        *val = (const char *)pcdom_attr_value(dom_elem->attr_class, &sz);
        if (len)
            *len = sz;
    }

    return 0;
}

static int get_text(purc_document_t doc, pcdoc_text_node_t text_node,
            const char **text, size_t *len)
{
    UNUSED_PARAM(doc);

    pcdom_text_t *dom_text;
    dom_text = pcdom_interface_text(text_node);
    *text = (const char *)dom_text->char_data.data.data;
    if (len)
        *len = dom_text->char_data.data.length;

    return 0;
}

static int
travel(purc_document_t doc, pcdoc_element_t ancestor,
            pcdoc_node_cb cb, struct pcdoc_travel_info *info)
{
    pcdom_node_t *ancestor_node = (pcdom_node_t *)ancestor;
    if (info->type == node_type(ancestor_node->type)) {
        int r = cb(doc, ancestor, info->ctxt);
        if (r)
            return -1;
        info->nr++;
    }

    pcdom_node_t *dom_node = pcdom_interface_node(ancestor);

    pcdom_node_t *child = dom_node->first_child;
    for (; child; child = child->next) {
        if (child->type == PCDOM_NODE_TYPE_ELEMENT) {
            pcdoc_element_t elem = (pcdoc_element_t)child;
            int r = travel(doc, elem, cb, info);
            if (r)
                return -1;
        }
        else if (node_type(child->type) == info->type) {
            int r = cb(doc, child, info->ctxt);
            if (r)
                return -1;
            info->nr++;
        }
    }

    return 0;
}

static int serialize(purc_document_t doc, pcdoc_node node,
            unsigned opts, purc_rwstream_t stm)
{
    if (node.type == PCDOC_NODE_OTHERS)
        return pchtml_doc_write_to_stream_ex(doc->impl, opts, stm);
    else {
        pcdom_node_t *dom_node = pcdom_interface_node(node.elem);
        return pcdom_node_write_to_stream_ex(dom_node, opts, stm);
    }
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
    .get_tag_name = get_tag_name,
    .get_parent = get_parent,
    .first_child = first_child,
    .last_child = last_child,
    .next_sibling = next_sibling,
    .prev_sibling = prev_sibling,
    .children_count = children_count,
    .get_child = get_child,
    .get_attribute = get_attribute,
    .get_special_attr = get_special_attr,
    .get_text = get_text,
    .get_data = NULL,
    .travel = travel,
    .serialize = serialize,
    .elem_coll_select = NULL,
    .elem_coll_filter = NULL,
};

