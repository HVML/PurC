#include "purc.h"
#include "private/html.h"
#include "private/edom.h"

#include <limits.h>
#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* NOTE:
    This PurC HTML parser is derived from lexbor parser. In lexbor parser,
    it exports lots of APIs for other modules.
    But for concise reason, in this version of purc html parser, only
    exports limited APIs.
    The user can move API reference from source code to include/private/html.h,
    and remove WTF_INTERNAL, when it is necessary.
    For convenience in this test case, when use APIs whose are not exported,
    I include the header file in source code directory directly.
*/


/* How to use html moudle
   PARSE HTML STREAM
   There are two ways to use html moudle to parse a html string:
   1. use parser and document

         pchtml_html_parser_t *parser = NULL;
         pchtml_html_document_t *doc = NULL;
         purc_rwstream_t rwstream =  purc_rwstream_new_xxxxxx (); 
         
         parser = pchtml_html_parser_create();      // create parser
         status = pchtml_html_parser_init(parser);  // initialize parser
         doc = pchtml_html_parse(parser, rwstream); // parse and get tree

         pchtml_html_parser_destroy(parser);        // destroy parse
         pchtml_html_document_destroy(doc);         // destroy document

    2. only use document

         pchtml_html_document_t *doc = NULL;
         purc_rwstream_t rwstream =  purc_rwstream_new_xxxxxx (); 

         doc = pchtml_html_document_create();       // create document
         status = pchtml_html_document_parse(doc, rwstream); // parse and get tree

         pchtml_html_document_destroy(doc);         // destroy document


     PARSE CHUNK
     1. use parser and document

         pchtml_html_parser_t *parser = NULL;
         pchtml_html_document_t *doc = NULL;
         purc_rwstream_t rwstream =  purc_rwstream_new_xxxxxx (); 
         
         parser = pchtml_html_parser_create();      // create parser
         status = pchtml_html_parser_init(parser);  // initialize parser

         doc = pchtml_html_parse_chunk_begin(parser);   // create document

         // use statement below repeatedly
         status = pchtml_html_parse_chunk_process(parser, rwstream);

         status = pchtml_html_parse_chunk_end(parser);

         pchtml_html_parser_destroy(parser);        // destroy parse
         pchtml_html_document_destroy(doc);         // destroy document

     2. only use document

         pchtml_html_document_t *doc = NULL;
         purc_rwstream_t rwstream =  purc_rwstream_new_xxxxxx (); 

         doc = pchtml_html_document_create();       // create document

         status = pchtml_html_document_parse_chunk_begin(doc);

         // use statement below repeatedly
         status = pchtml_html_document_parse_chunk(doc, rwstream);
         status = pchtml_html_document_parse_chunk_end(doc);

         pchtml_html_document_destroy(doc);         // destroy document
*/

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
    sprintf(result_file, "%s/%s.file", data_path, file_name);

    printf("HTML FILE TEST: %s.html :", file_name);

    // whether the file exists
    if ((access(test_file, F_OK | R_OK)) != 0)
    {
        printf(" ERROR, %s.html does not exist.\n", file_name);
        return;
    }
    if ((access(result_file, F_OK | R_OK)) != 0)
    {
        printf(" ERROR, %s.file does not exist.\n", file_name);
        return;
    }
    if ((stat(result_file, &file_stat) < 0) || (file_stat.st_size == 0))
    {
        printf(" ERROR, %s.file is empty.\n", file_name);
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
    serialization = (const char*)purc_rwstream_get_mem_buffer (rwstream, &size);

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
    sprintf(result_file, "%s/%s.chunk", data_path, file_name);

    printf("HTML CHUNK TEST: %s.html :", file_name);

    // whether the file exists
    if ((access(test_file, F_OK | R_OK)) != 0)
    {
        printf(" ERROR, %s.html does not exist.\n", file_name);
        return;
    }
    if ((access(result_file, F_OK | R_OK)) != 0)
    {
        printf(" ERROR, %s.chunk does not exist.\n", file_name);
        return;
    }
    if ((stat(result_file, &file_stat) < 0) || (file_stat.st_size == 0))
    {
        printf(" ERROR, %s.chunk is empty.\n", file_name);
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
    serialization = (const char*)purc_rwstream_get_mem_buffer (rwstream, &size);

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

#include "html/interfaces/document.h"

void test_parser_fragment(char * data_path, char * file_name)
{
    int ret = 0;
    size_t size = 0;
    purc_rwstream_t rwstream = NULL;
    pchtml_html_document_t *document = NULL;
    const char * serialization = NULL;
    struct stat file_stat;
    off_t off;
    size_t read_length = 0;
    FILE * fp = NULL;

    pchtml_html_element_t *element = NULL;
    pchtml_html_body_element_t * body = NULL;
    pchtml_html_parser_t * parser = NULL;

    // it is the buffer for compare result
    char test_file[8192] = {0};
    char test_frag[8192] = {0};
    char result_file[8192] = {0};

    // get the file name
    sprintf(test_file, "%s/%s.html", data_path, file_name);
    sprintf(test_frag, "%s/%s.frag", data_path, file_name);
    sprintf(result_file, "%s/%s.file", data_path, file_name);

    printf("HTML FRAGMNET TEST: %s.html :", file_name);

    // whether the file exists
    if ((access(test_file, F_OK | R_OK)) != 0)
    {
        printf(" ERROR, %s.html does not exist.\n", file_name);
        return;
    }
    if ((access(test_frag, F_OK | R_OK)) != 0)
    {
        printf(" ERROR, %s.frag does not exist.\n", file_name);
        return;
    }
    if ((access(result_file, F_OK | R_OK)) != 0)
    {
        printf(" ERROR, %s.file does not exist.\n", file_name);
        return;
    }
    if ((stat(result_file, &file_stat) < 0) || (file_stat.st_size == 0))
    {
        printf(" ERROR, %s.file is empty.\n", file_name);
        return;
    }
    
    // initialize the instance
    purc_instance_extra_info info = {0, 0};
    ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    parser = pchtml_html_parser_create();
    ret = pchtml_html_parser_init(parser);

    if (ret != PCHTML_STATUS_OK) {
        printf("Failed to create HTML parser\n");
    }

    /* Parse */
    rwstream = purc_rwstream_new_from_file (test_file, "r");
    ASSERT_NE(rwstream, nullptr);

    off = purc_rwstream_seek(rwstream, 0, SEEK_SET);
    ASSERT_NE(off, -1);

    document = pchtml_html_parse(parser, rwstream);
    if (document == NULL) {
        printf("Failed to create Document object\n");
    }

    purc_rwstream_close(rwstream);
    purc_rwstream_destroy (rwstream);
    pchtml_html_parser_destroy(parser);

    /* Get BODY elemenet */
    body = pchtml_html_document_body_element(document);

    rwstream = purc_rwstream_new_from_file (test_frag, "r");
    ASSERT_NE(rwstream, nullptr);

    off = purc_rwstream_seek(rwstream, 0, SEEK_SET);
    ASSERT_NE(off, -1);
    element = pchtml_html_element_inner_html_set(((pchtml_html_element_t *) (body)),
                                              rwstream);
    ASSERT_NE(element, nullptr);

    /* Print Result */
    // create rwstream object with buffer for serilization
    memset(test_file, 0, 8192);
    rwstream = purc_rwstream_new_from_mem(test_file, 8192);
    ret = pchtml_doc_write_to_stream(document, rwstream);
    ASSERT_EQ(ret, 0);
    serialization = (const char*)purc_rwstream_get_mem_buffer (rwstream, &size);

    read_length = 8192 > file_stat.st_size? file_stat.st_size: 8192;
    fp = fopen(result_file, "r");
    read_length = fread(result_file, 1, read_length, fp);
    fclose(fp);

    ret = strncmp(serialization, result_file, read_length - 1);
    ASSERT_EQ(ret, 0);

    // destroy rwstream
    purc_rwstream_close(rwstream);
    purc_rwstream_destroy (rwstream);

    /* Destroy all */
    pchtml_html_document_destroy(document);

    // clean instance
    purc_cleanup ();

    printf(" OK\n");
}


void test_parser_attribution(char * data_path, char * file_name)
{
    int ret = 0;
    purc_rwstream_t rwstream = NULL;
    pchtml_html_parser_t * parser = NULL;
    pchtml_html_document_t * document = NULL;
    pchtml_html_body_element_t * body = NULL;
    pcedom_collection_t * collection = NULL;
    pcedom_element_t * element = NULL;
    pcedom_attr_t * attr = NULL;
    off_t off;
    const char * serialization = NULL;
    struct stat file_stat;
    size_t read_length = 0;
    FILE * fp = NULL;
    size_t size = 0;

    bool is_exist = false;
    const char * value = NULL;
    size_t value_len = 0;

    const unsigned char name[] = "my-name";
    size_t name_size = sizeof(name) - 1;

    char test_file[8192] = {0};
    char result_file[8192] = {0};
    char remove_file[8192] = {0};

    // get the file name
    sprintf(test_file, "%s/%s.html", data_path, file_name);
    sprintf(result_file, "%s/%s1.file", data_path, file_name);
    sprintf(remove_file, "%s/%s2.file", data_path, file_name);

    printf("HTML ATTRIBUTION TEST: %s.html :", file_name);

    // whether the file exists
    if ((access(test_file, F_OK | R_OK)) != 0)
    {
        printf(" ERROR, %s.html does not exist.\n", file_name);
        return;
    }
    if ((access(result_file, F_OK | R_OK)) != 0)
    {
        printf(" ERROR, %s1.file does not exist.\n", file_name);
        return;
    }
    if ((access(remove_file, F_OK | R_OK)) != 0)
    {
        printf(" ERROR, %s2.file does not exist.\n", file_name);
        return;
    }
    if ((stat(result_file, &file_stat) < 0) || (file_stat.st_size == 0))
    {
        printf(" ERROR, %s1.file is empty.\n", file_name);
        return;
    }

    // initialize instance
    purc_instance_extra_info info = {0, 0};
    ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    // create parser
    parser = pchtml_html_parser_create();
    ret = pchtml_html_parser_init(parser);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    // create rwstream for parser
    rwstream = purc_rwstream_new_from_file (test_file, "r");
    ASSERT_NE(rwstream, nullptr);

    off = purc_rwstream_seek(rwstream, 0, SEEK_SET);
    ASSERT_NE(off, -1);

    // parse the file
    document = pchtml_html_parse(parser, rwstream);
    ASSERT_NE(document, nullptr);

    // clean the environment
    purc_rwstream_close(rwstream);
    purc_rwstream_destroy (rwstream);
    pchtml_html_parser_destroy(parser);

    // add new attribute my_name = "oh God" for div node
    // Create Collection for elements
    collection = pcedom_collection_make(&document->dom_document, 16);
    ASSERT_NE(collection, nullptr);

    // Get BODY elemenet (root for search)
    body = pchtml_html_document_body_element(document);
    element = pcedom_interface_element(body);

    // Find all DIV eleemnt, and put it in collection
    ret = pcedom_elements_by_tag_name(element, collection,
                                          (const unsigned char *)"div", 3);
    ASSERT_EQ (ret, PURC_ERROR_OK);
    ASSERT_NE (pcedom_collection_length(collection), 0);

    // Append new attrtitube, get the first div node
    element = pcedom_collection_element(collection, 0);
    attr = pcedom_element_set_attribute(element, name, name_size,
                                         (const unsigned char *) "oh God", 6);
    ASSERT_NE(attr, nullptr);

    // serialize document
    memset(test_file, 0, 8192);
    rwstream = purc_rwstream_new_from_mem(test_file, 8192);
    ret = pchtml_doc_write_to_stream(document, rwstream);
    ASSERT_EQ(ret, 0);
    serialization = (const char*)purc_rwstream_get_mem_buffer (rwstream, &size);

    // get the result
    read_length = 8192 > file_stat.st_size? file_stat.st_size: 8192;
    fp = fopen(result_file, "r");
    read_length = fread(result_file, 1, read_length, fp);
    fclose(fp);

#if 0
    printf("\n%s\n", serialization);
    int i = 0;
    for (i = 0; i < 200; i++)
        printf ("%c  ----  %c  ---- %d\n", *(serialization + i), *(result_file + i), i);
#endif
    ret = strncmp(serialization, result_file, read_length - 1);
    ASSERT_EQ(ret, 0);

    // destroy rwstream
    purc_rwstream_close(rwstream);
    purc_rwstream_destroy (rwstream);

    // Check whether new attribution exist
    is_exist = pcedom_element_has_attribute(element, name, name_size);
    ASSERT_EQ (is_exist, true);

    // Get value by qualified name
    value = (const char *)pcedom_element_get_attribute(element, name, name_size, &value_len);
    ASSERT_NE(value, nullptr);
    ASSERT_STREQ(value, "oh God");

    // change value
    ret = pcedom_attr_set_value(attr, (const unsigned char *) "new value", 9);
    ASSERT_EQ(ret, 0);
    value = (const char *)pcedom_element_get_attribute(element, name, name_size, &value_len);
    ASSERT_NE(value, nullptr);
    ASSERT_STREQ(value, "new value");

    // Remove new attrtitube by name
    pcedom_element_remove_attribute(element, name, name_size);

    // serialize document
    memset(test_file, 0, 8192);
    rwstream = purc_rwstream_new_from_mem(test_file, 8192);
    ret = pchtml_doc_write_to_stream(document, rwstream);
    ASSERT_EQ(ret, 0);
    serialization = (const char*)purc_rwstream_get_mem_buffer (rwstream, &size);

    if ((stat(remove_file, &file_stat) < 0) || (file_stat.st_size == 0))
    {
        printf(" ERROR, %s2.file is empty.\n", file_name);
        return;
    }
    // get the result
    read_length = 8192 > file_stat.st_size? file_stat.st_size: 8192;
    fp = fopen(remove_file, "r");
    read_length = fread(remove_file, 1, read_length, fp);
    fclose(fp);

    ret = strncmp(serialization, remove_file, read_length - 1);
    ASSERT_EQ(ret, 0);

    purc_rwstream_close(rwstream);
    purc_rwstream_destroy (rwstream);

    /* Destroy all */
    pchtml_html_document_destroy(document);

    // clean instance
    purc_cleanup ();

    printf(" OK\n");
}



TEST(html, html_parser_html)
{
    // get test_list file directory
    char* data_path = getenv("HTML_TEST_PATH");

    if (data_path) {
        char file_path[1024] = {0};
        strcpy (file_path, data_path);
        strcat (file_path, "/test_parser_list");   //full path of test_list

        FILE* fp = fopen(file_path, "r");   // open test_list
        if (fp) {
            char* line = NULL;
            size_t sz = 0;
            ssize_t read = 0;

            while ((read = getline(&line, &sz, fp)) != -1) {
                *(line + read - 1) = 0;
                test_html_file(data_path, line);     // get test file name
                test_html_chunk(data_path, line);
            }
            fclose(fp);
        }
    }
}

TEST(html, html_parser_fragment)
{
    // get test_list file directory
    char* data_path = getenv("HTML_TEST_PATH");

    if (data_path) {
        char file_path[1024] = {0};
        strcpy (file_path, data_path);
        strcat (file_path, "/test_fragment_list");   //full path of test_list

        FILE* fp = fopen(file_path, "r");   // open test_list
        if (fp) {
            char* line = NULL;
            size_t sz = 0;
            ssize_t read = 0;

            while ((read = getline(&line, &sz, fp)) != -1) {
                *(line + read - 1) = 0;
                test_parser_fragment(data_path, line);     // get test file name
            }
            fclose(fp);
        }
    }
}


TEST(html, html_parser_attribution)
{
    // get test_list file directory
    char* data_path = getenv("HTML_TEST_PATH");

    if (data_path) {
        char file_path[1024] = {0};
        strcpy (file_path, data_path);
        strcat (file_path, "/test_attribution_list");   //full path of test_list

        FILE* fp = fopen(file_path, "r");   // open test_list
        if (fp) {
            char* line = NULL;
            size_t sz = 0;
            ssize_t read = 0;

            while ((read = getline(&line, &sz, fp)) != -1) {
                *(line + read - 1) = 0;
                test_parser_attribution(data_path, line);     // get test file name
            }
            fclose(fp);
        }
    }
}


