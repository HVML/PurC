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

#include "purc.h"

#include "private/ejson.h"
#include "purc-rwstream.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>

TEST(stack, new_destory)
{
    pcutils_stack* stack = pcutils_stack_new(10);
    ASSERT_NE(stack, nullptr);
    ASSERT_EQ(stack->capacity, 32);
    ASSERT_EQ(stack->last, -1);

    pcutils_stack_destroy(stack);
}

TEST(stack, push_pop)
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

