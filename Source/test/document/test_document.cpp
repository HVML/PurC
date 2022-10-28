/*
** @file test_document.cpp
** @author Vincent Wei
** @date 2022/10/14
** @brief The program to test APIs about abstract document.
**
** Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of PurC (short for Purring Cat), an HVML interpreter.
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <purc/purc-document.h>

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

static const char *html_contents = ""
"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\">"
""
"<html lang=\"en\">"
"<head id=\"foo\">"
"<title>Cascading Style Sheets Level 2 Revision 1 (CSS&nbsp;2.1) Specification</title>"
"<link rel=\"stylesheet\" href=\"style/default.css\" type=\"text/css\">"
"<link rel=\"stylesheet\" href=\"https://www.w3.org/StyleSheets/TR/W3C-REC.css\" type=\"text/css\">"
"<link rel=\"next\" href=\"about.html\">"
"<link rel=\"contents\" href=\"cover.html#minitoc\">"
"<link rel=\"CSS-properties\" href=\"propidx.html\" title=\"properties\">"
"<link rel=\"index\" href=\"indexlist.html\" title=\"index\">"
"<link rel=\"first\" href=\"cover.html\">"
"<!--script src=\"http://www.w3c-test.org/css/harness/annotate.js#CSS21_DEV\" type=\"text/javascript\" defer></script-->"
"</head>"
""
"<body id=\"bar\" class=\"foo bar\tfoobar\" hvml:raw='true'>"
"<div class=\"quick toc\">"
"<h2><a name=\"minitoc\">Quick Table of Contents</a></h2>"
"<ul class=\"toc\">"
"  <li class=\"tocline1\"><a href=\"about.html#q1.0\" class=\"tocxref\">1 About the CSS&nbsp;2.1 Specification</a>"
"  <li class=\"tocline1\"><a href=\"intro.html#q2.0\" class=\"tocxref\">2 Introduction to CSS&nbsp;2.1</a>"
"  <li class=\"tocline1\"><a href=\"conform.html#q3.0\" class=\"tocxref\">3 Conformance: Requirements and Recommendations</a>"
"  <li class=\"tocline1\"><a href=\"syndata.html#q4.0\" class=\"tocxref\">4 Syntax and basic data types</a>"
"  <li class=\"tocline1\"><a href=\"selector.html#q5.0\" class=\"tocxref\">5 Selectors</a>"
"  <li class=\"tocline1\"><a href=\"cascade.html#q6.0\" class=\"tocxref\">6 Assigning property values, Cascading, and Inheritance</a>"
"  <li class=\"tocline1\"><a href=\"media.html#q7.0\" class=\"tocxref\">7 Media types</a>"
"  <li class=\"tocline1\"><a href=\"box.html#box-model\" class=\"tocxref\">8 Box model</a>"
"  <li class=\"tocline1\"><a href=\"visuren.html#q9.0\" class=\"tocxref\">9 Visual formatting model</a>"
"  <li class=\"tocline1\"><a href=\"visudet.html#q10.0\" class=\"tocxref\">10 Visual formatting model details</a>"
"  <li class=\"tocline1\"><a href=\"visufx.html#q11.0\" class=\"tocxref\">11 Visual effects</a>"
"  <li class=\"tocline1\"><a href=\"generate.html#generated-text\" class=\"tocxref\">12 Generated <span class=\"index-def\" title=\"generated content\">content</span>, automatic <span class=\"index-def\" title=\"automatic numbering\">numbering</span>, and lists</a>"
"  <li class=\"tocline1\"><a href=\"page.html#the-page\" class=\"tocxref\">13 Paged media</a>"
"  <li class=\"tocline1\"><a href=\"colors.html#q14.0\" class=\"tocxref\">14 Colors and Backgrounds</a>"
"  <li class=\"tocline1\"><a href=\"fonts.html#q15.0\" class=\"tocxref\">15 Fonts</a>"
"  <li class=\"tocline1\"><a href=\"text.html#q16.0\" class=\"tocxref\">16 Text</a>"
"  <li class=\"tocline1\"><a href=\"tables.html#q17.0\" class=\"tocxref\">17 Tables</a>"
"  <li class=\"tocline1\"><a href=\"ui.html#q18.0\" class=\"tocxref\">18 User interface</a>"
"  <li class=\"tocline1\"><a href=\"aural.html#q19.0\" class=\"tocxref\">Appendix A. Aural style sheets</a>"
"  <li class=\"tocline1\"><a href=\"refs.html#q20.0\" class=\"tocxref\">Appendix B. Bibliography</a>"
"  <li class=\"tocline1\"><a href=\"changes.html#q21.0\" class=\"tocxref\">Appendix C. Changes</a>"
"  <li class=\"tocline1\"><a href=\"sample.html#q22.0\" class=\"tocxref\">Appendix D. Default style sheet for HTML 4</a>"
"  <li class=\"tocline1\"><a href=\"zindex.html#q23.0\" class=\"tocxref\">Appendix E. Elaborate description of Stacking Contexts</a>"
"  <li class=\"tocline1\"><a href=\"propidx.html#q24.0\" class=\"tocxref\">Appendix F. Full property table</a>"
"  <li class=\"tocline1\"><a href=\"grammar.html#q25.0\" class=\"tocxref\">Appendix G. Grammar of CSS&nbsp;2.1</a>"
"  <li class=\"tocline1\"><a href=\"indexlist.html#q27.0\" class=\"tocxref\">Appendix I. Index</a>"
"</ul>"
"</div>"
"</body>"
"</html>"
;


TEST(document, basic)
{
    purc_document_t doc = purc_document_load(PCDOC_K_TYPE_HTML,
            html_contents, strlen(html_contents));
    ASSERT_NE(doc, nullptr);

    purc_document_type type;
    void *impl_entity;
    impl_entity = purc_document_impl_entity(doc, &type);
    ASSERT_NE(impl_entity, nullptr);
    ASSERT_EQ(type, PCDOC_K_TYPE_HTML);

    int ret;
    const char *local_name;
    size_t local_len;
    const char *prefix_name;
    size_t prefix_len;
    const char *ns_name;
    size_t ns_len;

    /* root element */
    pcdoc_element_t root = purc_document_root(doc);
    ASSERT_NE(root, nullptr);

    ret = pcdoc_element_get_tag_name(doc, root, &local_name, &local_len,
            &prefix_name, &prefix_len, &ns_name, &ns_len);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(local_len, 4);
    ASSERT_EQ(prefix_len, 0);
    ASSERT_EQ(ns_len, 4);

    ret = strncmp(local_name, "html", 4);
    ASSERT_EQ(ret, 0);

    ret = strncmp(ns_name, PCDOC_NSNAME_HTML, sizeof(PCDOC_NSNAME_HTML) - 1);
    ASSERT_EQ(ret, 0);

    /* head element */
    pcdoc_element_t head = purc_document_head(doc);
    ASSERT_NE(head, nullptr);

    ret = pcdoc_element_get_tag_name(doc, head,
            &local_name, &local_len, NULL, NULL, &ns_name, &ns_len);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(local_len, 4);
    ASSERT_EQ(prefix_len, 0);
    ASSERT_EQ(ns_len, 4);

    ret = strncmp(local_name, "head", 4);
    ASSERT_EQ(ret, 0);

    ret = strncmp(ns_name, PCDOC_NSNAME_HTML, sizeof(PCDOC_NSNAME_HTML) - 1);
    ASSERT_EQ(ret, 0);

    /* body element */
    pcdoc_element_t body = purc_document_body(doc);
    ASSERT_NE(body, nullptr);

    ret = pcdoc_element_get_tag_name(doc, body,
            &local_name, &local_len, NULL, NULL, &ns_name, &ns_len);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(local_len, 4);
    ASSERT_EQ(prefix_len, 0);
    ASSERT_EQ(ns_len, 4);

    ret = strncmp(local_name, "body", 4);
    ASSERT_EQ(ret, 0);

    ret = strncmp(ns_name, PCDOC_NSNAME_HTML, sizeof(PCDOC_NSNAME_HTML) - 1);
    ASSERT_EQ(ret, 0);

    const char *value;
    size_t len = 0;

    /* special attributes */
    // root element has no `id` attribute
    value = pcdoc_element_id(doc, root, NULL);
    ASSERT_EQ(value, nullptr);

    // root element has no `class` attribute
    value = pcdoc_element_class(doc, root, NULL);
    ASSERT_EQ(value, nullptr);

    // head element has `id` attribute: "foo"
    value = pcdoc_element_id(doc, head, NULL);
    ret = strncmp(value, "foo", len);
    ASSERT_EQ(ret, 0);

    // head element no `class` attribute
    value = pcdoc_element_class(doc, head, &len);
    ASSERT_EQ(value, nullptr);

    // body element has `id` attribute: "bar" */
    value = pcdoc_element_id(doc, body, NULL);
    ret = strncmp(value, "bar", len);
    ASSERT_EQ(ret, 0);

    ret = pcdoc_element_get_attribute(doc, body, "bad attr name", &value, &len);
    ASSERT_NE(ret, 0);

    ret = pcdoc_element_get_attribute(doc, body, "class", &value, &len);
    ASSERT_EQ(ret, 0);

    // body element has `class` attribute: foo bar foobar */
    value = pcdoc_element_class(doc, body, NULL);
    ASSERT_NE(value, nullptr);

    // bad class name: "foo bar"
    bool found;
    ret = pcdoc_element_has_class(doc, body, "foo bar", &found);
    ASSERT_NE(ret, 0);

    ret = pcdoc_element_has_class(doc, body, "foo", &found);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(found, true);

    ret = pcdoc_element_has_class(doc, body, "bar", &found);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(found, true);

    ret = pcdoc_element_has_class(doc, body, "foobar", &found);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(found, true);

    ret = pcdoc_element_has_class(doc, body, "foo-bar", &found);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(found, false);

    unsigned int refc = purc_document_delete(doc);
    ASSERT_EQ(refc, 1);
}

static int my_attribute_cb(purc_document_t doc, pcdoc_attr_t attr,
        const char *name, size_t name_len,
        const char *value, size_t value_len, void *ctxt)
{
    (void)doc;
    (void)attr;

    std::string *str = static_cast<std::string *>(ctxt);
    str->append(name, name_len);
    str->push_back(':');
    str->append(value, value_len);
    str->push_back('\n');

    return 0;
}

TEST(document, travel_attributes)
{
    purc_document_t doc = purc_document_load(PCDOC_K_TYPE_HTML,
            html_contents, strlen(html_contents));
    ASSERT_NE(doc, nullptr);

    /* body element */
    pcdoc_element_t body = purc_document_body(doc);
    ASSERT_NE(body, nullptr);

    size_t nr;
    std::string result;
    int ret = pcdoc_element_travel_attributes(doc,
        body, my_attribute_cb, &result, &nr);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(nr, 3);
    ASSERT_STREQ(result.c_str(), "id:bar\nclass:foo bar\tfoobar\nhvml:raw:true\n");

    const char *local_name;
    size_t local_len;
    const char *qualified_name;
    size_t qualified_len;
    const char *value;
    size_t value_len;

    pcdoc_attr_t attr;
    attr = pcdoc_element_first_attr(doc, body);
    ASSERT_NE(attr, nullptr);

    ret = pcdoc_attr_get_info(doc, attr,
            &local_name, &local_len,
            &qualified_name, &qualified_len,
            &value, &value_len);
    ASSERT_EQ(ret, 0);
    ret = strncmp(local_name, "id", local_len);
    ASSERT_EQ(ret, 0);
    ret = strncmp(qualified_name, "id", qualified_len);
    ASSERT_EQ(ret, 0);
    ret = strncmp(value, "bar", value_len);
    ASSERT_EQ(ret, 0);

    attr = pcdoc_attr_next_sibling(doc, attr);
    ASSERT_NE(attr, nullptr);

    ret = pcdoc_attr_get_info(doc, attr,
            &local_name, &local_len,
            &qualified_name, &qualified_len,
            &value, &value_len);
    ASSERT_EQ(ret, 0);
    ret = strncmp(local_name, "class", local_len);
    ASSERT_EQ(ret, 0);
    ret = strncmp(qualified_name, "class", qualified_len);
    ASSERT_EQ(ret, 0);
    ret = strncmp(value, "foo bar\tfoobar", value_len);
    ASSERT_EQ(ret, 0);

    attr = pcdoc_attr_next_sibling(doc, attr);
    ASSERT_NE(attr, nullptr);

    attr = pcdoc_attr_next_sibling(doc, attr);
    ASSERT_EQ(attr, nullptr);

    attr = pcdoc_element_last_attr(doc, body);
    ASSERT_NE(attr, nullptr);

    ret = pcdoc_attr_get_info(doc, attr,
            &local_name, &local_len,
            &qualified_name, &qualified_len,
            &value, &value_len);
    ASSERT_EQ(ret, 0);
    ret = strncmp(local_name, "hvml:raw", local_len);
    ASSERT_EQ(ret, 0);
    ret = strncmp(qualified_name, "hvml:raw", qualified_len);
    ASSERT_EQ(ret, 0);
    ret = strncmp(value, "true", value_len);
    ASSERT_EQ(ret, 0);

    attr = pcdoc_attr_prev_sibling(doc, attr);
    ASSERT_NE(attr, nullptr);

    ret = pcdoc_attr_get_info(doc, attr,
            &local_name, &local_len,
            &qualified_name, &qualified_len,
            &value, &value_len);
    ASSERT_EQ(ret, 0);
    ret = strncmp(local_name, "class", local_len);
    ASSERT_EQ(ret, 0);
    ret = strncmp(qualified_name, "class", qualified_len);
    ASSERT_EQ(ret, 0);
    ret = strncmp(value, "foo bar\tfoobar", value_len);
    ASSERT_EQ(ret, 0);

    attr = pcdoc_attr_prev_sibling(doc, attr);
    ASSERT_NE(attr, nullptr);

    attr = pcdoc_attr_prev_sibling(doc, attr);
    ASSERT_EQ(attr, nullptr);

    unsigned int refc = purc_document_delete(doc);
    ASSERT_EQ(refc, 1);
}

static int my_element_cb(purc_document_t doc,
        pcdoc_element_t element, void *ctxt)
{
    int ret;
    const char *local_name;
    size_t local_len;

    ret = pcdoc_element_get_tag_name(doc, element, &local_name, &local_len,
            NULL, NULL, NULL, NULL);
    if (ret == 0) {
        std::string *str = static_cast<std::string *>(ctxt);
        str->append(local_name, local_len);
        str->push_back(',');
    }

    return PCDOC_TRAVEL_GOON;
}

static int my_element_cb_2(purc_document_t doc,
        pcdoc_element_t element, void *ctxt)
{
    int ret;
    const char *local_name;
    size_t local_len;

    ret = pcdoc_element_get_tag_name(doc, element, &local_name, &local_len,
            NULL, NULL, NULL, NULL);
    if (ret == 0) {
        if (strncmp(local_name, "li", 2) == 0) {
            return PCDOC_TRAVEL_STOP;
        }

        std::string *str = static_cast<std::string *>(ctxt);
        str->append(local_name, local_len);
        str->push_back(',');
    }

    return PCDOC_TRAVEL_GOON;
}

static int my_text_cb_2(purc_document_t doc,
        pcdoc_text_node_t text_node, void *ctxt)
{
    int ret;
    pcdoc_element_t element;
    const char *local_name;
    size_t local_len;

    pcdoc_node node;
    node.type = PCDOC_NODE_TEXT;
    node.data = text_node;
    element = pcdoc_node_get_parent(doc, node);
    ret = pcdoc_element_get_tag_name(doc, element, &local_name, &local_len,
            NULL, NULL, NULL, NULL);
    if (ret == 0) {
        if (strncmp(local_name, "ul", 2) == 0) {
            return PCDOC_TRAVEL_STOP;
        }

        const char *text;
        size_t text_len;
        ret = pcdoc_text_content_get_text(doc, text_node, &text, &text_len);
        if (ret == 0) {
            std::string *str = static_cast<std::string *>(ctxt);
            str->append(text, text_len);
            str->push_back('\n');
        }
    }

    return PCDOC_TRAVEL_GOON;
}

TEST(document, travel_descendants)
{
    purc_document_t doc = purc_document_load(PCDOC_K_TYPE_HTML,
            html_contents, strlen(html_contents));
    ASSERT_NE(doc, nullptr);

    /* head element */
    pcdoc_element_t head = purc_document_head(doc);
    ASSERT_NE(head, nullptr);

    pcdoc_node node, void_node;
    node = pcdoc_element_first_child(doc, head);
    ASSERT_EQ(node.type, PCDOC_NODE_ELEMENT);

    void_node = pcdoc_node_prev_sibling(doc, node);
    ASSERT_EQ(void_node.type, PCDOC_NODE_VOID);

    node = pcdoc_node_next_sibling(doc, node);
    ASSERT_EQ(node.type, PCDOC_NODE_ELEMENT);

    int ret;
    const char *local_name;
    size_t local_len;
    const char *prefix_name;
    size_t prefix_len;
    const char *ns_name;
    size_t ns_len;

    ret = pcdoc_element_get_tag_name(doc, node.elem, &local_name, &local_len,
            &prefix_name, &prefix_len, &ns_name, &ns_len);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(local_len, 4);
    ASSERT_EQ(prefix_len, 0);
    ASSERT_EQ(ns_len, 4);
    ret = strncmp(local_name, "link", local_len);
    ASSERT_EQ(ret, 0);

    node = pcdoc_element_last_child(doc, head);
    ASSERT_EQ(node.type, PCDOC_NODE_OTHERS);

    void_node = pcdoc_node_next_sibling(doc, node);
    ASSERT_EQ(void_node.type, PCDOC_NODE_VOID);

    node = pcdoc_node_prev_sibling(doc, node);
    ASSERT_EQ(node.type, PCDOC_NODE_ELEMENT);

    ret = pcdoc_element_get_tag_name(doc, node.elem, &local_name, &local_len,
            &prefix_name, &prefix_len, &ns_name, &ns_len);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(local_len, 4);
    ASSERT_EQ(prefix_len, 0);
    ASSERT_EQ(ns_len, 4);
    ret = strncmp(local_name, "link", local_len);
    ASSERT_EQ(ret, 0);

    /* body element */
    pcdoc_element_t body = purc_document_body(doc);
    ASSERT_NE(body, nullptr);

    pcdoc_element_t root = purc_document_root(doc);
    ASSERT_NE(root, nullptr);

    node.type = PCDOC_NODE_ELEMENT;
    node.data = body;
    pcdoc_element_t parent = pcdoc_node_get_parent(doc, node);
    ASSERT_EQ(parent, root);

    node.type = PCDOC_NODE_ELEMENT;
    node.data = root;
    parent = pcdoc_node_get_parent(doc, node);
    ASSERT_EQ(parent, nullptr);

    size_t nr_elements;
    size_t nr_text_nodes;
    size_t nr_data_nodes;

    ret = pcdoc_element_children_count(doc, root,
                &nr_elements, &nr_text_nodes, &nr_data_nodes);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(nr_elements, 2);
    ASSERT_EQ(nr_text_nodes, 0);
    ASSERT_EQ(nr_data_nodes, 0);

    ret = pcdoc_element_children_count(doc, body,
                &nr_elements, &nr_text_nodes, &nr_data_nodes);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(nr_elements, 1);
    ASSERT_EQ(nr_text_nodes, 0);
    ASSERT_EQ(nr_data_nodes, 0);

    pcdoc_element_t div;
    div = pcdoc_element_get_child_element(doc, body, 1);
    ASSERT_EQ(div, nullptr);

    div = pcdoc_element_get_child_element(doc, body, 0);
    ASSERT_NE(div, nullptr);

    ret = pcdoc_element_get_tag_name(doc, div, &local_name, &local_len,
            &prefix_name, &prefix_len, &ns_name, &ns_len);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(local_len, 3);
    ASSERT_EQ(prefix_len, 0);
    ASSERT_EQ(ns_len, 4);
    ret = strncmp(local_name, "div", local_len);
    ASSERT_EQ(ret, 0);

    std::string result;
    size_t nr;
    ret = pcdoc_travel_descendant_elements(doc,
        head, my_element_cb, &result, &nr);

    ASSERT_EQ(ret, 0);
    ASSERT_EQ(nr, 9);
    ASSERT_STREQ(result.c_str(), "head,title,link,link,link,link,link,link,link,");

    result = "";
    ret = pcdoc_travel_descendant_elements(doc,
        div, my_element_cb_2, &result, &nr);

    ASSERT_EQ(ret, -1);
    ASSERT_STREQ(result.c_str(), "div,h2,a,ul,");
    ASSERT_EQ(nr, 5);

    result = "";
    ret = pcdoc_travel_descendant_text_nodes(doc,
        div, my_text_cb_2, &result, &nr);

    ASSERT_EQ(ret, -1);
    ASSERT_STREQ(result.c_str(), "Quick Table of Contents\n");
    ASSERT_EQ(nr, 2);

    unsigned int refc = purc_document_delete(doc);
    ASSERT_EQ(refc, 1);
}

