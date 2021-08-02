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
 *
 * This implementation of HTML parser is derived from Lexbor
 * <https://github.com/lexbor/lexbor>, which is licensed under Apahce 2.0:
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */


#ifndef PCEDOM_PRIVATE_INTERFACES_H
#define PCEDOM_PRIVATE_INTERFACES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/core_base.h"

#include "html/tag_tag_const.h"
#include "html/ns_const.h"

#include "edom/exception.h"


#define pcedom_interface_cdata_section(obj) ((pcedom_cdata_section_t *) (obj))
#define pcedom_interface_character_data(obj) ((pcedom_character_data_t *) (obj))
#define pcedom_interface_comment(obj) ((pcedom_comment_t *) (obj))
#define pcedom_interface_document(obj) ((pcedom_document_t *) (obj))
#define pcedom_interface_document_fragment(obj) ((pcedom_document_fragment_t *) (obj))
#define pcedom_interface_document_type(obj) ((pcedom_document_type_t *) (obj))
#define pcedom_interface_element(obj) ((pcedom_element_t *) (obj))
#define pcedom_interface_attr(obj) ((pcedom_attr_t *) (obj))
#define pcedom_interface_event_target(obj) ((pcedom_event_target_t *) (obj))
#define pcedom_interface_node(obj) ((pcedom_node_t *) (obj))
#define pcedom_interface_processing_instruction(obj) ((pcedom_processing_instruction_t *) (obj))
#define pcedom_interface_shadow_root(obj) ((pcedom_shadow_root_t *) (obj))
#define pcedom_interface_text(obj) ((pcedom_text_t *) (obj))


typedef struct pcedom_event_target pcedom_event_target_t;
typedef struct pcedom_node pcedom_node_t;
typedef struct pcedom_element pcedom_element_t;
typedef struct pcedom_attr pcedom_attr_t;
typedef struct pcedom_document pcedom_document_t;
typedef struct pcedom_document_type pcedom_document_type_t;
typedef struct pcedom_document_fragment pcedom_document_fragment_t;
typedef struct pcedom_shadow_root pcedom_shadow_root_t;
typedef struct pcedom_character_data pcedom_character_data_t;
typedef struct pcedom_text pcedom_text_t;
typedef struct pcedom_cdata_section pcedom_cdata_section_t;
typedef struct pcedom_processing_instruction pcedom_processing_instruction_t;
typedef struct pcedom_comment pcedom_comment_t;

typedef void pcedom_interface_t;

typedef void *
(*pcedom_interface_constructor_f)(void *document);

typedef void *
(*pcedom_interface_destructor_f)(void *intrfc);


typedef pcedom_interface_t *
(*pcedom_interface_create_f)(pcedom_document_t *document, pchtml_tag_id_t tag_id,
                              pchtml_ns_id_t ns);

typedef pcedom_interface_t *
(*pcedom_interface_destroy_f)(pcedom_interface_t *intrfc);


pcedom_interface_t *
pcedom_interface_create(pcedom_document_t *document, pchtml_tag_id_t tag_id,
                         pchtml_ns_id_t ns) WTF_INTERNAL;

pcedom_interface_t *
pcedom_interface_destroy(pcedom_interface_t *intrfc) WTF_INTERNAL;


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCEDOM_PRIVATE_INTERFACES_H */
