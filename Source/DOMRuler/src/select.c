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

#include "select.h"
#include "utils.h"
#include "hl_dom_element_node.h"
#include <stdio.h>
#include <string.h>

#define DEFAULT_CHARSET "UTF-8"
#define DEFAULT_URL "domruler_css_select"
#define UNUSED(x) ((x) = (x))

static const char *default_ua_css = "div { display: block; }"
    "hiweb { display: block; }"
    "hijs { display: block; }"
    "minigui { display: block; }";

static css_error resolve_url(void *pw,
        const char *base, lwc_string *rel, lwc_string **abs)
{
    UNUSED(pw);
    UNUSED(base);

    /* About as useless as possible */
    *abs = lwc_string_ref(rel);

    return CSS_OK;
}

static css_stylesheet *hlhl_css_stylesheet_create(const char *charset,
        const char *url, bool allow_quirks, bool inline_style)
{
    css_stylesheet_params params;
    css_stylesheet *sheet;
    css_error error;

    params.params_version = CSS_STYLESHEET_PARAMS_VERSION_1;
    params.level = CSS_LEVEL_DEFAULT;
    params.charset = charset;
    params.url = url;
    params.title = NULL;
    params.allow_quirks = allow_quirks;
    params.inline_style = inline_style;
    params.resolve = resolve_url;
    params.resolve_pw = NULL;
    params.import = NULL;
    params.import_pw = NULL;
    params.color = NULL;
    params.color_pw = NULL;
    params.font = NULL;
    params.font_pw = NULL;

    error = css_stylesheet_create(&params, &sheet);
    if (error != CSS_OK) {
        HL_LOGE("Failed create sheet: %d\n", error);
        return NULL;
    }

    return sheet;
}

css_stylesheet *hl_css_stylesheet_create_ua_css()
{
    css_stylesheet *sheet = hlhl_css_stylesheet_create(DEFAULT_CHARSET,
            DEFAULT_URL, true, false);
    if (sheet == NULL) {
        return NULL;
    }

    css_error error = hl_css_stylesheet_append_data(sheet,
            (const unsigned char *)default_ua_css, strlen(default_ua_css));
    if (error != CSS_OK) {
        HL_LOGE("failed append default_css: %d\n", error);
        css_stylesheet_destroy(sheet);
        return NULL;
    }

    error = css_stylesheet_data_done(sheet);
    if (error != CSS_OK) {
        HL_LOGE("failed completing parse: %d\n", error);
        css_stylesheet_destroy(sheet);
        return NULL;
    }

    return sheet;
}

int hl_css_stylesheet_append_data(css_stylesheet *sheet,
        const uint8_t *data, size_t len)
{
    if (sheet == NULL || data == NULL || len <= 0) {
        return DOMRULER_BADPARM;
    }

    css_error error = css_stylesheet_append_data(sheet, data, len);
    if (error != CSS_OK && error != CSS_NEEDDATA) {
        HL_LOGE("append css data failed|code=%d\n", error);
        return error;
    }
    return DOMRULER_OK;
}

int hl_css_stylesheet_data_done(css_stylesheet* sheet)
{
    if (sheet)
        return css_stylesheet_data_done(sheet);
    return DOMRULER_OK;
}

int hl_css_stylesheet_destroy(css_stylesheet* sheet)
{
    if (sheet)
        return css_stylesheet_destroy(sheet);
    return DOMRULER_OK;
}

css_stylesheet *hl_css_stylesheet_inline_style_create(
        const uint8_t *data, size_t len)
{
    css_stylesheet *sheet = hlhl_css_stylesheet_create(DEFAULT_CHARSET,
            DEFAULT_URL, true, true);
    if (sheet == NULL) {
        return NULL;
    }

    css_error error = hl_css_stylesheet_append_data(sheet, data, len);
    if (error != CSS_OK) {
        HL_LOGE("failed add inline style: %d\n", error);
        css_stylesheet_destroy(sheet);
        return NULL;
    }

    error = css_stylesheet_data_done(sheet);
    if (error != CSS_OK) {
        HL_LOGE("failed completing parse: %d\n", error);
        css_stylesheet_destroy(sheet);
        return NULL;
    }

    return sheet;
}


HLCSS *domruler_css_create()
{
    css_stylesheet *sheet = hlhl_css_stylesheet_create(DEFAULT_CHARSET,
            DEFAULT_URL, true, false);
    if (sheet == NULL)
    {
        return NULL;
    }

    HLCSS* css = (HLCSS*)malloc(sizeof(HLCSS));
    css->ua_sheet = hl_css_stylesheet_create_ua_css();
    css->sheet = sheet;
    css->done = 0;

    return css;
}

int domruler_css_append_data(HLCSS *css, const char *data, size_t len)
{
    if (css)
        return hl_css_stylesheet_append_data(css->sheet,
                (const uint8_t *)data, len);
    return DOMRULER_BADPARM;
}

int domruler_css_destroy(HLCSS *css)
{
    if (css == NULL)
        return DOMRULER_OK;

    hl_css_stylesheet_destroy(css->ua_sheet);
    hl_css_stylesheet_destroy(css->sheet);
    free(css);

    return DOMRULER_OK;
}

css_select_ctx *hl_css_select_ctx_create(HLCSS *css)
{
    uint32_t count;
    if (css == NULL || css->sheet == NULL) {
        HL_LOGW("css create select ctx|css=%p|css->sheet=%p|param error.\n",
                css, css->sheet);
        return NULL;
    }

    if (css->done != 1) {
        hl_css_stylesheet_data_done(css->sheet);
        css->done = 1;
    }

    css_select_ctx* select_ctx = NULL;;
    css_error code = css_select_ctx_create(&select_ctx);
    if (code != CSS_OK) {
        HL_LOGW("css create select ctx failed|code=%d\n", code);
        return NULL;
    }

    code = css_select_ctx_append_sheet(select_ctx, css->ua_sheet,
            CSS_ORIGIN_UA, NULL);
    if (code != CSS_OK) {
        HL_LOGW("append ua sheet to select ctx failed|code=%d\n", code);
        return NULL;
    }

    code = css_select_ctx_append_sheet(select_ctx, css->sheet,
            CSS_ORIGIN_AUTHOR, NULL);
    if (code != CSS_OK) {
        HL_LOGW("append sheet to select ctx failed|code=%d\n", code);
        return NULL;
    }

    code = css_select_ctx_count_sheets(select_ctx, &count);
    if (code != CSS_OK) {
        HL_LOGW("count select ctx sheets failed!|code=%d\n", code);
        hl_css_select_ctx_destroy(select_ctx);
        return NULL;
    }

    HL_LOGD("create select ctx|sheet count=%d\n", count);
    return select_ctx;
}

int hl_css_select_ctx_destroy(css_select_ctx* ctx)
{
    if (ctx)
        return css_select_ctx_destroy(ctx);
    return DOMRULER_OK;
}

void hl_computed_node_display(HiLayoutNode *node)
{
    bool root = hi_layout_node_is_root(node);
    uint8_t value = css_computed_display(node->computed_style, root);

    switch (value) {
    case CSS_DISPLAY_BLOCK:
        node->layout_type = LAYOUT_BLOCK;
        node->box_values.display = HL_DISPLAY_BLOCK;
        break;

    case CSS_DISPLAY_INLINE_BLOCK:
        node->layout_type = LAYOUT_INLINE_BLOCK;
        node->box_values.display = HL_DISPLAY_INLINE_BLOCK;
        break;

    case CSS_DISPLAY_GRID:
        node->layout_type = LAYOUT_GRID;
        node->box_values.display = HL_DISPLAY_GRID;
        break;

    case CSS_DISPLAY_INLINE_GRID:
        node->layout_type = LAYOUT_INLINE_GRID;
        node->box_values.display = HL_DISPLAY_INLINE_GRID;
        break;

    case CSS_DISPLAY_NONE:
        node->layout_type = LAYOUT_NONE;
        node->box_values.display = HL_DISPLAY_NONE;
        break;

    default:
        node->layout_type = LAYOUT_BLOCK;
        node->box_values.display = HL_DISPLAY_BLOCK;
        break;
    }
}

int hl_select_node_style(const css_media *media, css_select_ctx *select_ctx,
        HiLayoutNode *node)
{
    // filter non element node
    HLNodeType type = hi_layout_node_get_type(node);
    if (type != DOM_ELEMENT_NODE) {
        return DOMRULER_OK;
    }

    css_select_results* result = hl_get_node_style(media, select_ctx, node);
    if (result) {
        if (node->select_styles) {
            css_select_results_destroy(node->select_styles);
        }
        node->select_styles = result;
        node->computed_style = result->styles[CSS_PSEUDO_ELEMENT_NONE];
        hl_computed_node_display(node);
        return DOMRULER_OK;
    }
    return DOMRULER_SELECT_STYLE_ERR;
}

