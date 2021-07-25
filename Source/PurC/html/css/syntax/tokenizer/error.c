/**
 * @file error.c
 * @author
 * @date 2021/07/02
 * @brief The complementation of error operation for css.
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


#include "html/css/syntax/tokenizer/error.h"


pchtml_css_syntax_tokenizer_error_t *
pchtml_css_syntax_tokenizer_error_add(pchtml_array_obj_t *parse_errors,
                                   const unsigned char *pos,
                                   pchtml_css_syntax_tokenizer_error_id_t id)
{
    if (parse_errors == NULL) {
        return NULL;
    }

    pchtml_css_syntax_tokenizer_error_t *entry;

    entry = pchtml_array_obj_push(parse_errors);
    if (entry == NULL) {
        return NULL;
    }

    entry->id = id;
    entry->pos = pos;

    return entry;
}
