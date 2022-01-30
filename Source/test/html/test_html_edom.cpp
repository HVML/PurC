#include "purc.h"
#include "private/list.h"
#include "private/html.h"
#include "private/dom.h"
#include "purc-html.h"

#include <gtest/gtest.h>

#include <stdarg.h>

TEST(html, edom_basic)
{
    purc_instance_extra_info info = {};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    pchtml_html_document_t *doc;
    doc = pchtml_html_document_create();
    ASSERT_NE(doc, nullptr);

    char buf[8192];
    purc_rwstream_t ws = purc_rwstream_new_from_mem(buf, sizeof(buf));
    ASSERT_NE(ws, nullptr);

    int n;
    n = pchtml_doc_write_to_stream(doc, ws);
    ASSERT_EQ(n, 0);
    purc_rwstream_write(ws, "", 1);

    purc_rwstream_destroy(ws);

    // ASSERT_STREQ(buf, "");

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
    purc_rwstream_t ws = purc_rwstream_new_from_mem(buf, sizeof(buf));
    ASSERT_NE(ws, nullptr);

    enum pchtml_html_serialize_opt opt;
    opt = (enum pchtml_html_serialize_opt)(
          PCHTML_HTML_SERIALIZE_OPT_UNDEF |
          PCHTML_HTML_SERIALIZE_OPT_SKIP_WS_NODES |
          PCHTML_HTML_SERIALIZE_OPT_WITHOUT_TEXT_INDENT |
          PCHTML_HTML_SERIALIZE_OPT_FULL_DOCTYPE);

    // opt = PCHTML_HTML_SERIALIZE_OPT_SKIP_WS_NODES;

    int n;
    n = pchtml_doc_write_to_stream_ex(doc, opt, ws);
    ASSERT_EQ(n, 0);
    purc_rwstream_write(ws, "", 1);

    purc_rwstream_destroy(ws);

    pchtml_html_document_destroy(doc);

    ASSERT_STREQ(buf, "<html><head></head><body>hello</body></html>");

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
    purc_rwstream_t ws = purc_rwstream_new_from_mem(buf, sizeof(buf));
    ASSERT_NE(ws, nullptr);

    int n;
    n = pchtml_doc_write_to_stream(doc, ws);
    ASSERT_EQ(n, 0);
    purc_rwstream_write(ws, "", 1);

    purc_rwstream_destroy(ws);

    // ASSERT_STREQ(buf, "");

    pchtml_html_document_destroy(doc);

    purc_cleanup ();
}

static inline pcdom_element_t*
element_insert_child(pcdom_element_t* parent, const char *tag)
{
    pcdom_document_t *dom_doc = pcdom_interface_node(parent)->owner_document;
    pcdom_element_t *elem;
    elem = pcdom_document_create_element(dom_doc,
            (const unsigned char*)tag, strlen(tag), NULL);
    if (!elem)
        return NULL;

    pcdom_node_insert_child(pcdom_interface_node(parent),
                pcdom_interface_node(elem));

    return elem;
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

    char buf[8192];
    purc_rwstream_t ws = purc_rwstream_new_from_mem(buf, sizeof(buf));
    ASSERT_NE(ws, nullptr);

    enum pchtml_html_serialize_opt opt;
    opt = (enum pchtml_html_serialize_opt)(
          PCHTML_HTML_SERIALIZE_OPT_UNDEF |
          PCHTML_HTML_SERIALIZE_OPT_SKIP_WS_NODES |
          PCHTML_HTML_SERIALIZE_OPT_WITHOUT_TEXT_INDENT |
          PCHTML_HTML_SERIALIZE_OPT_FULL_DOCTYPE);

    int n;
    n = pchtml_doc_write_to_stream_ex(doc, opt, ws);
    ASSERT_EQ(n, 0);
    purc_rwstream_write(ws, "", 1);

    purc_rwstream_destroy(ws);

    ASSERT_STREQ(buf, "<html hello=\"world\"><head foo=\"bar\"><div name=\"b\"></div></head>"
                      "<body great=\"wall\"><div name=\"a\"></div><div hellox=\"worldX\"></div><foo worldx=\"helloX\"></foo></body></html>");

    pchtml_html_document_destroy(doc);

    purc_cleanup ();
}

