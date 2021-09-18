#include "purc.h"
#include "private/vdom.h"
#include "private/hvml.h"
#include "hvml-token.h"
#include "hvml-parser.h"

#include <gtest/gtest.h>

TEST(vdom_construction, basic)
{
    struct pcvdom_construction_stack *stack;
    stack = pcvdom_construction_stack_create();
    if (!stack)
        goto end;

    struct pcvdom_document *doc;
    doc = pcvdom_construction_stack_end(stack);

end:
    if (stack)
        pcvdom_construction_stack_destroy(stack);
    if (doc)
        pcvdom_document_destroy(doc);
}

TEST(vdom_construction, file)
{
    int r = 0;
    const char *src = NULL;
    FILE *fin = NULL;
    purc_rwstream_t rin = NULL;
    struct pchvml_parser *parser = NULL;
    struct pcvdom_construction_stack *stack = NULL;
    struct pcvdom_document *doc = NULL;
    struct pchvml_token *token = NULL;

    purc_instance_extra_info info = {0, 0};
    r = purc_init("cn.fmsoft.hybridos.test",
        "vdom_construction", &info);
    ASSERT_EQ(r, PURC_ERROR_OK);

    src = getenv("SOURCE_FILE");
    ASSERT_NE(src, nullptr) << "You shall specify via env `SOURCE_FILE`"
                            << std::endl;

    fin = fopen(src, "r");
    if (!fin) {
        int err = errno;
        EXPECT_NE(fin, nullptr) << "Failed to open ["
            << src << "]: [" << err << "]" << strerror(err) << std::endl;
        goto end;
    }

    rin = purc_rwstream_new_from_unix_fd(dup(fileno(fin)), 1024);
    if (!rin) {
        EXPECT_NE(rin, nullptr);
        goto end;
    }

    parser = pchvml_create(0, 0);
    if (!parser)
        goto end;

    stack = pcvdom_construction_stack_create();
    if (!stack)
        goto end;

again:
    if (token)
        pchvml_token_destroy(token);
    token = pchvml_next_token(parser, rin);

    if (0==pcvdom_construction_stack_push_token(stack, token)) {
        if (pchvml_token_is_type(token, PCHVML_TOKEN_EOF)) {
            doc = pcvdom_construction_stack_end(stack);
            goto end;
        }
        goto again;
    }

    ASSERT_TRUE(false) << "failed parsing" << std::endl;

end:
    if (token)
        pchvml_token_destroy(token);

    if (doc)
        pcvdom_document_destroy(doc);

    if (stack)
        pcvdom_construction_stack_destroy(stack);

    if (parser)
        pchvml_destroy(parser);

    if (rin)
        purc_rwstream_destroy(rin);

    if (fin)
        fclose(fin);

    purc_cleanup ();
}

