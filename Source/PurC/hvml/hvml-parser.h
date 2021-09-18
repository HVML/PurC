/*
 * @file hvml-parser.h
 * @author Xu Xiaohong
 * @date 2021/09/01
 * @brief The interfaces for hvml token.
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

#ifndef PURC_HVML_PARSER_H
#define PURC_HVML_PARSER_H

#include "config.h"
#include "purc-macros.h"
#include "purc-errors.h"
#include "private/hvml.h"
#include "private/vdom.h"

#include "hvml-token.h"

PCA_EXTERN_C_BEGIN

enum pcvdom_construction_insertion_mode {
    PCVDOM_CONSTRUCTION_INITIAL,
    PCVDOM_CONSTRUCTION_BEFORE_HVML,
    PCVDOM_CONSTRUCTION_BEFORE_HEAD,
    PCVDOM_CONSTRUCTION_IN_HEAD,
    PCVDOM_CONSTRUCTION_AFTER_HEAD,
    PCVDOM_CONSTRUCTION_IN_BODY,
    PCVDOM_CONSTRUCTION_TEXT,
    PCVDOM_CONSTRUCTION_AFTER_BODY,
    PCVDOM_CONSTRUCTION_AFTER_AFTER_BODY,
};

struct pcvdom_construction_stack {
    struct pcvdom_document   *doc;
    struct pcvdom_node       *curr;

    enum pcvdom_construction_insertion_mode      mode;
    unsigned int              eof:1;
};

struct pcvdom_construction_stack*
pcvdom_construction_stack_create(void);

int
pcvdom_construction_stack_push_token(struct pcvdom_construction_stack *stack,
    struct pchvml_token *token);

struct pcvdom_document*
pcvdom_construction_stack_end(struct pcvdom_construction_stack *stack);

void
pcvdom_construction_stack_destroy(struct pcvdom_construction_stack *stack);


PCA_EXTERN_C_END

#endif // PURC_HVML_PARSER_H

