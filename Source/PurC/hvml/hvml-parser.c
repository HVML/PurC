/*
 * @file hvml-parser.c
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

#include "private/instance.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"
#include "hvml-parser.h"

#include <libgen.h>

#ifndef VTT
#define VTT(x)  PCHVML_TOKEN##x
#endif // VTT

#ifdef TO_DEBUG
#ifndef D
#define D(fmt, ...)                                    \
    fprintf(stderr, "%s[%d]:%s(): " fmt "\n",          \
        basename((char*)__FILE__), __LINE__, __func__, \
        ##__VA_ARGS__);
#endif // D
#else // ! TO_DEBUG
#define D(fmt, ...)
#endif // TO_DEBUG

#ifndef FAIL_RET
#define FAIL_RET()        \
    do {                  \
        D("fail_ret");    \
        return -1;        \
    } while (0)
#endif // FAIL_RET

static int
_push_node(struct pcvdom_gen *gen, struct pcvdom_node *node)
{
    if (gen->nr_open + 1 >= gen->sz_elements) {
        size_t sz = gen->sz_elements + 16;
        struct pcvdom_node **elems;
        elems = (struct pcvdom_node**)realloc(gen->open_elements,
            sz * sizeof(*elems));
        if (!elems)
            return -1;
        gen->open_elements = elems;
        gen->sz_elements   = sz;
    }

    gen->open_elements[gen->nr_open++] = node;

    return 0;
}

static struct pcvdom_node*
_pop_node(struct pcvdom_gen *gen)
{
    if (!gen->open_elements)
        return NULL;

    if (gen->nr_open <= 0) {
        return NULL;
    }

    return gen->open_elements[--gen->nr_open];
}

static struct pcvdom_node*
_top_node(struct pcvdom_gen *gen)
{
    if (!gen->open_elements)
        return NULL;

    if (gen->nr_open <= 0) {
        return NULL;
    }

    return gen->open_elements[gen->nr_open-1];
}

static bool
_is_doc_node(struct pcvdom_gen *gen, struct pcvdom_node *node)
{
    return &gen->doc->node == node ? true : false;
}

static struct pcvdom_element*
_create_element(struct pcvdom_gen *gen, struct pchvml_token *token)
{
    UNUSED_PARAM(gen);

    const char *tag = pchvml_token_get_name(token);
    size_t nr_attrs = pchvml_token_get_attr_size(token);

    struct pcvdom_element *elem = NULL;
    elem = pcvdom_element_create_c(tag);
    if (!elem)
        goto end;

    int r = 0;

    for (size_t i=0; i<nr_attrs; ++i) {
        // TODO: how to traverse attr
        if (1) continue;
        struct pchvml_token_attr *attr;
        attr = pchvml_token_get_attr(token, i);
        const char *name;
        enum pchvml_attr_assignment op;
        struct pcvcm_node *vcm;
        name = pchvml_token_attr_get_name(attr);
        op = pchvml_token_attr_get_assignment(attr);
        vcm = (struct pcvcm_node*)pchvml_token_attr_get_value_ex(attr, true);

        struct pcvdom_attr *vattr;
        vattr = pcvdom_attr_create(name, op, vcm);
        if (vattr) {
            pcvdom_attr_destroy(vattr);
            continue;
        }

        if (!vattr) {
            r = -1;
            if (vcm) {
                pcvcm_node_destroy(vcm);
            }
            break;
        }

        r = pcvdom_element_append_attr(elem, vattr);
        if (r) {
            pcvdom_attr_destroy(vattr);
            break;
        }
    }

    if (r)
        goto end;

    return elem;

end:
    if (elem)
        pcvdom_node_destroy(&elem->node);

    return NULL;
}

struct pcvdom_gen*
pcvdom_gen_create(void)
{
    struct pcvdom_gen *gen;
    gen = (struct pcvdom_gen*)calloc(1, sizeof(*gen));
    if (!gen)
        return NULL;

    return gen;
}

struct pcvdom_document*
pcvdom_gen_end(struct pcvdom_gen *gen)
{
    struct pcvdom_document *doc = gen->doc;
    gen->doc  = NULL; // transfer ownership
    gen->curr = NULL;

    if (gen->open_elements) {
        free(gen->open_elements);
        gen->open_elements = NULL;
        gen->nr_open       = 0;
        gen->sz_elements   = 0;
    }
    gen->eof = 1;

    return doc;
}

void
pcvdom_gen_destroy(struct pcvdom_gen *gen)
{
    if (gen->doc) {
        pcvdom_document_destroy(gen->doc);
        gen->doc = NULL;
    }

    gen->doc  = NULL;
    gen->curr = NULL;

    if (gen->open_elements) {
        free(gen->open_elements);
        gen->open_elements = NULL;
        gen->nr_open       = 0;
        gen->sz_elements   = 0;
    }

    free(gen);
}

static int
_on_doctype(struct pcvdom_gen *gen, struct pchvml_token *token)
{
    D("");
    struct pcvdom_node *node = _top_node(gen);
    if (!_is_doc_node(gen, node)) {
        FAIL_RET();
    }

    const char *txt = pchvml_token_get_text(token);
    const char *id  = pchvml_token_get_public_identifier(token);
    const char *si  = pchvml_token_get_system_information(token);
    (void)txt; (void)id;

    int r = 0;
    if (si)
        r = pcvdom_document_set_doctype(gen->doc, si);

    // TODO: check r

    if (r)
        FAIL_RET();

    return r ? -1 : 0;
}

static int
_on_start_tag(struct pcvdom_gen *gen, struct pchvml_token *token)
{
    D("");
    struct pcvdom_node *node = _top_node(gen);
    int is_doc = _is_doc_node(gen, node);

    int r = 0;
    const char *tag = pchvml_token_get_name(token);
    if (is_doc) {
        if (strcmp(tag, "hvml"))
            FAIL_RET();
        if (gen->doc->root)
            FAIL_RET();
    } else {
        if (strcmp(tag, "hvml")==0)
            FAIL_RET();
        if (gen->doc->root==NULL)
            FAIL_RET();
    }

    struct pcvdom_element *elem;
    elem = _create_element(gen, token);
    if (!elem)
        FAIL_RET();

    if (!pchvml_token_is_self_closing(token)) {
        r = _push_node(gen, &elem->node);
        if (r) {
            pcvdom_node_destroy(&elem->node);
            FAIL_RET();
        }
    }

    if (is_doc) {
        r = pcvdom_document_set_root(gen->doc, elem);
        if (r) {
            if (!pchvml_token_is_self_closing(token)) {
                _pop_node(gen);
            }
            pcvdom_node_destroy(&elem->node);
            FAIL_RET();
        }

        gen->doc->root = elem;
    }

    return 0;
}

static int
_on_end_tag(struct pcvdom_gen *gen, struct pchvml_token *token)
{
    D("");
    struct pcvdom_node *node = _top_node(gen);
    int is_doc = _is_doc_node(gen, node);

    if (is_doc)
        FAIL_RET();

    struct pcvdom_element *elem;
    elem = container_of(node, struct pcvdom_element, node);
    const char *tagname = pcvdom_element_get_tagname(elem);
    const char *tag = pchvml_token_get_name(token);

    if (!tagname || !tag || strcmp(tagname, tag))
        FAIL_RET();

    _pop_node(gen);

    return 0;
}

static int
_on_comment(struct pcvdom_gen *gen, struct pchvml_token *token)
{
    D("");
    UNUSED_PARAM(gen);
    UNUSED_PARAM(token);
    FAIL_RET();
}

static int
_on_character(struct pcvdom_gen *gen, struct pchvml_token *token)
{
    D("");
    struct pcvdom_node *node = _top_node(gen);
    int is_doc = _is_doc_node(gen, node);

    if (is_doc)
        return 0; // ignore

    const char *txt = pchvml_token_get_text(token);
    struct pcvcm_node *vcm = NULL;
    struct pcvdom_content *content = NULL;
    vcm = pcvcm_node_new_string(txt);
    if (!vcm)
        FAIL_RET();

    content = pcvdom_content_create(vcm);
    if (!content) {
        pcvcm_node_destroy(vcm);
        FAIL_RET();
    }
    struct pcvdom_element *elem;
    elem = container_of(node, struct pcvdom_element, node);
    int r;
    r = pcvdom_element_append_content(elem, content);
    if (r) {
        pcvdom_node_destroy(&content->node);
        FAIL_RET();
    }

    return 0;
}

static int
_on_vcm_tree(struct pcvdom_gen *gen, struct pchvml_token *token)
{
    D("");
    UNUSED_PARAM(gen);
    UNUSED_PARAM(token);
    FAIL_RET();
}

static int
_on_eof(struct pcvdom_gen *gen)
{
    D("");
    if (gen->eof)
        FAIL_RET();

    struct pcvdom_node *node = NULL;
    while ((node=_pop_node(gen))) {
        if (_is_doc_node(gen, node))
            break;
    }

    gen->eof = 1;

    return 0;
}

int
pcvdom_gen_push_token(struct pcvdom_gen *gen,
    struct pchvml_token *token)
{
    if (gen->eof)
        return 0; // ignore

    if (!gen->doc) {
        // generate a new document object
        gen->doc = pcvdom_document_create();
        if (!gen->doc)
            FAIL_RET();
        if (_push_node(gen, &gen->doc->node)) {
            pcvdom_document_destroy(gen->doc);
            FAIL_RET();
        }
        PC_ASSERT(_is_doc_node(gen, _top_node(gen)));
    }

    int r = 0;

again:
    switch (pchvml_token_get_type(token)) {
        case VTT(_DOCTYPE):
            r = _on_doctype(gen, token);
            break;
        case VTT(_START_TAG):
            r = _on_start_tag(gen, token);
            break;
        case VTT(_END_TAG):
            r = _on_end_tag(gen, token);
            break;
        case VTT(_COMMENT):
            r = _on_comment(gen, token);
            break;
        case VTT(_CHARACTER):
            r = _on_character(gen, token);
            break;
        case VTT(_VCM_TREE):
            r = _on_vcm_tree(gen, token);
            break;
        case VTT(_EOF):
            r = _on_eof(gen);
            break;
        default: {
            PC_ASSERT(0);
        }
    }

    if (r == 0 && gen->reprocess)
        goto again;

    return r ? -1 : 0;
}

