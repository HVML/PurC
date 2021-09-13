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


TEST(hvml_tokenization, attribute)
{
    char hvml[] = "<hvml name=\"attr1\" vv=attr2></hvml>";
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

    size_t attr_size = 0;
    struct pchvml_token_attr* attr = NULL;
    const char* attr_name = NULL;
    const struct pcvcm_node* vcm = NULL;

    attr_size = pchvml_token_get_attr_size(token);
    ASSERT_EQ(attr_size, 2);

    attr = pchvml_token_get_attr(token, 0);
    ASSERT_NE(attr, nullptr);
    attr_name = pchvml_token_attr_get_name(attr);
    ASSERT_NE(attr_name, nullptr);
    ASSERT_STREQ(attr_name, "name");
    vcm = pchvml_token_attr_get_value(attr);
    ASSERT_NE(vcm, nullptr);
    ASSERT_EQ(vcm->type, PCVCM_NODE_TYPE_STRING);
    ASSERT_STREQ((char*)vcm->data.sz_ptr[1], "attr1");

    attr = pchvml_token_get_attr(token, 1);
    ASSERT_NE(attr, nullptr);
    attr_name = pchvml_token_attr_get_name(attr);
    ASSERT_NE(attr_name, nullptr);
    ASSERT_STREQ(attr_name, "vv");
    vcm = pchvml_token_attr_get_value(attr);
    ASSERT_NE(vcm, nullptr);
    ASSERT_EQ(vcm->type, PCVCM_NODE_TYPE_STRING);
    ASSERT_STREQ((char*)vcm->data.sz_ptr[1], "attr2");

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

TEST(hvml_tokenization, attr_no_value)
{
    char hvml[] = "<hvml attr></hvml>";
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

    size_t attr_size = 0;
    struct pchvml_token_attr* attr = NULL;
    const char* attr_name = NULL;
    const struct pcvcm_node* vcm = NULL;

    attr_size = pchvml_token_get_attr_size(token);
    ASSERT_EQ(attr_size, 1);

    attr = pchvml_token_get_attr(token, 0);
    ASSERT_NE(attr, nullptr);
    attr_name = pchvml_token_attr_get_name(attr);
    ASSERT_NE(attr_name, nullptr);
    ASSERT_STREQ(attr_name, "attr");
    vcm = pchvml_token_attr_get_value(attr);
    ASSERT_EQ(vcm, nullptr);

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
