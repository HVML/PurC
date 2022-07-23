/**
 * @file document.c
 * @author Vincent Wei
 * @date 2022/07/11
 * @brief The implementation of target document.
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

#include "purc-document.h"
#include "purc-errors.h"

#include "private/document.h"
#include "private/stringbuilder.h"

static struct doc_type {
    const char                 *target_name;
    struct purc_document_ops   *ops;
} doc_types[] = {
    { PCDOC_TYPE_VOID,    &_pcdoc_void_ops },
    { PCDOC_TYPE_PLAIN,   NULL /* &_pcdoc_plain_ops */ },
    { PCDOC_TYPE_HTML,    &_pcdoc_html_ops },
    { PCDOC_TYPE_XML,     NULL /* &_pcdoc_xml_ops */ },
    { PCDOC_TYPE_XGML,    NULL /* &_pcdoc_xgml_ops */ },
};

/* Make sure the size of doc_types matches the number of document types */
#define _COMPILE_TIME_ASSERT(name, x)               \
       typedef int _dummy_ ## name[(x) * 2 - 1]

_COMPILE_TIME_ASSERT(types,
        PCA_TABLESIZE(doc_types) == PCDOC_NR_TYPES);

#undef _COMPILE_TIME_ASSERT

purc_document_type
purc_document_retrieve_type(const char *target_name)
{
    if (UNLIKELY(target_name == NULL))
        goto fallback;

    for (size_t i = 0; i < PCA_TABLESIZE(doc_types); i++) {
        if (strcmp(target_name, doc_types[i].target_name) == 0) {
            if (doc_types[i].ops)
                return PCDOC_K_TYPE_FIRST + i;
            break;
        }
    }

fallback:
    return PCDOC_K_TYPE_VOID;   // fallback
}

purc_document_t
purc_document_new(purc_document_type type)
{
    struct purc_document_ops *ops = doc_types[type].ops;
    if (ops == NULL) {
        purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
        return NULL;
    }

    return ops->create(NULL, 0);
}


purc_document_t
purc_document_load(purc_document_type type, const char *content, size_t len)
{
    struct purc_document_ops *ops = doc_types[type].ops;
    if (ops == NULL) {
        PC_WARN("document type %d is not implemented\n", type);
        purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
        return NULL;
    }

    return ops->create(content, len);
}

unsigned int
purc_document_get_refc(purc_document_t doc)
{
    return doc->refc;
}

purc_document_t
purc_document_ref(purc_document_t doc)
{
    doc->refc++;
    return doc;
}

unsigned int
purc_document_unref(purc_document_t doc)
{
    doc->refc--;

    unsigned int refc = doc->refc;
    if (refc == 0) {
        doc->ops->destroy(doc);
    }

    return refc;
}

unsigned int
purc_document_delete(purc_document_t doc)
{
    unsigned int refc = doc->refc;
    doc->ops->destroy(doc);
    return refc;
}

pcdoc_element_t
purc_document_special_elem(purc_document_t doc, pcdoc_special_elem elem)
{
    return doc->ops->special_elem(doc, elem);
}

pcdoc_element_t
pcdoc_element_new_element(purc_document_t doc,
        pcdoc_element_t elem, pcdoc_operation op,
        const char *tag, bool self_close)
{
    return doc->ops->operate_element(doc, elem, op, tag, self_close);
}

void
pcdoc_element_clear(purc_document_t doc, pcdoc_element_t elem)
{
    doc->ops->operate_element(doc, elem, PCDOC_OP_CLEAR, NULL, 0);
}

void
pcdoc_element_erase(purc_document_t doc, pcdoc_element_t elem)
{
    doc->ops->operate_element(doc, elem, PCDOC_OP_ERASE, NULL, 0);
}

pcdoc_text_node_t
pcdoc_element_new_text_content(purc_document_t doc,
        pcdoc_element_t elem, pcdoc_operation op,
        const char *text, size_t len)
{
    return doc->ops->new_text_content(doc, elem, op, text, len);
}

pcdoc_data_node_t
pcdoc_element_set_data_content(purc_document_t doc,
        pcdoc_element_t elem, pcdoc_operation op,
        purc_variant_t data)
{
    if (doc->ops->new_data_content)
        return doc->ops->new_data_content(doc, elem, op, data);

    purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    return NULL;
}

pcdoc_node
pcdoc_element_new_content(purc_document_t doc,
        pcdoc_element_t elem, pcdoc_operation op,
        const char *content, size_t len)
{
    return doc->ops->new_content(doc, elem, op, content, len);
}

int
pcdoc_element_set_attribute(purc_document_t doc,
        pcdoc_element_t elem, pcdoc_operation op,
        const char *name, const char *val, size_t len)
{
    if (doc->ops->set_attribute) {
        return doc->ops->set_attribute(doc, elem, op, name, val, len);
    }

    return 0;
}

int
pcdoc_element_get_attribute(purc_document_t doc, pcdoc_element_t elem,
        const char *name, const char **val, size_t *len)
{
    if (doc->ops->get_attribute) {
        return doc->ops->get_attribute(doc, elem, name, val, len);
    }

    *val = "";
    if (len) *len = 0;
    return 0;
}

int
pcdoc_element_get_special_attr(purc_document_t doc, pcdoc_element_t elem,
        pcdoc_special_attr which, const char **val, size_t *len)
{
    if (doc->ops->get_special_attr) {
        return doc->ops->get_special_attr(doc, elem, which, val, len);
    }

    *val = "";
    if (len) *len = 0;
    return 0;
}

struct class_token_info {
    const char *klass;
    size_t      length;

    bool        found;
};

static int
class_token_finder(const char *token, const char *end, void *ud)
{
    struct class_token_info *info = (struct class_token_info *)ud;

    if ((size_t)(end - token) != info->length)
        return 0;

    if (strncmp(token, info->klass, info->length))
        return 0;

    info->found = true;
    return 1;
}

int
pcdoc_element_has_class(purc_document_t doc, pcdoc_element_t elem,
        const char *klass, bool *found)
{
    const char *s;
    size_t len;
    s = pcdoc_element_class(doc, elem, &len);

    if (s && len > 0) {
        struct class_token_info info = {
            .klass      = klass,
            .length     = strlen(klass),
            .found      = false,
        };

        pcutils_token_by_delim(s, s + len, ' ',
                &info, class_token_finder);
        *found = info.found;
        return 0;
    }

    return -1;
}

int
pcdoc_text_content_get_text(purc_document_t doc, pcdoc_text_node_t text_node,
        const char **text, size_t *len)
{
    if (doc->ops->get_text) {
        return doc->ops->get_text(doc, text_node, text, len);
    }

    *text = "";
    if (len) *len = 0;
    return 0;
}

int
pcdoc_data_content_get_data(purc_document_t doc, pcdoc_data_node_t data_node,
        purc_variant_t *data)
{
    if (doc->ops->get_data) {
        return doc->ops->get_data(doc, data_node, data);
    }

    *data = PURC_VARIANT_INVALID;
    return -1;
}

int
pcdoc_element_children_count(purc_document_t doc, pcdoc_element_t elem,
        size_t *nr_elements, size_t *nr_text_nodes, size_t *nr_data_nodes)
{
    size_t nrs[PCDOC_NODE_OTHERS + 1] = { };

    if (doc->ops->children_count) {
        if (doc->ops->children_count(doc, elem, nrs) == 0) {
            if (nr_elements)
                *nr_elements = nrs[PCDOC_NODE_ELEMENT];
            if (nr_text_nodes)
                *nr_text_nodes = nrs[PCDOC_NODE_TEXT];
            if (nr_data_nodes)
                *nr_data_nodes = nrs[PCDOC_NODE_DATA];
        }
        else {
            return -1;
        }
    }

    if (nr_elements)
        *nr_elements = 0;
    if (nr_text_nodes)
        *nr_text_nodes = 0;
    if (nr_data_nodes)
        *nr_data_nodes = 0;

    return 0;
}

pcdoc_element_t
pcdoc_element_get_child_element(purc_document_t doc, pcdoc_element_t elem,
        size_t idx)
{
    if (doc->ops->children_count) {
        pcdoc_node node;
        node = doc->ops->get_child(doc, elem, PCDOC_NODE_ELEMENT, idx);
        if (node.type == PCDOC_NODE_ELEMENT)
            return node.elem;
    }

    return NULL;
}

pcdoc_text_node_t
pcdoc_element_get_child_text_node(purc_document_t doc, pcdoc_element_t elem,
        size_t idx)
{
    if (doc->ops->children_count) {
        pcdoc_node node;
        node = doc->ops->get_child(doc, elem, PCDOC_NODE_TEXT, idx);
        if (node.type == PCDOC_NODE_TEXT)
            return node.text_node;
    }

    return NULL;
}

pcdoc_data_node_t
pcdoc_element_get_child_data_node(purc_document_t doc, pcdoc_element_t elem,
        size_t idx)
{
    if (doc->ops->children_count) {
        pcdoc_node node;
        node = doc->ops->get_child(doc, elem, PCDOC_NODE_DATA, idx);
        if (node.type == PCDOC_NODE_DATA)
            return node.data_node;
    }

    return NULL;
}

pcdoc_element_t
pcdoc_node_get_parent(purc_document_t doc, pcdoc_node node)
{
    return doc->ops->get_parent(doc, node);
}

int
pcdoc_travel_descendant_elements(purc_document_t doc,
        pcdoc_element_t ancestor, pcdoc_element_cb cb, void *ctxt, size_t *n)
{
    if (doc->ops->travel) {
        if (ancestor == NULL)
            ancestor = doc->ops->special_elem(doc, PCDOC_SPECIAL_ELEM_ROOT);

        struct pcdoc_travel_info info = { PCDOC_NODE_ELEMENT, 0, ctxt };
        int r = doc->ops->travel(doc, ancestor, (pcdoc_node_cb)cb, &info);
        if (n)
            *n = info.nr;
        return r;
    }

    if (n)
        *n = 0;
    return 0;
}

int
pcdoc_travel_descendant_text_nodes(purc_document_t doc,
        pcdoc_element_t ancestor, pcdoc_text_node_cb cb, void *ctxt, size_t *n)
{
    if (doc->ops->travel) {
        if (ancestor == NULL)
            ancestor = doc->ops->special_elem(doc, PCDOC_SPECIAL_ELEM_ROOT);

        struct pcdoc_travel_info info = { PCDOC_NODE_TEXT, 0, ctxt };
        int r = doc->ops->travel(doc, ancestor, (pcdoc_node_cb)cb, &info);
        if (n)
            *n = info.nr;
        return r;
    }

    if (n)
        *n = 0;
    return 0;
}

int
pcdoc_travel_descendant_data_nodes(purc_document_t doc,
        pcdoc_element_t ancestor, pcdoc_data_node_cb cb, void *ctxt, size_t *n)
{
    if (doc->ops->travel) {
        if (ancestor == NULL)
            ancestor = doc->ops->special_elem(doc, PCDOC_SPECIAL_ELEM_ROOT);

        struct pcdoc_travel_info info = { PCDOC_NODE_DATA, 0, ctxt };
        int r = doc->ops->travel(doc, ancestor, (pcdoc_node_cb)cb, &info);
        if (n)
            *n = info.nr;
        return r;
    }

    if (n)
        *n = 0;
    return 0;
}

struct serialize_info {
    unsigned        opts;
    purc_rwstream_t stm;
};

static int serialize_text_node(purc_document_t doc,
        pcdoc_text_node_t text_node, void *ctxt)
{
    struct serialize_info *info = ctxt;

    const char *text;
    size_t len;
    int r = pcdoc_text_content_get_text(doc, text_node, &text, &len);
    if (r)
        return r;

    if (purc_rwstream_write(info->stm, text, len) < 0)
        return -1;
    return 0;
}

int
pcdoc_serialize_text_contents_to_stream(purc_document_t doc,
        pcdoc_element_t ancestor, unsigned opts, purc_rwstream_t out)
{
    if (doc->ops->travel) {
        if (ancestor == NULL)
            ancestor = doc->ops->special_elem(doc, PCDOC_SPECIAL_ELEM_ROOT);

        struct serialize_info info = { opts, out };
        struct pcdoc_travel_info travel_info = { PCDOC_NODE_TEXT, 0, &info };
        int r = doc->ops->travel(doc, ancestor,
                (pcdoc_node_cb)serialize_text_node, &travel_info);
        return r;
    }

    return 0;
}

int
pcdoc_serialize_descendants_to_stream(purc_document_t doc,
        pcdoc_element_t ancestor, unsigned opts, purc_rwstream_t out)
{
    if (doc->ops->serialize) {
        pcdoc_node node = { PCDOC_NODE_ELEMENT };
        node.elem = ancestor;
        return doc->ops->serialize(doc, node, opts, out);
    }

    return 0;
}

int
purc_document_serialize_contents_to_stream(purc_document_t doc,
        unsigned opts, purc_rwstream_t out)
{
    if (doc->ops->serialize) {
        pcdoc_node node;
        node.type = PCDOC_NODE_OTHERS;
        node.others = doc->impl;
        return doc->ops->serialize(doc, node, opts, out);
    }

    return 0;
}

pcdoc_element_t
pcdoc_find_element_in_descendants(purc_document_t doc,
        pcdoc_element_t ancestor, const char *selector)
{
    pcdoc_element_t found;

    if (doc->ops->find_elem) {
        if (ancestor == NULL)
            ancestor = doc->ops->special_elem(doc, PCDOC_SPECIAL_ELEM_ROOT);

        found = doc->ops->find_elem(doc, ancestor, selector);
    }
    else {
        found = NULL;
    }

    return found;
}

static pcdoc_elem_coll_t
element_collection_new(const char *selector)
{
    pcdoc_elem_coll_t coll = calloc(1, sizeof(*coll));
    coll->selector = selector ? strdup(selector) : NULL;
    coll->refc = 1;
    coll->elems = pcutils_arrlist_new_ex(NULL, 4);

    return coll;
}

pcdoc_elem_coll_t
pcdoc_elem_coll_new_from_descendants(purc_document_t doc,
        pcdoc_element_t ancestor, const char *selector)
{
    pcdoc_elem_coll_t coll = element_collection_new(selector);

    if (doc->ops->elem_coll_select) {
        if (ancestor == NULL) {
            ancestor = doc->ops->special_elem(doc,
                    PCDOC_SPECIAL_ELEM_ROOT);
        }

        if (!doc->ops->elem_coll_select(doc, coll, ancestor, selector)) {
            pcdoc_elem_coll_delete(doc, coll);
            coll = NULL;
        }
    }

    return coll;
}

pcdoc_elem_coll_t
pcdoc_elem_coll_filter(purc_document_t doc,
        pcdoc_elem_coll_t elem_coll, const char *selector)
{
    pcdoc_elem_coll_t dst_coll = element_collection_new(selector);

    if (doc->ops->elem_coll_filter) {
        if (!doc->ops->elem_coll_filter(doc, dst_coll,
                elem_coll, selector)) {
            pcdoc_elem_coll_delete(doc, dst_coll);
            dst_coll = NULL;
        }
    }

    return dst_coll;
}

void
pcdoc_elem_coll_delete(purc_document_t doc,
        pcdoc_elem_coll_t elem_coll)
{
    UNUSED_PARAM(doc);

    pcutils_arrlist_free(elem_coll->elems);
    return free(elem_coll);
}

#if 0
static void
element_collection_delete(pcdoc_elem_coll_t coll)
{
    pcutils_arrlist_free(coll->elems);
    return free(coll);
}

static pcdoc_elem_coll_t
element_collection_ref(purc_document_t doc, pcdoc_elem_coll_t coll)
{
    UNUSED_PARAM(doc);

    coll->refc++;
    return coll;
}

static void
element_collection_unref(purc_document_t doc, pcdoc_elem_coll_t coll)
{
    UNUSED_PARAM(doc);

    if (coll->refc <= 1) {
        element_collection_delete(coll);
    }
    else {
        coll->refc--;
    }
}
#endif

