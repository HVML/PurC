/**
 * @file interface.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for dom interface.
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


#ifndef PCHTML_DOM_INTERFACES_H
#define PCHTML_DOM_INTERFACES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/core/base.h"

#include "html/tag/const.h"
#include "html/ns/const.h"

#include "html/dom/exception.h"


#define pchtml_dom_interface_cdata_section(obj) ((pchtml_dom_cdata_section_t *) (obj))
#define pchtml_dom_interface_character_data(obj) ((pchtml_dom_character_data_t *) (obj))
#define pchtml_dom_interface_comment(obj) ((pchtml_dom_comment_t *) (obj))
#define pchtml_dom_interface_document(obj) ((pchtml_dom_document_t *) (obj))
#define pchtml_dom_interface_document_fragment(obj) ((pchtml_dom_document_fragment_t *) (obj))
#define pchtml_dom_interface_document_type(obj) ((pchtml_dom_document_type_t *) (obj))
#define pchtml_dom_interface_element(obj) ((pchtml_dom_element_t *) (obj))
#define pchtml_dom_interface_attr(obj) ((pchtml_dom_attr_t *) (obj))
#define pchtml_dom_interface_event_target(obj) ((pchtml_dom_event_target_t *) (obj))
#define pchtml_dom_interface_node(obj) ((pchtml_dom_node_t *) (obj))
#define pchtml_dom_interface_processing_instruction(obj) ((pchtml_dom_processing_instruction_t *) (obj))
#define pchtml_dom_interface_shadow_root(obj) ((pchtml_dom_shadow_root_t *) (obj))
#define pchtml_dom_interface_text(obj) ((pchtml_dom_text_t *) (obj))


typedef struct pchtml_dom_event_target pchtml_dom_event_target_t;
typedef struct pchtml_dom_node pchtml_dom_node_t;
typedef struct pchtml_dom_element pchtml_dom_element_t;
typedef struct pchtml_dom_attr pchtml_dom_attr_t;
typedef struct pchtml_dom_document pchtml_dom_document_t;
typedef struct pchtml_dom_document_type pchtml_dom_document_type_t;
typedef struct pchtml_dom_document_fragment pchtml_dom_document_fragment_t;
typedef struct pchtml_dom_shadow_root pchtml_dom_shadow_root_t;
typedef struct pchtml_dom_character_data pchtml_dom_character_data_t;
typedef struct pchtml_dom_text pchtml_dom_text_t;
typedef struct pchtml_dom_cdata_section pchtml_dom_cdata_section_t;
typedef struct pchtml_dom_processing_instruction pchtml_dom_processing_instruction_t;
typedef struct pchtml_dom_comment pchtml_dom_comment_t;

typedef void pchtml_dom_interface_t;

typedef void *
(*pchtml_dom_interface_constructor_f)(void *document);

typedef void *
(*pchtml_dom_interface_destructor_f)(void *intrfc);


typedef pchtml_dom_interface_t *
(*pchtml_dom_interface_create_f)(pchtml_dom_document_t *document, pchtml_tag_id_t tag_id,
                              pchtml_ns_id_t ns);

typedef pchtml_dom_interface_t *
(*pchtml_dom_interface_destroy_f)(pchtml_dom_interface_t *intrfc);


pchtml_dom_interface_t *
pchtml_dom_interface_create(pchtml_dom_document_t *document, pchtml_tag_id_t tag_id,
                         pchtml_ns_id_t ns) WTF_INTERNAL;

pchtml_dom_interface_t *
pchtml_dom_interface_destroy(pchtml_dom_interface_t *intrfc) WTF_INTERNAL;


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_DOM_INTERFACES_H */
