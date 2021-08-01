/**
 * @file character_data.h 
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for character data.
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


#ifndef PCEDOM_CHARACTER_DATA_H
#define PCEDOM_CHARACTER_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/core/str.h"

#include "private/edom/document.h"
#include "private/edom/node.h"


struct pcedom_character_data {
    pcedom_node_t node;

    pchtml_str_t   data;
};


pcedom_character_data_t *
pcedom_character_data_interface_create(
                pcedom_document_t *document) WTF_INTERNAL;

pcedom_character_data_t *
pcedom_character_data_interface_destroy(
                pcedom_character_data_t *character_data) WTF_INTERNAL;

unsigned int
pcedom_character_data_replace(pcedom_character_data_t *ch_data,
                const unsigned char *data, size_t len,
                size_t offset, size_t count) WTF_INTERNAL;


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCEDOM_CHARACTER_DATA_H */
