/**
 * @file node.c
 * @author Vincent Wei
 * @date 2022/01/14
 * @brief The complementation of html tag.
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

#include "config.h"
#include "html/tag.h"
#include "private/dom.h"
#include "node.h"

bool
pchtml_html_node_is_void(pcdom_node_t *node)
{
    if (node->ns != PCHTML_NS_HTML) {
        return false;
    }

    switch (node->local_name) {
        case PCHTML_TAG_AREA:
        case PCHTML_TAG_BASE:
        case PCHTML_TAG_BASEFONT:
        case PCHTML_TAG_BGSOUND:
        case PCHTML_TAG_BR:
        case PCHTML_TAG_COL:
        case PCHTML_TAG_EMBED:
        case PCHTML_TAG_FRAME:
        case PCHTML_TAG_HR:
        case PCHTML_TAG_IMG:
        case PCHTML_TAG_INPUT:
        case PCHTML_TAG_KEYGEN:
        case PCHTML_TAG_LINK:
        case PCHTML_TAG_META:
        case PCHTML_TAG_PARAM:
        case PCHTML_TAG_SOURCE:
        case PCHTML_TAG_TRACK:
        case PCHTML_TAG_WBR:
            return true;

        default:
            return false;
    }

    return false;
}

