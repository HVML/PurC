#include "purc.h"
#include "private/html.h"
#include "private/variant.h"
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
    purc_instance_extra_info info = {};
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

    purc_rwstream_destroy (rwstream);

    // create rwstream object with buffer for serilization
    rwstream = purc_rwstream_new_from_mem(test_file, sizeof(test_file) - 1);

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
    purc_instance_extra_info info = {};
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

            purc_rwstream_destroy (rwstream);
        }
    }

    ret = pchtml_html_document_parse_chunk_end(document);
    ASSERT_EQ(ret, 0);

    // create rwstream object with buffer for serilization
    rwstream = purc_rwstream_new_from_mem(test_file, sizeof(test_file) - 1);

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
    purc_instance_extra_info info = {};
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
    memset(test_file, 0, sizeof(test_file));
    rwstream = purc_rwstream_new_from_mem(test_file, sizeof(test_file) - 1);
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
    purc_instance_extra_info info = {};
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
    memset(test_file, 0, sizeof(test_file));
    rwstream = purc_rwstream_new_from_mem(test_file, sizeof(test_file));
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
    memset(test_file, 0, sizeof(test_file));
    rwstream = purc_rwstream_new_from_mem(test_file, sizeof(test_file));
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



// an example for how to replace a node with a new edom tree.
static pcedom_node_t * get_node (pcedom_node_t *node, unsigned int tag, int *index)
{
    if (node && node->local_name == tag) {
        (*index)--;
        if ((*index) < 0)
            return node;
    }

    node = node->first_child;
    pcedom_node_t *return_node = NULL;

    while (node != NULL) {
        return_node = get_node (node, tag, index);
        if (return_node)
            return return_node;

        node = node->next;
    }

    return return_node;
}

#if 0
static unsigned int replace_node_with_fragment_chunk (
        pchtml_html_document_t *document, pcedom_node_t *root_node,
        const char fragment[][64])
{
    purc_rwstream_t rwstream = NULL;
    unsigned int status = PCHTML_STATUS_OK;
    pcedom_element_t *root_element = pcedom_interface_element (root_node);

    // start parse fragment chunk
    status = pchtml_html_document_parse_fragment_chunk_begin (document, root_element);
    if (status != PCHTML_STATUS_OK) {
        printf ("Failed to start parse HTML chunk");
        return status;
    }

    // parse fragment chunk
    for (size_t i = 0; fragment[i][0] != '\0'; i++) {
        rwstream = purc_rwstream_new_from_mem((void*)fragment[i],
                strlen((const char *) fragment[i]));

        status = pchtml_html_document_parse_fragment_chunk (document, rwstream);
        if (status != PCHTML_STATUS_OK) {
            printf ("Failed to parse HTML chunk");
            purc_rwstream_destroy (rwstream);
            return status;
        }

        purc_rwstream_destroy (rwstream);
    }

    // end parse fragment chunk, get the tree, which root is 'html'
    pcedom_node_t *fragment_root = pchtml_html_document_parse_fragment_chunk_end (document);
    if (fragment_root == NULL) {
        printf ("Failed to parse HTML");
        status = PCHTML_STATUS_ERROR;
        return status;
    }

    // remove all sons of root node
    while (root_node->first_child != NULL) {
        pcedom_node_destroy_deep(root_node->first_child);
    }

    pcedom_node_t *child = NULL;
    while (fragment_root->first_child != NULL) {
        child = fragment_root->first_child;

        pcedom_node_remove(child);
        pcedom_node_insert_child(root_node, child);
    }

    pcedom_node_destroy(fragment_root);

    return status;
}

static unsigned int replace_node_with_fragment (
        pcedom_node_t *node, const char *fragment, size_t size)
{
    purc_rwstream_t rwstream = NULL;
    unsigned int status = PCHTML_STATUS_OK;

    pchtml_html_element_t *html_element = pchtml_html_interface_element (node);
    rwstream = purc_rwstream_new_from_mem((void*)fragment, size);
    if (pchtml_html_element_inner_html_set (html_element, rwstream) == NULL)
        status = PCHTML_STATUS_ERROR;

    purc_rwstream_destroy (rwstream);
    return status;
}
#endif
static unsigned int serializer_callback(const unsigned char *data,  long unsigned int len, void *ctx)
{
    UNUSED_PARAM(ctx);
    printf("%.*s", (int) len, (const char *) data);

    return 0;
}

enum pchtml_html_serialize_opt {
    PCHTML_HTML_SERIALIZE_OPT_UNDEF               = 0x00,
    PCHTML_HTML_SERIALIZE_OPT_SKIP_WS_NODES       = 0x01,
    PCHTML_HTML_SERIALIZE_OPT_SKIP_COMMENT        = 0x02,
    PCHTML_HTML_SERIALIZE_OPT_RAW                 = 0x04,
    PCHTML_HTML_SERIALIZE_OPT_WITHOUT_CLOSING     = 0x08,
    PCHTML_HTML_SERIALIZE_OPT_TAG_WITH_NS         = 0x10,
    PCHTML_HTML_SERIALIZE_OPT_WITHOUT_TEXT_INDENT = 0x20,
    PCHTML_HTML_SERIALIZE_OPT_FULL_DOCTYPE        = 0x40
};

#if 0
TEST(html, html_parser_replace)
//int
//main(int argc, const char *argv[])
{
    purc_rwstream_t rwstream = NULL;
    unsigned int status;
    pchtml_html_document_t *doc;

    // original html
    static const char html[] = "<div><p>First<p>second</div><div><p>third<p>fourth</div>";
    size_t html_len = sizeof(html) - 1;

    // html fragment1, in string array
    static const char fragment1[][64] = {
        "<p cla",
        "ss=",
        "\"bestof",
        "class",
        "\">",
        "good for me",
        "</p>",
        "<p cla",
        "ss=",
        "\"bestof",
        "class",
        "\">",
        "good for you",
        "</p>",
        "\0"
    };

    // html fragment2, in one string
    static const char fragment2[] = "<h2>Flower</h2><img src=\"img_white_flower.jpg\" width=\"214\" height=\"204\">";

    int index = 0;

    // parse html file, get original edom tree
    // create document
    doc = pchtml_html_document_create();
    if (doc == NULL)
        return;

    rwstream = purc_rwstream_new_from_mem((void*)html, html_len);
    status = pchtml_html_document_parse(doc, rwstream);
    if (status != PCHTML_STATUS_OK) {
        printf ("Failed to parse HTML file");
    }
    purc_rwstream_destroy (rwstream);

    /* Serialization html*/
    printf("HTML Document:\n");

    status = pchtml_html_serialize_pretty_tree_cb(pcedom_interface_node(doc),
                                               PCHTML_HTML_SERIALIZE_OPT_UNDEF,
                                               0, serializer_callback, NULL);
    if (status != PCHTML_STATUS_OK) {
        printf ("Failed to serialization HTML tree");
    }


    // method 1: parse the fragment in string arry, replace the first div node
    // get the parent node, first div
    pcedom_node_t *div = get_node (&(doc->dom_document.node), PCHTML_TAG_DIV, &index);
    status = replace_node_with_fragment_chunk (doc, div, fragment1);
    if (status != PCHTML_STATUS_OK) {
        printf ("Failed to replace HTML tree");
    }

    // method 2: parse the fragment in one string, replace second div node
    // get the parent node, the second div
    index = 1;
    div = get_node (&(doc->dom_document.node), PCHTML_TAG_DIV, &index);
    status = replace_node_with_fragment (div, fragment2, strlen ((char *)fragment2));
    if (status != PCHTML_STATUS_OK) {
        printf ("Failed to replace HTML tree");
    }

    // print the result
    printf("\n\nAfter replace, HTML Document:\n");
    status = pchtml_html_serialize_pretty_tree_cb(pcedom_interface_node(doc),
                                               PCHTML_HTML_SERIALIZE_OPT_UNDEF,
                                               0, serializer_callback, NULL);
    if (status != PCHTML_STATUS_OK) {
        printf ("Failed to serialization HTML tree");
    }

    /* Destroy document*/
    pchtml_html_document_destroy(doc);
}
#endif

TEST(html, html_parser_replace)
{
    purc_rwstream_t rwstream = NULL;
    unsigned int status;
    pchtml_html_document_t *doc;

    // original html
    static const char html[] = "<div><p>First<p>second</div><div><p>third<p>fourth</div>";
    size_t html_len = sizeof(html) - 1;
    int index = 0;

    // html fragment2, in one string
    static const char fragment2[] = "<h2>Flower</h2><img src=\"img_white_flower.jpg\" width=\"214\" height=\"204\">";

/*  original tree
    <html>
        <head>
        </head>
        <body>
            <div>
                <p>
                    "First"
                </p>
                <p>
                    "second"
                </p>
            </div>

            <div>
                <p>
                    "third"
                </p>
                <p>
                    "fourth"
                </p>
            </div>
        </body>
    </html>

    fragment tree
    fragment#document
        <h2>
            "Flower"
        </h2>
        <img src="img_white_flower.jpg" width="214" height="204">
*/

    purc_instance_extra_info info = {};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    // parse html file, get original edom tree
    // create document
    doc = pchtml_html_document_create();
    if (doc == NULL)
        return;

    rwstream = purc_rwstream_new_from_mem((void*)html, html_len);
    status = pchtml_html_document_parse(doc, rwstream);
    if (status != PCHTML_STATUS_OK) {
        printf ("Failed to parse HTML file");
    }
    purc_rwstream_destroy (rwstream);

    /* Serialization html*/
    printf("HTML Document:\n");

    status = pchtml_html_serialize_pretty_tree_cb(pcedom_interface_node(doc),
                                               PCHTML_HTML_SERIALIZE_OPT_UNDEF,
                                               0, serializer_callback, NULL);
    if (status != PCHTML_STATUS_OK) {
        printf ("Failed to serialization HTML tree");
    }

    // test case1: append fragment tree under first div
/*  after append
    <html>
        <head>
        </head>
        <body>
            <div>
                <h2>
                    "Flower"
                </h2>
                <img src="img_white_flower.jpg" width="214" height="204">
            </div>

            <div>
                <p>
                    "third"
                </p>
                <p>
                    "fourth"
                </p>
            </div>
        </body>
    </html>
*/

    index = 0;
    pcedom_node_t *div = get_node (&(doc->dom_document.node), PCHTML_TAG_DIV, &index);
    if (div) {
        purc_rwstream_t rwstream = NULL;
        rwstream = purc_rwstream_new_from_mem((void*)fragment2,
                strlen((const char *) fragment2));

        // get the fragment root
        pcedom_node_t *fragment_root = pchtml_edom_document_parse_fragment (
                doc, div, rwstream);

        // set the fragment to the node. append: is the sub tree.
        pchtml_edom_insert_node (div, fragment_root, pcvariant_atom_append);

        purc_rwstream_destroy (rwstream);
    }
    // print the result
    printf("\n\nAfter replace, HTML Document:\n");
    status = pchtml_html_serialize_pretty_tree_cb(pcedom_interface_node(doc),
                                               PCHTML_HTML_SERIALIZE_OPT_UNDEF,
                                               0, serializer_callback, NULL);
    if (status != PCHTML_STATUS_OK) {
        printf ("Failed to serialization HTML tree");
    }


    // test case 2: insertBefore first div
/*  after insertBefore
    <html>
        <head>
        </head>
        <body>
            <h2>
                "Flower"
            </h2>
            <img src="img_white_flower.jpg" width="214" height="204">

            <div>
                <h2>
                    "Flower"
                </h2>
                <img src="img_white_flower.jpg" width="214" height="204">
            </div>

            <div>
                <p>
                    "third"
                </p>
                <p>
                    "fourth"
                </p>
            </div>
        </body>
    </html>
*/

    index = 0;
    div = get_node (&(doc->dom_document.node), PCHTML_TAG_DIV, &index);
    if (div) {
        purc_rwstream_t rwstream = NULL;
        rwstream = purc_rwstream_new_from_mem((void*)fragment2,
                strlen((const char *) fragment2));

        // get the fragment root
        pcedom_node_t *fragment_root = pchtml_edom_document_parse_fragment (
                doc, div, rwstream);

        // set the fragment to the node. insertBefore: before the node
        pchtml_edom_insert_node (div, fragment_root, pcvariant_atom_insertBefore);

        purc_rwstream_destroy (rwstream);
    }
    // print the result
    printf("\n\nAfter replace, HTML Document:\n");
    status = pchtml_html_serialize_pretty_tree_cb(pcedom_interface_node(doc),
                                               PCHTML_HTML_SERIALIZE_OPT_UNDEF,
                                               0, serializer_callback, NULL);
    if (status != PCHTML_STATUS_OK) {
        printf ("Failed to serialization HTML tree");
    }


    // test case 3: insertAfter second div
/*  Result:
    <html>
        <head>
        </head>
        <body>
            <h2>
                "Flower"
            </h2>
            <img src="img_white_flower.jpg" width="214" height="204">

            <div>
                <h2>
                    "Flower"
                </h2>
                <img src="img_white_flower.jpg" width="214" height="204">
            </div>

            <div>
                <p>
                    "third"
                </p>
                <p>
                    "fourth"
                </p>
            </div>

            <h2>
                "Flower"
            </h2>
            <img src="img_white_flower.jpg" width="214" height="204">
        </body>
    </html>
*/
    index = 1;
    div = get_node (&(doc->dom_document.node), PCHTML_TAG_DIV, &index);
    if (div) {
        purc_rwstream_t rwstream = NULL;
        rwstream = purc_rwstream_new_from_mem((void*)fragment2,
                strlen((const char *) fragment2));

        // get the fragment root
        pcedom_node_t *fragment_root = pchtml_edom_document_parse_fragment (
                doc, div, rwstream);

        // set the fragment to the node. insertBefore: before the node
        pchtml_edom_insert_node (div, fragment_root, pcvariant_atom_insertAfter);

        purc_rwstream_destroy (rwstream);
    }
    // print the result
    printf("\n\nAfter replace, HTML Document:\n");
    status = pchtml_html_serialize_pretty_tree_cb(pcedom_interface_node(doc),
                                               PCHTML_HTML_SERIALIZE_OPT_UNDEF,
                                               0, serializer_callback, NULL);
    if (status != PCHTML_STATUS_OK) {
        printf ("Failed to serialization HTML tree");
    }

    /* Destroy document*/
    pchtml_html_document_destroy(doc);

    // clean instance
    purc_cleanup ();

    printf(" OK\n");
}
