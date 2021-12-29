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
#include "private/html.h"
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

enum pcintr_stack_stage {
    STACK_STAGE_FIRST_ROUND,
    STACK_STAGE_EVENT_LOOP,
    STACK_STAGE_TERMINATING,
};

struct pcintr_edom_gen {
    pchtml_html_document_t     *doc;
    pchtml_html_parser_t       *parser;
    purc_rwstream_t             cache;
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

    enum pcintr_stack_stage       stage;

    // error or except info
    purc_atom_t     error_except;
    purc_variant_t  err_except_info;
    const char     *file;
    int             lineno;
    const char     *func;

    // executing statistics
    struct timespec time_executed;
    struct timespec time_idle;
    size_t          peak_mem_use;
    size_t          peak_nr_variants;

    /* coroutine that this stack `owns` */
    /* FIXME: switch owner-ship ? */
    struct pcintr_coroutine        co;

    // for observe
    struct pcutils_arrlist* common_variant_observer_list;
    struct pcutils_arrlist* dynamic_variant_observer_list;
    struct pcutils_arrlist* native_variant_observer_list;

    // streamlizing to generate edom in one pass
    struct pcintr_edom_gen     edom_gen;

    // collect fragments chunk by chunk by `update` during `iterate`
    // temporary, temporary, temporary
    // iterate[after_pushed]: make fragment or reset fragment
    // update: write content into fragment, CURRENT implementation
    //         concerning prepend operation, better use string-buffers instead
    // iterate[on_popping(true)]: write fragment into `edom_gen`
    // TODO: this is temporary implementation!!!!
    // ultimate: recording action as well as content, and `replay` to the
    //           right position in edom when done!
    purc_rwstream_t            fragment;
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
    // called after pushed
    void *(*after_pushed) (pcintr_stack_t stack, pcvdom_element_t pos);

    // called on popping
    bool (*on_popping) (pcintr_stack_t stack, void* ctxt);

    // called to rerun
    bool (*rerun) (pcintr_stack_t stack, void* ctxt);

    // select a child
    pcvdom_element_t (*select_child) (pcintr_stack_t stack, void* ctxt);
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
    // managed by element-implementer
    void *ctxt;
    void (*ctxt_destroy)(void *);

    // managed by coroutine-coordinator
    enum pcintr_stack_frame_next_step next_step;

    // coordinated between element-implementer and coroutine-coordinator
    preemptor_f        preemptor;
};

struct pcintr_dynamic_args {
    const char                    *name;
    purc_dvariant_method           getter;
    purc_dvariant_method           setter;
};

struct pcintr_observer {
    // the observed variant.
    purc_variant_t observed;

    // the type of the message observed (cloned from the `for` attribute)
    char* msg_type;

    // the sub type of the message observed (cloned from the `for` attribute; nullable).
    char* sub_type;

    pcvdom_element_t scope;

    // the `observe` element who creates this observer.
    pcvdom_element_t pos;

    pcvdom_element_t child;

    // the arraylist containing this struct pointer
    struct pcutils_arrlist* list;
};

struct pcinst;

struct pcintr_timers;

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
pcintr_pop_stack_frame(pcintr_stack_t stack);
struct pcintr_stack_frame*
pcintr_push_stack_frame(pcintr_stack_t stack);

void
pcintr_stack_write_fragment(pcintr_stack_t stack);

__attribute__ ((format (printf, 2, 3)))
void
pcintr_printf_to_fragment(pcintr_stack_t stack, const char *fmt, ...);

__attribute__ ((format (printf, 2, 3)))
int
pcintr_printf_to_edom(pcintr_stack_t stack, const char *fmt, ...);

int
pcintr_printf_start_element_to_edom(pcintr_stack_t stack);

int
pcintr_printf_end_element_to_edom(pcintr_stack_t stack);

purc_variant_t
pcintr_make_object_of_dynamic_variants(size_t nr_args,
    struct pcintr_dynamic_args *args);

static inline bool
pcintr_bind_document_variable(purc_vdom_t vdom, const char* name,
        purc_variant_t variant)
{
    return pcvdom_document_bind_variable(vdom, name, variant);
}

static inline bool
pcintr_unbind_document_variable(purc_vdom_t vdom, const char* name)
{
    return pcvdom_document_unbind_variable(vdom, name);
}

static inline purc_variant_t
pcintr_get_document_variable(purc_vdom_t vdom, const char* name)
{
    return pcvdom_document_get_variable(vdom, name);
}

static inline bool
pcintr_bind_scope_variable(pcvdom_element_t elem, const char* name,
        purc_variant_t variant)
{
    return pcvdom_element_bind_variable(elem, name, variant);
}

static inline bool
pcintr_unbind_scope_variable(pcvdom_element_t elem, const char* name)
{
    return pcvdom_element_unbind_variable(elem, name);
}

static inline purc_variant_t
pcintr_get_scope_variable(pcvdom_element_t elem, const char* name)
{
    return pcvdom_element_get_variable(elem, name);
}

purc_variant_t
pcintr_find_named_var(pcintr_stack_t stack, const char* name);

purc_variant_t
pcintr_get_symbolized_var (pcintr_stack_t stack, unsigned int number,
        char symbol);

purc_variant_t
pcintr_get_numbered_var (pcintr_stack_t stack, unsigned int number);

// $TIMERS
struct pcintr_timers*
pcintr_timers_init(pcintr_stack_t stack);

void
pcintr_timers_destroy(struct pcintr_timers* timers);

struct pcintr_observer*
pcintr_register_observer(purc_variant_t observed,
        purc_variant_t for_value, pcvdom_element_t scope,
        pcvdom_element_t pos, pcvdom_element_t child);

bool
pcintr_revoke_observer(struct pcintr_observer* observer);

struct pcintr_observer*
pcintr_find_observer(pcintr_stack_t stack, purc_variant_t observed,
        purc_variant_t msg_type, purc_variant_t sub_type);

bool
pcintr_is_observer_empty(pcintr_stack_t stack);

void
pcintr_dispatch_message(pcintr_stack_t stack, purc_variant_t source,
        purc_variant_t type, purc_variant_t sub_type, purc_variant_t extra);

PCA_EXTERN_C_END

#endif  /* PURC_PRIVATE_INTERPRETER_H */

