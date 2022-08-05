/*
 * @file hvml-gen.c
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
#include "private/vdom.h"
#include "private/tkz-helper.h"
#include "hvml-gen.h"

#include "../vdom/vdom-internal.h"

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

struct enum_to_str {
    unsigned int     e;
    const char      *s;
};

static const struct enum_to_str vtts[] = {
    VTT_REC(_DOCTYPE),
    VTT_REC(_START_TAG),
    VTT_REC(_END_TAG),
    VTT_REC(_COMMENT),
    VTT_REC(_CHARACTER),
    VTT_REC(_VCM_TREE),
    VTT_REC(_EOF),
};

static const struct enum_to_str vgims[] = {
    VGIM_REC(_INITIAL),
    VGIM_REC(_BEFORE_HVML),
    VGIM_REC(_IN_HVML),
    VGIM_REC(_BEFORE_HEAD),
    VGIM_REC(_IN_HEAD),
    VGIM_REC(_AFTER_HEAD),
    VGIM_REC(_IN_BODY),
    VGIM_REC(_AFTER_BODY),
    VGIM_REC(_AFTER_AFTER_BODY),
};

static const char*
vtt_to_string(struct pchvml_token *token)
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
vgim_to_string(struct pcvdom_gen *gen)
{
    if (!gen) return "_UNDEFINED";

    enum pcvdom_gen_insertion_mode mode = gen->insertion_mode;

    for (size_t i=0; i<PCA_TABLESIZE(vgims); ++i) {
        if (vgims[i].e == mode)
            return vgims[i].s;
    }

    return "_UNKNOWN";
}


#ifndef FAIL_RET
#define FAIL_RET()                                                \
    do {                                                          \
        PC_DEBUGX("%s[%s] @ %s: fail_ret",                        \
            vtt_to_string(token),                                 \
            pchvml_token_get_name(token),                         \
            vgim_to_string(gen));                                 \
        purc_set_error(PCHVML_ERROR_UNEXPECTED_CHARACTER);        \
        return -1;                                                \
    } while (0)
#endif // FAIL_RET

static inline int
push_node(struct pcvdom_gen *gen, struct pcvdom_node *node)
{
    gen->curr = node;

    return 0;
}

static inline struct pcvdom_node*
pop_node(struct pcvdom_gen *gen)
{
    if (!gen->curr)
        return NULL;

    struct pcvdom_node *node;
    node = container_of(gen->curr->node.parent,
            struct pcvdom_node, node);
    gen->curr = node;
    return node;
}

static inline struct pcvdom_node*
top_node(struct pcvdom_gen *gen)
{
    return gen->curr;
}

static inline bool
is_doc_node(struct pcvdom_gen *gen, struct pcvdom_node *node)
{
    return &gen->doc->node == node ? true : false;
}

static inline struct pcvdom_element*
top_element(struct pcvdom_gen *gen)
{
    struct pcvdom_node *node = top_node(gen);
    PC_ASSERT(PCVDOM_NODE_IS_ELEMENT(node));
    struct pcvdom_element *elem;
    elem = container_of(node, struct pcvdom_element, node);
    return elem;
}

static inline bool
is_element_of_hvml_data_cat(struct pcvdom_element *elem)
{
    const struct pchvml_tag_entry *entry;
    entry = pchvml_tag_static_get_by_id(elem->tag_id);

    return entry && (entry->cats & PCHVML_TAGCAT_DATA);
}

static inline bool
is_top_node_of_head(struct pcvdom_gen *gen)
{
    struct pcvdom_node *top = top_node(gen);
    if (is_doc_node(gen, top))
        return false;
    struct pcvdom_element *elem;
    elem = container_of(top, struct pcvdom_element, node);
    return gen->doc->head == elem ? true : false;
}

static inline enum pchvml_tag_id
tag_id_from_tag(const char *tag)
{
    if (!tag)
        return PCHVML_TAG__UNDEF;

    const struct pchvml_tag_entry *entry;
    entry = pchvml_tag_static_search(tag, strlen(tag));

    if (entry)
        return entry->id;

    return PCHVML_TAG__UNDEF;
}

static inline bool
is_tag_of_hvml_verb_cat(enum pchvml_tag_id id)
{
    const struct pchvml_tag_entry *entry;
    entry = pchvml_tag_static_get_by_id(id);

    return entry && (entry->cats & PCHVML_TAGCAT_VERB);
}

static inline bool
is_top_node_of_body(struct pcvdom_gen *gen)
{
    struct pcvdom_node *top = top_node(gen);
    if (is_doc_node(gen, top))
        return false;
    struct pcvdom_element *elem;
    elem = container_of(top, struct pcvdom_element, node);
    return gen->doc->body == elem ? true : false;
}

UNUSED_FUNCTION static inline bool
is_top_node_of_hvml_verb_cat(struct pcvdom_gen *gen)
{
    struct pcvdom_node *top = top_node(gen);
    if (is_doc_node(gen, top))
        return false;
    struct pcvdom_element *elem;
    elem = container_of(top, struct pcvdom_element, node);

    return is_tag_of_hvml_verb_cat(elem->tag_id);
}

static inline void
set_parser_state_if_necessary(struct pcvdom_gen *gen)
{
    struct pcvdom_node *top = top_node(gen);
    if (is_doc_node(gen, top))
        return;

    struct pcvdom_element *elem = top_element(gen);
    if (is_element_of_hvml_data_cat(elem)) {
        // FIXME:
        if (0)
            pchvml_switch_to_ejson_state(gen->parser);
    }
}

static struct pcvdom_element*
create_element(struct pcvdom_gen *gen, struct pchvml_token *token)
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
        struct pchvml_token_attr *attr;
        attr = pchvml_token_get_attr(token, i);
        const char *name;
        enum pchvml_attr_operator op;
        struct pcvcm_node *vcm;
        name = pchvml_token_attr_get_name(attr);
        op = pchvml_token_attr_get_operator(attr);
        vcm = (struct pcvcm_node*)pchvml_token_attr_get_value_ex(attr, true);

        struct pcvdom_attr *vattr;
        vattr = pcvdom_attr_create(name, op, vcm);

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
create_head(struct pcvdom_gen *gen, struct pchvml_token *token)
{
    int r = 0;
    struct pcvdom_element *elem = NULL;

    if (gen->doc->head)
        return -1;

    elem = pcvdom_element_create_c("head");

    if (!elem)
        FAIL_RET();

    struct pcvdom_element *top;
    top = top_element(gen);

    r = pcvdom_element_append_element(top, elem);
    if (r) {
        pcvdom_node_destroy(&elem->node);
        FAIL_RET();
    }

    if (token && !pchvml_token_is_self_closing(token)) {
        r = push_node(gen, &elem->node);
        if (r) {
            pcvdom_node_destroy(&elem->node);
            FAIL_RET();
        }
        gen->insertion_mode = VGIM(_IN_HEAD);
    }
    else {
        gen->insertion_mode = VGIM(_AFTER_HEAD);
        elem->self_closing = 1;
    }

    gen->doc->head      = elem;

    return 0;
}

static int
create_body(struct pcvdom_gen *gen, struct pchvml_token *token)
{
    int r = 0;
    struct pcvdom_element *elem = NULL;

    struct pcvdom_element *top;
    top = top_element(gen);

    if (token) {
        elem = create_element(gen, token);
    }

    if (!elem) {
        elem = pcvdom_element_create_c("body");
    }

    if (!elem)
        FAIL_RET();

    size_t nr;
    nr = pcutils_arrlist_length(gen->doc->bodies);

    r = pcutils_arrlist_put_idx(gen->doc->bodies, nr, elem);
    if (r) {
        pcvdom_node_destroy(&elem->node);
        FAIL_RET();
    }

    r = pcvdom_element_append_element(top, elem);
    if (r) {
        pcutils_arrlist_del_idx(gen->doc->bodies, nr, 1);
        pcvdom_node_destroy(&elem->node);
        FAIL_RET();
    }

    if (!token || !pchvml_token_is_self_closing(token)) {
        r = push_node(gen, &elem->node);
        if (r) {
            pcutils_arrlist_del_idx(gen->doc->bodies, nr, 1);
            pcvdom_node_destroy(&elem->node);
            FAIL_RET();
        }
        gen->insertion_mode = VGIM(_IN_BODY);
    } else {
        gen->insertion_mode = VGIM(_AFTER_BODY);
        elem->self_closing = 1;
    }

    gen->doc->body      = elem;

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

    gen->eof = 1;

    gen->parser = NULL;

    return doc;
}

void
pcvdom_gen_destroy(struct pcvdom_gen *gen)
{
    if (gen->doc) {
        pcvdom_document_unref(gen->doc);
        gen->doc = NULL;
    }

    gen->doc  = NULL;
    gen->curr = NULL;

    free(gen);
}

static int
create_doctype(struct pcvdom_gen *gen, struct pchvml_token *token)
{
    const char *name = NULL;
    if (token)
        name = pchvml_token_get_public_identifier(token);

    const char *si = NULL;
    if (token)
        si = pchvml_token_get_system_information(token);

    int r = 0;

    if (!name)
        name = ""; // FIXME: "hvml" ?
    if (!si)
        si = "v:";

    r = pcvdom_document_set_doctype(gen->doc, name, si);

    // TODO: check r

    if (r)
        FAIL_RET();

    if (pcutils_strcasecmp(name, "hvml"))
        gen->doc->quirks = 1;

    return 0;
}

static int
create_comment(struct pcvdom_gen *gen, struct pchvml_token *token)
{
    const char *text;
    text = pchvml_token_get_text(token);

    struct pcvdom_comment *comment;
    comment = pcvdom_comment_create(text);

    if (!comment)
        return -1;

    int r = 0;

    struct pcvdom_node *node = top_node(gen);
    PC_ASSERT(node);
    if (PCVDOM_NODE_IS_DOCUMENT(node)) {
        r = pcvdom_document_append_comment(gen->doc, comment);
    }
    else if (PCVDOM_NODE_IS_ELEMENT(node)) {
        struct pcvdom_element *element = PCVDOM_ELEMENT_FROM_NODE(node);
        r = pcvdom_element_append_comment(element, comment);
    }

    // TODO: check r

    if (r)
        FAIL_RET();

    return 0;
}

static int
set_vcm_tree(struct pcvdom_gen *gen, struct pchvml_token *token)
{
    UNUSED_PARAM(gen);

    // FIXME: filter whitespaces
    if (pchvml_token_is_whitespace(token))
        return 0;

    int r = 0;

    struct pcvdom_node *node = top_node(gen);
    PC_ASSERT(node);
    PC_ASSERT(PCVDOM_NODE_IS_ELEMENT(node));

    struct pcvdom_element *element = PCVDOM_ELEMENT_FROM_NODE(node);
    struct pcvcm_node *vcm_content;
    vcm_content = pchvml_token_detach_vcm_content(token);
    if (!vcm_content) // FIXME: why?
        return 0;
    r = pcvdom_element_set_vcm_content(element, vcm_content);
    PC_ASSERT(r == 0);

    if (r) {
        pcvcm_node_destroy(vcm_content);
        FAIL_RET();
    }

    return 0;
}

static int
create_hvml(struct pcvdom_gen *gen, struct pchvml_token *token)
{
    int r = 0;
    struct pcvdom_element *elem;
    elem = create_element(gen, token);
    if (!elem)
        FAIL_RET();

    bool self_closing = pchvml_token_is_self_closing(token);

    if (!self_closing) {
        r = push_node(gen, &elem->node);
        if (r) {
            pcvdom_node_destroy(&elem->node);
            FAIL_RET();
        }
    }
    else {
        elem->self_closing = 1;
    }

    r = pcvdom_document_set_root(gen->doc, elem);
    if (r) {
        if (!self_closing) {
            pop_node(gen);
        }
        pcvdom_node_destroy(&elem->node);
        FAIL_RET();
    }

    if (!self_closing) {
        gen->insertion_mode = VGIM(_BEFORE_HEAD);
    } else {
        gen->insertion_mode = VGIM(_AFTER_AFTER_BODY);
    }
    return 0;
}

static int
create_empty_hvml(struct pcvdom_gen *gen, struct pchvml_token *token)
{
    int r = 0;
    struct pcvdom_element *elem = NULL;
    elem = pcvdom_element_create_c("hvml");

    if (!elem)
        FAIL_RET();

    r = pcvdom_document_set_root(gen->doc, elem);
    if (r) {
        pcvdom_node_destroy(&elem->node);
        FAIL_RET();
    }

    r = push_node(gen, &elem->node);
    if (r) {
        pcvdom_node_destroy(&elem->node);
        FAIL_RET();
    }

    gen->insertion_mode = VGIM(_BEFORE_HEAD);
    return 0;
}

static int
on_mode_initial(struct pcvdom_gen *gen, struct pchvml_token *token)
{
    int r = 0;
    enum pchvml_token_type type = pchvml_token_get_type(token);
    if (type==VTT(_DOCTYPE)) {
        r = create_doctype(gen, token);

        if (r)
            FAIL_RET();

        gen->insertion_mode = VGIM(_BEFORE_HVML);
        return 0;
    }
    else if (type==VTT(_CHARACTER)) {
        FAIL_RET();
        return 0; // just ignore
    }
    else if (type==VTT(_COMMENT)) {
        r = create_comment(gen, token);

        if (r)
            FAIL_RET();

        return 0;
    }
    else if (type==VTT(_START_TAG)) {
        r = create_doctype(gen, NULL);

        if (r)
            FAIL_RET();

        gen->insertion_mode = VGIM(_BEFORE_HVML);
        gen->reprocess = 1;

        return 0;
    }
    else if (type==VTT(_EOF)) {
        PC_ASSERT(gen->doc);
        return 0;
    }
    else if (type==VTT(_VCM_TREE)) {
        if (pchvml_token_is_whitespace(token))
            return 0; // just ignore
        FAIL_RET();
        return 0;
    }

    FAIL_RET();
    PC_ASSERT(0);
    return -1;
}

static int
on_mode_before_hvml(struct pcvdom_gen *gen, struct pchvml_token *token)
{
    int r = 0;
    enum pchvml_token_type type = pchvml_token_get_type(token);
    if (type==VTT(_START_TAG)) {
        const char *tag = pchvml_token_get_name(token);
        const enum pchvml_tag_id tag_id = tag_id_from_tag(tag);
        if (tag_id == PCHVML_TAG_HVML) {
            return create_hvml(gen, token);
        }
        // fall through
        FAIL_RET();
    }
    else if (type==VTT(_EOF)) {
        if (create_empty_hvml(gen, token))
            FAIL_RET();
        gen->reprocess = 1;
        return 0;
    }
    else if (type==VTT(_CHARACTER)) {
        FAIL_RET();
        return 0; // just ignore
    }
    else if (type==VTT(_COMMENT)) {
        r = create_comment(gen, token);

        if (r)
            FAIL_RET();

        return 0;
    }
    else if (type==VTT(_DOCTYPE)) {
        FAIL_RET();
        return 0; // just ignore
    }
    else if (type==VTT(_VCM_TREE)) {
        if (pchvml_token_is_whitespace(token))
            return 0; // just ignore
        FAIL_RET();
        return 0;
    }

    FAIL_RET();
}

static int
on_mode_in_hvml(struct pcvdom_gen *gen, struct pchvml_token *token)
{
    int r = 0;
    enum pchvml_token_type type = pchvml_token_get_type(token);
    if (type==VTT(_DOCTYPE)) {
        FAIL_RET();
        return 0; // just ignore
    }

    if (type==VTT(_START_TAG)) {
        const char *tag = pchvml_token_get_name(token);
        const enum pchvml_tag_id tag_id = tag_id_from_tag(tag);
        if (tag_id == PCHVML_TAG_INIT||
            tag_id == PCHVML_TAG_SET ||
            tag_id == PCHVML_TAG_ARCHEDATA ||
            tag_id == PCHVML_TAG_BIND ||
            tag_id == PCHVML_TAG_CONNECT ||
            tag_id == PCHVML_TAG_ARCHETYPE)
        {
            // fall through
        }
        else if (is_tag_of_hvml_verb_cat(tag_id)) {
            // fall through
        }
        else if (tag_id == PCHVML_TAG_ERROR ||
            tag_id == PCHVML_TAG_EXCEPT)
        {
            // if (!is_top_node_of_hvml_verb_cat(gen))
            //     FAIL_RET();
            // fall through
        }
        else if (tag_id == PCHVML_TAG__UNDEF) {
            // fall through
        }
        else {
            FAIL_RET();
            return 0; // ignore for now
        }

        struct pcvdom_element *elem;
        elem = create_element(gen, token);
        if (!elem)
            FAIL_RET();

        struct pcvdom_element *top;
        top = top_element(gen);

        r = pcvdom_element_append_element(top, elem);
        if (r) {
            pcvdom_node_destroy(&elem->node);
            FAIL_RET();
        }

        if (!pchvml_token_is_self_closing(token)) {
            r = push_node(gen, &elem->node);
            if (r) {
                pcvdom_node_destroy(&elem->node);
                FAIL_RET();
            }
        }
        else {
            elem->self_closing = 1;
        }

        set_parser_state_if_necessary(gen);

        return 0;
    }
    else if (type==VTT(_END_TAG)) {
        const char *tag = pchvml_token_get_name(token);
        const enum pchvml_tag_id tag_id = tag_id_from_tag(tag);
        if (tag_id == PCHVML_TAG__UNDEF) {
            while (1) {
                struct pcvdom_node *node = top_node(gen);
                struct pcvdom_element *elem;
                elem = container_of(node, struct pcvdom_element, node);
                const char *tagname = pcvdom_element_get_tagname(elem);
                if (tag_id_from_tag(tagname) != PCHVML_TAG__UNDEF)
                    FAIL_RET();

                if (strcmp(tagname, tag)) {
                    pop_node(gen);
                    continue;
                }

                pop_node(gen);
                set_parser_state_if_necessary(gen);

                return 0;
            }
        }
        else if (tag_id == PCHVML_TAG_HVML) {
            pop_node(gen);
            gen->insertion_mode = VGIM(_AFTER_AFTER_BODY);
            set_parser_state_if_necessary(gen);
            return 0;
        }
        else {
            struct pcvdom_node *node = top_node(gen);
            struct pcvdom_element *elem;
            elem = container_of(node, struct pcvdom_element, node);
            const char *tagname = pcvdom_element_get_tagname(elem);
            if (tag_id_from_tag(tagname) != tag_id)
                FAIL_RET();

            pop_node(gen);
            set_parser_state_if_necessary(gen);

            return 0;
        }

        FAIL_RET();
        return 0; // just ignore
    }
    else if (type==VTT(_VCM_TREE)) {
        if (pchvml_token_is_whitespace(token)) {
            return 0; // just ignore
        }
        else {
            return set_vcm_tree(gen, token);
        }
    }
    else if (type==VTT(_EOF)) {
        FAIL_RET();
    }
    else if (type==VTT(_CHARACTER)) {
        FAIL_RET();
    }
    else if (type==VTT(_COMMENT)) {
        r = create_comment(gen, token);

        if (r)
            FAIL_RET();

        return 0;
    }

    FAIL_RET();
    return 0; // just ignore
}

static int
on_mode_before_head(struct pcvdom_gen *gen, struct pchvml_token *token)
{
    int r = 0;
    enum pchvml_token_type type = pchvml_token_get_type(token);
    if (type==VTT(_START_TAG)) {
        const char *tag = pchvml_token_get_name(token);
        const enum pchvml_tag_id tag_id = tag_id_from_tag(tag);
        if (tag_id == PCHVML_TAG_HVML) {
            FAIL_RET();
            return 0; // just ignore
        }
        else if (tag_id == PCHVML_TAG_HEAD) {
            r = create_head(gen, token);
            if (r)
                FAIL_RET();

            return 0;
        }
        else if (tag_id == PCHVML_TAG_BODY) {
            if (create_head(gen, NULL))
                FAIL_RET();
            gen->reprocess = 1;
            return 0;
        }
        else {
#if 0
            if (create_head(gen, NULL)) {
                FAIL_RET();
            }
            if (create_body(gen, NULL)) {
                FAIL_RET();
            }
#endif

            gen->insertion_mode = VGIM(_IN_HVML);
            gen->reprocess = 1;
            return 0;
        }
        FAIL_RET();
    }
    else if (type==VTT(_EOF)) {
        if (create_head(gen, NULL))
            FAIL_RET();
        gen->reprocess = 1;
        return 0;
    }
    else if (type==VTT(_CHARACTER)) {
        FAIL_RET();
    }
    else if (type==VTT(_COMMENT)) {
        r = create_comment(gen, token);

        if (r)
            FAIL_RET();

        return 0;
    }
    else if (type==VTT(_DOCTYPE)) {
        FAIL_RET();
        return 0; // just ignore
    }
    else if (type==VTT(_VCM_TREE)) {
        if (pchvml_token_is_whitespace(token)) {
            return 0; // just ignore
        }
        else {
            return set_vcm_tree(gen, token);
        }
    }
    else if (type==VTT(_END_TAG)) {
        const char *tag = pchvml_token_get_name(token);
        const enum pchvml_tag_id tag_id = tag_id_from_tag(tag);

        if (tag_id == PCHVML_TAG_HVML) {
            pop_node(gen);
            gen->insertion_mode = VGIM(_AFTER_AFTER_BODY);
            set_parser_state_if_necessary(gen);
            return 0;
        }

        FAIL_RET();
    }

    FAIL_RET();
    return 0; // just ignore
}

static int
on_mode_in_head(struct pcvdom_gen *gen, struct pchvml_token *token)
{
    int r = 0;
    enum pchvml_token_type type = pchvml_token_get_type(token);
    if (type==VTT(_START_TAG)) {
        struct pcvdom_element *elem;
        struct pcvdom_element *top;
        const char *tag = pchvml_token_get_name(token);
        const enum pchvml_tag_id tag_id = tag_id_from_tag(tag);
        if (tag_id == PCHVML_TAG_INIT||
            tag_id == PCHVML_TAG_SET ||
            tag_id == PCHVML_TAG_ARCHEDATA ||
            tag_id == PCHVML_TAG_BIND ||
            tag_id == PCHVML_TAG_CONNECT ||
            tag_id == PCHVML_TAG_ARCHETYPE)
        {
            // fall through
        }
        else if (is_tag_of_hvml_verb_cat(tag_id)) {
            // fall through
        }
        else if (tag_id == PCHVML_TAG__UNDEF) {
            // fall through
        }
        else if (tag_id == PCHVML_TAG_ERROR ||
            tag_id == PCHVML_TAG_EXCEPT)
        {
            // fall through
        }
        else {
            FAIL_RET();
            return 0; // ignore for now
        }

        elem = create_element(gen, token);
        if (!elem)
            FAIL_RET();

        top = top_element(gen);

        r = pcvdom_element_append_element(top, elem);
        if (r) {
            pcvdom_node_destroy(&elem->node);
            FAIL_RET();
        }

        if (!pchvml_token_is_self_closing(token)) {
            PC_ASSERT(elem->node.node.parent == &top->node.node);
            r = push_node(gen, &elem->node);
            if (r) {
                pcvdom_node_destroy(&elem->node);
                FAIL_RET();
            }
        }
        else {
            elem->self_closing = 1;
        }

        set_parser_state_if_necessary(gen);

        return 0;
    }
    else if (type==VTT(_END_TAG)) {
        const char *tag = pchvml_token_get_name(token);
        const enum pchvml_tag_id tag_id = tag_id_from_tag(tag);

        if (tag_id == PCHVML_TAG_HEAD) {
            if (is_top_node_of_head(gen)) {
                pop_node(gen);
                gen->insertion_mode = VGIM(_AFTER_HEAD);
                set_parser_state_if_necessary(gen);
                return 0;
            }
            FAIL_RET();
        }

        struct pcvdom_node *node = top_node(gen);
        struct pcvdom_element *elem;
        elem = container_of(node, struct pcvdom_element, node);
        const char *tagname = pcvdom_element_get_tagname(elem);

        if (pcutils_strcasecmp(tagname, tag) == 0) {
            pop_node(gen);
            set_parser_state_if_necessary(gen);
            return 0;
        }

        FAIL_RET();
    }
    else if (type==VTT(_VCM_TREE)) {
        return set_vcm_tree(gen, token);
    }
    else if (type==VTT(_CHARACTER)) {
        FAIL_RET();
    }
    else if (type==VTT(_COMMENT)) {
        r = create_comment(gen, token);

        if (r)
            FAIL_RET();

        return 0;
    }
    else if (type==VTT(_DOCTYPE)) {
        FAIL_RET();
        return 0; // just ignore
    }
    else if (type==VTT(_EOF)) {
        FAIL_RET();
    }

    FAIL_RET();
    return 0; // just ignore
}

static int
on_mode_after_head(struct pcvdom_gen *gen, struct pchvml_token *token)
{
    int r = 0;
    enum pchvml_token_type type = pchvml_token_get_type(token);
    if (type==VTT(_DOCTYPE)) {
        FAIL_RET();
        return 0; // just ignore
    }

    if (type==VTT(_START_TAG)) {
        const char *tag = pchvml_token_get_name(token);
        const enum pchvml_tag_id tag_id = tag_id_from_tag(tag);
        if (tag_id == PCHVML_TAG_BODY) {
            r = create_body(gen, token);
            if (r)
                FAIL_RET();

            return 0;
        }
        FAIL_RET();
    }
    else if (type==VTT(_EOF)) {
        r = create_body(gen, token);
        if (r)
            FAIL_RET();

        pop_node(gen);
        gen->insertion_mode = VGIM(_AFTER_BODY);
        gen->reprocess = 1;

        return 0;
    }
    else if (type==VTT(_CHARACTER)) {
        FAIL_RET();
    }
    else if (type==VTT(_COMMENT)) {
        r = create_comment(gen, token);

        if (r)
            FAIL_RET();

        return 0;
    }
    else if (type==VTT(_VCM_TREE)) {
        if (pchvml_token_is_whitespace(token)) {
            return 0; // just ignore
        }
        else {
            return set_vcm_tree(gen, token);
        }
    }
    else if (type==VTT(_END_TAG)) {
        const char *tag = pchvml_token_get_name(token);
        const enum pchvml_tag_id tag_id = tag_id_from_tag(tag);

        if (tag_id == PCHVML_TAG_HVML) {
            pop_node(gen);
            gen->insertion_mode = VGIM(_AFTER_AFTER_BODY);
            set_parser_state_if_necessary(gen);
            return 0;
        }

        FAIL_RET();
    }

    FAIL_RET();
    return 0; // just ignore
}

static int
on_mode_in_body(struct pcvdom_gen *gen, struct pchvml_token *token)
{
    int r = 0;
    enum pchvml_token_type type = pchvml_token_get_type(token);
    if (type==VTT(_DOCTYPE)) {
        FAIL_RET();
        return 0; // just ignore
    }

    if (type==VTT(_START_TAG)) {
        const char *tag = pchvml_token_get_name(token);
        const enum pchvml_tag_id tag_id = tag_id_from_tag(tag);
        if (tag_id == PCHVML_TAG_INIT||
            tag_id == PCHVML_TAG_SET ||
            tag_id == PCHVML_TAG_ARCHEDATA ||
            tag_id == PCHVML_TAG_BIND ||
            tag_id == PCHVML_TAG_CONNECT ||
            tag_id == PCHVML_TAG_ARCHETYPE)
        {
            // fall through
        }
        else if (is_tag_of_hvml_verb_cat(tag_id)) {
            // fall through
        }
        else if (tag_id == PCHVML_TAG_ERROR ||
            tag_id == PCHVML_TAG_EXCEPT)
        {
            // if (!is_top_node_of_hvml_verb_cat(gen))
            //     FAIL_RET();
            // fall through
        }
        else if (tag_id == PCHVML_TAG__UNDEF) {
            // fall through
        }
        else {
            FAIL_RET();
            return 0; // ignore for now
        }

        struct pcvdom_element *elem;
        elem = create_element(gen, token);
        if (!elem)
            FAIL_RET();

        struct pcvdom_element *top;
        top = top_element(gen);

        r = pcvdom_element_append_element(top, elem);
        if (r) {
            pcvdom_node_destroy(&elem->node);
            FAIL_RET();
        }

        if (!pchvml_token_is_self_closing(token)) {
            r = push_node(gen, &elem->node);
            if (r) {
                pcvdom_node_destroy(&elem->node);
                FAIL_RET();
            }
        }
        else {
            elem->self_closing = 1;
        }

        set_parser_state_if_necessary(gen);

        return 0;
    }
    else if (type==VTT(_END_TAG)) {
        const char *tag = pchvml_token_get_name(token);
        const enum pchvml_tag_id tag_id = tag_id_from_tag(tag);
        if (tag_id == PCHVML_TAG_BODY) {
            if (is_top_node_of_body(gen)) {
                pop_node(gen);
                set_parser_state_if_necessary(gen);
                gen->insertion_mode = VGIM(_AFTER_BODY);
                return 0;
            }
            return 0; // just ignore
        }
        else if (tag_id == PCHVML_TAG__UNDEF) {
            while (1) {
                struct pcvdom_node *node = top_node(gen);
                struct pcvdom_element *elem;
                elem = container_of(node, struct pcvdom_element, node);
                const char *tagname = pcvdom_element_get_tagname(elem);
                if (tag_id_from_tag(tagname) != PCHVML_TAG__UNDEF)
                    FAIL_RET();

                if (strcmp(tagname, tag)) {
                    pop_node(gen);
                    continue;
                }

                pop_node(gen);
                set_parser_state_if_necessary(gen);

                return 0;
            }
        }
        else if (tag_id == PCHVML_TAG_HVML) {
            pop_node(gen);
            gen->insertion_mode = VGIM(_AFTER_AFTER_BODY);
            set_parser_state_if_necessary(gen);
            return 0;
        }
        else {
            struct pcvdom_node *node = top_node(gen);
            struct pcvdom_element *elem;
            elem = container_of(node, struct pcvdom_element, node);
            const char *tagname = pcvdom_element_get_tagname(elem);
            if (tag_id_from_tag(tagname) != tag_id)
                FAIL_RET();

            pop_node(gen);
            set_parser_state_if_necessary(gen);

            return 0;
        }

        FAIL_RET();
        return 0; // just ignore
    }
    else if (type==VTT(_VCM_TREE)) {
        return set_vcm_tree(gen, token);
    }
    else if (type==VTT(_EOF)) {
        FAIL_RET();
    }
    else if (type==VTT(_CHARACTER)) {
        FAIL_RET();
    }
    else if (type==VTT(_COMMENT)) {
        r = create_comment(gen, token);

        if (r)
            FAIL_RET();

        return 0;
    }

    FAIL_RET();
    return 0; // just ignore
}

static int
on_mode_after_body(struct pcvdom_gen *gen, struct pchvml_token *token)
{
    int r = 0;
    enum pchvml_token_type type = pchvml_token_get_type(token);
    if (type==VTT(_DOCTYPE)) {
        FAIL_RET();
        return 0; // just ignore
    }
    else if (type==VTT(_END_TAG)) {
        const char *tag = pchvml_token_get_name(token);
        const enum pchvml_tag_id tag_id = tag_id_from_tag(tag);
        if (tag_id == PCHVML_TAG_HVML) {
            gen->insertion_mode = VGIM(_AFTER_AFTER_BODY);
            return 0;
        }
        else {
            FAIL_RET();
            return 0; // just ignore
        }
    }
    else if (type==VTT(_EOF)) {
        gen->insertion_mode = VGIM(_AFTER_AFTER_BODY);
        gen->reprocess = 1;
        return 0;
    }
    else if (type==VTT(_CHARACTER)) {
        FAIL_RET();
    }
    else if (type==VTT(_COMMENT)) {
        r = create_comment(gen, token);

        if (r)
            FAIL_RET();

        return 0;
    }
    else if (type==VTT(_VCM_TREE)) {
        if (pchvml_token_is_whitespace(token)) {
            return 0; // just ignore
        }
        else {
            return set_vcm_tree(gen, token);
        }
    }
    else if (type==VTT(_START_TAG)) {
        PC_ASSERT(gen->doc);
        PC_ASSERT(gen->doc->body);

        gen->insertion_mode = VGIM(_AFTER_HEAD);
        gen->reprocess = 1;
        return 0;
    }

    FAIL_RET();
    return 0; // just ignore
}

static int
on_mode_after_after_body(struct pcvdom_gen *gen, struct pchvml_token *token)
{
    int r = 0;

    enum pchvml_token_type type = pchvml_token_get_type(token);

    if (type==VTT(_EOF)) {
        if (gen->eof)
            FAIL_RET();

        struct pcvdom_node *node = NULL;
        while ((node=pop_node(gen))) {
            if (is_doc_node(gen, node))
                break;
        }

        gen->eof = 1;

        return 0;
    }
    else if (type==VTT(_CHARACTER)) {
        FAIL_RET();
        return 0; // just ignore
    }
    else if (type==VTT(_COMMENT)) {
        r = create_comment(gen, token);

        if (r)
            FAIL_RET();

        return 0;
    }
    else if (type==VTT(_VCM_TREE)) {
        if (pchvml_token_is_whitespace(token)) {
            return 0; // just ignore
        }
        else {
            return set_vcm_tree(gen, token);
        }
    }

    FAIL_RET();
    return 0; // just ignore
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
        if (push_node(gen, &gen->doc->node)) {
            pcvdom_document_unref(gen->doc);
            gen->doc = NULL;
            FAIL_RET();
        }
        PC_ASSERT(is_doc_node(gen, top_node(gen)));
    }

again:
    gen->reprocess = 0;

    switch (gen->insertion_mode) {
        case VGIM(_INITIAL):
            r = on_mode_initial(gen, token);
            break;
        case VGIM(_BEFORE_HVML):
            r = on_mode_before_hvml(gen, token);
            break;
        case VGIM(_IN_HVML):
            r = on_mode_in_hvml(gen, token);
            break;
        case VGIM(_BEFORE_HEAD):
            r = on_mode_before_head(gen, token);
            break;
        case VGIM(_IN_HEAD):
            r = on_mode_in_head(gen, token);
            break;
        case VGIM(_AFTER_HEAD):
            r = on_mode_after_head(gen, token);
            break;
        case VGIM(_IN_BODY):
            r = on_mode_in_body(gen, token);
            break;
        case VGIM(_AFTER_BODY):
            r = on_mode_after_body(gen, token);
            break;
        case VGIM(_AFTER_AFTER_BODY):
            r = on_mode_after_after_body(gen, token);
            break;
        default:
            PC_ASSERT(0);
            break;
    }

    if (r == 0 && gen->reprocess)
        goto again;

    return r ? -1 : 0;
}

int pchvml_parser_get_curr_pos(struct pchvml_parser* parser,
    uint32_t *character, int *line, int *column, int *position)
{
    if (!parser)
        return -1;

    struct tkz_uc *uc = parser->curr_uc;
    if (!uc)
        return -1;

    if (character)
        *character = uc->character;

    if (line)
        *line = uc->line;

    if (column)
        *column = uc->column;

    if (position)
        *position = uc->position;

    return 0;
}

struct pcvdom_document*
pcvdom_util_document_from_stream(purc_rwstream_t in, struct pcvdom_pos *pos)
{
    struct pchvml_parser *parser = NULL;
    struct pcvdom_gen *gen = NULL;
    struct pcvdom_document *doc = NULL;
    struct pchvml_token *token = NULL;

    PC_ASSERT(in);

    parser = pchvml_create(0, 0);
    if (!parser)
        goto end;

    gen = pcvdom_gen_create();
    if (!gen)
        goto end;

again:
    if (token)
        pchvml_token_destroy(token);

    token = pchvml_next_token(parser, in);
    if (token == NULL) {
        if (pos) {
            int r;
            r = pchvml_parser_get_curr_pos(parser,
                    &pos->c, &pos->line, &pos->col, &pos->pos);
            PC_ASSERT(r == 0);
        }
    }

    if (token && 0==pcvdom_gen_push_token(gen, parser, token)) {
        if (pchvml_token_is_type(token, PCHVML_TOKEN_EOF)) {
            doc = pcvdom_gen_end(gen);
            goto end;
        }
        goto again;
    }

end:
    if (token)
        pchvml_token_destroy(token);

    if (gen)
        pcvdom_gen_destroy(gen);

    if (parser)
        pchvml_destroy(parser);

    return doc;
}

struct pcvdom_document*
pcvdom_util_document_from_buf(const unsigned char *buf, size_t len,
        struct pcvdom_pos *pos)
{
    struct pcvdom_document *doc;
    purc_rwstream_t in;
    in = purc_rwstream_new_from_mem((void*)buf, len);
    if (!in)
        return NULL;

    doc = pcvdom_util_document_from_stream(in, pos);
    purc_rwstream_destroy(in);

    return doc;
}

struct combined_stream {
    purc_rwstream_t           leading;
    purc_rwstream_t           ctnt;
    purc_rwstream_t           tail;

    int                       idx;
};

static ssize_t combined_stream_read(void *ctxt, void *buf, size_t count)
{
    if (count == 0)
        return 0;

    struct combined_stream *rs = (struct combined_stream*)ctxt;
    purc_rwstream_t in;

again:

    switch (rs->idx) {
        case 0:
            in = rs->leading;
            break;
        case 1:
            in = rs->ctnt;
            break;
        case 2:
            in = rs->tail;
            break;
        default:
            purc_set_error(PURC_ERROR_OVERFLOW);
            return -1;
    }

    ssize_t n = purc_rwstream_read(in, buf, count);
    if (n == 0) {
        rs->idx += 1;
        if (rs->idx == 3)
            return count;
        goto again;
    }

    return n;
}

struct pcvdom_element*
parse_fragment(purc_rwstream_t in, struct pcvdom_pos *pos)
{
    struct pcvdom_document *doc;
    doc = pcvdom_util_document_from_stream(in, pos);
    PC_ASSERT(doc);
    if (!doc)
        return NULL;

    struct pcvdom_element *elem = doc->body;
    PC_ASSERT(elem);

    size_t nr;
    nr = pcutils_arrlist_length(doc->bodies);
    for (size_t i=0; i<nr; ++i) {
        void *p = pcutils_arrlist_get_idx(doc->bodies, i);
        struct pcvdom_element *body;
        body = (struct pcvdom_element*)p;
        if (body == doc->body) {
            pcutils_arrlist_del_idx(doc->bodies, i, 1);
            break;
        }
    }

    pcvdom_node_remove(&elem->node);

    doc->body = NULL;
    pcvdom_document_unref(doc);

    return elem;
}

struct pcvdom_element*
pcvdom_util_document_parse_fragment(purc_rwstream_t in,
        struct pcvdom_pos *pos)
{
    if (!in) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }
    UNUSED_PARAM(pos);

    const char *leading = "<hvml><body>";
    const char *tail    = "</body></hvml>";

    struct combined_stream combined = {
        .leading = purc_rwstream_new_from_mem((void*)leading, strlen(leading)),
        .ctnt    = in,
        .tail    = purc_rwstream_new_from_mem((void*)tail, strlen(tail)),
        .idx     = 0,
    };

    purc_rwstream_t rs = NULL;

    if (combined.leading && combined.tail)
        rs = purc_rwstream_new_for_read(&combined, combined_stream_read);

    struct pcvdom_element *elem;
    elem = parse_fragment(rs, pos);

    if (rs) {
        purc_rwstream_destroy(rs);
        rs = NULL;
    }
    if (combined.leading) {
        purc_rwstream_destroy(combined.leading);
        combined.leading = NULL;
    }
    if (combined.tail) {
        purc_rwstream_destroy(combined.tail);
        combined.tail = NULL;
    }

    return elem;
}

struct pcvdom_element*
pcvdom_util_document_parse_fragment_buf(const unsigned char *buf, size_t len,
        struct pcvdom_pos *pos)
{
    purc_rwstream_t in;
    in = purc_rwstream_new_from_mem((void*)buf, len);
    if (!in)
        return NULL;

    pcvdom_element_t elem;
    elem = pcvdom_util_document_parse_fragment(in, pos);
    purc_rwstream_destroy(in);
    return elem;
}


