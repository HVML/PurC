/**
 * @file tokenizer.c
 * @author
 * @date 2021/07/02
 * @brief The complementation of tokenizer during html parsing.
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

#include "purc.h"
#include "config.h"
#include "private/instance.h"
#include "private/errors.h"
#include "purc-rwstream.h"

#include "html/html/tokenizer.h"
#include "html/html/tokenizer/state.h"
#include "html/html/tokenizer/state_rcdata.h"
#include "html/html/tokenizer/state_rawtext.h"
#include "html/html/tokenizer/state_script.h"
#include "html/html/tree.h"

#define PCHTML_HTML_TAG_RES_DATA
#define PCHTML_HTML_TAG_RES_SHS_DATA
#include "html/html/tag_res.h"


#define PCHTML_HTML_TKZ_TEMP_SIZE (4096 * 4)


enum {
    PCHTML_HTML_TOKENIZER_OPT_UNDEF           = 0x00,
    PCHTML_HTML_TOKENIZER_OPT_TAGS_SELF       = 0x01,
    PCHTML_HTML_TOKENIZER_OPT_ATTRS_SELF      = 0x02,
    PCHTML_HTML_TOKENIZER_OPT_ATTRS_MRAW_SELF = 0x04
};


const unsigned char *pchtml_html_tokenizer_eof = (const unsigned char *) "\x00";


static pchtml_html_token_t *
pchtml_html_tokenizer_token_done(pchtml_html_tokenizer_t *tkz,
                              pchtml_html_token_t *token, void *ctx);


pchtml_html_tokenizer_t *
pchtml_html_tokenizer_create(void)
{
    return pchtml_calloc(1, sizeof(pchtml_html_tokenizer_t));
}

unsigned int
pchtml_html_tokenizer_init(pchtml_html_tokenizer_t *tkz)
{
    unsigned int status;

    if (tkz == NULL) {
        pcinst_set_error (PCHTML_OBJECT_IS_NULL);
        return PCHTML_STATUS_ERROR_OBJECT_IS_NULL;
    }

    /* mraw for templary strings or structures */
    tkz->mraw = pchtml_mraw_create();
    status = pchtml_mraw_init(tkz->mraw, 1024);
    if (status != PCHTML_STATUS_OK) {
        return status;
    }

    /* Init Token */
    tkz->token = NULL;

    tkz->dobj_token = pchtml_dobject_create();
    status = pchtml_dobject_init(tkz->dobj_token,
                                 4096, sizeof(pchtml_html_token_t));
    if (status != PCHTML_STATUS_OK) {
        return status;
    }

    /* Init Token Attributes */
    tkz->dobj_token_attr = pchtml_dobject_create();
    status = pchtml_dobject_init(tkz->dobj_token_attr, 4096,
                                 sizeof(pchtml_html_token_attr_t));
    if (status != PCHTML_STATUS_OK) {
        return status;
    }

    /* Parse errors */
    tkz->parse_errors = pchtml_array_obj_create();
    status = pchtml_array_obj_init(tkz->parse_errors, 16,
                                   sizeof(pchtml_html_tokenizer_error_t));
    if (status != PCHTML_STATUS_OK) {
        return status;
    }

    /* Temporary memory for tag name and attributes. */
    tkz->start = pchtml_malloc(PCHTML_HTML_TKZ_TEMP_SIZE * sizeof(unsigned char));
    if (tkz->start == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }

    tkz->pos = tkz->start;
    tkz->end = tkz->start + PCHTML_HTML_TKZ_TEMP_SIZE;

    tkz->tree = NULL;
    tkz->tags = NULL;
    tkz->attrs = NULL;
    tkz->attrs_mraw = NULL;

    tkz->state = pchtml_html_tokenizer_state_data_before;
    tkz->state_return = NULL;

    tkz->callback_token_done = pchtml_html_tokenizer_token_done;
    tkz->callback_token_ctx = NULL;

    tkz->is_eof = false;
    tkz->status = PCHTML_STATUS_OK;

    tkz->base = NULL;
    tkz->ref_count = 1;

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_html_tokenizer_inherit(pchtml_html_tokenizer_t *tkz_to,
                           pchtml_html_tokenizer_t *tkz_from)
{
    unsigned int status;

    tkz_to->tags = tkz_from->tags;
    tkz_to->attrs = tkz_from->attrs;
    tkz_to->attrs_mraw = tkz_from->attrs_mraw;
    tkz_to->mraw = tkz_from->mraw;

    /* Token and Attributes */
    tkz_to->token = NULL;

    tkz_to->dobj_token = tkz_from->dobj_token;
    tkz_to->dobj_token_attr = tkz_from->dobj_token_attr;

    /* Parse errors */
    tkz_to->parse_errors = pchtml_array_obj_create();
    status = pchtml_array_obj_init(tkz_to->parse_errors, 16,
                                   sizeof(pchtml_html_tokenizer_error_t));
    if (status != PCHTML_STATUS_OK) {
        return status;
    }

    tkz_to->state = pchtml_html_tokenizer_state_data_before;
    tkz_to->state_return = NULL;

    tkz_to->callback_token_done = pchtml_html_tokenizer_token_done;
    tkz_to->callback_token_ctx = NULL;

    tkz_to->is_eof = false;
    tkz_to->status = PCHTML_STATUS_OK;

    tkz_to->base = tkz_from;
    tkz_to->ref_count = 1;

    tkz_to->start = tkz_from->start;
    tkz_to->end = tkz_from->end;
    tkz_to->pos = tkz_to->start;

    return PCHTML_STATUS_OK;
}

pchtml_html_tokenizer_t *
pchtml_html_tokenizer_ref(pchtml_html_tokenizer_t *tkz)
{
    if (tkz == NULL) {
        return NULL;
    }

    if (tkz->base != NULL) {
        return pchtml_html_tokenizer_ref(tkz->base);
    }

    tkz->ref_count++;

    return tkz;
}

pchtml_html_tokenizer_t *
pchtml_html_tokenizer_unref(pchtml_html_tokenizer_t *tkz)
{
    if (tkz == NULL || tkz->ref_count == 0) {
        return NULL;
    }

    if (tkz->base != NULL) {
        tkz->base = pchtml_html_tokenizer_unref(tkz->base);
    }

    tkz->ref_count--;

    if (tkz->ref_count == 0) {
        pchtml_html_tokenizer_destroy(tkz);
    }

    return NULL;
}

void
pchtml_html_tokenizer_clean(pchtml_html_tokenizer_t *tkz)
{
    tkz->tree = NULL;

    tkz->state = pchtml_html_tokenizer_state_data_before;
    tkz->state_return = NULL;

    tkz->is_eof = false;
    tkz->status = PCHTML_STATUS_OK;

    tkz->pos = tkz->start;

    pchtml_mraw_clean(tkz->mraw);
    pchtml_dobject_clean(tkz->dobj_token);
    pchtml_dobject_clean(tkz->dobj_token_attr);

    pchtml_array_obj_clean(tkz->parse_errors);
}

pchtml_html_tokenizer_t *
pchtml_html_tokenizer_destroy(pchtml_html_tokenizer_t *tkz)
{
    if (tkz == NULL) {
        return NULL;
    }

    if (tkz->base == NULL) {
        if (tkz->opt & PCHTML_HTML_TOKENIZER_OPT_TAGS_SELF) {
            pchtml_html_tokenizer_tags_destroy(tkz);
        }

        if (tkz->opt & PCHTML_HTML_TOKENIZER_OPT_ATTRS_SELF) {
            pchtml_html_tokenizer_attrs_destroy(tkz);
        }

        pchtml_mraw_destroy(tkz->mraw, true);
        pchtml_dobject_destroy(tkz->dobj_token, true);
        pchtml_dobject_destroy(tkz->dobj_token_attr, true);
        pchtml_free(tkz->start);
    }

    tkz->parse_errors = pchtml_array_obj_destroy(tkz->parse_errors, true);

    return pchtml_free(tkz);
}

unsigned int
pchtml_html_tokenizer_tags_make(pchtml_html_tokenizer_t *tkz, size_t table_size)
{
    tkz->tags = pchtml_hash_create();
    return pchtml_hash_init(tkz->tags, table_size, sizeof(pchtml_tag_data_t));
}

void
pchtml_html_tokenizer_tags_destroy(pchtml_html_tokenizer_t *tkz)
{
    tkz->tags = pchtml_hash_destroy(tkz->tags, true);
}

unsigned int
pchtml_html_tokenizer_attrs_make(pchtml_html_tokenizer_t *tkz, size_t table_size)
{
    tkz->attrs = pchtml_hash_create();
    return pchtml_hash_init(tkz->attrs, table_size,
                            sizeof(pcedom_attr_data_t));
}

void
pchtml_html_tokenizer_attrs_destroy(pchtml_html_tokenizer_t *tkz)
{
    tkz->attrs = pchtml_hash_destroy(tkz->attrs, true);
}

unsigned int
pchtml_html_tokenizer_begin(pchtml_html_tokenizer_t *tkz)
{
    if (tkz->tags == NULL) {
        tkz->status = pchtml_html_tokenizer_tags_make(tkz, 256);
        if (tkz->status != PCHTML_STATUS_OK) {
            return tkz->status;
        }

        tkz->opt |= PCHTML_HTML_TOKENIZER_OPT_TAGS_SELF;
    }

    if (tkz->attrs == NULL) {
        tkz->status = pchtml_html_tokenizer_attrs_make(tkz, 256);
        if (tkz->status != PCHTML_STATUS_OK) {
            return tkz->status;
        }

        tkz->opt |= PCHTML_HTML_TOKENIZER_OPT_ATTRS_SELF;
    }

    if (tkz->attrs_mraw == NULL) {
        tkz->attrs_mraw = tkz->mraw;

        tkz->opt |= PCHTML_HTML_TOKENIZER_OPT_ATTRS_MRAW_SELF;
    }

    tkz->token = pchtml_html_token_create(tkz->dobj_token);
    if (tkz->token == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_html_tokenizer_chunk(pchtml_html_tokenizer_t *tkz, const purc_rwstream_t html,
                         size_t size)
{
    if ((html == NULL) || (size == 0))
        return PCHTML_ERROR;

    size_t rwsize = 0;
    const unsigned char * data = (unsigned char *)purc_rwstream_get_mem_buffer (html, &rwsize);

    if ((rwsize != size) || (data == NULL))
        return PCHTML_ERROR;

    const unsigned char *end = data + size;

    tkz->is_eof = false;
    tkz->status = PCHTML_STATUS_OK;
    tkz->last = end;

    while (data < end) {
        data = tkz->state(tkz, data, end);
    }

    return tkz->status;
}

unsigned int
pchtml_html_tokenizer_end(pchtml_html_tokenizer_t *tkz)
{
    const unsigned char *data, *end;

    tkz->status = PCHTML_STATUS_OK;

    /* Send a fake EOF data. */
    data = pchtml_html_tokenizer_eof;
    end = pchtml_html_tokenizer_eof + 1UL;

    tkz->is_eof = true;

    while (tkz->state(tkz, data, end) < end) {
        /* empty loop */
    }

    tkz->is_eof = false;

    if (tkz->status != PCHTML_STATUS_OK) {
        return tkz->status;
    }

    /* Emit fake token: END OF FILE */
    pchtml_html_token_clean(tkz->token);

    tkz->token->tag_id = PCHTML_TAG__END_OF_FILE;

    tkz->token = tkz->callback_token_done(tkz, tkz->token,
                                          tkz->callback_token_ctx);

    if (tkz->token == NULL && tkz->status == PCHTML_STATUS_OK) {
        pcinst_set_error (PCHTML_ERROR);
        tkz->status = PCHTML_STATUS_ERROR;
    }

    return tkz->status;
}

static pchtml_html_token_t *
pchtml_html_tokenizer_token_done(pchtml_html_tokenizer_t *tkz,
                              pchtml_html_token_t *token, void *ctx)
{
    UNUSED_PARAM(tkz);
    UNUSED_PARAM(ctx);

    return token;
}

pchtml_ns_id_t
pchtml_html_tokenizer_current_namespace(pchtml_html_tokenizer_t *tkz)
{
    if (tkz->tree == NULL) {
        return PCHTML_NS__UNDEF;
    }

    pcedom_node_t *node = pchtml_html_tree_adjusted_current_node(tkz->tree);

    if (node == NULL) {
        return PCHTML_NS__UNDEF;
    }

    return node->ns;
}

void
pchtml_html_tokenizer_set_state_by_tag(pchtml_html_tokenizer_t *tkz, bool scripting,
                                    pchtml_tag_id_t tag_id, pchtml_ns_id_t ns)
{
    if (ns != PCHTML_NS_HTML) {
        tkz->state = pchtml_html_tokenizer_state_data_before;

        return;
    }

    switch (tag_id) {
        case PCHTML_TAG_TITLE:
        case PCHTML_TAG_TEXTAREA:
            tkz->tmp_tag_id = tag_id;
            tkz->state = pchtml_html_tokenizer_state_rcdata_before;

            break;

        case PCHTML_TAG_STYLE:
        case PCHTML_TAG_XMP:
        case PCHTML_TAG_IFRAME:
        case PCHTML_TAG_NOEMBED:
        case PCHTML_TAG_NOFRAMES:
            tkz->tmp_tag_id = tag_id;
            tkz->state = pchtml_html_tokenizer_state_rawtext_before;

            break;

        case PCHTML_TAG_SCRIPT:
            tkz->tmp_tag_id = tag_id;
            tkz->state = pchtml_html_tokenizer_state_script_data_before;

            break;

        case PCHTML_TAG_NOSCRIPT:
            if (scripting) {
                tkz->tmp_tag_id = tag_id;
                tkz->state = pchtml_html_tokenizer_state_rawtext_before;

                return;
            }

            tkz->state = pchtml_html_tokenizer_state_data_before;

            break;

        case PCHTML_TAG_PLAINTEXT:
            tkz->state = pchtml_html_tokenizer_state_plaintext_before;

            break;

        default:
            break;
    }
}

/*
 * No inline functions for ABI.
 */
void
pchtml_html_tokenizer_status_set_noi(pchtml_html_tokenizer_t *tkz,
                                  unsigned int status)
{
    pchtml_html_tokenizer_status_set(tkz, status);
}

void
pchtml_html_tokenizer_callback_token_done_set_noi(pchtml_html_tokenizer_t *tkz,
                                               pchtml_html_tokenizer_token_f call_func,
                                               void *ctx)
{
    pchtml_html_tokenizer_callback_token_done_set(tkz, call_func, ctx);
}

void *
pchtml_html_tokenizer_callback_token_done_ctx_noi(pchtml_html_tokenizer_t *tkz)
{
    return pchtml_html_tokenizer_callback_token_done_ctx(tkz);
}

void
pchtml_html_tokenizer_state_set_noi(pchtml_html_tokenizer_t *tkz,
                                 pchtml_html_tokenizer_state_f state)
{
    pchtml_html_tokenizer_state_set(tkz, state);
}

void
pchtml_html_tokenizer_tmp_tag_id_set_noi(pchtml_html_tokenizer_t *tkz,
                                      pchtml_tag_id_t tag_id)
{
    pchtml_html_tokenizer_tmp_tag_id_set(tkz, tag_id);
}

pchtml_html_tree_t *
pchtml_html_tokenizer_tree_noi(pchtml_html_tokenizer_t *tkz)
{
    return pchtml_html_tokenizer_tree(tkz);
}

void
pchtml_html_tokenizer_tree_set_noi(pchtml_html_tokenizer_t *tkz,
                                pchtml_html_tree_t *tree)
{
    pchtml_html_tokenizer_tree_set(tkz, tree);
}

pchtml_mraw_t *
pchtml_html_tokenizer_mraw_noi(pchtml_html_tokenizer_t *tkz)
{
    return pchtml_html_tokenizer_mraw(tkz);
}

pchtml_hash_t *
pchtml_html_tokenizer_tags_noi(pchtml_html_tokenizer_t *tkz)
{
    return pchtml_html_tokenizer_tags(tkz);
}
