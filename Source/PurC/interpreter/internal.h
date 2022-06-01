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
#include "private/fetcher.h"

#include "keywords.h"

struct pcvdom_template_node {
    struct list_head              node;
    struct pcvcm_node            *vcm;
};

struct pcvdom_template {
    struct list_head             list;
};

PCA_EXTERN_C_BEGIN

int
pcintr_check_insertion_mode_for_normal_element(pcintr_stack_t stack);

typedef int (*pcintr_attr_f)(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name,
        struct pcvdom_attr *attr,
        void *ud);
int
pcintr_vdom_walk_attrs(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element, void *ud, pcintr_attr_f cb);

bool
pcintr_is_element_silently(struct pcvdom_element *element);

int
pcintr_set_symbol_var(struct pcintr_stack_frame *frame,
        enum purc_symbol_var symbol, purc_variant_t val);
purc_variant_t
pcintr_get_symbol_var(struct pcintr_stack_frame *frame,
        enum purc_symbol_var symbol);

int
pcintr_set_at_var(struct pcintr_stack_frame *frame, purc_variant_t val);
int
pcintr_refresh_at_var(struct pcintr_stack_frame *frame);

int
pcintr_set_question_var(struct pcintr_stack_frame *frame, purc_variant_t val);
purc_variant_t
pcintr_get_question_var(struct pcintr_stack_frame *frame);

int
pcintr_set_exclamation_var(struct pcintr_stack_frame *frame,
        purc_variant_t val);
purc_variant_t
pcintr_get_exclamation_var(struct pcintr_stack_frame *frame);

int
pcintr_inc_percent_var(struct pcintr_stack_frame *frame);
purc_variant_t
pcintr_get_percent_var(struct pcintr_stack_frame *frame);

// $<
void
pcintr_set_input_var(pcintr_stack_t stack, purc_variant_t val);

int
pcintr_set_edom_attribute(pcintr_stack_t stack, struct pcvdom_attr *attr);

purc_variant_t
pcintr_eval_vdom_attr(pcintr_stack_t stack, struct pcvdom_attr *attr);

purc_variant_t
pcintr_load_from_uri(pcintr_stack_t stack, const char* uri);

purc_variant_t
pcintr_load_from_uri_async(pcintr_stack_t stack, const char* uri,
        pcfetcher_response_handler handler, void* ctxt);

bool
pcintr_save_async_request_id(pcintr_stack_t stack, purc_variant_t req_id);

bool
pcintr_remove_async_request_id(pcintr_stack_t stack, purc_variant_t req_id);

purc_variant_t
pcintr_load_vdom_fragment_from_uri(pcintr_stack_t stack, const char* uri);

purc_variant_t
pcintr_doc_query(purc_vdom_t vdom, const char* css, bool silently);



purc_variant_t
pcintr_template_make(void);

int
pcintr_template_append(purc_variant_t val, struct pcvcm_node *vcm);


typedef int
(*pcintr_template_walk_cb)(struct pcvcm_node *vcm, void *ctxt);

void
pcintr_template_walk(purc_variant_t val, void *ctxt,
        pcintr_template_walk_cb cb);


typedef purc_variant_t
(*pcintr_attribute_op)(purc_variant_t left, purc_variant_t right);

pcintr_attribute_op
pcintr_attribute_get_op(enum pchvml_attr_operator op);

pcrdr_msg *pcintr_rdr_send_request_and_wait_response(struct pcrdr_conn *conn,
        pcrdr_msg_target target, uint64_t target_value, const char *operation,
        pcrdr_msg_element_type element_type, const char *element,
        const char *property, pcrdr_msg_data_type data_type,
        purc_variant_t data);

uintptr_t pcintr_rdr_create_workspace(struct pcrdr_conn *conn,
        uintptr_t session, const char *id, const char *title,
        const char* classes, const char *style);

bool pcintr_rdr_destroy_workspace(struct pcrdr_conn *conn,
        uintptr_t session, uintptr_t workspace);

// property: title, class, style
bool pcintr_rdr_update_workspace(struct pcrdr_conn *conn,
        uintptr_t session, uintptr_t workspace,
        const char *property, const char *value);



uintptr_t pcintr_rdr_create_plain_window(struct pcrdr_conn *conn,
        uintptr_t workspace, pcrdr_page_type page_type, const char *id,
        const char *title, const char *classes, const char *style);

bool pcintr_rdr_destroy_plain_window(struct pcrdr_conn *conn,
        uintptr_t session, uintptr_t workspace, uintptr_t plain_window);

// property: title, class, style
bool pcintr_rdr_update_plain_window(struct pcrdr_conn *conn,
        uintptr_t session, uintptr_t workspace, uintptr_t plain_window,
        const char *property, const char *value);

bool pcintr_rdr_reset_page_groups(struct pcrdr_conn *conn,
        uintptr_t session, uintptr_t workspace, const char *data
        );

uintptr_t pcintr_rdr_add_page_groups(struct pcrdr_conn *conn,
        uintptr_t session, uintptr_t workspace, const char *id,
        const char *title, const char* classes, const char *style,
        const char* level);

bool pcintr_rdr_destroy_page_groups(struct pcrdr_conn *conn,
        uintptr_t session, uintptr_t workspace, uintptr_t tabbed_window);

// property: title, class, style
bool pcintr_rdr_update_page_groups(struct pcrdr_conn *conn,
        uintptr_t session, uintptr_t workspace, uintptr_t tabbed_window,
        const char *property, const char *value);


uintptr_t pcintr_rdr_create_page(struct pcrdr_conn *conn,
        uintptr_t tabbed_window, const char *id, const char *title);

bool pcintr_rdr_destroy_page(struct pcrdr_conn *conn,
        uintptr_t tabbed_window, uintptr_t tab_page);

// property: title, class, style
bool pcintr_rdr_update_page(struct pcrdr_conn *conn,
        uintptr_t tabbed_window, uintptr_t tab_page,
        const char *property, const char *value);

bool
pcintr_rdr_page_control_load(pcintr_stack_t stack);

pcrdr_msg *
pcintr_rdr_send_dom_req(pcintr_stack_t stack, const char *operation,
        pcdom_element_t *element, const char* property,
        pcrdr_msg_data_type data_type, purc_variant_t data);

pcrdr_msg *
pcintr_rdr_send_dom_req_raw(pcintr_stack_t stack, const char *operation,
        pcdom_element_t *element, const char* property,
        pcrdr_msg_data_type data_type, const char *data);

bool
pcintr_rdr_send_dom_req_simple(pcintr_stack_t stack, const char *operation,
        pcdom_element_t *element, const char* property,
        pcrdr_msg_data_type data_type, purc_variant_t data);

bool
pcintr_rdr_send_dom_req_simple_raw(pcintr_stack_t stack,
        const char *operation, pcdom_element_t *element,
        const char *property, pcrdr_msg_data_type data_type, const char *data);


#define pcintr_rdr_dom_append_content(stack, element, content)                \
    pcintr_rdr_send_dom_req_simple_raw(stack, PCRDR_OPERATION_APPEND,         \
            element, NULL, PCRDR_MSG_DATA_TYPE_TEXT, content)

#define pcintr_rdr_dom_prepend_content(stack, element, content)               \
    pcintr_rdr_send_dom_req_simple_raw(stack, PCRDR_OPERATION_PREPEND,        \
            element, NULL, PCRDR_MSG_DATA_TYPE_TEXT, content)

#define pcintr_rdr_dom_insert_before_element(stack, element, content)         \
    pcintr_rdr_send_dom_req_simple_raw(stack, PCRDR_OPERATION_INSERTBEFORE,   \
            element, NULL, PCRDR_MSG_DATA_TYPE_TEXT, content)

#define pcintr_rdr_dom_insert_after_element(stack, element, content)          \
    pcintr_rdr_send_dom_req_simple_raw(stack, PCRDR_OPERATION_INSERTAFTER,    \
            element, NULL, PCRDR_MSG_DATA_TYPE_TEXT, content)

#define pcintr_rdr_dom_displace_content(stack, element, content)              \
    pcintr_rdr_send_dom_req_simple_raw(stack, PCRDR_OPERATION_DISPLACE,       \
            element, NULL, PCRDR_MSG_DATA_TYPE_TEXT, content)

#define pcintr_rdr_dom_clear_element_content(stack, element)                  \
    pcintr_rdr_send_dom_req_simple(stack, PCRDR_OPERATION_CLEAR,              \
            element, NULL, PCRDR_MSG_DATA_TYPE_VOID, PURC_VARIANT_INVALID)

#define pcintr_rdr_dom_erase_element(stack, element)                          \
    pcintr_rdr_send_dom_req_simple(stack, PCRDR_OPERATION_ERASE,              \
            element, NULL, PCRDR_MSG_DATA_TYPE_VOID, PURC_VARIANT_INVALID)

#define pcintr_rdr_dom_erase_element_property(stack, element, prop)           \
    pcintr_rdr_send_dom_req_simple(stack, PCRDR_OPERATION_ERASE,              \
            element, prop, PCRDR_MSG_DATA_TYPE_VOID, PURC_VARIANT_INVALID)

#define pcintr_rdr_dom_update_element_content_text(stack, element, content)   \
    pcintr_rdr_send_dom_req_simple_raw(stack, PCRDR_OPERATION_UPDATE,         \
            element, NULL, PCRDR_MSG_DATA_TYPE_TEXT, content)

#define pcintr_rdr_dom_update_element_content_ejson(stack, element, data)     \
    pcintr_rdr_send_dom_req_simple(stack, PCRDR_OPERATION_UPDATE,             \
            element, NULL, PCRDR_MSG_DATA_TYPE_EJSON, data)

#define pcintr_rdr_dom_update_element_property(stack, element, prop, content) \
    pcintr_rdr_send_dom_req_simple_raw(stack, PCRDR_OPERATION_UPDATE,         \
            element, prop, PCRDR_MSG_DATA_TYPE_TEXT, content)

bool
pcintr_rdr_dom_append_child(pcintr_stack_t stack, pcdom_element_t *element,
        pcdom_node_t *child);

bool
pcintr_rdr_dom_displace_child(pcintr_stack_t stack, pcdom_element_t *element,
        pcdom_node_t *child);

purc_variant_t
pcintr_wrap_vdom(pcvdom_element_t vdom);

pcvdom_element_t
pcintr_get_vdom_from_variant(purc_variant_t val);

PCA_EXTERN_C_END

#endif  /* PURC_INTERPRETER_INTERNAL_H */

