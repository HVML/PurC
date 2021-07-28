/*
 * @file stack.h
 * @author XueShuming
 * @date 2021/07/28
 * @brief The interfaces for N-ary trees.
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

#ifndef PURC_PRIVATE_STACK_H
#define PURC_PRIVATE_STACK_H

#include <stddef.h>
#include <stdint.h>

struct pcutils_stack {
    uintptr_t* buf;
    uint32_t capacity;
    int32_t last;
};

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * Create a new stack.
 */
struct pcutils_stack* pcutils_stack_new (size_t sz_init);

/**
 * Check if the stack is empty.
 */
bool pcutils_stack_is_empty (struct pcutils_stack* stack);

/**
 * Get the size of the stack.
 */
size_t pcutils_stack_size (struct pcutils_stack* stack);

/**
 * Push a element to the stack.
 */
void pcutils_stack_push (struct pcutils_stack* stack, uintptr_t e);

/**
 * Pop a element from the stack.
 */
uintptr_t pcutils_stack_pop (struct pcutils_stack* stack);

/**
 * Get the first character of the stack.
 */
uintptr_t pcutils_stack_first (struct pcutils_stack* stack);

/**
 * Get the last character of the stack.
 */
uintptr_t pcutils_stack_last (struct pcutils_stack* stack);

/**
 * Destory stack.
 */
void pcutils_stack_destroy (struct pcutils_stack* stack);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* not defined PURC_PRIVATE_STACK_H */

