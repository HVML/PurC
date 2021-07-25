/**
 * @file token_attr.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for html token attrbution.
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


#ifndef PCHTML_HTML_TOKEN_ATTR_H
#define PCHTML_HTML_TOKEN_ATTR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/core/in.h"
#include "html/core/str.h"
#include "html/core/dobject.h"

#include "html/dom/interfaces/attr.h"

#include "html/html/base.h"


typedef struct pchtml_html_token_attr pchtml_html_token_attr_t;
typedef int pchtml_html_token_attr_type_t;

enum pchtml_html_token_attr_type {
    PCHTML_HTML_TOKEN_ATTR_TYPE_UNDEF      = 0x0000,
    PCHTML_HTML_TOKEN_ATTR_TYPE_NAME_NULL  = 0x0001,
    PCHTML_HTML_TOKEN_ATTR_TYPE_VALUE_NULL = 0x0002
};

struct pchtml_html_token_attr {
    const unsigned char           *name_begin;
    const unsigned char           *name_end;

    const unsigned char           *value_begin;
    const unsigned char           *value_end;

    const pchtml_dom_attr_data_t  *name;
    unsigned char                 *value;
    size_t                     value_size;

    pchtml_in_node_t           *in_name;
    pchtml_in_node_t           *in_value;

    pchtml_html_token_attr_t      *next;
    pchtml_html_token_attr_t      *prev;

    pchtml_html_token_attr_type_t type;
};


pchtml_html_token_attr_t *
pchtml_html_token_attr_create(pchtml_dobject_t *dobj) WTF_INTERNAL;

void
pchtml_html_token_attr_clean(pchtml_html_token_attr_t *attr) WTF_INTERNAL;

pchtml_html_token_attr_t *
pchtml_html_token_attr_destroy(pchtml_html_token_attr_t *attr,
                pchtml_dobject_t *dobj) WTF_INTERNAL;

const unsigned char *
pchtml_html_token_attr_name(pchtml_html_token_attr_t *attr, 
                size_t *length) WTF_INTERNAL;


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_HTML_TOKEN_ATTR_H */
