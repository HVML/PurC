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
#include "private/map.h"
#include "private/list.h"
#include "private/vdom.h"
#include "private/timer.h"
#include "private/sorted-array.h"
#include "private/avl.h"

#define PCINTR_MOVE_BUFFER_SIZE 64
#define CRTN_TOKEN_LEN          15

#define MSG_EVENT_SEPARATOR          ':'

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
#define MSG_TYPE_FETCHER_STATE        "fetcherState"
#define MSG_TYPE_REQUEST_CHAN         "requestChan"
#define MSG_TYPE_NEW_RENDERER         "newRenderer"


#define MSG_SUB_TYPE_ASTERISK         "*"
#define MSG_SUB_TYPE_TIMEOUT          "timeout"
#define MSG_SUB_TYPE_SUCCESS          "success"
#define MSG_SUB_TYPE_EXCEPT           "except"
#define MSG_SUB_TYPE_CLOSE            "close"
#define MSG_SUB_TYPE_ATTACHED         "attached"
#define MSG_SUB_TYPE_DETACHED         "detached"
#define MSG_SUB_TYPE_DISPLACED        "displaced"
#define MSG_SUB_TYPE_EXITED           "exited"
#define MSG_SUB_TYPE_PAGE_LOADED      "pageLoaded"
#define MSG_SUB_TYPE_PAGE_SUPPRESSED  "pageSuppressed"
#define MSG_SUB_TYPE_PAGE_RELOADED    "pageReloaded"
#define MSG_SUB_TYPE_PAGE_CLOSED      "pageClosed"
#define MSG_SUB_TYPE_CONN_LOST        "connLost"
#define MSG_SUB_TYPE_OBSERVING        "observing"
#define MSG_SUB_TYPE_PROGRESS         "progress"
#define MSG_SUB_TYPE_NEW_RENDERER     "newRenderer"

#define CRTN_TOKEN_MAIN               "_main"
#define CRTN_TOKEN_FIRST              "_first"
#define CRTN_TOKEN_LAST               "_last"

#define CHAN_METHOD_POST              "post"

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
    struct pcinst      *owner;

    // currently running coroutine
    pcintr_coroutine_t  running_coroutine;

    struct list_head    crtns;
    struct list_head    stopped_crtns;
    struct avl_tree     wait_timeout_crtns_avl;

    size_t              nr_stopped_crtns;

    pcutils_map        *name_chan_map;  // name to channel map.
    pcutils_map        *token_crtn_map; // token to crtn map.

    /* coroutines which were loaded to the renderer */
    struct sorted_array *loaded_crtn_handles;

    purc_atom_t         move_buff;
    pcintr_timer_t     *event_timer;    // 10ms

    purc_cond_handler   cond_handler;
    unsigned int        keep_alive:1;
    double              timestamp;
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
typedef void (*observer_on_revoke_fn)(struct pcintr_observer *observer,
        void *data);

typedef bool
(*observer_match_fn)(pcintr_coroutine_t cor, struct pcintr_observer *observer,
        pcrdr_msg *msg, purc_variant_t observed, const char *type,
        const char *sub_type);

typedef int
(*observer_handle_fn)(pcintr_coroutine_t cor, struct pcintr_observer *observer,
        pcrdr_msg *msg, const char *type, const char *sub_type, void *data);

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
    char                         *tag_prefix;

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
    uint32_t                      terminated:1;
    uint32_t                      inherit:1;

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

    struct pcvcm_eval_ctxt       *vcm_ctxt;
    int                           vcm_eval_pos;         // -1 content, 0~n attr
    bool                          timeout;

    // for observe
    // struct pcintr_observer
    /* create by interpreter yield */
    struct list_head              intr_observers;

    /* create by hvml <observe on...> */
    struct list_head              hvml_observers;

    // async request ids (array)
    purc_variant_t                async_request_ids;

    // key: vdom_node  val: pcvarmgr_t
    struct rb_root                scoped_variables;

    // current dom text content
    pcdoc_element_t               curr_edom_elem;
    pcutils_mraw_t               *mraw;
    pcutils_str_t                *curr_edom_elem_text_content;
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
    char                        token[CRTN_TOKEN_LEN + 1];

    /* fields for renderer */
    pcrdr_page_type_k           target_page_type;
    uint64_t                    target_workspace_handle;
    uint64_t                    target_page_handle;
    uint64_t                    target_dom_handle;
    purc_variant_t              doc_contents;
    purc_variant_t              doc_wrotten_len;

    /* purc_renderer_extra_info */
    char                       *klass;
    char                       *title;
    char                       *page_groups;
    char                       *layout_style;
    purc_variant_t              toolkit_style;

    struct rb_node              node;     /* heap::coroutines */
    struct list_head            ln;       /* heap::crtns, stopped_crtns */

    struct list_head            doc_node;   /* doc::owner_list */

    const char                 *error_except;

    struct pcintr_stack         stack;  /* stack that holds this coroutine */

    enum pcintr_coroutine_stage stage;
    enum pcintr_coroutine_state state;
    int                         waits;  /* FIXME: nr of registered events */

    struct list_head            ln_stopped;
    struct list_head            registered_cancels;

    struct pcinst_msg_queue    *mq;     /* message queue */
    struct list_head            tasks;  /* one event with multiple observers */

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

    /** The default timeout value for remote requests or channel operations. */
    struct timespec             timeout;
    /* $CRTN  end */

    struct pcintr_timers       *timers;     // $TIMERS
    struct pcvarmgr            *variables;  // coroutine level named variable

    /* AVL node for the AVL tree sorted by stopped timeout */
    struct avl_node             avl;

    void                       *user_data;
    unsigned long               run_idx;
    time_t                      stopped_timeout;

    /* misc. flags go here */
    uint32_t                    is_main:1;
    uint32_t                    sending_document_by_url:1;
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

enum pcintr_stack_frame_eval_step {
    STACK_FRAME_EVAL_STEP_ATTR,
    STACK_FRAME_EVAL_STEP_CONTENT,
    STACK_FRAME_EVAL_STEP_DONE,
};

enum pcintr_element_step {
    ELEMENT_STEP_PREPARE,
    ELEMENT_STEP_EVAL_ATTR,
    ELEMENT_STEP_EVAL_CONTENT,
    ELEMENT_STEP_LOGIC,
    ELEMENT_STEP_DONE,
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
    /* element id attr value */
    purc_variant_t    elem_id;
    purc_variant_t    attr_in;

    unsigned int       silently:1;
    unsigned int       must_yield:1;
    unsigned int       handle_event:1;

    enum pcintr_stack_frame_eval_step eval_step;
    enum pcintr_element_step elem_step;
    size_t             eval_attr_pos;
    pcutils_array_t   *attrs_result;
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

enum pcintr_observer_source {
    OBSERVER_SOURCE_HVML,
    OBSERVER_SOURCE_INTR,
};

struct pcintr_observer {
    struct list_head            node;

    enum pcintr_observer_source source;
    int                         cor_stage;
    int                         cor_state;

    pcintr_stack_t              stack;
    // the observed variant.
    purc_variant_t observed;

    // the type of the message observed (cloned from the `for` attribute)
    char* type;

    // the sub type of the message observed (cloned from the `for` attribute; nullable).
    char* sub_type;

    pcvdom_element_t scope;
    pcdoc_element_t  edom_element;

    // the `observe` element who creates this observer.
    pcvdom_element_t pos;

    // the arraylist containing this struct pointer
    struct list_head* list;

    // callback when revoke observer
    observer_on_revoke_fn on_revoke;
    void *on_revoke_data;

    observer_match_fn   is_match;
    observer_handle_fn  handle;
    void               *handle_data;
    bool                auto_remove;
    uint64_t            timestamp;
};

struct pcinst;

struct pcintr_timers;

PCA_EXTERN_C_BEGIN

extern time_t g_purc_run_monotonic_ms;

bool pcintr_bind_builtin_runner_variables(void);

struct pcintr_heap* pcintr_get_heap(void);

pcintr_stack_t pcintr_get_stack(void);
pcintr_coroutine_t pcintr_get_coroutine(void);
// NOTE: null if current thread not initialized with purc_init
purc_runloop_t pcintr_get_runloop(void);

/* stop the specific coroutine; stop forever if timeout is NULL. */
void pcintr_stop_coroutine(pcintr_coroutine_t crtn,
        const struct timespec *timeout) WTF_INTERNAL;
/* resume the specific coroutine */
void pcintr_resume_coroutine(pcintr_coroutine_t crtn) WTF_INTERNAL;

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

void pcintr_resume(pcintr_coroutine_t cor, pcrdr_msg *msg);

int pcintr_yield(
        int                       cor_stage,
        int                       cor_state,
        purc_variant_t            observed,
        const char               *event_type,
        const char               *event_sub_type,
        observer_match_fn         observer_is_match,
        observer_handle_fn        observer_handle,
        void                     *observer_handle_data,
        bool                      observer_auto_remove
        );

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
        const char* name, purc_variant_t variant, pcvarmgr_t *mgr);

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
pcintr_get_named_var_for_event(pcintr_stack_t stack, const char *name,
        pcvarmgr_t mgr);

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
pcintr_register_observer(pcintr_stack_t  stack,
        enum pcintr_observer_source source,
        int                         cor_stage,
        int                         cor_state,
        purc_variant_t              observed,
        const char                 *type,
        const char                 *sub_type,
        pcvdom_element_t            scope,
        pcdoc_element_t             edom_element,
        pcvdom_element_t            pos,
        observer_on_revoke_fn       on_revoke,
        void                       *on_revoke_data,
        observer_match_fn           is_match,
        observer_handle_fn          handle,
        void                       *handle_data,
        bool                        auto_remove
        );

struct pcintr_observer*
pcintr_register_inner_observer(
        pcintr_stack_t            stack,
        int                       cor_stage,
        int                       cor_state,
        purc_variant_t            observed,
        const char               *event_type,
        const char               *event_sub_type,
        observer_match_fn         is_match,
        observer_handle_fn        handle,
        void                     *handle_data,
        bool                      auto_remove
        );

void
pcintr_revoke_observer(struct pcintr_observer* observer);

void
pcintr_revoke_observer_ex(pcintr_stack_t stack, purc_variant_t observed,
        const char *type, const char *sub_type);

bool
pcintr_load_dynamic_variant(pcintr_coroutine_t cor, const char *so_name,
    const char *var_name, const char *bind_name);

// utilities

pcdoc_element_t
pcintr_util_new_element(purc_document_t doc, pcdoc_element_t elem,
        pcdoc_operation_k op, const char *tag, bool self_close, bool sync_to_rdr);

void
pcintr_util_clear_element(purc_document_t doc, pcdoc_element_t elem,
        bool sync_to_rdr);

void
pcintr_util_erase_element(purc_document_t doc, pcdoc_element_t elem,
        bool sync_to_rdr);

int
pcintr_util_new_text_content(purc_document_t doc, pcdoc_element_t elem,
        pcdoc_operation_k op, const char *txt, size_t len, bool sync_to_rdr,
        bool no_return);

pcdoc_node
pcintr_util_new_content(purc_document_t doc,
        pcdoc_element_t elem, pcdoc_operation_k op,
        const char *content, size_t len, purc_variant_t data_type,
        bool sync_to_rdr, bool no_return);

pcdoc_data_node_t
pcintr_util_set_data_content(purc_document_t doc,
        pcdoc_element_t elem, pcdoc_operation_k op,
        purc_variant_t data, bool sync_to_rdr, bool no_return);

int
pcintr_util_set_attribute(purc_document_t doc,
        pcdoc_element_t elem, pcdoc_operation_k op,
        const char *name, const char *val, size_t len, bool sync_to_rdr,
        bool no_return);

static inline int pcintr_util_remove_attribute(purc_document_t doc,
        pcdoc_element_t elem, const char *name, bool sync_to_rdr,
        bool no_return)
{
    return pcintr_util_set_attribute(doc, elem, PCDOC_OP_ERASE,
        name, NULL, 0, sync_to_rdr, no_return);
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
        pcrdr_page_type_k page_type, const char *target_workspace,
        const char *target_group, const char *page_name,
        purc_renderer_extra_info *extra_info);

int
pcintr_post_event(purc_atom_t rid, purc_atom_t cid,
        pcrdr_msg_event_reduce_opt reduce_op, purc_variant_t source_uri,
        purc_variant_t element_value, purc_variant_t event_name,
        purc_variant_t data, purc_variant_t request_id);

int
pcintr_post_event_by_ctype(purc_atom_t rid, purc_atom_t cid,
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

pcrdr_msg_data_type
pcintr_rdr_retrieve_data_type(const char *type_name);


/* return true to ignore eval */
typedef bool (before_eval_attr_fn)(pcintr_stack_t stack,
        struct pcintr_stack_frame *frame, const char *attr_name,
        struct pcvcm_node *vcm);

int
pcintr_stack_frame_eval_attr_and_content_full(pcintr_stack_t stack,
        struct pcintr_stack_frame *frame, before_eval_attr_fn before_eval_attr,
        bool ignore_content);

static inline int
pcintr_stack_frame_eval_attr_and_content(pcintr_stack_t stack,
        struct pcintr_stack_frame *frame, bool ignore_content
        )
{
    return pcintr_stack_frame_eval_attr_and_content_full(stack, frame, NULL,
            ignore_content);
}

bool
pcintr_is_valid_crtn_token(const char *token);

const char *
pcintr_coroutine_get_token(pcintr_coroutine_t cor);

int
pcintr_coroutine_set_token(pcintr_coroutine_t cor, const char *token);

pcintr_coroutine_t
pcintr_get_first_crtn(struct pcinst *inst);

pcintr_coroutine_t
pcintr_get_last_crtn(struct pcinst *inst);

pcintr_coroutine_t
pcintr_get_main_crtn(struct pcinst *inst);

pcintr_coroutine_t
pcintr_get_crtn_by_token(struct pcinst *inst, const char *token);

int
pcintr_bind_named_variable(pcintr_stack_t stack,
        struct pcintr_stack_frame *frame, const char *name, purc_variant_t at,
        bool temporarily, bool runner_level_enable, purc_variant_t v);

purc_variant_t
pcintr_get_named_variable(pcintr_stack_t stack,
        struct pcintr_stack_frame *frame, const char *name, purc_variant_t at,
        bool temporarily, bool runner_level_enable);

int
pcintr_switch_new_renderer(struct pcinst *inst);

/* ms */
time_t pcintr_tick_count();

PCA_EXTERN_C_END

#endif  /* PURC_PRIVATE_INTERPRETER_H */

