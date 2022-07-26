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
 *
 * This implementation of HTML parser is derived from Lexbor
 * <https://github.com/lexbor/lexbor>, which is licensed under the Apache
 * License, Version 2.0:
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */


#ifndef PCHTML_HTML_SERIALIZE_H
#define PCHTML_HTML_SERIALIZE_H

#include "config.h"
#include "private/html.h"
#include "private/dom.h"

#include "private/str.h"
#include "html/base.h"

#ifdef __cplusplus
extern "C" {
#endif

unsigned int
pchtml_html_serialize_cb(pcdom_node_t *node,
                pchtml_html_serialize_cb_f cb, void *ctx) WTF_INTERNAL;

unsigned int
pchtml_html_serialize_str(pcdom_node_t *node, 
                pcutils_str_t *str) WTF_INTERNAL;

unsigned int
pchtml_html_serialize_tree_cb(pcdom_node_t *node,
                pchtml_html_serialize_cb_f cb, void *ctx) WTF_INTERNAL;

unsigned int
pchtml_html_serialize_tree_str(pcdom_node_t *node, 
                pcutils_str_t *str) WTF_INTERNAL;

unsigned int
pchtml_html_serialize_deep_cb(pcdom_node_t *node,
                pchtml_html_serialize_cb_f cb, void *ctx) WTF_INTERNAL;

unsigned int
pchtml_html_serialize_deep_str(pcdom_node_t *node, 
                pcutils_str_t *str) WTF_INTERNAL;

unsigned int
pchtml_html_serialize_pretty_cb(pcdom_node_t *node,
                pchtml_html_serialize_opt_t opt, size_t indent,
                pchtml_html_serialize_cb_f cb, void *ctx) WTF_INTERNAL;

unsigned int
pchtml_html_serialize_pretty_str(pcdom_node_t *node,
                pchtml_html_serialize_opt_t opt, size_t indent,
                pcutils_str_t *str) WTF_INTERNAL;

#if 0   //gengyue
unsigned int
pchtml_html_serialize_pretty_tree_cb(pcdom_node_t *node,
                pchtml_html_serialize_opt_t opt, size_t indent,
                pchtml_html_serialize_cb_f cb, void *ctx) WTF_INTERNAL;
#endif

unsigned int
pchtml_html_serialize_pretty_tree_str(pcdom_node_t *node,
                pchtml_html_serialize_opt_t opt, size_t indent,
                pcutils_str_t *str) WTF_INTERNAL;

unsigned int
pchtml_html_serialize_pretty_deep_cb(pcdom_node_t *node,
                pchtml_html_serialize_opt_t opt, size_t indent,
                pchtml_html_serialize_cb_f cb, void *ctx) WTF_INTERNAL;

unsigned int
pchtml_html_serialize_pretty_deep_str(pcdom_node_t *node,
                pchtml_html_serialize_opt_t opt, size_t indent,
                pcutils_str_t *str) WTF_INTERNAL;

#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_HTML_SERIALIZE_H */
