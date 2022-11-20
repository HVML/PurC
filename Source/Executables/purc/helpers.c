/*
 * @file helpers.c
 * @author Vincent Wei
 * @date 2022/11/13
 * @brief The imlementation of some helpers.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of purc, which is an HVML interpreter with
 * a command line interface (CLI).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "foil.h"

#include <purc/purc-document.h>

int foil_doc_get_element_lang(purc_document_t doc, pcdoc_element_t ele,
        const char **lang, size_t *len)
{
    int ret = pcdoc_element_get_attribute(doc, ele, "lang", lang, len);
    if (ret == 0)
        return 0;

    pcdoc_node node;
    node.type = PCDOC_NODE_ELEMENT;
    node.elem = ele;
    pcdoc_element_t parent = pcdoc_node_get_parent(doc, node);
    if (parent) {
        return foil_doc_get_element_lang(doc, parent, lang, len);
    }

    return -1;
}

