/**
 * @file html.h
 * @author 
 * @date 2021/07/02
 * @brief The internal interfaces for html parser.
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
 * <https://github.com/lexbor/lexbor>, which is licensed under the Apache 
 * License Version 2.0:
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

#ifndef PURC_PRIVATE_HTML_H
#define PURC_PRIVATE_HTML_H

#include "config.h"
#include "private/edom.h"

#include <assert.h>


#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

// initialize html module (once)
void pchtml_init_once(void) WTF_INTERNAL;

struct pcinst;

// initialize the html module for a PurC instance.
void pchtml_init_instance(struct pcinst* inst) WTF_INTERNAL;
// clean up the html module for a PurC instance.
void pchtml_cleanup_instance(struct pcinst* inst) WTF_INTERNAL;

struct pchtml_html_document;
typedef struct pchtml_html_document pchtml_html_document_t;

// operations about  html document
pchtml_html_document_t *
pchtml_html_document_create(void);

void
pchtml_html_document_clean(pchtml_html_document_t *document);

pchtml_html_document_t *
pchtml_html_document_destroy(pchtml_html_document_t *document);


typedef unsigned int
(*pchtml_html_serialize_cb_f)(const unsigned char *data, size_t len, void *ctx);

unsigned int
pchtml_html_serialize_pretty_tree_cb(pcedom_node_t *node,
                int opt, size_t indent,
                pchtml_html_serialize_cb_f cb, void *ctx) ;


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* PURC_PRIVATE_HTML_H */

