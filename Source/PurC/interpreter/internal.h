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

#include "keywords.h"

#include <libgen.h>

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

// $<
void
pcintr_set_input_var(pcintr_stack_t stack, purc_variant_t val);

int
pcintr_set_edom_attribute(pcintr_stack_t stack, struct pcvdom_attr *attr);

purc_variant_t
pcintr_eval_vdom_attr(pcintr_stack_t stack, struct pcvdom_attr *attr);

void pcintr_coroutine_ready(void);

void
pcintr_set_base_uri(pcintr_stack_t stack, const char* base_uri);

purc_variant_t
pcintr_load_from_uri(pcintr_stack_t stack, const char* uri);

purc_variant_t
pcintr_doc_query(purc_vdom_t vdom, const char* css);



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

bool
pcintr_rdr_page_control_load(pcintr_stack_t stack);

bool
pcintr_rdr_dom_displace_content(pcintr_stack_t stack,
        pcdom_element_t *node, const char *content);

PCA_EXTERN_C_END

#endif  /* PURC_INTERPRETER_INTERNAL_H */



