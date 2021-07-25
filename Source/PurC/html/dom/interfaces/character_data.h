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
 */


#ifndef PCHTML_DOM_CHARACTER_DATA_H
#define PCHTML_DOM_CHARACTER_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/core/str.h"

#include "html/dom/interfaces/document.h"
#include "html/dom/interfaces/node.h"


struct pchtml_dom_character_data {
    pchtml_dom_node_t node;

    pchtml_str_t   data;
};


pchtml_dom_character_data_t *
pchtml_dom_character_data_interface_create(
                pchtml_dom_document_t *document) WTF_INTERNAL;

pchtml_dom_character_data_t *
pchtml_dom_character_data_interface_destroy(
                pchtml_dom_character_data_t *character_data) WTF_INTERNAL;

unsigned int
pchtml_dom_character_data_replace(pchtml_dom_character_data_t *ch_data,
                const unsigned char *data, size_t len,
                size_t offset, size_t count) WTF_INTERNAL;


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_DOM_CHARACTER_DATA_H */
