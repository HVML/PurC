#include "purc.h"
#include "purc-rwstream.h"
#include "private/html.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

unsigned int
serializer_callback(const unsigned char  *data, size_t len, void *ctx)
{
    UNUSED_PARAM(ctx);
    printf("%.*s", (int) len, (const char *) data);

    return PCHTML_STATUS_OK;
}

// to test:
TEST(html, html_parser_chunk)
{
    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    unsigned int  status = 0;
    pchtml_html_document_t *document;

    static const unsigned char html[][64] = {
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


    purc_rwstream_t rwstream = NULL; 

    /* Initialization */
    document = pchtml_html_document_create();
    if (document == NULL) {
        printf("Failed to create HTML Document");
    }

    /* Parse HTML */
    status = pchtml_html_document_parse_chunk_begin(document);
    if (status != PCHTML_STATUS_OK) {
        printf("Failed to parse HTML");
    }

    printf("Incoming HTML chunks:");

    for (size_t i = 0; html[i][0] != '\0'; i++) {
        printf("%s", (const char *) html[i]);

        rwstream = purc_rwstream_new_from_mem((void *)html[i], (size_t)strlen((const char *) html[i]));

        status = pchtml_html_document_parse_chunk(document, rwstream,
                                               strlen((const char *) html[i]));
        if (status != PCHTML_STATUS_OK) {
            printf("Failed to parse HTML chunk");
        }

        purc_rwstream_destroy (rwstream);
    }

    status = pchtml_html_document_parse_chunk_end(document);
    if (status != PCHTML_STATUS_OK) {
        printf("Failed to parse HTML");
    }

    /* Print Result */
    printf("\nHTML Tree:");
    pchtml_html_serialize_pretty_tree_cb((pcedom_node_t *)document,
                                          0x00, 0, serializer_callback, NULL);

    /* Destroy document */
    pchtml_html_document_destroy(document);

    purc_cleanup ();
}
