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

void test_html_file(char * data_path, char * file_name)
{
    int ret = 0;
    size_t size = 0;
    purc_rwstream_t rwstream = NULL;
    const char * serialization = NULL;
    struct stat file_stat;
    size_t read_length = 0;
    FILE * fp = NULL;

    // it is the buffer for compare result
    char test_file[8192] = {0};
    char result_file[8192] = {0};

    // get the file name
    sprintf(test_file, "%s/%s.html", data_path, file_name);
    sprintf(result_file, "%s/%s.dat", data_path, file_name);

    printf("HTML FILE TEST: %s.html :", file_name);

    // whether the file exists
    if ((access(test_file, F_OK | R_OK)) != 0)
    {
        printf(" ERROR, %s.html does not exist.\n", file_name);
        return;
    }
    if ((access(result_file, F_OK | R_OK)) != 0)
    {
        printf(" ERROR, %s.dat does not exist.\n", file_name);
        return;
    }
    if ((stat(result_file, &file_stat) < 0) || (file_stat.st_size == 0))
    {
        printf(" ERROR, %s.dat is empty.\n", file_name);
        return;
    }
    
    // initialize the instance
    purc_instance_extra_info info = {0, 0};
    ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    // parse whole html file
    rwstream = purc_rwstream_new_from_file (test_file, "r"); 
    ASSERT_NE(rwstream, nullptr);

    off_t off;
    off = purc_rwstream_seek(rwstream, 0, SEEK_SET);
    ASSERT_NE(off, -1);

    pchtml_document_t doc = NULL;
    doc = pchtml_doc_load_from_stream (rwstream);
    ASSERT_NE(doc, nullptr);

    purc_rwstream_close (rwstream);
    purc_rwstream_destroy (rwstream);

    // create rwstream object with buffer for serilization
    rwstream = purc_rwstream_new_from_mem(test_file, 8192);

    // serialize documnet
    ret = pchtml_doc_write_to_stream(doc, rwstream);
    ASSERT_EQ(ret, 0);

    // get the buffer of serialization
    serialization = purc_rwstream_get_mem_buffer (rwstream, &size);

    // read result file
    read_length = 8192 > file_stat.st_size? file_stat.st_size: 8192;
    fp = fopen(result_file, "r");
    read_length = fread(result_file, 1, read_length, fp);
    fclose(fp);

printf("%s\n", serialization);
    // compare
    ret = strncmp(serialization, result_file, read_length);
    ASSERT_EQ(ret, 0);

    // clean rwstream and document
    purc_rwstream_destroy(rwstream);
    pchtml_doc_destroy(doc);

    // clean instance
    purc_cleanup ();

    printf(" OK\n");

}

void test_html_chunk(char * data_path, char * file_name)
{
    int ret = 0;
    size_t size = 0;
    purc_rwstream_t rwstream = NULL;
    const char * serialization = NULL;
    struct stat file_stat;
    size_t read_length = 0;

    // it is the buffer for compare result
    char test_file[8192] = {0};
    char result_file[8192] = {0};

    // get the file name
    sprintf(test_file, "%s/%s.html", data_path, file_name);
    sprintf(result_file, "%s/%s.dat", data_path, file_name);

    printf("HTML CHUNK TEST: %s.html :", file_name);

    // whether the file exists
    if ((access(test_file, F_OK | R_OK)) != 0)
    {
        printf(" ERROR, %s.html does not exist.\n", file_name);
        return;
    }
    if ((access(result_file, F_OK | R_OK)) != 0)
    {
        printf(" ERROR, %s.dat does not exist.\n", file_name);
        return;
    }
    if ((stat(result_file, &file_stat) < 0) || (file_stat.st_size == 0))
    {
        printf(" ERROR, %s.dat is empty.\n", file_name);
        return;
    }
    
    // initialize the instance
    purc_instance_extra_info info = {0, 0};
    ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    // parse the file, chunk by chunk
    pchtml_parser_t parser;
    parser = pchtml_parser_create();
    ASSERT_NE(parser, nullptr);

    FILE* fp = fopen(test_file, "r");       // open test_file
    if (fp) {
        char* line = NULL;
        int read = 0;
        
        // step 1: read one line from html file;
        // step 2: create rwstream object with read content;
        // step 3: feed HTML PARSER with rwstream object.
        while ((read = getline(&line, &size, fp)) != -1) {
            *(line + read - 1) = 0;

            rwstream = purc_rwstream_new_from_mem((void*)line, read - 1);
            ASSERT_NE(rwstream, nullptr);

            ret = pchtml_parser_parse_chunk(parser, rwstream);
            ASSERT_EQ(ret, 0);

            purc_rwstream_close (rwstream);
            purc_rwstream_destroy (rwstream);
        }
    }

    pchtml_document_t * doc = pchtml_parser_get_doc(parser);
    ASSERT_NE(doc, nullptr);

    ret = pchtml_parser_parse_end(parser);
    ASSERT_EQ(ret, 0);


    // create rwstream object with buffer for serilization
    rwstream = purc_rwstream_new_from_mem(test_file, 8192);

    // serialize documnet
    ret = pchtml_doc_write_to_stream(*doc, rwstream);
    ASSERT_EQ(ret, 0);

    // get the buffer of serialization
    serialization = purc_rwstream_get_mem_buffer (rwstream, &size);

    // read result file
    read_length = 8192 > file_stat.st_size? file_stat.st_size: 8192;
    fp = fopen(result_file, "r");
    read_length = fread(result_file, 1, read_length, fp);
    fclose(fp);

    // compare
    ret = strncmp(serialization, result_file, read_length);
    ASSERT_EQ(ret, 0);

    // clean rwstream and document
    purc_rwstream_destroy(rwstream);
    pchtml_doc_destroy(*doc);

    // clean instance
    purc_cleanup ();

    printf(" OK\n");
}

TEST(html, html_parser_html_file)
{
    // get test_list file directory
    char* data_path = getenv("HTML_TEST_PATH");

    if (data_path) {
        char file_path[1024] = {0};
        strcpy (file_path, data_path);
        strcat (file_path, "/test_list");   //full path of test_list

        FILE* fp = fopen(file_path, "r");   // open test_list
        if (fp) {
            char* line = NULL;
            size_t sz = 0;
            ssize_t read = 0;

            while ((read = getline(&line, &sz, fp)) != -1) {
                *(line + read - 1) = 0;
                test_html_file(data_path, line);     // get test file name
//                test_html_chunk(data_path, line);
            }
            fclose(fp);
        }
    }
}

#if 0
// test html parser for whole html file
TEST(html, html_parser_html_file_1)
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

    pchtml_document_t doc = pchtml_doc_load_from_stream(io);
    ASSERT_NE(doc, nullptr);

    purc_rwstream_destroy(io);

    io = purc_rwstream_new_from_fp(stdout);

    int n;
    n = pchtml_doc_write_to_stream(doc, io);
    ASSERT_EQ(n, 0);

    size_t size = 0;
    const char * buffer = purc_rwstream_get_mem_buffer (io, &size);
    char buf[1000] = {0,};
    memcpy(buf, buffer, size);
    printf("%s\n", buf);


    pchtml_doc_destroy(doc);

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
        pchtml_document_t doc;
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

        in = purc_rwstream_new_from_fp(fin);
        ASSERT_NE(in, nullptr);
        doc = pchtml_doc_load_from_stream(in);
        ASSERT_NE(doc, nullptr);

        r = pchtml_doc_write_to_stream(doc, out);
        ASSERT_EQ(r, 0);

        r = pchtml_doc_destroy(doc);
        ASSERT_EQ(r, 0);
        purc_rwstream_destroy(in);
        // pclose(fin);
    }

    purc_rwstream_destroy(out);

    if (line) {
        free(line);
    }

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

    pchtml_parser_t parser;
    parser = pchtml_parser_create();
    ASSERT_NE(parser, nullptr);

    purc_rwstream_t io;

    ASSERT_NE(io, nullptr);
    for (size_t i = 0; html[i][0] != '\0'; i++) {
        const char *buf = html[i];
        size_t      len = strlen(buf);
        io = purc_rwstream_new_from_mem((void*)buf, len);
        ASSERT_NE(io, nullptr);
        int r = pchtml_parser_parse_chunk(parser, io);
        ASSERT_EQ(r, 0);
        purc_rwstream_destroy(io);
    }

    pchtml_document_t doc = NULL;
    int r = pchtml_parser_parse_end(parser);//, &doc);
    ASSERT_EQ(r, 0);
    ASSERT_NE(doc, nullptr);

    io = purc_rwstream_new_from_fp(stdout);

    int n;
    n = pchtml_doc_write_to_stream(doc, io);
    ASSERT_EQ(n, 0);
    purc_rwstream_destroy(io);

    pchtml_doc_destroy(doc);

    purc_cleanup ();
}
#endif
