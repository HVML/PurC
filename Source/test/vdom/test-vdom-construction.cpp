#include "purc.h"
#include "private/vdom.h"
#include "private/hvml.h"
#include "hvml-token.h"
#include "hvml-parser.h"

#include <gtest/gtest.h>

TEST(vdom_gen, basic)
{
    struct pcvdom_gen *gen;
    gen = pcvdom_gen_create();
    if (!gen)
        goto end;

    struct pcvdom_document *doc;
    doc = pcvdom_gen_end(gen);

end:
    if (gen)
        pcvdom_gen_destroy(gen);
    if (doc)
        pcvdom_document_destroy(doc);
}

TEST(vdom_gen, file)
{
    int r = 0;
    const char *src = NULL;
    FILE *fin = NULL;
    purc_rwstream_t rin = NULL;
    struct pchvml_parser *parser = NULL;
    struct pcvdom_gen *gen = NULL;
    struct pcvdom_document *doc = NULL;
    struct pchvml_token *token = NULL;

    purc_instance_extra_info info = {0, 0};
    r = purc_init("cn.fmsoft.hybridos.test",
        "vdom_gen", &info);
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

    gen = pcvdom_gen_create();
    if (!gen)
        goto end;

again:
    if (token)
        pchvml_token_destroy(token);
    token = pchvml_next_token(parser, rin);

    if (0==pcvdom_gen_push_token(gen, token)) {
        if (pchvml_token_is_type(token, PCHVML_TOKEN_EOF)) {
            doc = pcvdom_gen_end(gen);
            goto end;
        }
        goto again;
    }

    EXPECT_TRUE(false) << "failed parsing" << std::endl;

end:
    if (token)
        pchvml_token_destroy(token);

    if (doc)
        pcvdom_document_destroy(doc);

    if (gen)
        pcvdom_gen_destroy(gen);

    if (parser)
        pchvml_destroy(parser);

    if (rin)
        purc_rwstream_destroy(rin);

    if (fin)
        fclose(fin);

    purc_cleanup ();
}

