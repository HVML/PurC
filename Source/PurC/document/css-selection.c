/*
** @file css-selection.c
** @author Vincent Wei
** @date 2022/10/08
** @brief The implementation of CSS selection handlers.
**
** Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of purc, which is an HVML interpreter with
** a command line interface (CLI).
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "purc-document.h"

#include "csseng/csseng.h"
#include "purc/purc-dom.h"
#include "private/document.h"

#include <assert.h>
#include <string.h>

/******************************************************************************
 * Style selection callbacks                                                  *
 ******************************************************************************/

#define TAG_NAME_TEXT               "__TEXT"
#define TAG_NAME_FOREIGN            "__FOREIGN"
#define TAG_NAME_DISINTERESTED      "__DISINTERESTED"

#define PX_PER_EM                   10

#define cast_to_pcdoc_element_t(node) ((pcdoc_element_t)(node))

static int
doc_get_element_lang(purc_document_t doc, pcdoc_element_t ele,
        const char **lang, size_t *len)
{
    int ret = pcdoc_element_get_attribute(doc, ele, "lang", lang, len);
    if (ret == 0)
        return 0;

    pcdoc_node node;
    node.type = PCDOC_NODE_ELEMENT;
    node.elem = ele;
    pcdoc_element_t parent = pcdoc_node_get_parent(doc, node);
    if (parent) {
        return doc_get_element_lang(doc, parent, lang, len);
    }

    return -1;
}

/**
 * Callback to retrieve a node's name.
 *
 * \param pw     HTML document
 * \param node   DOM node
 * \param qname  Pointer to location to receive node name
 * \return CSS_OK on success,
 *         CSS_NOMEM on memory exhaustion.
 */
static css_error
node_name(void *pw, void *n, css_qname *qname)
{
    purc_document_t doc = (purc_document_t)pw;
    pcdoc_element_t ele = n;

    const char *name = NULL;
    size_t len;
    pcdoc_element_get_tag_name(doc, ele,
            &name, &len, NULL, NULL, NULL, NULL);

    if (name &&
            lwc_intern_string(name, len, &qname->name) == lwc_error_oom) {
        qname->name = NULL;
        return CSS_NOMEM;
    }

    return CSS_OK;
}

#define CLASS_SEPARATOR " \f\n\r\t\v"

/**
 * Callback to retrieve a node's classes.
 *
 * \param pw         HTML document
 * \param node       DOM node
 * \param classes    Pointer to location to receive class name array
 * \param n_classes  Pointer to location to receive length of class name array
 * \return CSS_OK on success,
 *         CSS_NOMEM on memory exhaustion.
 *
 * \note The returned array will be destroyed by libcss. Therefore, it must
 *       be allocated using the same allocator as used by libcss during style
 *       selection.
 */
static css_error
node_classes(void *pw, void *n, lwc_string ***classes, uint32_t *n_classes)
{
    purc_document_t doc = (purc_document_t)pw;
    pcdoc_element_t ele = n;

    lwc_string **my_classes = NULL;
    uint32_t nr_classes = 0;
    char *cloned = NULL;

    const char *value;
    size_t len;
    value = pcdoc_element_class(doc, ele, &len);
    if (value == NULL) {
        goto done;
    }

    cloned = strndup(value, len);
    if (cloned == NULL)
        goto failed;

    char *str;
    char *saveptr;
    char *klass;
    for (str = cloned; ; str = NULL) {
        klass = strtok_r(str, CLASS_SEPARATOR, &saveptr);
        if (klass) {
            nr_classes++;
            my_classes = (lwc_string **)realloc(my_classes,
                    sizeof(lwc_string *) * nr_classes);
            if (my_classes == NULL)
                goto failed;

            lwc_error ec =
                lwc_intern_string(klass, strlen(klass),
                        my_classes + nr_classes - 1);
            if (ec == lwc_error_oom) {
                nr_classes--;
                break;
            }
        }
        else {
            break;
        }
    }

done:
    if (cloned)
        free(cloned);

    *classes = my_classes;
    *n_classes = nr_classes;
    return CSS_OK;

failed:
    if (cloned)
        free(cloned);

    if (my_classes && nr_classes > 0) {
        for (uint32_t i = 0; i < nr_classes; i++) {
            lwc_string_destroy(my_classes[i]);
        }

        free(my_classes);
    }

    *classes = NULL;
    *n_classes = 0;
    return CSS_NOMEM;
}

/**
 * Callback to retrieve a node's ID.
 *
 * \param pw    HTML document
 * \param node  DOM node
 * \param id    Pointer to location to receive id value
 * \return CSS_OK on success,
 *         CSS_NOMEM on memory exhaustion.
 */
static css_error
node_id(void *pw, void *n, lwc_string **id)
{
    purc_document_t doc = (purc_document_t)pw;
    pcdoc_element_t ele = n;

    const char *value;
    size_t len;
    value = pcdoc_element_id(doc, ele, &len);
    if (value) {
        if (lwc_intern_string(value, len, id) == lwc_error_oom) {
            goto failed;
        }
    }
    else {
        *id = NULL;
    }

    return CSS_OK;

failed:
    *id = NULL;
    return CSS_NOMEM;
}

/**
 * Callback to find a named parent node
 *
 * \param pw      HTML document
 * \param node    DOM node
 * \param qname   Node name to search for
 * \param parent  Pointer to location to receive parent
 * \return CSS_OK.
 *
 * \post \a parent will contain the result, or NULL if there is no match
 */
static css_error
named_parent_node(void *pw, void *n, const css_qname *qname, void **_parent)
{
    purc_document_t doc = (purc_document_t)pw;

    *_parent = NULL;

    pcdoc_node node = { PCDOC_NODE_ELEMENT, { n } };
    pcdoc_element_t parent = pcdoc_node_get_parent(doc, node);
    if (parent && node.elem != doc->root4select) {
        const char *name1, *name2;
        size_t len1, len2;

        pcdoc_element_get_tag_name(doc, parent,
                &name1, &len1, NULL, NULL, NULL, NULL);

        name2 = lwc_string_data(qname->name);
        len2 = lwc_string_length(qname->name);

        if (len1 == len2 && strncasecmp(name1, name2, len1) == 0) {
            *_parent = (void *)parent;
            goto done;
        }
    }

done:
    return CSS_OK;
}

/**
 * Callback to find a named previous sibling node.
 *
 * \param pw       HTML document
 * \param node     DOM node
 * \param qname    Node name to search for
 * \param sibling  Pointer to location to receive sibling
 * \return CSS_OK.
 *
 * \post \a sibling will contain the result, or NULL if there is no match
 */
static css_error
named_sibling_node(void *pw, void *n, const css_qname *qname, void **sibling)
{
    purc_document_t doc = (purc_document_t)pw;

    *sibling = NULL;

    pcdoc_node node = { PCDOC_NODE_ELEMENT, { n } };
    if (node.elem == doc->root4select) {
        goto done;
    }

    pcdoc_node prev = pcdoc_node_prev_sibling(doc, node);
    while (prev.data) {
        if (prev.type == PCDOC_NODE_ELEMENT) {
            const char *name1, *name2;
            size_t len1, len2;

            pcdoc_element_get_tag_name(doc, prev.elem,
                    &name1, &len1, NULL, NULL, NULL, NULL);

            name2 = lwc_string_data(qname->name);
            len2 = lwc_string_length(qname->name);

            if (len1 == len2 && strncasecmp(name1, name2, len1) == 0) {
                *sibling = prev.data;
            }

            break;
        }

        prev = pcdoc_node_prev_sibling(doc, prev);
    }

done:
    return CSS_OK;
}

/**
 * Callback to find a named previous generic sibling node.
 *
 * \param pw       HTML document
 * \param node     DOM node
 * \param qname    Node name to search for
 * \param sibling  Pointer to location to receive mached sibling
 * \return CSS_OK.
 *
 * \post \a sibling will contain the result, or NULL if there is no match
 */
static css_error
named_generic_sibling_node(void *pw, void *n, const css_qname *qname,
        void **sibling)
{
    purc_document_t doc = (purc_document_t)pw;

    *sibling = NULL;

    pcdoc_node node = { PCDOC_NODE_ELEMENT, { n } };
    if (node.elem == doc->root4select) {
        goto done;
    }

    pcdoc_node prev = pcdoc_node_prev_sibling(doc, node);
    while (prev.data) {
        if (prev.type == PCDOC_NODE_ELEMENT) {
            const char *name1, *name2;
            size_t len1, len2;

            pcdoc_element_get_tag_name(doc, prev.elem,
                    &name1, &len1, NULL, NULL, NULL, NULL);

            name2 = lwc_string_data(qname->name);
            len2 = lwc_string_length(qname->name);

            if (len1 == len2 && strncasecmp(name1, name2, len1) == 0) {
                *sibling = prev.data;
                break;
            }

            if (prev.elem == doc->root4select) {
                break;
            }
        }

        prev = pcdoc_node_prev_sibling(doc, prev);
    }

done:
    return CSS_OK;
}

/**
 * Callback to retrieve the parent of a node.
 *
 * \param pw      HTML document
 * \param node    DOM node
 * \param parent  Pointer to location to receive parent
 * \return CSS_OK.
 *
 * \post \a parent will contain the result, or NULL if there is no match
 */
static css_error
parent_node(void *pw, void *n, void **_parent)
{
    purc_document_t doc = (purc_document_t)pw;

    pcdoc_node node = { PCDOC_NODE_ELEMENT, { n } };
    pcdoc_element_t parent = pcdoc_node_get_parent(doc, node);
    if (parent && (node.elem != doc->root4select)) {
        *_parent = (void *)parent;
    }
    else {
        *_parent = NULL;
    }

    return CSS_OK;
}

/**
 * Callback to retrieve the previous sibling of a node.
 *
 * \param pw       HTML document
 * \param node     DOM node
 * \param sibling  Pointer to location to receive sibling
 * \return CSS_OK.
 *
 * \post \a sibling will contain the result, or NULL if there is no match
 */
static css_error
sibling_node(void *pw, void *n, void **sibling)
{
    purc_document_t doc = (purc_document_t)pw;
    *sibling = NULL;

    pcdoc_node node = { PCDOC_NODE_ELEMENT, { n } };
    if (node.elem == doc->root4select) {
        goto done;
    }

    pcdoc_node prev = pcdoc_node_prev_sibling(doc, node);
    while (prev.data) {
        if (prev.type == PCDOC_NODE_ELEMENT) {
            *sibling = prev.data;
            break;
        }

        prev = pcdoc_node_prev_sibling(doc, prev);
    }

done:
    return CSS_OK;
}

/**
 * Callback to determine if a node has the given name.
 *
 * \param pw     HTML document
 * \param node   DOM node
 * \param qname  Name to match
 * \param match  Pointer to location to receive result
 * \return CSS_OK.
 *
 * \post \a match will contain true if the node matches and false otherwise.
 */
static css_error
node_has_name(void *pw, void *n, const css_qname *qname, bool *match)
{
    purc_document_t doc = (purc_document_t)pw;
    pcdoc_element_t ele = n;

    const char *name1 = NULL;
    size_t len1;
    pcdoc_element_get_tag_name(doc, ele,
            &name1, &len1, NULL, NULL, NULL, NULL);

    const char *name2 = lwc_string_data(qname->name);
    size_t len2 = lwc_string_length(qname->name);
    if (len1 == len2 && strncasecmp(name1, name2, len1) == 0) {
        *match = true;
    }
    else
        *match = false;

    return CSS_OK;
}

/**
 * Callback to determine if a node has the given class.
 *
 * \param pw     HTML document
 * \param node   DOM node
 * \param name   Name to match
 * \param match  Pointer to location to receive result
 * \return CSS_OK.
 *
 * \post \a match will contain true if the node matches and false otherwise.
 */
static css_error
node_has_class(void *pw, void *n, lwc_string *name, bool *match)
{
    purc_document_t doc = (purc_document_t)pw;
    pcdoc_element_t ele = n;
    *match = false;

    const char *str1;
    size_t len1;
    str1 = pcdoc_element_class(doc, ele, &len1);
    if (str1) {
        const char *str2 = lwc_string_data(name);
        size_t len2 = lwc_string_length(name);

        char *haystack = strndup(str1, len1);

        char *str;
        char *saveptr;
        char *klass;
        for (str = haystack; ; str = NULL) {
            klass = strtok_r(str, CLASS_SEPARATOR, &saveptr);
            if (klass == NULL) {
                break;
            }

            /* to match values caseinsensitively */
            if (klass && strncasecmp(klass, str2, len2) == 0) {
                *match = true;
                break;
            }
        }

        free(haystack);
    }

    return CSS_OK;
}

/**
 * Callback to determine if a node has the given id.
 *
 * \param pw     HTML document
 * \param node   DOM node
 * \param name   Name to match
 * \param match  Pointer to location to receive result
 * \return CSS_OK.
 *
 * \post \a match will contain true if the node matches and false otherwise.
 */
static css_error
node_has_id(void *pw, void *n, lwc_string *name, bool *match)
{
    purc_document_t doc = (purc_document_t)pw;
    pcdoc_element_t ele = n;

    *match = false;

    const char *str1;
    size_t len1;
    str1 = pcdoc_element_id(doc, ele, &len1);
    if (str1) {

        const char *str2 = lwc_string_data(name);
        size_t len2 = lwc_string_length(name);

        if (len1 == len2 && strncasecmp(str1, str2, len1) == 0) {
            *match = true;
        }
    }

    return CSS_OK;
}

/**
 * Callback to determine if a node has an attribute with the given name.
 *
 * \param pw     HTML document
 * \param node   DOM node
 * \param qname  Name to match
 * \param match  Pointer to location to receive result
 * \return CSS_OK on success,
 *         CSS_NOMEM on memory exhaustion.
 *
 * \post \a match will contain true if the node matches and false otherwise.
 */
static css_error
node_has_attribute(void *pw, void *n, const css_qname *qname, bool *match)
{
    purc_document_t doc = (purc_document_t)pw;
    pcdoc_element_t ele = n;

    *match = false;

    const char *name = lwc_string_data(qname->name);
    size_t name_len = lwc_string_length(qname->name);
    assert(name && name_len > 0);

    char *my_name = strndup(name, name_len);

    const char *value;
    int ret = pcdoc_element_get_attribute(doc, ele, my_name, &value, NULL);
    if (ret == 0 && value != NULL) {
        *match = true;
    }

    free(my_name);

    return CSS_OK;
}

struct attr_to_match {
    const char *name;
    size_t name_len;

    const char *value;
    size_t value_len;

    bool matched;
};

static int attr_equal_cb(purc_document_t doc, pcdoc_attr_t attr,
        const char *name, size_t name_len,
        const char *value, size_t value_len, void *ctxt)
{
    (void)doc;
    (void)attr;
    struct attr_to_match *to_match = ctxt;

    if (to_match->name_len == name_len &&
            strncasecmp(to_match->name, name, name_len) == 0) {

        /* to match values casesensitively */
        if (to_match->value_len == value_len &&
                strncmp(to_match->value, value, value_len) == 0) {
            to_match->matched = true;
        }

        return -1;  // stop travel
    }

    return 0;
}

/**
 * Callback to determine if a node has an attribute with given name and value.
 *
 * \param pw     HTML document
 * \param node   DOM node
 * \param qname  Name to match
 * \param value  Value to match
 * \param match  Pointer to location to receive result
 * \return CSS_OK on success,
 *         CSS_NOMEM on memory exhaustion.
 *
 * \post \a match will contain true if the node matches and false otherwise.
 */
static css_error
node_has_attribute_equal(void *pw, void *n, const css_qname *qname,
        lwc_string *value, bool *match)
{
    purc_document_t doc = (purc_document_t)pw;
    pcdoc_element_t ele = n;

    struct attr_to_match to_match;

    to_match.name = lwc_string_data(qname->name);
    to_match.name_len = lwc_string_length(qname->name);
    to_match.value = lwc_string_data(value);
    to_match.value_len = lwc_string_length(value);
    to_match.matched = false;

    pcdoc_element_travel_attributes(doc, ele,
            attr_equal_cb, &to_match, NULL);

    *match = to_match.matched;
    return CSS_OK;
}

static int attr_dashmatch_cb(purc_document_t doc, pcdoc_attr_t attr,
        const char *name, size_t name_len,
        const char *value, size_t value_len, void *ctxt)
{
    (void)doc;
    (void)attr;
    struct attr_to_match *to_match = ctxt;

    if (to_match->name_len == name_len &&
            strncasecmp(to_match->name, name, name_len) == 0) {

        /* to match values casesensitively */
        if (strncmp(value, to_match->value, value_len) == 0) {
            if (value_len == to_match->value_len) {
                to_match->matched = true;
            }
            else if (value_len > to_match->value_len &&
                    value[to_match->value_len] == '-') {
                to_match->matched = true;
            }
        }

        return -1;  // stop travel
    }

    return 0;
}

/**
 * Callback to determine if a node has an attribute with the given name whose
 * value dashmatches that given.
 *
 * \param pw     HTML document
 * \param node   DOM node
 * \param qname  Name to match
 * \param value  Value to match
 * \param match  Pointer to location to receive result
 * \return CSS_OK on success,
 *         CSS_NOMEM on memory exhaustion.
 *
 * \post \a match will contain true if the node matches and false otherwise.
 */
static css_error
node_has_attribute_dashmatch(void *pw, void *n, const css_qname *qname,
        lwc_string *value, bool *match)
{
    purc_document_t doc = (purc_document_t)pw;
    pcdoc_element_t ele = n;

    struct attr_to_match to_match;

    to_match.name = lwc_string_data(qname->name);
    to_match.name_len = lwc_string_length(qname->name);
    to_match.value = lwc_string_data(value);
    to_match.value_len = lwc_string_length(value);
    to_match.matched = false;

    pcdoc_element_travel_attributes(doc, ele,
            attr_dashmatch_cb, &to_match, NULL);

    *match = to_match.matched;
    return CSS_OK;
}

static int attr_includes_cb(purc_document_t doc, pcdoc_attr_t attr,
        const char *name, size_t name_len,
        const char *value, size_t value_len, void *ctxt)
{
    (void)doc;
    (void)attr;
    struct attr_to_match *to_match = ctxt;

    if (to_match->name_len == name_len &&
            strncasecmp(to_match->name, name, name_len) == 0) {

        char *haystack = strndup(value, value_len);

        char *str;
        char *saveptr;
        char *token;
        for (str = haystack; ; str = NULL) {
            token = strtok_r(str, CLASS_SEPARATOR, &saveptr);
            if (token == NULL)
                break;

            /* to match values casesensitively */
            if (token && strncasecmp(token, to_match->value,
                        to_match->value_len) == 0) {
                to_match->matched = true;
                break;
            }
        }

        free(haystack);
        return -1;  // stop travel
    }

    return 0;
}

/**
 * Callback to determine if a node has an attribute with the given name whose
 * value includes that given.
 *
 * \param pw     HTML document
 * \param node   DOM node
 * \param qname  Name to match
 * \param value  Value to match
 * \param match  Pointer to location to receive result
 * \return CSS_OK on success,
 *         CSS_NOMEM on memory exhaustion.
 *
 * \post \a match will contain true if the node matches and false otherwise.
 */
static css_error
node_has_attribute_includes(void *pw, void *n, const css_qname *qname,
        lwc_string *value, bool *match)
{
    purc_document_t doc = (purc_document_t)pw;
    pcdoc_element_t ele = n;

    struct attr_to_match to_match;

    to_match.name = lwc_string_data(qname->name);
    to_match.name_len = lwc_string_length(qname->name);
    to_match.value = lwc_string_data(value);
    to_match.value_len = lwc_string_length(value);
    to_match.matched = false;

    pcdoc_element_travel_attributes(doc, ele,
            attr_includes_cb, &to_match, NULL);

    *match = to_match.matched;
    return CSS_OK;
}

static int attr_prefix_cb(purc_document_t doc, pcdoc_attr_t attr,
        const char *name, size_t name_len,
        const char *value, size_t value_len, void *ctxt)
{
    (void)doc;
    (void)attr;
    struct attr_to_match *to_match = ctxt;

    if (to_match->name_len == name_len &&
            strncasecmp(to_match->name, name, name_len) == 0) {

        /* to match values casesensitively */
        if (value_len >= to_match->value_len &&
                strncmp(value, to_match->value, to_match->value_len) == 0) {
            to_match->matched = true;
        }

        return -1;  // stop travel
    }

    return 0;
}

/**
 * Callback to determine if a node has an attribute with the given name whose
 * value has the prefix given.
 *
 * \param pw     HTML document
 * \param node   DOM node
 * \param qname  Name to match
 * \param value  Value to match
 * \param match  Pointer to location to receive result
 * \return CSS_OK on success,
 *         CSS_NOMEM on memory exhaustion.
 *
 * \post \a match will contain true if the node matches and false otherwise.
 */
static css_error
node_has_attribute_prefix(void *pw, void *n, const css_qname *qname,
        lwc_string *value, bool *match)
{
    purc_document_t doc = (purc_document_t)pw;
    pcdoc_element_t ele = n;

    struct attr_to_match to_match;

    to_match.name = lwc_string_data(qname->name);
    to_match.name_len = lwc_string_length(qname->name);
    to_match.value = lwc_string_data(value);
    to_match.value_len = lwc_string_length(value);
    to_match.matched = false;

    pcdoc_element_travel_attributes(doc, ele,
            attr_prefix_cb, &to_match, NULL);

    *match = to_match.matched;
    return CSS_OK;
}

static int attr_suffix_cb(purc_document_t doc, pcdoc_attr_t attr,
        const char *name, size_t name_len,
        const char *value, size_t value_len, void *ctxt)
{
    (void)doc;
    (void)attr;
    struct attr_to_match *to_match = ctxt;

    if (to_match->name_len == name_len &&
            strncasecmp(to_match->name, name, name_len) == 0) {

        /* to match values casesensitively */
        if (value_len >= to_match->value_len) {
            value += value_len;
            value -= to_match->value_len;
            if (strncmp(value, to_match->value, to_match->value_len) == 0) {
                to_match->matched = true;
            }
        }

        return -1;  // stop travel
    }

    return 0;
}

/**
 * Callback to determine if a node has an attribute with the given name whose
 * value has the suffix given.
 *
 * \param pw     HTML document
 * \param node   DOM node
 * \param qname  Name to match
 * \param value  Value to match
 * \param match  Pointer to location to receive result
 * \return CSS_OK on success,
 *         CSS_NOMEM on memory exhaustion.
 *
 * \post \a match will contain true if the node matches and false otherwise.
 */
static css_error
node_has_attribute_suffix(void *pw, void *n, const css_qname *qname,
        lwc_string *value, bool *match)
{
    purc_document_t doc = (purc_document_t)pw;
    pcdoc_element_t ele = n;

    struct attr_to_match to_match;

    to_match.name = lwc_string_data(qname->name);
    to_match.name_len = lwc_string_length(qname->name);
    to_match.value = lwc_string_data(value);
    to_match.value_len = lwc_string_length(value);
    to_match.matched = false;

    pcdoc_element_travel_attributes(doc, ele,
            attr_suffix_cb, &to_match, NULL);

    *match = to_match.matched;
    return CSS_OK;
}

static int attr_substring_cb(purc_document_t doc, pcdoc_attr_t attr,
        const char *name, size_t name_len,
        const char *value, size_t value_len, void *ctxt)
{
    (void)doc;
    (void)attr;
    struct attr_to_match *to_match = ctxt;

    if (to_match->name_len == name_len &&
            strncasecmp(to_match->name, name, name_len) == 0) {

        char *haystack = strndup(value, value_len);
        char *needle = strndup(to_match->value, to_match->value_len);

        /* to match values casesensitively */
        if (strstr(haystack, needle)) {
            to_match->matched = true;
        }

        free(haystack);
        free(needle);

        return -1;  // stop travel
    }

    return 0;
}

/**
 * Callback to determine if a node has an attribute with the given name whose
 * value contains the substring given.
 *
 * \param pw     HTML document
 * \param node   DOM node
 * \param qname  Name to match
 * \param value  Value to match
 * \param match  Pointer to location to receive result
 * \return CSS_OK on success,
 *         CSS_NOMEM on memory exhaustion.
 *
 * \post \a match will contain true if the node matches and false otherwise.
 */
static css_error
node_has_attribute_substring(void *pw, void *n, const css_qname *qname,
        lwc_string *value, bool *match)
{
    purc_document_t doc = (purc_document_t)pw;
    pcdoc_element_t ele = n;

    struct attr_to_match to_match;

    to_match.name = lwc_string_data(qname->name);
    to_match.name_len = lwc_string_length(qname->name);
    to_match.value = lwc_string_data(value);
    to_match.value_len = lwc_string_length(value);
    to_match.matched = false;

    pcdoc_element_travel_attributes(doc, ele,
            attr_substring_cb, &to_match, NULL);

    *match = to_match.matched;
    return CSS_OK;
}

/**
 * Callback to determine if a node is the root node of the document.
 *
 * \param pw     HTML document
 * \param node   DOM node
 * \param match  Pointer to location to receive result
 * \return CSS_OK.
 *
 * \post \a match will contain true if the node matches and false otherwise.
 */
static css_error
node_is_root(void *pw, void *n, bool *match)
{
    purc_document_t doc = (purc_document_t)pw;
    pcdoc_node node = { PCDOC_NODE_ELEMENT, { n } };

    if (pcdoc_node_get_parent(doc, node) == NULL ||
            node.elem == doc->root4select) {
        *match = true;
    }
    else {
        *match = false;
    }

    return CSS_OK;
}

/**
 * Callback to count a node's siblings.
 *
 * \param pw         HTML document
 * \param n          DOM node
 * \param same_name  Only count siblings with the same name, or all
 * \param after      Count anteceding instead of preceding siblings
 * \param count      Pointer to location to receive result
 * \return CSS_OK.
 *
 * \post \a count will contain the number of siblings
 */
static css_error
node_count_siblings(void *pw, void *n, bool same_name, bool after,
        int32_t *count)
{
    purc_document_t doc = (purc_document_t)pw;
    pcdoc_node node = { PCDOC_NODE_ELEMENT, { n } };

    int32_t my_count = 0;

    const char *name1 = NULL;
    size_t len1 = 0;
    if (same_name) {
        pcdoc_element_get_tag_name(doc, node.elem, &name1, &len1,
                NULL, NULL, NULL, NULL);
    }

    if (after) {
        pcdoc_node next = pcdoc_node_next_sibling(doc, node);

        while (next.data) {
            if (next.type == PCDOC_NODE_ELEMENT) {
                if (name1) {
                    const char *name2;
                    size_t len2;

                    pcdoc_element_get_tag_name(doc, next.elem, &name2, &len2,
                            NULL, NULL, NULL, NULL);
                    if (len1 == len2 && strncasecmp(name1, name2, len1) == 0)
                        my_count++;
                }
                else {
                    my_count++;
                }
            }

            next = pcdoc_node_next_sibling(doc, next);
        }
    }
    else {
        pcdoc_node prev = pcdoc_node_prev_sibling(doc, node);

        while (prev.data) {
            if (prev.type == PCDOC_NODE_ELEMENT) {
                if (name1) {
                    const char *name2;
                    size_t len2;

                    pcdoc_element_get_tag_name(doc, prev.elem, &name2, &len2,
                            NULL, NULL, NULL, NULL);
                    if (len1 == len2 && strncasecmp(name1, name2, len1) == 0)
                        my_count++;
                }
                else {
                    my_count++;
                }
            }

            prev = pcdoc_node_prev_sibling(doc, prev);
        }
    }

    *count = my_count;
    return CSS_OK;
}

/**
 * Callback to determine if a node is empty.
 *
 * \param pw     HTML document
 * \param node   DOM node
 * \param match  Pointer to location to receive result
 * \return CSS_OK.
 *
 * \post \a match will contain true if the node is empty and false otherwise.
 */
static css_error
node_is_empty(void *pw, void *n, bool *match)
{
    purc_document_t doc = (purc_document_t)pw;
    pcdoc_element_t ele = n;

    size_t nr_child_elements = 0;
    pcdoc_element_children_count(doc, ele, &nr_child_elements, NULL, NULL);

    if (nr_child_elements == 0) {
        *match = true;
    }
    else {
        *match = false;
    }

    return CSS_OK;
}

/**
 * Callback to determine if a node is a linking element.
 *
 * \param pw     HTML document
 * \param n      DOM node
 * \param match  Pointer to location to receive result
 * \return CSS_OK.
 *
 * \post \a match will contain true if the node matches and false otherwise.
 */
static css_error
node_is_link(void *pw, void *n, bool *match)
{
    (void)pw;
    (void)n;
    *match = false;
    return CSS_OK;
}

/**
 * Callback to determine if a node is currently being hovered over.
 *
 * \param pw     HTML document
 * \param node   DOM node
 * \param match  Pointer to location to receive result
 * \return CSS_OK.
 *
 * \post \a match will contain true if the node matches and false otherwise.
 */
static css_error
node_is_hover(void *pw, void *n, bool *match)
{
    (void)pw;
    (void)n;
    *match = false;
    return CSS_OK;
}

/**
 * Callback to determine if a node is currently activated.
 *
 * \param pw     HTML document
 * \param node   DOM node
 * \param match  Pointer to location to receive result
 * \return CSS_OK.
 *
 * \post \a match will contain true if the node matches and false otherwise.
 */
static css_error
node_is_active(void *pw, void *n, bool *match)
{
    (void)pw;
    (void)n;
    *match = false;
    return CSS_OK;
}

/**
 * Callback to determine if a node has the input focus.
 *
 * \param pw     HTML document
 * \param node   DOM node
 * \param match  Pointer to location to receive result
 * \return CSS_OK.
 *
 * \post \a match will contain true if the node matches and false otherwise.
 */
static css_error
node_is_focus(void *pw, void *n, bool *match)
{
    (void)pw;
    (void)n;
    *match = false;
    return CSS_OK;
}

/**
 * Callback to determine if a node is enabled.
 *
 * \param pw     HTML document
 * \param node   DOM node
 * \param match  Pointer to location to receive result
 * \return CSS_OK.
 *
 * \post \a match with contain true if the node is enabled and false otherwise.
 */
static css_error
node_is_enabled(void *pw, void *n, bool *match)
{
    (void)pw;
    (void)n;
    *match = false;
    return CSS_OK;
}

/**
 * Callback to determine if a node is disabled.
 *
 * \param pw     HTML document
 * \param node   DOM node
 * \param match  Pointer to location to receive result
 * \return CSS_OK.
 *
 * \post \a match with contain true if the node is disabled and false otherwise.
 */
static css_error
node_is_disabled(void *pw, void *n, bool *match)
{
    (void)pw;
    (void)n;
    *match = false;
    return CSS_OK;
}

/**
 * Callback to determine if a node is checked.
 *
 * \param pw     HTML document
 * \param node   DOM node
 * \param match  Pointer to location to receive result
 * \return CSS_OK.
 *
 * \post \a match with contain true if the node is checked and false otherwise.
 */
static css_error
node_is_checked(void *pw, void *n, bool *match)
{
    (void)pw;
    (void)n;
    *match = false;
    return CSS_OK;
}

/**
 * Callback to determine if a node is the target of the document URL.
 *
 * \param pw     HTML document
 * \param node   DOM node
 * \param match  Pointer to location to receive result
 * \return CSS_OK.
 *
 * \post \a match with contain true if the node matches and false otherwise.
 */
static css_error
node_is_target(void *pw, void *n, bool *match)
{
    (void)pw;
    (void)n;
    *match = false;
    return CSS_OK;
}

/**
 * Callback to determine if a node has the given language
 *
 * \param pw     HTML document
 * \param node   DOM node
 * \param lang   Language specifier to match
 * \param match  Pointer to location to receive result
 * \return CSS_OK.
 *
 * \post \a match will contain true if the node matches and false otherwise.
 */
static css_error node_is_lang(void *pw, void *n,
        lwc_string *lang, bool *match)
{
    purc_document_t doc = (purc_document_t)pw;
    pcdoc_element_t ele = n;

    const char *target_lang = lwc_string_data(lang);
    size_t target_len = lwc_string_length(lang);

    const char *found_lang;
    size_t found_len;
    int ret = doc_get_element_lang(doc, ele, &found_lang, &found_len);

    *match = false;
    if (ret == 0 && found_lang &&
            strncasecmp(target_lang, found_lang, target_len) == 0) {
        *match = true;
    }

    return CSS_OK;
}

/**
 * Callback to retrieve the User-Agent defaults for a CSS property.
 *
 * \param pw        HTML document
 * \param property  Property to retrieve defaults for
 * \param hint      Pointer to hint object to populate
 * \return CSS_OK       on success,
 *         CSS_INVALID  if the property should not have a user-agent default.
 */
static css_error
ua_default_for_property(void *pw, uint32_t property, css_hint *hint)
{
    (void)pw;

    if (property == CSS_PROP_COLOR) {
        hint->data.color = 0xFFFFFFFF;
        hint->status = CSS_COLOR_COLOR;
    } else if (property == CSS_PROP_FONT_FAMILY) {
        hint->data.strings = NULL;
        hint->status = CSS_FONT_FAMILY_MONOSPACE;
    } else if (property == CSS_PROP_QUOTES) {
        /* Not exactly useful :) */
        hint->data.strings = NULL;
        hint->status = CSS_QUOTES_NONE;
    } else if (property == CSS_PROP_VOICE_FAMILY) {
        /** \todo Fix this when we have voice-family done */
        hint->data.strings = NULL;
        hint->status = 0;
    } else {
        return CSS_INVALID;
    }

    return CSS_OK;
}

/**
 * Callback to find a named ancestor node.
 *
 * \param pw        HTML document
 * \param node      DOM node
 * \param qname     Node name to search for
 * \param ancestor  Pointer to location to receive ancestor
 * \return CSS_OK.
 *
 * \post \a ancestor will contain the result, or NULL if there is no match
 */
static css_error
named_ancestor_node(void *pw, void *n, const css_qname *qname,
        void **ancestor)
{
    purc_document_t doc = (purc_document_t)pw;

    const char *to_match_name = lwc_string_data(qname->name);
    size_t to_match_len  = lwc_string_length(qname->name);

    *ancestor = NULL;

    pcdoc_node node = { PCDOC_NODE_ELEMENT, { n } };
    pcdoc_element_t parent = pcdoc_node_get_parent(doc, node);
    while (parent) {
        const char *name;
        size_t len;

        pcdoc_element_get_tag_name(doc, parent, &name, &len,
                NULL, NULL, NULL, NULL);

        if (to_match_len == len &&
                strncasecmp(to_match_name, name, len) == 0) {
            *ancestor = (void *)parent;
            break;
        }

        node.type = PCDOC_NODE_ELEMENT;
        node.data = parent;
        parent = pcdoc_node_get_parent(doc, node);
    }

    return CSS_OK;
}

/**
 * Callback to determine if a node is a linking element whose target has been
 * visited.
 *
 * \param pw     HTML document
 * \param node   DOM node
 * \param match  Pointer to location to receive result
 * \return CSS_OK.
 *
 * \post \a match will contain true if the node matches and false otherwise.
 */
static css_error
node_is_visited(void *pw, void *n, bool *match)
{
    (void)pw;
    (void)n;
    *match = false;
    return CSS_OK;
}

/**
 * Callback to retrieve presentational hints for a node
 *
 * \param[in] pw HTML document
 * \param[in] node DOM node
 * \param[out] nhints number of hints retrieved
 * \param[out] hints retrieved hints
 * \return CSS_OK               on success,
 *         CSS_PROPERTY_NOT_SET if there is no hint for the requested property,
 *         CSS_NOMEM            on memory exhaustion.
 */
static css_error node_presentational_hint( void *pw, void *n,
        uint32_t *nhints, css_hint **hints)
{
    (void)pw;
    (void)n;
    *nhints = 0;
    *hints = NULL;
    return CSS_OK;
}

static css_error
set_node_data(void *pw, void *n, void *node_data)
{
    /* never reached here!!! */
    purc_document_t doc = (purc_document_t)pw;
    pcdoc_node node = { PCDOC_NODE_ELEMENT, { n } };

    pcdoc_node_set_user_data(doc, node, node_data);

    return CSS_OK;
}

static css_error
get_node_data(void *pw, void *n, void **node_data)
{
    /* never reached here!!! */
    purc_document_t doc = (purc_document_t)pw;
    pcdoc_node node = { PCDOC_NODE_ELEMENT, { n } };

    pcdoc_node_get_user_data(doc, node, node_data);
    return CSS_OK;
}

static css_error
compute_font_size(void *pw, const css_hint *parent, css_hint *size)
{
    (void)pw;
    (void)parent;

    /* for Foil, the font size always be 10px */
    size->data.length.value = FLTTOFIX(PX_PER_EM);
    size->data.length.unit = CSS_UNIT_PX;
    size->status = CSS_FONT_SIZE_DIMENSION;

    return CSS_OK;
}

css_select_handler purc_document_css_select_handler = {
    CSS_SELECT_HANDLER_VERSION_1,

    node_name,
    node_classes,
    node_id,
    named_ancestor_node,
    named_parent_node,
    named_sibling_node,
    named_generic_sibling_node,
    parent_node,
    sibling_node,
    node_has_name,
    node_has_class,
    node_has_id,
    node_has_attribute,
    node_has_attribute_equal,
    node_has_attribute_dashmatch,
    node_has_attribute_includes,
    node_has_attribute_prefix,
    node_has_attribute_suffix,
    node_has_attribute_substring,
    node_is_root,
    node_count_siblings,
    node_is_empty,
    node_is_link,
    node_is_visited,
    node_is_hover,
    node_is_active,
    node_is_focus,
    node_is_enabled,
    node_is_disabled,
    node_is_checked,
    node_is_target,
    node_is_lang,
    node_presentational_hint,
    ua_default_for_property,
    compute_font_size,
    set_node_data,
    get_node_data
};

