/**
 * @file serialize.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for serialization.
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


#ifndef PCHTML_HTML_SERIALIZE_H
#define PCHTML_HTML_SERIALIZE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/core/str.h"
#include "private/html.h"
#include "private/edom/element.h"

#include "html/html/base.h"


typedef int pchtml_html_serialize_opt_t;

enum pchtml_html_serialize_opt {
    PCHTML_HTML_SERIALIZE_OPT_UNDEF               = 0x00,
    PCHTML_HTML_SERIALIZE_OPT_SKIP_WS_NODES       = 0x01,
    PCHTML_HTML_SERIALIZE_OPT_SKIP_COMMENT        = 0x02,
    PCHTML_HTML_SERIALIZE_OPT_RAW                 = 0x04,
    PCHTML_HTML_SERIALIZE_OPT_WITHOUT_CLOSING     = 0x08,
    PCHTML_HTML_SERIALIZE_OPT_TAG_WITH_NS         = 0x10,
    PCHTML_HTML_SERIALIZE_OPT_WITHOUT_TEXT_INDENT = 0x20,
    PCHTML_HTML_SERIALIZE_OPT_FULL_DOCTYPE        = 0x40
};


#if 0  // gengyue
typedef unsigned int
(*pchtml_html_serialize_cb_f)(const unsigned char *data, size_t len, void *ctx);
#endif

unsigned int
pchtml_html_serialize_cb(pcedom_node_t *node,
                pchtml_html_serialize_cb_f cb, void *ctx) WTF_INTERNAL;

unsigned int
pchtml_html_serialize_str(pcedom_node_t *node, 
                pchtml_str_t *str) WTF_INTERNAL;

unsigned int
pchtml_html_serialize_tree_cb(pcedom_node_t *node,
                pchtml_html_serialize_cb_f cb, void *ctx) WTF_INTERNAL;

unsigned int
pchtml_html_serialize_tree_str(pcedom_node_t *node, 
                pchtml_str_t *str) WTF_INTERNAL;

unsigned int
pchtml_html_serialize_deep_cb(pcedom_node_t *node,
                pchtml_html_serialize_cb_f cb, void *ctx) WTF_INTERNAL;

unsigned int
pchtml_html_serialize_deep_str(pcedom_node_t *node, 
                pchtml_str_t *str) WTF_INTERNAL;

unsigned int
pchtml_html_serialize_pretty_cb(pcedom_node_t *node,
                pchtml_html_serialize_opt_t opt, size_t indent,
                pchtml_html_serialize_cb_f cb, void *ctx) WTF_INTERNAL;

unsigned int
pchtml_html_serialize_pretty_str(pcedom_node_t *node,
                pchtml_html_serialize_opt_t opt, size_t indent,
                pchtml_str_t *str) WTF_INTERNAL;

#if 0   //gengyue
unsigned int
pchtml_html_serialize_pretty_tree_cb(pcedom_node_t *node,
                pchtml_html_serialize_opt_t opt, size_t indent,
                pchtml_html_serialize_cb_f cb, void *ctx) WTF_INTERNAL;
#endif

unsigned int
pchtml_html_serialize_pretty_tree_str(pcedom_node_t *node,
                pchtml_html_serialize_opt_t opt, size_t indent,
                pchtml_str_t *str) WTF_INTERNAL;

unsigned int
pchtml_html_serialize_pretty_deep_cb(pcedom_node_t *node,
                pchtml_html_serialize_opt_t opt, size_t indent,
                pchtml_html_serialize_cb_f cb, void *ctx) WTF_INTERNAL;

unsigned int
pchtml_html_serialize_pretty_deep_str(pcedom_node_t *node,
                pchtml_html_serialize_opt_t opt, size_t indent,
                pchtml_str_t *str) WTF_INTERNAL;

#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_HTML_SERIALIZE_H */
