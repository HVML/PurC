#include "purc.h"

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
    ASSERT_EQ(parser->state, EJSON_INIT_STATE);
    ASSERT_EQ(parser->depth, 10);
    ASSERT_EQ(parser->flags, 1);

    parser->state = EJSON_FINISHED_STATE;
    ASSERT_NE(parser->state, EJSON_INIT_STATE);
    ASSERT_EQ(parser->state, EJSON_FINISHED_STATE);

    pcejson_reset(parser, 20, 2);
    ASSERT_EQ(parser->state, EJSON_INIT_STATE);
    ASSERT_EQ(parser->depth, 20);
    ASSERT_EQ(parser->flags, 2);

    pcejson_destroy(parser);
}

TEST(ejson_token, create_destroy)
{
    struct pcejson_token* token = pcejson_token_new(EJSON_TOKEN_START_OBJECT, NULL);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_EQ(token->buf, nullptr);

    pcejson_token_destroy(token);
}

TEST(ejson_token, next_token)
{
    char json[] = "{ \"key\" : \"value\" }";

    purc_rwstream_t rws = purc_rwstream_new_from_mem(json, strlen(json));
    struct pcejson* parser = pcejson_create(10, 1);

    struct pcejson_token* token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);

    pcejson_token_destroy(token);
    pcejson_destroy(parser);
    purc_rwstream_destroy(rws);
}

TEST(ejson_stack, new_destory)
{
    pcutils_stack* stack = pcutils_stack_new(10);
    ASSERT_NE(stack, nullptr);
    ASSERT_EQ(stack->capacity, 32);
    ASSERT_EQ(stack->last, -1);

    pcutils_stack_destroy(stack);
}

TEST(ejson_stack, push_pop)
{
    pcutils_stack* stack = pcutils_stack_new(10);
    ASSERT_NE(stack, nullptr);
    ASSERT_EQ(stack->capacity, 32);
    ASSERT_EQ(stack->last, -1);

    bool empty = pcutils_stack_is_empty(stack);
    ASSERT_EQ(empty, true);

    pcutils_stack_push(stack, 1);
    ASSERT_EQ(stack->last, 0);
    ASSERT_EQ(1, pcutils_stack_size(stack));

    uint8_t v = pcutils_stack_top(stack);
    ASSERT_EQ(v, 1);

    pcutils_stack_pop(stack);
    ASSERT_EQ(stack->last, -1);
    ASSERT_EQ(0, pcutils_stack_size(stack));

    pcutils_stack_push(stack, 1);
    pcutils_stack_push(stack, 2);
    pcutils_stack_push(stack, 3);
    pcutils_stack_push(stack, 4);
    pcutils_stack_push(stack, 5);
    pcutils_stack_push(stack, 6);
    pcutils_stack_push(stack, 7);
    pcutils_stack_push(stack, 8);
    pcutils_stack_push(stack, 9);
    pcutils_stack_push(stack, 10);
    pcutils_stack_push(stack, 11);
    pcutils_stack_push(stack, 12);
    pcutils_stack_push(stack, 13);
    pcutils_stack_push(stack, 14);
    pcutils_stack_push(stack, 15);
    pcutils_stack_push(stack, 16);
    pcutils_stack_push(stack, 17);
    pcutils_stack_push(stack, 18);
    pcutils_stack_push(stack, 19);
    pcutils_stack_push(stack, 20);
    pcutils_stack_push(stack, 21);
    pcutils_stack_push(stack, 22);
    pcutils_stack_push(stack, 23);
    pcutils_stack_push(stack, 24);
    pcutils_stack_push(stack, 25);
    pcutils_stack_push(stack, 26);
    pcutils_stack_push(stack, 27);
    pcutils_stack_push(stack, 28);
    pcutils_stack_push(stack, 29);
    pcutils_stack_push(stack, 30);
    pcutils_stack_push(stack, 31);
    pcutils_stack_push(stack, 32);
    pcutils_stack_push(stack, 33);
    ASSERT_EQ(stack->last, 32);
    ASSERT_GT(stack->capacity, 32);
    ASSERT_EQ(33, pcutils_stack_size(stack));

    pcutils_stack_push(stack, 34);
    ASSERT_EQ(stack->last, 33);
    ASSERT_GT(stack->capacity, 32);
    ASSERT_EQ(34, pcutils_stack_size(stack));

    pcutils_stack_pop(stack);
    ASSERT_EQ(stack->last, 32);
    ASSERT_EQ(33, pcutils_stack_size(stack));

    v = pcutils_stack_top(stack);
    ASSERT_EQ(v, 33);

    pcutils_stack_pop(stack);
    pcutils_stack_pop(stack);
    pcutils_stack_pop(stack);
    pcutils_stack_pop(stack);
    pcutils_stack_pop(stack);
    ASSERT_EQ(stack->last, 27);
    ASSERT_EQ(28, pcutils_stack_size(stack));

    v = pcutils_stack_top(stack);
    ASSERT_EQ(v, 28);

    pcutils_stack_destroy(stack);
}

TEST(ejson_token, parse_unquoted_key_AND_single_quoted_value)
{
    char json[] = "{ key : \'value\' }";

    purc_rwstream_t rws = purc_rwstream_new_from_mem(json, strlen(json));
    struct pcejson* parser = pcejson_create(10, 1);

    struct pcejson_token* token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ(token->buf, "value");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);

    pcejson_token_destroy(token);
    pcejson_destroy(parser);
    purc_rwstream_destroy(rws);
}

TEST(ejson_token, parse_unquoted_key_AND_double_quoted_value)
{
    char json[] = "{ key : \"value\" }";

    purc_rwstream_t rws = purc_rwstream_new_from_mem(json, strlen(json));
    struct pcejson* parser = pcejson_create(10, 1);

    struct pcejson_token* token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ(token->buf, "value");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);

    pcejson_token_destroy(token);
    pcejson_destroy(parser);
    purc_rwstream_destroy(rws);
}

TEST(ejson_token, parse_double_quoted_key_AND_single_quoted_value)
{
    char json[] = "{ \"key\" : 'value' }";

    purc_rwstream_t rws = purc_rwstream_new_from_mem(json, strlen(json));
    struct pcejson* parser = pcejson_create(10, 1);

    struct pcejson_token* token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ(token->buf, "value");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);

    pcejson_token_destroy(token);
    pcejson_destroy(parser);
    purc_rwstream_destroy(rws);
}

TEST(ejson_token, parse_double_quoted_key_AND_double_quoted_value)
{
    char json[] = "{ \"key\" : \"value\" }";

    purc_rwstream_t rws = purc_rwstream_new_from_mem(json, strlen(json));
    struct pcejson* parser = pcejson_create(10, 1);

    struct pcejson_token* token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ(token->buf, "value");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);

    pcejson_token_destroy(token);
    pcejson_destroy(parser);
    purc_rwstream_destroy(rws);
}

TEST(ejson_token, parse_single_quoted_key_AND_single_quoted_value)
{
    char json[] = "{ 'key' : 'value' }";

    purc_rwstream_t rws = purc_rwstream_new_from_mem(json, strlen(json));
    struct pcejson* parser = pcejson_create(10, 1);

    struct pcejson_token* token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ(token->buf, "value");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);

    pcejson_token_destroy(token);
    pcejson_destroy(parser);
    purc_rwstream_destroy(rws);
}

TEST(ejson_token, parse_single_quoted_key_AND_double_quoted_value)
{
    char json[] = "{ 'key' : \"value\" }";

    purc_rwstream_t rws = purc_rwstream_new_from_mem(json, strlen(json));
    struct pcejson* parser = pcejson_create(10, 1);

    struct pcejson_token* token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ(token->buf, "value");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);

    pcejson_token_destroy(token);
    pcejson_destroy(parser);
    purc_rwstream_destroy(rws);
}

TEST(ejson_token, parse_no_space_unquoted_key_with_single_quoted_value)
{
    char json[] = "{key:'value'}";

    purc_rwstream_t rws = purc_rwstream_new_from_mem(json, strlen(json));
    struct pcejson* parser = pcejson_create(10, 1);

    struct pcejson_token* token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ(token->buf, "value");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);

    pcejson_token_destroy(token);
    pcejson_destroy(parser);
    purc_rwstream_destroy(rws);
}

TEST(ejson_token, parse_no_space_unquoted_key_with_double_quoted_value)
{
    char json[] = "{key:\"value\"}";

    purc_rwstream_t rws = purc_rwstream_new_from_mem(json, strlen(json));
    struct pcejson* parser = pcejson_create(10, 1);

    struct pcejson_token* token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ(token->buf, "value");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);

    pcejson_token_destroy(token);
    pcejson_destroy(parser);
    purc_rwstream_destroy(rws);
}

TEST(ejson_token, parse_no_space_single_quoted_key_with_single_quoted_value)
{
    char json[] = "{'key':'value'}";

    purc_rwstream_t rws = purc_rwstream_new_from_mem(json, strlen(json));
    struct pcejson* parser = pcejson_create(10, 1);

    struct pcejson_token* token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ(token->buf, "value");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);

    pcejson_token_destroy(token);
    pcejson_destroy(parser);
    purc_rwstream_destroy(rws);
}

TEST(ejson_token, parse_no_space_single_quoted_key_with_double_quoted_value)
{
    char json[] = "{'key':\"value\"}";

    purc_rwstream_t rws = purc_rwstream_new_from_mem(json, strlen(json));
    struct pcejson* parser = pcejson_create(10, 1);

    struct pcejson_token* token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ(token->buf, "value");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);

    pcejson_token_destroy(token);
    pcejson_destroy(parser);
    purc_rwstream_destroy(rws);
}

TEST(ejson_token, parse_no_space_double_quoted_key_with_single_quoted_value)
{
    char json[] = "{\"key\":'value'}";

    purc_rwstream_t rws = purc_rwstream_new_from_mem(json, strlen(json));
    struct pcejson* parser = pcejson_create(10, 1);

    struct pcejson_token* token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ(token->buf, "value");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);

    pcejson_token_destroy(token);
    pcejson_destroy(parser);
    purc_rwstream_destroy(rws);
}

TEST(ejson_token, parse_no_space_double_quoted_key_with_double_quoted_value)
{
    char json[] = "{\"key\":\"value\"}";

    purc_rwstream_t rws = purc_rwstream_new_from_mem(json, strlen(json));
    struct pcejson* parser = pcejson_create(10, 1);

    struct pcejson_token* token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ(token->buf, "value");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);

    pcejson_token_destroy(token);
    pcejson_destroy(parser);
    purc_rwstream_destroy(rws);
}

TEST(ejson_token, parse_true_false)
{
    char json[] = "{key:true,key2:false}";

    purc_rwstream_t rws = purc_rwstream_new_from_mem(json, strlen(json));
    struct pcejson* parser = pcejson_create(10, 1);

    struct pcejson_token* token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_BOOLEAN);
    ASSERT_STREQ(token->buf, "true");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_COMMA);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "key2");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_BOOLEAN);
    ASSERT_STREQ(token->buf, "false");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);

    pcejson_token_destroy(token);
    pcejson_destroy(parser);
    purc_rwstream_destroy(rws);
}

TEST(ejson_token, parse_array)
{
    char json[] = "{key:[\"a\", \"b\"]}";

    purc_rwstream_t rws = purc_rwstream_new_from_mem(json, strlen(json));
    struct pcejson* parser = pcejson_create(10, 1);

    struct pcejson_token* token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_ARRAY);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ(token->buf, "a");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_COMMA);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ(token->buf, "b");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_ARRAY);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);

    pcejson_token_destroy(token);
    pcejson_destroy(parser);
    purc_rwstream_destroy(rws);
}

TEST(ejson_token, parse_object)
{
    char json[] = "{key:{\"a\":\"b\"},key2:'v2'}";

    purc_rwstream_t rws = purc_rwstream_new_from_mem(json, strlen(json));
    struct pcejson* parser = pcejson_create(10, 1);

    struct pcejson_token* token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "a");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ(token->buf, "b");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_COMMA);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "key2");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ(token->buf, "v2");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);

    pcejson_token_destroy(token);
    pcejson_destroy(parser);
    purc_rwstream_destroy(rws);
}

TEST(ejson_token, parse_object_and_array)
{
    char json[] = "{key:[{\"a\":\"b\"},{key2:'v2'}]}";

    purc_rwstream_t rws = purc_rwstream_new_from_mem(json, strlen(json));
    struct pcejson* parser = pcejson_create(10, 1);

    struct pcejson_token* token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_ARRAY);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "a");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ(token->buf, "b");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_COMMA);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "key2");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ(token->buf, "v2");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_ARRAY);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);

    pcejson_token_destroy(token);
    pcejson_destroy(parser);
    purc_rwstream_destroy(rws);
}

TEST(ejson_token, parse_number)
{
    char json[1024] = "{key:123}";

    purc_rwstream_t rws = purc_rwstream_new_from_mem(json, strlen(json));
    struct pcejson* parser = pcejson_create(10, 1);

    struct pcejson_token* token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_NUMBER);
    ASSERT_STREQ(token->buf, "123");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);
    pcejson_token_destroy(token);

    purc_rwstream_destroy(rws);

    // reset
    pcejson_reset(parser, 10, 1);
    strcpy(json, "{key:-123}");
    rws = purc_rwstream_new_from_mem(json, strlen(json));

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_NUMBER);
    ASSERT_STREQ(token->buf, "-123");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);
    pcejson_token_destroy(token);

    purc_rwstream_destroy(rws);

    pcejson_destroy(parser);
}

TEST(ejson_token, parse_float_number)
{
    char json[1024] = "{key:1.23}";

    purc_rwstream_t rws = purc_rwstream_new_from_mem(json, strlen(json));
    struct pcejson* parser = pcejson_create(10, 1);

    struct pcejson_token* token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_NUMBER);
    ASSERT_STREQ(token->buf, "1.23");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);
    pcejson_token_destroy(token);

    purc_rwstream_destroy(rws);

    // reset
    pcejson_reset(parser, 10, 1);
    strcpy(json, "{key:1.2e3}");
    rws = purc_rwstream_new_from_mem(json, strlen(json));

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_NUMBER);
    ASSERT_STREQ(token->buf, "1.2e3");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);
    pcejson_token_destroy(token);

    // reset
    pcejson_reset(parser, 10, 1);
    strcpy(json, "{key:1.211111e-3}");
    rws = purc_rwstream_new_from_mem(json, strlen(json));

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_NUMBER);
    ASSERT_STREQ(token->buf, "1.211111e-3");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);
    pcejson_token_destroy(token);

    purc_rwstream_destroy(rws);

    pcejson_destroy(parser);
}

TEST(ejson_token, parse_comma)
{
    char json[1024] = "{key:1.23,}";

    purc_rwstream_t rws = purc_rwstream_new_from_mem(json, strlen(json));
    struct pcejson* parser = pcejson_create(10, 1);

    struct pcejson_token* token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_NUMBER);
    ASSERT_STREQ(token->buf, "1.23");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_COMMA);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);
    pcejson_token_destroy(token);

    purc_rwstream_destroy(rws);

    // reset
    pcejson_reset(parser, 10, 1);
    strcpy(json, "{key:['a','b',]}");
    rws = purc_rwstream_new_from_mem(json, strlen(json));

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_ARRAY);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ(token->buf, "a");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_COMMA);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ(token->buf, "b");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_COMMA);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_ARRAY);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);
    pcejson_token_destroy(token);

    purc_rwstream_destroy(rws);

    pcejson_destroy(parser);
}

TEST(ejson_token, parse_number_with_suffix)
{
    char json[1024] = "{key:123456789L,}";

    purc_rwstream_t rws = purc_rwstream_new_from_mem(json, strlen(json));
    struct pcejson* parser = pcejson_create(10, 1);

    struct pcejson_token* token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_LONG_INT);
    ASSERT_STREQ(token->buf, "123456789L");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_COMMA);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);
    pcejson_token_destroy(token);

    purc_rwstream_destroy(rws);

    // reset
    pcejson_reset(parser, 10, 1);
    strcpy(json, "{key:123456789UL}");
    rws = purc_rwstream_new_from_mem(json, strlen(json));

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_ULONG_INT);
    ASSERT_STREQ(token->buf, "123456789UL");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);
    pcejson_token_destroy(token);

    purc_rwstream_destroy(rws);

    // reset
    pcejson_reset(parser, 10, 1);
    strcpy(json, "{key:1.23456789FL}");
    rws = purc_rwstream_new_from_mem(json, strlen(json));

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_LONG_DOUBLE);
    ASSERT_STREQ(token->buf, "1.23456789FL");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);
    pcejson_token_destroy(token);

    purc_rwstream_destroy(rws);

    // reset
    pcejson_reset(parser, 10, 1);
    strcpy(json, "{key:123456789FL}");
    rws = purc_rwstream_new_from_mem(json, strlen(json));

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_LONG_DOUBLE);
    ASSERT_STREQ(token->buf, "123456789FL");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);
    pcejson_token_destroy(token);

    purc_rwstream_destroy(rws);

    pcejson_destroy(parser);
}

TEST(ejson_token, parse_sequence)
{
    char json[1024] = "{key:bx12345abcdf,}";

    purc_rwstream_t rws = purc_rwstream_new_from_mem(json, strlen(json));
    struct pcejson* parser = pcejson_create(10, 1);

    struct pcejson_token* token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_BYTE_SQUENCE);
    ASSERT_STREQ(token->buf, "bx12345abcdf");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_COMMA);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);
    pcejson_token_destroy(token);

    purc_rwstream_destroy(rws);

    // reset
    pcejson_reset(parser, 10, 1);
    strcpy(json, "{key:bb11.00.11.00}");
    rws = purc_rwstream_new_from_mem(json, strlen(json));

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_BYTE_SQUENCE);
    ASSERT_STREQ(token->buf, "bb11001100");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);
    pcejson_token_destroy(token);

    purc_rwstream_destroy(rws);

    // reset
    pcejson_reset(parser, 10, 1);
    strcpy(json,
        "{key:b64UHVyQyBpcyBhbiBIVk1MIHBhcnNlciBhbmQgaW50ZXJwcmV0ZXIuCiA=}");
    rws = purc_rwstream_new_from_mem(json, strlen(json));

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_BYTE_SQUENCE);
    ASSERT_STREQ(token->buf,
        "b64UHVyQyBpcyBhbiBIVk1MIHBhcnNlciBhbmQgaW50ZXJwcmV0ZXIuCiA=");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);
    pcejson_token_destroy(token);

    purc_rwstream_destroy(rws);
    pcejson_destroy(parser);
}

TEST(ejson_token, parse_text)
{
    char text[] = " this is text\n  一个长字符串\n\n 第三行啊\n";
    char json[1024];
    sprintf(json, "{key:\"\"\"%s\"\"\"}", text);

    purc_rwstream_t rws = purc_rwstream_new_from_mem(json, strlen(json));
    struct pcejson* parser = pcejson_create(10, 1);

    struct pcejson_token* token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_TEXT);
    ASSERT_STREQ(token->buf, text);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);
    pcejson_token_destroy(token);

    purc_rwstream_destroy(rws);

    pcejson_destroy(parser);
}

TEST(ejson_token, parse_unquoted_key)
{
    char json[1024] = "{k_e-y9:'v'}";

    purc_rwstream_t rws = purc_rwstream_new_from_mem(json, strlen(json));
    struct pcejson* parser = pcejson_create(10, 1);

    struct pcejson_token* token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "k_e-y9");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ(token->buf, "v");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);
    pcejson_token_destroy(token);

    purc_rwstream_destroy(rws);

    // reset
    pcejson_reset(parser, 10, 1);
    strcpy(json, "{1key:'v'}");
    rws = purc_rwstream_new_from_mem(json, strlen(json));

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);

    purc_rwstream_destroy(rws);
    pcejson_destroy(parser);
}

TEST(ejson_token, parse_escape)
{
    int ret = purc_init ("cn.fmsoft.hybridos.test", "ejson", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    char json[1024] = "{key:'abc\"'}";

    purc_rwstream_t rws = purc_rwstream_new_from_mem(json, strlen(json));
    struct pcejson* parser = pcejson_create(10, 1);

    struct pcejson_token* token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ(token->buf, "abc\"");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);
    pcejson_token_destroy(token);

    purc_rwstream_destroy(rws);

    // reset
    pcejson_reset(parser, 10, 1);
    strcpy(json, "{key:\"v'\"}");
    rws = purc_rwstream_new_from_mem(json, strlen(json));

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "key");
    pcejson_token_destroy(token);


    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ(token->buf, "v'");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);

    purc_rwstream_destroy(rws);

    // reset
    pcejson_reset(parser, 10, 1);
    strcpy(json, "{key:\"b\\\"\"}");
    rws = purc_rwstream_new_from_mem(json, strlen(json));

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "key");
    pcejson_token_destroy(token);


    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ(token->buf, "b\\\"");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);

    purc_rwstream_destroy(rws);

    // reset
    pcejson_reset(parser, 10, 1);
    strcpy(json,
        "{key:\"c\\b\\/\\f\\n\\r\\t\\uabcd\"}");
    rws = purc_rwstream_new_from_mem(json, strlen(json));

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ(token->buf, "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ(token->buf, "c\\b\\/\\f\\n\\r\\t\\uabcd");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ(token->buf, nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);

    purc_rwstream_destroy(rws);

    pcejson_destroy(parser);
    purc_cleanup ();
}

TEST(ejson_token, pcejson_parse)
{
    int ret = purc_init ("cn.fmsoft.hybridos.test", "ejson", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    char json[] = "{key:[{\"a\":\"b\"},{key2:'v2'}]}";
    purc_rwstream_t rws = purc_rwstream_new_from_mem(json, strlen(json));

    struct pcvcm_node* root = NULL;
    pcejson_parse (&root, rws);
    ASSERT_NE (root, nullptr);

    purc_variant_t vt = pcvcm_eval (root);
    ASSERT_NE(vt, PURC_VARIANT_INVALID);

    char buf[1024];
    purc_rwstream_t my_rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(my_rws, nullptr);

    size_t len_expected = 0;
    ssize_t n = purc_variant_serialize(vt, my_rws,
            0, PCVARIANT_SERIALIZE_OPT_PLAIN, &len_expected);
    ASSERT_GT(n, 0);
    buf[n] = 0;
    ASSERT_STREQ(buf, "{\"key\":[{\"a\":\"b\"},{\"key2\":\"v2\"}]}");

    purc_variant_unref(vt);
    purc_rwstream_destroy(my_rws);
    purc_rwstream_destroy(rws);
    purc_cleanup ();
}
