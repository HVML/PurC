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
#include <private/arraylist.h>
#include "tempbuffer.h"

#define pchvml_token_append_to_name pchvml_token_append
#define pchvml_token_append_to_comment pchvml_token_append
#define pchvml_token_append_to_character pchvml_token_append

enum pchvml_attribute_assignment {
    PCHVML_ATTRIBUTE_ASSIGNMENT,           // =
    PCHVML_ATTRIBUTE_ADDITION_ASSIGNMENT,  // +=
    PCHVML_ATTRIBUTE_SUBTRACTION_ASSIGNMENT, // -=
    PCHVML_ATTRIBUTE_REMAINDER_ASSIGNMENT,  // %=
    PCHVML_ATTRIBUTE_REPLACE_ASSIGNMENT,   // ~=
    PCHVML_ATTRIBUTE_HEAD_ASSIGNMENT,   // ^=
    PCHVML_ATTRIBUTE_TAIL_ASSIGNMENT,   // $=
};

enum pchvml_token_type {
    PCHVML_TOKEN_DOCTYPE,
    PCHVML_TOKEN_START_TAG,
    PCHVML_TOKEN_END_TAG,
    PCHVML_TOKEN_COMMENT,
    PCHVML_TOKEN_CHARACTER,
    PCHVML_TOKEN_VCM_TREE,
    PCHVML_TOKEN_EOF
};

struct pchvml_token_attribute {
    enum pchvml_attribute_assignment assignment;
    struct pchvml_temp_buffer* name;
    struct pchvml_temp_buffer* value;
    struct pcvcm_node* vcm;
};

struct pchvml_token {
    enum pchvml_token_type type;
    char* data;
    struct pcutils_arrlist* attr_list;
    struct pchvml_token_attribute* curr_attr;
    struct pchvml_temp_buffer* temp_buffer;
    bool self_closing;
};


#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

struct pchvml_token* pchvml_token_new (enum pchvml_token_type type);
void pchvml_token_destroy (struct pchvml_token* token);

void pchvml_token_attribute_begin (struct pchvml_token* token);
void pchvml_token_attribute_append_to_name (struct pchvml_token* token,
        const char* bytes, size_t sz_bytes);
void pchvml_token_attribute_append_to_value (struct pchvml_token* token,
        const char* bytes, size_t sz_bytes);
void pchvml_token_attribute_set_assignment (struct pchvml_token* token,
        enum pchvml_attribute_assignment assignment);
void pchvml_token_attribute_end (struct pchvml_token* token);

struct pchvml_token* pchvml_token_new_character ();
struct pchvml_token* pchvml_token_new_start_tag ();
struct pchvml_token* pchvml_token_new_end_tag ();

void pchvml_token_append (struct pchvml_token* token,
        const char* bytes, size_t sz_bytes);

void pchvml_token_done (struct pchvml_token* token);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* not defined PURC_HVML_TOKEN_H */

