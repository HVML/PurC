/**
 * @file serialize.c
 * @author
 * @date 2021/07/02
 * @brief The complementation of serializing html node.
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

#include "purc.h"
#include "config.h"
#include "private/instance.h"
#include "private/errors.h"
#include "private/dom.h"

#include "html/serialize.h"
#include "html/tree.h"
#include "html/interfaces/template_element.h"

#define PCHTML_TOKENIZER_CHARS_MAP
#include "str_res.h"

#define html_serialize_send(data, len, ctx)                                \
    do {                                                                       \
        status = cb((const unsigned char *) data, len, ctx);                      \
        if (status != PCHTML_STATUS_OK) {                                         \
            return status;                                                     \
        }                                                                      \
    }                                                                          \
    while (0)

#define html_serialize_send_indent(count, ctx)                             \
    do {                                                                       \
        for (size_t i = 0; i < count; i++) {                                   \
            html_serialize_send("  ", 2, ctx);                             \
        }                                                                      \
    }                                                                          \
    while (0)


typedef struct {
    pcutils_str_t  *str;
    pcutils_mraw_t *mraw;
}
pchtml_html_serialize_ctx_t;

static unsigned int
html_serialize_str_callback(const unsigned char *data, size_t len, void *ctx);

static unsigned int
html_serialize_node_cb(pcdom_node_t *node,
        pchtml_html_serialize_cb_f cb, void *ctx);

static unsigned int
html_serialize_element_cb(pcdom_element_t *element,
        pchtml_html_serialize_cb_f cb, void *ctx);

static unsigned int
html_serialize_element_closed_cb(pcdom_element_t *element,
        pchtml_html_serialize_cb_f cb, void *ctx);

static unsigned int
html_serialize_text_cb(pcdom_text_t *text,
        pchtml_html_serialize_cb_f cb, void *ctx);

static unsigned int
html_serialize_comment_cb(pcdom_comment_t *comment,
        pchtml_html_serialize_cb_f cb, void *ctx);

static unsigned int
html_serialize_processing_instruction_cb(pcdom_processing_instruction_t *pi,
        pchtml_html_serialize_cb_f cb, void *ctx);

static unsigned int
html_serialize_document_type_cb(pcdom_document_type_t *doctype,
        pchtml_html_serialize_cb_f cb, void *ctx);

static unsigned int
html_serialize_document_type_full_cb(pcdom_document_type_t *doctype,
        pchtml_html_serialize_cb_f cb, void *ctx);

static unsigned int
html_serialize_document_cb(pcdom_document_t *document,
        pchtml_html_serialize_cb_f cb, void *ctx);

static unsigned int
html_serialize_send_escaping_attribute_string(const unsigned char *data,
        size_t len, pchtml_html_serialize_cb_f cb, void *ctx);

static unsigned int
html_serialize_send_escaping_string(const unsigned char *data, size_t len,
        pchtml_html_serialize_cb_f cb, void *ctx);

static unsigned int
html_serialize_attribute_cb(pcdom_attr_t *attr, bool has_raw,
        pchtml_html_serialize_cb_f cb, void *ctx);

static unsigned int
html_serialize_pretty_node_cb(pcdom_node_t *node,
        pchtml_html_serialize_opt_t opt, size_t deep,
        pchtml_html_serialize_cb_f cb, void *ctx);

static unsigned int
html_serialize_pretty_element_cb(pcdom_element_t *element,
        pchtml_html_serialize_opt_t opt, size_t indent,
        pchtml_html_serialize_cb_f cb, void *ctx);

static unsigned int
html_serialize_pretty_text_cb(pcdom_text_t *text,
        pchtml_html_serialize_opt_t opt, size_t indent,
        pchtml_html_serialize_cb_f cb, void *ctx);

static unsigned int
html_serialize_pretty_comment_cb(pcdom_comment_t *comment,
        pchtml_html_serialize_opt_t opt,
        size_t indent, bool with_indent,
        pchtml_html_serialize_cb_f cb, void *ctx);

static unsigned int
html_serialize_pretty_document_cb(pcdom_document_t *document,
        pchtml_html_serialize_cb_f cb, void *ctx);

static unsigned int
html_serialize_pretty_send_escaping_string(const unsigned char *data, size_t len,
        pchtml_html_serialize_opt_t opt,
        size_t indent, bool with_indent,
        pchtml_html_serialize_cb_f cb, void *ctx);

static unsigned int
html_serialize_pretty_send_string(const unsigned char *data, size_t len,
        size_t indent, bool with_indent,
        pchtml_html_serialize_cb_f cb, void *ctx);


unsigned int
pchtml_html_serialize_cb(pcdom_node_t *node,
                      pchtml_html_serialize_cb_f cb, void *ctx)
{
    switch (node->type) {
        case PCDOM_NODE_TYPE_ELEMENT:
            return html_serialize_element_cb(
                    pcdom_interface_element(node), cb, ctx);

        case PCDOM_NODE_TYPE_TEXT:
            return html_serialize_text_cb(
                    pcdom_interface_text(node), cb, ctx);

        case PCDOM_NODE_TYPE_COMMENT:
            return html_serialize_comment_cb(
                    pcdom_interface_comment(node), cb, ctx);

        case PCDOM_NODE_TYPE_PROCESSING_INSTRUCTION:
            return html_serialize_processing_instruction_cb(
                    pcdom_interface_processing_instruction(node), cb, ctx);

        case PCDOM_NODE_TYPE_DOCUMENT_TYPE:
            return html_serialize_document_type_cb(
                    pcdom_interface_document_type(node), cb, ctx);

        case PCDOM_NODE_TYPE_DOCUMENT:
            return html_serialize_document_cb(
                    pcdom_interface_document(node), cb, ctx);

        default:
            break;
    }
    pcinst_set_error (PURC_ERROR_HTML);
    return PCHTML_STATUS_ERROR;
}

unsigned int
pchtml_html_serialize_str(pcdom_node_t *node, pcutils_str_t *str)
{
    pchtml_html_serialize_ctx_t ctx;

    if (str->data == NULL) {
        pcutils_str_init(str, node->owner_document->text, 1024);

        if (str->data == NULL) {
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }
    }

    ctx.str = str;
    ctx.mraw = node->owner_document->text;

    return pchtml_html_serialize_cb(node, html_serialize_str_callback, &ctx);
}

static unsigned int
html_serialize_str_callback(const unsigned char *data, size_t len, void *ctx)
{
    unsigned char *ret;
    pchtml_html_serialize_ctx_t *s_ctx = ctx;

    ret = pcutils_str_append(s_ctx->str, s_ctx->mraw, data, len);
    if (ret == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_html_serialize_deep_cb(pcdom_node_t *node,
                           pchtml_html_serialize_cb_f cb, void *ctx)
{
    unsigned int status;

    node = node->first_child;

    while (node != NULL) {
        status = html_serialize_node_cb(node, cb, ctx);
        if (status != PCHTML_STATUS_OK) {
            return status;
        }

        node = node->next;
    }

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_html_serialize_deep_str(pcdom_node_t *node, pcutils_str_t *str)
{
    pchtml_html_serialize_ctx_t ctx;

    if (str->data == NULL) {
        pcutils_str_init(str, node->owner_document->text, 1024);

        if (str->data == NULL) {
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }
    }

    ctx.str = str;
    ctx.mraw = node->owner_document->text;

    return pchtml_html_serialize_deep_cb(node,
                                      html_serialize_str_callback, &ctx);
}

static unsigned int
html_serialize_node_cb(pcdom_node_t *node,
                           pchtml_html_serialize_cb_f cb, void *ctx)
{
    unsigned int status;
    pcdom_node_t *root = node;

    while (node != NULL) {
        status = pchtml_html_serialize_cb(node, cb, ctx);
        if (status != PCHTML_STATUS_OK) {
            return status;
        }

        if (node->local_name == PCHTML_TAG_TEMPLATE) {
            pchtml_html_template_element_t *temp;

            temp = pchtml_html_interface_template(node);

            if (temp->content != NULL) {
                if (temp->content->node.first_child != NULL)
                {
                    status = pchtml_html_serialize_deep_cb(&temp->content->node,
                                                        cb, ctx);
                    if (status != PCHTML_STATUS_OK) {
                        return status;
                    }
                }
            }
        }

        if (!pchtml_html_node_is_void(node) && node->first_child != NULL) {
            node = node->first_child;
        }
        else {
            while(node != root && node->next == NULL)
            {
                if (node->type == PCDOM_NODE_TYPE_ELEMENT
                    && pchtml_html_node_is_void(node) == false)
                {
                    status = html_serialize_element_closed_cb(
                            pcdom_interface_element(node), cb, ctx);
                    if (status != PCHTML_STATUS_OK) {
                        return status;
                    }
                }

                node = node->parent;
            }

            if (node->type == PCDOM_NODE_TYPE_ELEMENT
                && pchtml_html_node_is_void(node) == false)
            {
                status = html_serialize_element_closed_cb(
                        pcdom_interface_element(node), cb, ctx);
                if (status != PCHTML_STATUS_OK) {
                    return status;
                }
            }

            if (node == root) {
                break;
            }

            node = node->next;
        }
    }

    return PCHTML_STATUS_OK;
}

static inline bool node_is_self_close(pcdom_node_t *node)
{
    pcdom_element_t *element = pcdom_interface_element(node);
    return pchtml_html_node_is_void(node) ||
            (element->self_close && node->first_child == NULL);
}

static unsigned int
html_serialize_element_cb(pcdom_element_t *element,
                              pchtml_html_serialize_cb_f cb, void *ctx)
{
    unsigned int status;
    const unsigned char *tag_name;
    size_t len = 0;

    pcdom_attr_t *attr;

    tag_name = pcdom_element_qualified_name(element, &len);
    if (tag_name == NULL) {
        pcinst_set_error (PURC_ERROR_HTML);
        return PCHTML_STATUS_ERROR;
    }

    html_serialize_send("<", 1, ctx);
    html_serialize_send(tag_name, len, ctx);

    if (element->is_value != NULL && element->is_value->data != NULL) {
        attr = pcdom_element_attr_is_exist(element,
                                             (const unsigned char *) "is", 2);
        if (attr == NULL) {
            html_serialize_send(" is=\"", 5, ctx);

            status = html_serialize_send_escaping_attribute_string(
                    element->is_value->data,
                    element->is_value->length, cb, ctx);
            if (status != PCHTML_STATUS_OK) {
                return status;
            }

            html_serialize_send("\"", 1, ctx);
        }
    }

    attr = element->first_attr;

    while (attr != NULL) {
        html_serialize_send(" ", 1, ctx);

        status = html_serialize_attribute_cb(attr, false, cb, ctx);
        if (status != PCHTML_STATUS_OK) {
            return status;
        }

        attr = attr->next;
    }

    if (node_is_self_close(pcdom_interface_node(element))) {
        html_serialize_send("/>", 2, ctx);
    }
    else {
        html_serialize_send("/", 1, ctx);
    }

    return PCHTML_STATUS_OK;
}

static unsigned int
html_serialize_element_closed_cb(pcdom_element_t *element,
                                     pchtml_html_serialize_cb_f cb, void *ctx)
{
    unsigned int status;
    const unsigned char *tag_name;
    size_t len = 0;

    pcdom_node_t *node = pcdom_interface_node(element);
    bool is_void = pchtml_html_node_is_void(node);
    bool has_children = node->first_child != NULL;
    if (is_void || (element->self_close && !has_children)) {
        return PCHTML_STATUS_OK;
    }

    tag_name = pcdom_element_qualified_name(element, &len);
    if (tag_name == NULL) {
        pcinst_set_error (PURC_ERROR_HTML);
        return PCHTML_STATUS_ERROR;
    }

    html_serialize_send("</", 2, ctx);
    html_serialize_send(tag_name, len, ctx);
    html_serialize_send(">", 1, ctx);

    return PCHTML_STATUS_OK;
}

static unsigned int
html_serialize_text_cb(pcdom_text_t *text,
                           pchtml_html_serialize_cb_f cb, void *ctx)
{
    unsigned int status;

    pcdom_node_t *node = pcdom_interface_node(text);
    pcdom_document_t *doc = node->owner_document;
    pcutils_str_t *data = &text->char_data.data;

    switch (node->parent->local_name) {
        case PCHTML_TAG_STYLE:
        case PCHTML_TAG_SCRIPT:
        case PCHTML_TAG_XMP:
        case PCHTML_TAG_IFRAME:
        case PCHTML_TAG_NOEMBED:
        case PCHTML_TAG_NOFRAMES:
        case PCHTML_TAG_PLAINTEXT:
            html_serialize_send(data->data, data->length, ctx);

            return PCHTML_STATUS_OK;

        case PCHTML_TAG_NOSCRIPT:
            if (doc->scripting) {
                html_serialize_send(data->data, data->length, ctx);

                return PCHTML_STATUS_OK;
            }

            break;

        default:
            break;
    }

    return html_serialize_send_escaping_string(data->data, data->length,
                                                   cb, ctx);
}

static unsigned int
html_serialize_comment_cb(pcdom_comment_t *comment,
                              pchtml_html_serialize_cb_f cb, void *ctx)
{
    unsigned int status;
    pcutils_str_t *data = &comment->char_data.data;

    html_serialize_send("<!--", 4, ctx);
    html_serialize_send(data->data, data->length, ctx);
    html_serialize_send("-->", 3, ctx);

    return PCHTML_STATUS_OK;
}

static unsigned int
html_serialize_processing_instruction_cb(
        pcdom_processing_instruction_t *pi,
        pchtml_html_serialize_cb_f cb, void *ctx)
{
    unsigned int status;
    pcutils_str_t *data = &pi->char_data.data;

    html_serialize_send("<?", 2, ctx);
    html_serialize_send(pi->target.data, pi->target.length, ctx);
    html_serialize_send(" ", 1, ctx);
    html_serialize_send(data->data, data->length, ctx);
    html_serialize_send(">", 1, ctx);

    return PCHTML_STATUS_OK;
}

static unsigned int
html_serialize_document_type_cb(pcdom_document_type_t *doctype,
        pchtml_html_serialize_cb_f cb, void *ctx)
{
    size_t length;
    const unsigned char *name;
    unsigned int status;

    html_serialize_send("<!DOCTYPE", 9, ctx);
    html_serialize_send(" ", 1, ctx);

    name = pcdom_document_type_name(doctype, &length);

    if (length != 0) {
        html_serialize_send(name, length, ctx);
    }

    html_serialize_send(">", 1, ctx);

    return PCHTML_STATUS_OK;
}

static unsigned int
html_serialize_document_type_full_cb(pcdom_document_type_t *doctype,
        pchtml_html_serialize_cb_f cb, void *ctx)
{
    size_t length;
    const unsigned char *name;
    unsigned int status;

    html_serialize_send("<!DOCTYPE", 9, ctx);
    html_serialize_send(" ", 1, ctx);

    name = pcdom_document_type_name(doctype, &length);

    if (length != 0) {
        html_serialize_send(name, length, ctx);
    }

    if (doctype->public_id.data != NULL && doctype->public_id.length != 0) {
        html_serialize_send(" PUBLIC ", 8, ctx);
        html_serialize_send("\"", 1, ctx);

        html_serialize_send(doctype->public_id.data,
                                doctype->public_id.length, ctx);

        html_serialize_send("\"", 1, ctx);
    }

    if (doctype->system_id.data != NULL && doctype->system_id.length != 0) {
        if (doctype->public_id.length == 0) {
            html_serialize_send(" SYSTEM", 7, ctx);
        }

        html_serialize_send(" \"", 2, ctx);

        html_serialize_send(doctype->system_id.data,
                                doctype->system_id.length, ctx);

        html_serialize_send("\"", 1, ctx);
    }

    html_serialize_send(">", 1, ctx);

    return PCHTML_STATUS_OK;
}

static unsigned int
html_serialize_document_cb(pcdom_document_t *document,
                               pchtml_html_serialize_cb_f cb, void *ctx)
{
    UNUSED_PARAM(document);

    unsigned int status;

    html_serialize_send("<#document>", 11, ctx);

    return PCHTML_STATUS_OK;
}

static unsigned int
html_serialize_send_escaping_attribute_string(
        const unsigned char *data, size_t len,
        pchtml_html_serialize_cb_f cb, void *ctx)
{
    unsigned int status;
    const unsigned char *pos = data;
    const unsigned char *end = data + len;

    while (data != end) {
        switch (*data) {
            /* U+0026 AMPERSAND (&) */
            case 0x26:
                if (pos != data) {
                    html_serialize_send(pos, (data - pos), ctx);
                }

                html_serialize_send("&amp;", 5, ctx);

                data++;
                pos = data;

                break;

            /* {0xC2, 0xA0} NO-BREAK SPACE */
            case 0xC2:
                data += 1;
                if (data == end) {
                    break;
                }

                if (*data != 0xA0) {
                    continue;
                }

                data -= 1;

                if (pos != data) {
                    html_serialize_send(pos, (data - pos), ctx);
                }

                html_serialize_send("&nbsp;", 6, ctx);

                data += 2;
                pos = data;

                break;

            /* U+0022 QUOTATION MARK (") */
            case 0x22:
                if (pos != data) {
                    html_serialize_send(pos, (data - pos), ctx);
                }

                html_serialize_send("&quot;", 6, ctx);

                data++;
                pos = data;

                break;

            /* U+0027 double quotation marks (') */
            case 0x27:
                if (pos != data) {
                    html_serialize_send(pos, (data - pos), ctx);
                }

                html_serialize_send("&#039;", 6, ctx);

                data++;
                pos = data;

                break;

            /* U+003C LESS-THAN SIGN (<) */
            case 0x3C:
                if (pos != data) {
                    html_serialize_send(pos, (data - pos), ctx);
                }

                html_serialize_send("&lt;", 4, ctx);

                data++;
                pos = data;

                break;

            /* U+003E GREATER-THAN SIGN (>) */
            case 0x3E:
                if (pos != data) {
                    html_serialize_send(pos, (data - pos), ctx);
                }

                html_serialize_send("&gt;", 4, ctx);

                data++;
                pos = data;

                break;

            default:
                data++;

                break;
        }
    }

    if (pos != data) {
        html_serialize_send(pos, (data - pos), ctx);
    }

    return PCHTML_STATUS_OK;
}

static unsigned int
html_serialize_send_escaping_string( const unsigned char *data,
        size_t len, pchtml_html_serialize_cb_f cb, void *ctx)
{
    unsigned int status;
    const unsigned char *pos = data;
    const unsigned char *end = data + len;

    while (data != end) {
        switch (*data) {
            /* U+0026 AMPERSAND (&) */
            case 0x26:
                if (pos != data) {
                    html_serialize_send(pos, (data - pos), ctx);
                }

                html_serialize_send("&amp;", 5, ctx);

                data++;
                pos = data;

                break;

            /* {0xC2, 0xA0} NO-BREAK SPACE */
            case 0xC2:
                data += 1;
                if (data == end) {
                    break;
                }

                if (*data != 0xA0) {
                    continue;
                }

                data -= 1;

                if (pos != data) {
                    html_serialize_send(pos, (data - pos), ctx);
                }

                html_serialize_send("&nbsp;", 6, ctx);

                data += 2;
                pos = data;

                break;

            /* U+003C LESS-THAN SIGN (<) */
            case 0x3C:
                if (pos != data) {
                    html_serialize_send(pos, (data - pos), ctx);
                }

                html_serialize_send("&lt;", 4, ctx);

                data++;
                pos = data;

                break;

            /* U+003E GREATER-THAN SIGN (>) */
            case 0x3E:
                if (pos != data) {
                    html_serialize_send(pos, (data - pos), ctx);
                }

                html_serialize_send("&gt;", 4, ctx);

                data++;
                pos = data;

                break;

            default:
                data++;

                break;
        }
    }

    if (pos != data) {
        html_serialize_send(pos, (data - pos), ctx);
    }

    return PCHTML_STATUS_OK;
}

static unsigned int
html_serialize_attribute_cb(pcdom_attr_t *attr, bool has_raw,
                                pchtml_html_serialize_cb_f cb, void *ctx)
{
    size_t length;
    unsigned int status;
    const unsigned char *str;
    const pcdom_attr_data_t *data;

    data = pcdom_attr_data_by_id(attr->node.owner_document->attrs,
                                   attr->node.local_name);
    if (data == NULL) {
        pcinst_set_error (PURC_ERROR_HTML);
        return PCHTML_STATUS_ERROR;
    }

    if (attr->node.ns == PCHTML_NS__UNDEF) {
        html_serialize_send(pcutils_hash_entry_str(&data->entry),
                                data->entry.length, ctx);
        goto value;
    }

    if (attr->node.ns == PCHTML_NS_XML) {
        html_serialize_send((const unsigned char *) "xml:", 4, ctx);
        html_serialize_send(pcutils_hash_entry_str(&data->entry),
                                data->entry.length, ctx);

        goto value;
    }

    if (attr->node.ns == PCHTML_NS_XMLNS)
    {
        if (data->entry.length == 5
            && pcutils_str_data_cmp(pcutils_hash_entry_str(&data->entry),
                                   (const unsigned char *) "xmlns"))
        {
            html_serialize_send((const unsigned char *) "xmlns", 5, ctx);
        }
        else {
            html_serialize_send((const unsigned char *) "xmlns:", 5, ctx);
            html_serialize_send(pcutils_hash_entry_str(&data->entry),
                                    data->entry.length, ctx);
        }

        goto value;
    }

    if (attr->node.ns == PCHTML_NS_XLINK) {
        html_serialize_send((const unsigned char *) "xlink:", 6, ctx);
        html_serialize_send(pcutils_hash_entry_str(&data->entry),
                                data->entry.length, ctx);

        goto value;
    }

    str = pcdom_attr_qualified_name(attr, &length);
    if (str == NULL) {
        pcinst_set_error (PURC_ERROR_HTML);
        return PCHTML_STATUS_ERROR;
    }

    html_serialize_send(str, length, ctx);

value:

    if (attr->value == NULL) {
        return PCHTML_STATUS_OK;
    }

    html_serialize_send("=\"", 2, ctx);

    if (has_raw) {
        html_serialize_send(attr->value->data, attr->value->length, ctx);
    }
    else {
        status = html_serialize_send_escaping_attribute_string(
                attr->value->data, attr->value->length, cb, ctx);
        if (status != PCHTML_STATUS_OK) {
            return status;
        }
    }

    html_serialize_send("\"", 1, ctx);

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_html_serialize_pretty_cb(pcdom_node_t *node,
                             pchtml_html_serialize_opt_t opt, size_t indent,
                             pchtml_html_serialize_cb_f cb, void *ctx)
{
    unsigned int status;

    switch (node->type) {
        case PCDOM_NODE_TYPE_ELEMENT:
            if ((opt & PCHTML_HTML_SERIALIZE_OPT_SKIP_WS_NODES)==0) {
                html_serialize_send("\n", 1, ctx);
            }

            if ((opt & PCHTML_HTML_SERIALIZE_OPT_WITHOUT_TEXT_INDENT)==0) {
                html_serialize_send_indent(indent, ctx);
            }

            status = html_serialize_pretty_element_cb(
                    pcdom_interface_element(node), opt, indent, cb, ctx);
            break;

        case PCDOM_NODE_TYPE_TEXT:
            switch (node->parent->local_name) {
                case PCHTML_TAG_STYLE:
                case PCHTML_TAG_SCRIPT:
                case PCHTML_TAG_XMP:
                case PCHTML_TAG_IFRAME:
                case PCHTML_TAG_NOEMBED:
                case PCHTML_TAG_NOFRAMES:
                case PCHTML_TAG_PLAINTEXT:
                case PCHTML_TAG_NOSCRIPT:
                    if ((opt & PCHTML_HTML_SERIALIZE_OPT_SKIP_WS_NODES)==0) {
                        html_serialize_send("\n", 1, ctx);
                    }
                    break;

                default:
                    if ((opt & PCHTML_HTML_SERIALIZE_OPT_RAW) == 0)
                        indent = 0;
                    break;
            }

            return html_serialize_pretty_text_cb(
                    pcdom_interface_text(node), opt, indent, cb, ctx);

        case PCDOM_NODE_TYPE_COMMENT: {
            bool with_indent;

            if (opt & PCHTML_HTML_SERIALIZE_OPT_SKIP_COMMENT) {
                return PCHTML_STATUS_OK;
            }

            if ((opt & PCHTML_HTML_SERIALIZE_OPT_SKIP_WS_NODES)==0) {
                html_serialize_send("\n", 1, ctx);
            }

            with_indent = (opt & PCHTML_HTML_SERIALIZE_OPT_WITHOUT_TEXT_INDENT) == 0;
            status = html_serialize_pretty_comment_cb(
                    pcdom_interface_comment(node), opt, indent, with_indent,
                    cb, ctx);

            break;
        }

        case PCDOM_NODE_TYPE_PROCESSING_INSTRUCTION:
            if ((opt & PCHTML_HTML_SERIALIZE_OPT_SKIP_WS_NODES)==0) {
                html_serialize_send("\n", 1, ctx);
            }

            html_serialize_send_indent(indent, ctx);

            status = html_serialize_processing_instruction_cb(
                    pcdom_interface_processing_instruction(node), cb, ctx);
            break;

        case PCDOM_NODE_TYPE_DOCUMENT_TYPE:
            html_serialize_send_indent(indent, ctx);

            if (opt & PCHTML_HTML_SERIALIZE_OPT_FULL_DOCTYPE) {
                status = html_serialize_document_type_full_cb(
                        pcdom_interface_document_type(node), cb, ctx);
            }
            else {
                status = html_serialize_document_type_cb(
                        pcdom_interface_document_type(node), cb, ctx);
            }
            break;

        case PCDOM_NODE_TYPE_DOCUMENT:
            if ((opt & PCHTML_HTML_SERIALIZE_OPT_SKIP_WS_NODES)==0) {
                html_serialize_send("\n", 1, ctx);
            }

            html_serialize_send_indent(indent, ctx);

            status = html_serialize_pretty_document_cb(
                    pcdom_interface_document(node), cb, ctx);
            break;

        default:
            PC_DEBUGX("type: 0x%x", node->type);
            pcinst_set_error (PURC_ERROR_HTML);
            PC_ASSERT(0);
            return PCHTML_STATUS_ERROR;
    }

    if (status != PCHTML_STATUS_OK) {
        PC_ASSERT(0);
        return status;
    }

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_html_serialize_pretty_str(pcdom_node_t *node,
                              pchtml_html_serialize_opt_t opt, size_t indent,
                              pcutils_str_t *str)
{
    pchtml_html_serialize_ctx_t ctx;

    if (str->data == NULL) {
        pcutils_str_init(str, node->owner_document->text, 1024);

        if (str->data == NULL) {
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }
    }

    ctx.str = str;
    ctx.mraw = node->owner_document->text;

    return pchtml_html_serialize_pretty_cb(node, opt, indent,
            html_serialize_str_callback, &ctx);
}

unsigned int
pchtml_html_serialize_pretty_deep_cb(pcdom_node_t *node,
        pchtml_html_serialize_opt_t opt, size_t indent,
        pchtml_html_serialize_cb_f cb, void *ctx)
{
    unsigned int status;

    node = node->first_child;

    while (node != NULL) {
        status = html_serialize_pretty_node_cb( node, opt, indent,
                cb, ctx);
        if (status != PCHTML_STATUS_OK) {
            return status;
        }

        node = node->next;
    }

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_html_serialize_pretty_deep_str(pcdom_node_t *node,
        pchtml_html_serialize_opt_t opt, size_t indent, pcutils_str_t *str)
{
    pchtml_html_serialize_ctx_t ctx;

    if (str->data == NULL) {
        pcutils_str_init(str, node->owner_document->text, 1024);

        if (str->data == NULL) {
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }
    }

    ctx.str = str;
    ctx.mraw = node->owner_document->text;

    return pchtml_html_serialize_pretty_deep_cb(node, opt, indent,
                                             html_serialize_str_callback,
                                             &ctx);
}

static unsigned int
html_serialize_pretty_node_cb(pcdom_node_t *node,
        pchtml_html_serialize_opt_t opt, size_t deep,
        pchtml_html_serialize_cb_f cb, void *ctx)
{
    unsigned int status;
    pcdom_node_t *root = node;

    while (node != NULL) {
        status = pchtml_html_serialize_pretty_cb(node, opt, deep, cb, ctx);
        if (status != PCHTML_STATUS_OK) {
            PC_ASSERT(0);
            return status;
        }

        if (pchtml_html_tree_node_is(node, PCHTML_TAG_TEMPLATE)) {
            pchtml_html_template_element_t *temp;

            temp = pchtml_html_interface_template(node);

            if (temp->content != NULL) {
                if (temp->content->node.first_child != NULL) {
                    html_serialize_send_indent((deep + 1), ctx);
                    html_serialize_send("#document-fragment", 18, ctx);
                    if ((opt & PCHTML_HTML_SERIALIZE_OPT_SKIP_WS_NODES)==0) {
                        html_serialize_send("\n", 1, ctx);
                    }

                    status = pchtml_html_serialize_pretty_deep_cb(
                            &temp->content->node, opt, (deep + 2), cb, ctx);
                    if (status != PCHTML_STATUS_OK) {
                        PC_ASSERT(0);
                        return status;
                    }
                }
            }
        }

        if (!pchtml_html_node_is_void(node) && node->first_child != NULL) {
            deep++;

            node = node->first_child;
        }
        else {
            while (node != root && node->next == NULL) {
                if (node->type == PCDOM_NODE_TYPE_ELEMENT &&
                        !node_is_self_close(node)) {

                    // VW: add newline if last child is a void element
                    if (node->last_child &&
                            node->last_child->type == PCDOM_NODE_TYPE_ELEMENT &&
                            node_is_self_close(node->last_child) &&
                            (opt & PCHTML_HTML_SERIALIZE_OPT_SKIP_WS_NODES) == 0) {
                        html_serialize_send("\n", 1, ctx);
                    }

                    if ((opt & PCHTML_HTML_SERIALIZE_OPT_WITHOUT_CLOSING) == 0) {
                        if (node->last_child &&
                                node->last_child->type != PCDOM_NODE_TYPE_TEXT &&
                                (opt & PCHTML_HTML_SERIALIZE_OPT_WITHOUT_TEXT_INDENT) == 0) {
                            html_serialize_send_indent(deep, ctx);
                        }

                        status = html_serialize_element_closed_cb(
                                pcdom_interface_element(node), cb, ctx);
                        if (status != PCHTML_STATUS_OK) {
                            PC_ASSERT(0);
                            return status;
                        }

                        if ((opt & PCHTML_HTML_SERIALIZE_OPT_SKIP_WS_NODES) == 0) {
                            html_serialize_send("\n", 1, ctx);
                        }
                    }
                }

                deep--;

                node = node->parent;
            }

            if (node->type == PCDOM_NODE_TYPE_ELEMENT &&
                    !node_is_self_close(node)) {
                if ((opt & PCHTML_HTML_SERIALIZE_OPT_WITHOUT_CLOSING) == 0) {

                    // VW: add newline if last child is a void element
                    if (node->last_child &&
                            node->last_child->type == PCDOM_NODE_TYPE_ELEMENT &&
                            node_is_self_close(node->last_child) &&
                            (opt & PCHTML_HTML_SERIALIZE_OPT_SKIP_WS_NODES) == 0) {
                        html_serialize_send("\n", 1, ctx);
                    }

                    if (node->last_child &&
                            node->last_child->type != PCDOM_NODE_TYPE_TEXT &&
                            (opt & PCHTML_HTML_SERIALIZE_OPT_WITHOUT_TEXT_INDENT) == 0) {
                        html_serialize_send_indent(deep, ctx);
                    }

                    status = html_serialize_element_closed_cb(
                            pcdom_interface_element(node), cb, ctx);
                    if (status != PCHTML_STATUS_OK) {
                        PC_ASSERT(0);
                        return status;
                    }

                    if (node->next == NULL &&
                            (opt & PCHTML_HTML_SERIALIZE_OPT_SKIP_WS_NODES)==0) {
                        html_serialize_send("\n", 1, ctx);
                    }
                }
            }

            if (node == root) {
                break;
            }

            node = node->next;
        }
    }

    return PCHTML_STATUS_OK;
}

#define LEN_BUFF_LONGLONGINT    128

static unsigned int
html_serialize_pretty_element_cb(pcdom_element_t *element,
        pchtml_html_serialize_opt_t opt, size_t indent,
        pchtml_html_serialize_cb_f cb, void *ctx)
{
    UNUSED_PARAM(indent);

    unsigned int status;
    const unsigned char *tag_name;
    size_t len = 0;

    pcdom_attr_t *attr;
    pcdom_node_t *node = pcdom_interface_node(element);

    tag_name = pcdom_element_qualified_name(element, &len);
    if (tag_name == NULL) {
        pcinst_set_error (PURC_ERROR_HTML);
        return PCHTML_STATUS_ERROR;
    }

    html_serialize_send("<", 1, ctx);

    if (element->node.ns != PCHTML_NS_HTML
        && opt & PCHTML_HTML_SERIALIZE_OPT_TAG_WITH_NS)
    {
        const pchtml_ns_prefix_data_t *data = NULL;

        if (element->node.prefix != PCHTML_NS__UNDEF) {
            data = pchtml_ns_prefix_data_by_id(node->owner_document->prefix,
                                            element->node.prefix);
        }
        else if (element->node.ns < PCHTML_NS__LAST_ENTRY) {
            data = pchtml_ns_prefix_data_by_id(node->owner_document->prefix,
                                             element->node.ns);
        }

        if (data != NULL) {
            html_serialize_send(pcutils_hash_entry_str(&data->entry),
                                    data->entry.length, ctx);
            html_serialize_send(":", 1, ctx);
        }
    }

    html_serialize_send(tag_name, len, ctx);

    if (element->is_value != NULL && element->is_value->data != NULL) {
        attr = pcdom_element_attr_is_exist(element,
                                             (const unsigned char *) "is", 2);
        if (attr == NULL) {
            html_serialize_send(" is=\"", 5, ctx);

            if (opt & PCHTML_HTML_SERIALIZE_OPT_RAW) {
                html_serialize_send(element->is_value->data,
                                        element->is_value->length, ctx);
            }
            else {
                status = html_serialize_send_escaping_attribute_string(
                        element->is_value->data,
                        element->is_value->length, cb, ctx);
                if (status != PCHTML_STATUS_OK) {
                    return status;
                }
            }

            html_serialize_send("\"", 1, ctx);
        }
    }

    attr = element->first_attr;

    while (attr != NULL) {
        html_serialize_send(" ", 1, ctx);

        status = html_serialize_attribute_cb(attr,
                (opt & PCHTML_HTML_SERIALIZE_OPT_RAW), cb, ctx);
        if (status != PCHTML_STATUS_OK) {
            return status;
        }

        attr = attr->next;
    }

    if (opt & PCHTML_HTML_SERIALIZE_OPT_WITH_HVML_HANDLE) {
        char buff[LEN_BUFF_LONGLONGINT];
        int n = snprintf(buff, sizeof(buff),
                "%llx", (unsigned long long int)(uintptr_t)element);
        if (n < 0) {
            return PCHTML_STATUS_ERROR;
        }
        else if ((size_t)n >= sizeof (buff)) {
            PC_DEBUG ("Too small buffer to serialize message.\n");
            return PCRDR_ERROR_TOO_SMALL_BUFF;
        }
        html_serialize_send(" hvml-handle=", 13, ctx);
        html_serialize_send(buff, n, ctx);
    }

    if (node_is_self_close(node)) {
        html_serialize_send("/>", 2, ctx);
    }
    else {
        html_serialize_send(">", 1, ctx);
    }

    return PCHTML_STATUS_OK;
}

static unsigned int
html_serialize_pretty_text_cb(pcdom_text_t *text,
        pchtml_html_serialize_opt_t opt, size_t indent,
        pchtml_html_serialize_cb_f cb, void *ctx)
{
    unsigned int status;
    pcdom_node_t *node = pcdom_interface_node(text);
    pcdom_document_t *doc = node->owner_document;
    pcutils_str_t *data = &text->char_data.data;

    bool with_indent = (opt & PCHTML_HTML_SERIALIZE_OPT_WITHOUT_TEXT_INDENT) == 0;

    if (opt & PCHTML_HTML_SERIALIZE_OPT_SKIP_WS_NODES) {
        const unsigned char *pos = data->data;
        const unsigned char *end = pos + data->length;

        while (pos != end) {
            if (pchtml_tokenizer_chars_map[ *pos ]
                != PCHTML_STR_RES_MAP_CHAR_WHITESPACE)
            {
                break;
            }

            pos++;
        }

        if (pos == end)
            return PCHTML_STATUS_OK;
    }

    switch (node->parent->local_name) {
        case PCHTML_TAG_STYLE:
        case PCHTML_TAG_SCRIPT:
        case PCHTML_TAG_XMP:
        case PCHTML_TAG_IFRAME:
        case PCHTML_TAG_NOEMBED:
        case PCHTML_TAG_NOFRAMES:
        case PCHTML_TAG_PLAINTEXT:
            status = html_serialize_pretty_send_string(data->data,
                    data->length, indent, with_indent, cb, ctx);
            goto end;

        case PCHTML_TAG_NOSCRIPT:
            if (doc->scripting) {
                status = html_serialize_pretty_send_string(data->data,
                        data->length, indent, with_indent, cb, ctx);
                goto end;
            }

            break;

        default:
            break;
    }

    if (opt & PCHTML_HTML_SERIALIZE_OPT_RAW) {
        status = html_serialize_pretty_send_string(
                data->data, data->length, indent, with_indent, cb, ctx);
    }
    else {
        status = html_serialize_pretty_send_escaping_string(data->data,
                data->length, opt, indent, with_indent, cb, ctx);
    }

end:

    if (status != PCHTML_STATUS_OK) {
        return status;
    }

    if (indent > 0 &&
            (opt & PCHTML_HTML_SERIALIZE_OPT_SKIP_WS_NODES) == 0) {
        html_serialize_send("\n", 1, ctx);
    }

    return PCHTML_STATUS_OK;
}

static unsigned int
html_serialize_pretty_comment_cb(pcdom_comment_t *comment,
        pchtml_html_serialize_opt_t opt,
        size_t indent, bool with_indent,
        pchtml_html_serialize_cb_f cb, void *ctx)
{
    unsigned int status;

    if ((opt & PCHTML_HTML_SERIALIZE_OPT_WITHOUT_TEXT_INDENT)==0) {
        html_serialize_send_indent(indent, ctx);
    }
    html_serialize_send("<!-- ", 5, ctx);

    if (with_indent) {
        const unsigned char *data = comment->char_data.data.data;
        const unsigned char *pos = data;
        const unsigned char *end = pos + comment->char_data.data.length;

        while (data != end) {
            /*
             * U+000A LINE FEED (LF)
             * U+000D CARRIAGE RETURN (CR)
             */
            if (*data == 0x0A || *data == 0x0D) {
                if (pos != data) {
                    html_serialize_send(pos, (data - pos), ctx);
                }

                html_serialize_send(data, 1, ctx);
                if ((opt & PCHTML_HTML_SERIALIZE_OPT_WITHOUT_TEXT_INDENT)==0) {
                    html_serialize_send_indent(indent, ctx);
                }

                data++;
                pos = data;
            }
            else {
                data++;
            }
        }

        if (pos != data) {
            html_serialize_send(pos, (data - pos), ctx);
        }
    }
    else {
        html_serialize_send(comment->char_data.data.data,
                                comment->char_data.data.length, ctx);
    }

    html_serialize_send(" -->", 4, ctx);

    return PCHTML_STATUS_OK;
}

static unsigned int
html_serialize_pretty_document_cb(pcdom_document_t *document,
        pchtml_html_serialize_cb_f cb, void *ctx)
{
    UNUSED_PARAM(document);

    unsigned int status;

    html_serialize_send("#document", 9, ctx);

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_html_serialize_tree_cb(pcdom_node_t *node,
        pchtml_html_serialize_cb_f cb, void *ctx)
{
    /* For a document we must serialize all children without document node. */
    if (node->local_name == PCHTML_TAG__DOCUMENT) {
        node = node->first_child;

        while (node != NULL) {
            unsigned int status = html_serialize_node_cb(node, cb, ctx);
            if (status != PCHTML_STATUS_OK) {
                return status;
            }

            node = node->next;
        }

        return PCHTML_STATUS_OK;
    }

    return html_serialize_node_cb(node, cb, ctx);
}

unsigned int
pchtml_html_serialize_tree_str(pcdom_node_t *node, pcutils_str_t *str)
{
    pchtml_html_serialize_ctx_t ctx;

    if (str->data == NULL) {
        pcutils_str_init(str, node->owner_document->text, 1024);

        if (str->data == NULL) {
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }
    }

    ctx.str = str;
    ctx.mraw = node->owner_document->text;

    return pchtml_html_serialize_tree_cb(node,
            html_serialize_str_callback, &ctx);
}

unsigned int
pchtml_html_serialize_pretty_tree_cb(pcdom_node_t *node,
        pchtml_html_serialize_opt_t opt, size_t indent,
        pchtml_html_serialize_cb_f cb, void *ctx)
{
    /* For a document we must serialize all children without document node. */
    if (node->local_name == PCHTML_TAG__DOCUMENT) {
        node = node->first_child;

        while (node != NULL) {
            unsigned int status = html_serialize_pretty_node_cb(node,
                    opt, indent, cb, ctx);
            if (status != PCHTML_STATUS_OK) {
                PC_ASSERT(0);
                return status;
            }

            node = node->next;
        }

        return PCHTML_STATUS_OK;
    }

    return html_serialize_pretty_node_cb(node, opt, indent, cb, ctx);
}

unsigned int
pchtml_html_serialize_pretty_tree_str(pcdom_node_t *node,
        pchtml_html_serialize_opt_t opt, size_t indent, pcutils_str_t *str)
{
    pchtml_html_serialize_ctx_t ctx;

    if (str->data == NULL) {
        pcutils_str_init(str, node->owner_document->text, 1024);

        if (str->data == NULL) {
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }
    }

    ctx.str = str;
    ctx.mraw = node->owner_document->text;

    return pchtml_html_serialize_pretty_tree_cb(node, opt, indent,
            html_serialize_str_callback, &ctx);
}

static unsigned int
html_serialize_pretty_send_escaping_string(
        const unsigned char *data, size_t len,
        pchtml_html_serialize_opt_t opt,
        size_t indent, bool with_indent,
        pchtml_html_serialize_cb_f cb, void *ctx)
{
    unsigned int status;
    const unsigned char *pos = data;
    const unsigned char *end = data + len;

    if ((opt & PCHTML_HTML_SERIALIZE_OPT_WITHOUT_TEXT_INDENT)==0) {
        html_serialize_send_indent(indent, ctx);
    }
//    html_serialize_send("\"", 1, ctx);

    while (data != end) {
        switch (*data) {
            /* U+0026 AMPERSAND (&) */
            case 0x26:
                if (pos != data) {
                    html_serialize_send(pos, (data - pos), ctx);
                }

                html_serialize_send("&amp;", 5, ctx);

                data++;
                pos = data;

                break;

            /* {0xC2, 0xA0} NO-BREAK SPACE */
            case 0xC2:
                data += 1;
                if (data == end) {
                    break;
                }

                if (*data != 0xA0) {
                    continue;
                }

                data -= 1;

                if (pos != data) {
                    html_serialize_send(pos, (data - pos), ctx);
                }

                html_serialize_send("&nbsp;", 6, ctx);

                data += 2;
                pos = data;

                break;

            /* U+003C LESS-THAN SIGN (<) */
            case 0x3C:
                if (pos != data) {
                    html_serialize_send(pos, (data - pos), ctx);
                }

                html_serialize_send("&lt;", 4, ctx);

                data++;
                pos = data;

                break;

            /* U+003E GREATER-THAN SIGN (>) */
            case 0x3E:
                if (pos != data) {
                    html_serialize_send(pos, (data - pos), ctx);
                }

                html_serialize_send("&gt;", 4, ctx);

                data++;
                pos = data;

                break;

            /* U+0022 double quotation marks (") */
            case 0x22:
                if (pos != data) {
                    html_serialize_send(pos, (data - pos), ctx);
                }

                html_serialize_send("&quot;", 6, ctx);

                data++;
                pos = data;

                break;

            /* U+0027 double quotation marks (') */
            case 0x27:
                if (pos != data) {
                    html_serialize_send(pos, (data - pos), ctx);
                }

                html_serialize_send("&#039;", 6, ctx);

                data++;
                pos = data;

                break;

            /*
             * U+000A LINE FEED (LF)
             * U+000D CARRIAGE RETURN (CR)
             */
            case 0x0A:
            case 0x0D:
                if (with_indent) {
                    if (pos != data) {
                        html_serialize_send(pos, (data - pos), ctx);
                    }

                    html_serialize_send("\n", 1, ctx);
                    html_serialize_send_indent(indent, ctx);

                    data++;
                    pos = data;

                    break;
                }
                /* fall through */

            default:
                data++;

                break;
        }
    }

    if (pos != data) {
        html_serialize_send(pos, (data - pos), ctx);
    }

//    html_serialize_send("\"", 1, ctx);

    return PCHTML_STATUS_OK;
}

static unsigned int
html_serialize_pretty_send_string(const unsigned char *data,
        size_t len, size_t indent, bool with_indent,
        pchtml_html_serialize_cb_f cb, void *ctx)
{
    unsigned int status;

    html_serialize_send_indent(indent, ctx);
//    html_serialize_send("\"", 1, ctx);

    if (with_indent) {
        const unsigned char *pos = data;
        const unsigned char *end = data + len;

        while (data != end) {
            /*
             * U+000A LINE FEED (LF)
             * U+000D CARRIAGE RETURN (CR)
             */
            if (*data == 0x0A || *data == 0x0D) {
                if (pos != data) {
                    html_serialize_send(pos, (data - pos), ctx);
                }

                html_serialize_send(data, 1, ctx);
                html_serialize_send_indent(indent, ctx);

                data++;
                pos = data;
            }
            else {
                data++;
            }
        }

        if (pos != data) {
            html_serialize_send(pos, (data - pos), ctx);
        }
    }
    else {
        html_serialize_send(data, len, ctx);
    }

//    html_serialize_send("\"", 1, ctx);

    return PCHTML_STATUS_OK;
}
