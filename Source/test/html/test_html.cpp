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
    pchtml_html_document_t *document = NULL;
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

    document = pchtml_html_document_create();
    ASSERT_NE(document, nullptr);

    ret = pchtml_html_document_parse(document, rwstream);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_rwstream_close (rwstream);
    purc_rwstream_destroy (rwstream);

    // create rwstream object with buffer for serilization
    rwstream = purc_rwstream_new_from_mem(test_file, 8192);

    // serialize documnet
    ret = pchtml_doc_write_to_stream(document, rwstream);
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
    pchtml_html_document_destroy (document);

    // clean instance
    purc_cleanup ();

    printf(" OK\n");

}

void test_html_chunk(char * data_path, char * file_name)
{
    int ret = 0;
    size_t size = 0;
    purc_rwstream_t rwstream = NULL;
    pchtml_html_document_t *document = NULL;
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
    document = pchtml_html_document_create();
    ASSERT_NE(document, nullptr);

    ret = pchtml_html_document_parse_chunk_begin(document);
    ASSERT_EQ (ret, PURC_ERROR_OK);

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

            ret = pchtml_html_document_parse_chunk(document, rwstream);
            ASSERT_EQ(ret, 0);

            purc_rwstream_close (rwstream);
            purc_rwstream_destroy (rwstream);
        }
    }

    ret = pchtml_html_document_parse_chunk_end(document);
    ASSERT_EQ(ret, 0);

    // create rwstream object with buffer for serilization
    rwstream = purc_rwstream_new_from_mem(test_file, 8192);

    // serialize documnet
    ret = pchtml_doc_write_to_stream(document, rwstream);
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
    pchtml_html_document_destroy(document);

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
//                test_html_file(data_path, line);     // get test file name
                test_html_chunk(data_path, line);
            }
            fclose(fp);
        }
    }
}


#include "html/interfaces/document.h"

TEST(html, html_parser_fragment)
{
    char buffer[8192] = {0};
    const char * serialization = NULL;
    size_t size = 0;
    char before[] = "<html>\n  <head>\n  </head>\n  <body>\n    <div>\n      <span>\n        \"blah-blah-blah\"\n      </span>\n    </div>\n  </body>\n</html>\n\n";
    char after[] =  "<html>\n  <head>\n  </head>\n  <body>\n    <ul>\n      <li>\n        \"1\"\n      </li>\n      <li>\n        \"2\"\n      </li>\n      <li>\n        \"3\"\n      </li>\n    </ul>\n  </body>\n</html>\n";

    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    unsigned int  status = 0;
    pchtml_html_document_t *document = NULL;
    pchtml_html_element_t *element = NULL;
    pchtml_html_body_element_t * body = NULL;
    pchtml_html_parser_t * parser = NULL;
    purc_rwstream_t rwstream = NULL;

    static const unsigned char html[] = "<div><span>blah-blah-blah</div>";
    static const unsigned char inner[] = "<ul><li>1<li>2<li>3</ul>";

    parser = pchtml_html_parser_create();
    status = pchtml_html_parser_init(parser);

    if (status != PCHTML_STATUS_OK) {
        printf("Failed to create HTML parser\n");
    }

    /* Parse */
    rwstream = purc_rwstream_new_from_mem((void *)html, (size_t)strlen((const char *) html));
    document = pchtml_html_parse(parser, rwstream);
    if (document == NULL) {
        printf("Failed to create Document object\n");
    }

    purc_rwstream_close(rwstream);
    purc_rwstream_destroy (rwstream);
    pchtml_html_parser_destroy(parser);

    printf("HTML Tree Before:\n");
    // create rwstream object with buffer for serilization
    rwstream = purc_rwstream_new_from_mem(buffer, 8192);
    ret = pchtml_doc_write_to_stream(document, rwstream);
    ASSERT_EQ(ret, 0);
    serialization = purc_rwstream_get_mem_buffer (rwstream, &size);
    ret = strncmp(serialization, before, strlen(before)- 1);
    ASSERT_EQ(ret, 0);

    purc_rwstream_close(rwstream);
    purc_rwstream_destroy (rwstream);


    /* Get BODY elemenet */
    body = pchtml_html_document_body_element(document);
    rwstream = purc_rwstream_new_from_mem((void *)inner, (size_t)strlen((const char *) inner));
    element = pchtml_html_element_inner_html_set(((pchtml_html_element_t *) (body)),
                                              rwstream);
    ASSERT_NE(element, nullptr);

    /* Print Result */
    printf("Tree after innerHTML set:\n");
    // create rwstream object with buffer for serilization
    memset(buffer, 0, 8192);
    rwstream = purc_rwstream_new_from_mem(buffer, 8192);
    ret = pchtml_doc_write_to_stream(document, rwstream);
    ASSERT_EQ(ret, 0);
    serialization = purc_rwstream_get_mem_buffer (rwstream, &size);
    ret = strncmp(serialization, after, strlen(after) - 1);
    ASSERT_EQ(ret, 0);

    purc_rwstream_close(rwstream);
    purc_rwstream_destroy (rwstream);

    /* Destroy all */
    pchtml_html_document_destroy(document);

    purc_cleanup ();
}
