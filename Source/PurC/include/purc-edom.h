/**
 * @file purc-edom.h
 * @author Vincent Wei
 * @date 2022/01/02
 * @brief The API of eDOM.
 *
 * Copyright (C) 2021, 2022 FMSoft <https://www.fmsoft.cn>
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
 * <https://github.com/lexbor/lexbor>, which is licensed under the Apache
 * License, Version 2.0:
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

#ifndef PURC_PURC_EDOM_H
#define PURC_PURC_EDOM_H

#include "purc-macros.h"
#include "purc-rwstream.h"
#include "purc-utils.h"
#include "purc-errors.h"

#define PURC_ERROR_EDOM PURC_ERROR_FIRST_EDOM

struct pcedom_node;
typedef struct pcedom_node pcedom_node_t;
struct pcedom_document;
typedef struct pcedom_document pcedom_document_t;
struct pcedom_document_fragment;
typedef struct pcedom_document_fragment pcedom_document_fragment_t;
struct pcedom_attr;
typedef struct pcedom_attr pcedom_attr_t;
struct pcedom_document_type;
typedef struct pcedom_document_type pcedom_document_type_t;
struct pcedom_element;
typedef struct pcedom_element pcedom_element_t;
struct pcedom_character_data;
typedef struct pcedom_character_data pcedom_character_data_t;
struct pcedom_processing_instruction;
typedef struct pcedom_processing_instruction pcedom_processing_instruction_t;
struct pcedom_shadow_root;
typedef struct pcedom_shadow_root pcedom_shadow_root_t;
struct pcedom_event_target;
typedef struct pcedom_event_target pcedom_event_target_t;
struct pcedom_text;
typedef struct pcedom_text pcedom_text_t;
struct pcedom_cdata_section;
typedef struct pcedom_cdata_section pcedom_cdata_section_t;
struct pcedom_comment;
typedef struct pcedom_comment pcedom_comment_t;

PCA_EXTERN_C_BEGIN

PCA_EXTERN_C_END

#endif  /* PURC_PURC_EDOM_H */

