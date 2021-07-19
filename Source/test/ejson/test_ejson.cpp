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
