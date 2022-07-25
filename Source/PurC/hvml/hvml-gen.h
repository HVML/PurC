/*
 * @file hvml-gen.h
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

enum pcvdom_gen_insertion_mode {
    PCVDOM_GEN_INSERTION_MODE_INITIAL,
    PCVDOM_GEN_INSERTION_MODE_BEFORE_HVML,
    PCVDOM_GEN_INSERTION_MODE_IN_HVML,
    PCVDOM_GEN_INSERTION_MODE_BEFORE_HEAD,
    PCVDOM_GEN_INSERTION_MODE_IN_HEAD,
    PCVDOM_GEN_INSERTION_MODE_AFTER_HEAD,
    PCVDOM_GEN_INSERTION_MODE_IN_BODY,
    PCVDOM_GEN_INSERTION_MODE_AFTER_BODY,
    PCVDOM_GEN_INSERTION_MODE_AFTER_AFTER_BODY,
};

struct pcvdom_gen {
    enum pcvdom_gen_insertion_mode      insertion_mode;

    struct pcvdom_document   *doc;
    struct pcvdom_node       *curr;

    /* exists for tokenizer state change */
    struct pchvml_parser     *parser;

    unsigned int              eof:1;
    unsigned int              reprocess:1;
};

struct pcvdom_gen*
pcvdom_gen_create(void);

int
pcvdom_gen_push_token(struct pcvdom_gen *stack,
    struct pchvml_parser     *parser, /* exists for tokenizer state change */
    struct pchvml_token *token);

struct pcvdom_document*
pcvdom_gen_end(struct pcvdom_gen *stack);

void
pcvdom_gen_destroy(struct pcvdom_gen *stack);


PCA_EXTERN_C_END

#endif // PURC_HVML_PARSER_H

