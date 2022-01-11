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
 *
 * This implementation of HTML parser is derived from Lexbor
 * <https://github.com/lexbor/lexbor>, which is licensed under the Apache
 * License, Version 2.0:
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */


#ifndef PCDOM_PRIVATE_SHADOW_ROOT_H
#define PCDOM_PRIVATE_SHADOW_ROOT_H

#include "config.h"
#include "private/dom.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PCDOM_SHADOW_ROOT_MODE_OPEN   = 0x00,
    PCDOM_SHADOW_ROOT_MODE_CLOSED = 0x01
}
pcdom_shadow_root_mode_t;

struct pcdom_shadow_root {
    pcdom_document_fragment_t document_fragment;

    pcdom_shadow_root_mode_t  mode;
    pcdom_element_t           *host;
};


pcdom_shadow_root_t *
pcdom_shadow_root_interface_create(
                pcdom_document_t *document) WTF_INTERNAL;

pcdom_shadow_root_t *
pcdom_shadow_root_interface_destroy(
                pcdom_shadow_root_t *shadow_root) WTF_INTERNAL;


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCDOM_PRIVATE_SHADOW_ROOT_H */
