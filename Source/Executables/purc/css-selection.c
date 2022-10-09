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

#include "foil.h"
#include "csseng/csseng.h"
#include "purc/purc-dom.h"

#include <assert.h>
#include <string.h>

/******************************************************************************
 * Style selection callbacks                                                  *
 ******************************************************************************/

#define TAG_NAME_TEXT               "__TEXT"
#define TAG_NAME_FOREIGN            "__FOREIGN"
#define TAG_NAME_DISINTERESTED      "__DISINTERESTED"

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
    (void)pw;
    pcdom_node_t *node = n;
    assert(node->type == PCDOM_NODE_TYPE_ELEMENT);

#if 0
    if (node->type == PCDOM_NODE_TYPE_TEXT) {
        name = TAG_NAME_TEXT;
        len = sizeof(TAG_NAME_TEXT) - 1;
    }
    else {
        name = TAG_NAME_DISINTERESTED;
        len = sizeof(TAG_NAME_DISINTERESTED) - 1;
    }
        else {
            name = TAG_NAME_FOREIGN;
            len = sizeof(TAG_NAME_FOREIGN) - 1;
        }
#endif

    const char *name = NULL;
    size_t len;
    pcdom_element_t *element;
    element = pcdom_interface_element(node);
    name = (const char *)pcdom_element_local_name(element, &len);

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
    (void)pw;
    pcdom_node_t *node = (pcdom_node_t *)n;
    lwc_string **my_classes = NULL;
    uint32_t nr_classes = 0;
    char *cloned = NULL;

    assert(node->type == PCDOM_NODE_TYPE_ELEMENT);

    pcdom_element_t *element;
    element = pcdom_interface_element(node);

    const char *value;
    if (element->attr_class) {
        value = (const char *)pcdom_attr_value(element->attr_class, NULL);
    }
    else
        goto done;

    cloned = strdup(value);
    if (cloned == NULL)
        goto failed;

    char *str;
    char *saveptr;
    char *klass;
    for (str = cloned; ; str = NULL) {
        klass = strtok_r(str, CLASS_SEPARATOR, &saveptr);
        if (klass && purc_is_valid_identifier(klass)) {
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
                goto done;
            }
        }
    }

    return nr_classes;

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
    (void)pw;
    pcdom_node_t *node = (pcdom_node_t *)n;

    *id = NULL;
    if (node->type == PCDOM_NODE_TYPE_ELEMENT) {
        pcdom_element_t *element;
        element = pcdom_interface_element(node);
        if (element->attr_id) {
            const char *my_id;
            size_t len;

            my_id = (const char *)pcdom_attr_value(element->attr_id, &len);
            if (lwc_intern_string(my_id, len, id) == lwc_error_oom) {
                goto failed;
            }
        }
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
    (void)pw;
    *_parent = NULL;

    pcdom_node_t *node = (pcdom_node_t *)n;
    pcdom_node_t *parent = node->parent;
    if (parent && parent->type == PCDOM_NODE_TYPE_ELEMENT) {
        pcdom_element_t *element;
        const char *name1, *name2;
        size_t len1, len2;

        element = pcdom_interface_element(parent);
        name1 = (const char *)pcdom_element_local_name(element, &len1);

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
    (void)pw;
    *sibling = NULL;

    pcdom_node_t *node = (pcdom_node_t *)n;
    assert(node->type == PCDOM_NODE_TYPE_ELEMENT);

    pcdom_node_t *prev = node->prev;
    while (prev) {
        if (prev->type == PCDOM_NODE_TYPE_ELEMENT) {

            pcdom_element_t *element;
            const char *name1, *name2;
            size_t len1, len2;

            element = pcdom_interface_element(prev);
            name1 = (const char *)pcdom_element_local_name(element, &len1);

            name2 = lwc_string_data(qname->name);
            len2 = lwc_string_length(qname->name);

            if (len1 == len2 && strncasecmp(name1, name2, len1) == 0) {
                *sibling = (void *)prev;
            }

            break;
        }

        prev = prev->prev;
    }

    return CSS_OK;
}

/**
 * Callback to find a named previous generic sibling node.
 *
 * \param pw       HTML document
 * \param node     DOM node
 * \param qname    Node name to search for
 * \param sibling  Pointer to location to receive ancestor
 * \return CSS_OK.
 *
 * \post \a sibling will contain the result, or NULL if there is no match
 */
static css_error
named_generic_sibling_node(void *pw, void *n, const css_qname *qname,
        void **sibling)
{
    (void)pw;
    *sibling = NULL;

    pcdom_node_t *node = (pcdom_node_t *)n;
    pcdom_node_t *prev = node->prev;
    while (prev) {
        if (prev->type == PCDOM_NODE_TYPE_ELEMENT) {

            pcdom_element_t *element;
            const char *name1, *name2;
            size_t len1, len2;

            element = pcdom_interface_element(prev);
            name1 = (const char *)pcdom_element_local_name(element, &len1);

            name2 = lwc_string_data(qname->name);
            len2 = lwc_string_length(qname->name);

            if (len1 == len2 && strncasecmp(name1, name2, len1) == 0) {
                *sibling = (void *)prev;
                break;
            }
        }

        prev = prev->prev;
    }

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
    (void)pw;

    pcdom_node_t *node = (pcdom_node_t *)n;
    pcdom_node_t *parent = node->parent;
    if (parent && parent->type == PCDOM_NODE_TYPE_ELEMENT) {
        *_parent = (void *)parent;
    }
    else {
        *_parent = NULL;
    }

    return CSS_OK;
}

/**
 * Callback to retrieve the sibling of a node.
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
    (void)pw;
    *sibling = NULL;

    pcdom_node_t *node = (pcdom_node_t *)n;
    pcdom_node_t *prev = node->prev;
    while (prev) {
        if (prev->type == PCDOM_NODE_TYPE_ELEMENT) {
            *sibling = (void *)prev;
            break;
        }

        prev = prev->prev;
    }

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
    (void)pw;
    pcdom_node_t *node = n;
    assert(node->type == PCDOM_NODE_TYPE_ELEMENT);

    pcdom_element_t *element;
    element = pcdom_interface_element(node);

    const char *name1;
    size_t len1;
    name1 = (const char *)pcdom_element_local_name(element, &len1);

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
    (void)pw;
    *match = false;

    pcdom_node_t *node = (pcdom_node_t *)n;
    assert(node->type == PCDOM_NODE_TYPE_ELEMENT);

    pcdom_element_t *element;
    element = pcdom_interface_element(node);

    if (element->attr_class) {
        const char *str1;
        size_t len1;
        str1 = (const char *)pcdom_attr_value(element->attr_class, &len1);

        const char *str2 = lwc_string_data(name);
        size_t len2 = lwc_string_length(name);

        char *haystack = strndup(str1, len1);

        char *str;
        char *saveptr;
        char *klass;
        for (str = haystack; ; str = NULL) {
            klass = strtok_r(str, CLASS_SEPARATOR, &saveptr);
            /* to match values caseinsensitively */
            if (klass && purc_is_valid_identifier(klass) &&
                    strncasecmp(klass, str2, len2) == 0) {
                *match = true;
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
    (void)pw;
    pcdom_node_t *node = (pcdom_node_t *)n;
    assert(node->type == PCDOM_NODE_TYPE_ELEMENT);

    *match = false;
    pcdom_element_t *element;
    element = pcdom_interface_element(node);
    if (element->attr_id) {
        const char *str1;
        size_t len1;

        str1 = (const char *)pcdom_attr_value(element->attr_id, &len1);

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
    (void)pw;
    *match = false;

    pcdom_node_t *node = (pcdom_node_t *)n;
    assert(node->type == PCDOM_NODE_TYPE_ELEMENT);

    pcdom_element_t *element;
    element = pcdom_interface_element(node);

    pcdom_attr_t *attr;
    attr = pcdom_element_first_attribute(element);
    while (attr) {

        const char *str1;
        size_t len1;
        str1 = (const char *)pcdom_attr_local_name(attr, &len1);

        const char *str2 = lwc_string_data(qname->name);
        size_t len2 = lwc_string_length(qname->name);

        if (len1 == len2 && strncasecmp(str1, str2, len1) == 0) {
            *match = true;
            break;
        }

        attr = pcdom_element_next_attribute(attr);
    }

    return CSS_OK;
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
    (void)pw;
    *match = false;

    pcdom_node_t *node = (pcdom_node_t *)n;
    assert(node->type == PCDOM_NODE_TYPE_ELEMENT);

    pcdom_element_t *element;
    element = pcdom_interface_element(node);

    pcdom_attr_t *attr;
    attr = pcdom_element_first_attribute(element);
    while (attr) {

        const char *str1;
        size_t len1;
        str1 = (const char *)pcdom_attr_local_name(attr, &len1);

        const char *str2 = lwc_string_data(qname->name);
        size_t len2 = lwc_string_length(qname->name);

        if (len1 == len2 && strncasecmp(str1, str2, len1) == 0) {
            str1 = (const char *)pcdom_attr_value (attr, &len1);
            str2 = lwc_string_data(value);
            len2 = lwc_string_length(value);

            /* to match values casesensitively */
            if (len1 == len2 && strncmp(str1, str2, len1) == 0) {
                *match = true;
            }

            break;
        }

        attr = pcdom_element_next_attribute(attr);
    }

    return CSS_OK;
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
    (void)pw;
    *match = false;

    pcdom_node_t *node = (pcdom_node_t *)n;
    assert(node->type == PCDOM_NODE_TYPE_ELEMENT);

    pcdom_element_t *element;
    element = pcdom_interface_element(node);

    pcdom_attr_t *attr;
    attr = pcdom_element_first_attribute(element);
    while (attr) {

        const char *str1;
        size_t len1;
        str1 = (const char *)pcdom_attr_local_name(attr, &len1);

        const char *str2 = lwc_string_data(qname->name);
        size_t len2 = lwc_string_length(qname->name);

        if (len1 == len2 && strncasecmp(str1, str2, len1) == 0) {
            str1 = (const char *)pcdom_attr_value (attr, &len1);
            str2 = lwc_string_data(value);
            len2 = lwc_string_length(value);

            /* to match values casesensitively */
            if (strncmp(str1, str2, len1) == 0) {
                if (len1 == len2) {
                    *match = true;
                }
                else if (len1 > len2 && str1[len2] == '-') {
                    *match = true;
                }
            }

            break;
        }

        attr = pcdom_element_next_attribute(attr);
    }

    return CSS_OK;
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
    (void)pw;
    *match = false;

    pcdom_node_t *node = (pcdom_node_t *)n;
    assert(node->type == PCDOM_NODE_TYPE_ELEMENT);

    pcdom_element_t *element;
    element = pcdom_interface_element(node);

    pcdom_attr_t *attr;
    attr = pcdom_element_first_attribute(element);
    while (attr) {

        const char *str1;
        size_t len1;
        str1 = (const char *)pcdom_attr_local_name(attr, &len1);

        const char *str2 = lwc_string_data(qname->name);
        size_t len2 = lwc_string_length(qname->name);

        if (len1 == len2 && strncasecmp(str1, str2, len1) == 0) {
            str1 = (const char *)pcdom_attr_value(attr, &len1);
            str2 = lwc_string_data(value);
            len2 = lwc_string_length(value);

            char *haystack = strndup(str1, len1);

            char *str;
            char *saveptr;
            char *token;
            for (str = haystack; ; str = NULL) {
                token = strtok_r(str, CLASS_SEPARATOR, &saveptr);
                /* to match values casesensitively */
                if (token && strncasecmp(token, str2, len2) == 0) {
                    *match = true;
                }
            }

            free(haystack);
            break;
        }

        attr = pcdom_element_next_attribute(attr);
    }

    return CSS_OK;
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
    (void)pw;
    *match = false;

    pcdom_node_t *node = (pcdom_node_t *)n;
    assert(node->type == PCDOM_NODE_TYPE_ELEMENT);

    pcdom_element_t *element;
    element = pcdom_interface_element(node);

    pcdom_attr_t *attr;
    attr = pcdom_element_first_attribute(element);
    while (attr) {

        const char *str1;
        size_t len1;
        str1 = (const char *)pcdom_attr_local_name(attr, &len1);

        const char *str2 = lwc_string_data(qname->name);
        size_t len2 = lwc_string_length(qname->name);

        if (len1 == len2 && strncasecmp(str1, str2, len1) == 0) {
            str1 = (const char *)pcdom_attr_value(attr, &len1);
            str2 = lwc_string_data(value);
            len2 = lwc_string_length(value);

            /* to match values casesensitively */
            if (len1 >= len2 && strncmp(str1, str2, len2) == 0) {
                *match = true;
            }

            break;
        }

        attr = pcdom_element_next_attribute(attr);
    }

    return CSS_OK;
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
    (void)pw;
    *match = false;

    pcdom_node_t *node = (pcdom_node_t *)n;
    assert(node->type == PCDOM_NODE_TYPE_ELEMENT);

    pcdom_element_t *element;
    element = pcdom_interface_element(node);

    pcdom_attr_t *attr;
    attr = pcdom_element_first_attribute(element);
    while (attr) {

        const char *str1;
        size_t len1;
        str1 = (const char *)pcdom_attr_local_name(attr, &len1);

        const char *str2 = lwc_string_data(qname->name);
        size_t len2 = lwc_string_length(qname->name);

        if (len1 == len2 && strncasecmp(str1, str2, len1) == 0) {
            str1 = (const char *)pcdom_attr_value(attr, &len1);
            str2 = lwc_string_data(value);
            len2 = lwc_string_length(value);

            /* to match values casesensitively */
            if (len1 >= len2) {
                str1 += len1;
                str1 -= len2;
                if (strncmp(str1, str2, len2) == 0) {
                    *match = true;
                }
            }

            break;
        }

        attr = pcdom_element_next_attribute(attr);
    }

    return CSS_OK;
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
    (void)pw;
    *match = false;

    pcdom_node_t *node = (pcdom_node_t *)n;
    assert(node->type == PCDOM_NODE_TYPE_ELEMENT);

    pcdom_element_t *element;
    element = pcdom_interface_element(node);

    pcdom_attr_t *attr;
    attr = pcdom_element_first_attribute(element);
    while (attr) {

        const char *str1;
        size_t len1;
        str1 = (const char *)pcdom_attr_local_name(attr, &len1);

        const char *str2 = lwc_string_data(qname->name);
        size_t len2 = lwc_string_length(qname->name);

        if (len1 == len2 && strncasecmp(str1, str2, len1) == 0) {
            str1 = (const char *)pcdom_attr_value(attr, &len1);
            str2 = lwc_string_data(value);
            len2 = lwc_string_length(value);

            char *haystack = strndup(str1, len1);
            char *needle = strndup(str2, len2);

            /* to match values casesensitively */
            if (strstr(haystack, needle)) {
                *match = true;
            }

            free(haystack);
            free(needle);

            break;
        }

        attr = pcdom_element_next_attribute(attr);
    }

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
    (void)pw;

    pcdom_node_t *node = (pcdom_node_t *)n;
    if (node->parent == NULL ||
            node->parent->type == PCDOM_NODE_TYPE_DOCUMENT) {
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
    (void)pw;

    pcdom_node_t *node = (pcdom_node_t *)n;
    assert(node->type == PCDOM_NODE_TYPE_ELEMENT);

    int32_t my_count = 0;
    pcdom_element_t *element;

    const char *name1 = NULL;
    size_t len1 = 0;
    if (same_name) {
        element = pcdom_interface_element(node);
        name1 = (const char *)pcdom_element_local_name(element, &len1);
    }

    if (after) {
        pcdom_node_t *next = node->next;

        while (next) {
            if (next->type == PCDOM_NODE_TYPE_ELEMENT) {
                if (name1) {
                    const char *name2;
                    size_t len2;

                    element = pcdom_interface_element(next);
                    name2 = (const char *)pcdom_element_local_name(element,
                            &len2);

                    if (len1 == len2 && strncasecmp(name1, name2, len1) == 0)
                        my_count++;
                }
                else {
                    my_count++;
                }
            }

            next = next->next;
        }
    }
    else {
        pcdom_node_t *prev = node->prev;

        while (prev) {
            if (prev->type == PCDOM_NODE_TYPE_ELEMENT) {
                if (name1) {
                    const char *name2;
                    size_t len2;

                    element = pcdom_interface_element(prev);
                    name2 = (const char *)pcdom_element_local_name(element,
                            &len2);

                    if (len1 == len2 && strncasecmp(name1, name2, len1) == 0)
                        my_count++;
                }
                else {
                    my_count++;
                }
            }

            prev = prev->prev;
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
    (void)pw;
    pcdom_node_t *node = (pcdom_node_t *)n;
    assert(node->type == PCDOM_NODE_TYPE_ELEMENT);

    if (node->first_child == NULL)
        *match = true;
    else
        *match = false;

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
    (void)pw;
    (void)n;
    (void)lang;
    *match = false;
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
        hint->data.color = 0x00000000;
        hint->status = CSS_COLOR_COLOR;
    } else if (property == CSS_PROP_FONT_FAMILY) {
        hint->data.strings = NULL;
        hint->status = CSS_FONT_FAMILY_SANS_SERIF;
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
    (void)pw;

    pcdom_node_t *node = n;
    assert(node->type == PCDOM_NODE_TYPE_ELEMENT);

    *ancestor = NULL;
    pcdom_node_t *parent = node->parent;
    while (parent && parent->type != PCDOM_NODE_TYPE_DOCUMENT) {
        if (parent->type == PCDOM_NODE_TYPE_ELEMENT) {
            pcdom_element_t *element;
            const char *name1, *name2;
            size_t len1, len2;

            element = pcdom_interface_element(parent);
            name1 = (const char *)pcdom_element_local_name(element, &len1);

            name2 = lwc_string_data(qname->name);
            len2 = lwc_string_length(qname->name);

            if (len1 == len2 && strncasecmp(name1, name2, len1) == 0) {
                *ancestor = (void *)parent;
                break;
            }
        }

        parent = parent->parent;
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
    (void)pw;
    pcdom_node_t *node = n;
    node->user = node_data;
    return CSS_OK;
}

static css_error
get_node_data(void *pw, void *n, void **node_data)
{
    (void)pw;
    pcdom_node_t* node = n;
    *node_data = node->user;
    return CSS_OK;
}

static css_error
compute_font_size(void *pw, const css_hint *parent, css_hint *size)
{
    (void)pw;
    (void)parent;

    /* for Foil, the font size always be 10px */
    size->data.length.value = INTTOFIX(10);
    size->data.length.unit = CSS_UNIT_PX;
    size->status = CSS_FONT_SIZE_DIMENSION;

    return CSS_OK;
}

css_select_handler foil_css_select_handler = {
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

