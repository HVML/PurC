#include "private/ejson.h"
#include "purc-rwstream.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


TEST(ejson, create_reset_destroy)
{
    struct pcejson* parser = pcejson_create(10, 1);
    ASSERT_NE(parser, nullptr);
    ASSERT_EQ(parser->state, ejson_init_state);
    ASSERT_EQ(parser->depth, 10);
    ASSERT_EQ(parser->flags, 1);

    parser->state = ejson_finished_state;
    ASSERT_NE(parser->state, ejson_init_state);
    ASSERT_EQ(parser->state, ejson_finished_state);

    pcejson_reset(parser, 20, 2);
    ASSERT_EQ(parser->state, ejson_init_state);
    ASSERT_EQ(parser->depth, 20);
    ASSERT_EQ(parser->flags, 2);

    pcejson_destroy(parser);
}

TEST(ejson_token, create_destroy)
{
    struct pcejson_token* token = pcejson_token_new(ejson_token_start_object, -1, 0);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, ejson_token_start_object);
    ASSERT_EQ(token->rws, nullptr);

    pcejson_token_destroy(token);
}

TEST(ejson_token, next_token)
{
    char json[] = "{ \"key\" : \"value\" }";

    purc_rwstream_t rws = purc_rwstream_new_from_mem(json, strlen(json));
    struct pcejson* parser = pcejson_create(10, 1);

    struct pcejson_token* token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);

    pcejson_token_destroy(token);
    pcejson_destroy(parser);
    purc_rwstream_destroy(rws);
}
