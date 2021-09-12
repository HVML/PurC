#include "purc.h"

#include "private/hvml.h"
#include "hvml-token.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>

TEST(hvml_tokenization, new_destory)
{
    struct pchvml_parser* parser = pchvml_create(0, 32);
    ASSERT_NE(parser, nullptr);

    pchvml_destroy (parser);
}


TEST(hvml_tokenization, begin_tag_and_end_tag)
{
    char hvml[] = "<hvml></hvml>";
    purc_rwstream_t rws = purc_rwstream_new_from_mem(hvml, strlen(hvml));

    struct pchvml_parser* parser = pchvml_create(0, 32);
    ASSERT_NE(parser, nullptr);

    enum pchvml_token_type type;
    struct pchvml_token* token = NULL;
    const char* name = NULL;

    token = pchvml_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    type = pchvml_token_get_type(token);
    ASSERT_EQ(type, PCHVML_TOKEN_START_TAG);
    name = pchvml_token_get_name(token);
    ASSERT_NE(name, nullptr);
    ASSERT_STREQ(name, "hvml");

    token = pchvml_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    type = pchvml_token_get_type(token);
    ASSERT_EQ(type, PCHVML_TOKEN_END_TAG);
    name = pchvml_token_get_name(token);
    ASSERT_NE(name, nullptr);
    ASSERT_STREQ(name, "hvml");

    pchvml_destroy (parser);
    purc_rwstream_destroy(rws);
}


