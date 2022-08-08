/**
 * @file interpreter.h
 * @author Xu Xiaohong, Vincent Wei
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
#include "private/document.h"
#include "private/utils.h"
#include "private/list.h"
#include "private/vdom.h"
#include "private/timer.h"

#define PCINTR_MOVE_BUFFER_SIZE 64

#define MSG_TYPE_IDLE                 "idle"
#define MSG_TYPE_SLEEP                "sleep"
#define MSG_TYPE_CHANGE               "change"
#define MSG_TYPE_CALL_STATE           "callState"
#define MSG_TYPE_SUB_EXIT             "subExit"
#define MSG_TYPE_LAST_MSG             "lastMsg"
#define MSG_TYPE_ASYNC                "async"
#define MSG_TYPE_GROW                 "grow"
#define MSG_TYPE_SHRINK               "shrink"
#define MSG_TYPE_CHANGE               "change"
#define MSG_TYPE_CORSTATE             "corState"
#define MSG_TYPE_DESTROY              "destroy"
#define MSG_TYPE_RDR_STATE            "rdrState"
#define MSG_TYPE_REQUEST              "request"
#define MSG_TYPE_RESPONSE             "response"


#define MSG_SUB_TYPE_TIMEOUT          "timeout"
#define MSG_SUB_TYPE_SUCCESS          "success"
#define MSG_SUB_TYPE_EXCEPT           "except"
#define MSG_SUB_TYPE_CLOSE            "close"
#define MSG_SUB_TYPE_ATTACHED         "attached"
#define MSG_SUB_TYPE_DETACHED         "detached"
#define MSG_SUB_TYPE_DISPLACED        "displaced"
#define MSG_SUB_TYPE_EXITED           "exited"
#define MSG_SUB_TYPE_PAGE_CLOSED      "pageClosed"
#define MSG_SUB_TYPE_CONN_LOST        "connLost"
#define MSG_SUB_TYPE_OBSERVING        "observing"

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

struct pcintr_cancel;
typedef struct pcintr_cancel pcintr_cancel;
typedef struct pcintr_cancel *pcintr_cancel_t;

struct pcintr_coroutine_child;
typedef struct pcintr_coroutine_child pcintr_coroutine_child;
typedef struct pcintr_coroutine_child *pcintr_coroutine_child_t;

struct pcintr_cancel {
    void                        *ctxt;
    void (*cancel)(void *ctxt);

    struct list_head            *list;
    struct list_head             node;
};

struct pcintr_heap {
    // owner instance
    struct pcinst        *owner;

    // currently running coroutine
    pcintr_coroutine_t    running_coroutine;

    // those running under and managed by this heap
    // key as atom, val as struct pcintr_coroutine
    struct rb_root        coroutines;

    struct list_head      routines;     // struct pcintr_routine

    int64_t               next_coroutine_id;
    purc_atom_t           move_buff;
    pcintr_timer_t        *event_timer; // 10ms

    purc_cond_handler    cond_handler;
    unsigned int         keep_alive:1;
    double               timestamp;
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

struct pcintr_exception {
    int                      errcode;
    purc_atom_t              error_except;
    purc_variant_t           exinfo;
    struct pcvdom_element   *err_element;

    struct pcdebug_backtrace  *bt;
};

struct pcintr_stack {
    struct list_head              frames;
    // the number of stack frames.
    size_t                        nr_frames;

    // the pointer to the vDOM tree.
    purc_vdom_t                   vdom;
    purc_document_t               doc;

    struct pcvdom_element        *entry;

    // for `back` to use
    struct pcintr_stack_frame    *back_anchor;

    enum pcintr_stack_vdom_insertion_mode        mode;

    // executing state
    // FIXME: move to struct pcintr_coroutine?
    // uint32_t                   error:1;
    uint32_t                      except:1;
    uint32_t                      exited:1;
    uint32_t volatile             last_msg_sent:1;
    uint32_t volatile             last_msg_read:1;
    /* uint32_t                   paused:1; */
    uint32_t                      observe_idle:1;

    // error or except info
    // valid only when except == 1
    struct pcintr_exception       exception;

    // executing statistics
    struct timespec               time_executed;
    struct timespec               time_idle;
    size_t                        peak_mem_use;
    size_t                        peak_nr_variants;

    /* coroutine that this stack `owns` */
    /* FIXME: switch owner-ship ? */
    struct pcintr_coroutine      *co;
    char                         *body_id;

    // for observe
    // struct pcintr_observer
    struct list_head              common_observers;
    struct list_head              dynamic_observers;
    struct list_head              native_observers;

    // async request ids (array)
    purc_variant_t                async_request_ids;

    // key: vdom_node  val: pcvarmgr_t
    struct rb_root                scoped_variables;
};

enum pcintr_coroutine_stage {
    CO_STAGE_SCHEDULED  = 0x01,
    CO_STAGE_FIRST_RUN  = 0x02,
    CO_STAGE_OBSERVING  = 0x04,
    CO_STAGE_CLEANUP    = 0x08,
};

enum pcintr_coroutine_state {
    CO_STATE_READY      = 0x01,          /* ready to run next step */
    CO_STATE_RUNNING    = 0x02,          /* is running */
    CO_STATE_STOPPED    = 0x04,          /* is waiting for event */
    CO_STATE_OBSERVING  = 0x08,
    CO_STATE_EXITED     = 0x10,
    CO_STATE_TERMINATED = 0x20,
    CO_STATE_TRACKED    = 0x40,
};

typedef void (pcintr_msg_callback_f)(void *ctxt);

struct pcintr_msg {
    void                       *ctxt;
    void (*on_msg)(void *ctxt);

    struct list_head            node;
};

struct pcintr_coroutine_child {
    struct list_head            ln;
    purc_atom_t                 cid;
};

struct pcintr_coroutine {
    pcintr_heap_t               owner;    /* owner heap */
    purc_atom_t                 cid;
    purc_atom_t                 curator;

    purc_vdom_t                 vdom;

    /* fields for renderer */
    pcrdr_page_type             target_page_type;
    uint64_t                    target_workspace_handle;
    uint64_t                    target_page_handle;
    uint64_t                    target_dom_handle;
    purc_variant_t              doc_contents;
    purc_variant_t              doc_wrotten_len;

    struct rb_node              node;     /* heap::coroutines */

    struct list_head            children; /* struct pcintr_coroutine_child */

    const char                 *error_except;

    struct pcintr_stack         stack;  /* stack that holds this coroutine */

    enum pcintr_coroutine_stage stage;
    enum pcintr_coroutine_state state;
    int                         waits;  /* FIXME: nr of registered events */

    struct list_head            registered_cancels;
    void                       *yielded_ctxt;
    void (*continuation)(void *ctxt, pcrdr_msg *msg);

    purc_variant_t              wait_request_id;    /* pcrdr_msg.requestId */
    purc_variant_t              wait_element_value; /* pcrdr_msg.elementValue */
    purc_variant_t              wait_event_name;    /* pcrdr_msg.eventName */

    struct pcinst_msg_queue    *mq;     /* message queue */
    struct list_head            tasks;  /* one event with multiple observers */
    struct list_head            event_handlers; /* struct pcintr_event_handler */
    struct pcintr_event_handler *sleep_handler;

    /* $CRTN  begin */
    /** The target as a null-terminated string. */
    char                       *target;

    /** The base URL as a null-terminated string. */
    char                       *base_url_string;

    /** The base URL broken down. */
    struct purc_broken_down_url base_url_broken_down;

    /** The maximal iteration count. */
    uint64_t                    max_iteration_count;
    /** The maximal recursion depth. */
    uint64_t                    max_recursion_depth;
    /** The maximal embedded levels of a EJSON container. */
    uint64_t                    max_embedded_levels;

    /** The timeout value for a remote request. */
    struct timespec             timeout;
    /* $CRTN  end */

    struct pcintr_timers       *timers;     // $TIMERS
    struct pcvarmgr            *variables;  // coroutine level named variable

    // for loaded dynamic variants
    struct rb_root              loaded_vars;  // struct pcintr_loaded_var*

    void                       *user_data;
    unsigned long               run_idx;
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
    pcdoc_element_t edom_element;

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

    pcintr_stack_t              stack;
    // the observed variant.
    purc_variant_t observed;

    // the type of the message observed (cloned from the `for` attribute)
    purc_atom_t msg_type_atom;

    // the sub type of the message observed (cloned from the `for` attribute; nullable).
    char* sub_type;

    pcvdom_element_t scope;
    pcdoc_element_t  edom_element;

    // the `observe` element who creates this observer.
    pcvdom_element_t pos;

    // the arraylist containing this struct pointer
    struct list_head* list;

    // callback when revoke observer
    pcintr_on_revoke_observer on_revoke;
    void *on_revoke_data;
};

struct pcinst;

struct pcintr_timers;

PCA_EXTERN_C_BEGIN

bool pcintr_bind_builtin_runner_variables(void);

struct pcintr_heap* pcintr_get_heap(void);

pcintr_stack_t pcintr_get_stack(void);
pcintr_coroutine_t pcintr_get_coroutine(void);
// NOTE: null if current thread not initialized with purc_init
purc_runloop_t pcintr_get_runloop(void);

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

void pcintr_yield(void *ctxt, void (*continuation)(void *ctxt, pcrdr_msg *msg),
        purc_variant_t request_id, purc_variant_t element_value,
        purc_variant_t event_name, bool custom_event_handler);
void pcintr_resume(pcintr_coroutine_t cor, pcrdr_msg *msg);

void
pcintr_push_stack_frame_pseudo(pcvdom_element_t vdom_element);
void
pcintr_pop_stack_frame_pseudo(void);

void
pcintr_exception_clear(struct pcintr_exception *exception);

void
pcintr_exception_move(struct pcintr_exception *dst,
        struct pcintr_exception *src);

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

static inline pcvarmgr_t
pcintr_get_coroutine_variables(purc_coroutine_t cor)
{
    return cor->variables;
}

pcvarmgr_t
pcintr_get_scoped_variables(purc_coroutine_t cor, struct pcvdom_node *node);

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
struct pcintr_timers *
pcintr_timers_init(purc_coroutine_t cor);

void
pcintr_timers_destroy(struct pcintr_timers* timers);

bool
pcintr_is_timers(purc_coroutine_t cor, purc_variant_t v);

// type:sub_type
bool
pcintr_parse_event(const char *event, purc_variant_t *type,
        purc_variant_t *sub_type);

struct pcintr_observer*
pcintr_register_observer(pcintr_stack_t stack,
        purc_variant_t observed,
        purc_variant_t for_value,
        purc_atom_t msg_type_atom, const char *sub_type,
        pcvdom_element_t scope,
        pcdoc_element_t edom_element,
        pcvdom_element_t pos,
        pcintr_on_revoke_observer on_revoke,
        void *on_revoke_data
        );

void
pcintr_revoke_observer(struct pcintr_observer* observer);

void
pcintr_revoke_observer_ex(pcintr_stack_t stack, purc_variant_t observed,
        purc_atom_t msg_type_atom, const char *sub_type);

bool
pcintr_load_dynamic_variant(pcintr_coroutine_t cor,
    const char *name, size_t len);

// utilities

pcdoc_element_t
pcintr_util_new_element(purc_document_t doc, pcdoc_element_t elem,
        pcdoc_operation op, const char *tag, bool self_close);

pcdoc_text_node_t
pcintr_util_new_text_content(purc_document_t doc, pcdoc_element_t elem,
        pcdoc_operation op, const char *txt, size_t len);

pcdoc_node
pcintr_util_new_content(purc_document_t doc,
        pcdoc_element_t elem, pcdoc_operation op,
        const char *content, size_t len);

int
pcintr_util_set_attribute(purc_document_t doc,
        pcdoc_element_t elem, pcdoc_operation op,
        const char *name, const char *val, size_t len);

static inline int pcintr_util_remove_attribute(purc_document_t doc,
        pcdoc_element_t elem, const char *name)
{
    return pcintr_util_set_attribute(doc, elem, PCDOC_OP_ERASE,
        name, NULL, 0);
}

int
pcintr_init_vdom_under_stack(pcintr_stack_t stack);

purc_runloop_t
pcintr_co_get_runloop(pcintr_coroutine_t co);

void*
pcintr_load_module(const char *module,
        const char *env_name, const char *prefix);

void
pcintr_unload_module(void *handle);

int
pcintr_init_loader_once(void);

bool
pcintr_attach_to_renderer(pcintr_coroutine_t cor,
        pcrdr_page_type page_type, const char *target_workspace,
        const char *target_group, const char *page_name,
        purc_renderer_extra_info *extra_info);

int
pcintr_post_event(purc_atom_t cid,
        pcrdr_msg_event_reduce_opt reduce_op, purc_variant_t source_uri,
        purc_variant_t element_value, purc_variant_t event_name,
        purc_variant_t data, purc_variant_t request_id);

int
pcintr_post_event_by_ctype(purc_atom_t cid,
        pcrdr_msg_event_reduce_opt reduce_op, purc_variant_t source_uri,
        purc_variant_t element_value, const char *event_type,
        const char *event_sub_type, purc_variant_t data,
        purc_variant_t request_id);

int
pcintr_coroutine_post_event(purc_atom_t cid,
        pcrdr_msg_event_reduce_opt reduce_op,
        purc_variant_t element_value, const char *event_type,
        const char *event_sub_type, purc_variant_t data,
        purc_variant_t request_id);

static inline const char*
pcintr_coroutine_get_uri(pcintr_coroutine_t co)
{
    return purc_atom_to_string(co->cid);
}

void
pcintr_schedule(void *ctxt);

void
pcintr_coroutine_set_result(pcintr_coroutine_t co, purc_variant_t result);

purc_variant_t
pcintr_coroutine_get_result(pcintr_coroutine_t co);

bool
pcintr_is_variable_token(const char *str);


PCA_EXTERN_C_END

#endif  /* PURC_PRIVATE_INTERPRETER_H */

