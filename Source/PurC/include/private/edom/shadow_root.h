/**
 * @file shadow_root.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for shadow root.
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


#ifndef PCHTML_DOM_SHADOW_ROOT_H
#define PCHTML_DOM_SHADOW_ROOT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "private/edom/document.h"
#include "private/edom/element.h"
#include "private/edom/document_fragment.h"


typedef enum {
    PCHTML_DOM_SHADOW_ROOT_MODE_OPEN   = 0x00,
    PCHTML_DOM_SHADOW_ROOT_MODE_CLOSED = 0x01
}
pchtml_dom_shadow_root_mode_t;

struct pchtml_dom_shadow_root {
    pchtml_dom_document_fragment_t document_fragment;

    pchtml_dom_shadow_root_mode_t  mode;
    pchtml_dom_element_t           *host;
};


pchtml_dom_shadow_root_t *
pchtml_dom_shadow_root_interface_create(
                pchtml_dom_document_t *document) WTF_INTERNAL;

pchtml_dom_shadow_root_t *
pchtml_dom_shadow_root_interface_destroy(
                pchtml_dom_shadow_root_t *shadow_root) WTF_INTERNAL;


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_DOM_SHADOW_ROOT_H */
