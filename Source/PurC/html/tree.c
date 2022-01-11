/**
 * @file tree.c
 * @author
 * @date 2021/07/02
 * @brief The complementation of html dom tree.
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

#include "html/tree.h"
#include "html/tree_res.h"
#include "html/tree/insertion_mode.h"
#include "html/tree/open_elements.h"
#include "html/tree/active_formatting.h"
#include "html/tree/template_insertion.h"
#include "html/html_interface.h"
#include "html/html_interface.h"
#include "html/interfaces/template_element.h"
#include "html/interfaces/unknown_element.h"
#include "html/tokenizer/state_rawtext.h"
#include "html/tokenizer/state_rcdata.h"


pcdom_attr_data_t *
pcdom_attr_local_name_append(pcutils_hash_t *hash,
                               const unsigned char *name, size_t length);

pcdom_attr_data_t *
pcdom_attr_qualified_name_append(pcutils_hash_t *hash, const unsigned char *name,
                                   size_t length);

const pchtml_tag_data_t *
pchtml_tag_append_lower(pcutils_hash_t *hash,
                     const unsigned char *name, size_t length);

static pchtml_html_token_t *
pchtml_html_tree_token_callback(pchtml_html_tokenizer_t *tkz,
                             pchtml_html_token_t *token, void *ctx);

static unsigned int
pchtml_html_tree_insertion_mode(pchtml_html_tree_t *tree, pchtml_html_token_t *token);


pchtml_html_tree_t *
pchtml_html_tree_create(void)
{
    return pcutils_calloc(1, sizeof(pchtml_html_tree_t));
}

unsigned int
pchtml_html_tree_init(pchtml_html_tree_t *tree, pchtml_html_tokenizer_t *tkz)
{
    if (tree == NULL) {
        pcinst_set_error (PURC_ERROR_NULL_OBJECT);
        return PCHTML_STATUS_ERROR_OBJECT_IS_NULL;
    }

    if (tkz == NULL) {
        pcinst_set_error (PURC_ERROR_TOO_SMALL_SIZE);
        return PCHTML_STATUS_ERROR_WRONG_ARGS;
    }

    unsigned int status;

    /* Stack of open elements */
    tree->open_elements = pcutils_array_create();
    status = pcutils_array_init(tree->open_elements, 128);
    if (status != PCHTML_STATUS_OK) {
        return status;
    }

    /* Stack of active formatting */
    tree->active_formatting = pcutils_array_create();
    status = pcutils_array_init(tree->active_formatting, 128);
    if (status != PCHTML_STATUS_OK) {
        return status;
    }

    /* Stack of template insertion modes */
    tree->template_insertion_modes = pcutils_array_obj_create();
    status = pcutils_array_obj_init(tree->template_insertion_modes, 64,
                                   sizeof(pchtml_html_tree_template_insertion_t));
    if (status != PCHTML_STATUS_OK) {
        return status;
    }

    /* Stack of pending table character tokens */
    tree->pending_table.text_list = pcutils_array_obj_create();
    status = pcutils_array_obj_init(tree->pending_table.text_list, 16,
                                   sizeof(pcutils_str_t));
    if (status != PCHTML_STATUS_OK) {
        return status;
    }

    /* Parse errors */
    tree->parse_errors = pcutils_array_obj_create();
    status = pcutils_array_obj_init(tree->parse_errors, 16,
                                                sizeof(pchtml_html_tree_error_t));
    if (status != PCHTML_STATUS_OK) {
        return status;
    }

    tree->tkz_ref = pchtml_html_tokenizer_ref(tkz);

    tree->document = NULL;
    tree->fragment = NULL;

    tree->form = NULL;

    tree->foster_parenting = false;
    tree->frameset_ok = true;

    tree->mode = pchtml_html_tree_insertion_mode_initial;
    tree->before_append_attr = NULL;

    tree->status = PCHTML_STATUS_OK;

    tree->ref_count = 1;

    pchtml_html_tokenizer_callback_token_done_set(tkz,
                                               pchtml_html_tree_token_callback,
                                               tree);

    return PCHTML_STATUS_OK;
}

pchtml_html_tree_t *
pchtml_html_tree_ref(pchtml_html_tree_t *tree)
{
    if (tree == NULL) {
        return NULL;
    }

    tree->ref_count++;

    return tree;
}

pchtml_html_tree_t *
pchtml_html_tree_unref(pchtml_html_tree_t *tree)
{
    if (tree == NULL || tree->ref_count == 0) {
        return NULL;
    }

    tree->ref_count--;

    if (tree->ref_count == 0) {
        pchtml_html_tree_destroy(tree);
    }

    return NULL;
}

void
pchtml_html_tree_clean(pchtml_html_tree_t *tree)
{
    pcutils_array_clean(tree->open_elements);
    pcutils_array_clean(tree->active_formatting);
    pcutils_array_obj_clean(tree->template_insertion_modes);
    pcutils_array_obj_clean(tree->pending_table.text_list);
    pcutils_array_obj_clean(tree->parse_errors);

    tree->document = NULL;
    tree->fragment = NULL;

    tree->form = NULL;

    tree->foster_parenting = false;
    tree->frameset_ok = true;

    tree->mode = pchtml_html_tree_insertion_mode_initial;
    tree->before_append_attr = NULL;

    tree->status = PCHTML_STATUS_OK;
}

pchtml_html_tree_t *
pchtml_html_tree_destroy(pchtml_html_tree_t *tree)
{
    if (tree == NULL) {
        return NULL;
    }

    tree->open_elements = pcutils_array_destroy(tree->open_elements, true);
    tree->active_formatting = pcutils_array_destroy(tree->active_formatting,
                                                   true);
    tree->template_insertion_modes = pcutils_array_obj_destroy(tree->template_insertion_modes,
                                                              true);
    tree->pending_table.text_list = pcutils_array_obj_destroy(tree->pending_table.text_list,
                                                             true);

    tree->parse_errors = pcutils_array_obj_destroy(tree->parse_errors, true);
    tree->tkz_ref = pchtml_html_tokenizer_unref(tree->tkz_ref);

    return pcutils_free(tree);
}

static pchtml_html_token_t *
pchtml_html_tree_token_callback(pchtml_html_tokenizer_t *tkz,
                             pchtml_html_token_t *token, void *ctx)
{
    unsigned int status;

    status = pchtml_html_tree_insertion_mode(ctx, token);
    if (status != PCHTML_STATUS_OK) {
        tkz->status = status;
        return NULL;
    }

    return token;
}

/* TODO: not complite!!! */
unsigned int
pchtml_html_tree_stop_parsing(pchtml_html_tree_t *tree)
{
    tree->document->ready_state = PCHTML_HTML_DOCUMENT_READY_STATE_COMPLETE;

    return PCHTML_STATUS_OK;
}

bool
pchtml_html_tree_process_abort(pchtml_html_tree_t *tree)
{
    if (tree->status == PCHTML_STATUS_OK) {
        tree->status = PCHTML_STATUS_ABORTED;
    }

    tree->open_elements->length = 0;
    tree->document->ready_state = PCHTML_HTML_DOCUMENT_READY_STATE_COMPLETE;

    return true;
}

void
pchtml_html_tree_parse_error(pchtml_html_tree_t *tree, pchtml_html_token_t *token,
                          pchtml_html_tree_error_id_t id)
{
    pchtml_html_tree_error_add(tree->parse_errors, token, id);
}

bool
pchtml_html_tree_construction_dispatcher(pchtml_html_tree_t *tree,
                                      pchtml_html_token_t *token)
{
    pcdom_node_t *adjusted;

    adjusted = pchtml_html_tree_adjusted_current_node(tree);

    if (adjusted == NULL || adjusted->ns == PCHTML_NS_HTML) {
        return tree->mode(tree, token);
    }

    if (pchtml_html_tree_mathml_text_integration_point(adjusted))
    {
        if ((token->type & PCHTML_HTML_TOKEN_TYPE_CLOSE) == 0
            && token->tag_id != PCHTML_TAG_MGLYPH
            && token->tag_id != PCHTML_TAG_MALIGNMARK)
        {
            return tree->mode(tree, token);
        }

        if (token->tag_id == PCHTML_TAG__TEXT) {
            return tree->mode(tree, token);
        }
    }

    if (adjusted->local_name == PCHTML_TAG_ANNOTATION_XML
        && adjusted->ns == PCHTML_NS_MATH
        && (token->type & PCHTML_HTML_TOKEN_TYPE_CLOSE) == 0
        && token->tag_id == PCHTML_TAG_SVG)
    {
        return tree->mode(tree, token);
    }

    if (pchtml_html_tree_html_integration_point(adjusted)) {
        if ((token->type & PCHTML_HTML_TOKEN_TYPE_CLOSE) == 0
            || token->tag_id == PCHTML_TAG__TEXT)
        {
            return tree->mode(tree, token);
        }
    }

    if (token->tag_id == PCHTML_TAG__END_OF_FILE) {
        return tree->mode(tree, token);
    }

    return pchtml_html_tree_insertion_mode_foreign_content(tree, token);
}

static unsigned int
pchtml_html_tree_insertion_mode(pchtml_html_tree_t *tree, pchtml_html_token_t *token)
{
    while (pchtml_html_tree_construction_dispatcher(tree, token) == false) {}

    return tree->status;
}

/*
 * Action
 */
pcdom_node_t *
pchtml_html_tree_appropriate_place_inserting_node(pchtml_html_tree_t *tree,
                                       pcdom_node_t *override_target,
                                       pchtml_html_tree_insertion_position_t *ipos)
{
    pcdom_node_t *target, *adjusted_location = NULL;

    *ipos = PCHTML_HTML_TREE_INSERTION_POSITION_CHILD;

    if (override_target != NULL) {
        target = override_target;
    }
    else {
        target = pchtml_html_tree_current_node(tree);
    }

    if (tree->foster_parenting && target->ns == PCHTML_NS_HTML
           && (target->local_name == PCHTML_TAG_TABLE
            || target->local_name == PCHTML_TAG_TBODY
            || target->local_name == PCHTML_TAG_TFOOT
            || target->local_name == PCHTML_TAG_THEAD
            || target->local_name == PCHTML_TAG_TR))
    {
        pcdom_node_t *last_temp, *last_table;
        size_t last_temp_idx, last_table_idx;

        last_temp = pchtml_html_tree_open_elements_find_reverse(tree,
                                                          PCHTML_TAG_TEMPLATE,
                                                          PCHTML_NS_HTML,
                                                          &last_temp_idx);

        last_table = pchtml_html_tree_open_elements_find_reverse(tree,
                                                             PCHTML_TAG_TABLE,
                                                             PCHTML_NS_HTML,
                                                             &last_table_idx);

        if(last_temp != NULL && (last_table == NULL
                         || last_temp_idx > last_table_idx))
        {
            pcdom_document_fragment_t *doc_fragment;

            doc_fragment = pchtml_html_interface_template(last_temp)->content;

            return pcdom_interface_node(doc_fragment);
        }
        else if (last_table == NULL) {
            adjusted_location = pchtml_html_tree_open_elements_first(tree);

            pchtml_assert(adjusted_location != NULL);
            pchtml_assert(adjusted_location->local_name == PCHTML_TAG_HTML);
        }
        else if (last_table->parent != NULL) {
            adjusted_location = last_table;

            *ipos = PCHTML_HTML_TREE_INSERTION_POSITION_BEFORE;
        }
        else {
            pchtml_assert(last_table_idx != 0);

            adjusted_location = pchtml_html_tree_open_elements_get(tree,
                                                            last_table_idx - 1);
        }
    }
    else {
        adjusted_location = target;
    }

    if (adjusted_location == NULL) {
        return NULL;
    }

    /*
     * In Spec it is not entirely clear what is meant:
     *
     * If the adjusted insertion location is inside a template element,
     * let it instead be inside the template element's template contents,
     * after its last child (if any).
     */
    if (pchtml_html_tree_node_is(adjusted_location, PCHTML_TAG_TEMPLATE)) {
        pcdom_document_fragment_t *df;

        df = pchtml_html_interface_template(adjusted_location)->content;
        adjusted_location = pcdom_interface_node(df);
    }

    return adjusted_location;
}

pchtml_html_element_t *
pchtml_html_tree_insert_foreign_element(pchtml_html_tree_t *tree,
                                     pchtml_html_token_t *token, pchtml_ns_id_t ns)
{
    unsigned int status;
    pcdom_node_t *pos;
    pchtml_html_element_t *element;
    pchtml_html_tree_insertion_position_t ipos;

    pos = pchtml_html_tree_appropriate_place_inserting_node(tree, NULL, &ipos);

    if (ipos == PCHTML_HTML_TREE_INSERTION_POSITION_CHILD) {
        element = pchtml_html_tree_create_element_for_token(tree, token, ns, pos);
    }
    else {
        element = pchtml_html_tree_create_element_for_token(tree, token, ns,
                                                         pos->parent);
    }

    if (element == NULL) {
        return NULL;
    }

    if (pos != NULL) {
        pchtml_html_tree_insert_node(pos, pcdom_interface_node(element), ipos);
    }

    status = pchtml_html_tree_open_elements_push(tree,
                                              pcdom_interface_node(element));
    if (status != PCHTML_STATUS_OK) {
        return pchtml_html_interface_destroy(element);
    }

    return element;
}

pchtml_html_element_t *
pchtml_html_tree_create_element_for_token(pchtml_html_tree_t *tree,
                                       pchtml_html_token_t *token, pchtml_ns_id_t ns,
                                       pcdom_node_t *parent)
{
    UNUSED_PARAM(parent);

    pcdom_node_t *node = pchtml_html_tree_create_node(tree, token->tag_id, ns);
    if (node == NULL) {
        return NULL;
    }

    unsigned int status;
    pcdom_element_t *element = pcdom_interface_element(node);

    if (token->base_element == NULL) {
        status = pchtml_html_tree_append_attributes(tree, element, token, ns);
    }
    else {
        status = pchtml_html_tree_append_attributes_from_element(tree, element,
                                                       token->base_element, ns);
    }

    if (status != PCHTML_STATUS_OK) {
        return pchtml_html_interface_destroy(element);
    }

    return pchtml_html_interface_element(node);
}

unsigned int
pchtml_html_tree_append_attributes(pchtml_html_tree_t *tree,
                                pcdom_element_t *element,
                                pchtml_html_token_t *token, pchtml_ns_id_t ns)
{
    unsigned int status;
    pcdom_attr_t *attr;
    pcutils_str_t local_name;
    pchtml_html_token_attr_t *token_attr = token->attr_first;
    pcutils_mraw_t *mraw = element->node.owner_document->text;

    local_name.data = NULL;

    while (token_attr != NULL) {
        attr = pcdom_element_attr_by_local_name_data(element,
                                                       token_attr->name);
        if (attr != NULL) {
            token_attr = token_attr->next;
            continue;
        }

        attr = pcdom_attr_interface_create(element->node.owner_document);
        if (attr == NULL) {
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }

        if (token_attr->value_begin != NULL) {
            status = pcdom_attr_set_value_wo_copy(attr, token_attr->value,
                                                    token_attr->value_size);
            if (status != PCHTML_STATUS_OK) {
                return status;
            }
        }

        attr->node.local_name = token_attr->name->attr_id;
        attr->node.ns = ns;

        /* Fix for adjust MathML/SVG attributes */
        if (tree->before_append_attr != NULL) {
            status = tree->before_append_attr(tree, attr, NULL);
            if (status != PCHTML_STATUS_OK) {
                return status;
            }
        }

        pcdom_element_attr_append(element, attr);

        token_attr = token_attr->next;
    }

    if (local_name.data != NULL) {
        pcutils_mraw_free(mraw, local_name.data);
    }

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_html_tree_append_attributes_from_element(pchtml_html_tree_t *tree,
                                             pcdom_element_t *element,
                                             pcdom_element_t *from,
                                             pchtml_ns_id_t ns)
{
    UNUSED_PARAM(ns);

    unsigned int status;
    pcdom_attr_t *attr = from->first_attr;
    pcdom_attr_t *new_attr;

    while (attr != NULL) {
        new_attr = pcdom_attr_interface_create(element->node.owner_document);
        if (new_attr == NULL) {
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }

        status = pcdom_attr_clone_name_value(attr, new_attr);
        if (status != PCHTML_STATUS_OK) {
            return status;
        }

        new_attr->node.ns = attr->node.ns;

        /* Fix for  adjust MathML/SVG attributes */
        if (tree->before_append_attr != NULL) {
            status = tree->before_append_attr(tree, new_attr, NULL);
            if (status != PCHTML_STATUS_OK) {
                return status;
            }
        }

        pcdom_element_attr_append(element, attr);

        attr = attr->next;
    }

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_html_tree_adjust_mathml_attributes(pchtml_html_tree_t *tree,
                                       pcdom_attr_t *attr, void *ctx)
{
    UNUSED_PARAM(tree);
    UNUSED_PARAM(ctx);

    pcutils_hash_t *attrs;
    const pcdom_attr_data_t *data;

    attrs = attr->node.owner_document->attrs;
    data = pcdom_attr_data_by_id(attrs, attr->node.local_name);

    if (data->entry.length == 13
        && pcutils_str_data_cmp(pcutils_hash_entry_str(&data->entry),
                               (const unsigned char *) "definitionurl"))
    {
        data = pcdom_attr_qualified_name_append(attrs,
                                      (const unsigned char *) "definitionURL", 13);
        if (data == NULL) {
            pcinst_set_error (PURC_ERROR_HTML);
            return PCHTML_STATUS_ERROR;
        }

        attr->qualified_name = data->attr_id;
    }

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_html_tree_adjust_svg_attributes(pchtml_html_tree_t *tree,
                                    pcdom_attr_t *attr, void *ctx)
{
    UNUSED_PARAM(tree);
    UNUSED_PARAM(ctx);

    pcutils_hash_t *attrs;
    const pcdom_attr_data_t *data;
    const pchtml_html_tree_res_attr_adjust_t *adjust;

    size_t len = sizeof(pchtml_html_tree_res_attr_adjust_svg_map)
        / sizeof(pchtml_html_tree_res_attr_adjust_t);

    attrs = attr->node.owner_document->attrs;

    data = pcdom_attr_data_by_id(attrs, attr->node.local_name);

    for (size_t i = 0; i < len; i++) {
        adjust = &pchtml_html_tree_res_attr_adjust_svg_map[i];

        if (data->entry.length == adjust->len
            && pcutils_str_data_cmp(pcutils_hash_entry_str(&data->entry),
                                   (const unsigned char *) adjust->from))
        {
            data = pcdom_attr_qualified_name_append(attrs,
                                (const unsigned char *) adjust->to, adjust->len);
            if (data == NULL) {
                pcinst_set_error (PURC_ERROR_HTML);
                return PCHTML_STATUS_ERROR;
            }

            attr->qualified_name = data->attr_id;

            return PCHTML_STATUS_OK;
        }
    }

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_html_tree_adjust_foreign_attributes(pchtml_html_tree_t *tree,
                                        pcdom_attr_t *attr, void *ctx)
{
    UNUSED_PARAM(tree);
    UNUSED_PARAM(ctx);

    size_t lname_length;
    pcutils_hash_t *tags, *attrs, *prefix;
    const pchtml_tag_data_t *tag_data;
    const pchtml_ns_prefix_data_t *prefix_data;
    const pcdom_attr_data_t *data;
    const pchtml_html_tree_res_attr_adjust_foreign_t *adjust;

    size_t len = sizeof(pchtml_html_tree_res_attr_adjust_foreign_map)
        / sizeof(pchtml_html_tree_res_attr_adjust_foreign_t);

    tags = attr->node.owner_document->tags;
    attrs = attr->node.owner_document->attrs;
    prefix = attr->node.owner_document->prefix;

    data = pcdom_attr_data_by_id(attrs, attr->node.local_name);

    for (size_t i = 0; i < len; i++) {
        adjust = &pchtml_html_tree_res_attr_adjust_foreign_map[i];

        if (data->entry.length == adjust->name_len
            && pcutils_str_data_cmp(pcutils_hash_entry_str(&data->entry),
                                   (const unsigned char *) adjust->name))
        {
            if (adjust->prefix_len != 0) {
                data = pcdom_attr_qualified_name_append(attrs,
                           (const unsigned char *) adjust->name, adjust->name_len);
                if (data == NULL) {
                    pcinst_set_error (PURC_ERROR_HTML);
                    return PCHTML_STATUS_ERROR;
                }

                attr->qualified_name = data->attr_id;

                lname_length = adjust->name_len - adjust->prefix_len - 1;

                tag_data = pchtml_tag_append_lower(tags,
                         (const unsigned char *) adjust->local_name, lname_length);
                if (tag_data == NULL) {
                    pcinst_set_error (PURC_ERROR_HTML);
                    return PCHTML_STATUS_ERROR;
                }

                attr->node.local_name = tag_data->tag_id;

                prefix_data = pchtml_ns_prefix_append(prefix,
                       (const unsigned char *) adjust->prefix, adjust->prefix_len);
                if (prefix_data == NULL) {
                    pcinst_set_error (PURC_ERROR_HTML);
                    return PCHTML_STATUS_ERROR;
                }

                attr->node.prefix = prefix_data->prefix_id;
            }

            attr->node.ns = adjust->ns;

            return PCHTML_STATUS_OK;
        }
    }

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_html_tree_insert_character(pchtml_html_tree_t *tree, pchtml_html_token_t *token,
                               pcdom_node_t **ret_node)
{
    size_t size;
    unsigned int status;
    pcutils_str_t str = {0};

    size = token->text_end - token->text_start;

    pcutils_str_init(&str, tree->document->dom_document.text, size + 1);
    if (str.data == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }

    memcpy(str.data, token->text_start, size);

    str.data[size] = 0x00;
    str.length = size;

    status = pchtml_html_tree_insert_character_for_data(tree, &str, ret_node);
    if (status != PCHTML_STATUS_OK) {
        return status;
    }

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_html_tree_insert_character_for_data(pchtml_html_tree_t *tree,
                                        pcutils_str_t *str,
                                        pcdom_node_t **ret_node)
{
    const unsigned char *data;
    pcdom_node_t *pos;
    pcdom_character_data_t *chrs = NULL;
    pchtml_html_tree_insertion_position_t ipos;

    if (ret_node != NULL) {
        *ret_node = NULL;
    }

    pos = pchtml_html_tree_appropriate_place_inserting_node(tree, NULL, &ipos);
    if (pos == NULL) {
        pcinst_set_error (PURC_ERROR_HTML);
        return PCHTML_STATUS_ERROR;
    }

    if (pchtml_html_tree_node_is(pos, PCHTML_TAG__DOCUMENT)) {
        goto destroy_str;
    }

    if (ipos == PCHTML_HTML_TREE_INSERTION_POSITION_BEFORE) {
        /* No need check namespace */
        if (pos->prev != NULL && pos->prev->local_name == PCHTML_TAG__TEXT) {
            chrs = pcdom_interface_character_data(pos->prev);

            if (ret_node != NULL) {
                *ret_node = pos->prev;
            }
        }
    }
    else {
        /* No need check namespace */
        if (pos->last_child != NULL
            && pos->last_child->local_name == PCHTML_TAG__TEXT)
        {
            chrs = pcdom_interface_character_data(pos->last_child);

            if (ret_node != NULL) {
                *ret_node = pos->last_child;
            }
        }
    }

    if (chrs != NULL) {
        /* This is error. This can not happen, but... */
        if (chrs->data.data == NULL) {
            data = pcutils_str_init(&chrs->data, tree->document->dom_document.text,
                                   str->length);
            if (data == NULL) {
                pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
                return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
            }
        }

        data = pcutils_str_append(&chrs->data, tree->document->dom_document.text,
                                 str->data, str->length);
        if (data == NULL) {
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }

        goto destroy_str;
    }

    pcdom_node_t *text = pchtml_html_tree_create_node(tree, PCHTML_TAG__TEXT,
                                                     PCHTML_NS_HTML);
    if (text == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }

    pcdom_interface_text(text)->char_data.data = *str;

    if (ret_node != NULL) {
        *ret_node = text;
    }

    pchtml_html_tree_insert_node(pos, text, ipos);

    return PCHTML_STATUS_OK;

destroy_str:

    pcutils_str_destroy(str, tree->document->dom_document.text, false);

    return PCHTML_STATUS_OK;
}

pcdom_comment_t *
pchtml_html_tree_insert_comment(pchtml_html_tree_t *tree,
                             pchtml_html_token_t *token, pcdom_node_t *pos)
{
    pcdom_node_t *node;
    pcdom_comment_t *comment;
    pchtml_html_tree_insertion_position_t ipos;

    if (pos == NULL) {
        pos = pchtml_html_tree_appropriate_place_inserting_node(tree, NULL, &ipos);
    }
    else {
        ipos = PCHTML_HTML_TREE_INSERTION_POSITION_CHILD;
    }

    pchtml_assert(pos != NULL);

    node = pchtml_html_tree_create_node(tree, token->tag_id, pos->ns);
    comment = pcdom_interface_comment(node);

    if (comment == NULL) {
        return NULL;
    }

    tree->status = pchtml_html_token_make_text(token, &comment->char_data.data,
                                            tree->document->dom_document.text);
    if (tree->status != PCHTML_STATUS_OK) {
        return NULL;
    }

    pchtml_html_tree_insert_node(pos, node, ipos);

    return comment;
}

pcdom_document_type_t *
pchtml_html_tree_create_document_type_from_token(pchtml_html_tree_t *tree,
                                              pchtml_html_token_t *token)
{
    unsigned int status;
    pcdom_node_t *doctype_node;
    pcdom_document_type_t *doc_type;

    /* Create */
    doctype_node = pchtml_html_tree_create_node(tree, token->tag_id, PCHTML_NS_HTML);
    if (doctype_node == NULL) {
        return NULL;
    }

    doc_type = pcdom_interface_document_type(doctype_node);

    /* Parse */
    status = pchtml_html_token_doctype_parse(token, doc_type);
    if (status != PCHTML_STATUS_OK) {
        return pcdom_document_type_interface_destroy(doc_type);
    }

    return doc_type;
}

/*
 * TODO: need use ref and unref for nodes (ref counter)
 * Not implemented until the end. It is necessary to finish it.
 */
void
pchtml_html_tree_node_delete_deep(pchtml_html_tree_t *tree, pcdom_node_t *node)
{
    UNUSED_PARAM(tree);

    pcdom_node_remove(node);
}

pchtml_html_element_t *
pchtml_html_tree_generic_rawtext_parsing(pchtml_html_tree_t *tree,
                                      pchtml_html_token_t *token)
{
    pchtml_html_element_t *element;

    element = pchtml_html_tree_insert_html_element(tree, token);
    if (element == NULL) {
        return NULL;
    }

    /*
     * Need for tokenizer state RAWTEXT
     * See description for 'pchtml_html_tokenizer_state_rawtext_before' function
     */
    pchtml_html_tokenizer_tmp_tag_id_set(tree->tkz_ref, token->tag_id);
    pchtml_html_tokenizer_state_set(tree->tkz_ref,
                                 pchtml_html_tokenizer_state_rawtext_before);

    tree->original_mode = tree->mode;
    tree->mode = pchtml_html_tree_insertion_mode_text;

    return element;
}

/* Magic of CopyPast power! */
pchtml_html_element_t *
pchtml_html_tree_generic_rcdata_parsing(pchtml_html_tree_t *tree,
                                     pchtml_html_token_t *token)
{
    pchtml_html_element_t *element;

    element = pchtml_html_tree_insert_html_element(tree, token);
    if (element == NULL) {
        return NULL;
    }

    /*
     * Need for tokenizer state RCDATA
     * See description for 'pchtml_html_tokenizer_state_rcdata_before' function
     */
    pchtml_html_tokenizer_tmp_tag_id_set(tree->tkz_ref, token->tag_id);
    pchtml_html_tokenizer_state_set(tree->tkz_ref,
                                 pchtml_html_tokenizer_state_rcdata_before);

    tree->original_mode = tree->mode;
    tree->mode = pchtml_html_tree_insertion_mode_text;

    return element;
}

void
pchtml_html_tree_generate_implied_end_tags(pchtml_html_tree_t *tree,
                                        pchtml_tag_id_t ex_tag, pchtml_ns_id_t ex_ns)
{
    pcdom_node_t *node;

    pchtml_assert(tree->open_elements != 0);

    while (pcutils_array_length(tree->open_elements) != 0) {
        node = pchtml_html_tree_current_node(tree);

        pchtml_assert(node != NULL);

        switch (node->local_name) {
            case PCHTML_TAG_DD:
            case PCHTML_TAG_DT:
            case PCHTML_TAG_LI:
            case PCHTML_TAG_OPTGROUP:
            case PCHTML_TAG_OPTION:
            case PCHTML_TAG_P:
            case PCHTML_TAG_RB:
            case PCHTML_TAG_RP:
            case PCHTML_TAG_RT:
            case PCHTML_TAG_RTC:
                if(node->local_name == ex_tag && node->ns == ex_ns) {
                    return;
                }

                pchtml_html_tree_open_elements_pop(tree);

                break;

            default:
                return;
        }
    }
}

void
pchtml_html_tree_generate_all_implied_end_tags_thoroughly(pchtml_html_tree_t *tree,
                                                       pchtml_tag_id_t ex_tag,
                                                       pchtml_ns_id_t ex_ns)
{
    pcdom_node_t *node;

    pchtml_assert(tree->open_elements != 0);

    while (pcutils_array_length(tree->open_elements) != 0) {
        node = pchtml_html_tree_current_node(tree);

        pchtml_assert(node != NULL);

        switch (node->local_name) {
            case PCHTML_TAG_CAPTION:
            case PCHTML_TAG_COLGROUP:
            case PCHTML_TAG_DD:
            case PCHTML_TAG_DT:
            case PCHTML_TAG_LI:
            case PCHTML_TAG_OPTGROUP:
            case PCHTML_TAG_OPTION:
            case PCHTML_TAG_P:
            case PCHTML_TAG_RB:
            case PCHTML_TAG_RP:
            case PCHTML_TAG_RT:
            case PCHTML_TAG_RTC:
            case PCHTML_TAG_TBODY:
            case PCHTML_TAG_TD:
            case PCHTML_TAG_TFOOT:
            case PCHTML_TAG_TH:
            case PCHTML_TAG_THEAD:
            case PCHTML_TAG_TR:
                if(node->local_name == ex_tag && node->ns == ex_ns) {
                    return;
                }

                pchtml_html_tree_open_elements_pop(tree);

                break;

            default:
                return;
        }
    }
}

void
pchtml_html_tree_reset_insertion_mode_appropriately(pchtml_html_tree_t *tree)
{
    pcdom_node_t *node;
    size_t idx = tree->open_elements->length;

    /* Step 1 */
    bool last = false;
    void **list = tree->open_elements->list;

    /* Step 3 */
    while (idx != 0) {
        idx--;

        /* Step 2 */
        node = list[idx];

        /* Step 3 */
        if (idx == 0) {
            last = true;

            if (tree->fragment != NULL) {
                node = tree->fragment;
            }
        }

        pchtml_assert(node != NULL);

        /* Step 16 */
        if (node->ns != PCHTML_NS_HTML) {
            if (last) {
                tree->mode = pchtml_html_tree_insertion_mode_in_body;
                return;
            }

            continue;
        }

        /* Step 4 */
        if (node->local_name == PCHTML_TAG_SELECT) {
            /* Step 4.1 */
            if (last) {
                tree->mode = pchtml_html_tree_insertion_mode_in_select;
                return;
            }

            /* Step 4.2 */
            size_t ancestor = idx;

            for (;;) {
                /* Step 4.3 */
                if (ancestor == 0) {
                    tree->mode = pchtml_html_tree_insertion_mode_in_select;
                    return;
                }

                /* Step 4.4 */
                ancestor--;

                /* Step 4.5 */
                pcdom_node_t *ancestor_node = list[ancestor];

                if(pchtml_html_tree_node_is(ancestor_node, PCHTML_TAG_TEMPLATE)) {
                    tree->mode = pchtml_html_tree_insertion_mode_in_select;
                    return;
                }

                /* Step 4.6 */
                else if(pchtml_html_tree_node_is(ancestor_node, PCHTML_TAG_TABLE)) {
                    tree->mode = pchtml_html_tree_insertion_mode_in_select_in_table;
                    return;
                }
            }
        }

        /* Step 5-15 */
        switch (node->local_name) {
            case PCHTML_TAG_TD:
            case PCHTML_TAG_TH:
                if (last == false) {
                    tree->mode = pchtml_html_tree_insertion_mode_in_cell;
                    return;
                }

                break;

            case PCHTML_TAG_TR:
                tree->mode = pchtml_html_tree_insertion_mode_in_row;
                return;

            case PCHTML_TAG_TBODY:
            case PCHTML_TAG_TFOOT:
            case PCHTML_TAG_THEAD:
                tree->mode = pchtml_html_tree_insertion_mode_in_table_body;
                return;

            case PCHTML_TAG_CAPTION:
                tree->mode = pchtml_html_tree_insertion_mode_in_caption;
                return;

            case PCHTML_TAG_COLGROUP:
                tree->mode = pchtml_html_tree_insertion_mode_in_column_group;
                return;

            case PCHTML_TAG_TABLE:
                tree->mode = pchtml_html_tree_insertion_mode_in_table;
                return;

            case PCHTML_TAG_TEMPLATE:
                tree->mode = pchtml_html_tree_template_insertion_current(tree);

                pchtml_assert(tree->mode != NULL);

                return;

            case PCHTML_TAG_HEAD:
                if (last == false) {
                    tree->mode = pchtml_html_tree_insertion_mode_in_head;
                    return;
                }

                break;

            case PCHTML_TAG_BODY:
                tree->mode = pchtml_html_tree_insertion_mode_in_body;
                return;

            case PCHTML_TAG_FRAMESET:
                tree->mode = pchtml_html_tree_insertion_mode_in_frameset;
                return;

            case PCHTML_TAG_HTML: {
                if (tree->document->head == NULL) {
                    tree->mode = pchtml_html_tree_insertion_mode_before_head;
                    return;
                }

                tree->mode = pchtml_html_tree_insertion_mode_after_head;
                return;
            }

            default:
                break;
        }

        /* Step 16 */
        if (last) {
            tree->mode = pchtml_html_tree_insertion_mode_in_body;
            return;
        }
    }
}

pcdom_node_t *
pchtml_html_tree_element_in_scope(pchtml_html_tree_t *tree, pchtml_tag_id_t tag_id,
                               pchtml_ns_id_t ns, pchtml_html_tag_category_t ct)
{
    pcdom_node_t *node;

    size_t idx = tree->open_elements->length;
    void **list = tree->open_elements->list;

    while (idx != 0) {
        idx--;
        node = list[idx];

        if (node->local_name == tag_id && node->ns == ns) {
            return node;
        }

        if (pchtml_html_tag_is_category(node->local_name, node->ns, ct)) {
            return NULL;
        }
    }

    return NULL;
}

pcdom_node_t *
pchtml_html_tree_element_in_scope_by_node(pchtml_html_tree_t *tree,
                                       pcdom_node_t *by_node,
                                       pchtml_html_tag_category_t ct)
{
    pcdom_node_t *node;

    size_t idx = tree->open_elements->length;
    void **list = tree->open_elements->list;

    while (idx != 0) {
        idx--;
        node = list[idx];

        if (node == by_node) {
            return node;
        }

        if (pchtml_html_tag_is_category(node->local_name, node->ns, ct)) {
            return NULL;
        }
    }

    return NULL;
}

pcdom_node_t *
pchtml_html_tree_element_in_scope_h123456(pchtml_html_tree_t *tree)
{
    pcdom_node_t *node;

    size_t idx = tree->open_elements->length;
    void **list = tree->open_elements->list;

    while (idx != 0) {
        idx--;
        node = list[idx];

        switch (node->local_name) {
            case PCHTML_TAG_H1:
            case PCHTML_TAG_H2:
            case PCHTML_TAG_H3:
            case PCHTML_TAG_H4:
            case PCHTML_TAG_H5:
            case PCHTML_TAG_H6:
                if (node->ns == PCHTML_NS_HTML) {
                    return node;
                }

                break;

            default:
                break;
        }

        if (pchtml_html_tag_is_category(node->local_name, PCHTML_NS_HTML,
                                     PCHTML_HTML_TAG_CATEGORY_SCOPE))
        {
            return NULL;
        }
    }

    return NULL;
}

pcdom_node_t *
pchtml_html_tree_element_in_scope_tbody_thead_tfoot(pchtml_html_tree_t *tree)
{
    pcdom_node_t *node;

    size_t idx = tree->open_elements->length;
    void **list = tree->open_elements->list;

    while (idx != 0) {
        idx--;
        node = list[idx];

        switch (node->local_name) {
            case PCHTML_TAG_TBODY:
            case PCHTML_TAG_THEAD:
            case PCHTML_TAG_TFOOT:
                if (node->ns == PCHTML_NS_HTML) {
                    return node;
                }

                break;

            default:
                break;
        }

        if (pchtml_html_tag_is_category(node->local_name, PCHTML_NS_HTML,
                                     PCHTML_HTML_TAG_CATEGORY_SCOPE_TABLE))
        {
            return NULL;
        }
    }

    return NULL;
}

pcdom_node_t *
pchtml_html_tree_element_in_scope_td_th(pchtml_html_tree_t *tree)
{
    pcdom_node_t *node;

    size_t idx = tree->open_elements->length;
    void **list = tree->open_elements->list;

    while (idx != 0) {
        idx--;
        node = list[idx];

        switch (node->local_name) {
            case PCHTML_TAG_TD:
            case PCHTML_TAG_TH:
                if (node->ns == PCHTML_NS_HTML) {
                    return node;
                }

                break;

            default:
                break;
        }

        if (pchtml_html_tag_is_category(node->local_name, PCHTML_NS_HTML,
                                     PCHTML_HTML_TAG_CATEGORY_SCOPE_TABLE))
        {
            return NULL;
        }
    }

    return NULL;
}

bool
pchtml_html_tree_check_scope_element(pchtml_html_tree_t *tree)
{
    pcdom_node_t *node;

    for (size_t i = 0; i < tree->open_elements->length; i++) {
        node = tree->open_elements->list[i];

        switch (node->local_name) {
            case PCHTML_TAG_DD:
            case PCHTML_TAG_DT:
            case PCHTML_TAG_LI:
            case PCHTML_TAG_OPTGROUP:
            case PCHTML_TAG_OPTION:
            case PCHTML_TAG_P:
            case PCHTML_TAG_RB:
            case PCHTML_TAG_RP:
            case PCHTML_TAG_RT:
            case PCHTML_TAG_RTC:
            case PCHTML_TAG_TBODY:
            case PCHTML_TAG_TD:
            case PCHTML_TAG_TFOOT:
            case PCHTML_TAG_TH:
            case PCHTML_TAG_THEAD:
            case PCHTML_TAG_TR:
            case PCHTML_TAG_BODY:
            case PCHTML_TAG_HTML:
                return true;

            default:
                break;
        }
    }

    return false;
}

void
pchtml_html_tree_close_p_element(pchtml_html_tree_t *tree, pchtml_html_token_t *token)
{
    pchtml_html_tree_generate_implied_end_tags(tree, PCHTML_TAG_P, PCHTML_NS_HTML);

    pcdom_node_t *node = pchtml_html_tree_current_node(tree);

    if (pchtml_html_tree_node_is(node, PCHTML_TAG_P) == false) {
        pchtml_html_tree_parse_error(tree, token,
                                  PCHTML_HTML_RULES_ERROR_UNELINOPELST);
    }

    pchtml_html_tree_open_elements_pop_until_tag_id(tree, PCHTML_TAG_P, PCHTML_NS_HTML,
                                                 true);
}

#include "html/serialize.h"

bool
pchtml_html_tree_adoption_agency_algorithm(pchtml_html_tree_t *tree,
                                        pchtml_html_token_t *token,
                                        unsigned int *status)
{
    pchtml_assert(tree->open_elements->length != 0);

    /* State 1 */
    bool is;
    short outer_loop;
    pchtml_html_element_t *element;
    pcdom_node_t *node, *marker, **oel_list, **afe_list;

    pchtml_tag_id_t subject = token->tag_id;

    oel_list = (pcdom_node_t **) tree->open_elements->list;
    afe_list = (pcdom_node_t **) tree->active_formatting->list;
    marker = (pcdom_node_t *) pchtml_html_tree_active_formatting_marker();

    *status = PCHTML_STATUS_OK;

    /* State 2 */
    node = pchtml_html_tree_current_node(tree);
    pchtml_assert(node != NULL);

    if (pchtml_html_tree_node_is(node, subject)) {
        is = pchtml_html_tree_active_formatting_find_by_node_reverse(tree, node,
                                                                  NULL);
        if (is == false) {
            pchtml_html_tree_open_elements_pop(tree);

            return false;
        }
    }

    /* State 3 */
    outer_loop = 0;

    /* State 4 */
    while (outer_loop < 8) {
        /* State 5 */
        outer_loop++;

        /* State 6 */
        size_t formatting_index = 0;
        size_t idx = tree->active_formatting->length;
        pcdom_node_t *formatting_element = NULL;

        while (idx) {
            idx--;

            if (afe_list[idx] == marker) {
                    return true;
            }
            else if (afe_list[idx]->local_name == subject) {
                formatting_index = idx;
                formatting_element = afe_list[idx];

                break;
            }
        }

        if (formatting_element == NULL) {
            return true;
        }

        /* State 7 */
        size_t oel_formatting_idx;
        is = pchtml_html_tree_open_elements_find_by_node_reverse(tree,
                                                              formatting_element,
                                                              &oel_formatting_idx);
        if (is == false) {
            pchtml_html_tree_parse_error(tree, token,
                                      PCHTML_HTML_RULES_ERROR_MIELINOPELST);

            pchtml_html_tree_active_formatting_remove_by_node(tree,
                                                           formatting_element);

            return false;
        }

        /* State 8 */
        node = pchtml_html_tree_element_in_scope_by_node(tree, formatting_element,
                                                      PCHTML_HTML_TAG_CATEGORY_SCOPE);
        if (node == NULL) {
            pchtml_html_tree_parse_error(tree, token,
                                      PCHTML_HTML_RULES_ERROR_MIELINSC);
            return false;
        }

        /* State 9 */
        node = pchtml_html_tree_current_node(tree);

        if (formatting_element != node) {
            pchtml_html_tree_parse_error(tree, token,
                                      PCHTML_HTML_RULES_ERROR_UNELINOPELST);
        }

        /* State 10 */
        pcdom_node_t *furthest_block = NULL;
        size_t furthest_block_idx = 0;
        size_t oel_idx = tree->open_elements->length;

        for (furthest_block_idx = oel_formatting_idx;
             furthest_block_idx < oel_idx;
             furthest_block_idx++)
        {
            is = pchtml_html_tag_is_category(oel_list[furthest_block_idx]->local_name,
                                          oel_list[furthest_block_idx]->ns,
                                          PCHTML_HTML_TAG_CATEGORY_SPECIAL);
            if (is) {
                furthest_block = oel_list[furthest_block_idx];

                break;
            }
        }

        /* State 11 */
        if (furthest_block == NULL) {
            pchtml_html_tree_open_elements_pop_until_node(tree, formatting_element,
                                                       true);

            pchtml_html_tree_active_formatting_remove_by_node(tree,
                                                           formatting_element);

            return false;
        }

        pchtml_assert(oel_formatting_idx != 0);

        /* State 12 */
        pcdom_node_t *common_ancestor = oel_list[oel_formatting_idx - 1];

        /* State 13 */
        size_t bookmark = formatting_index;

        /* State 14 */
        pcdom_node_t *node;
        pcdom_node_t *last = furthest_block;
        size_t node_idx = furthest_block_idx;

        /* State 14.1 */
        size_t inner_loop_counter = 0;

        /* State 14.2 */
        while (1) {
            inner_loop_counter++;

            /* State 14.3 */
            pchtml_assert(node_idx != 0);

            if (node_idx == 0) {
                return false;
            }

            node_idx--;
            node = oel_list[node_idx];

            /* State 14.4 */
            if (node == formatting_element) {
                break;
            }

            /* State 14.5 */
            size_t afe_node_idx;
            is = pchtml_html_tree_active_formatting_find_by_node_reverse(tree,
                                                                      node,
                                                                      &afe_node_idx);
            /* State 14.5 */
            if (inner_loop_counter > 3 && is) {
                pchtml_html_tree_active_formatting_remove_by_node(tree, node);

                continue;
            }

            /* State 14.6 */
            if (is == false) {
                pchtml_html_tree_open_elements_remove_by_node(tree, node);

                continue;
            }

            /* State 14.7 */
            pchtml_html_token_t fake_token = {0};

            fake_token.tag_id = node->local_name;
            fake_token.base_element = node;

            element = pchtml_html_tree_create_element_for_token(tree, &fake_token,
                                                             PCHTML_NS_HTML,
                                                             common_ancestor);
            if (element == NULL) {
                pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
                *status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

                return false;
            }

            node = pcdom_interface_node(element);

            afe_list[afe_node_idx] = node;
            oel_list[node_idx] = node;

            /* State 14.8 */
            if (last == furthest_block) {
                bookmark = afe_node_idx + 1;

                pchtml_assert(bookmark < tree->active_formatting->length);
            }

            /* State 14.9 */
            if (last->parent != NULL) {
                pcdom_node_remove(last);
            }

            pcdom_node_insert_child(node, last);

            /* State 14.10 */
            last = node;
        }

        if (last->parent != NULL) {
            pcdom_node_remove(last);
        }

        /* State 15 */
        pcdom_node_t *pos;
        pchtml_html_tree_insertion_position_t ipos;

        pos = pchtml_html_tree_appropriate_place_inserting_node(tree,
                                                             common_ancestor,
                                                             &ipos);
        if (pos == NULL) {
            return false;
        }

        pchtml_html_tree_insert_node(pos, last, ipos);

        /* State 16 */
        pchtml_html_token_t fake_token = {0};

        fake_token.tag_id = formatting_element->local_name;
        fake_token.base_element = formatting_element;

        element = pchtml_html_tree_create_element_for_token(tree, &fake_token,
                                                         PCHTML_NS_HTML,
                                                         furthest_block);
        if (element == NULL) {
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            *status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

            return false;
        }

        /* State 17 */
        pcdom_node_t *next;
        node = furthest_block->first_child;

        while (node != NULL) {
            next = node->next;

            pcdom_node_remove(node);
            pcdom_node_insert_child(pcdom_interface_node(element), node);

            node = next;
        }

        node = pcdom_interface_node(element);

        /* State 18 */
        pcdom_node_insert_child(furthest_block, node);

        /* State 19 */
        pchtml_html_tree_active_formatting_remove(tree, formatting_index);

        if (bookmark > tree->active_formatting->length) {
            bookmark = tree->active_formatting->length;
        }

        *status = pchtml_html_tree_active_formatting_insert(tree, node, bookmark);
        if (*status != PCHTML_STATUS_OK) {
            return false;
        }

        /* State 20 */
        pchtml_html_tree_open_elements_remove_by_node(tree, formatting_element);

        pchtml_html_tree_open_elements_find_by_node(tree, furthest_block,
                                                 &furthest_block_idx);

        *status = pchtml_html_tree_open_elements_insert_after(tree, node,
                                                           furthest_block_idx);
        if (*status != PCHTML_STATUS_OK) {
            return false;
        }
    }

    return false;
}

bool
pchtml_html_tree_html_integration_point(pcdom_node_t *node)
{
    if (node->ns == PCHTML_NS_MATH
        && node->local_name == PCHTML_TAG_ANNOTATION_XML)
    {
        pcdom_attr_t *attr;
        attr = pcdom_element_attr_is_exist(pcdom_interface_element(node),
                                             (const unsigned char *) "encoding",
                                             8);
        if (attr == NULL || attr->value == NULL) {
            return NULL;
        }

        if (attr->value->length == 9
            && pcutils_str_data_casecmp(attr->value->data,
                                       (const unsigned char *) "text/html"))
        {
            return true;
        }

        if (attr->value->length == 21
            && pcutils_str_data_casecmp(attr->value->data,
                                       (const unsigned char *) "application/xhtml+xml"))
        {
            return true;
        }

        return false;
    }

    if (node->ns == PCHTML_NS_SVG
        && (node->local_name == PCHTML_TAG_FOREIGNOBJECT
            || node->local_name == PCHTML_TAG_DESC
            || node->local_name == PCHTML_TAG_TITLE))
    {
        return true;
    }

    return false;
}

unsigned int
pchtml_html_tree_adjust_attributes_mathml(pchtml_html_tree_t *tree,
                                       pcdom_attr_t *attr, void *ctx)
{
    unsigned int status;

    status = pchtml_html_tree_adjust_mathml_attributes(tree, attr, ctx);
    if (status !=PCHTML_STATUS_OK) {
        return status;
    }

    return pchtml_html_tree_adjust_foreign_attributes(tree, attr, ctx);
}

unsigned int
pchtml_html_tree_adjust_attributes_svg(pchtml_html_tree_t *tree,
                                    pcdom_attr_t *attr, void *ctx)
{
    unsigned int status;

    status = pchtml_html_tree_adjust_svg_attributes(tree, attr, ctx);
    if (status !=PCHTML_STATUS_OK) {
        return status;
    }

    return pchtml_html_tree_adjust_foreign_attributes(tree, attr, ctx);
}
