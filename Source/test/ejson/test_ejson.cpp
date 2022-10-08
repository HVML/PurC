/*
** Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of PurC (short for Purring Cat), an HVML interpreter.
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "purc/purc.h"

#include "private/utils.h"

#include "private/ejson.h"
#include "purc/purc-rwstream.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>


#if 0
TEST(ejson, create_reset_destroy)
{
    struct pcejson* parser = pcejson_create(10, 1);
    ASSERT_NE(parser, nullptr);
    ASSERT_EQ(parser->state, EJSON_INIT_STATE);
    ASSERT_EQ(parser->max_depth, 10);
    ASSERT_EQ(parser->flags, 1);

    parser->state = EJSON_FINISHED_STATE;
    ASSERT_NE(parser->state, EJSON_INIT_STATE);
    ASSERT_EQ(parser->state, EJSON_FINISHED_STATE);

    pcejson_reset(parser, 20, 2);
    ASSERT_EQ(parser->state, EJSON_INIT_STATE);
    ASSERT_EQ(parser->max_depth, 20);
    ASSERT_EQ(parser->flags, 2);

    pcejson_destroy(parser);
}

TEST(ejson_token, create_destroy)
{
    struct pcejson_token* token = pcejson_token_new(EJSON_TOKEN_START_OBJECT, 
            NULL, 0);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_EQ(token->sz_ptr[0], 0);
    ASSERT_EQ(token->sz_ptr[1], 0);

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
#endif

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

#if 0
TEST(ejson_token, parse_unquoted_key_AND_single_quoted_value)
{
    char json[] = "{ key : \'value\' }";

    purc_rwstream_t rws = purc_rwstream_new_from_mem(json, strlen(json));
    struct pcejson* parser = pcejson_create(10, 1);

    struct pcejson_token* token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ((char*)token->sz_ptr[1], "value");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
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
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ((char*)token->sz_ptr[1], "value");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
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
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ((char*)token->sz_ptr[1], "value");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
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
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ((char*)token->sz_ptr[1], "value");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
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
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ((char*)token->sz_ptr[1], "value");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
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
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ((char*)token->sz_ptr[1], "value");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
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
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ((char*)token->sz_ptr[1], "value");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
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
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ((char*)token->sz_ptr[1], "value");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
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
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ((char*)token->sz_ptr[1], "value");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
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
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ((char*)token->sz_ptr[1], "value");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
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
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ((char*)token->sz_ptr[1], "value");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
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
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ((char*)token->sz_ptr[1], "value");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
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
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_BOOLEAN);
    ASSERT_EQ(token->b, true);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_COMMA);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key2");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_BOOLEAN);
    ASSERT_EQ(token->b, false);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
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
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_ARRAY);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ((char*)token->sz_ptr[1], "a");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_COMMA);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ((char*)token->sz_ptr[1], "b");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_ARRAY);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
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
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "a");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ((char*)token->sz_ptr[1], "b");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_COMMA);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key2");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ((char*)token->sz_ptr[1], "v2");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
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
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_ARRAY);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "a");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ((char*)token->sz_ptr[1], "b");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_COMMA);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key2");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ((char*)token->sz_ptr[1], "v2");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_ARRAY);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
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
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_NUMBER);
    ASSERT_EQ(token->d, 123);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
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
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_NUMBER);
    ASSERT_EQ(token->d, -123);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
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
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_NUMBER);
    ASSERT_EQ(token->d, 1.23);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
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
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_NUMBER);
    ASSERT_EQ(token->d, 1.2e3);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);
    pcejson_token_destroy(token);
    purc_rwstream_destroy(rws);

    // reset
    pcejson_reset(parser, 10, 1);
    strcpy(json, "{key:1.211111e-3}");
    rws = purc_rwstream_new_from_mem(json, strlen(json));

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_NUMBER);
    ASSERT_EQ(token->d, 1.211111e-3);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
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
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_NUMBER);
    ASSERT_EQ(token->d, 1.23);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_COMMA);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
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
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_ARRAY);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ((char*)token->sz_ptr[1], "a");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_COMMA);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ((char*)token->sz_ptr[1], "b");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_COMMA);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_ARRAY);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
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
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_LONG_INT);
    ASSERT_EQ(token->i64, 123456789);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_COMMA);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
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
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_ULONG_INT);
    ASSERT_EQ(token->u64, 123456789);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
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
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_LONG_DOUBLE);
    ASSERT_DOUBLE_EQ(token->ld, 1.23456789);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
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
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_LONG_DOUBLE);
    ASSERT_EQ(token->ld, 123456789);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
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
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_BYTE_SQUENCE);
    ASSERT_EQ((int)token->sz_ptr[0], 5);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_COMMA);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);
    pcejson_token_destroy(token);

    purc_rwstream_destroy(rws);

    // reset
    pcejson_reset(parser, 10, 1);
    strcpy(json, "{key:bb1100.1100}");
    rws = purc_rwstream_new_from_mem(json, strlen(json));

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_BYTE_SQUENCE);
    ASSERT_EQ((int)token->sz_ptr[0], 1);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
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
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_BYTE_SQUENCE);
    ASSERT_STREQ((char*)token->sz_ptr[1],
        "PurC is an HVML parser and interpreter.\n ");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
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
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_TEXT);
    ASSERT_STREQ((char*)token->sz_ptr[1], text);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
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
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "k_e-y9");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ((char*)token->sz_ptr[1], "v");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
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
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);

    purc_rwstream_destroy(rws);
    pcejson_destroy(parser);
}

TEST(ejson_token, parse_escape)
{
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hybridos.test",
            "ejson", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    char json[1024] = "{key:'abc\"'}";

    purc_rwstream_t rws = purc_rwstream_new_from_mem(json, strlen(json));
    struct pcejson* parser = pcejson_create(10, 1);

    struct pcejson_token* token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ((char*)token->sz_ptr[1], "abc\"");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
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
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key");
    pcejson_token_destroy(token);


    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ((char*)token->sz_ptr[1], "v'");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
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
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key");
    pcejson_token_destroy(token);


    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ((char*)token->sz_ptr[1], "b\"");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
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
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_STRING);
    ASSERT_STREQ((char*)token->sz_ptr[1], "c\\b/\\f\\n\\r\\t\\uabcd");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    ASSERT_STREQ((char*)token->sz_ptr[1], nullptr);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);

    purc_rwstream_destroy(rws);

    pcejson_destroy(parser);
    purc_cleanup ();
}

TEST(ejson_token, pcejson_parse)
{
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hybridos.test",
            "ejson", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    char json[] = "{key:[{\"a\":\"b\"},{key2:'v2'}]}";
    purc_rwstream_t rws = purc_rwstream_new_from_mem(json, strlen(json));

    struct pcvcm_node* root = NULL;
    struct pcejson* parser = NULL;
    pcejson_parse (&root, &parser, rws, 0);
    ASSERT_NE (root, nullptr);

    purc_variant_t vt = pcvcm_eval (root, NULL);
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

    pcvcm_node_destroy (root);

    pcejson_destroy(parser);
    purc_cleanup ();
}

TEST(ejson_token, string_variant)
{
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hybridos.test",
            "ejson", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    char key[] = "name";

    purc_variant_t key_vt = purc_variant_make_string (key, false);
    purc_variant_t value_vt = purc_variant_make_string ("tom", false);

    purc_variant_t object = purc_variant_make_object (0,
                         PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);

    purc_variant_object_set (object, key_vt, value_vt);

    purc_variant_unref(key_vt);
    purc_variant_unref(value_vt);

    char buf[1024];
    purc_rwstream_t my_rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(my_rws, nullptr);

    size_t len_expected = 0;
    ssize_t n = purc_variant_serialize(object, my_rws,
            0, PCVARIANT_SERIALIZE_OPT_PLAIN, &len_expected);
    ASSERT_GT(n, 0);
    buf[n] = 0;
    ASSERT_STREQ(buf, "{\"name\":\"tom\"}");

    purc_variant_unref(object);
    purc_rwstream_destroy(my_rws);

    purc_cleanup ();
}

TEST(ejson_token, pcejson_parse_longstring)
{
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hybridos.test",
            "ejson", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    char json[] = "{key:[{\"a\":\"b\"},{key2:'abcdefghijklmnopq'}]}";
    purc_rwstream_t rws = purc_rwstream_new_from_mem(json, strlen(json));

    struct pcvcm_node* root = NULL;
    struct pcejson* parser = NULL;
    pcejson_parse (&root, &parser, rws, 0);
    ASSERT_NE (root, nullptr);

    purc_variant_t vt = pcvcm_eval (root, NULL);
    ASSERT_NE(vt, PURC_VARIANT_INVALID);

    char buf[1024];
    purc_rwstream_t my_rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(my_rws, nullptr);

    size_t len_expected = 0;
    ssize_t n = purc_variant_serialize(vt, my_rws,
            0, PCVARIANT_SERIALIZE_OPT_PLAIN, &len_expected);
    ASSERT_GT(n, 0);
    buf[n] = 0;
    ASSERT_STREQ(buf, "{\"key\":[{\"a\":\"b\"},{\"key2\":\"abcdefghijklmnopq\"}]}");

    purc_variant_unref(vt);
    purc_rwstream_destroy(my_rws);
    purc_rwstream_destroy(rws);

    pcvcm_node_destroy (root);

    pcejson_destroy(parser);
    purc_cleanup ();
}

purc_variant_t make_object(const char* key, const char* value)
{
    purc_variant_t key_vt = purc_variant_make_string (key, false);
    purc_variant_t value_vt = purc_variant_make_string (value, false);

    purc_variant_t object = purc_variant_make_object (0,
                         PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);

    purc_variant_object_set (object, key_vt, value_vt);

    purc_variant_unref(key_vt);
    purc_variant_unref(value_vt);
    return object;
}

TEST(ejson_token, string_func_variant)
{
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hybridos.test",
            "ejson", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t object = purc_variant_make_object (0,
                         PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);

    purc_variant_t array = purc_variant_make_array (0, PURC_VARIANT_INVALID);

    purc_variant_t element = make_object("elementKey1", "elementV1");
    purc_variant_t element2 = make_object("elementKey2", "elementV2");

    purc_variant_array_append (array, element);
    purc_variant_array_append (array, element2);


    purc_variant_t key = purc_variant_make_string ("keyX", false);
    purc_variant_object_set (object, key, array);

    char buf[1024];
    purc_rwstream_t my_rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(my_rws, nullptr);

    size_t len_expected = 0;
    ssize_t n = purc_variant_serialize(object, my_rws,
            0, PCVARIANT_SERIALIZE_OPT_PLAIN, &len_expected);
    ASSERT_GT(n, 0);
    buf[n] = 0;
    ASSERT_STREQ(buf, "{\"keyX\":[{\"elementKey1\":\"elementV1\"},{\"elementKey2\":\"elementV2\"}]}");

    purc_variant_unref (key);
    purc_variant_unref (element);
    purc_variant_unref (element2);
    purc_variant_unref (array);
    purc_variant_unref(object);
    purc_rwstream_destroy(my_rws);

    purc_cleanup ();
}

TEST(ejson_token, pcejson_parse_segment)
{
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hybridos.test",
            "ejson", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    char json[] = "{key:[{\"a\":\"b\"}";
    char json2[] = ",{key2:'v2'}]}";
    purc_rwstream_t rws = purc_rwstream_new_from_mem(json, strlen(json));
    purc_rwstream_t rws2 = purc_rwstream_new_from_mem(json2, strlen(json2));

    struct pcvcm_node* root = NULL;
    struct pcejson* parser = NULL;
    pcejson_parse (&root, &parser, rws, 0);
    ASSERT_NE (root, nullptr);

    pcejson_parse (&root, &parser, rws2, 32);
    ASSERT_NE (root, nullptr);

    purc_variant_t vt = pcvcm_eval (root, NULL);
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
    purc_rwstream_destroy(rws2);

    pcvcm_node_destroy (root);

    pcejson_destroy(parser);
    purc_cleanup ();
}

TEST(ejson_token, pcejson_infinity)
{
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hybridos.test",
            "ejson", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    char json[1024] = "{key:Infinity}";
    purc_rwstream_t rws = purc_rwstream_new_from_mem(json, strlen(json));

    struct pcejson* parser = pcejson_create(10, 1);

    struct pcejson_token* token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_INFINITY);
    ASSERT_DOUBLE_EQ(token->d, INFINITY);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    pcejson_token_destroy(token);

    purc_rwstream_destroy(rws);

    // reset
    pcejson_reset(parser, 10, 1);
    strcpy(json, "{key:-Infinity}");
    rws = purc_rwstream_new_from_mem(json, strlen(json));

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_INFINITY);
    ASSERT_DOUBLE_EQ(token->d, -INFINITY);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    pcejson_token_destroy(token);

    purc_rwstream_destroy(rws);

    pcejson_destroy(parser);
    purc_cleanup ();
}

TEST(ejson_token, pcejson_nan)
{
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hybridos.test",
            "ejson", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    char json[1024] = "{key:NaN}";
    purc_rwstream_t rws = purc_rwstream_new_from_mem(json, strlen(json));

    struct pcejson* parser = pcejson_create(10, 1);

    struct pcejson_token* token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_NAN);
    ASSERT_EQ(isnan(token->d), true);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    pcejson_token_destroy(token);

    purc_rwstream_destroy(rws);

    // reset
    pcejson_reset(parser, 10, 1);
    strcpy(json, "{key:-NaN}");
    rws = purc_rwstream_new_from_mem(json, strlen(json));

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_KEY);
    ASSERT_STREQ((char*)token->sz_ptr[1], "key");
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_EQ(token, nullptr);

    purc_rwstream_destroy(rws);

    pcejson_destroy(parser);
    purc_cleanup ();
}

TEST(ejson_token, pcejson_parse_infinity_nan)
{
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hybridos.test",
            "ejson", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    char json[] = "{key:[{\"a\":NaN},{key2:Infinity},{key3:-Infinity}]}";
    purc_rwstream_t rws = purc_rwstream_new_from_mem(json, strlen(json));

    struct pcvcm_node* root = NULL;
    struct pcejson* parser = NULL;
    pcejson_parse (&root, &parser, rws, 0);
    ASSERT_NE (root, nullptr);

    purc_variant_t vt = pcvcm_eval (root, NULL);
    ASSERT_NE(vt, PURC_VARIANT_INVALID);

    char buf[1024];
    purc_rwstream_t my_rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(my_rws, nullptr);

    size_t len_expected = 0;
    ssize_t n = purc_variant_serialize(vt, my_rws,
            0, PCVARIANT_SERIALIZE_OPT_PLAIN, &len_expected);
    ASSERT_GT(n, 0);
    buf[n] = 0;
    ASSERT_STREQ(buf,
            "{\"key\":[{\"a\":NaN},{\"key2\":Infinity},{\"key3\":-Infinity}]}");

    purc_variant_unref(vt);
    purc_rwstream_destroy(my_rws);
    purc_rwstream_destroy(rws);

    pcvcm_node_destroy (root);

    pcejson_destroy(parser);
    purc_cleanup ();
}

TEST(ejson_token, pcejson_parse_array)
{
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hybridos.test",
            "ejson", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    char json[] = "[123.456e-789]";
    purc_rwstream_t rws = purc_rwstream_new_from_mem(json, strlen(json));

    struct pcvcm_node* root = NULL;
    struct pcejson* parser = NULL;
    pcejson_parse (&root, &parser, rws, 0);
    ASSERT_NE (root, nullptr);

    purc_variant_t vt = pcvcm_eval (root, NULL);
    ASSERT_NE(vt, PURC_VARIANT_INVALID);

    char buf[1024];
    purc_rwstream_t my_rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(my_rws, nullptr);

    size_t len_expected = 0;
    ssize_t n = purc_variant_serialize(vt, my_rws,
            0, PCVARIANT_SERIALIZE_OPT_PLAIN, &len_expected);
    ASSERT_GT(n, 0);
    buf[n] = 0;
    ASSERT_STREQ(buf, "[0]");

    purc_variant_unref(vt);
    purc_rwstream_destroy(my_rws);
    purc_rwstream_destroy(rws);

    pcvcm_node_destroy (root);

    pcejson_destroy(parser);
    purc_cleanup ();
}

TEST(ejson_token, pcejson_parse_empty_object)
{
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hybridos.test",
            "ejson", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    char json[1024] = "{}";
    purc_rwstream_t rws = purc_rwstream_new_from_mem(json, strlen(json));

    struct pcejson* parser = pcejson_create(10, 1);

    struct pcejson_token* token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_START_OBJECT);
    pcejson_token_destroy(token);

    token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, EJSON_TOKEN_END_OBJECT);
    pcejson_token_destroy(token);

    purc_rwstream_destroy(rws);

    pcejson_destroy(parser);
    purc_cleanup ();
}

TEST(ejson_token, pcejson_parse_serial_empty_object)
{
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hybridos.test",
            "ejson", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    char json[] = "{}";
    purc_rwstream_t rws = purc_rwstream_new_from_mem(json, strlen(json));

    struct pcvcm_node* root = NULL;
    struct pcejson* parser = NULL;
    pcejson_parse (&root, &parser, rws, 0);
    ASSERT_NE (root, nullptr);

    purc_variant_t vt = pcvcm_eval (root, NULL);
    ASSERT_NE(vt, PURC_VARIANT_INVALID);

    char buf[1024];
    purc_rwstream_t my_rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(my_rws, nullptr);

    size_t len_expected = 0;
    ssize_t n = purc_variant_serialize(vt, my_rws,
            0, PCVARIANT_SERIALIZE_OPT_PLAIN, &len_expected);
    ASSERT_GT(n, 0);
    buf[n] = 0;
    ASSERT_STREQ(buf, "{}");

    purc_variant_unref(vt);
    purc_rwstream_destroy(my_rws);
    purc_rwstream_destroy(rws);

    pcvcm_node_destroy (root);

    pcejson_destroy(parser);
    purc_cleanup ();

}
#endif

