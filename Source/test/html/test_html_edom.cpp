#include "purc.h"
#include "private/list.h"
#include "private/html.h"
#include "private/dom.h"
#include "purc-html.h"
#include "./html/interfaces/document.h"

#include <gtest/gtest.h>

#include <stdarg.h>

static inline void write_edom_node(char *buf, size_t sz, pcdom_node_t *node)
{
    purc_rwstream_t ws = purc_rwstream_new_from_mem(buf, sz);
    ASSERT_NE(ws, nullptr);

    enum pchtml_html_serialize_opt opt;
    opt = (enum pchtml_html_serialize_opt)(
          PCHTML_HTML_SERIALIZE_OPT_UNDEF |
          PCHTML_HTML_SERIALIZE_OPT_SKIP_WS_NODES |
          PCHTML_HTML_SERIALIZE_OPT_WITHOUT_TEXT_INDENT |
          PCHTML_HTML_SERIALIZE_OPT_FULL_DOCTYPE);

    int n;
    n = pcdom_node_write_to_stream_ex(node, opt, ws);
    ASSERT_EQ(n, 0);
    purc_rwstream_write(ws, "", 1);

    purc_rwstream_destroy(ws);
}

static inline pcdom_element_t*
element_insert_child(pcdom_element_t* parent, const char *tag)
{
    pcdom_node_t *node = pcdom_interface_node(parent);
    pcdom_document_t *dom_doc = node->owner_document;
    pcdom_element_t *elem;
    elem = pcdom_document_create_element(dom_doc,
            (const unsigned char*)tag, strlen(tag), NULL);
    if (!elem)
        return NULL;

    pcdom_node_insert_child(node, pcdom_interface_node(elem));

    return elem;
}

TEST(html, edom_basic)
{
    purc_instance_extra_info info = {};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    pchtml_html_document_t *doc;
    doc = pchtml_html_document_create();
    ASSERT_NE(doc, nullptr);

    char buf[8192];
    write_edom_node(buf, sizeof(buf), pcdom_interface_node(doc));

    ASSERT_STREQ(buf, "");

    pchtml_html_document_destroy(doc);

    purc_cleanup ();
}


__attribute__ ((format (printf, 2, 3)))
static int
document_printf(pchtml_html_document_t *doc, const char *fmt, ...)
{
    char buf[8192];
    buf[0] = '\0';
    size_t nr = 0;

    va_list ap, dp;
    va_start(ap, fmt);
    va_copy(dp, ap);

    char *p = NULL;
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    do {
        if (n<0)
            break;

        p = buf;
        if ((size_t)n < sizeof(buf)) {
            nr = n;
            break;
        }

        size_t sz = n+1;
        p = (char*)malloc(sz);
        if (!p)
            break;

        *p = '\0';
        n = vsnprintf(p, sz, fmt, dp);
        va_end(dp);

        if (n<0 || (size_t)n >= sizeof(buf)) {
            free(p);
            p = NULL;
            break;
        }
        nr = n;
    } while (0);

    if (!p)
        return -1;

    unsigned int r;
    r = pchtml_html_document_parse_chunk_begin(doc);
    if (r==0) {
        r = pchtml_html_document_parse_chunk(doc,
                (const unsigned char*)p, nr);
        unsigned int rr = pchtml_html_document_parse_chunk_end(doc);
        if (rr)
            abort();
    }

    if (p!=buf)
        free(p);

    return r ? -1 : 0;
}

TEST(html, edom_parse_simple)
{
    purc_instance_extra_info info = {};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    pchtml_html_document_t *doc;
    doc = pchtml_html_document_create();
    ASSERT_NE(doc, nullptr);

    const char *html = "<html></html>";
    purc_rwstream_t rs;
    rs = purc_rwstream_new_from_mem((void*)html, strlen(html));

    unsigned int r;
    r = pchtml_html_document_parse(doc, rs);
    ASSERT_EQ(r, 0);

    purc_rwstream_destroy(rs);

    enum pchtml_html_serialize_opt opt;
    opt = (enum pchtml_html_serialize_opt)(
          PCHTML_HTML_SERIALIZE_OPT_UNDEF |
          PCHTML_HTML_SERIALIZE_OPT_SKIP_WS_NODES |
          PCHTML_HTML_SERIALIZE_OPT_WITHOUT_TEXT_INDENT |
          PCHTML_HTML_SERIALIZE_OPT_FULL_DOCTYPE);

    char buf[8192];
    size_t nr = sizeof(buf);
    char *p;
    p = pchtml_doc_snprintf(doc, opt, "", buf, &nr);
    ASSERT_NE(p, nullptr);
    ASSERT_STREQ(p, "<html><head></head><body></body></html>");
    if (p != buf)
        free(p);

    ASSERT_NE(doc->head, nullptr);
    write_edom_node(buf, sizeof(buf), pcdom_interface_node(doc->head));
    ASSERT_STREQ(buf, "<head></head>");

    ASSERT_NE(doc->body, nullptr);
    write_edom_node(buf, sizeof(buf), pcdom_interface_node(doc->body));
    ASSERT_STREQ(buf, "<body></body>");

    pchtml_html_document_destroy(doc);

    purc_cleanup ();
}

TEST(html, edom_parse)
{
    purc_instance_extra_info info = {};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    pchtml_html_document_t *doc;
    doc = pchtml_html_document_create();
    ASSERT_NE(doc, nullptr);

    const char *html = "<html>hello</html>";
    purc_rwstream_t rs;
    rs = purc_rwstream_new_from_mem((void*)html, strlen(html));

    unsigned int r;
    r = pchtml_html_document_parse(doc, rs);
    ASSERT_EQ(r, 0);

    purc_rwstream_destroy(rs);

    char buf[8192];
    write_edom_node(buf, sizeof(buf), pcdom_interface_node(doc));

    pchtml_html_document_destroy(doc);

    ASSERT_STREQ(buf, "<html><head></head><body>hello</body></html>");

    purc_cleanup ();
}

TEST(html, edom_parse_id)
{
    purc_instance_extra_info info = {};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    pchtml_html_document_t *doc;
    doc = pchtml_html_document_create();
    ASSERT_NE(doc, nullptr);

    const char *html = "<html><head></head><body><div id=\"hello\"></div></body></html>";
    purc_rwstream_t rs;
    rs = purc_rwstream_new_from_mem((void*)html, strlen(html));

    unsigned int r;
    r = pchtml_html_document_parse(doc, rs);
    ASSERT_EQ(r, 0);

    purc_rwstream_destroy(rs);

    char buf[8192];
    write_edom_node(buf, sizeof(buf), pcdom_interface_node(doc));

    ASSERT_STREQ(buf, "<html><head></head><body><div id=\"hello\"></div></body></html>");

    pchtml_html_document_destroy(doc);

    purc_cleanup ();
}

TEST(html, edom_parse_and_add)
{
    purc_instance_extra_info info = {};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    pchtml_html_document_t *doc;
    doc = pchtml_html_document_create();
    ASSERT_NE(doc, nullptr);

    const char *html = "<html></html>";
    purc_rwstream_t rs;
    rs = purc_rwstream_new_from_mem((void*)html, strlen(html));

    unsigned int r;
    r = pchtml_html_document_parse(doc, rs);
    ASSERT_EQ(r, 0);

    purc_rwstream_destroy(rs);

    char buf[8192];
    write_edom_node(buf, sizeof(buf), pcdom_interface_node(doc));

    ASSERT_STREQ(buf, "<html><head></head><body></body></html>");

    pcdom_node_t *head = pcdom_interface_node(doc->head);
    write_edom_node(buf, sizeof(buf), pcdom_interface_node(head));
    ASSERT_STREQ(buf, "<head></head>");

    ASSERT_NE(doc->body, nullptr);
    pcdom_node_t *body = pcdom_interface_node(doc->body);
    write_edom_node(buf, sizeof(buf), pcdom_interface_node(body));
    ASSERT_STREQ(buf, "<body></body>");

    pcdom_element_t *div;
    div = element_insert_child(pcdom_interface_element(body), "div");
    write_edom_node(buf, sizeof(buf), pcdom_interface_node(div));
    ASSERT_STREQ(buf, "<div></div>");
    write_edom_node(buf, sizeof(buf), pcdom_interface_node(body));
    ASSERT_STREQ(buf, "<body><div></div></body>");

    if (0) {
        const pcdom_attr_data_t *data;
        pcutils_hash_t *attrs = div->node.owner_document->attrs;
        data = pcdom_attr_data_by_local_name(attrs, (const unsigned char*)"id", 2);
        ASSERT_NE(data, nullptr);
        fprintf(stderr, "data->attr_id: %lx/%x\n", data->attr_id, PCDOM_ATTR_ID);
    }

    pcdom_attr_t *key;
    key = pcdom_element_set_attribute(div,
                (const unsigned char*)"class", 5,
                (const unsigned char*)"world", 5);
    ASSERT_NE(key, nullptr);
    write_edom_node(buf, sizeof(buf), pcdom_interface_node(div));
    ASSERT_STREQ(buf, "<div class=\"world\"></div>");
    write_edom_node(buf, sizeof(buf), pcdom_interface_node(body));
    ASSERT_STREQ(buf, "<body><div class=\"world\"></div></body>");

    if (1) {
        pcdom_element_t *body = pcdom_interface_element(doc->body);
        pcdom_document_t *document = pcdom_interface_document(doc);
        pcdom_collection_t *collection;
        collection = pcdom_collection_create(document);
        ASSERT_NE(collection, nullptr);
        unsigned int ui;
        ui = pcdom_collection_init(collection, 10);
        ASSERT_EQ(ui, 0);
        ui = pcdom_elements_by_class_name(body, collection,
                (const unsigned char*)"world", 5);
        ASSERT_EQ(ui, 0);
        size_t nr = pcdom_collection_length(collection);
        ASSERT_EQ(nr, 1);
        pcdom_collection_destroy(collection, true);
    }

    key = pcdom_element_set_attribute(div,
                (const unsigned char*)"id", 2,
                (const unsigned char*)"hello", 5);
    ASSERT_NE(key, nullptr);
    write_edom_node(buf, sizeof(buf), pcdom_interface_node(div));
    ASSERT_STREQ(buf, "<div class=\"world\" id=\"hello\"></div>");
    write_edom_node(buf, sizeof(buf), pcdom_interface_node(body));
    ASSERT_STREQ(buf, "<body><div class=\"world\" id=\"hello\"></div></body>");

    if (1) {
        pcdom_element_t *body = pcdom_interface_element(doc->body);
        pcdom_document_t *document = pcdom_interface_document(doc);
        pcdom_collection_t *collection;
        collection = pcdom_collection_create(document);
        ASSERT_NE(collection, nullptr);
        unsigned int ui;
        ui = pcdom_collection_init(collection, 10);
        ASSERT_EQ(ui, 0);
        ui = pcdom_elements_by_attr(body, collection,
                (const unsigned char*)"id", 2,
                (const unsigned char*)"hello", 5,
                false);
        ASSERT_EQ(ui, 0);
        size_t nr = pcdom_collection_length(collection);
        ASSERT_EQ(nr, 1);
        pcdom_collection_destroy(collection, true);
    }

    pchtml_html_document_destroy(doc);

    purc_cleanup ();
}

TEST(html, edom_element)
{
    purc_instance_extra_info info = {};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    pchtml_html_document_t *doc;
    doc = pchtml_html_document_create();
    ASSERT_NE(doc, nullptr);

    int r;
    r = document_printf(doc, "%s", "<html><head/><body>hello</body></html>");
    ASSERT_EQ(r, 0);

    char buf[8192];
    write_edom_node(buf, sizeof(buf), pcdom_interface_node(doc));

    ASSERT_STREQ(buf, "<html><head></head><body>hello</body></html>");

    pchtml_html_document_destroy(doc);

    purc_cleanup ();
}

TEST(html, edom_gen)
{
    purc_instance_extra_info info = {};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    pchtml_html_document_t *doc;
    doc = pchtml_html_document_create();
    ASSERT_NE(doc, nullptr);

    pcdom_document_t *dom_doc;
    dom_doc = pcdom_interface_document(doc);
    ASSERT_NE(dom_doc, nullptr);
    ASSERT_EQ(dom_doc->parser, nullptr);

    unsigned int r;
    r = pchtml_html_document_parse_chunk_begin(doc);
    ASSERT_EQ(r, 0);

    r = pchtml_html_document_parse_chunk_end(doc);
    ASSERT_EQ(r, 0);

    ASSERT_NE(dom_doc->parser, nullptr);
    pchtml_html_parser_t* parser;
    parser = pchtml_doc_get_parser(doc);
    ASSERT_NE(parser, nullptr);

    pcdom_element_t *html;
    html = dom_doc->element;
    ASSERT_NE(html, nullptr);

    pcdom_attr_t *key;
    key = pcdom_element_set_attribute(html,
                (const unsigned char*)"hello", 5,
                (const unsigned char*)"world", 5);
    ASSERT_NE(key, nullptr);

    pcdom_element_t *head;
    head = pchtml_doc_get_head(doc);
    ASSERT_NE(head, nullptr);
    void *p;
    p = head->node.parent;
    if (p != &html->node) {
        ASSERT_TRUE(false);
    }
    key = pcdom_element_set_attribute(head,
                (const unsigned char*)"foo", 5,
                (const unsigned char*)"bar", 5);
    ASSERT_NE(key, nullptr);

    pcdom_element_t *body;
    body = pchtml_doc_get_body(doc);
    ASSERT_NE(body, nullptr);
    p = body->node.parent;
    if (p != &html->node) {
        ASSERT_TRUE(false);
    }
    key = pcdom_element_set_attribute(body,
                (const unsigned char*)"great", 5,
                (const unsigned char*)"wall", 5);
    ASSERT_NE(key, nullptr);

    if (1) {
        const char *content = "<div name='a'/>";
        purc_rwstream_t in;
        in = purc_rwstream_new_from_mem((void*)content, strlen(content));
        if (in) {
            pcdom_node_t *node;
            node = pchtml_html_document_parse_fragment(doc, body, in);
            ASSERT_NE(node, nullptr);
            purc_rwstream_destroy(in);
            pcdom_merge_fragment_append(&body->node, node);
        }
    }
    if (1) {
        const char *content = "<div name='b'/>";
        purc_rwstream_t in;
        in = purc_rwstream_new_from_mem((void*)content, strlen(content));
        if (in) {
            pcdom_node_t *node;
            node = pchtml_html_document_parse_fragment(doc, head, in);
            ASSERT_NE(node, nullptr);
            purc_rwstream_destroy(in);
            pcdom_merge_fragment_append(&head->node, node);
        }
    }
    if (1) {
        const char *content = "contentA";
        purc_rwstream_t in;
        in = purc_rwstream_new_from_mem((void*)content, strlen(content));
        if (in) {
            pcdom_node_t *node;
            node = pchtml_html_document_parse_fragment(doc, head, in);
            ASSERT_NE(node, nullptr);
            purc_rwstream_destroy(in);
            pcdom_merge_fragment_append(&head->node, node);
        }
    }
    if (1) {
        const char *content = "contentB";
        purc_rwstream_t in;
        in = purc_rwstream_new_from_mem((void*)content, strlen(content));
        if (in) {
            pcdom_node_t *node;
            node = pchtml_html_document_parse_fragment(doc, head, in);
            ASSERT_NE(node, nullptr);
            purc_rwstream_destroy(in);
            pcdom_merge_fragment_append(&head->node, node);
        }
    }

#if 0
    pcdom_element_t *html;
    html = pcdom_document_create_element(dom_doc,
            (const unsigned char*)"html", 4, NULL);
    ASSERT_NE(html, nullptr);

    pcdom_document_attach_element(dom_doc, html);
    pcdom_node_insert_child(pcdom_interface_node(dom_doc),
                pcdom_interface_node(html));

    pcdom_element_t *head;
    head = element_insert_child(html, "head");
    ASSERT_NE(head, nullptr);
    pcdom_element_t *body;
    body = element_insert_child(html, "body");
    ASSERT_NE(body, nullptr);
#endif

    pcdom_element_t *div;
    div = element_insert_child(body, "div");
    ASSERT_NE(div, nullptr);


    pcdom_element_t *foo;
    foo = element_insert_child(body, "foo");
    ASSERT_NE(foo, nullptr);

    key = pcdom_element_set_attribute(div,
                (const unsigned char*)"helloX", 6, // tolower internally
                (const unsigned char*)"worldX", 6);
    ASSERT_NE(key, nullptr);

    key = pcdom_element_set_attribute(foo,
                (const unsigned char*)"worldX", 6, // tolower internally
                (const unsigned char*)"helloX", 6);
    ASSERT_NE(key, nullptr);

    pcdom_text_t *text;
    text = pcdom_text_interface_create(dom_doc);
    ASSERT_NE(text, nullptr);
    r = pcdom_character_data_replace(&text->char_data,
            (const unsigned char*)"yes", 3, 0, 0);
    ASSERT_EQ(r, 0);
    pcdom_node_insert_child(pcdom_interface_node(foo),
                pcdom_interface_node(text));

    char buf[8192];
    write_edom_node(buf, sizeof(buf), pcdom_interface_node(doc));

    ASSERT_STREQ(buf, "<html hello=\"world\"><head foo=\"bar\"><div name=\"b\"></div>contentAcontentB</head>"
                      "<body great=\"wall\"><div name=\"a\"></div><div hellox=\"worldX\"></div><foo worldx=\"helloX\">yes</foo></body></html>");

    pchtml_html_document_destroy(doc);

    purc_cleanup ();
}

TEST(html, edom_gen_attr)
{
    purc_instance_extra_info info = {};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    pchtml_html_document_t *doc;
    doc = pchtml_html_document_create();
    ASSERT_NE(doc, nullptr);

    pcdom_document_t *dom_doc;
    dom_doc = pcdom_interface_document(doc);
    ASSERT_NE(dom_doc, nullptr);
    ASSERT_EQ(dom_doc->parser, nullptr);

    unsigned int r;
    r = pchtml_html_document_parse_chunk_begin(doc);
    ASSERT_EQ(r, 0);

    r = pchtml_html_document_parse_chunk_end(doc);
    ASSERT_EQ(r, 0);

    ASSERT_NE(dom_doc->parser, nullptr);
    pchtml_html_parser_t* parser;
    parser = pchtml_doc_get_parser(doc);
    ASSERT_NE(parser, nullptr);

    char buf[8192];
    write_edom_node(buf, sizeof(buf), pcdom_interface_node(doc));

    ASSERT_STREQ(buf, "<html><head></head><body></body></html>");

    pchtml_html_document_destroy(doc);

    purc_cleanup ();
}

TEST(html, edom_gen_chunk_body)
{
    char buf[8192];

    purc_instance_extra_info info = {};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    pchtml_html_document_t *doc;
    doc = pchtml_html_document_create();
    ASSERT_NE(doc, nullptr);

    const char *html = "<html/>";
    unsigned int r;
    r = pchtml_html_document_parse_with_buf(doc,
            (const unsigned char*)html, strlen(html));
    ASSERT_EQ(r, 0);

    for (size_t i=0; i<10; ++i) {
        pcdom_element_t *body;
        body = pcdom_interface_element(doc->body);

        if (0) {
            write_edom_node(buf, sizeof(buf), pcdom_interface_node(doc));
            ASSERT_STREQ(buf, "<html><head></head><body></body></html>");
        }

        const char *chunk = "<foo></foo><bar></bar>";

        pcdom_node_t *node;
        node = pchtml_html_document_parse_fragment_with_buf(doc, body,
                (const unsigned char*)chunk, strlen(chunk));
        ASSERT_NE(node, nullptr);

        while (pcdom_interface_node(body)->first_child) {
            pcdom_node_destroy_deep(pcdom_interface_node(body)->first_child);
        }
        write_edom_node(buf, sizeof(buf), pcdom_interface_node(body));
        ASSERT_STREQ(buf, "<body></body>");
        write_edom_node(buf, sizeof(buf), node);
        ASSERT_STREQ(buf, "<html><foo></foo><bar></bar></html>");

        while (pcdom_interface_node(node)->first_child) {
            pcdom_node_t *p = pcdom_interface_node(node)->first_child;
            pcdom_node_remove(p);
            pcdom_node_insert_child(pcdom_interface_node(body), p);
        }
        write_edom_node(buf, sizeof(buf), pcdom_interface_node(body));
        ASSERT_STREQ(buf, "<body><foo></foo><bar></bar></body>");
        write_edom_node(buf, sizeof(buf), node);
        ASSERT_STREQ(buf, "<html></html>");

        ASSERT_EQ(node->owner_document, pcdom_interface_node(body)->owner_document);
        pcdom_node_destroy_deep(node);
    }

    write_edom_node(buf, sizeof(buf), pcdom_interface_node(doc));
    ASSERT_STREQ(buf, "<html><head></head><body><foo></foo><bar></bar></body></html>");

    pchtml_html_document_destroy(doc);

    purc_cleanup ();
}

TEST(html, edom_gen_chunk_other)
{
    char buf[8192];

    purc_instance_extra_info info = {};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    pchtml_html_document_t *doc;
    doc = pchtml_html_document_create();
    ASSERT_NE(doc, nullptr);

    const char *html = "<html><head></head><body><div></div></body></html>";
    unsigned int r;
    r = pchtml_html_document_parse_with_buf(doc,
            (const unsigned char*)html, strlen(html));
    ASSERT_EQ(r, 0);

    write_edom_node(buf, sizeof(buf), pcdom_interface_node(doc));
    ASSERT_STREQ(buf, "<html><head></head><body><div></div></body></html>");

    pcdom_collection_t *set;
    set = pcdom_collection_create(pcdom_interface_document(doc));
    ASSERT_NE(set, nullptr);
    r = pcdom_collection_init(set, 10);
    ASSERT_EQ(r, 0);
    pcdom_element_t *body;
    body = pcdom_interface_element(doc->body);
    ASSERT_NE(body, nullptr);
    pcdom_element_t *div;
    ASSERT_NE(pcdom_interface_node(body)->first_child, nullptr);
    div = pcdom_interface_element(pcdom_interface_node(body)->first_child);
    // fprintf(stderr, "0x%lx\n", div->node.local_name);
    // ASSERT_TRUE(0);
    // r = pcdom_elements_by_tag_name(body, set,
    //         (const unsigned char*)"div", 3);
    // ASSERT_EQ(r, 1);
    // ASSERT_EQ(pcdom_collection_length(set), 1);
    pcdom_collection_destroy(set, true);

    for (size_t i=0; i<10; ++i) {
        // pcdom_element_t *body;
        // body = pcdom_interface_element(doc->body);

        if (0) {
            write_edom_node(buf, sizeof(buf), pcdom_interface_node(doc));
            ASSERT_STREQ(buf, "<html><head></head><body></body></html>");
        }

        const char *chunk = "<foo></foo><bar></bar>";

        pcdom_node_t *node;
        node = pchtml_html_document_parse_fragment_with_buf(doc, div,
                (const unsigned char*)chunk, strlen(chunk));
        ASSERT_NE(node, nullptr);

        while (pcdom_interface_node(div)->first_child) {
            pcdom_node_destroy_deep(pcdom_interface_node(div)->first_child);
        }
        write_edom_node(buf, sizeof(buf), pcdom_interface_node(div));
        ASSERT_STREQ(buf, "<div></div>");
        write_edom_node(buf, sizeof(buf), node);
        ASSERT_STREQ(buf, "<html><foo></foo><bar></bar></html>");

        while (pcdom_interface_node(node)->first_child) {
            pcdom_node_t *p = pcdom_interface_node(node)->first_child;
            pcdom_node_remove(p);
            pcdom_node_insert_child(pcdom_interface_node(div), p);
        }
        write_edom_node(buf, sizeof(buf), pcdom_interface_node(div));
        ASSERT_STREQ(buf, "<div><foo></foo><bar></bar></div>");
        write_edom_node(buf, sizeof(buf), node);
        ASSERT_STREQ(buf, "<html></html>");

        ASSERT_EQ(node->owner_document, pcdom_interface_node(body)->owner_document);
        pcdom_node_destroy_deep(node);
    }

    write_edom_node(buf, sizeof(buf), pcdom_interface_node(doc));
    ASSERT_STREQ(buf, "<html><head></head><body><div><foo></foo><bar></bar></div></body></html>");

    pchtml_html_document_destroy(doc);

    purc_cleanup ();
}

