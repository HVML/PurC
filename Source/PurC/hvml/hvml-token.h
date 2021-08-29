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

enum hvml_attribute_assignment {
    HVML_ATTRIBUTE_ASSIGNMENT,           // =
    HVML_ATTRIBUTE_ADDITION_ASSIGNMENT,  // +=
    HVML_ATTRIBUTE_SUBTRACTION_ASSIGNMENT, // -=
    HVML_ATTRIBUTE_REMAINDER_ASSIGNMENT,  // %=
    HVML_ATTRIBUTE_REPLACE_ASSIGNMENT,   // ~=
    HVML_ATTRIBUTE_HEAD_ASSIGNMENT,   // ^=
    HVML_ATTRIBUTE_TAIL_ASSIGNMENT,   // $=
};

enum hvml_token_type {
    HVML_TOKEN_DOCTYPE,
    HVML_TOKEN_START_TAG,
    HVML_TOKEN_END_TAG,
    HVML_TOKEN_COMMENT,
    HVML_TOKEN_CHARACTER,
    HVML_TOKEN_VCM_TREE,
    HVML_TOKEN_EOF
};

struct pchvml_token_attribute {
    char* name;
    struct pcvcm_node* value;
    enum hvml_attribute_assignment assignment;
};

struct pchvml_token {
    enum hvml_token_type type;
    struct pcutils_arrlist* attr_list;
    struct pchvml_token_attribute* curr_attr;
};


#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

struct pchvml_token* pchvml_token_new (enum hvml_token_type type);
void pchvml_token_destroy (struct pchvml_token* token);

void pchvml_token_begin_attribute (struct pchvml_token* token);
void pchvml_token_append_to_attribute_name (struct pchvml_token* token,
        char* bytes);
void pchvml_token_append_to_attribute_value (struct pchvml_token* token,
        struct pcvcm_node* node);
void pchvml_token_end_attribute (struct pchvml_token* token);

struct pchvml_token* pchvml_token_new_character (const char* buf);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* not defined PURC_HVML_TOKEN_H */

