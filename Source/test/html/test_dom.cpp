/*
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

#undef NDEBUG

#include "purc.h"
#include "private/list.h"
#include "private/html.h"
#include "private/dom.h"
#include "purc-html.h"
#include "./html/interfaces/document.h"
#include "private/interpreter.h"
#include "html_ops.h"

#include <gtest/gtest.h>

#include <stdarg.h>

#define lxb_status_t                               int
#define LXB_STATUS_OK                              PCHTML_STATUS_OK
#define lxb_char_t                                 unsigned char

#define LXB_HTML_SERIALIZE_OPT_UNDEF               PCHTML_HTML_SERIALIZE_OPT_UNDEF

#define FAILED(_fmt, ...) do {                     \
    fprintf(stderr, "" _fmt "\n", ##__VA_ARGS__);  \
    abort();                                       \
} while (0)

#define serialize_node(_node) html_dom_dump_node(_node)

#define serializer_callback 0
#define pchtml_html_serialize_pretty_tree_cb(_node, _opt, _n, _cb, _ext) \
({                                                                       \
    (void)_opt;                                                          \
    (void)_n;                                                            \
    (void)_cb;                                                           \
    (void)_ext;                                                          \
    html_dom_dump_node(_node);                                           \
    0;                                                                   \
})

pchtml_html_document_t*
load_document(const char *html)
{
    lxb_status_t status;
    pchtml_html_parser_t *parser;
    pchtml_html_document_t *doc;

    /* Initialization */
    parser = pchtml_html_parser_create();
    status = pchtml_html_parser_init(parser);

    if (status != LXB_STATUS_OK) {
        if (parser) {
            pchtml_html_parser_destroy(parser);
            return NULL;
        }
    }

    doc = pchtml_html_parse_with_buf(parser, (const lxb_char_t*)html, strlen(html));
    pchtml_html_parser_destroy(parser);

    if (!doc)
        return NULL;

    if (pcdom_interface_document(doc) != pcdom_interface_node(doc)->owner_document) {
        abort();
    }
    if (pcdom_interface_document(doc) != pcdom_interface_document(doc)) {
        abort();
    }
    if ((void*)pcdom_interface_document(doc) != (void*)doc) {
        abort();
    }
    if ((void*)doc != (void*)pcdom_interface_node(doc)->owner_document) {
        abort();
    }

    return doc;
}

pcdom_element_t*
append_element(pcdom_element_t *parent, const char *tag)
{
    pcdom_document_t *doc = pcdom_interface_node(parent)->owner_document;
    const lxb_char_t *tag_name = (const lxb_char_t*)tag;
    size_t tag_name_len = strlen(tag);

    pcdom_element_t *element;
    element = pcdom_document_create_element(doc, tag_name, tag_name_len, NULL, false);
    if (element == NULL) {
        return NULL;
    }

    pcdom_node_append_child(pcdom_interface_node(parent), pcdom_interface_node(element));

    return element;
}

pcdom_text_t*
append_content(pcdom_element_t *parent, const char *text)
{
    pcdom_document_t *doc = pcdom_interface_node(parent)->owner_document;
    const lxb_char_t *content = (const lxb_char_t*)text;
    size_t content_len = strlen(text);

    pcdom_text_t *text_node;
    text_node = pcdom_document_create_text_node(doc, content, content_len);
    if (text_node == NULL)
        return NULL;

    pcdom_node_append_child(pcdom_interface_node(parent), pcdom_interface_node(text_node));

    return text_node;
}

pcdom_attr_t*
set_attribute(pcdom_element_t *parent, const char *name, const char *value)
{
    pcdom_attr_t *attr;
    attr = pcdom_element_set_attribute(parent, (const lxb_char_t*)name, strlen(name), (const lxb_char_t*)value, strlen(value));
    assert(pcdom_interface_node(attr)->owner_document == pcdom_interface_node(parent)->owner_document);

    return attr;
}

enum merge_opt {
    MERGE_APPEND,
    MERGE_REPLACE,
};

pcdom_node_t*
merge_inner_html(pcdom_element_t *parent, const char *inner_html, enum merge_opt opt)
{
    pcdom_node_t *node, *child;
    pcdom_node_t *root = pcdom_interface_node(parent);
    pchtml_html_document_t *doc = pchtml_html_interface_document(root->owner_document);
    assert(doc);

    node = pchtml_html_document_parse_fragment_with_buf(doc, parent, (const lxb_char_t*)inner_html, strlen(inner_html));
    assert(node);

    if (opt == MERGE_REPLACE) {
        while (root->first_child != NULL) {
            pcdom_node_destroy_deep(root->first_child);
        }
    }

    pcdom_node_t *first = node->first_child;

    while (node->first_child != NULL) {
        child = node->first_child;

        pcdom_node_remove(child);
        pcdom_node_append_child(root, child);
        fprintf(stderr, "%s[%d]:%s(): child[%p] added into parent[%p]\n",
                __FILE__, __LINE__, __func__,
                (void*)child, (void*)root);
    }

    pcdom_node_destroy(node);

    return first;
}

pcdom_node_t*
append_inner_html(pcdom_element_t *parent, const char *inner_html)
{
    return merge_inner_html(parent, inner_html, MERGE_APPEND);
}

pcdom_node_t*
replace_inner_html(pcdom_element_t *parent, const char *inner_html)
{
    if (0)
        return merge_inner_html(parent, inner_html, MERGE_REPLACE);

    pchtml_html_element_t *first;
    first = pchtml_html_element_inner_html_set_with_buf(pchtml_html_interface_element(parent), (const unsigned char*)inner_html, strlen(inner_html));
    assert(first);
    assert(pcdom_interface_element(first) == parent);

    return pcdom_interface_node(first);
}

void test0(void)
{
    const char *html = "<div id='a'>xyz</div>";
    pchtml_html_document_t *doc = load_document(html);
    pchtml_html_document_destroy(doc);
}

void test1(void)
{
    lxb_status_t status;
    const char *html = "<div id='a'></div>";
    pchtml_html_document_t *doc = load_document(html);

    /* Serialization */
    printf("Document:\n");

    status = pchtml_html_serialize_pretty_tree_cb(pcdom_interface_node(doc),
                                               LXB_HTML_SERIALIZE_OPT_UNDEF,
                                               0, serializer_callback, NULL);
    if (status != LXB_STATUS_OK) {
        FAILED("Failed to serialization HTML tree");
    }

    pchtml_html_body_element_t *body = pchtml_html_document_body_element(doc);
    const lxb_char_t *tag_name = (const lxb_char_t*)"div";
    size_t tag_name_len = strlen((const char*)tag_name);
    const lxb_char_t *content = (const lxb_char_t*)"hello world";
    size_t content_len = strlen((const char*)content);

    pcdom_element_t *element;
    pcdom_text_t *text;
    element = pcdom_document_create_element(&doc->dom_document,
            tag_name, tag_name_len, NULL, false);
    if (element == NULL) {
        FAILED("Failed to create element for tag \"%s\"",
                (const char *) tag_name);
    }

    printf("Create element by tag name \"%s\" and append text node: ",
            (const char *) tag_name);

    text = pcdom_document_create_text_node(&doc->dom_document,
            content, content_len);
    if (text == NULL) {
        FAILED("Failed to create text node for \"%s\"",
                (const char *) content);
    }

    pcdom_node_append_child(pcdom_interface_node(element),
            pcdom_interface_node(text));

    serialize_node(pcdom_interface_node(element));

    pcdom_node_append_child(pcdom_interface_node(body),
            pcdom_interface_node(element));

    status = pchtml_html_serialize_pretty_tree_cb(pcdom_interface_node(doc),
                                               LXB_HTML_SERIALIZE_OPT_UNDEF,
                                               0, serializer_callback, NULL);
    if (status != LXB_STATUS_OK) {
        FAILED("Failed to serialization HTML tree");
    }

    /* Destroy */
    pchtml_html_document_destroy(doc);

    /*
    "<hvml target=\"html\" lang=\"en\">"
    "    <head>"
    "    </head>"
    ""
    "    <body>"
    "        <span id=\"clock\">def</span>"
    "        <div>"
    "            <xinput xtype=\"xt\" xype=\"abd\" />"
    "        </div>"
    "        <update on=\"#clock\" at=\"textContent\" to=\"displace\" with=\"xyz\" />"
    "    </body>"
    ""
    "</hvml>";
    */

    doc = NULL;
    if (1) {
        const char *html = "<html/>";
        printf("Loading document:\n%s\n", html);
        doc = load_document(html);
        assert(doc);
        printf("Serializing document:\n");
        assert(LXB_STATUS_OK == pchtml_html_serialize_pretty_tree_cb(pcdom_interface_node(doc), LXB_HTML_SERIALIZE_OPT_UNDEF, 0, serializer_callback, NULL));
    }

    pchtml_html_head_element_t *head = NULL;
    if (1 && doc) {
        head = pchtml_html_document_head_element(doc);
        assert(head);

        body = pchtml_html_document_body_element(doc);
        assert(body);

        pcdom_element_t *span;
        span = append_element(pcdom_interface_element(body), "span");
        assert(span);

        pcdom_attr_t *attr;
        attr = set_attribute(span, "id", "clock");
        assert(attr);

        pcdom_text_t *def;
        def = append_content(span, "def");
        assert(def);

        pcdom_element_t *div;
        div = append_element(pcdom_interface_element(body), "div");
        assert(div);

        pcdom_element_t *xinput;
        xinput = append_element(div, "xinput");
        assert(xinput);

        attr = set_attribute(xinput, "xtype", "xt");
        assert(attr);

        attr = set_attribute(xinput, "xype", "abd");
        assert(attr);

        if (0) {
            pchtml_html_element_t *ghi;
            ghi = pchtml_html_element_inner_html_set_with_buf(pchtml_html_interface_element(body), (const unsigned char*)"hello", 5);
            assert(ghi);
        }
        if (0) {
            pchtml_html_element_t *ghi;
            ghi = pchtml_html_element_inner_html_set_with_buf(pchtml_html_interface_element(span), (const unsigned char*)"hello", 5);
            assert(ghi);
        }
        if (0) {
            pchtml_html_element_t *ghi;
            ghi = pchtml_html_element_inner_html_set_with_buf(pchtml_html_interface_element(head), (const unsigned char*)"hello", 5);
            assert(ghi);
        }
        if (1) {
            pcdom_node_t *node, *child;
            pcdom_node_t *root = pcdom_interface_node(span);
            pchtml_html_document_t *docx = pchtml_html_interface_document(root->owner_document);
            assert(doc == docx);

            const char *inner = "<div>hello</div>";
            node = pchtml_html_document_parse_fragment_with_buf(doc, span, (const lxb_char_t*)inner, strlen(inner));
            assert(node);

            if (0) {
                while (root->first_child != NULL) {
                    pcdom_node_destroy_deep(root->first_child);
                }
            }

            while (node->first_child != NULL) {
                child = node->first_child;

                pcdom_node_remove(child);
                pcdom_node_append_child(root, child);
            }

            pcdom_node_destroy(node);
        }
    }

    if (doc) {
        printf("Serializing document:\n");
        assert(LXB_STATUS_OK == pchtml_html_serialize_pretty_tree_cb(pcdom_interface_node(doc), LXB_HTML_SERIALIZE_OPT_UNDEF, 0, serializer_callback, NULL));
        pchtml_html_document_destroy(doc);
    }
}

void test2(void)
{
    pchtml_html_body_element_t *body = NULL;
    pchtml_html_head_element_t *head = NULL;
    pchtml_html_document_t *doc = NULL;

    /*
    "<hvml target=\"html\" lang=\"en\">"
    "    <head>"
    "    </head>"
    ""
    "    <body>"
    "        <span id=\"clock\">def</span>"
    "        <div>"
    "            <xinput xtype=\"xt\" xype=\"abd\" />"
    "        </div>"
    "        <update on=\"#clock\" at=\"textContent\" to=\"displace\" with=\"xyz\" />"
    "    </body>"
    ""
    "</hvml>";
    */

    const char *html = "<html/>";
    printf("Loading document:\n%s\n", html);
    doc = load_document(html);
    assert(doc);
    printf("Serializing document:\n");
    assert(LXB_STATUS_OK == pchtml_html_serialize_pretty_tree_cb(pcdom_interface_node(doc), LXB_HTML_SERIALIZE_OPT_UNDEF, 0, serializer_callback, NULL));

    head = pchtml_html_document_head_element(doc);
    assert(head);

    body = pchtml_html_document_body_element(doc);
    assert(body);

    pcdom_element_t *span;
    span = append_element(pcdom_interface_element(body), "span");
    assert(span);

    pcdom_attr_t *attr;
    attr = set_attribute(span, "id", "clock");
    assert(attr);

    fprintf(stderr, "%s[%d]:%s()\n", __FILE__, __LINE__, __func__);
    // assert(0 == append_inner_html(span, "def"));
    assert(pchtml_html_element_inner_html_set_with_buf(pchtml_html_interface_element(span), (const unsigned char*)"hello", 5));
    fprintf(stderr, "%s[%d]:%s()\n", __FILE__, __LINE__, __func__);

    pcdom_element_t *div;
    div = append_element(pcdom_interface_element(body), "div");
    assert(div);

    pcdom_element_t *xinput;
    xinput = append_element(div, "xinput");
    assert(xinput);

    attr = set_attribute(xinput, "xtype", "xt");
    assert(attr);

    fprintf(stderr, "%s[%d]:%s()\n", __FILE__, __LINE__, __func__);
    // assert(0 == replace_inner_html(span, "<abc/>"));
    assert(pchtml_html_element_inner_html_set_with_buf(pchtml_html_interface_element(span), (const unsigned char*)"world", 5));

    attr = set_attribute(xinput, "xype", "abd");
    assert(attr);

    printf("Serializing document:\n");
    assert(LXB_STATUS_OK == pchtml_html_serialize_pretty_tree_cb(pcdom_interface_node(doc), LXB_HTML_SERIALIZE_OPT_UNDEF, 0, serializer_callback, NULL));
    pchtml_html_document_destroy(doc);
}

pcdom_node_t*
append_child(pcdom_element_t *parent, const char *inner_html)
{
    pcdom_element_t *anchor;
    anchor = append_element(parent, "div");
    if (!anchor)
        return NULL;

    pcdom_node_t *anchor_node = pcdom_interface_node(anchor);

    pchtml_html_element_t *root;
    root = pchtml_html_element_inner_html_set_with_buf(pchtml_html_interface_element(anchor), (const unsigned char*)inner_html, strlen(inner_html));
    if (!root) {
        pcdom_node_destroy(anchor_node);
        // pcdom_element_destroy(anchor);
        return NULL;
    }
    assert(root == pchtml_html_interface_element(anchor));
    pcdom_node_t *first_node = anchor_node->first_child;
    while (anchor_node->first_child) {
        pcdom_node_t *child = anchor_node->first_child;
        pcdom_node_remove(child);
        pcdom_node_append_child(pcdom_interface_node(parent), child);
    }

    pcdom_node_destroy(anchor_node);
    // pcdom_element_destroy(anchor);

    return first_node;
}

pcdom_node_t*
set_child(pcdom_element_t *parent, const char *inner_html)
{
    fprintf(stderr, "%s[%d]:%s()\n", __FILE__, __LINE__, __func__);
    assert(LXB_STATUS_OK == pchtml_html_serialize_pretty_tree_cb(pcdom_interface_node(parent), LXB_HTML_SERIALIZE_OPT_UNDEF, 0, serializer_callback, NULL));

    pcdom_element_t *anchor;
    anchor = append_element(parent, "div");
    if (!anchor)
        return NULL;

    pcdom_node_t *anchor_node = pcdom_interface_node(anchor);

    pchtml_html_element_t *root;
    root = pchtml_html_element_inner_html_set_with_buf(pchtml_html_interface_element(anchor), (const unsigned char*)inner_html, strlen(inner_html));
    if (!root) {
        pcdom_node_destroy(anchor_node);
        // pcdom_element_destroy(anchor);
        return NULL;
    }
    assert(root == pchtml_html_interface_element(anchor));

    pcdom_node_t *first_node;
    first_node = anchor_node->first_child;
    assert(first_node);

    pcdom_node_t *parent_node = pcdom_interface_node(parent);
    pcdom_node_t *child = parent_node->first_child;
    while (child) {
        pcdom_node_t *next = child->next;
        if (child != anchor_node) {
            pcdom_node_destroy_deep(child);
        }
        else {
            pcdom_node_remove(child);
        }
        child = next;
    }

    while (anchor_node->first_child) {
        pcdom_node_t *child = anchor_node->first_child;
        pcdom_node_remove(child);
        pcdom_node_append_child(pcdom_interface_node(parent), child);
    }

    pcdom_node_destroy(anchor_node);
    // pcdom_element_destroy(anchor);

    return first_node;
}

void test3(void)
{
    pchtml_html_body_element_t *body = NULL;
    pchtml_html_head_element_t *head = NULL;
    pchtml_html_document_t *doc = NULL;

    /*
    "<hvml target=\"html\" lang=\"en\">"
    "    <head>"
    "    </head>"
    ""
    "    <body>"
    "        <span id=\"clock\">def</span>"
    "        <div>"
    "            <xinput xtype=\"xt\" xype=\"abd\" />"
    "        </div>"
    "        <update on=\"#clock\" at=\"textContent\" to=\"displace\" with=\"xyz\" />"
    "    </body>"
    ""
    "</hvml>";
    */

    const char *html = "<html/>";
    printf("Loading document:\n%s\n", html);
    doc = load_document(html);
    assert(doc);
    printf("Serializing document:\n");
    assert(LXB_STATUS_OK == pchtml_html_serialize_pretty_tree_cb(pcdom_interface_node(doc), LXB_HTML_SERIALIZE_OPT_UNDEF, 0, serializer_callback, NULL));

    head = pchtml_html_document_head_element(doc);
    assert(head);

    body = pchtml_html_document_body_element(doc);
    assert(body);

    const char *inner;
    pchtml_html_element_t *span;

    inner = "hello";
    span = pchtml_html_element_inner_html_set_with_buf(pchtml_html_interface_element(body), (const unsigned char*)inner, strlen(inner));
    assert(span);
    assert(span == pchtml_html_interface_element(body));
    assert(LXB_STATUS_OK == pchtml_html_serialize_pretty_tree_cb(pcdom_interface_node(span), LXB_HTML_SERIALIZE_OPT_UNDEF, 0, serializer_callback, NULL));

    inner = "foo<hello>bar</hello>";
    span = pchtml_html_element_inner_html_set_with_buf(pchtml_html_interface_element(body), (const unsigned char*)inner, strlen(inner));
    assert(span);
    assert(span == pchtml_html_interface_element(body));
    assert(LXB_STATUS_OK == pchtml_html_serialize_pretty_tree_cb(pcdom_interface_node(span), LXB_HTML_SERIALIZE_OPT_UNDEF, 0, serializer_callback, NULL));

    printf("Serializing document:\n");
    assert(LXB_STATUS_OK == pchtml_html_serialize_pretty_tree_cb(pcdom_interface_node(doc), LXB_HTML_SERIALIZE_OPT_UNDEF, 0, serializer_callback, NULL));
    pchtml_html_document_destroy(doc);
}

void test4(void)
{
    pchtml_html_body_element_t *body = NULL;
    pchtml_html_head_element_t *head = NULL;
    pchtml_html_document_t *doc = NULL;

    /*
    "<hvml target=\"html\" lang=\"en\">"
    "    <head>"
    "    </head>"
    ""
    "    <body>"
    "        <span id=\"clock\">def</span>"
    "        <div>"
    "            <xinput xtype=\"xt\" xype=\"abd\" />"
    "        </div>"
    "        <update on=\"#clock\" at=\"textContent\" to=\"displace\" with=\"xyz\" />"
    "    </body>"
    ""
    "</hvml>";
    */

    const char *html = "<html/>";
    printf("Loading document:\n%s\n", html);
    doc = load_document(html);
    assert(doc);
    printf("Serializing document:\n");
    assert(LXB_STATUS_OK == pchtml_html_serialize_pretty_tree_cb(pcdom_interface_node(doc), LXB_HTML_SERIALIZE_OPT_UNDEF, 0, serializer_callback, NULL));

    head = pchtml_html_document_head_element(doc);
    assert(head);

    body = pchtml_html_document_body_element(doc);
    assert(body);

    pcdom_node_t *span;
    span = append_child(pcdom_interface_element(body), "<span id=\"clock\"></span>");
    assert(span);
    assert(LXB_STATUS_OK == pchtml_html_serialize_pretty_tree_cb(pcdom_interface_node(span), LXB_HTML_SERIALIZE_OPT_UNDEF, 0, serializer_callback, NULL));

    pcdom_node_t *def;
    def = append_child(pcdom_interface_element(span), "def");
    assert(def);
    assert(LXB_STATUS_OK == pchtml_html_serialize_pretty_tree_cb(pcdom_interface_node(def), LXB_HTML_SERIALIZE_OPT_UNDEF, 0, serializer_callback, NULL));

    pcdom_node_t *div;
    div = append_child(pcdom_interface_element(body), "<div></div>");
    assert(div);
    assert(LXB_STATUS_OK == pchtml_html_serialize_pretty_tree_cb(pcdom_interface_node(div), LXB_HTML_SERIALIZE_OPT_UNDEF, 0, serializer_callback, NULL));

    pcdom_node_t *xinput;
    xinput = append_child(pcdom_interface_element(div), "<xinput xtype=\"xt\" xype=\"abd\"></xinput>");
    assert(xinput);
    assert(LXB_STATUS_OK == pchtml_html_serialize_pretty_tree_cb(pcdom_interface_node(xinput), LXB_HTML_SERIALIZE_OPT_UNDEF, 0, serializer_callback, NULL));

    pcdom_node_t *xyz;
    xyz = set_child(pcdom_interface_element(span), "xyz");
    assert(xyz);
    assert(LXB_STATUS_OK == pchtml_html_serialize_pretty_tree_cb(pcdom_interface_node(xyz), LXB_HTML_SERIALIZE_OPT_UNDEF, 0, serializer_callback, NULL));

    printf("Serializing document:\n");
    assert(LXB_STATUS_OK == pchtml_html_serialize_pretty_tree_cb(pcdom_interface_node(doc), LXB_HTML_SERIALIZE_OPT_UNDEF, 0, serializer_callback, NULL));
    pchtml_html_document_destroy(doc);
}

void test5(void)
{
    pchtml_html_body_element_t *body = NULL;
    pchtml_html_head_element_t *head = NULL;
    pchtml_html_document_t *doc = NULL;

    /*
    "<hvml target=\"html\" lang=\"en\">"
    "    <head>"
    "    </head>"
    ""
    "    <body>"
    "        <span id=\"clock\">def</span>"
    "        <div>"
    "            <xinput xtype=\"xt\" xype=\"abd\" />"
    "        </div>"
    "        <update on=\"#clock\" at=\"textContent\" to=\"displace\" with=\"xyz\" />"
    "    </body>"
    ""
    "</hvml>";
    */

    const char *html = "<html/>";
    printf("Loading document:\n%s\n", html);
    doc = load_document(html);
    assert(doc);
    printf("Serializing document:\n");
    assert(LXB_STATUS_OK == pchtml_html_serialize_pretty_tree_cb(pcdom_interface_node(doc), LXB_HTML_SERIALIZE_OPT_UNDEF, 0, serializer_callback, NULL));

    head = pchtml_html_document_head_element(doc);
    assert(head);

    body = pchtml_html_document_body_element(doc);
    assert(body);

    pcdom_element_t *span;
    span = append_element(pcdom_interface_element(body), "span");
    assert(span);

    pcdom_attr_t *attr;
    attr = set_attribute(span, "id", "clock");
    assert(attr);

#if 1
    pcdom_text_t *def;
    def = append_content(span, "def");
    assert(def);
#else
    pcdom_node_t *def;
    def = set_child(pcdom_interface_element(span), "def");
    assert(def);
#endif
    assert(LXB_STATUS_OK == pchtml_html_serialize_pretty_tree_cb(pcdom_interface_node(def), LXB_HTML_SERIALIZE_OPT_UNDEF, 0, serializer_callback, NULL));

    pcdom_element_t *div;
    div = append_element(pcdom_interface_element(body), "div");
    assert(div);
    assert(LXB_STATUS_OK == pchtml_html_serialize_pretty_tree_cb(pcdom_interface_node(div), LXB_HTML_SERIALIZE_OPT_UNDEF, 0, serializer_callback, NULL));

    pcdom_element_t *xinput;
    xinput = append_element(div, "xinput");
    assert(xinput);

    attr = set_attribute(xinput, "xtype", "xt");
    assert(attr);

    attr = set_attribute(xinput, "xype", "abd");
    assert(attr);

    pcdom_node_t *xyz;
    xyz = set_child(pcdom_interface_element(span), "xyz");
    assert(xyz);
    assert(LXB_STATUS_OK == pchtml_html_serialize_pretty_tree_cb(pcdom_interface_node(xyz), LXB_HTML_SERIALIZE_OPT_UNDEF, 0, serializer_callback, NULL));

    printf("Serializing document:\n");
    assert(LXB_STATUS_OK == pchtml_html_serialize_pretty_tree_cb(pcdom_interface_node(doc), LXB_HTML_SERIALIZE_OPT_UNDEF, 0, serializer_callback, NULL));
    pchtml_html_document_destroy(doc);
}

TEST(inner, basic)
{
    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_HTML, "cn.fmsoft.hybridos.test",
            "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    if (0) test0();
    if (0) test1();
    if (0) test2();
    if (0) test3();
    if (0) test4();
    test5();

    purc_cleanup ();
}

