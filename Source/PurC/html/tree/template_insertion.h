/**
 * @file template_insertion.h 
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for html template insertion.
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
 * License Version 2.0:
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */


#ifndef PCHTML_HTML_TEMPLATE_INSERTION_H
#define PCHTML_HTML_TEMPLATE_INSERTION_H

#include "purc.h"
#include "config.h"
#include "private/instance.h"
#include "private/errors.h"
#include "private/array.h"

#include "html/tree.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    pchtml_html_tree_insertion_mode_f mode;
}
pchtml_html_tree_template_insertion_t;


/*
 * Inline functions
 */
static inline pchtml_html_tree_insertion_mode_f
pchtml_html_tree_template_insertion_current(pchtml_html_tree_t *tree)
{
    if (pcutils_array_obj_length(tree->template_insertion_modes) == 0) {
        return NULL;
    }

    pchtml_html_tree_template_insertion_t *tmp_ins;

    tmp_ins = (pchtml_html_tree_template_insertion_t *)
              pcutils_array_obj_last(tree->template_insertion_modes);

    return tmp_ins->mode;
}

static inline pchtml_html_tree_insertion_mode_f
pchtml_html_tree_template_insertion_get(pchtml_html_tree_t *tree, size_t idx)
{
    pchtml_html_tree_template_insertion_t *tmp_ins;

    tmp_ins = (pchtml_html_tree_template_insertion_t *)
              pcutils_array_obj_get(tree->template_insertion_modes, idx);
    if (tmp_ins == NULL) {
        return NULL;
    }

    return tmp_ins->mode;
}

static inline pchtml_html_tree_insertion_mode_f
pchtml_html_tree_template_insertion_first(pchtml_html_tree_t *tree)
{
    return pchtml_html_tree_template_insertion_get(tree, 0);
}

static inline unsigned int
pchtml_html_tree_template_insertion_push(pchtml_html_tree_t *tree,
                                      pchtml_html_tree_insertion_mode_f mode)
{
    pchtml_html_tree_template_insertion_t *tmp_ins;

    tmp_ins = (pchtml_html_tree_template_insertion_t *)
              pcutils_array_obj_push(tree->template_insertion_modes);
    if (tmp_ins == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }

    tmp_ins->mode = mode;

    return PCHTML_STATUS_OK;
}

static inline pchtml_html_tree_insertion_mode_f
pchtml_html_tree_template_insertion_pop(pchtml_html_tree_t *tree)
{
    pchtml_html_tree_template_insertion_t *tmp_ins;

    tmp_ins = (pchtml_html_tree_template_insertion_t *)
              pcutils_array_obj_pop(tree->template_insertion_modes);
    if (tmp_ins == NULL) {
        return NULL;
    }

    return tmp_ins->mode;
}


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_HTML_TEMPLATE_INSERTION_H */

