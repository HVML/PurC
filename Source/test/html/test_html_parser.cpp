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

#include "purc.h"
#include "private/list.h"
#include "private/html.h"
#include "private/dom.h"

#include <limits.h>
#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// test html parser for whole html file
TEST(html, html_parser_html_file_x)
{
    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_HTML, "cn.fmsoft.hybridos.test",
            "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    static const char html[][64] = {
        "<!DOCT",
        "YPE htm",
        "l>",
        "<html><head>",
        "<ti",
        "tle>HTML chun",
        "ks parsing</",
        "title>",
        "</head><bod",
        "y><div cla",
        "ss=",
        "\"bestof",
        "class",
        "\">",
        "good for 我 me",
        "</div>",
        "\0"
    };

    purc_rwstream_t io;
    io= purc_rwstream_new_buffer(1024, 1024*8);
    ASSERT_NE(io, nullptr);

    for (size_t i = 0; html[i][0] != '\0'; i++) {
        const char *buf = html[i];
        size_t      len = strlen(buf);
        ssize_t sz;
        sz = purc_rwstream_write(io, buf, len);
        ASSERT_GT(sz, 0);
        ASSERT_EQ((size_t)sz, len);
    }

    off_t off;
    off = purc_rwstream_seek(io, 0, SEEK_SET);
    ASSERT_NE(off, -1);

    pchtml_html_document_t *doc;
    doc = pchtml_html_document_create();
    ASSERT_NE(doc, nullptr);

    unsigned int r;
    r = pchtml_html_document_parse(doc, io);
    ASSERT_EQ(r, PCHTML_STATUS_OK);

    purc_rwstream_destroy(io);

    io = purc_rwstream_new_from_unix_fd(dup(fileno(stderr)));

    int n;
    n = pchtml_doc_write_to_stream(doc, io);
    ASSERT_EQ(n, 0);

    size_t size = 0;
    char* buffer = (char*)purc_rwstream_get_mem_buffer (io, &size);
    char buf[1000] = {0,};
    memcpy(buf, buffer, size);
    printf("%s\n", buf);

    pchtml_html_document_destroy(doc);
    purc_rwstream_destroy(io);

    purc_cleanup ();
}

TEST(html, html_parser_chunk)
{
    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_HTML, "cn.fmsoft.hybridos.test",
            "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    static const char html[][64] = {
        "<!DOCT",
        "YPE htm",
        "l>",
        "<html><head>",
        "<ti",
        "tle>HTML chun",
        "ks parsing</",
        "title>",
        "</head><bod",
        "y><div cla",
        "ss=",
        "\"bestof",
        "class",
        "\">",
        "good for 我 me",
        "</div>",
        "\0"
    };

    purc_rwstream_t io;

    pchtml_html_document_t *doc;
    doc = pchtml_html_document_create();
    ASSERT_NE(doc, nullptr);
    unsigned int ur;
    ur = pchtml_html_document_parse_chunk_begin(doc);
    ASSERT_EQ(ur, PCHTML_STATUS_OK);

    for (size_t i = 0; html[i][0] != '\0'; i++) {
        const char *buf = html[i];
        size_t      len = strlen(buf);
        ur = pchtml_html_document_parse_chunk(doc, (unsigned char*)buf, len);
        ASSERT_EQ(ur, PCHTML_STATUS_OK);
    }

    ur = pchtml_html_document_parse_chunk_end(doc);
    ASSERT_EQ(ur, PCHTML_STATUS_OK);

    io = purc_rwstream_new_from_unix_fd(dup(fileno(stderr)));

    int n;
    n = pchtml_doc_write_to_stream(doc, io);
    ASSERT_EQ(n, 0);
    purc_rwstream_destroy(io);

    pchtml_html_document_destroy(doc);

    purc_cleanup ();
}

TEST(html, load_from_html)
{
    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_HTML, "cn.fmsoft.hybridos.test",
            "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    char this_file[] = __FILE__;
    char urls_txt[PATH_MAX];
    char cmd[1024*8];
    int n;
    FILE *furls;
    purc_rwstream_t out;

    out = purc_rwstream_new_from_unix_fd(dup(fileno(stderr)));
    ASSERT_NE(out, nullptr);

    n = snprintf(urls_txt, sizeof(urls_txt),
            "%s/urls.txt", dirname(this_file));
    ASSERT_LT(n, sizeof(urls_txt));

    furls = fopen(urls_txt, "r");
    ASSERT_NE(furls, nullptr);
    ssize_t ssz;
    char *line = NULL;
    size_t sz = 0;

    while ((ssz = getline(&line, &sz, furls)) != -1) {
        FILE *fin;
        purc_rwstream_t in;
        pchtml_html_document_t *doc;
        unsigned int ur;

        int n, r;

        char *p;
        p = strchr(line, '\n');
        if (p) *p = '\0';
        p = strchr(line, '\r');
        if (p) *p = '\0';
        if (strlen(line)==0) continue;

        n = snprintf(cmd, sizeof(cmd), "curl --no-progress-meter %s", line);
        ASSERT_LT(n, sizeof(cmd));

        std::cerr << "curling: [" << line << "]" << std::endl;
        std::cerr << "curling: [" << cmd << "]" << std::endl;
        fin = popen(cmd, "r");
        ASSERT_NE(fin, nullptr);

        in = purc_rwstream_new_from_fp(fdopen(dup(fileno(fin)), "r"));
        ASSERT_NE(in, nullptr);

        doc = pchtml_html_document_create();
        ASSERT_NE(doc, nullptr);
        ur = pchtml_html_document_parse(doc, in);
        ASSERT_EQ(ur, PCHTML_STATUS_OK);

        r = pchtml_doc_write_to_stream(doc, out);
        ASSERT_EQ(r, 0);

        pchtml_html_document_destroy(doc);
        purc_rwstream_destroy(in);

        pclose(fin);
    }

    purc_rwstream_destroy(out);

    if (line) {
        free(line);
    }

    purc_cleanup ();
}

static inline void
process_html_document(purc_rwstream_t in, bool *ok)
{
    *ok = false;
    pchtml_html_document_t *doc = pchtml_html_document_create();
    ASSERT_NE(doc, nullptr);

    unsigned int ur = pchtml_html_document_parse(doc, in);
    ASSERT_EQ(ur, PCHTML_STATUS_OK);

    struct pcdom_document *document;
    pcdom_document_type_t *doc_type;
    const unsigned char *doctype;
    size_t len;
    document = pchtml_doc_get_document(doc);
    ASSERT_NE(document, nullptr);
    doc_type = document->doctype;
    ASSERT_NE(doc_type, nullptr);

    doctype = pcdom_document_type_name(doc_type, &len);
    fprintf(stderr, "doctype: %zd[%s]\n", len, doctype);

    const unsigned char *s_public;
    s_public = pcdom_document_type_public_id(doc_type, &len);
    fprintf(stderr, "doctype.public: %zd[%s]\n", len, s_public);

    const unsigned char *s_system;
    s_system = pcdom_document_type_system_id(doc_type, &len);
    fprintf(stderr, "doctype.system: %zd[%s]\n", len, s_system);

    pchtml_html_document_destroy(doc);
}

TEST(html, document)
{
    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_HTML, "cn.fmsoft.hybridos.test",
            "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    const char *htmls[] = {
        "<!DOCTYPE html><html></html>",
        "<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\"><html/>",
    };

    for (size_t i=0; i<PCA_TABLESIZE(htmls); ++i) {
        const char *html = htmls[i];
        purc_rwstream_t rs = purc_rwstream_new_from_mem((void*)html, strlen(html));
        ASSERT_NE(rs, nullptr);
        bool ok;
        process_html_document(rs, &ok);
        purc_rwstream_destroy(rs);
    }

    purc_cleanup ();
}

static inline void
print_node_type(pcdom_node_type_t type)
{
    const char *s = "";
    switch (type)
    {
        case PCDOM_NODE_TYPE_UNDEF:
            s = "PCDOM_NODE_TYPE_UNDEF";
            break;
        case PCDOM_NODE_TYPE_ELEMENT:
            s = "PCDOM_NODE_TYPE_ELEMENT";
            break;
        case PCDOM_NODE_TYPE_ATTRIBUTE:
            s = "PCDOM_NODE_TYPE_ATTRIBUTE";
            break;
        case PCDOM_NODE_TYPE_TEXT:
            s = "PCDOM_NODE_TYPE_TEXT";
            break;
        case PCDOM_NODE_TYPE_CDATA_SECTION:
            s = "PCDOM_NODE_TYPE_CDATA_SECTION";
            break;
        case PCDOM_NODE_TYPE_ENTITY_REFERENCE:
            s = "PCDOM_NODE_TYPE_ENTITY_REFERENCE";
            break;
        case PCDOM_NODE_TYPE_ENTITY:
            s = "PCDOM_NODE_TYPE_ENTITY";
            break;
        case PCDOM_NODE_TYPE_PROCESSING_INSTRUCTION:
            s = "PCDOM_NODE_TYPE_PROCESSING_INSTRUCTION";
            break;
        case PCDOM_NODE_TYPE_COMMENT:
            s = "PCDOM_NODE_TYPE_COMMENT";
            break;
        case PCDOM_NODE_TYPE_DOCUMENT:
            s = "PCDOM_NODE_TYPE_DOCUMENT";
            break;
        case PCDOM_NODE_TYPE_DOCUMENT_TYPE:
            s = "PCDOM_NODE_TYPE_DOCUMENT_TYPE";
            break;
        case PCDOM_NODE_TYPE_DOCUMENT_FRAGMENT:
            s = "PCDOM_NODE_TYPE_DOCUMENT_FRAGMENT";
            break;
        case PCDOM_NODE_TYPE_NOTATION:
            s = "PCDOM_NODE_TYPE_NOTATION";
            break;
        case PCDOM_NODE_TYPE_LAST_ENTRY:
            s = "PCDOM_NODE_TYPE_LAST_ENTRY";
            break;
        default:
            ASSERT_TRUE(0);
            break;
    }
    fprintf(stderr, "node_type: [%s]\n", s);
}

static inline void
print_attr_id(const char *s, pcdom_attr_id_t *attr)
{
    fprintf(stderr, "attr_id:%s:[%ld]\n", s, *attr);
}

static inline void
print_attr(pcdom_attr_t *attr)
{
    const unsigned char *s;
    size_t len;

    s = pcdom_attr_qualified_name(attr, &len);
    fprintf(stderr, "attr.qualified_name:[%s]\n", s);

    s = pcdom_attr_local_name(attr, &len);
    fprintf(stderr, "attr.local_name:[%s]\n", s);

    s = pcdom_attr_value(attr, &len);
    fprintf(stderr, "attr.value:[%s]\n", s);
}

static inline void
print_element_attrs(pcdom_node_t *node)
{
    struct pcdom_element *elem;
    elem = container_of(node, struct pcdom_element, node);

    pcdom_attr_t *attr = pcdom_element_first_attribute(elem);
    for (; attr; attr = pcdom_element_next_attribute(attr)) {
        print_attr(attr);
    }
}

static inline void
print_element(pcdom_node_t *node)
{
    struct pcdom_element *elem;
    elem = container_of(node, struct pcdom_element, node);

    const unsigned char *s;
    size_t len;
    s = pcdom_element_qualified_name(elem, &len);
    fprintf(stderr, "qualified_name: [%s]\n", s);

    s = pcdom_element_qualified_name_upper(elem, &len);
    fprintf(stderr, "qualified_name_upper: [%s]\n", s);

    s = pcdom_element_local_name(elem, &len);
    fprintf(stderr, "local_name: [%s]\n", s);

    s = pcdom_element_prefix(elem, &len);
    fprintf(stderr, "prefix: [%s]\n", s);

    s = pcdom_element_tag_name(elem, &len);
    fprintf(stderr, "tag_name: [%s]\n", s);

    s = pcdom_element_id(elem, &len);
    fprintf(stderr, "id: [%s]\n", s);

    s = pcdom_element_class(elem, &len);
    fprintf(stderr, "class: [%s]\n", s);

    print_element_attrs(node);

    if (1) return;

    print_attr_id("upper_name", &elem->upper_name);
    print_attr_id("qualified_name", &elem->qualified_name);
    fprintf(stderr, "is_value: [%p]\n", elem->is_value);
}

static inline void
print_text(pcdom_node_t *node)
{
    pcdom_text_t *text = pcdom_interface_text(node);
    pcdom_character_data_t *char_data = &text->char_data;
    pcutils_str_t *str = &char_data->data;
    fprintf(stderr, "text: [%s]\n", str->data);
}

static inline void
print_node(pcdom_node_t *node)
{
    print_node_type(node->type);
    switch (node->type)
    {
        case PCDOM_NODE_TYPE_UNDEF:
            ASSERT_TRUE(0);
            break;
        case PCDOM_NODE_TYPE_ELEMENT:
            print_element(node);
            break;
        case PCDOM_NODE_TYPE_ATTRIBUTE:
            break;
        case PCDOM_NODE_TYPE_TEXT:
            print_text(node);
            break;
        case PCDOM_NODE_TYPE_CDATA_SECTION:
            break;
        case PCDOM_NODE_TYPE_ENTITY_REFERENCE:
            break;
        case PCDOM_NODE_TYPE_ENTITY:
            break;
        case PCDOM_NODE_TYPE_PROCESSING_INSTRUCTION:
            break;
        case PCDOM_NODE_TYPE_COMMENT:
            break;
        case PCDOM_NODE_TYPE_DOCUMENT:
            break;
        case PCDOM_NODE_TYPE_DOCUMENT_TYPE:
            break;
        case PCDOM_NODE_TYPE_DOCUMENT_FRAGMENT:
            break;
        case PCDOM_NODE_TYPE_NOTATION:
            break;
        case PCDOM_NODE_TYPE_LAST_ENTRY:
            break;
        default:
            ASSERT_TRUE(0);
            break;
    }

    if (1) return;

    fprintf(stderr, "local_name:[%ld]\n", node->local_name);
    fprintf(stderr, "prefix:[%ld]\n", node->prefix);
    fprintf(stderr, "ns:[%ld]\n", node->ns);
}

static inline void
traverse(pcdom_node_t *node)
{
    if (!node)
        return;

    print_node(node);

    pcdom_node_t *child = node->first_child;
    for (; child; child = child->next) {
        traverse(child);
    }
}

static inline void
process_html_element(purc_rwstream_t in, bool *ok)
{
    *ok = false;
    pchtml_html_document_t *doc = pchtml_html_document_create();
    ASSERT_NE(doc, nullptr);

    unsigned int ur = pchtml_html_document_parse(doc, in);
    ASSERT_EQ(ur, PCHTML_STATUS_OK);

    struct pcdom_document *document;
    document = pchtml_doc_get_document(doc);
    ASSERT_NE(document, nullptr);

    pcdom_node_t *node = &document->node;
    traverse(node);

    pchtml_html_document_destroy(doc);

    *ok = true;
}

TEST(html, element)
{
    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_HTML, "cn.fmsoft.hybridos.test",
            "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    const char *htmls[] = {
        "<html/>",
        "<html><hello id='yes' class='no' name='world' age='34'>a\nb<world/>c\nd</hello></html>",
        "<!DOCTYPE html><html><head/><body><div id='hello'/></body></html>",
        "<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\"><html/>",
    };

    for (size_t i=0; i<PCA_TABLESIZE(htmls); ++i) {
        const char *html = htmls[i];
        purc_rwstream_t rs = purc_rwstream_new_from_mem((void*)html, strlen(html));
        ASSERT_NE(rs, nullptr);
        bool ok;
        process_html_element(rs, &ok);
        ASSERT_TRUE(ok);
        purc_rwstream_destroy(rs);
    }

    purc_cleanup ();
}

static inline void
do_collection(pcdom_collection_t *collection, pcdom_element_t *element)
{
    unsigned int ur;
    ur = pcdom_elements_by_tag_name(element, collection,
            (const unsigned char*)"*", 1);
    ASSERT_EQ(ur, 0);

    size_t n = pcdom_collection_length(collection);
    for (size_t i=0; i<n; ++i) {
        pcdom_node_t *node = pcdom_collection_node(collection, i);
        ASSERT_NE(node, nullptr);
        fprintf(stderr, "=========%zd=========\n", i);
        print_node(node);
    }
}

static inline void
process_html_collection(purc_rwstream_t in, bool *ok)
{
    *ok = false;
    pchtml_html_document_t *doc = pchtml_html_document_create();
    ASSERT_NE(doc, nullptr);

    unsigned int ur = pchtml_html_document_parse(doc, in);
    ASSERT_EQ(ur, PCHTML_STATUS_OK);

    struct pcdom_document *document;
    document = pchtml_doc_get_document(doc);
    ASSERT_NE(document, nullptr);

    pcdom_element_t *element;
    element = document->element;
    const unsigned char *s;
    size_t len;
    s = pcdom_element_tag_name(element, &len);
    ASSERT_STREQ((const char*)s, "HTML");

    do {
        pcdom_collection_t *collection;
        collection = pcdom_collection_create(document);
        ASSERT_NE(collection, nullptr);

        ur = pcdom_collection_init(collection, 1024*1024);
        ASSERT_EQ(ur, 0);

        do_collection(collection, element);

        pcdom_collection_destroy(collection, true);
    } while (0);

    pchtml_html_document_destroy(doc);

    *ok = true;
}

TEST(html, collection)
{
    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_HTML, "cn.fmsoft.hybridos.test",
            "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    const char *htmls[] = {
        "<html><head x='y'/><body><div id='3'/></body></html>",
    };

    for (size_t i=0; i<PCA_TABLESIZE(htmls); ++i) {
        const char *html = htmls[i];
        purc_rwstream_t rs = purc_rwstream_new_from_mem((void*)html, strlen(html));
        ASSERT_NE(rs, nullptr);
        bool ok;
        process_html_collection(rs, &ok);
        purc_rwstream_destroy(rs);
    }

    purc_cleanup ();
}

