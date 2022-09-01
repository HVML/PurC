/*
 * @file stack.c
 * @author XueShuming
 * @date 2021/07/28
 * @brief The API for stack.
 *
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of PurC (short for Purring Cat), an HVML interpreter.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "config.h"
#include "purc-utils.h"
#include "purc-errors.h"
#include "private/errors.h"
#include "private/stack.h"

#define MIN_STACK_CAPACITY 32

static size_t get_stack_size (size_t sz_stack)
{
    size_t stack = pcutils_get_next_fibonacci_number(sz_stack);
    return stack < MIN_STACK_CAPACITY ? MIN_STACK_CAPACITY : stack;
}

struct pcutils_stack* pcutils_stack_new (size_t sz_init)
{
    struct pcutils_stack* stack = (struct pcutils_stack*) calloc(
            1, sizeof(struct pcutils_stack));
    sz_init = get_stack_size(sz_init);
    stack->buf = (uintptr_t*) calloc (sz_init, sizeof(uintptr_t));
    stack->last = -1;
    stack->capacity = sz_init;
    return stack;
}

bool pcutils_stack_is_empty (struct pcutils_stack* stack)
{
    return stack->last == -1;
}

size_t pcutils_stack_size (struct pcutils_stack* stack)
{
    return stack->last + 1;
}

void pcutils_stack_push (struct pcutils_stack* stack, uintptr_t p)
{
    if (stack->last == (int32_t)(stack->capacity - 1))
    {
        size_t sz = get_stack_size(stack->capacity);
        stack->buf = (uintptr_t*) realloc(stack->buf, sz * sizeof(uintptr_t));
        if (stack->buf == NULL)
        {
            pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
            return;
        }
        stack->capacity = sz;
    }
    stack->buf[++stack->last] = p;
}

uintptr_t pcutils_stack_pop (struct pcutils_stack* stack)
{
    if (pcutils_stack_is_empty(stack))
    {
        return 0;
    }
    return stack->buf[stack->last--];
}

uintptr_t pcutils_stack_bottom (struct pcutils_stack* stack)
{
    if (pcutils_stack_is_empty(stack))
    {
        return 0;
    }
    return stack->buf[0];
}

uintptr_t pcutils_stack_top (struct pcutils_stack* stack)
{
    if (pcutils_stack_is_empty(stack)) {
        return 0;
    }
    return stack->buf[stack->last];
}

void pcutils_stack_clear (struct pcutils_stack* stack)
{
    if (pcutils_stack_is_empty(stack)) {
        return;
    }
    stack->last = -1;
}

void pcutils_stack_destroy (struct pcutils_stack* stack)
{
    if (stack) {
        free(stack->buf);
        stack->buf = NULL;
        stack->last = -1;
        stack->capacity = 0;
        free(stack);
    }
}

uintptr_t pcutils_stack_get (struct pcutils_stack* stack, int idx)
{
    if (idx < 0 || idx > stack->last) {
        return 0;
    }
    return stack->buf[idx];
}

