/*
** Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of PurC (short for Purring Cat), an HVML interpreter.
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/


#ifndef PURC_TEST_HTML_OPS_H
#define PURC_TEST_HTML_OPS_H

#include "purc/purc-macros.h"
#include "purc/purc-html.h"

#include "config.h"

PCA_EXTERN_C_BEGIN

pcdom_element_t*
html_dom_append_element(pcdom_element_t* parent, const char *tag);

pcdom_text_t*
html_dom_append_content(pcdom_element_t* parent, const char *txt);

pcdom_text_t*
html_dom_displace_content(pcdom_element_t* parent, const char *txt);

int
html_dom_set_attribute(pcdom_element_t *elem,
        const char *key, const char *val);

int
html_dom_remove_attribute(pcdom_element_t *elem, const char *key);

int
html_dom_add_child_chunk(pcdom_element_t *parent, const char *chunk);

int
html_dom_set_child_chunk(pcdom_element_t *parent, const char *chunk);

WTF_ATTRIBUTE_PRINTF(2, 3)
int
html_dom_add_child(pcdom_element_t *parent, const char *fmt, ...);

WTF_ATTRIBUTE_PRINTF(2, 3)
int
html_dom_set_child(pcdom_element_t *parent, const char *fmt, ...);

pchtml_html_document_t*
html_dom_load_document(const char *html);

int
html_dom_comp_docs(pchtml_html_document_t *docl,
    pchtml_html_document_t *docr, int *diff);

bool
html_dom_is_ancestor(pcdom_node_t *ancestor, pcdom_node_t *descendant);

void
html_dom_dump_node_ex(pcdom_node_t *node,
    const char *file, int line, const char *func);

#define html_dom_dump_node(_node)        \
    html_dom_dump_node_ex(_node, __FILE__, __LINE__, __func__)

PCA_EXTERN_C_END

#endif /* PURC_TEST_HTML_OPS_H */

