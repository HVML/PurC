#include "purc.h"
#include "private/html.h"

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

    io = purc_rwstream_new_from_unix_fd(dup(fileno(stdout)), -1);

    int n;
    n = pchtml_doc_write_to_stream(doc, io);
    ASSERT_EQ(n, 0);

    size_t size = 0;
    const char * buffer = purc_rwstream_get_mem_buffer (io, &size);
    char buf[1000] = {0,};
    memcpy(buf, buffer, size);
    printf("%s\n", buf);

    pchtml_html_document_destroy(doc);
    purc_rwstream_destroy(io);

    purc_cleanup ();
}

TEST(html, html_parser_chunk)
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

    pchtml_html_document_t *doc;
    doc = pchtml_html_document_create();
    ASSERT_NE(doc, nullptr);
    unsigned int ur;
    ur = pchtml_html_document_parse_chunk_begin(doc);
    ASSERT_EQ(ur, PCHTML_STATUS_OK);

    for (size_t i = 0; html[i][0] != '\0'; i++) {
        const char *buf = html[i];
        size_t      len = strlen(buf);
        io = purc_rwstream_new_from_mem((void*)buf, len);
        ASSERT_NE(io, nullptr);
        ur = pchtml_html_document_parse_chunk(doc, io);
        ASSERT_EQ(ur, PCHTML_STATUS_OK);
        purc_rwstream_destroy(io);
    }

    ur = pchtml_html_document_parse_chunk_end(doc);
    ASSERT_EQ(ur, PCHTML_STATUS_OK);

    io = purc_rwstream_new_from_unix_fd(dup(fileno(stdout)), -1);

    int n;
    n = pchtml_doc_write_to_stream(doc, io);
    ASSERT_EQ(n, 0);
    purc_rwstream_destroy(io);

    pchtml_html_document_destroy(doc);

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

    out = purc_rwstream_new_from_unix_fd(dup(fileno(stdout)), -1);
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

