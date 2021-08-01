/**
 * @file comment.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for comment.
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
 * This implementation of HTML parser is derived from Lexbor <http://lexbor.com/>.
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */


#ifndef PCEDOM_COMMENT_H
#define PCEDOM_COMMENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "private/edom/document.h"
#include "private/edom/character_data.h"


struct pcedom_comment {
    pcedom_character_data_t char_data;
};


pcedom_comment_t *
pcedom_comment_interface_create(
                pcedom_document_t *document) WTF_INTERNAL;

pcedom_comment_t *
pcedom_comment_interface_destroy(
                pcedom_comment_t *comment) WTF_INTERNAL;


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCEDOM_COMMENT_H */
