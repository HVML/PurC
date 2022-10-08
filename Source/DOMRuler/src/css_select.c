/*
** This file is part of DOM Ruler. DOM Ruler is a library to
** maintain a DOM tree, lay out and stylize the DOM nodes by
** using CSS (Cascaded Style Sheets).
**
** Copyright (C) 2022 Beijing FMSoft Technologies Co., Ltd.
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General License for more details.
**
** You should have received a copy of the GNU Lesser General License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "select.h"
#include "utils.h"
#include "node.h"
#include <stdio.h>
#include <string.h>


/******************************************************************************
 * Style selection callbacks                                                  *
 ******************************************************************************/

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
    HLLayoutNode *node = n;

    if (node->inner_tag == NULL) {
        qname->name = NULL;
        return CSS_NOMEM;
    }

    qname->name = lwc_string_ref(node->inner_tag);
    return CSS_OK;
}

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
    HLLayoutNode *node = n;

    if (node->nr_inner_classes > 0) {
        *classes = node->inner_classes;
        *n_classes = node->nr_inner_classes;

        for (int i = 0; i < node->nr_inner_classes; i++) {
            (void) lwc_string_ref((*classes)[i]);
        }
    } else {
        *n_classes = 0;
        *classes = NULL;
    }
    return CSS_OK;
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
    HLLayoutNode *node = n;

    if (node->inner_id == NULL) {
        *id = NULL;
        return CSS_OK;
    }

    *id = lwc_string_ref(node->inner_id);
    return CSS_OK;
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
named_parent_node(void *pw, void *n, const css_qname *qname, void **parent)
{
    (void)pw;
    HLLayoutNode* node = n;

    *parent = NULL;

    for (node = hl_layout_node_get_parent(node); node != NULL; node = hl_layout_node_get_parent(node)) {
        HLNodeType type = hl_layout_node_get_type(node);
        if (type != DOM_ELEMENT_NODE) {
            continue;
        }

        assert(node->inner_tag != NULL);

        bool match = false;
        if (lwc_string_caseless_isequal(node->inner_tag, qname->name,
                    &match) == lwc_error_ok && match)
        {
            *parent = (HLLayoutNode *)node;
        }
        break;
    }

    return CSS_OK;
}

static int
dom_node_get_previous_sibling(HLLayoutNode *node, HLLayoutNode **result)
{
    /* Attr nodes have no previous siblings */
    HLNodeType type = hl_layout_node_get_type(node);
    if (type == DOM_ATTRIBUTE_NODE) {
        *result = NULL;
        return CSS_OK;
    }

    *result = hl_layout_node_previous(node);
    return CSS_OK;
}

/**
 * Callback to find a named sibling node.
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
    HLLayoutNode *node = n;
    HLLayoutNode *prev;

    *sibling = NULL;

    /* Find sibling element */
    dom_node_get_previous_sibling(node, &node);

    while (node != NULL) {
        HLNodeType type = hl_layout_node_get_type(node);
        if (type == DOM_ELEMENT_NODE) {
            break;
        }

        dom_node_get_previous_sibling(node, &prev);
        node = prev;
    }

    if (node != NULL) {
        bool match = false;
        if (lwc_string_caseless_isequal(node->inner_tag, qname->name,
                    &match) == lwc_error_ok && match)
        {
            *sibling = node;
        }
    }

    return CSS_OK;
}

/**
 * Callback to find a named generic sibling node.
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
    HLLayoutNode *node = n;
    HLLayoutNode *prev;

    *sibling = NULL;
    dom_node_get_previous_sibling(node, &node);

    while (node != NULL) {
        HLNodeType type = hl_layout_node_get_type(node);

        if (type == DOM_ELEMENT_NODE) {
            bool match = false;
            if (lwc_string_caseless_isequal(node->inner_tag, qname->name,
                        &match) == lwc_error_ok && match)
            {
                *sibling = node;
                break;
            }
        }

        dom_node_get_previous_sibling(node, &prev);
        node = prev;
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
parent_node(void *pw, void *n, void **parent)
{
    (void)pw;
    HLLayoutNode *node = n;
    *parent = NULL;
    for (node = hl_layout_node_get_parent(node); node != NULL;
            node = hl_layout_node_get_parent(node)) {
        HLNodeType type = hl_layout_node_get_type(node);
        if (type != DOM_ELEMENT_NODE) {
            continue;
        }

        *parent = node;
        break;
    }
    return CSS_OK;
}

/**
 * Callback to retrieve the preceding sibling of a node.
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
    HLLayoutNode *node = n;
    HLLayoutNode *prev;

    *sibling = NULL;

    /* Find sibling element */
    dom_node_get_previous_sibling(node, &node);

    while (node != NULL) {
        HLNodeType type = hl_layout_node_get_type(node);
        if (type == DOM_ELEMENT_NODE)
            break;

        dom_node_get_previous_sibling(node, &prev);
        node = prev;
    }

    if (node != NULL) {
        *sibling = node;
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
    HLLayoutNode *node = n;
    lwc_string_caseless_isequal(node->inner_tag, qname->name, match);
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
    int class;
    HLLayoutNode *node = n;

    /* Short-circuit case where we have no classes */
    if (node->nr_inner_classes == 0) {
        *match = false;
        return CSS_OK;
    }

    /* Standards mode: case sensitively match */
    for (class = 0; class < node->nr_inner_classes; class++) {
        if (lwc_error_ok == lwc_string_caseless_isequal(name,
                    node->inner_classes[class], match) &&
                *match == true)
            return CSS_OK;
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
    HLLayoutNode *node = n;
    lwc_string_caseless_isequal(node->inner_id, name, match);
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
node_has_attribute(void *pw, void *node, const css_qname *qname, bool *match)
{
    (void)pw;
    (void)node;
    (void)qname;
    *match = false;
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
node_has_attribute_equal(void *pw, void *node, const css_qname *qname,
        lwc_string *value, bool *match)
{
    (void)pw;
    (void)node;
    (void)qname;
    (void)value;
    *match = false;
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
node_has_attribute_dashmatch(void *pw, void *node, const css_qname *qname,
        lwc_string *value, bool *match)
{
    (void)pw;
    (void)node;
    (void)qname;
    (void)value;
    *match = false;
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
node_has_attribute_includes(void *pw, void *node, const css_qname *qname,
        lwc_string *value, bool *match)
{
    (void)pw;
    (void)node;
    (void)qname;
    (void)value;
    *match = false;
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
node_has_attribute_prefix(void *pw, void *node, const css_qname *qname,
        lwc_string *value, bool *match)
{
    (void)pw;
    (void)node;
    (void)qname;
    (void)value;
    *match = false;
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
node_has_attribute_suffix(void *pw, void *node, const css_qname *qname,
        lwc_string *value, bool *match)
{
    (void)pw;
    (void)node;
    (void)qname;
    (void)value;
    *match = false;
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
node_has_attribute_substring(void *pw, void *node, const css_qname *qname,
        lwc_string *value, bool *match)
{
    (void)pw;
    (void)node;
    (void)qname;
    (void)value;
    *match = false;
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
    *match = hl_layout_node_is_root((HLLayoutNode *)n);
    return CSS_OK;
}

static int
dom_node_get_next_sibling(HLLayoutNode *node, HLLayoutNode **result)
{
    /* Attr nodes have no next siblings */
    HLNodeType type = hl_layout_node_get_type(node);
    if (type == DOM_ATTRIBUTE_NODE) {
        *result = NULL;
        return CSS_OK;
    }

    *result = hl_layout_node_next(node);
    return CSS_OK;
}

static int
node_count_siblings_check(HLLayoutNode *node,
              bool check_name,
              lwc_string *name)
{
    HLNodeType type;
    int ret = 0;

    if (node == NULL)
        return 0;

    type = hl_layout_node_get_type(node);
    if (type != DOM_ELEMENT_NODE) {
        return 0;
    }

    if (check_name) {
        lwc_string *node_name = node->inner_tag;

        if (node_name != NULL) {

            bool match = false;
            if (lwc_string_caseless_isequal(node_name, name,
                    &match) == lwc_error_ok && match) {
                ret = 1;
            }
        }
    } else {
        ret = 1;
    }
    return ret;
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
    int32_t cnt = 0;
    lwc_string *node_name = NULL;

    if (same_name) {
        HLLayoutNode *node = n;
        node_name = node->inner_tag;
        if (node_name == NULL) {
            return CSS_NOMEM;
        }
    }

    if (after) {
        HLLayoutNode *node = n;
        HLLayoutNode *next;

        do {
            dom_node_get_next_sibling(node, &next);
            node = next;
            cnt += node_count_siblings_check(node, same_name, node_name);
        } while (node != NULL);
    } else {
        HLLayoutNode *node = n;
        HLLayoutNode *next;

        do {
            dom_node_get_previous_sibling(node, &next);
            node = next;
            cnt += node_count_siblings_check(node, same_name, node_name);
        } while (node != NULL);
    }

    *count = cnt;
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
node_is_empty(void *pw, void *node, bool *match)
{
    (void)pw;
    (void)node;
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
node_is_link(void *pw, void *node, bool *match)
{
    (void)pw;
    (void)node;
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
node_is_hover(void *pw, void *node, bool *match)
{
    (void)pw;
    (void)node;
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
node_is_active(void *pw, void *node, bool *match)
{
    (void)pw;
    (void)node;
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
node_is_focus(void *pw, void *node, bool *match)
{
    (void)pw;
    (void)node;
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
node_is_enabled(void *pw, void *node, bool *match)
{
    (void)pw;
    (void)node;
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
node_is_disabled(void *pw, void *node, bool *match)
{
    (void)pw;
    (void)node;
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
node_is_checked(void *pw, void *node, bool *match)
{
    (void)pw;
    (void)node;
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
node_is_target(void *pw, void *node, bool *match)
{
    (void)pw;
    (void)node;
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
static css_error node_is_lang(void *pw, void *node,
        lwc_string *lang, bool *match)
{
    (void)pw;
    (void)node;
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

typedef struct _HlCSSDataPackage {
    HLLayoutNode* node;
    void* node_data;
} HlCSSDataPackage;

static void destroy_hl_css_data_package(void* data);

static css_error
set_node_data(void *pw, void *n, void *node_data)
{
    (void)pw;
    HlCSSDataPackage* pkg = (HlCSSDataPackage*)calloc(1,
            sizeof(HlCSSDataPackage));
    HLLayoutNode *node = n;
    pkg->node = node;
    pkg->node_data = node_data;
    hl_layout_node_set_inner_data(node, HL_INNER_CSS_SELECT_ATTACH, pkg,
            destroy_hl_css_data_package);
    return CSS_OK;
}

static css_error 
get_node_data(void *pw, void *n, void **node_data)
{
    (void)pw;
    HLLayoutNode* node = n;
    HlCSSDataPackage* pkg = hl_layout_node_get_inner_data(node,
            HL_INNER_CSS_SELECT_ATTACH);
    *node_data = pkg ? pkg->node_data : NULL;
    return CSS_OK;
}


static css_error
compute_font_size(void *pw, const css_hint *parent, css_hint *size)
{
    (void)pw;

    static css_hint_length sizes[] = {
        { FLTTOFIX(6.75), CSS_UNIT_PT },
        { FLTTOFIX(7.50), CSS_UNIT_PT },
        { FLTTOFIX(9.75), CSS_UNIT_PT },
        { FLTTOFIX(12.0), CSS_UNIT_PT },
        { FLTTOFIX(13.5), CSS_UNIT_PT },
        { FLTTOFIX(18.0), CSS_UNIT_PT },
        { FLTTOFIX(24.0), CSS_UNIT_PT }
    };
    const css_hint_length *parent_size;

    /* Grab parent size, defaulting to medium if none */
    if (parent == NULL) {
        parent_size = &sizes[CSS_FONT_SIZE_MEDIUM - 1];
    } else {
        assert(parent->status == CSS_FONT_SIZE_DIMENSION);
        assert(parent->data.length.unit != CSS_UNIT_EM);
        assert(parent->data.length.unit != CSS_UNIT_EX);
        parent_size = &parent->data.length;
    }

    assert(size->status != CSS_FONT_SIZE_INHERIT);

    if (size->status < CSS_FONT_SIZE_LARGER) {
        /* Keyword -- simple */
        size->data.length = sizes[size->status - 1];
    } else if (size->status == CSS_FONT_SIZE_LARGER) {
        /** \todo Step within table, if appropriate */
        size->data.length.value =
                FMUL(parent_size->value, FLTTOFIX(1.2));
        size->data.length.unit = parent_size->unit;
    } else if (size->status == CSS_FONT_SIZE_SMALLER) {
        /** \todo Step within table, if appropriate */
        size->data.length.value =
                FMUL(parent_size->value, FLTTOFIX(1.2));
        size->data.length.unit = parent_size->unit;
    } else if (size->data.length.unit == CSS_UNIT_EM ||
            size->data.length.unit == CSS_UNIT_EX) {
        size->data.length.value =
            FMUL(size->data.length.value, parent_size->value);

        if (size->data.length.unit == CSS_UNIT_EX) {
            size->data.length.value = FMUL(size->data.length.value,
                    FLTTOFIX(0.6));
        }

        size->data.length.unit = parent_size->unit;
    } else if (size->data.length.unit == CSS_UNIT_PCT) {
        size->data.length.value = FDIV(FMUL(size->data.length.value,
                parent_size->value), FLTTOFIX(100));
        size->data.length.unit = parent_size->unit;
    }

    size->status = CSS_FONT_SIZE_DIMENSION;

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
    HLLayoutNode *node = n;

    *ancestor = NULL;

    for (node = hl_layout_node_get_parent(node); node != NULL;
            node = hl_layout_node_get_parent(node)) {
        HLNodeType type = hl_layout_node_get_type(node);
        if (type != DOM_ELEMENT_NODE)
            continue;

        assert(node->inner_tag!= NULL);

        bool match = false;
        if (lwc_string_caseless_isequal(node->inner_tag, qname->name,
                    &match) == lwc_error_ok && match)
        {
            *ancestor = node;
            break;
        }
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
node_is_visited(void *pw, void *node, bool *match)
{
    (void)pw;
    (void)node;
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
static css_error node_presentational_hint( void *pw, void *node,
        uint32_t *nhints, css_hint **hints)
{
    (void)pw;
    (void)node;
    *nhints = 0;
    *hints = NULL;
    return CSS_OK;
}

css_select_handler hl_css_select_handler = {
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

void
destroy_hl_css_data_package(void* data)
{
    if (data == NULL) {
        return;
    }
    HlCSSDataPackage* pkg = (HlCSSDataPackage*)data;
    /* VW: changed from CSS_NODE_MODIFIED to CSS_NODE_DELETED */
    css_node_data_handler(&hl_css_select_handler, CSS_NODE_DELETED,
            NULL, pkg->node, NULL, pkg->node_data);
    free(pkg);
}

css_select_results *hl_get_node_style(const css_media *media,
        css_select_ctx *select_ctx, HLLayoutNode *node)
{
    if (media == NULL || select_ctx == NULL || node == NULL) {
        HL_LOGW("get node style failed.\n");
        return NULL;
    }

    // prepare inline style
    css_stylesheet* inline_style = NULL;
    const char* style = hl_layout_node_get_attr(node, ATTR_STYLE);
    if (style != NULL) {
        inline_style = hl_css_stylesheet_inline_style_create(
                (const uint8_t *)style, strlen(style));
    }

    /* Select style for node */
    css_select_results *styles;
    css_error error = css_select_style(select_ctx, node, media, inline_style,
            &hl_css_select_handler, NULL, &styles);

    if (error != CSS_OK || styles == NULL) {
        /* VW: clear the inner data if failed. */
        HlCSSDataPackage* pkg = hl_layout_node_get_inner_data(node,
                HL_INNER_CSS_SELECT_ATTACH);
        if (pkg) {
            free(pkg);
        }
        hl_layout_node_set_inner_data(node, HL_INNER_CSS_SELECT_ATTACH,
                NULL, NULL);

        /* Failed selecting partial style -- bail out */
        hl_css_stylesheet_destroy(inline_style);
        return NULL;
    }

    int pseudo_element;
    css_computed_style *composed = NULL;
    for (pseudo_element = CSS_PSEUDO_ELEMENT_NONE + 1;
            pseudo_element < CSS_PSEUDO_ELEMENT_COUNT;
            pseudo_element++) {

        if (pseudo_element == CSS_PSEUDO_ELEMENT_FIRST_LETTER ||
                pseudo_element == CSS_PSEUDO_ELEMENT_FIRST_LINE)
            /* TODO: Handle first-line and first-letter pseudo
             *       element computed style completion */
            continue;

        if (styles->styles[pseudo_element] == NULL)
            /* There were no rules concerning this pseudo element */
            continue;

        /* Complete the pseudo element's computed style, by composing
         * with the base element's style */
        error = css_computed_style_compose(
                styles->styles[CSS_PSEUDO_ELEMENT_NONE],
                styles->styles[pseudo_element],
                compute_font_size, NULL,
                &composed);
        if (error != CSS_OK) {
            /* TODO: perhaps this shouldn't be quite so
             * catastrophic? */
            css_select_results_destroy(styles);
            hl_css_stylesheet_destroy(inline_style);
            return NULL;
        }

        /* Replace select_results style with composed style */
        css_computed_style_destroy(styles->styles[pseudo_element]);
        styles->styles[pseudo_element] = composed;
    }

    hl_css_stylesheet_destroy(inline_style);
    return styles;
}

css_select_results *hl_css_select_style(const HLCSS* css, HLLayoutNode *n,
        const css_media *media, const css_stylesheet *inlineStyleSheet,
        css_select_handler *handler)
{
    css_computed_style *composed;
    css_select_results *styles;
    int pseudo_element;
    css_error error;
    css_error code;
    css_select_ctx *select_ctx;
    uint32_t count;

    if (css == NULL || css->sheet == NULL) {
        HL_LOGW("css select style param error.\n");
        return NULL;
    }

    if (css->done != 1) {
        hl_css_stylesheet_data_done(css->sheet);
    }

    css_stylesheet* styleSheet = css->sheet;

    code = css_select_ctx_create(&select_ctx);
    if (code != CSS_OK) {
        fprintf(stderr, "css_select_ctx_create failed! code=%d\n", code);
        return NULL;
    }

    code = css_select_ctx_append_sheet(select_ctx, styleSheet,
            CSS_ORIGIN_AUTHOR, NULL);
    if (code != CSS_OK) {
        fprintf(stderr, "css_select_ctx_append_sheet failed! code=%d\n", code);
        hl_css_select_ctx_destroy(select_ctx);
        return NULL;
    }

    code = css_select_ctx_count_sheets(select_ctx, &count);
    if (code != CSS_OK) {
        fprintf(stderr, "css_select_ctx_count_sheets failed! code=%d\n", code);
        hl_css_select_ctx_destroy(select_ctx);
        return NULL;
    }

    fprintf(stderr, "created selection context with %i sheets\n", count);

    /* Select style for node */
    error = css_select_style(select_ctx, n, media, inlineStyleSheet,
            handler ? handler : &hl_css_select_handler, NULL, &styles);

    if (error != CSS_OK || styles == NULL) {
        /* Failed selecting partial style -- bail out */
        hl_css_select_ctx_destroy(select_ctx);
        return NULL;
    }

    for (pseudo_element = CSS_PSEUDO_ELEMENT_NONE + 1;
            pseudo_element < CSS_PSEUDO_ELEMENT_COUNT;
            pseudo_element++) {

        if (pseudo_element == CSS_PSEUDO_ELEMENT_FIRST_LETTER ||
                pseudo_element == CSS_PSEUDO_ELEMENT_FIRST_LINE)
            /* TODO: Handle first-line and first-letter pseudo
             *       element computed style completion */
            continue;

        if (styles->styles[pseudo_element] == NULL)
            /* There were no rules concerning this pseudo element */
            continue;

        /* Complete the pseudo element's computed style, by composing
         * with the base element's style */
        error = css_computed_style_compose(
                styles->styles[CSS_PSEUDO_ELEMENT_NONE],
                styles->styles[pseudo_element],
                compute_font_size, NULL,
                &composed);
        if (error != CSS_OK) {
            /* TODO: perhaps this shouldn't be quite so
             * catastrophic? */
            css_select_results_destroy(styles);
            hl_css_select_ctx_destroy(select_ctx);
            return NULL;
        }

        /* Replace select_results style with composed style */
        css_computed_style_destroy(styles->styles[pseudo_element]);
        styles->styles[pseudo_element] = composed;
    }

    hl_css_select_ctx_destroy(select_ctx);
    return styles;
}

int hl_css_select_result_destroy(css_select_results *result)
{
    if (result)
        return css_select_results_destroy(result);
    return 0;
}

css_fixed css_screen_dpi = F_90;
css_fixed css_baseline_pixel_density = F_96;
static inline css_fixed css_pixels_css_to_physical(
        css_fixed css_pixels)
{
    return FDIV(FMUL(css_pixels, css_screen_dpi),
            css_baseline_pixel_density);
}

