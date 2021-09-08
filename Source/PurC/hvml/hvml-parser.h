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
#include "private/vdom.h"

#include "hvml-token.h"

PCA_EXTERN_C_BEGIN


struct pchvml_vdom_tokenizer;

struct pchvml_vdom_parser {
    struct pcvdom_document   *doc;
    struct pcvdom_node       *curr;

    struct pchvml_vdom_tokenizer  *tokenizer;
    int                            eof;
};

struct pchvml_token*
pchvml_vdom_next_token(struct pchvml_vdom_tokenizer *tokenizer,
    purc_rwstream_t in);

struct pchvml_vdom_parser*
pchvml_vdom_parser_create(struct pchvml_vdom_tokenizer *tokenizer);

int
pchvml_vdom_parser_parse(struct pchvml_vdom_parser *parser,
        purc_rwstream_t in);

int
pchvml_vdom_parser_parse_fragment(struct pchvml_vdom_parser *parser,
        struct pcvdom_node *node, purc_rwstream_t in);

int
pchvml_vdom_parser_end(struct pchvml_vdom_parser *parser);

struct pcvdom_document*
pchvml_vdom_parser_reset(struct pchvml_vdom_parser *parser);

void
pchvml_vdom_parser_destroy(struct pchvml_vdom_parser *parser);


PCA_EXTERN_C_END

#endif // PURC_HVML_PARSER_H

