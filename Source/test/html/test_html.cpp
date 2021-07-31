#include "purc.h"
#include "private/html.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>
#include <libgen.h>

unsigned int
serializer_callback(const unsigned char  *data, size_t len, void *ctx)
{
    UNUSED_PARAM(ctx);
    printf("%.*s", (int) len, (const char *) data);

    return PCHTML_STATUS_OK;
}

// to test:
TEST(html, html_parser_chunk_raw)
{
    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    unsigned int  status = 0;
    pchtml_html_document_t *document;

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
        "good for me",
        "</div>",
        "\0"
    };


    /* Initialization */
    document = pchtml_html_document_create();
    if (document == NULL) {
        printf("Failed to create HTML Document\n");
    }

    /* Parse HTML */
    status = pchtml_html_document_parse_chunk_begin(document);
    if (status != PCHTML_STATUS_OK) {
        printf("Failed to parse HTML\n");
    }

    printf("Incoming HTML chunks:\n");

    for (size_t i = 0; html[i][0] != '\0'; i++) {
        printf("%s\n", (const char *) html[i]);

        const char *data = html[i];
        size_t      len  = strlen(data);

        status = pchtml_html_document_parse_chunk(document,
                    (const unsigned char*)data, len);

        if (status != PCHTML_STATUS_OK) {
            printf("Failed to parse HTML chunk\n");
        }
    }

    status = pchtml_html_document_parse_chunk_end(document);
    if (status != PCHTML_STATUS_OK) {
        printf("Failed to parse HTML\n");
    }

    /* Print Result */
    printf("\nHTML Tree:\n");
    pchtml_html_serialize_pretty_tree_cb((pcedom_node_t *)document,
                                          0x00, 0, serializer_callback, NULL);

    /* Destroy document */
    pchtml_html_document_destroy(document);

    purc_cleanup ();
}

TEST(html, html_parser_chunk_purc)
{
    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
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
    io = purc_rwstream_new_buffer(1024, 1024*8);
    ASSERT_NE(io, nullptr);
    for (size_t i = 0; html[i][0] != '\0'; i++) {
        const char *buf = html[i];
        size_t      len = strlen(buf);
        printf("%s\n", buf);
        ssize_t sz;
        sz = purc_rwstream_write(io, buf, len);
        ASSERT_GT(sz, 0);
        ASSERT_EQ((size_t)sz, len);
    }

    off_t off;
    off = purc_rwstream_seek(io, 0, SEEK_SET);
    ASSERT_NE(off, -1);

    purc_html_document_t doc = purc_html_doc_load_from_stream(io);
    ASSERT_NE(doc, nullptr);

    purc_rwstream_destroy(io);

    io = purc_rwstream_new_from_fp(stdout);

    int n;
    n = purc_html_doc_write_to_stream(doc, io);
    ASSERT_EQ(n, 0);
    purc_rwstream_destroy(io);

    purc_html_doc_destroy(doc);

    purc_cleanup ();
}

TEST(html, load_from_html)
{
    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    char this_file[] = __FILE__;
    char urls_txt[PATH_MAX];
    char cmd[1024*8];
    int n;
    FILE *furls;
    purc_rwstream_t out;

    out = purc_rwstream_new_from_fp(stdout);
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
        purc_html_document_t doc;
        int n, r;

        n = snprintf(cmd, sizeof(cmd), "curl %s", line);
        ASSERT_LT(n, sizeof(cmd));

        fin = popen(cmd, "r");
        ASSERT_NE(fin, nullptr);

        in = purc_rwstream_new_from_fp(fin);
        ASSERT_NE(in, nullptr);
        doc = purc_html_doc_load_from_stream(in);
        ASSERT_NE(doc, nullptr);

        r = purc_html_doc_write_to_stream(doc, out);
        ASSERT_EQ(r, 0);

        r = purc_html_doc_destroy(doc);
        ASSERT_EQ(r, 0);
        purc_rwstream_destroy(in);
        // pclose(fin);
    }

    purc_rwstream_destroy(out);

    purc_cleanup ();
}

