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
#include "csseng/csseng.h"

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

purc_document_type_k
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
purc_document_new(purc_document_type_k type)
{
    struct purc_document_ops *ops = doc_types[type].ops;
    if (ops == NULL) {
        purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
        return NULL;
    }

    purc_document_t doc = ops->create(NULL, 0);
    if (doc) {
        doc->refc = 1;
        doc->expc = 0;
        list_head_init(&doc->owner_list);
    }

    return doc;
}


purc_document_t
purc_document_load(purc_document_type_k type, const char *content, size_t len)
{
    struct purc_document_ops *ops = doc_types[type].ops;
    if (ops == NULL) {
        PC_WARN("document type %d is not implemented\n", type);
        purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
        return NULL;
    }

    purc_document_t doc = ops->create(content, len);
    if (doc) {
        doc->refc = 1;
        doc->expc = 0;
        list_head_init(&doc->owner_list);
    }

    return doc;
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

void *
purc_document_impl_entity(purc_document_t doc, purc_document_type_k *type)
{
    if (type)
        *type = doc->type;
    return doc->impl;
}

unsigned int
purc_document_delete(purc_document_t doc)
{
    unsigned int refc = doc->refc;
    doc->ops->destroy(doc);
    return refc;
}

pcdoc_element_t
purc_document_special_elem(purc_document_t doc, pcdoc_special_elem_k elem)
{
    return doc->ops->special_elem(doc, elem);
}

pcdoc_element_t
pcdoc_element_new_element(purc_document_t doc,
        pcdoc_element_t elem, pcdoc_operation_k op,
        const char *tag, bool self_close)
{
    doc->age++;
    return doc->ops->operate_element(doc, elem, op, tag, self_close);
}

void
pcdoc_element_clear(purc_document_t doc, pcdoc_element_t elem)
{
    doc->age++;
    doc->ops->operate_element(doc, elem, PCDOC_OP_CLEAR, NULL, 0);
}

void
pcdoc_element_erase(purc_document_t doc, pcdoc_element_t elem)
{
    doc->age++;
    doc->ops->operate_element(doc, elem, PCDOC_OP_ERASE, NULL, 0);
}

pcdoc_text_node_t
pcdoc_element_new_text_content(purc_document_t doc,
        pcdoc_element_t elem, pcdoc_operation_k op,
        const char *text, size_t len)
{
    doc->age++;
    return doc->ops->new_text_content(doc, elem, op, text, len);
}

pcdoc_data_node_t
pcdoc_element_set_data_content(purc_document_t doc,
        pcdoc_element_t elem, pcdoc_operation_k op,
        purc_variant_t data)
{
    doc->age++;
    if (doc->ops->new_data_content)
        return doc->ops->new_data_content(doc, elem, op, data);

    purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    return NULL;
}

pcdoc_node
pcdoc_element_new_content(purc_document_t doc,
        pcdoc_element_t elem, pcdoc_operation_k op,
        const char *content, size_t len)
{
    doc->age++;
    return doc->ops->new_content(doc, elem, op, content, len);
}

int
pcdoc_element_get_tag_name(purc_document_t doc, pcdoc_element_t elem,
        const char **local_name, size_t *local_len,
        const char **prefix, size_t *prefix_len,
        const char **ns_name, size_t *ns_len)
{
    assert(doc->ops->get_tag_name);

    return doc->ops->get_tag_name(doc, elem,
            local_name, local_len, prefix, prefix_len, ns_name, ns_len);
}

int
pcdoc_element_set_attribute(purc_document_t doc,
        pcdoc_element_t elem, pcdoc_operation_k op,
        const char *name, const char *val, size_t len)
{
    doc->age++;
    if (doc->ops->set_attribute) {
        return doc->ops->set_attribute(doc, elem, op, name, val, len);
    }

    return 0;
}

int
pcdoc_element_get_attribute(purc_document_t doc, pcdoc_element_t elem,
        const char *name, const char **val, size_t *len)
{
    // must be a valid attribute name (without space characters)
    if (!purc_is_valid_identifier(name))
        return -1;

    if (doc->ops->get_attribute) {
        return doc->ops->get_attribute(doc, elem, name, val, len);
    }

    *val = "";
    if (len) *len = 0;
    return 0;
}

int
pcdoc_element_get_special_attr(purc_document_t doc, pcdoc_element_t elem,
        pcdoc_special_attr_k which, const char **val, size_t *len)
{
    if (doc->ops->get_special_attr) {
        return doc->ops->get_special_attr(doc, elem, which, val, len);
    }

    *val = "";
    if (len) *len = 0;
    return 0;
}

#define CLASS_SEPARATOR " \f\n\r\t\v"

int
pcdoc_element_has_class(purc_document_t doc, pcdoc_element_t elem,
        const char *klass, bool *found)
{
    const char *value;
    size_t len;

    // must be a valid attribute name (without space characters)
    if (!purc_is_valid_identifier(klass))
        return -1;

    *found = false;
    value = pcdoc_element_class(doc, elem, &len);
    if (value == NULL) {
        return 0;
    }

    char *haystack = strndup(value, len);

    char *str;
    char *saveptr;
    char *token;
    for (str = haystack; ; str = NULL) {
        token = strtok_r(str, CLASS_SEPARATOR, &saveptr);
        /* to match class name caseinsensitively */
        if (token) {
            if (strcasecmp(token, klass) == 0) {
                *found = true;
                break;
            }
        }
        else
            break;
    }

    free(haystack);
    return 0;
}

int
pcdoc_element_travel_attributes(purc_document_t doc,
        pcdoc_element_t element, pcdoc_attribute_cb cb, void *ctxt, size_t *n)
{
    int ret = 0;

    if (n)
        *n = 0;

    if (doc->ops->travel_attrs) {
        struct pcdoc_travel_attrs_info info = { 0, ctxt };
        ret = doc->ops->travel_attrs(doc, element, cb, &info);
        if (ret == 0 && n)
            *n = info.nr;
    }

    return ret;
}

pcdoc_attr_t
pcdoc_element_first_attr(purc_document_t doc, pcdoc_element_t elem)
{
    if (doc->ops->first_attr) {
        return doc->ops->first_attr(doc, elem);
    }

    return NULL;
}

pcdoc_attr_t
pcdoc_element_last_attr(purc_document_t doc, pcdoc_element_t elem)
{
    if (doc->ops->last_attr) {
        return doc->ops->last_attr(doc, elem);
    }

    return NULL;
}

pcdoc_attr_t
pcdoc_attr_next_sibling(purc_document_t doc, pcdoc_attr_t attr)
{
    if (doc->ops->next_attr) {
        return doc->ops->next_attr(doc, attr);
    }

    return NULL;
}

pcdoc_attr_t
pcdoc_attr_prev_sibling(purc_document_t doc, pcdoc_attr_t attr)
{
    if (doc->ops->prev_attr) {
        return doc->ops->prev_attr(doc, attr);
    }

    return NULL;
}

int
pcdoc_attr_get_info(purc_document_t doc, pcdoc_attr_t attr,
        const char **local_name, size_t *local_len,
        const char **qualified_name, size_t *qualified_len,
        const char **value, size_t *value_len)
{
    if (doc->ops->get_attr_info) {
        return doc->ops->get_attr_info(doc, attr,
                local_name, local_len,
                qualified_name, qualified_len,
                value, value_len);
    }

    return -1;
}

int
pcdoc_node_get_user_data(purc_document_t doc, pcdoc_node node,
        void **user_data)
{
    if (doc->ops->get_user_data) {
        return doc->ops->get_user_data(doc, node, user_data);
    }

    return -1;
}

int
pcdoc_node_set_user_data(purc_document_t doc, pcdoc_node node,
        void *user_data)
{
    if (doc->ops->set_user_data) {
        return doc->ops->set_user_data(doc, node, user_data);
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
    int ret = 0;
    size_t nrs[PCDOC_NODE_OTHERS + 1] = { };

    if (nr_elements)
        *nr_elements = 0;
    if (nr_text_nodes)
        *nr_text_nodes = 0;
    if (nr_data_nodes)
        *nr_data_nodes = 0;

    if (doc->ops->children_count) {
        ret = doc->ops->children_count(doc, elem, nrs);
        if (ret == 0) {
            if (nr_elements)
                *nr_elements = nrs[PCDOC_NODE_ELEMENT];
            if (nr_text_nodes)
                *nr_text_nodes = nrs[PCDOC_NODE_TEXT];
            if (nr_data_nodes)
                *nr_data_nodes = nrs[PCDOC_NODE_DATA];
        }
    }

    return ret;
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

pcdoc_node
pcdoc_element_first_child(purc_document_t doc, pcdoc_element_t elem)
{
    if (doc->ops->first_child)
        return doc->ops->first_child(doc, elem);

    pcdoc_node first = { PCDOC_NODE_VOID, { NULL } };
    return first;
}

pcdoc_node
pcdoc_element_last_child(purc_document_t doc, pcdoc_element_t elem)
{
    if (doc->ops->last_child)
        return doc->ops->last_child(doc, elem);

    pcdoc_node last = { PCDOC_NODE_VOID, { NULL } };
    return last;
}

pcdoc_node
pcdoc_node_next_sibling(purc_document_t doc, pcdoc_node node)
{
    if (doc->ops->next_sibling && node.type != PCDOC_NODE_VOID)
        return doc->ops->next_sibling(doc, node);

    pcdoc_node next = { PCDOC_NODE_VOID, { NULL } };
    return next;
}

pcdoc_node
pcdoc_node_prev_sibling(purc_document_t doc, pcdoc_node node)
{
    if (doc->ops->prev_sibling && node.type != PCDOC_NODE_VOID)
        return doc->ops->prev_sibling(doc, node);

    pcdoc_node prev = { PCDOC_NODE_VOID, { NULL } };
    return prev;
}

int
pcdoc_travel_descendant_elements(purc_document_t doc,
        pcdoc_element_t ancestor, pcdoc_element_cb cb, void *ctxt, size_t *n)
{
    if (doc->ops->travel) {
        if (ancestor == NULL)
            ancestor = doc->ops->special_elem(doc, PCDOC_SPECIAL_ELEM_ROOT);

        struct pcdoc_travel_info info = { PCDOC_NODE_ELEMENT, true, 0, ctxt };
        doc->ops->travel(doc, ancestor, (pcdoc_node_cb)cb, &info);
        if (n)
            *n = info.nr;
        return info.all ? 0 : -1;
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

        struct pcdoc_travel_info info = { PCDOC_NODE_TEXT, true, 0, ctxt };
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

        struct pcdoc_travel_info info = { PCDOC_NODE_DATA, true, 0, ctxt };
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
        struct pcdoc_travel_info travel_info = {
            PCDOC_NODE_TEXT, true, 0, &info };
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
        pcdoc_node node = { };
        node.type = PCDOC_NODE_ELEMENT;
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
        pcdoc_node node = { };
        node.type = PCDOC_NODE_OTHERS;
        node.others = doc->impl;
        return doc->ops->serialize(doc, node, opts, out);
    }

    return 0;
}

static inline void
selector_delete(pcdoc_selector_t selector)
{
    if (selector) {
        if (selector->selector) {
            css_element_selector_destroy(selector->selector);
        }
        if (selector->id) {
            free(selector->id);
        }
        free(selector);
    }
}

static pcdoc_selector_t
pcdoc_selector_ref(pcdoc_selector_t selector)
{
    selector->refc++;
    return selector;
}

static void
pcdoc_selector_unref(pcdoc_selector_t selector)
{
    if (selector->refc <= 1) {
        selector_delete(selector);
    }
    else {
        selector->refc--;
    }
}

pcdoc_selector_t
pcdoc_selector_new(const char *selector)
{
    pcdoc_selector_t ret = NULL;
    if (!selector) {
        goto out;
    }

    ret = (pcdoc_selector_t) calloc(1, sizeof(*ret));
    if (!ret) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }

    if (selector[0] == '#') {
        ret->id = strdup(selector);
    }
    else {
        css_error err = css_element_selector_create(selector, &ret->selector);
        if (err != CSS_OK) {
            goto out_clear_ret;
        }
    }


    ret->refc = 1;
    goto out;

out_clear_ret:
    free(ret);
    ret = NULL;

out:
    return ret;
}

int
pcdoc_selector_delete(pcdoc_selector_t selector)
{
    pcdoc_selector_unref(selector);
    return 0;
}

struct travel_elem_id {
    pcdoc_element_t elem;
    const char     *id;
};

static int
travel_elem_id_cb(purc_document_t doc, pcdoc_element_t element, void *ctxt)
{
    struct travel_elem_id *args = (struct travel_elem_id*)ctxt;
    size_t len;
    const char *s = pcdoc_element_id(doc, element, &len);

    if (s && (s[len] == '\0') && (strncmp(s, args->id, len) == 0)) {
        args->elem = element;
        return PCDOC_TRAVEL_STOP;
    }

    return PCDOC_TRAVEL_GOON;
}

pcdoc_element_t
pcdoc_get_element_by_id_in_descendants(purc_document_t doc,
        pcdoc_element_t ancestor, const char *id)
{
    pcdoc_element_t ret;
    if (ancestor == NULL) {
        ancestor = doc->ops->special_elem(doc, PCDOC_SPECIAL_ELEM_ROOT);
    }

    if (doc->ops->get_elem_by_id) {
        ret = doc->ops->get_elem_by_id(doc, ancestor, id);
        goto out;
    }

    struct travel_elem_id data = {
        .elem = NULL,
        .id = id
    };

    pcdoc_travel_descendant_elements(doc, ancestor, travel_elem_id_cb,
            &data, NULL);
    ret = data.elem;

out:
    return ret;
}

extern css_select_handler purc_document_css_select_handler;

struct travel_find_elem {
    pcdoc_element_t  elem;
    pcdoc_selector_t selector;
};

static int
travel_find_elem_cb(purc_document_t doc, pcdoc_element_t element, void *ctxt)
{
    UNUSED_PARAM(doc);
    UNUSED_PARAM(element);
    UNUSED_PARAM(ctxt);
    struct travel_find_elem *args = (struct travel_find_elem*)ctxt;
    bool match = false;

    css_element_selector_match(args->selector->selector, element,
            &purc_document_css_select_handler, doc, &match);

    if (match) {
        args->elem = element;
        return PCDOC_TRAVEL_STOP;
    }

    return PCDOC_TRAVEL_GOON;
}

pcdoc_element_t
pcdoc_find_element_in_descendants(purc_document_t doc,
        pcdoc_element_t ancestor, pcdoc_selector_t selector)
{
    pcdoc_element_t ret;
    if (ancestor == NULL) {
        ancestor = doc->ops->special_elem(doc, PCDOC_SPECIAL_ELEM_ROOT);
    }

    if (doc->ops->find_elem) {
        ret = doc->ops->find_elem(doc, ancestor, selector);
        goto out;
    }

    if (selector->id) {
        ret = pcdoc_get_element_by_id_in_descendants(doc, ancestor,
                selector->id + 1);
        goto out;
    }

    struct travel_find_elem data = {
        .selector = selector,
        .elem = NULL
    };

    doc->root4select = ancestor;
    pcdoc_travel_descendant_elements(doc, ancestor, travel_find_elem_cb,
            &data, NULL);
    doc->root4select = NULL;
    ret = data.elem;

out:
    return ret;
}

static pcdoc_elem_coll_t
element_collection_new(purc_document_t doc, pcdoc_element_t ancestor,
        pcdoc_selector_t selector, pcdoc_elem_coll_type type)
{
    pcdoc_elem_coll_t coll = calloc(1, sizeof(*coll));
    coll->type = type;
    coll->ancestor = ancestor;
    coll->doc = purc_document_ref(doc);
    coll->selector = selector ? pcdoc_selector_ref(selector) : NULL;
    coll->refc = 1;
    coll->select_size = -1;
    coll->elems = pcutils_arrlist_new_ex(NULL, 4);

    return coll;
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
        if (coll->selector) {
            pcdoc_selector_unref(coll->selector);
        }

        if (coll->parent) {
            element_collection_unref(doc, coll->parent);
        }

        if (coll->doc) {
            purc_document_unref(doc);
        }

        pcutils_arrlist_free(coll->elems);
        free(coll);
    }
    else {
        coll->refc--;
    }
}

static int
travel_select_elem_cb(purc_document_t doc, pcdoc_element_t element, void *ctxt)
{
    pcdoc_elem_coll_t coll = (pcdoc_elem_coll_t)ctxt;
    bool match = false;

    css_element_selector_match(coll->selector->selector, element,
            &purc_document_css_select_handler, doc, &match);

    if (match) {
        pcutils_arrlist_append(coll->elems, element);
        coll->nr_elems++;
    }
    return PCDOC_TRAVEL_GOON;
}

pcdoc_elem_coll_t
pcdoc_elem_coll_new_from_descendants(purc_document_t doc,
        pcdoc_element_t ancestor, pcdoc_selector_t selector)
{
    pcdoc_elem_coll_t coll = element_collection_new(doc, ancestor, selector,
            PCDOC_ELEM_COLL_TYPE_DOC_QUERY);
    if (!coll) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }

    coll->doc_age = doc->age;
    if (ancestor == NULL) {
        ancestor = doc->ops->special_elem(doc,
                PCDOC_SPECIAL_ELEM_ROOT);
    }

    if (selector->id) {
        coll->type = PCDOC_ELEM_COLL_TYPE_DOC_SELECT;
        pcdoc_element_t elem  = pcdoc_get_element_by_id_in_descendants(doc,
                ancestor, selector->id + 1);
        if (elem) {
            pcutils_arrlist_append(coll->elems, elem);
            coll->nr_elems++;
            coll->select_begin = 0;
        }
        goto out;
    }

    doc->root4select = ancestor;
    pcdoc_travel_descendant_elements(doc, ancestor, travel_select_elem_cb,
            coll, NULL);
    doc->root4select = NULL;
out:
    return coll;
}

pcdoc_elem_coll_t
pcdoc_elem_coll_select(purc_document_t doc,
        pcdoc_elem_coll_t elem_coll, pcdoc_selector_t selector)
{
    pcdoc_element_t ancestor = NULL;
    pcdoc_elem_coll_t coll = element_collection_new(doc, ancestor, selector,
            PCDOC_ELEM_COLL_TYPE_COLL_SELECT);
    if (!coll) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }

    if (ancestor == NULL) {
        ancestor = doc->ops->special_elem(doc,
                PCDOC_SPECIAL_ELEM_ROOT);
    }

    if (doc->ops->elem_coll_select) {
        if (!doc->ops->elem_coll_select(doc, elem_coll, ancestor, selector)) {
            pcdoc_elem_coll_delete(doc, coll);
            coll = NULL;
        }
        goto out;
    }
    coll->doc_age = elem_coll->doc_age;

    size_t nr_elems = elem_coll->nr_elems;
    for (size_t i = 0; i < nr_elems; i++) {
        pcdoc_element_t elem = pcdoc_elem_coll_get(doc, elem_coll, i);
        doc->root4select = elem;
        pcdoc_travel_descendant_elements(doc, elem, travel_select_elem_cb,
            coll, NULL);
        doc->root4select = NULL;
    }

out:
    return coll;
}

pcdoc_elem_coll_t
pcdoc_elem_coll_filter(purc_document_t doc,
        pcdoc_elem_coll_t elem_coll, pcdoc_selector_t selector)
{
    pcdoc_elem_coll_t dst_coll = element_collection_new(doc, elem_coll->ancestor,
            selector, PCDOC_ELEM_COLL_TYPE_COLL_FILTER);

    if (doc->ops->elem_coll_filter) {
        if (!doc->ops->elem_coll_filter(doc, dst_coll, elem_coll, selector)) {
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
    element_collection_unref(doc, elem_coll);
}

ssize_t
pcdoc_elem_coll_count(purc_document_t doc,
        pcdoc_elem_coll_t elem_coll)
{
    UNUSED_PARAM(doc);
    return elem_coll ? elem_coll->nr_elems : 0;
}

pcdoc_element_t
pcdoc_elem_coll_get(purc_document_t doc,
    pcdoc_elem_coll_t elem_coll, size_t idx)
{
    UNUSED_PARAM(doc);
    pcdoc_element_t elem = NULL;
    if (!elem_coll || idx >= elem_coll->nr_elems) {
        goto out;
    }

    elem = pcutils_arrlist_get_idx(elem_coll->elems, idx);
out:
    return elem;
}

pcdoc_elem_coll_t
pcdoc_elem_coll_sub(purc_document_t doc,
        pcdoc_elem_coll_t elem_coll, int offset, size_t length)
{
    UNUSED_PARAM(doc);
    pcdoc_elem_coll_t coll = NULL;
    if (!elem_coll || (size_t)offset >= elem_coll->nr_elems ||
            length > (elem_coll->nr_elems - offset)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    coll = element_collection_new(doc, elem_coll->ancestor, elem_coll->selector,
            PCDOC_ELEM_COLL_TYPE_COLL_SUB);
    if (!coll) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }
    coll->parent = elem_coll;
    element_collection_ref(doc, elem_coll);

    coll->select_begin = offset;
    coll->select_size = length;

    size_t end = offset + length;
    end = end > elem_coll->nr_elems ? elem_coll->nr_elems : end;

    for (size_t i = offset; i < end; i++) {
        void *v = pcutils_arrlist_get_idx(elem_coll->elems, i);
        pcutils_arrlist_append(coll->elems, v);
    }

    coll->nr_elems = pcutils_arrlist_length(coll->elems);

out:
    return coll;
}

int
pcdoc_elem_coll_travel(purc_document_t doc, pcdoc_elem_coll_t elem_coll,
        pcdoc_element_cb cb, void *ctxt, size_t *n)
{
    int ret = -1;
    if (!elem_coll || !cb) {
        goto out;
    }

    size_t i;
    pcdoc_element_t elem;
    for (i = 0; i < elem_coll->nr_elems; i++) {
        elem = pcutils_arrlist_get_idx(elem_coll->elems, i);
        if (cb(doc, elem, ctxt)) {
            break;
        }
    }

    if (n) {
        *n = i;
    }

    ret = i == elem_coll->nr_elems ? 0 : -1;
out:
    return ret;
}

int
elem_coll_update_query(pcdoc_elem_coll_t elem_coll)
{
    int ret = -1;

    if (!elem_coll->selector) {
        ret = 0;
        goto out;
    }

    pcdoc_elem_coll_t new_coll = pcdoc_elem_coll_new_from_descendants(
            elem_coll->doc, elem_coll->ancestor, elem_coll->selector);
    if (!new_coll) {
        goto out;
    }

    struct pcutils_arrlist *elems = elem_coll->elems;
    elem_coll->elems = new_coll->elems;
    new_coll->elems = elems;

    elem_coll->nr_elems = new_coll->nr_elems;
    elem_coll->doc_age = elem_coll->doc->age;
    pcdoc_elem_coll_delete(new_coll->doc, new_coll);
    ret = 0;

out:
    return ret;
}

int
elem_coll_update_sub(pcdoc_elem_coll_t elem_coll)
{
    int ret = -1;

    pcdoc_elem_coll_t parent_coll = elem_coll->parent;
    if (!parent_coll) {
        goto out;
    }

    pcutils_arrlist_free(elem_coll->elems);
    elem_coll->elems = pcutils_arrlist_new_ex(NULL, 4);
    if (!elem_coll->elems) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }

    size_t offset = elem_coll->select_begin;
    size_t end = parent_coll->nr_elems;
    if (elem_coll->select_size != (size_t)-1) {
        end = offset + elem_coll->select_size;
        end = end > parent_coll->nr_elems ? parent_coll->nr_elems : end;
    }

    for (size_t i = offset; i < end; i++) {
        void *v = pcutils_arrlist_get_idx(parent_coll->elems, i);
        pcutils_arrlist_append(elem_coll->elems, v);
    }

    elem_coll->nr_elems = pcutils_arrlist_length(elem_coll->elems);
    elem_coll->doc_age = elem_coll->doc->age;

    ret = 0;

out:
    return ret;
}

int
elem_coll_update_select(pcdoc_elem_coll_t elem_coll)
{
    int ret = -1;

    pcdoc_elem_coll_t parent_coll = elem_coll->parent;
    if (!parent_coll) {
        goto out;
    }

    pcutils_arrlist_free(elem_coll->elems);
    elem_coll->elems = pcutils_arrlist_new_ex(NULL, 4);
    if (!elem_coll->elems) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }

    elem_coll->doc_age = parent_coll->doc_age;
    elem_coll->nr_elems = 0;

    purc_document_t doc = parent_coll->doc;
    size_t nr_elems = parent_coll->nr_elems;
    for (size_t i = 0; i < nr_elems; i++) {
        pcdoc_element_t elem = pcdoc_elem_coll_get(doc, parent_coll, i);
        doc->root4select = elem;
        pcdoc_travel_descendant_elements(doc, elem, travel_select_elem_cb,
            elem_coll, NULL);
        doc->root4select = NULL;
    }

    ret = 0;

out:
    return ret;
}

int
pcdoc_elem_coll_update(pcdoc_elem_coll_t elem_coll)
{
    int ret = -1;
    if (!elem_coll) {
        goto out;
    }

    if (elem_coll->parent) {
        pcdoc_elem_coll_update(elem_coll->parent);
    }

    switch (elem_coll->type) {
    case PCDOC_ELEM_COLL_TYPE_DOC_QUERY:
        ret = elem_coll_update_query(elem_coll);
        break;

    case PCDOC_ELEM_COLL_TYPE_COLL_SUB:
        ret = elem_coll_update_sub(elem_coll);
        break;

    case PCDOC_ELEM_COLL_TYPE_COLL_SELECT:
        ret = elem_coll_update_select(elem_coll);
        break;

    case PCDOC_ELEM_COLL_TYPE_DOC_SELECT:
    case PCDOC_ELEM_COLL_TYPE_COLL_FILTER:
    case PCDOC_ELEM_COLL_TYPE_FROM_ELEM:
    default:
        ret = 0;
        break;
    }

out:
    return ret;
}

pcdoc_elem_coll_t
pcdoc_elem_coll_new_from_element(purc_document_t doc, pcdoc_element_t elem)
{
    pcdoc_elem_coll_t coll = element_collection_new(doc, NULL, NULL,
            PCDOC_ELEM_COLL_TYPE_FROM_ELEM);
    if (!coll) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }

    coll->doc_age = doc->age;
    coll->nr_elems = 1;
    pcutils_arrlist_append(coll->elems, elem);

out:
    return coll;
}

