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


#include "html_ops.h"
#include "purc-html.h"

#include "private/debug.h"
#include "./html/interfaces/document.h"

pcdom_element_t*
html_dom_append_element(pcdom_element_t* parent, const char *tag)
{
    pcdom_node_t *node = pcdom_interface_node(parent);
    pcdom_document_t *dom_doc = node->owner_document;
    pcdom_element_t *elem;
    elem = pcdom_document_create_element(dom_doc,
            (const unsigned char*)tag, strlen(tag), NULL, false);
    if (!elem)
        return NULL;

    pcdom_node_append_child(node, pcdom_interface_node(elem));

    return elem;
}

static pcdom_text_t*
html_dom_append_content_inner(pcdom_element_t* parent, const char *txt)
{
    pcdom_document_t *doc = pcdom_interface_node(parent)->owner_document;
    const unsigned char *content = (const unsigned char*)txt;
    size_t content_len = strlen(txt);

    pcdom_text_t *text_node;
    text_node = pcdom_document_create_text_node(doc, content, content_len);
    if (text_node == NULL)
        return NULL;

    pcdom_node_append_child(pcdom_interface_node(parent),
            pcdom_interface_node(text_node));

    return text_node;
}

pcdom_text_t*
html_dom_append_content(pcdom_element_t* parent, const char *txt)
{
    pcdom_text_t* text_node = html_dom_append_content_inner(parent, txt);
    if (text_node == NULL) {
        return NULL;
    }

    return text_node;
}

pcdom_text_t*
html_dom_displace_content(pcdom_element_t* parent, const char *txt)
{
    pcdom_node_t *parent_node = pcdom_interface_node(parent);
    while (parent_node->first_child)
        pcdom_node_destroy_deep(parent_node->first_child);

    pcdom_text_t* text_node = html_dom_append_content_inner(parent, txt);
    if (text_node == NULL) {
        return NULL;
    }

    return text_node;
}

int
html_dom_set_attribute(pcdom_element_t *elem,
        const char *key, const char *val)
{
    pcdom_attr_t *attr;
    attr = pcdom_element_set_attribute(elem,
            (const unsigned char*)key, strlen(key),
            (const unsigned char*)val, strlen(val));
    if (!attr) {
        return -1;
    }

    return 0;
}

int
html_dom_remove_attribute(pcdom_element_t *elem, const char *key)
{
    unsigned int ret = pcdom_element_remove_attribute(elem,
            (const unsigned char *)key, strlen(key));

    //TODO: send to rdr
    return ret;
}

pchtml_html_document_t*
html_dom_load_document(const char *html)
{
    pchtml_html_document_t *doc;
    doc = pchtml_html_document_create();
    if (!doc)
        return NULL;

    unsigned int r;
    r = pchtml_html_document_parse_with_buf(doc,
            (const unsigned char*)html, strlen(html));
    if (r) {
        pchtml_html_document_destroy(doc);
        return NULL;
    }

    return doc;
}

int
html_dom_comp_docs(pchtml_html_document_t *docl,
    pchtml_html_document_t *docr, int *diff)
{
    char lbuf[1024], rbuf[1024];
    size_t lsz = sizeof(lbuf), rsz = sizeof(rbuf);
    char *pl = pchtml_doc_snprintf_plain(docl, lbuf, &lsz, "");
    char *pr = pchtml_doc_snprintf_plain(docr, rbuf, &rsz, "");
    int err = -1;
    if (pl && pr) {
        *diff = strcmp(pl, pr);
        if (*diff) {
            PC_DEBUGX("diff:\n%s\n%s", pl, pr);
        }
        err = 0;
    }

    if (pl != lbuf)
        free(pl);
    if (pr != rbuf)
        free(pr);

    return err;
}

bool
html_dom_is_ancestor(pcdom_node_t *ancestor, pcdom_node_t *descendant)
{
    pcdom_node_t *node = descendant;
    do {
        if (node->parent && node->parent == ancestor)
            return true;
        node = node->parent;
    } while (node);

    return false;
}

int
html_dom_add_child_chunk(pcdom_element_t *parent, const char *chunk)
{
    int r = -1;

    size_t nr = strlen(chunk);

    pcdom_node_t *root = NULL;
    do {
        pchtml_html_document_t *doc;
        doc = pchtml_html_interface_document(
                pcdom_interface_node(parent)->owner_document);
        unsigned int ui;
        ui = pchtml_html_document_parse_fragment_chunk_begin(doc, parent);
        if (ui == 0) {
            do {
                ui = pchtml_html_document_parse_fragment_chunk(doc,
                        (const unsigned char*)"<div>", 5);
                if (ui)
                    break;

                ui = pchtml_html_document_parse_fragment_chunk(doc,
                        (const unsigned char*)chunk, nr);
                if (ui)
                    break;

                ui = pchtml_html_document_parse_fragment_chunk(doc,
                        (const unsigned char*)"</div>", 6);
            } while (0);
        }
        pcdom_node_t *div;
        root = pchtml_html_document_parse_fragment_chunk_end(doc);
        if (root) {
            PC_ASSERT(root->first_child == root->last_child);
            PC_ASSERT(root->first_child);
            PC_ASSERT(root->first_child->type == PCDOM_NODE_TYPE_ELEMENT);
            div = root->first_child;
        }
        if (ui)
            break;

        while (div->first_child) {
            pcdom_node_t *child = div->first_child;
            pcdom_node_remove(child);
            pcdom_node_append_child(pcdom_interface_node(parent), child);
        }
        r = 0;
    } while (0);

    if (root)
        pcdom_node_destroy(pcdom_interface_node(root));

    return r ? -1 : 0;
}

int
html_dom_add_child(pcdom_element_t *parent, const char *fmt, ...)
{
    char buf[1024];
    size_t nr = sizeof(buf);
    char *p;
    va_list ap;
    va_start(ap, fmt);
    p = pcutils_vsnprintf(buf, &nr, fmt, ap);
    va_end(ap);

    if (!p) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }

    int r = html_dom_add_child_chunk(parent, p);

    if (p != buf)
        free(p);

    return r ? -1 : 0;
}

int
html_dom_set_child_chunk(pcdom_element_t *parent, const char *chunk)
{
    int r = -1;

    size_t nr = strlen(chunk);

    pcdom_node_t *root = NULL;
    do {
        pchtml_html_document_t *doc;
        doc = pchtml_html_interface_document(
                pcdom_interface_node(parent)->owner_document);
        unsigned int ui;
        ui = pchtml_html_document_parse_fragment_chunk_begin(doc, parent);
        if (ui == 0) {
            do {
                ui = pchtml_html_document_parse_fragment_chunk(doc,
                        (const unsigned char*)"<div>", 5);
                if (ui)
                    break;

                ui = pchtml_html_document_parse_fragment_chunk(doc,
                        (const unsigned char*)chunk, nr);
                if (ui)
                    break;

                ui = pchtml_html_document_parse_fragment_chunk(doc,
                        (const unsigned char*)"</div>", 6);
            } while (0);
        }
        pcdom_node_t *div;
        root = pchtml_html_document_parse_fragment_chunk_end(doc);
        if (root) {
            PC_ASSERT(root->first_child == root->last_child);
            PC_ASSERT(root->first_child);
            PC_ASSERT(root->first_child->type == PCDOM_NODE_TYPE_ELEMENT);
            div = root->first_child;
        }
        if (ui)
            break;

        pcdom_node_remove(div);
        while (pcdom_interface_node(parent)->first_child)
            pcdom_node_destroy_deep(pcdom_interface_node(parent)->first_child);

        while (div->first_child) {
            pcdom_node_t *child = div->first_child;
            pcdom_node_remove(child);
            pcdom_node_append_child(pcdom_interface_node(parent), child);
        }
        r = 0;
    } while (0);

    if (root)
        pcdom_node_destroy(pcdom_interface_node(root));

    return r ? -1 : 0;
}

int
html_dom_set_child(pcdom_element_t *parent, const char *fmt, ...)
{
    char buf[1024];
    size_t nr = sizeof(buf);
    char *p;
    va_list ap;
    va_start(ap, fmt);
    p = pcutils_vsnprintf(buf, &nr, fmt, ap);
    va_end(ap);

    if (!p) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }

    int r = html_dom_set_child_chunk(parent, p);

    if (p != buf)
        free(p);

    return r ? -1 : 0;
}

void
html_dom_dump_node_ex(pcdom_node_t *node,
    const char *file, int line, const char *func)
{
    PC_ASSERT(node);

    char buf[1024];
    size_t nr = sizeof(buf);
    int opt = 0;
    opt |= PCHTML_HTML_SERIALIZE_OPT_UNDEF;
    opt |= PCHTML_HTML_SERIALIZE_OPT_SKIP_WS_NODES;
    opt |= PCHTML_HTML_SERIALIZE_OPT_WITHOUT_TEXT_INDENT;
    opt |= PCHTML_HTML_SERIALIZE_OPT_FULL_DOCTYPE;
    char *p = pcdom_node_snprintf_ex(node,
            (enum pchtml_html_serialize_opt)opt, buf, &nr, "");
    if (p) {
        fprintf(stderr, "%s[%d]:%s():%p\n%s\n",
                pcutils_basename((char*)file), line, func, node, p);
        if (p != buf)
            free(p);
    }
}

