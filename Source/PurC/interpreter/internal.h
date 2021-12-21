/**
 * @file internal.h
 * @author Xu Xiaohong
 * @date 2021/12/18
 * @brief The internal interfaces for interpreter/internal
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

#ifndef PURC_INTERPRETER_INTERNAL_H
#define PURC_INTERPRETER_INTERNAL_H

#include "purc-macros.h"

#include "private/interpreter.h"

enum pcintr_stack_state {
    STACK_STATE_WAITING    = 0x01,
    STACK_STATE_TERMINATED = 0x02,
    /* STACK_STATE_PAUSED     = 0x10, */
};

#define pcintr_stack_is_waiting(stack)     \
    (stack->state & STACK_STATE_WAITING)
#define pcintr_stack_is_terminated(stack)  \
    (stack->state & STACK_STATE_TERMINATED)
#define pcintr_stack_is_paused(stack)      \
    (stack->state & STACK_STATE_PAUSED)

enum pcintr_coroutine_state {
    CO_STATE_READY,            /* ready to run next step */
    CO_STATE_RUN,              /* is running */
    CO_STATE_WAIT,             /* is waiting for event */
    CO_STATE_TERMINATED,       /* can never execute any hvml code */
    /* STATE_PAUSED, */
};

struct pcintr_coroutine {
    struct list_head            node;   /* sibling coroutines */

    struct pcintr_stack        *stack;  /* stack that holds this coroutine */

    enum pcintr_coroutine_state state;
    int                         waits;  /* FIXME: nr of registered events */
};

struct pcintr_stack {
    struct list_head frames;

    // the number of stack frames.
    size_t nr_frames;

    // the pointer to the vDOM tree.
    purc_vdom_t vdom;

    // the returned variant
    purc_variant_t ret_var;

    // executing state
    // FIXME: move to struct pcintr_coroutine?
    uint32_t        error:1;
    uint32_t        except:1;
    /* uint32_t        paused:1; */

    uint32_t        state; /* TODO: remove */

    // error or except info
    purc_atom_t     error_except;
    purc_variant_t  err_except_info;

    // executing statistics
    struct timespec time_executed;
    struct timespec time_idle;
    size_t          peak_mem_use;
    size_t          peak_nr_variants;

    /* coroutine that this stack `owns` */
    /* FIXME: switch owner-ship ? */
    struct pcintr_coroutine        co; 
};

enum purc_symbol_var {
    PURC_SYMBOL_VAR_QUESTION_MARK = 0,  // ?
    PURC_SYMBOL_VAR_AT_SIGN,            // @
    PURC_SYMBOL_VAR_NUMBER_SIGN,        // #
    PURC_SYMBOL_VAR_ASTERISK,           // *
    PURC_SYMBOL_VAR_COLON,              // :
    PURC_SYMBOL_VAR_AMPERSAND,          // &
    PURC_SYMBOL_VAR_PERCENT_SIGN,       // %

    PURC_SYMBOL_VAR_MAX
};

struct pcintr_element_ops {
    // FIXME:
    // all function returns nothing
    // let's take `after_pushed` as an example to explain:
    // specifically, during after_pushed call, the coroutine might yield
    // it's execution(eg.: <init ... from=<url> .../>),
    // thus caller can not simply rely on return'd status to determine
    // what next step it shall take. as a result, return'd value means nothing
    // to caller
    // NOTE: because all functions returns nothing, these functions shall
    // set coroutine's next step correctly when returning.
    // eg.: ref. Source/PurC/interpreter/undefined.c
    void (*after_pushed) (pcintr_coroutine_t co,
            struct pcintr_stack_frame *frame);

    // called on popping
    void (*on_popping) (pcintr_coroutine_t co,
            struct pcintr_stack_frame *frame);

    // called to rerun
    void (*rerun) (pcintr_coroutine_t co,
            struct pcintr_stack_frame *frame);

    // called after executed
    void (*select_child) (pcintr_coroutine_t co,
            struct pcintr_stack_frame *frame);
};

enum pcintr_stack_frame_next_step {
    NEXT_STEP_AFTER_PUSHED = 0,
    NEXT_STEP_ON_POPPING,
    NEXT_STEP_RERUN,
    NEXT_STEP_SELECT_CHILD,
};

typedef void (*preemptor_f) (pcintr_coroutine_t co,
            struct pcintr_stack_frame *frame);

struct pcintr_stack_frame {
    // pointers to sibling frames.
    struct list_head node;

    // the current scope.
    pcvdom_element_t scope;

    // the current execution position.
    pcvdom_element_t pos;

    // the symbolized variables
    purc_variant_t symbol_vars[PURC_SYMBOL_VAR_MAX];

    // all attribute variants are managed by a map (attribute name -> variant).
    purc_variant_t attr_vars;

    // the evaluated content variant
    purc_variant_t ctnt_var;

    // all intermediate variants are managed by an array.
    purc_variant_t mid_vars;

    struct pcintr_element_ops ops;

    // context for current action
    void *ctxt;
    int   next_step;

    preemptor_f        preemptor;
};

PCA_EXTERN_C_BEGIN

int
pcintr_element_eval_attrs(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element);

int
pcintr_element_eval_vcm_content(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element);


void pcintr_coroutine_ready(void);

PCA_EXTERN_C_END

#endif  /* PURC_INTERPRETER_INTERNAL_H */



