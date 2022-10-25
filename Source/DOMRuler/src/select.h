/*
** This file is part of DOM Ruler. DOM Ruler is a library to
** maintain a DOM tree, lay out and stylize the DOM nodes by
** using CSS (Cascaded Style Sheets).
**
** Copyright (C) 2022 Beijing FMSoft Technologies Co., Ltd.
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General License for more details.
**
** You should have received a copy of the GNU Lesser General License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _HL_SELECT_H_
#define _HL_SELECT_H_

#include <csseng/csseng.h>

#include "domruler.h"
#include "node.h"

struct HLCSS_ {
    css_stylesheet* sheet;
    css_stylesheet* ua_sheet;
    int done;
};

#ifdef __cplusplus
extern "C" {
#endif

// css_stylesheet
css_stylesheet *hl_domruler_css_stylesheet_create(const char *charset,
        const char *url, bool allow_quirks, bool inline_style);
css_stylesheet *hl_css_stylesheet_inline_style_create(const uint8_t *data,
        size_t len);
css_stylesheet *hl_css_stylesheet_create_ua_css();
int hl_css_stylesheet_append_data(css_stylesheet *sheet,
        const uint8_t *data, size_t len);
int hl_css_stylesheet_data_done(css_stylesheet *sheet);
int hl_css_stylesheet_destroy(css_stylesheet *sheet);

// css_select_ctx
css_select_ctx *hl_css_select_ctx_create(HLCSS *css);
int hl_css_select_ctx_destroy(css_select_ctx *ctx);

// css_select_results
css_select_results *hl_get_node_style(const css_media *media,
        css_select_ctx *select_ctx, HLLayoutNode *node);

css_select_results* hl_css_select_style(const HLCSS* css, HLLayoutNode *node,
        const css_media *media, const css_stylesheet *inline_style,
        css_select_handler *op);

int hl_css_select_result_destroy(css_select_results *result);


// select node style
int hl_select_node_style(const css_media *media, css_select_ctx *select_ctx,
        HLLayoutNode *node);


#ifdef __cplusplus
}
#endif

#endif // _HL_SELECT_H_
