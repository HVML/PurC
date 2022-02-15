#include "purc.h"
#include "private/list.h"
#include "private/html.h"
#include "private/dom.h"
#include "purc-html.h"
#include "./html/interfaces/document.h"
#include "private/interpreter.h"

#include <gtest/gtest.h>

#include <stdarg.h>

#define TO_DEBUG 1

#define ASSERT_DOC_DOC_EQ(_l, _r) do {                               \
    int _diff = 0;                                                   \
    ASSERT_EQ(pcintr_util_comp_docs(_l, _r, &_diff), 0);             \
    ASSERT_EQ(_diff, 0);                                             \
} while (0)

#define ASSERT_DOC_HTML_EQ(_doc, _html) do {         \
    pchtml_html_document_t *_tmp;                    \
    _tmp = pcintr_util_load_document(_html);         \
    ASSERT_NE(_tmp, nullptr);                        \
    ASSERT_DOC_DOC_EQ(_doc, _tmp);                   \
    pchtml_html_document_destroy(_tmp);              \
} while (0)

#define ASSERT_TAG_NAME_EQ(_elem, _tag_name) do {    \
    const unsigned char *_t;                         \
    size_t _len;                                     \
    _t = pcdom_element_local_name(_elem, &_len);     \
    ASSERT_NE(_t, nullptr);                          \
    ASSERT_EQ(_t[_len], '\0');                       \
    ASSERT_STREQ((const char*)_t, _tag_name);        \
} while (0);

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

static pchtml_html_document_t*
load_document(const char *html)
{
    pchtml_html_document_t *doc;

    doc = pchtml_html_document_create();
    if (doc) {
        int r = document_printf(doc, "%s", html);
        if (r == 0)
            return doc;

        pchtml_html_document_destroy(doc);
    }

    return NULL;
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
    p = pchtml_doc_snprintf_ex(doc, opt, buf, &nr, "");
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

TEST(html, edom_parse_bad_input_adjust)
{
    purc_instance_extra_info info = {};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    pchtml_html_document_t *doc;
    doc = pchtml_html_document_create();
    ASSERT_NE(doc, nullptr);

    const char *html = "<html><head><div>hello</div></head></html>";
    purc_rwstream_t rs;
    rs = purc_rwstream_new_from_mem((void*)html, strlen(html));

    unsigned int r;
    r = pchtml_html_document_parse(doc, rs);
    ASSERT_EQ(r, 0);

    purc_rwstream_destroy(rs);

    char buf[8192];
    write_edom_node(buf, sizeof(buf), pcdom_interface_node(doc));

    pchtml_html_document_destroy(doc);

    ASSERT_STREQ(buf, "<html><head></head><body><div>hello</div></body></html>");

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
    div = pcintr_util_insert_element(pcdom_interface_element(body), "div");
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

TEST(html, parser_validate)
{
    const char *inputs[100][100] = {
        {
            "<html/>",
            "<html></html>",
            "<html><head/><body></body></html>",
            NULL,
        },
        {   // move content from head to body
            "<html><head>hello</head><body></body></html>",
            "<html><head></head><body>hello</body></html>",
            "<html><head/><body>hello</body></html>",
            "<html><body>hello</body></html>",
            "<html>hello</html>",
            "<html>hello",
            "hello",
            NULL,
        },
        {   // move content and element from head to body
            "<html><head>hello<title>world</title></head><body></body></html>",
            "<html><head></head><body>hello<title>world</title></body></html>",
            NULL,
        },
        {   // move content outside of hvml to body
            "hello<html/>world",
            "<html><head></head><body>helloworld</body></html>",
            NULL,
        },
    };

    purc_instance_extra_info info = {};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);


    for (size_t i=0; i<sizeof(inputs)/sizeof(inputs[0]); ++i) {
        const char **htmls = inputs[i];
        if (!htmls)
            break;

        const char *html = htmls[0];
        if (!html)
            continue;

        pchtml_html_document_t *doc;
        doc = load_document(html);
        ASSERT_NE(doc, nullptr);

        char buf[8192];
        write_edom_node(buf, sizeof(buf), pcdom_interface_node(doc));

        for (size_t j=1; htmls[j]; ++j) {
            const char *p = htmls[j];
            pchtml_html_document_t *d;
            d = load_document(p);
            ASSERT_NE(d, nullptr);

            char b[8192];
            write_edom_node(b, sizeof(b), pcdom_interface_node(d));

            if (strcmp(buf, b)) {
                fprintf(stderr, "for inputs:\n%s\n%s\n", html, p);
                ASSERT_STREQ(buf, b);
            }

            pchtml_html_document_destroy(d);
        }

        pchtml_html_document_destroy(doc);
    }

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
    head = pcintr_util_insert_element(html, "head");
    ASSERT_NE(head, nullptr);
    pcdom_element_t *body;
    body = pcintr_util_insert_element(html, "body");
    ASSERT_NE(body, nullptr);
#endif

    pcdom_element_t *div;
    div = pcintr_util_insert_element(body, "div");
    ASSERT_NE(div, nullptr);


    pcdom_element_t *foo;
    foo = pcintr_util_insert_element(body, "foo");
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

static pcdom_node_t*
document_parse_fragment_with_buf(pchtml_html_document_t *doc,
        pcdom_element_t *element, const char *chunk, size_t len)
{
    pchtml_html_parser_t *parser;
    parser = pchtml_html_parser_create();
    if (!parser)
        return NULL;

    unsigned int r;
    r = pchtml_html_parser_init(parser);
    if (r) {
        pchtml_html_parser_destroy(parser);
        return NULL;
    }

    r = pchtml_html_parse_fragment_chunk_begin(parser, doc,
            element->node.local_name,
            element->node.ns);
    if (r) {
        pchtml_html_parser_destroy(parser);
        return NULL;
    }

    for (size_t i=0; i<len; ++i) {
        r = pchtml_html_parse_fragment_chunk_process_with_format(parser,
                "%c", chunk[i]);
        if (r)
            break;
    }

    pcdom_node_t *node;
    node = pchtml_html_parse_fragment_chunk_end(parser);
    pchtml_html_parser_destroy(parser);

    if (r) {
        pcdom_node_destroy_deep(node);
        return NULL;
    }

    return node;
}

TEST(html, edom_gen_chunk_parser)
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
        node = document_parse_fragment_with_buf(doc, div,
                (const char*)chunk, strlen(chunk));
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


struct path_node {
    struct list_head        node;

    char                   *path;
};

static struct path_node*
path_node_gen(const char *tag_name, size_t len)
{
    struct path_node *p = (struct path_node*)calloc(1, sizeof(*p));
    if (!p)
        return NULL;
    p->path = strndup(tag_name, len);
    if (!p->path) {
        free(p);
        return NULL;
    }

    return p;
}

static char*
element_full_path(pcdom_element_t *elem)
{
    struct list_head paths;
    INIT_LIST_HEAD(&paths);

    do {
        const unsigned char *tag_name;
        size_t len;
        tag_name = pcdom_element_local_name(elem, &len);
        struct path_node *node;
        node = path_node_gen((const char*)tag_name, len);
        if (!node)
            break;

        list_add_tail(&node->node, &paths);

        elem = pcdom_interface_element(elem->node.parent);
    } while (elem);

    char *path = NULL;

    if (elem == NULL) {
        size_t sz = 0;
        struct path_node *p;
        list_for_each_entry(p, &paths, node) {
            sz += strlen(p->path) + 1;
        }
        path = (char*)malloc(sz+1);
        if (path) {
            path[0] = '\0';
            list_for_each_entry_reverse(p, &paths, node) {
                if (*p->path == '#')
                    continue;
                strcat(path, "/");
                strcat(path, p->path);
            }
        }
    }

    struct path_node *p, *n;
    list_for_each_entry_safe(p, n, &paths, node) {
        list_del(&p->node);
        free(p->path);
        free(p);
    }

    return path;
}

struct key_val {
    const char *key;
    const char *val;
};

struct edom_raw_action {
    int                     valid;
    const char             *parent;
    const char             *tag_name;
    struct key_val          kvs[100];
    const char             *fragment;
    int                     append;   // 1: append; 0: displace
    const char             *output;
};

struct edom_raw_sample {
    const char             *html;
    struct edom_raw_action actions[100];
};

struct edom_raw_walker_data {
    const struct edom_raw_sample     *sample;
    pchtml_html_document_t           *doc;
};

static void
edom_raw_process_fragment(pcdom_element_t *elem,
        const struct edom_raw_action *action)
{
    const char *fragment = action->fragment;
    pchtml_html_document_t *doc;
    doc = pchtml_html_interface_document(
            pcdom_interface_node(elem)->owner_document);
    ASSERT_NE(doc, nullptr);
    D("fragment: [%s]", fragment);

    pcdom_element_t *p;
    p = pcintr_util_parse_fragment_ex(elem, fragment);
    ASSERT_NE(p, nullptr);

    if (action->append) {
        pcdom_merge_fragment_append(pcdom_interface_node(elem),
                pcdom_interface_node(p));
    }
    else {
        pcdom_displace_fragment(pcdom_interface_node(elem),
                pcdom_interface_node(p));
    }
}

static void
edom_raw_process_element(pcdom_element_t *elem,
        pchtml_html_document_t *doc,
        const struct edom_raw_action *action)
{
    pcdom_element_t *parent = elem;
    pcdom_element_t *child = NULL;
    if (action->tag_name) {
        child = pcintr_util_insert_element(parent, action->tag_name);
        ASSERT_NE(child, nullptr);
    }

    pcdom_element_t *curr = child ? child : parent;
    for (size_t i=0; i<sizeof(action->kvs)/sizeof(action->kvs[0]); ++i) {
        const struct key_val *kv = action->kvs + i;
        if (kv->key == NULL || kv->val == NULL)
            break;

        int r = pcintr_util_set_attribute(curr, kv->key, kv->val);
        ASSERT_EQ(r, 0);
    }

    if (action->fragment) {
        edom_raw_process_fragment(curr, action);
    }

    ASSERT_DOC_HTML_EQ(doc, action->output);
}

void
edom_raw_process_action(pcdom_node_t *node,
        pchtml_html_document_t *doc,
        const struct edom_raw_action *action)
{
    pcdom_node_type_t type = node->type;
    if (type == PCDOM_NODE_TYPE_ELEMENT) {
        char *path = element_full_path(pcdom_interface_element(node));
        if (strcmp(path, action->parent)==0) {
            edom_raw_process_element(pcdom_interface_element(node),
                    doc, action);
        }
        free(path);
    }
}

static pchtml_action_t
edom_raw_walker(pcdom_node_t *node, void *ctx)
{
    struct edom_raw_walker_data *ud;
    ud = (struct edom_raw_walker_data*)ctx;
    const struct edom_raw_sample *sample = ud->sample;
    pchtml_html_document_t *doc = ud->doc;
    for (size_t i=0; i<sizeof(sample->actions)/sizeof(sample->actions[0]); ++i) {
        const struct edom_raw_action *action = sample->actions + i;
        if (action->valid)
            edom_raw_process_action(node, doc, action);
    }
    return PCHTML_ACTION_OK;
}

static void
edom_raw_process(const struct edom_raw_sample *sample)
{
    ASSERT_NE(sample, nullptr);
    ASSERT_NE(sample->html, nullptr);

    pchtml_html_document_t *doc;
    doc = load_document(sample->html);
    ASSERT_NE(doc, nullptr);

    struct pcdom_element *root;
    root = pcdom_interface_document(doc)->element;
    ASSERT_NE(root, nullptr);
    ASSERT_TAG_NAME_EQ(root, "html");

    struct edom_raw_walker_data ud = {
        .sample         = sample,
        .doc            = doc,
    };

    pcdom_node_simple_walk(pcdom_interface_node(doc), edom_raw_walker, &ud);

    pchtml_html_document_destroy(doc);
}

TEST(html, edom_raw)
{
    struct edom_raw_sample samples[] = {
#if 0              /* { */
        {
            "<html/>",
            {
                {
                    1,
                    "/html/head",
                    "title",
                    {{"hello", "world"}, {"foo", "bar"}},
                    NULL,
                    1,
                    "<html><head><title hello=\"world\" foo=\"bar\"></title></head></html>",
                },
            },
        },
        {
            "<html/>",
            {
                {
                    1,
                    "/html/head",
                    NULL,
                    {{"hello", "world"}, {"foo", "bar"}},
                    NULL,
                    1,
                    "<html><head hello=\"world\" foo=\"bar\"></head></html>",
                },
            },
        },
        {
            "<html/>",
            {
                {
                    1,
                    "/html/body",
                    NULL,
                    {{"hello", "world"}, {"foo", "bar"}},
                    "world",
                    1,
                    "<html><body hello=\"world\" foo=\"bar\">world</body></html>",
                },
            },
        },
        {
            "<html/>",
            {
                {
                    1,
                    "/html/body",
                    NULL,
                    {{"hello", "world"}, {"foo", "bar"}},
                    "world<foo></foo>bar",
                    1,
                    "<html><body hello=\"world\" foo=\"bar\">world<foo></foo>bar</body></html>",
                },
            },
        },
        {
            "<html/>",
            {
                {
                    1,
                    "/html/head",
                    "title",
                    {{"hello", "world"}, {"foo", "bar"}},
                    "foobar",
                    1,
                    "<html><head><title hello=\"world\" foo=\"bar\">foobar</title></head></html>",
                },
            },
        },
        {
            "<html/>",
            {
                {
                    1,
                    "/html/head",
                    "title",
                    {},
                    "foobar",
                    1,
                    "<html><head><title>foobar</title></head></html>",
                },
            },
        },
        {
            "<html/>",
            {
                {
                    1,
                    "/html/head",
                    "title",
                    {{"hello", "world"}, {"foo", "bar"}},
                    NULL,
                    1,
                    "<html><head><title hello=\"world\" foo=\"bar\"></title></head></html>",
                },
            },
        },
        /////////////////
        //{ // FIXME: failed, because of HTML title element?
        //  //        generated as `expected` but non-formal HTML
        //    "<html/>",
        //    {
        //        {
        //            1,
        //            "/html/head",
        //            "title",
        //            {{"hello", "world"}, {"foo", "bar"}},
        //            "foo<a></a>bar",
        //            1,
        //            "<html><head><title hello=\"world\" foo=\"bar\">foo<a></a>bar</title></head></html>", // this is non-formal HTML
        //        },
        //    },
        //},
        //{ // FIXME: failed, because of HTML title element?
        //  //        generated as `expected` but non-formal HTML
        //    "<html/>",
        //    {
        //        {
        //            1,
        //            "/html/body",
        //            "title",
        //            {{"hello", "world"}, {"foo", "bar"}},
        //            "foo<a></a>bar",
        //            1,
        //            "<html><body><title hello=\"world\" foo=\"bar\">foo<a></a>bar</title></body></html>", // this is non-formal HTML
        //        },
        //    },
        //},
        /////////////////
        // "<hvml><head><title>hello</title></head><body><span id=\"clock\">xyz</span><xinput xid=\"xexp\"></xinput><update on=\"#clock\" at=\"textContent\" to=\"displace\" with=\"abc\"/></body></hvml>";
        {
            "<html/>",
            {
                {
                    1,
                    "/html/head",
                    "title",
                    {},
                    "hello",
                    1,
                    "<html><head><title>hello</title></head></html>",
                },
            },
        },
        {
            "<html><head><title>hello</title></html>",
            {
                {
                    1,
                    "/html/body",
                    "span",
                    {{"id", "clock"}},
                    "xyz",
                    1,
                    "<html><head><title>hello</title><body><span id=\"clock\">xyz</span></body></html>",
                },
            },
        },
        {
            "<html><head><title>hello</title><body><span id=\"clock\">xyz</span></body></html>",
            {
                {
                    1,
                    "/html/body",
                    "xinput",
                    {{"xid", "xexp"}},
                    NULL,
                    1,
                    "<html><head><title>hello</title><body><span id=\"clock\">xyz</span><xinput xid=\"xexp\"></xinput></body></html>",
                },
            },
        },
        {
            "<html><head><title>hello</title><body><span id=\"clock\">xyz</span><xinput xid=\"xexp\"></xinput></body></html>",
            {
                {
                    1,
                    "/html/body/span",
                    NULL,
                    {},
                    "abc",
                    1,
                    "<html><head><title>hello</title><body><span id=\"clock\">xyzabc</span><xinput xid=\"xexp\"></xinput></body></html>",
                },
            },
        },
#endif             /* } */
        // "<hvml><head><title>hello</title></head><body><span id=\"clock\">xyz</span><xinput xid=\"xexp\"></xinput><update on=\"#clock\" at=\"textContent\" to=\"displace\" with=\"abc\"/></body></hvml>";
        {
            "<html/>",
            {
                {
                    1,
                    "/html/head",
                    "title",
                    {},
                    "hello",
                    1,
                    "<html><head><title>hello</title></head></html>",
                },
                {
                    1,
                    "/html/body",
                    "span",
                    {{"id", "clock"}},
                    "xyz",
                    1,
                    "<html><head><title>hello</title><body><span id=\"clock\">xyz</span></body></html>",
                },
                {
                    1,
                    "/html/body",
                    "xinput",
                    {{"xid", "xexp"}},
                    NULL,
                    1,
                    "<html><head><title>hello</title><body><span id=\"clock\">xyz</span><xinput xid=\"xexp\"></xinput></body></html>",
                },
                {
                    1,
                    "/html/body/span",
                    NULL,
                    {},
                    "abc",
                    0,
                    "<html><head><title>hello</title><body><span id=\"clock\">abc</span><xinput xid=\"xexp\"></xinput></body></html>",
                },
            },
        },
    };
    purc_instance_extra_info info = {};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    for (size_t i=0; i<sizeof(samples)/sizeof(samples[0]); ++i) {
        struct edom_raw_sample *sample = samples + i;

        edom_raw_process(sample);
    }

#if 0              /* { */
    pchtml_html_document_t *doc;
    doc = load_document("<html/>");
    ASSERT_NE(doc, nullptr);

    ASSERT_DOC_HTML_EQ(doc, "<html><head></head><body></body></html>");

    struct pcdom_element *root;
    root = pcdom_interface_document(doc)->element;
    ASSERT_NE(root, nullptr);
    ASSERT_TAG_NAME_EQ(root, "html");

    struct pcdom_element *head;
    head = pchtml_doc_get_head(doc);
    ASSERT_NE(head, nullptr);
    ASSERT_TAG_NAME_EQ(head, "head");

    struct pcdom_element *body;
    body = pchtml_doc_get_body(doc);
    ASSERT_NE(body, nullptr);
    ASSERT_TAG_NAME_EQ(body, "body");

    pcdom_element_t *head_title;
if (0) {
    head_title = pcintr_util_insert_element(head, "title");
    ASSERT_NE(head_title, nullptr);
    ASSERT_TAG_NAME_EQ(head_title, "title");
    ASSERT_DOC_HTML_EQ(doc, "<html><head><title></title></head><body></body></html>");
}
if (0) {
    head_title = pcintr_util_insert_element(body, "title");
    ASSERT_NE(head_title, nullptr);
    ASSERT_TAG_NAME_EQ(head_title, "title");
    ASSERT_DOC_HTML_EQ(doc, "<html><head><title></title></head><body><title></title></body></html>");
}
if (1) {
    head_title = pcintr_util_insert_element(head, "title");
    ASSERT_NE(head_title, nullptr);
    ASSERT_TAG_NAME_EQ(head_title, "title");
    pcintr_dump_edom_node(NULL, pcdom_interface_node(doc));
    ASSERT_TRUE(0);
    ASSERT_DOC_HTML_EQ(doc, "<html><head><title></title><title></title></head><body><title></title></body></html>");
}
    head_title = pcintr_util_insert_element(root, "title");
    ASSERT_NE(head_title, nullptr);
    ASSERT_TAG_NAME_EQ(head_title, "title");
    pcintr_dump_edom_node(NULL, pcdom_interface_node(doc));
    ASSERT_DOC_HTML_EQ(doc, "<html><head></head><body></body><titlex></titlex></html>");

    pchtml_html_document_destroy(doc);
#endif             /* } */

    purc_cleanup ();
}

TEST(html, edom)
{
    purc_instance_extra_info info = {};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    pchtml_html_document_t *doc;
    doc = pcintr_util_load_document("<html/>");
    ASSERT_NE(doc, nullptr);
    ASSERT_DOC_HTML_EQ(doc, "<html><head></head><body></body></html>");

    struct pcdom_element *body;
    body = pchtml_doc_get_body(doc);
    ASSERT_NE(body, nullptr);

    ASSERT_EQ(doc, pchtml_html_interface_document(body->node.owner_document));

    pcdom_element_t *elem;
    elem = pcintr_util_add_element(body, "div");
    ASSERT_NE(elem, nullptr);

    ASSERT_DOC_HTML_EQ(doc, "<html><head></head><body><div></div></body></html>");

    ASSERT_EQ(0, pcintr_util_add_child(body, "hello"));
    ASSERT_DOC_HTML_EQ(doc, "<html><head></head><body><div></div>hello</body></html>");

    elem = pcintr_util_add_element(elem, "span");
    ASSERT_NE(elem, nullptr);
    ASSERT_DOC_HTML_EQ(doc, "<html><head></head><body><div><span></span></div>hello</body></html>");

    int r = pcintr_util_set_attribute(elem, "id", "clock");
    ASSERT_EQ(r, 0);
    ASSERT_DOC_HTML_EQ(doc, "<html><head></head><body><div><span id=\"clock\"></span></div>hello</body></html>");

    ASSERT_EQ(0, pcintr_util_set_child(elem, "<foo></foo>"));
    ASSERT_DOC_HTML_EQ(doc, "<html><head></head><body><div><span id=\"clock\"><foo></foo></span></div>hello</body></html>");

    ASSERT_EQ(0, pcintr_util_set_child(elem, "hello"));
    ASSERT_DOC_HTML_EQ(doc, "<html><head></head><body><div><span id=\"clock\">hello</span></div>hello</body></html>");

    ASSERT_EQ(0, pcintr_util_set_child(elem, "world"));
    ASSERT_DOC_HTML_EQ(doc, "<html><head></head><body><div><span id=\"clock\">world</span></div>hello</body></html>");

    pchtml_html_document_destroy(doc);

    purc_cleanup ();
}

TEST(html, buggy)
{
    // "<hvml><body><span id=\"clock\">xyz</span><xinput xid=\"xexp\"></xinput><update on=\"#clock\" at=\"textContent\" to=\"displace\" with=\"abc\"/></body></hvml>";
    purc_instance_extra_info info = {};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    pchtml_html_document_t *doc;
    doc = pcintr_util_load_document("<html/>");
    ASSERT_NE(doc, nullptr);
    ASSERT_DOC_HTML_EQ(doc, "<html><head></head><body></body></html>");

    struct pcdom_element *head;
    head = pchtml_doc_get_head(doc);
    ASSERT_NE(head, nullptr);
    ASSERT_EQ(doc, pchtml_html_interface_document(head->node.owner_document));

    struct pcdom_element *body;
    body = pchtml_doc_get_body(doc);
    ASSERT_NE(body, nullptr);
    ASSERT_EQ(doc, pchtml_html_interface_document(body->node.owner_document));

    pcdom_element_t *span;
    span = pcintr_util_add_element(body, "span");
    ASSERT_NE(span, nullptr);
    ASSERT_DOC_HTML_EQ(doc, "<html><head></head><body><span></span></body></html>");

    ASSERT_EQ(0, pcintr_util_set_attribute(span, "id", "clock"));
    ASSERT_DOC_HTML_EQ(doc, "<html><head></head><body><span id=\"clock\"></span></body></html>");

    ASSERT_EQ(0, pcintr_util_add_child(span, "xyz"));
    ASSERT_DOC_HTML_EQ(doc, "<html><head></head><body><span id=\"clock\">xyz</span></body></html>");

    pcdom_element_t *xinput;
    xinput = pcintr_util_add_element(body, "xinput");
    ASSERT_NE(xinput, nullptr);
    ASSERT_DOC_HTML_EQ(doc, "<html><head></head><body><span id=\"clock\">xyz</span><xinput></xinput></body></html>");

    ASSERT_EQ(0, pcintr_util_set_attribute(xinput, "xid", "xexp"));
    ASSERT_DOC_HTML_EQ(doc, "<html><head></head><body><span id=\"clock\">xyz</span><xinput xid=\"xexp\"></xinput></body></html>");

    pcdom_document_t *document = pcdom_interface_document(doc);
    pcdom_collection_t *collection;
    collection = pcdom_collection_create(document);
    ASSERT_NE(collection, nullptr);
    ASSERT_EQ(0, pcintr_util_set_child(span, "def"));
    ASSERT_DOC_HTML_EQ(doc, "<html><head></head><body><span id=\"clock\">def</span><xinput xid=\"xexp\"></xinput></body></html>");
    unsigned int ui;
    ui = pcdom_collection_init(collection, 10);
    ASSERT_EQ(ui, 0);
    ui = pcdom_elements_by_attr(body, collection,
            (const unsigned char*)"id", 2,
            (const unsigned char*)"clock", 5,
            false);
    ASSERT_EQ(ui, 0);
    size_t nr = pcdom_collection_length(collection);
    ASSERT_EQ(nr, 1);
    pcdom_collection_destroy(collection, true);

    pchtml_html_document_destroy(doc);

    purc_cleanup ();
}

