/**
 * @file tree.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for html node tree.
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


#ifndef PCHTML_HTML_TREE_H
#define PCHTML_HTML_TREE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/dom/interfaces/node.h"
#include "html/dom/interfaces/attr.h"

#include "html/html/base.h"
#include "html/html/node.h"
#include "html/html/tokenizer.h"
#include "html/html/interfaces/document.h"
#include "html/html/tag.h"
#include "html/html/tree/error.h"


typedef bool
(*pchtml_html_tree_insertion_mode_f)(pchtml_html_tree_t *tree,
                                  pchtml_html_token_t *token);

typedef unsigned int
(*pchtml_html_tree_append_attr_f)(pchtml_html_tree_t *tree,
                               pchtml_dom_attr_t *attr, void *ctx);

typedef struct {
    pchtml_array_obj_t *text_list;
    bool               have_non_ws;
}
pchtml_html_tree_pending_table_t;

struct pchtml_html_tree {
    pchtml_html_tokenizer_t           *tkz_ref;

    pchtml_html_document_t            *document;
    pchtml_dom_node_t                 *fragment;

    pchtml_html_form_element_t        *form;

    pchtml_array_t                 *open_elements;
    pchtml_array_t                 *active_formatting;
    pchtml_array_obj_t             *template_insertion_modes;

    pchtml_html_tree_pending_table_t  pending_table;

    pchtml_array_obj_t             *parse_errors;

    bool                           foster_parenting;
    bool                           frameset_ok;
    bool                           scripting;

    pchtml_html_tree_insertion_mode_f mode;
    pchtml_html_tree_insertion_mode_f original_mode;
    pchtml_html_tree_append_attr_f    before_append_attr;

    unsigned int                   status;

    size_t                         ref_count;
};

typedef enum {
    PCHTML_HTML_TREE_INSERTION_POSITION_CHILD  = 0x00,
    PCHTML_HTML_TREE_INSERTION_POSITION_BEFORE = 0x01
}
pchtml_html_tree_insertion_position_t;


pchtml_html_tree_t *
pchtml_html_tree_create(void) WTF_INTERNAL;

unsigned int
pchtml_html_tree_init(pchtml_html_tree_t *tree, 
                pchtml_html_tokenizer_t *tkz) WTF_INTERNAL;

pchtml_html_tree_t *
pchtml_html_tree_ref(pchtml_html_tree_t *tree) WTF_INTERNAL;

pchtml_html_tree_t *
pchtml_html_tree_unref(pchtml_html_tree_t *tree) WTF_INTERNAL;

void
pchtml_html_tree_clean(pchtml_html_tree_t *tree) WTF_INTERNAL;

pchtml_html_tree_t *
pchtml_html_tree_destroy(pchtml_html_tree_t *tree) WTF_INTERNAL;

unsigned int
pchtml_html_tree_stop_parsing(pchtml_html_tree_t *tree) WTF_INTERNAL;

bool
pchtml_html_tree_process_abort(pchtml_html_tree_t *tree) WTF_INTERNAL;

void
pchtml_html_tree_parse_error(pchtml_html_tree_t *tree, 
                pchtml_html_token_t *token,
                pchtml_html_tree_error_id_t id) WTF_INTERNAL;

bool
pchtml_html_tree_construction_dispatcher(pchtml_html_tree_t *tree,
                pchtml_html_token_t *token) WTF_INTERNAL;

pchtml_dom_node_t *
pchtml_html_tree_appropriate_place_inserting_node(pchtml_html_tree_t *tree,
                pchtml_dom_node_t *override_target,
                pchtml_html_tree_insertion_position_t *ipos) WTF_INTERNAL;

pchtml_html_element_t *
pchtml_html_tree_insert_foreign_element(pchtml_html_tree_t *tree,
                pchtml_html_token_t *token, pchtml_ns_id_t ns) WTF_INTERNAL;

pchtml_html_element_t *
pchtml_html_tree_create_element_for_token(pchtml_html_tree_t *tree,
                pchtml_html_token_t *token, pchtml_ns_id_t ns,
                pchtml_dom_node_t *parent) WTF_INTERNAL;

unsigned int
pchtml_html_tree_append_attributes(pchtml_html_tree_t *tree,
                pchtml_dom_element_t *element,
                pchtml_html_token_t *token, pchtml_ns_id_t ns) WTF_INTERNAL;

unsigned int
pchtml_html_tree_append_attributes_from_element(pchtml_html_tree_t *tree,
                pchtml_dom_element_t *element, pchtml_dom_element_t *from,
                pchtml_ns_id_t ns) WTF_INTERNAL;

unsigned int
pchtml_html_tree_adjust_mathml_attributes(pchtml_html_tree_t *tree,
                pchtml_dom_attr_t *attr, void *ctx) WTF_INTERNAL;

unsigned int
pchtml_html_tree_adjust_svg_attributes(pchtml_html_tree_t *tree,
                pchtml_dom_attr_t *attr, void *ctx) WTF_INTERNAL;

unsigned int
pchtml_html_tree_adjust_foreign_attributes(pchtml_html_tree_t *tree,
                pchtml_dom_attr_t *attr, void *ctx) WTF_INTERNAL;

unsigned int
pchtml_html_tree_insert_character(pchtml_html_tree_t *tree, pchtml_html_token_t *token,
                pchtml_dom_node_t **ret_node) WTF_INTERNAL;

unsigned int
pchtml_html_tree_insert_character_for_data(pchtml_html_tree_t *tree,
                pchtml_str_t *str, pchtml_dom_node_t **ret_node) WTF_INTERNAL;

pchtml_dom_comment_t *
pchtml_html_tree_insert_comment(pchtml_html_tree_t *tree,
                pchtml_html_token_t *token, pchtml_dom_node_t *pos) WTF_INTERNAL;

pchtml_dom_document_type_t *
pchtml_html_tree_create_document_type_from_token(pchtml_html_tree_t *tree,
                pchtml_html_token_t *token) WTF_INTERNAL;

void
pchtml_html_tree_node_delete_deep(pchtml_html_tree_t *tree, 
                pchtml_dom_node_t *node) WTF_INTERNAL;

pchtml_html_element_t *
pchtml_html_tree_generic_rawtext_parsing(pchtml_html_tree_t *tree,
                pchtml_html_token_t *token) WTF_INTERNAL;

pchtml_html_element_t *
pchtml_html_tree_generic_rcdata_parsing(pchtml_html_tree_t *tree,
                pchtml_html_token_t *token) WTF_INTERNAL;

void
pchtml_html_tree_generate_implied_end_tags(pchtml_html_tree_t *tree,
                pchtml_tag_id_t ex_tag, pchtml_ns_id_t ex_ns) WTF_INTERNAL;

void
pchtml_html_tree_generate_all_implied_end_tags_thoroughly(pchtml_html_tree_t *tree,
                pchtml_tag_id_t ex_tag, pchtml_ns_id_t ex_ns) WTF_INTERNAL;

void
pchtml_html_tree_reset_insertion_mode_appropriately(pchtml_html_tree_t *tree);

pchtml_dom_node_t *
pchtml_html_tree_element_in_scope(pchtml_html_tree_t *tree, pchtml_tag_id_t tag_id,
                pchtml_ns_id_t ns, pchtml_html_tag_category_t ct) WTF_INTERNAL;

pchtml_dom_node_t *
pchtml_html_tree_element_in_scope_by_node(pchtml_html_tree_t *tree,
                pchtml_dom_node_t *by_node,
                pchtml_html_tag_category_t ct) WTF_INTERNAL;

pchtml_dom_node_t *
pchtml_html_tree_element_in_scope_h123456(pchtml_html_tree_t *tree) WTF_INTERNAL;

pchtml_dom_node_t *
pchtml_html_tree_element_in_scope_tbody_thead_tfoot(
                pchtml_html_tree_t *tree) WTF_INTERNAL;

pchtml_dom_node_t *
pchtml_html_tree_element_in_scope_td_th(pchtml_html_tree_t *tree) WTF_INTERNAL;

bool
pchtml_html_tree_check_scope_element(pchtml_html_tree_t *tree) WTF_INTERNAL;

void
pchtml_html_tree_close_p_element(pchtml_html_tree_t *tree, 
                pchtml_html_token_t *token) WTF_INTERNAL;

bool
pchtml_html_tree_adoption_agency_algorithm(pchtml_html_tree_t *tree,
                pchtml_html_token_t *token, unsigned int *status) WTF_INTERNAL;

bool
pchtml_html_tree_html_integration_point(pchtml_dom_node_t *node) WTF_INTERNAL;

unsigned int
pchtml_html_tree_adjust_attributes_mathml(pchtml_html_tree_t *tree,
                pchtml_dom_attr_t *attr, void *ctx) WTF_INTERNAL;

unsigned int
pchtml_html_tree_adjust_attributes_svg(pchtml_html_tree_t *tree,
                pchtml_dom_attr_t *attr, void *ctx) WTF_INTERNAL;


/*
 * Inline functions
 */
static inline unsigned int
pchtml_html_tree_begin(pchtml_html_tree_t *tree, pchtml_html_document_t *document)
{
    tree->document = document;

    return pchtml_html_tokenizer_begin(tree->tkz_ref);
}

static inline unsigned int
pchtml_html_tree_chunk(pchtml_html_tree_t *tree, const unsigned char *html, size_t size)
{
    return pchtml_html_tokenizer_chunk(tree->tkz_ref, html, size);
}

static inline unsigned int
pchtml_html_tree_end(pchtml_html_tree_t *tree)
{
    return pchtml_html_tokenizer_end(tree->tkz_ref);
}

static inline unsigned int
pchtml_html_tree_build(pchtml_html_tree_t *tree, pchtml_html_document_t *document,
                    const unsigned char *html, size_t size)
{
    tree->status = pchtml_html_tree_begin(tree, document);
    if (tree->status != PCHTML_STATUS_OK) {
        return tree->status;
    }

    tree->status = pchtml_html_tree_chunk(tree, html, size);
    if (tree->status != PCHTML_STATUS_OK) {
        return tree->status;
    }

    return pchtml_html_tree_end(tree);
}

static inline pchtml_dom_node_t *
pchtml_html_tree_create_node(pchtml_html_tree_t *tree,
                          pchtml_tag_id_t tag_id, pchtml_ns_id_t ns)
{
    return (pchtml_dom_node_t *) pchtml_html_interface_create(tree->document,
                                                        tag_id, ns);
}

static inline bool
pchtml_html_tree_node_is(pchtml_dom_node_t *node, pchtml_tag_id_t tag_id)
{
    return node->local_name == tag_id && node->ns == PCHTML_NS_HTML;
}

static inline pchtml_dom_node_t *
pchtml_html_tree_current_node(pchtml_html_tree_t *tree)
{
    if (tree->open_elements->length == 0) {
        return NULL;
    }

    return (pchtml_dom_node_t *)
        tree->open_elements->list[ (tree->open_elements->length - 1) ];
}

static inline pchtml_dom_node_t *
pchtml_html_tree_adjusted_current_node(pchtml_html_tree_t *tree)
{
    if(tree->fragment != NULL && tree->open_elements->length == 1) {
        return pchtml_dom_interface_node(tree->fragment);
    }

    return pchtml_html_tree_current_node(tree);
}

static inline pchtml_html_element_t *
pchtml_html_tree_insert_html_element(pchtml_html_tree_t *tree,
                                  pchtml_html_token_t *token)
{
    return pchtml_html_tree_insert_foreign_element(tree, token, PCHTML_NS_HTML);
}

static inline void
pchtml_html_tree_insert_node(pchtml_dom_node_t *to, pchtml_dom_node_t *node,
                          pchtml_html_tree_insertion_position_t ipos)
{
    if (ipos == PCHTML_HTML_TREE_INSERTION_POSITION_BEFORE) {
        pchtml_dom_node_insert_before(to, node);
        return;
    }

    pchtml_dom_node_insert_child(to, node);
}

/* TODO: if we not need to save parse errors?! */
static inline void
pchtml_html_tree_acknowledge_token_self_closing(pchtml_html_tree_t *tree,
                                             pchtml_html_token_t *token)
{
    if ((token->type & PCHTML_HTML_TOKEN_TYPE_CLOSE_SELF) == 0) {
        return;
    }

    bool is_void = pchtml_html_tag_is_void(token->tag_id);

    if (is_void) {
        pchtml_html_tree_parse_error(tree, token,
                                  PCHTML_HTML_RULES_ERROR_NOVOHTELSTTAWITRSO);
    }
}

static inline bool
pchtml_html_tree_mathml_text_integration_point(pchtml_dom_node_t *node)
{
    if (node->ns == PCHTML_NS_MATH) {
        switch (node->local_name) {
            case PCHTML_TAG_MI:
            case PCHTML_TAG_MO:
            case PCHTML_TAG_MN:
            case PCHTML_TAG_MS:
            case PCHTML_TAG_MTEXT:
                return true;
        }
    }

    return false;
}

static inline bool
pchtml_html_tree_scripting(pchtml_html_tree_t *tree)
{
    return tree->scripting;
}

static inline void
pchtml_html_tree_scripting_set(pchtml_html_tree_t *tree, bool scripting)
{
    tree->scripting = scripting;
}

static inline void
pchtml_html_tree_attach_document(pchtml_html_tree_t *tree, pchtml_html_document_t *doc)
{
    tree->document = doc;
}

/*
 * No inline functions for ABI.
 */
unsigned int
pchtml_html_tree_begin_noi(pchtml_html_tree_t *tree, pchtml_html_document_t *document);

unsigned int
pchtml_html_tree_chunk_noi(pchtml_html_tree_t *tree, const unsigned char *html,
                        size_t size);

unsigned int
pchtml_html_tree_end_noi(pchtml_html_tree_t *tree);

unsigned int
pchtml_html_tree_build_noi(pchtml_html_tree_t *tree, pchtml_html_document_t *document,
                        const unsigned char *html, size_t size);

pchtml_dom_node_t *
pchtml_html_tree_create_node_noi(pchtml_html_tree_t *tree,
                              pchtml_tag_id_t tag_id, pchtml_ns_id_t ns);

bool
pchtml_html_tree_node_is_noi(pchtml_dom_node_t *node, pchtml_tag_id_t tag_id);

pchtml_dom_node_t *
pchtml_html_tree_current_node_noi(pchtml_html_tree_t *tree);

pchtml_dom_node_t *
pchtml_html_tree_adjusted_current_node_noi(pchtml_html_tree_t *tree);

pchtml_html_element_t *
pchtml_html_tree_insert_html_element_noi(pchtml_html_tree_t *tree,
                                      pchtml_html_token_t *token);

void
pchtml_html_tree_insert_node_noi(pchtml_dom_node_t *to, pchtml_dom_node_t *node,
                              pchtml_html_tree_insertion_position_t ipos);

void
pchtml_html_tree_acknowledge_token_self_closing_noi(pchtml_html_tree_t *tree,
                                             pchtml_html_token_t *token);

bool
pchtml_html_tree_mathml_text_integration_point_noi(pchtml_dom_node_t *node);

bool
pchtml_html_tree_scripting_noi(pchtml_html_tree_t *tree);

void
pchtml_html_tree_scripting_set_noi(pchtml_html_tree_t *tree, bool scripting);

void
pchtml_html_tree_attach_document_noi(pchtml_html_tree_t *tree,
                                  pchtml_html_document_t *doc);


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_HTML_TREE_H */
