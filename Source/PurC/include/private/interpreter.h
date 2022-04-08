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

#include "config.h"

#include "purc-macros.h"
#include "purc-variant.h"

#include "private/debug.h"
#include "private/errors.h"
#include "private/html.h"
#include "private/list.h"
#include "private/vdom.h"
#include "private/timer.h"

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
    STACK_STAGE_FIRST_ROUND                = 0x00,
    STACK_STAGE_EVENT_LOOP                 = 0x01,
    STACK_STAGE_TERMINATING                = 0x02,
};

struct pcintr_loaded_var {
    struct rb_node              node;
    char                       *name;
    purc_variant_t              val;
};

enum pcintr_stack_vdom_insertion_mode {
    STACK_VDOM_BEFORE_HVML,
    STACK_VDOM_BEFORE_HEAD,
    STACK_VDOM_IN_HEAD,
    STACK_VDOM_AFTER_HEAD,
    STACK_VDOM_IN_BODY,
    STACK_VDOM_AFTER_BODY,
    STACK_VDOM_AFTER_HVML,
};

// experimental: currently for test-case-only
struct pcintr_supervisor_ops {
    void (*on_terminated)(pcintr_stack_t stack, void *ctxt);
    void (*on_cleanup)(pcintr_stack_t stack, void *ctxt);
};

struct pcintr_exception {
    int                      errcode;
    purc_atom_t              error_except;
    purc_variant_t           exinfo;

    struct pcdebug_backtrace  *bt;
};

void
pcintr_exception_clear(struct pcintr_exception *exception);

void
pcintr_exception_move(struct pcintr_exception *dst,
        struct pcintr_exception *src);

struct pcintr_stack {
    struct list_head frames;

    // the number of stack frames.
    size_t nr_frames;

    // the pointer to the vDOM tree.
    purc_vdom_t vdom;

    enum pcintr_stack_vdom_insertion_mode        mode;

    // the returned variant
    purc_variant_t ret_var;

    // executing state
    // FIXME: move to struct pcintr_coroutine?
    // uint32_t        error:1;
    uint32_t        except:1;
    /* uint32_t        paused:1; */

    enum pcintr_stack_stage       stage;

    // error or except info
    // valid only when except == 1
    struct pcintr_exception       exception;

    // for `back` to use
    struct pcintr_stack_frame         *back_anchor;

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

    pchtml_html_document_t     *doc;

    // for loaded dynamic variants
    struct rb_root             loaded_vars;  // struct pcintr_loaded_var*

    // base uri
    char* base_uri;

    // experimental: currently for test-case-only
    struct pcintr_supervisor_ops        ops;
    void                                *ctxt;  // no-owner-ship!!!

    pcintr_timer_t                      *event_timer; // 10ms
};

enum purc_symbol_var {
    PURC_SYMBOL_VAR_QUESTION_MARK = 0,  // ?
    PURC_SYMBOL_VAR_LESS_THAN,          // <
    PURC_SYMBOL_VAR_AT_SIGN,            // @
    PURC_SYMBOL_VAR_EXCLAMATION,        // !
    PURC_SYMBOL_VAR_COLON,              // :
    PURC_SYMBOL_VAR_EQUAL,              // =
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
    // the current edom element;
    pcdom_element_t *edom_element;

    // the current execution position.
    pcvdom_element_t pos;

    // the symbolized variables for this frame, $0?/$0@/...
    purc_variant_t symbol_vars[PURC_SYMBOL_VAR_MAX];

    // all attribute variants are managed by a map (attribute name -> variant).
    purc_variant_t attr_vars;

    // the evaluated content variant
    purc_variant_t ctnt_var;

    // the evaluated variant which is to be used by parent element
    // eg.: test/match, reclusive
    purc_variant_t result_from_child;

    struct pcintr_element_ops ops;

    // context for current action
    // managed by element-implementer
    void *ctxt;
    void (*ctxt_destroy)(void *);

    // managed by coroutine-coordinator
    enum pcintr_stack_frame_next_step next_step;

    // coordinated between element-implementer and coroutine-coordinator
    preemptor_f        preemptor;

    bool silently;

    pcintr_stack_t     owner;
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
    pcdom_element_t *edom_element;

    // the `observe` element who creates this observer.
    pcvdom_element_t pos;

    // the arraylist containing this struct pointer
    struct pcutils_arrlist* list;

    // variant listener for object, set, array
    struct pcvar_listener* listener;
};

struct pcinst;

struct pcintr_timers;

PCA_EXTERN_C_BEGIN

void pcintr_stack_init_once(void) WTF_INTERNAL;
void pcintr_stack_init_instance(struct pcinst* inst) WTF_INTERNAL;
void pcintr_stack_cleanup_instance(struct pcinst* inst) WTF_INTERNAL;

pcintr_stack_t pcintr_get_stack(void);
struct pcintr_stack_frame*
pcintr_stack_get_bottom_frame(pcintr_stack_t stack);
struct pcintr_stack_frame*
pcintr_stack_frame_get_parent(struct pcintr_stack_frame *frame);

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

int
pcintr_unbind_named_var(pcintr_stack_t stack, const char *name);

// return observed variant
purc_variant_t
pcintr_get_named_var_observed(pcintr_stack_t stack, const char* name);

// return observed variant
purc_variant_t
pcintr_add_named_var_observer(pcintr_stack_t stack, const char* name,
        const char* event);

// return observed variant
purc_variant_t
pcintr_remove_named_var_observer(pcintr_stack_t stack, const char* name,
        const char* event);

// $TIMERS
struct pcintr_timers*
pcintr_timers_init(pcintr_stack_t stack);

void
pcintr_timers_destroy(struct pcintr_timers* timers);

bool
pcintr_is_timers(pcintr_stack_t stack, purc_variant_t v);

struct pcintr_observer*
pcintr_register_observer(purc_variant_t observed,
        purc_variant_t for_value, pcvdom_element_t scope,
        pcdom_element_t *edom_element,
        pcvdom_element_t pos,
        struct pcvar_listener* listener);

bool
pcintr_revoke_observer(struct pcintr_observer* observer);

bool
pcintr_revoke_observer_ex(purc_variant_t observed, purc_variant_t for_value);

struct pcintr_observer*
pcintr_find_observer(pcintr_stack_t stack, purc_variant_t observed,
        purc_variant_t msg_type, purc_variant_t sub_type);

bool
pcintr_is_observer_empty(pcintr_stack_t stack);

int
pcintr_dispatch_message(pcintr_stack_t stack, purc_variant_t source,
        purc_variant_t for_value, purc_variant_t extra);

int
pcintr_dispatch_message_ex(pcintr_stack_t stack, purc_variant_t source,
        purc_variant_t type, purc_variant_t sub_type, purc_variant_t extra);

bool
pcintr_load_dynamic_variant(pcintr_stack_t stack,
    const char *name, size_t len);

// utilities
void
pcintr_util_dump_document_ex(pchtml_html_document_t *doc,
    const char *file, int line, const char *func);

void
pcintr_util_dump_edom_node_ex(pcdom_node_t *node,
    const char *file, int line, const char *func);

#define pcintr_util_dump_document(_doc)          \
    pcintr_util_dump_document_ex(_doc, __FILE__, __LINE__, __func__)

#define pcintr_util_dump_edom_node(_node)        \
    pcintr_util_dump_edom_node_ex(_node, __FILE__, __LINE__, __func__)

#define pcintr_dump_document(_stack)             \
    pcintr_util_dump_document_ex(_stack->doc, __FILE__, __LINE__, __func__)

#define pcintr_dump_edom_node(_stack, _node)      \
    pcintr_util_dump_edom_node_ex(_node, __FILE__, __LINE__, __func__)

void
pcintr_dump_frame_edom_node(pcintr_stack_t stack);

pcdom_element_t*
pcintr_util_append_element(pcdom_element_t* parent, const char *tag);

pcdom_text_t*
pcintr_util_append_content(pcdom_element_t* parent, const char *txt);

pcdom_text_t*
pcintr_util_displace_content(pcdom_element_t* parent, const char *txt);

int
pcintr_util_set_attribute(pcdom_element_t *elem,
        const char *key, const char *val);

int
pcintr_util_add_child_chunk(pcdom_element_t *parent, const char *chunk);

int
pcintr_util_set_child_chunk(pcdom_element_t *parent, const char *chunk);

WTF_ATTRIBUTE_PRINTF(2, 3)
int
pcintr_util_add_child(pcdom_element_t *parent, const char *fmt, ...);

WTF_ATTRIBUTE_PRINTF(2, 3)
int
pcintr_util_set_child(pcdom_element_t *parent, const char *fmt, ...);

pchtml_html_document_t*
pcintr_util_load_document(const char *html);

int
pcintr_util_comp_docs(pchtml_html_document_t *docl,
    pchtml_html_document_t *docr, int *diff);

bool
pcintr_util_is_ancestor(pcdom_node_t *ancestor, pcdom_node_t *descendant);


purc_vdom_t
purc_load_hvml_from_string_ex(const char* string,
        struct pcintr_supervisor_ops *ops, void *ctxt);

purc_vdom_t
purc_load_hvml_from_file_ex(const char* file,
        struct pcintr_supervisor_ops *ops, void *ctxt);

purc_vdom_t
purc_load_hvml_from_url_ex(const char* url,
        struct pcintr_supervisor_ops *ops, void *ctxt);

purc_vdom_t
purc_load_hvml_from_rwstream_ex(purc_rwstream_t stream,
        struct pcintr_supervisor_ops *ops, void *ctxt);

PCA_EXTERN_C_END

#endif  /* PURC_PRIVATE_INTERPRETER_H */

