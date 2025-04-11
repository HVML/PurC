/*
 * @file hvml-token.h
 * @author XueShuming
 * @date 2021/08/29
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

#ifndef PURC_HVML_TOKEN_H
#define PURC_HVML_TOKEN_H

#include <stddef.h>
#include <stdint.h>
#include <private/vdom.h>
#include "private/tkz-helper.h"

enum pchvml_token_type {
    PCHVML_TOKEN_DOCTYPE,
    PCHVML_TOKEN_START_TAG,
    PCHVML_TOKEN_END_TAG,
    PCHVML_TOKEN_COMMENT,
    PCHVML_TOKEN_CHARACTER,
    PCHVML_TOKEN_VCM_TREE,
    PCHVML_TOKEN_EOF
};

struct pchvml_token_attr;
struct pchvml_token;

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

struct pchvml_token* pchvml_token_new(enum pchvml_token_type type);

PCA_INLINE
struct pchvml_token* pchvml_token_new_start_tag()
{
    return pchvml_token_new(PCHVML_TOKEN_START_TAG);
}

PCA_INLINE
struct pchvml_token* pchvml_token_new_end_tag()
{
    return pchvml_token_new(PCHVML_TOKEN_END_TAG);
}

PCA_INLINE
struct pchvml_token* pchvml_token_new_eof() {
    return pchvml_token_new(PCHVML_TOKEN_EOF);
}

PCA_INLINE
struct pchvml_token* pchvml_token_new_comment() {
    return pchvml_token_new(PCHVML_TOKEN_COMMENT);
}

PCA_INLINE
struct pchvml_token* pchvml_token_new_doctype() {
    return pchvml_token_new(PCHVML_TOKEN_DOCTYPE);
}

struct pchvml_token* pchvml_token_new_vcm(struct pcvcm_node* vcm);

void pchvml_token_done(struct pchvml_token* token);

void pchvml_token_destroy(struct pchvml_token* token);

struct tkz_uc* pchvml_token_get_first_uc(struct pchvml_token* token);

void pchvml_token_set_first_uc(struct pchvml_token* token, struct tkz_uc *uc);

void pchvml_token_append_to_name(struct pchvml_token* token, uint32_t uc);

void pchvml_token_append_buffer_to_name(struct pchvml_token* token,
        struct tkz_buffer* buffer);

const char* pchvml_token_get_name(struct pchvml_token* token);

void pchvml_token_append_bytes_to_text(struct pchvml_token* token,
        const char* bytes, size_t sz_bytes);

const char* pchvml_token_get_text(struct pchvml_token* token);

void pchvml_token_append_to_public_identifier(struct pchvml_token* token,
        uint32_t uc);
const char* pchvml_token_get_public_identifier(struct pchvml_token* token);

void pchvml_token_reset_public_identifier(struct pchvml_token* token);

void pchvml_token_append_to_system_information(struct pchvml_token* token,
        uint32_t uc);
const char* pchvml_token_get_system_information(struct pchvml_token* token);

void pchvml_token_reset_system_information(struct pchvml_token* token);

bool pchvml_token_is_type(struct pchvml_token* token,
        enum pchvml_token_type type);

enum pchvml_token_type pchvml_token_get_type(struct pchvml_token* token);

const char* pchvml_token_type_name(enum pchvml_token_type type);

const char* pchvml_token_get_type_name(struct pchvml_token* token);

struct pcvcm_node*
pchvml_token_get_vcm_content(struct pchvml_token* token);

struct pcvcm_node*
pchvml_token_detach_vcm_content(struct pchvml_token* token);

void pchvml_token_set_self_closing(struct pchvml_token* token, bool b);

bool pchvml_token_is_self_closing(struct pchvml_token* token);

void pchvml_token_set_force_quirks(struct pchvml_token* token, bool b);

bool pchvml_token_is_force_quirks(struct pchvml_token* token);

void pchvml_token_set_is_whitespace(struct pchvml_token* token, bool b);

bool pchvml_token_is_whitespace(struct pchvml_token* token);

void pchvml_token_begin_attr(struct pchvml_token* token);

void pchvml_token_append_to_attr_name(struct pchvml_token* token,
        uint32_t uc);

void pchvml_token_append_bytes_to_attr_name(struct pchvml_token* token,
        const char* bytes, size_t sz_bytes);

void pchvml_token_append_to_attr_value(struct pchvml_token* token,
        uint32_t);

void pchvml_token_append_bytes_to_attr_value(struct pchvml_token* token,
        const char* bytes, size_t sz_bytes);

void pchvml_token_append_vcm_to_attr(struct pchvml_token* token,
        struct pcvcm_node* vcm);

void pchvml_token_set_assignment_to_attr(struct pchvml_token* token,
        enum pchvml_attr_operator assignment);

void pchvml_token_set_quote(struct pchvml_token* token, uint32_t quote);

bool pchvml_token_is_in_attr(struct pchvml_token* token);

struct pchvml_token_attr* pchvml_token_get_curr_attr(
        struct pchvml_token* token);

bool pchvml_token_is_curr_attr_duplicate(
        struct pchvml_token* token);

void pchvml_token_end_attr(struct pchvml_token* token);

size_t pchvml_token_get_attr_size(struct pchvml_token* token);

bool pchvml_token_has_raw_attr(struct pchvml_token* token);

struct pchvml_token_attr* pchvml_token_get_attr(
        struct pchvml_token* token, size_t i);

const char* pchvml_token_attr_get_name(struct pchvml_token_attr* attr);
struct pcvcm_node* pchvml_token_attr_get_value_ex(
        struct pchvml_token_attr* attr, bool res_vcm);

PCA_INLINE
const struct pcvcm_node* pchvml_token_attr_get_value(
        struct pchvml_token_attr* attr)
{
    return pchvml_token_attr_get_value_ex(attr, false);
}
enum pchvml_attr_operator pchvml_token_attr_get_operator(
        struct pchvml_token_attr* attr);

struct tkz_buffer* pchvml_token_attr_to_string(
        struct pchvml_token_attr* attr);
struct tkz_buffer* pchvml_token_to_string(struct pchvml_token* token);

void
pchvml_util_dump_token(const struct pchvml_token *token,
        const char *file, int line, const char *func);

#define PRINT_TOKEN(_token)           \
    pchvml_util_dump_token(_token, __FILE__, __LINE__, __func__)

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* not defined PURC_HVML_TOKEN_H */

