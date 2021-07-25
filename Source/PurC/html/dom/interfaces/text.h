/**
 * @file text.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for html text content.
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


#ifndef PCHTML_DOM_TEXT_H
#define PCHTML_DOM_TEXT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/dom/interfaces/document.h"
#include "html/dom/interfaces/character_data.h"


struct pchtml_dom_text {
    pchtml_dom_character_data_t char_data;
};


pchtml_dom_text_t *
pchtml_dom_text_interface_create(pchtml_dom_document_t *document) WTF_INTERNAL;

pchtml_dom_text_t *
pchtml_dom_text_interface_destroy(pchtml_dom_text_t *text) WTF_INTERNAL;


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_DOM_TEXT_H */
