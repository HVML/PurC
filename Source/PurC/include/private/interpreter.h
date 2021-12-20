/**
 * @file interpreter.h
 * @author Xu Xiaohong
 * @date 2021/11/18
 * @brief The internal interfaces for interpreter
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
 *
 */

#ifndef PURC_PRIVATE_INTERPRETER_H
#define PURC_PRIVATE_INTERPRETER_H

#include "purc.h"

#include "purc-macros.h"
#include "purc-variant.h"

#include "private/debug.h"
#include "private/errors.h"
#include "private/list.h"
#include "private/vdom.h"

struct pcintr_coroutine;
typedef struct pcintr_coroutine pcintr_coroutine;
typedef struct pcintr_coroutine *pcintr_coroutine_t;

struct pcintr_heap {
    struct list_head      coroutines;
    pcintr_coroutine_t    running_coroutine;
};

struct pcintr_stack;
typedef struct pcintr_stack pcintr_stack;
typedef struct pcintr_stack *pcintr_stack_t;

struct pcintr_stack_frame;
typedef struct pcintr_stack_frame pcintr_stack_frame;
typedef struct pcintr_stack_frame *pcintr_stack_frame_t;

struct pcintr_dynamic_args {
    const char                    *name;
    purc_dvariant_method           getter;
    purc_dvariant_method           setter;
};

struct pcinst;

typedef void* pcintr_timer_t;
typedef void (*pcintr_timer_fire_func)(const char* id, void* ctxt);

PCA_EXTERN_C_BEGIN

void pcintr_stack_init_once(void) WTF_INTERNAL;
void pcintr_stack_init_instance(struct pcinst* inst) WTF_INTERNAL;
void pcintr_stack_cleanup_instance(struct pcinst* inst) WTF_INTERNAL;
pcintr_stack_t purc_get_stack (void);
struct pcintr_stack_frame*
pcintr_stack_get_bottom_frame(pcintr_stack_t stack);
struct pcintr_stack_frame*
pcintr_stack_frame_get_parent(struct pcintr_stack_frame *frame);
void
pop_stack_frame(pcintr_stack_t stack);
struct pcintr_stack_frame*
push_stack_frame(pcintr_stack_t stack);

struct pcintr_element_ops*
pcintr_get_element_ops(pcvdom_element_t element);

purc_variant_t
pcintr_make_object_of_dynamic_variants(size_t nr_args,
    struct pcintr_dynamic_args *args);

bool
pcintr_bind_buildin_variable(struct pcvdom_document* doc, const char* name,
        purc_variant_t variant);

bool
pcintr_unbind_buildin_variable(struct pcvdom_document* doc,
        const char* name);

bool
pcintr_bind_scope_variable(pcvdom_element_t elem, const char* name,
        purc_variant_t variant);

bool
pcintr_unbind_scope_variable(pcvdom_element_t elem, const char* name);

purc_variant_t
pcintr_find_named_var(pcintr_stack_t stack, const char* name);

purc_variant_t
pcintr_get_symbolized_var (pcintr_stack_t stack, unsigned int number,
        char symbol);

purc_variant_t
pcintr_get_numbered_var (pcintr_stack_t stack, unsigned int number);

pcintr_timer_t
pcintr_timer_create(const char* id, void* ctxt, pcintr_timer_fire_func func);

void
pcintr_timer_set_interval(pcintr_timer_t timer, uint32_t interval);

uint32_t
pcintr_timer_get_interval(pcintr_timer_t timer);

void
pcintr_timer_start(pcintr_timer_t timer);

void
pcintr_timer_start_oneshot(pcintr_timer_t timer);

void
pcintr_timer_stop(pcintr_timer_t timer);

void
pcintr_timer_destroy(pcintr_timer_t timer);

PCA_EXTERN_C_END

#endif  /* PURC_PRIVATE_INTERPRETER_H */

