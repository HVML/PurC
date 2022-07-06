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
#include "private/utils.h"
#include "private/list.h"
#include "private/vdom.h"
#include "private/timer.h"

struct pcintr_heap;
typedef struct pcintr_heap pcintr_heap;
typedef struct pcintr_heap *pcintr_heap_t;

struct pcintr_coroutine;
typedef struct pcintr_coroutine pcintr_coroutine;
typedef struct pcintr_coroutine *pcintr_coroutine_t;

struct pcintr_stack;
typedef struct pcintr_stack pcintr_stack;
typedef struct pcintr_stack *pcintr_stack_t;

struct pcintr_msg;
typedef struct pcintr_msg pcintr_msg;
typedef struct pcintr_msg *pcintr_msg_t;

struct pcintr_event;
typedef struct pcintr_event pcintr_event;
typedef struct pcintr_event *pcintr_event_t;

struct pcintr_cancel;
typedef struct pcintr_cancel pcintr_cancel;
typedef struct pcintr_cancel *pcintr_cancel_t;

struct pcintr_coroutine_result;
typedef struct pcintr_coroutine_result pcintr_coroutine_result;
typedef struct pcintr_coroutine_result *pcintr_coroutine_result_t;

struct pcintr_cancel {
    void                        *ctxt;
    void (*cancel)(void *ctxt);

    struct list_head            *list;
    struct list_head             node;
};

struct pcintr_heap {
    // owner instance
    struct pcinst        *owner;

    struct list_head     *owning_heaps;
    struct list_head      sibling;         // struct pcintr_heap

    // currently running coroutine
    pcintr_coroutine_t    running_coroutine;

    // those running under and managed by this heap
    // key as atom, val as struct pcintr_coroutine
    struct rb_root        coroutines;

    pthread_mutex_t       locker;
    volatile bool         exiting;
    struct list_head      routines;     // struct pcintr_routine

    int64_t               next_coroutine_id;
    purc_atom_t           move_buff;
    pcintr_timer_t        *event_timer; // 10ms

    purc_event_handler    event_handler;
};

struct pcintr_stack_frame;
typedef struct pcintr_stack_frame pcintr_stack_frame;
typedef struct pcintr_stack_frame *pcintr_stack_frame_t;

struct pcintr_stack_frame_normal;
typedef struct pcintr_stack_frame_normal pcintr_stack_frame_normal;
typedef struct pcintr_stack_frame_normal *pcintr_stack_frame_normal_t;

struct pcintr_stack_frame_pseudo;
typedef struct pcintr_stack_frame_pseudo pcintr_stack_frame_pseudo;
typedef struct pcintr_stack_frame_pseudo *pcintr_stack_frame_pseudo_t;

struct pcintr_observer;
typedef void (*pcintr_on_revoke_observer)(struct pcintr_observer *observer,
        void *data);

enum pcintr_stack_stage {
    STACK_STAGE_FIRST_ROUND                = 0x00,
    STACK_STAGE_EVENT_LOOP                 = 0x01,
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

#if 0 // VW
// experimental: currently for test-case-only
struct pcintr_supervisor_ops {
    void (*on_terminated)(pcintr_stack_t stack, void *ctxt);
    void (*on_cleanup)(pcintr_stack_t stack, void *ctxt);
};
#endif

struct pcintr_exception {
    int                      errcode;
    purc_atom_t              error_except;
    purc_variant_t           exinfo;
    struct pcvdom_element   *err_element;

    struct pcdebug_backtrace  *bt;
};

struct pcintr_stack {
    struct pcintr_heap           *owning_heap;
    struct list_head frames;

    // the number of stack frames.
    size_t nr_frames;

    // the pointer to the vDOM tree.
    purc_vdom_t vdom;
    struct pcvdom_element         *entry;

    enum pcintr_stack_vdom_insertion_mode        mode;

    // the returned variant
    purc_variant_t ret_var;

    // executing state
    // FIXME: move to struct pcintr_coroutine?
    // uint32_t        error:1;
    uint32_t        except:1;
    uint32_t        exited:1;
    uint32_t volatile       last_msg_sent:1;
    uint32_t volatile       last_msg_read:1;
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
    struct pcintr_coroutine       *co;

    // for observe
    // struct pcintr_observer
    struct list_head common_variant_observer_list;
    struct list_head dynamic_variant_observer_list;
    struct list_head native_variant_observer_list;

    pchtml_html_document_t     *doc;

    // for loaded dynamic variants
    struct rb_root             loaded_vars;  // struct pcintr_loaded_var*

    // base uri
    char* base_uri;

#if 0 // VW
    // experimental: currently for test-case-only
    struct pcintr_supervisor_ops         ops;
    void                                *ctxt;  // no-owner-ship!!!
#endif

    purc_variant_t async_request_ids;       // async request ids (array)

    struct rb_root  scoped_variables; // key: vdom_node
                                      // val: pcvarmgr_t
    struct pcintr_timers  *timers;
};

enum pcintr_coroutine_state {
    CO_STATE_READY,            /* ready to run next step */
    CO_STATE_RUN,              /* is running */
    CO_STATE_WAIT,             /* is waiting for event */
    /* STATE_PAUSED, */
};

typedef void (pcintr_msg_callback_f)(void *ctxt);

struct pcintr_msg {
    void                       *ctxt;
    void (*on_msg)(void *ctxt);

    struct list_head            node;
};

struct pcintr_event {
    purc_atom_t msg_type;
    purc_variant_t msg_sub_type;
    purc_variant_t src;
    purc_variant_t payload;
};

struct pcintr_coroutine_result {
    purc_variant_t              as;
    purc_variant_t              result;
    struct list_head            node;     /* parent:children */
};

struct pcintr_coroutine {
    pcintr_heap_t               owner;    /* owner heap */
    char                       *full_name;   /* prefixed with runnerName/ */
    purc_atom_t                 ident;
    purc_atom_t                 curator;

    purc_vdom_t                 vdom;

    const struct purc_hvml_ctrl_props     *hvml_ctrl_props;
    char **dump_buff;

    /* fields for renderer */
    pcrdr_page_type target_page_type;
    uint64_t        target_workspace_handle;
    uint64_t        target_page_handle;
    uint64_t        target_dom_handle;

    struct rb_node              node;     /* heap::coroutines */

    struct list_head            children; /* struct pcintr_coroutine_result */

    pcintr_coroutine_result_t   result;

    purc_variant_t              val_from_return_or_exit;
    const char                 *error_except;

    struct pcintr_stack         stack;  /* stack that holds this coroutine */

    enum pcintr_coroutine_state state;
    int                         waits;  /* FIXME: nr of registered events */

    struct list_head            registered_cancels;
    void                       *yielded_ctxt;
    void (*continuation)(void *ctxt, void *extra);

    struct list_head            msgs;   /* struct pcintr_msg */

    struct pcinst_msg_queue    *mq;     /* message queue */
    unsigned int volatile       msg_pending:1;
    unsigned int volatile       execution_pending:1;

    void                       *user_data;
};

enum purc_symbol_var {
    PURC_SYMBOL_VAR_QUESTION_MARK = 0,  // ?
    PURC_SYMBOL_VAR_LESS_THAN,          // <
    PURC_SYMBOL_VAR_AT_SIGN,            // @
    PURC_SYMBOL_VAR_EXCLAMATION,        // !
    PURC_SYMBOL_VAR_COLON,              // :
    PURC_SYMBOL_VAR_EQUAL,              // =
    PURC_SYMBOL_VAR_PERCENT_SIGN,       // %
    PURC_SYMBOL_VAR_CARET,              // ^

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

enum pcintr_stack_frame_type {
    STACK_FRAME_TYPE_NORMAL,
    STACK_FRAME_TYPE_PSEUDO,
};

struct pcintr_stack_frame {
    enum pcintr_stack_frame_type             type;
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

    pcintr_stack_t     owner;

    purc_variant_t     except_templates;
    purc_variant_t     error_templates;

    unsigned int       silently:1;
};

struct pcintr_stack_frame_normal {
    int                                 dummy_guard;
    struct pcintr_stack_frame           frame;
};

struct pcintr_stack_frame_pseudo {
    int                                 dummy_guard;
    struct pcintr_stack_frame           frame;
};

struct pcintr_dynamic_args {
    const char                    *name;
    purc_dvariant_method           getter;
    purc_dvariant_method           setter;
};

struct pcintr_observer {
    struct list_head            node;

    // the observed variant.
    purc_variant_t observed;

    // the type of the message observed (cloned from the `for` attribute)
    purc_atom_t msg_type_atom;

    // the sub type of the message observed (cloned from the `for` attribute; nullable).
    char* sub_type;

    pcvdom_element_t scope;
    pcdom_element_t *edom_element;

    // the `observe` element who creates this observer.
    pcvdom_element_t pos;

    // keep at_symbol
    purc_variant_t at_symbol;

    // the arraylist containing this struct pointer
    struct list_head* list;

    // callback when revoke observer
    pcintr_on_revoke_observer on_revoke;
    void *on_revoke_data;
};

struct pcinst;

struct pcintr_timers;

PCA_EXTERN_C_BEGIN

struct pcintr_heap* pcintr_get_heap(void);
bool pcintr_is_current_thread(void);

void pcintr_add_heap(struct list_head *all_heaps);
void pcintr_remove_heap(struct list_head *all_heaps);

pcintr_stack_t pcintr_get_stack(void);
pcintr_coroutine_t pcintr_get_coroutine(void);
// NOTE: null if current thread not initialized with purc_init
purc_runloop_t pcintr_get_runloop(void);

const char*
pcintr_get_first_app_name(void);

void pcintr_check_after_execution(void);
void pcintr_set_current_co_with_location(pcintr_coroutine_t co,
        const char *file, int line, const char *func);

#define pcintr_set_current_co(co) \
    pcintr_set_current_co_with_location(co, __FILE__, __LINE__, __func__)

bool pcintr_is_ready_for_event(void);

void pcintr_cancel_init(pcintr_cancel_t cancel,
        void *ctxt, void (*cancel_routine)(void *ctxt));

void pcintr_register_cancel(pcintr_cancel_t cancel);
void pcintr_unregister_cancel(pcintr_cancel_t cancel);

void pcintr_set_exit(purc_variant_t val);

struct pcintr_stack_frame*
pcintr_stack_get_bottom_frame(pcintr_stack_t stack);
struct pcintr_stack_frame*
pcintr_stack_frame_get_parent(struct pcintr_stack_frame *frame);

void pcintr_yield(void *ctxt, void (*continuation)(void *ctxt, void *extra));
void pcintr_resume(void *extra);

void
pcintr_push_stack_frame_pseudo(pcvdom_element_t vdom_element);
void
pcintr_pop_stack_frame_pseudo(void);

pcintr_coroutine_t
pcintr_create_child_co(pcvdom_element_t vdom_element,
        purc_variant_t as, purc_variant_t within);

pcintr_coroutine_t
pcintr_load_child_co(const char *hvml,
        purc_variant_t as, purc_variant_t within);

void
pcintr_exception_clear(struct pcintr_exception *exception);

void
pcintr_exception_move(struct pcintr_exception *dst,
        struct pcintr_exception *src);

void
pcintr_post_msg(void *ctxt, pcintr_msg_callback_f cb);

void
pcintr_post_msg_to_target(pcintr_coroutine_t target, void *ctxt,
        pcintr_msg_callback_f cb);

purc_variant_t
pcintr_make_object_of_dynamic_variants(size_t nr_args,
    struct pcintr_dynamic_args *args);

static inline bool
pcintr_bind_coroutine_variable(purc_coroutine_t cor, const char* name,
        purc_variant_t variant)
{
    return purc_coroutine_bind_variable(cor, name, variant);
}

static inline bool
pcintr_unbind_coroutine_variable(purc_coroutine_t cor, const char* name)
{
    return purc_coroutine_unbind_variable(cor, name);
}

static inline purc_variant_t
pcintr_get_coroutine_variable(purc_coroutine_t cor, const char* name)
{
    return purc_coroutine_get_variable(cor, name);
}

pcvarmgr_t
pcintr_get_scoped_variables(purc_coroutine_t cor, struct pcvdom_node *node);

static inline pcvarmgr_t
pcintr_get_coroutine_variables(purc_coroutine_t cor)
{
    return pcintr_get_scoped_variables(cor, pcvdom_doc_cast_to_node(cor->vdom));
}

static inline pcvarmgr_t
pcintr_get_scope_variables(purc_coroutine_t cor, pcvdom_element_t elem)
{
    return pcintr_get_scoped_variables(cor, pcvdom_ele_cast_to_node(elem));
}

bool
pcintr_bind_scope_variable(purc_coroutine_t cor, pcvdom_element_t elem,
        const char* name, purc_variant_t variant);

bool
pcintr_unbind_scope_variable(purc_coroutine_t cor, pcvdom_element_t elem,
        const char* name);

purc_variant_t
pcintr_get_scope_variable(purc_coroutine_t cor, pcvdom_element_t elem,
        const char* name);

purc_variant_t
pcintr_find_named_var(pcintr_stack_t stack, const char* name);

purc_variant_t
pcintr_get_symbolized_var (pcintr_stack_t stack, unsigned int number,
        char symbol);

purc_variant_t
pcintr_find_anchor_symbolized_var(pcintr_stack_t stack, const char *anchor,
        char symbol);

int
pcintr_unbind_named_var(pcintr_stack_t stack, const char *name);

purc_variant_t
pcintr_get_named_var_for_observed(pcintr_stack_t stack, const char *name,
        pcvdom_element_t elem);

purc_variant_t
pcintr_get_named_var_for_event(pcintr_stack_t stack, const char *name);

bool
pcintr_is_named_var_for_event(purc_variant_t val);

// $TIMERS
struct pcintr_timers*
pcintr_timers_init(pcintr_stack_t stack);

void
pcintr_timers_destroy(struct pcintr_timers* timers);

bool
pcintr_is_timers(pcintr_stack_t stack, purc_variant_t v);

// type:sub_type
bool
pcintr_parse_event(const char *event, purc_variant_t *type,
        purc_variant_t *sub_type);

struct pcintr_observer*
pcintr_register_observer(purc_variant_t observed,
        purc_variant_t for_value,
        purc_atom_t msg_type_atom, const char *sub_type,
        pcvdom_element_t scope,
        pcdom_element_t *edom_element,
        pcvdom_element_t pos,
        purc_variant_t at_symbol,
        pcintr_on_revoke_observer on_revoke,
        void *on_revoke_data
        );

void
pcintr_revoke_observer(struct pcintr_observer* observer);

void
pcintr_revoke_observer_ex(purc_variant_t observed,
        purc_atom_t msg_type_atom, const char *sub_type);

void
pcintr_on_event(purc_atom_t msg_type, purc_variant_t msg_sub_type,
        purc_variant_t src, purc_variant_t payload);

void
pcintr_fire_event_to_target(pcintr_coroutine_t target,
        purc_atom_t msg_type,
        purc_variant_t msg_sub_type,
        purc_variant_t src,
        purc_variant_t payload);

bool
pcintr_load_dynamic_variant(pcintr_stack_t stack,
    const char *name, size_t len);

// utilities
void
pcintr_util_dump_document_ex(pchtml_html_document_t *doc, char **dump_buff,
    const char *file, int line, const char *func);

void
pcintr_util_dump_edom_node_ex(pcdom_node_t *node,
    const char *file, int line, const char *func);

#define pcintr_util_dump_document(_doc)          \
    pcintr_util_dump_document_ex(_doc, NULL, __FILE__, __LINE__, __func__)

#define pcintr_util_dump_edom_node(_node)        \
    pcintr_util_dump_edom_node_ex(_node, __FILE__, __LINE__, __func__)

#define pcintr_dump_document(_stack)             \
    pcintr_util_dump_document_ex(_stack->doc, _stack->co->dump_buff, \
            __FILE__, __LINE__, __func__)

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
pcintr_util_remove_attribute(pcdom_element_t *elem, const char *key);

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

#if 0
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
#endif

int
pcintr_init_vdom_under_stack(pcintr_stack_t stack);

pcvarmgr_t
pcintr_create_scoped_variables(struct pcvdom_node *node);

purc_runloop_t
pcintr_co_get_runloop(pcintr_coroutine_t co);

typedef void (*co_routine_f)(void);

void
pcintr_wakeup_target(pcintr_coroutine_t target, co_routine_f routine);

void
pcintr_apply_routine(co_routine_f routine, pcintr_coroutine_t target);

void
pcintr_wakeup_target_with(pcintr_coroutine_t target, void *ctxt,
        void (*func)(void *ctxt));

void*
pcintr_load_module(const char *module,
        const char *env_name, const char *prefix);

void
pcintr_unload_module(void *handle);

int
pcintr_init_loader_once(void);

static inline void
pcintr_coroutine_set_dump_buff(purc_coroutine_t co, char **dump_buff)
{
    co->dump_buff = dump_buff;
}

bool
pcintr_attach_to_renderer(pcintr_coroutine_t cor,
        pcrdr_page_type page_type, const char *target_workspace,
        const char *target_group, const char *page_name,
        purc_renderer_extra_info *extra_info);

int
pcintr_post_event(purc_atom_t co_id,
        pcrdr_msg_event_reduce_opt reduce_op, purc_variant_t source_uri,
        purc_variant_t observed, purc_variant_t event_name,
        purc_variant_t data);

int
pcintr_post_event_by_ctype(purc_atom_t co_id,
        pcrdr_msg_event_reduce_opt reduce_op, purc_variant_t source_uri,
        purc_variant_t observed, const char *event_type,
        const char *event_sub_type, purc_variant_t data);

PCA_EXTERN_C_END

#endif  /* PURC_PRIVATE_INTERPRETER_H */

