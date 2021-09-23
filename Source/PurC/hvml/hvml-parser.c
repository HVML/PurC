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
#define VTT(x)         PCHVML_TOKEN##x
#define VTT_S(x)       #x
#define VTT_REC(x)     { VTT(x), VTT_S(x) }
#endif // VTT

#ifndef VGIM
#define VGIM(x)        PCVDOM_GEN_INSERTION_MODE##x
#define VGIM_S(x)      #x
#define VGIM_REC(x)    { VGIM(x), VGIM_S(x) }
#endif // VGIM

struct _enum_to_str {
    unsigned int     e;
    const char      *s;
};

static const struct _enum_to_str vtts[] = {
    VTT_REC(_DOCTYPE),
    VTT_REC(_START_TAG),
    VTT_REC(_END_TAG),
    VTT_REC(_COMMENT),
    VTT_REC(_CHARACTER),
    VTT_REC(_VCM_TREE),
    VTT_REC(_EOF),
};

static const struct _enum_to_str vgims[] = {
    VGIM_REC(_INITIAL),
    VGIM_REC(_BEFORE_HVML),
    VGIM_REC(_BEFORE_HEAD),
    VGIM_REC(_IN_HEAD),
    VGIM_REC(_AFTER_HEAD),
    VGIM_REC(_IN_BODY),
    VGIM_REC(_TEXT),
    VGIM_REC(_AFTER_BODY),
    VGIM_REC(_AFTER_AFTER_BODY),
};

static const char*
_vtt_to_string(struct pchvml_token *token)
{
    if (!token) return "_UNDEFINED";

    enum pchvml_token_type type = pchvml_token_get_type(token);

    for (size_t i=0; i<PCA_TABLESIZE(vtts); ++i) {
        if (vtts[i].e == type)
            return vtts[i].s;
    }

    return "_UNKNOWN";
}

static const char*
_vgim_to_string(struct pcvdom_gen *gen)
{
    if (!gen) return "_UNDEFINED";

    enum pcvdom_gen_insertion_mode mode = gen->insertion_mode;

    for (size_t i=0; i<PCA_TABLESIZE(vgims); ++i) {
        if (vgims[i].e == mode)
            return vgims[i].s;
    }

    return "_UNKNOWN";
}


#define TO_DEBUG

#ifdef TO_DEBUG
#ifndef D
#define D(fmt, ...)                                           \
    fprintf(stderr, "%s[%d]:%s(): %s @ %s" fmt "\n",          \
        basename((char*)__FILE__), __LINE__, __func__,        \
        _vtt_to_string(token),                                \
        _vgim_to_string(gen),                                 \
        ##__VA_ARGS__);
#endif // D
#else // ! TO_DEBUG
#define D(fmt, ...) ({                                        \
    UNUSED_PARAM(_vtt_to_string);                             \
    UNUSED_PARAM(_vgim_to_string);                            \
    })
#endif // TO_DEBUG

#ifndef FAIL_RET
#define FAIL_RET()        \
    do {                  \
        D("fail_ret");    \
        return -1;        \
    } while (0)
#endif // FAIL_RET

static inline int
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

static inline struct pcvdom_node*
_pop_node(struct pcvdom_gen *gen)
{
    if (!gen->open_elements)
        return NULL;

    if (gen->nr_open <= 0) {
        return NULL;
    }

    return gen->open_elements[--gen->nr_open];
}

static inline struct pcvdom_node*
_top_node(struct pcvdom_gen *gen)
{
    if (!gen->open_elements)
        return NULL;

    if (gen->nr_open <= 0) {
        return NULL;
    }

    return gen->open_elements[gen->nr_open-1];
}

static inline bool
_is_doc_node(struct pcvdom_gen *gen, struct pcvdom_node *node)
{
    return &gen->doc->node == node ? true : false;
}

static inline struct pcvdom_element*
_top_element(struct pcvdom_gen *gen)
{
    struct pcvdom_node *node = _top_node(gen);
    struct pcvdom_element *elem;
    elem = container_of(node, struct pcvdom_element, node);
    return elem;
}

static struct pcvdom_element*
_create_element(struct pcvdom_gen *gen, struct pchvml_token *token)
{
    UNUSED_PARAM(gen);

    int r = 0;

    const char *tag = pchvml_token_get_name(token);
    size_t nr_attrs = pchvml_token_get_attr_size(token);

    struct pcvdom_element *elem = NULL;
    elem = pcvdom_element_create_c(tag);
    if (!elem)
        goto end;

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

static int
_create_head(struct pcvdom_gen *gen, struct pchvml_token *token)
{
    int r = 0;
    struct pcvdom_element *elem = NULL;
    elem = pcvdom_element_create_c("head");

    if (!elem)
        FAIL_RET();

    struct pcvdom_element *top;
    top = _top_element(gen);

    r = pcvdom_element_append_element(top, elem);
    if (r) {
        pcvdom_node_destroy(&elem->node);
        FAIL_RET();
    }

    if (!pchvml_token_is_self_closing(token)) {
        r = _push_node(gen, &elem->node);
        if (r) {
            pcvdom_node_destroy(&elem->node);
            FAIL_RET();
        }
        gen->insertion_mode = VGIM(_IN_HEAD);
        return 0;
    }

    gen->insertion_mode = VGIM(_AFTER_HEAD);
    return 0;
}

static int
_create_empty_head(struct pcvdom_gen *gen, struct pchvml_token *token)
{
    int r = 0;
    struct pcvdom_element *elem = NULL;
    elem = pcvdom_element_create_c("head");

    if (!elem)
        FAIL_RET();

    struct pcvdom_element *top;
    top = _top_element(gen);

    r = pcvdom_element_append_element(top, elem);
    if (r) {
        pcvdom_node_destroy(&elem->node);
        FAIL_RET();
    }

    gen->insertion_mode = VGIM(_AFTER_HEAD);
    return 0;
}

static int
_create_body(struct pcvdom_gen *gen, struct pchvml_token *token)
{
    int r = 0;
    struct pcvdom_element *elem = NULL;
    elem = pcvdom_element_create_c("body");

    if (!elem)
        FAIL_RET();

    struct pcvdom_element *top;
    top = _top_element(gen);

    r = pcvdom_element_append_element(top, elem);
    if (r) {
        pcvdom_node_destroy(&elem->node);
        FAIL_RET();
    }

    if (!pchvml_token_is_self_closing(token)) {
        r = _push_node(gen, &elem->node);
        if (r) {
            pcvdom_node_destroy(&elem->node);
            FAIL_RET();
        }
        gen->insertion_mode = VGIM(_IN_BODY);
        return 0;
    }

    gen->insertion_mode = VGIM(_AFTER_BODY);
    return 0;
}

static int
_create_empty_body(struct pcvdom_gen *gen, struct pchvml_token *token)
{
    int r = 0;
    struct pcvdom_element *elem = NULL;
    elem = pcvdom_element_create_c("body");

    if (!elem)
        FAIL_RET();

    struct pcvdom_element *top;
    top = _top_element(gen);

    r = pcvdom_element_append_element(top, elem);
    if (r) {
        pcvdom_node_destroy(&elem->node);
        FAIL_RET();
    }

    gen->insertion_mode = VGIM(_AFTER_BODY);
    return 0;
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

    gen->parser = NULL;

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
_create_doctype(struct pcvdom_gen *gen, struct pchvml_token *token)
{
    const char *si = "v: SYSTEM MATH FILE FS";
    if (token)
        si = pchvml_token_get_system_information(token);
    if (!si)
        si = "v: SYSTEM MATH FILE FS";

    int r = 0;

    r = pcvdom_document_set_doctype(gen->doc, si);

    // TODO: check r

    if (r)
        FAIL_RET();

    return 0;
}

static int
_create_hvml(struct pcvdom_gen *gen, struct pchvml_token *token)
{
    int r = 0;
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

    r = pcvdom_document_set_root(gen->doc, elem);
    if (r) {
        if (!pchvml_token_is_self_closing(token)) {
            _pop_node(gen);
        }
        pcvdom_node_destroy(&elem->node);
        FAIL_RET();
    }

    gen->doc->root = elem;
    gen->insertion_mode = VGIM(_BEFORE_HEAD);
    return 0;
}

static int
_on_mode_initial(struct pcvdom_gen *gen, struct pchvml_token *token)
{
    D("");
    int r = 0;
    enum pchvml_token_type type = pchvml_token_get_type(token);
    if (type==VTT(_DOCTYPE)) {
        if (gen->doc->doctype)
            return 0; // just ignore

        r = _create_doctype(gen, token);

        // TODO: check r

        if (r)
            FAIL_RET();

        return r ? -1 : 0;
    }

    if (type==VTT(_START_TAG)) {
        const char *tag = pchvml_token_get_name(token);
        if (strcmp(tag, "hvml")==0) {
            if (gen->doc->doctype==NULL) {
                r = _create_doctype(gen, NULL);

                // TODO: check r

                if (r)
                    FAIL_RET();
            }
            gen->insertion_mode = VGIM(_BEFORE_HVML);
            gen->reprocess = 1;
            return 0;
        }
        goto anything_else;
    }

    if (type==VTT(_CHARACTER)) {
        return 0; // just ignore
    }

    if (type==VTT(_EOF)) {
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

anything_else:
    UNUSED_PARAM(gen);
    UNUSED_PARAM(token);
    return -1;
}

static int
_on_mode_before_hvml(struct pcvdom_gen *gen, struct pchvml_token *token)
{
    D("");
    enum pchvml_token_type type = pchvml_token_get_type(token);
    if (type==VTT(_DOCTYPE)) {
        return 0; // just ignore
    }

    if (type==VTT(_START_TAG)) {
        const char *tag = pchvml_token_get_name(token);
        if (strcmp(tag, "hvml")==0) {
            return _create_hvml(gen, token);
        }
        goto anything_else;
    }

    if (type==VTT(_CHARACTER)) {
        return 0; // just ignore
    }

anything_else:
    UNUSED_PARAM(gen);
    UNUSED_PARAM(token);
    return -1;
}

static int
_on_mode_before_head(struct pcvdom_gen *gen, struct pchvml_token *token)
{
    int r = 0;
    D("");
    enum pchvml_token_type type = pchvml_token_get_type(token);
    if (type==VTT(_DOCTYPE)) {
        return 0; // just ignore
    }

    if (type==VTT(_START_TAG)) {
        const char *tag = pchvml_token_get_name(token);
        if (strcmp(tag, "hvml")==0)
            FAIL_RET();

        if (strcmp(tag, "head")==0) {
            r = _create_head(gen, token);
            if (r)
                FAIL_RET();

            return 0;
        }

        r = _create_empty_head(gen, NULL);
        if (r)
            FAIL_RET();

        gen->reprocess = 1;
        return 0;
    }

    if (type==VTT(_END_TAG)) {
        struct pcvdom_node *node = _top_node(gen);
        struct pcvdom_element *elem;
        elem = container_of(node, struct pcvdom_element, node);
        const char *tagname = pcvdom_element_get_tagname(elem);
        const char *tag = pchvml_token_get_name(token);

        if (!tagname || !tag || strcmp(tagname, tag))
            FAIL_RET();

        _pop_node(gen);
        return 0;
    }

    if (type==VTT(_EOF)) {
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

    if (type==VTT(_CHARACTER)) {
        return 0; // just ignore
    }

    UNUSED_PARAM(gen);
    UNUSED_PARAM(token);
    return -1;
}

static int
_on_mode_in_head(struct pcvdom_gen *gen, struct pchvml_token *token)
{
    int r = 0;
    D("");
    enum pchvml_token_type type = pchvml_token_get_type(token);
    if (type==VTT(_DOCTYPE)) {
        return 0; // just ignore
    }

    if (type==VTT(_START_TAG)) {
        const char *tag = pchvml_token_get_name(token);
        if (strcmp(tag, "hvml")==0)
            FAIL_RET();

        struct pcvdom_element *elem;
        elem = _create_element(gen, token);
        if (!elem)
            FAIL_RET();

        struct pcvdom_element *top;
        top = _top_element(gen);

        r = pcvdom_element_append_element(top, elem);
        if (r) {
            pcvdom_node_destroy(&elem->node);
            FAIL_RET();
        }

        if (!pchvml_token_is_self_closing(token)) {
            r = _push_node(gen, &elem->node);
            if (r) {
                pcvdom_node_destroy(&elem->node);
                FAIL_RET();
            }
        }

        if (strcmp(tag, "init")==0) {
            gen->parser->state = PCHVML_EJSON_DATA_STATE;
        }

        return 0;
    }

    if (type==VTT(_END_TAG)) {
        struct pcvdom_node *node = _top_node(gen);
        struct pcvdom_element *elem;
        elem = container_of(node, struct pcvdom_element, node);
        const char *tagname = pcvdom_element_get_tagname(elem);
        const char *tag = pchvml_token_get_name(token);

        if (!tagname || !tag || strcmp(tagname, tag))
            FAIL_RET();

        _pop_node(gen);

        if (strcmp(tag, "head")==0) {
            gen->insertion_mode = VGIM(_AFTER_HEAD);
        }

        return 0;
    }

    if (type==VTT(_VCM_TREE)) {
        return 0; // ignore for now
    }

    if (type==VTT(_EOF)) {
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

    if (type==VTT(_CHARACTER)) {
        return 0; // just ignore
    }

    UNUSED_PARAM(gen);
    UNUSED_PARAM(token);
    return -1;
}

static int
_on_mode_after_head(struct pcvdom_gen *gen, struct pchvml_token *token)
{
    int r = 0;
    D("");
    enum pchvml_token_type type = pchvml_token_get_type(token);
    if (type==VTT(_DOCTYPE)) {
        return 0; // just ignore
    }

    if (type==VTT(_START_TAG)) {
        const char *tag = pchvml_token_get_name(token);
        if (strcmp(tag, "hvml")==0)
            FAIL_RET();

        if (strcmp(tag, "body")==0) {
            r = _create_body(gen, token);
            if (r)
                FAIL_RET();

            return 0;
        }

        r = _create_empty_body(gen, NULL);
        if (r)
            FAIL_RET();

        gen->reprocess = 1;
        return 0;
    }

    if (type==VTT(_END_TAG)) {
        struct pcvdom_node *node = _top_node(gen);
        struct pcvdom_element *elem;
        elem = container_of(node, struct pcvdom_element, node);
        const char *tagname = pcvdom_element_get_tagname(elem);
        const char *tag = pchvml_token_get_name(token);

        if (!tagname || !tag || strcmp(tagname, tag))
            FAIL_RET();

        _pop_node(gen);

        return 0;
    }

    if (type==VTT(_EOF)) {
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

    if (type==VTT(_CHARACTER)) {
        return 0; // just ignore
    }

    UNUSED_PARAM(gen);
    UNUSED_PARAM(token);
    return -1;
}

static int
_on_mode_in_body(struct pcvdom_gen *gen, struct pchvml_token *token)
{
    int r = 0;
    D("");
    enum pchvml_token_type type = pchvml_token_get_type(token);
    if (type==VTT(_DOCTYPE)) {
        return 0; // just ignore
    }

    if (type==VTT(_START_TAG)) {
        const char *tag = pchvml_token_get_name(token);
        if (strcmp(tag, "hvml")==0)
            FAIL_RET();

        struct pcvdom_element *elem;
        elem = _create_element(gen, token);
        if (!elem)
            FAIL_RET();

        struct pcvdom_element *top;
        top = _top_element(gen);

        r = pcvdom_element_append_element(top, elem);
        if (r) {
            pcvdom_node_destroy(&elem->node);
            FAIL_RET();
        }

        if (!pchvml_token_is_self_closing(token)) {
            r = _push_node(gen, &elem->node);
            if (r) {
                pcvdom_node_destroy(&elem->node);
                FAIL_RET();
            }
        }

        if (strcmp(tag, "init")==0) {
            gen->parser->state = PCHVML_EJSON_DATA_STATE;
        }

        return 0;
    }

    if (type==VTT(_END_TAG)) {
        struct pcvdom_node *node = _top_node(gen);
        struct pcvdom_element *elem;
        elem = container_of(node, struct pcvdom_element, node);
        const char *tagname = pcvdom_element_get_tagname(elem);
        const char *tag = pchvml_token_get_name(token);

        if (!tagname || !tag || strcmp(tagname, tag))
            FAIL_RET();

        _pop_node(gen);

        if (strcmp(tag, "body")==0) {
            gen->insertion_mode = VGIM(_AFTER_BODY);
        }

        return 0;
    }

    if (type==VTT(_VCM_TREE)) {
        return 0; // ignore for now
    }

    if (type==VTT(_EOF)) {
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

    if (type==VTT(_CHARACTER)) {
        return 0; // just ignore
    }

    UNUSED_PARAM(gen);
    UNUSED_PARAM(token);
    return -1;
}

static int
_on_mode_text(struct pcvdom_gen *gen, struct pchvml_token *token)
{
    D("");
    UNUSED_PARAM(gen);
    UNUSED_PARAM(token);
    return -1;
}

static int
_on_mode_after_body(struct pcvdom_gen *gen, struct pchvml_token *token)
{
    D("");
    enum pchvml_token_type type = pchvml_token_get_type(token);
    if (type==VTT(_DOCTYPE)) {
        return 0; // just ignore
    }

    if (type==VTT(_END_TAG)) {
        struct pcvdom_node *node = _top_node(gen);
        struct pcvdom_element *elem;
        elem = container_of(node, struct pcvdom_element, node);
        const char *tagname = pcvdom_element_get_tagname(elem);
        const char *tag = pchvml_token_get_name(token);

        if (!tagname || !tag || strcmp(tagname, tag))
            FAIL_RET();

        _pop_node(gen);

        if (strcmp(tag, "hvml")==0) {
            gen->insertion_mode = VGIM(_AFTER_AFTER_BODY);
        }
        return 0;
    }

    if (type==VTT(_EOF)) {
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

    if (type==VTT(_CHARACTER)) {
        return 0; // just ignore
    }
    UNUSED_PARAM(gen);
    UNUSED_PARAM(token);
    return -1;
}

static int
_on_mode_after_after_body(struct pcvdom_gen *gen, struct pchvml_token *token)
{
    D("");

    enum pchvml_token_type type = pchvml_token_get_type(token);

    if (type==VTT(_EOF)) {
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

    if (type==VTT(_CHARACTER)) {
        return 0; // just ignore
    }

    UNUSED_PARAM(gen);
    UNUSED_PARAM(token);
    return -1;
}


int
pcvdom_gen_push_token(struct pcvdom_gen *gen,
    struct pchvml_parser     *parser, /* exists for tokenizer state change */
    struct pchvml_token *token)
{
    int r = 0;

    if (gen->eof)
        return 0; // ignore

    gen->parser = parser;

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

again:
    gen->reprocess = 0;

    switch (gen->insertion_mode) {
        case VGIM(_INITIAL):
            r = _on_mode_initial(gen, token);
            break;
        case VGIM(_BEFORE_HVML):
            r = _on_mode_before_hvml(gen, token);
            break;
        case VGIM(_BEFORE_HEAD):
            r = _on_mode_before_head(gen, token);
            break;
        case VGIM(_IN_HEAD):
            r = _on_mode_in_head(gen, token);
            break;
        case VGIM(_AFTER_HEAD):
            r = _on_mode_after_head(gen, token);
            break;
        case VGIM(_IN_BODY):
            r = _on_mode_in_body(gen, token);
            break;
        case VGIM(_TEXT):
            r = _on_mode_text(gen, token);
            break;
        case VGIM(_AFTER_BODY):
            r = _on_mode_after_body(gen, token);
            break;
        case VGIM(_AFTER_AFTER_BODY):
            r = _on_mode_after_after_body(gen, token);
            break;
        default:
            PC_ASSERT(0);
            break;
    }

    if (r == 0 && gen->reprocess)
        goto again;

    return r ? -1 : 0;
}

