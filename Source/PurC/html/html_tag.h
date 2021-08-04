/**
 * @file html_tag.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for html tag.
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
 * <https://github.com/lexbor/lexbor>, which is licensed under Apahce 2.0:
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

#ifndef PCHTML_HTML_TAG_H
#define PCHTML_HTML_TAG_H

#include "config.h"
#include "html/base.h"

#include "html/tag.h"
#include "html/ns.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef int pchtml_html_tag_category_t;

enum pchtml_html_tag_category {
    PCHTML_HTML_TAG_CATEGORY__UNDEF          = 0x0000,
    PCHTML_HTML_TAG_CATEGORY_ORDINARY        = 0x0001,
    PCHTML_HTML_TAG_CATEGORY_SPECIAL         = 0x0002,
    PCHTML_HTML_TAG_CATEGORY_FORMATTING      = 0x0004,
    PCHTML_HTML_TAG_CATEGORY_SCOPE           = 0x0008,
    PCHTML_HTML_TAG_CATEGORY_SCOPE_LIST_ITEM = 0x0010,
    PCHTML_HTML_TAG_CATEGORY_SCOPE_BUTTON    = 0x0020,
    PCHTML_HTML_TAG_CATEGORY_SCOPE_TABLE     = 0x0040,
    PCHTML_HTML_TAG_CATEGORY_SCOPE_SELECT    = 0x0080,
};

typedef struct {
    const unsigned char *name;
    unsigned int     len;
}
pchtml_html_tag_fixname_t;


#define PCHTML_HTML_TAG_RES_CATS
#define PCHTML_HTML_TAG_RES_FIXNAME_SVG
#include "html_tag_res_ext.h"


/*
 * Inline functions
 */
static inline bool
pchtml_html_tag_is_category(pchtml_tag_id_t tag_id, pchtml_ns_id_t ns,
                         pchtml_html_tag_category_t cat)
{
    if (tag_id < PCHTML_TAG__LAST_ENTRY && ns < PCHTML_NS__LAST_ENTRY) {
        return pchtml_html_tag_res_cats[tag_id][ns] & cat;
    }

    return (PCHTML_HTML_TAG_CATEGORY_ORDINARY|PCHTML_HTML_TAG_CATEGORY_SCOPE_SELECT) & cat;
}

static inline const pchtml_html_tag_fixname_t *
pchtml_html_tag_fixname_svg(pchtml_tag_id_t tag_id)
{
    if (tag_id >= PCHTML_TAG__LAST_ENTRY) {
        return NULL;
    }

    return &pchtml_html_tag_res_fixname_svg[tag_id];
}

static inline bool
pchtml_html_tag_is_void(pchtml_tag_id_t tag_id)
{
    switch (tag_id) {
        case PCHTML_TAG_AREA:
        case PCHTML_TAG_BASE:
        case PCHTML_TAG_BR:
        case PCHTML_TAG_COL:
        case PCHTML_TAG_EMBED:
        case PCHTML_TAG_HR:
        case PCHTML_TAG_IMG:
        case PCHTML_TAG_INPUT:
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


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_HTML_TAG_H */
