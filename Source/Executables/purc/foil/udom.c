/*
** @file udom.c
** @author Vincent Wei
** @date 2022/10/06
** @brief The implementation of uDOM (the rendering tree).
**
** Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of purc, which is an HVML interpreter with
** a command line interface (CLI).
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

// #undef NDEBUG

#include "udom.h"
#include "page.h"
#include "widget.h"
#include "rdrbox.h"
#include "rdrbox-internal.h"
#include "util/sorted-array.h"
#include "util/list.h"
#include "unicode/unicode.h"

#include <assert.h>

static css_stylesheet *def_ua_sheet;

/* copy from https://www.w3.org/TR/2011/REC-CSS2-20110607/sample.html#q22.0 */
static const char *def_style_sheet = ""
    "html, address,"
    "blockquote,"
    "body, dd, div,"
    "dl, dt, fieldset, form,"
    "frame, frameset,"
    "h1, h2, h3, h4,"
    "h5, h6, noframes,"
    "ol, p, ul, center,"
    "dir, hr, menu, pre,"
    "header, nav, article, footer," // HTML 5 tags
    "section, address, aside { display: block; unicode-bidi: embed }"
    "abbr            { display: inline }"
    "li              { display: list-item }"
    "datalist, template, slot, dialog,"  // HTML 5 tags
    "head, area      { display: none }"
    "table           { display: table }"
    "tr              { display: table-row }"
    "thead           { display: table-header-group }"
    "tbody           { display: table-row-group }"
    "tfoot           { display: table-footer-group }"
    "col             { display: table-column }"
    "colgroup        { display: table-column-group }"
    "td, th          { display: table-cell }"
    "caption         { display: table-caption }"
    "th              { font-weight: bolder; text-align: center }"
    "caption         { text-align: center }"
    "address         { font-style: italic }"
    "body            { margin: 1em 1ex }"
    "h1              { margin: 2em 0 1em 0 }"
    "h2              { margin: 2em 0 1em 0 }"
    "h3              { margin: 1em 0 1em 0 }"
    "h4, p,"
    "blockquote, ul,"
    "fieldset, form,"
    "ol, dl, dir,"
    "menu            { margin: 1em 0 }"
    "h5              { margin: 1em 0 }"
    "h6              { margin: 1em 0 }"
    "h1, h2, h3, h4,"
    "h5, h6, b,"
    "strong          { font-weight: bold }"
    "blockquote      { margin-left: 4ex; margin-right: 4ex }"
    "i, cite, em,"
    "var, address    { font-style: italic }"
    "pre, tt, code,"
    "kbd, samp       { font-family: monospace }"
    "pre             { white-space: pre }"
    "button, textarea,"
    "input, select   { appearance: auto; display: inline-block }"
    "progress        { appearance: auto; display: inline-block; height: 1em; width: 10em; }"
    "meter           { appearance: auto; display: inline-block; height: 1em; width: 5em; }"
    "big             { font-size: 1em }"
    "small, sub, sup { font-size: 1em }"
    "sub             { vertical-align: sub }"
    "sup             { vertical-align: super }"
    "table           { border-spacing: 2px; }"
    "thead, tbody,"
    "tfoot           { vertical-align: middle }"
    "td, th, tr      { vertical-align: inherit }"
    "s, strike, del  { text-decoration: line-through }"
    "hr              { border: 1px inset }"
    "ol, ul, dir,"
    "menu, dd        { margin-left: 4em }"
    "ol              { list-style-type: decimal }"
    "ol ul, ul ol,"
    "ul ul, ol ol    { margin-top: 0; margin-bottom: 0 }"
    "u, ins          { text-decoration: underline }"
    "br:before       { content: \"\\A\"; white-space: pre-line }"
    "center          { text-align: center }"
    ":link, :visited { text-decoration: underline }"
    ":focus          { outline: thin dotted invert }"
    ""
    /* Insert quotes before and after Q element content */
    "q:before        { content: open-quote }"   // HTML 5 tag
    "q:after         { content: close-quote }"  // HTML 5 tag
    ""
    /* Begin bidirectionality settings (do not change) */
    "BDO[DIR=\"ltr\"]  { direction: ltr; unicode-bidi: bidi-override }"
    "BDO[DIR=\"rtl\"]  { direction: rtl; unicode-bidi: bidi-override }"
    ""
    "*[DIR=\"ltr\"]    { direction: ltr; unicode-bidi: embed }"
    "*[DIR=\"rtl\"]    { direction: rtl; unicode-bidi: embed }"
;

static css_error resolve_url(void *pw,
        const char *base, lwc_string *rel, lwc_string **abs)
{
    (void)pw;
    (void)base;

    /* About as useless as possible */
    *abs = lwc_string_ref(rel);

    return CSS_OK;
}

int foil_udom_module_init(pcmcth_renderer *rdr)
{
    if (foil_rdrbox_module_init(rdr))
        return -1;

    css_stylesheet_params params;
    css_error err;

    memset(&params, 0, sizeof(params));
    params.params_version = CSS_STYLESHEET_PARAMS_VERSION_1;
    params.level = CSS_LEVEL_DEFAULT;
    params.charset = FOIL_DEF_CHARSET;
    params.url = "foo";
    params.title = "foo";
    params.resolve = resolve_url;
#if 0
    params.allow_quirks = false;
    params.inline_style = false;
    params.resolve_pw = NULL;
    params.import = NULL;
    params.import_pw = NULL;
    params.color = NULL;
    params.color_pw = NULL;
    params.font = NULL;
    params.font_pw = NULL;
#endif

    err = css_stylesheet_create(&params, &def_ua_sheet);
    if (err != CSS_OK) {
        LOG_ERROR("Failed to create default user agent sheet: %d\n", err);
        return -1;
    }

    err = css_stylesheet_append_data(def_ua_sheet,
            (const unsigned char *)def_style_sheet, strlen(def_style_sheet));
    if (err != CSS_OK && err != CSS_NEEDDATA ) {
        LOG_ERROR("Failed to append data to UA style sheet: %d\n", err);
        css_stylesheet_destroy(def_ua_sheet);
        return -1;
    }

    css_stylesheet_data_done(def_ua_sheet);

    return 0;
}

void foil_udom_module_cleanup(pcmcth_renderer *rdr)
{
    if (def_ua_sheet)
        css_stylesheet_destroy(def_ua_sheet);

    foil_rdrbox_module_cleanup(rdr);
}

extern css_select_handler foil_css_select_handler;

static void udom_cleanup(pcmcth_udom *udom)
{
    if (udom->elem2nodedata) {
        size_t n = sorted_array_count(udom->elem2nodedata);

        for (size_t i = 0; i < n; i++) {
            void *node_data;
            uint64_t node = sorted_array_get(udom->elem2nodedata,
                    i, &node_data);
            css_node_data_handler(&foil_css_select_handler, CSS_NODE_DELETED,
                udom, INT2PTR(node), NULL, node_data);
        }

        sorted_array_destroy(udom->elem2nodedata);
    }

    if (udom->elem2rdrbox)
        sorted_array_destroy(udom->elem2rdrbox);
    if (udom->title_ucs)
        free(udom->title_ucs);
    if (udom->base)
        pcutils_broken_down_url_delete(udom->base);
    if (udom->author_sheet)
        css_stylesheet_destroy(udom->author_sheet);
    if (udom->select_ctx)
        css_select_ctx_destroy(udom->select_ctx);
    if (udom->root_stk_ctxt) {
        foil_stacking_context_delete(udom->root_stk_ctxt);
    }
    if (udom->initial_cblock)
        foil_rdrbox_delete_deep(udom->initial_cblock);
}

pcmcth_udom *foil_udom_new(pcmcth_page *page)
{
    pcmcth_udom* udom = calloc(1, sizeof(pcmcth_udom));
    if (udom == NULL) {
        goto failed;
    }

    udom->page = page;

    udom->elem2nodedata = sorted_array_create(SAFLAG_DEFAULT, 8, NULL, NULL);
    if (udom->elem2nodedata == NULL) {
        goto failed;
    }

    udom->elem2rdrbox = sorted_array_create(SAFLAG_DEFAULT, 8, NULL, NULL);
    if (udom->elem2rdrbox == NULL) {
        goto failed;
    }

    udom->base = pcutils_broken_down_url_new();
    if (udom->base == NULL) {
        goto failed;
    }

    css_error err = css_select_ctx_create(&udom->select_ctx);
    if (err != CSS_OK) {
        goto failed;
    }

    err = css_select_ctx_append_sheet(udom->select_ctx, def_ua_sheet,
            CSS_ORIGIN_UA, NULL);
    if (err != CSS_OK) {
        goto failed;
    }

    foil_widget *widget = foil_widget_from_page(page);
    foil_widget_reset_viewport(widget);
    int cols = foil_widget_client_width(widget);
    int rows = foil_widget_client_height(widget);
    int width = cols * FOIL_PX_GRID_CELL_W;
    int height = rows * FOIL_PX_GRID_CELL_H;

    udom->vw = width;
    udom->vh = height;

    /* create the initial containing block */
    udom->initial_cblock = foil_rdrbox_new(FOIL_RDRBOX_TYPE_BLOCK);
    if (udom->initial_cblock == NULL) {
        LOG_ERROR("Failed to allocate initial containing block\n");
        goto failed;
    }

    /* set some fileds having non-zero values of
       the initial containing block */
    udom->initial_cblock->udom = udom;

    udom->initial_cblock->is_initial = 1;
    udom->initial_cblock->is_block_level = 1;
    udom->initial_cblock->is_block_container = 1;
    udom->initial_cblock->is_width_resolved = 1;

    udom->initial_cblock->width = width;
    udom->initial_cblock->height = height;
    LOG_INFO("width of initial containing block: %d\n", width);

    udom->initial_cblock->color.specified = 0;
    udom->initial_cblock->color.argb = FOIL_DEF_FGC;
    udom->initial_cblock->background_color.specified = 0;
    udom->initial_cblock->background_color.argb = FOIL_DEF_BGC;

    udom->initial_cblock->ctnt_rect.left = 0;
    udom->initial_cblock->ctnt_rect.top = 0;
    udom->initial_cblock->ctnt_rect.right = width;
    udom->initial_cblock->ctnt_rect.bottom = height;
    udom->initial_cblock->cblock_creator = NULL;

    udom->media.type = CSS_MEDIA_TTY;
    udom->media.width  = INTTOFIX(width);
    udom->media.height = INTTOFIX(height);
    // left fields of css_media are not used
    udom->media.aspect_ratio.width = INTTOFIX(cols);
    udom->media.aspect_ratio.height = INTTOFIX(rows);
    udom->media.orientation = (cols > rows) ?
        CSS_MEDIA_ORIENTATION_LANDSCAPE : CSS_MEDIA_ORIENTATION_PORTRAIT;
    udom->media.resolution.value = INTTOFIX(96);
    udom->media.resolution.unit = CSS_UNIT_DPI;
    udom->media.scan = CSS_MEDIA_SCAN_PROGRESSIVE;
    udom->media.grid = INTTOFIX(1);
    udom->media.update = CSS_MEDIA_UPDATE_FREQUENCY_NORMAL;
    udom->media.overflow_block = CSS_MEDIA_OVERFLOW_BLOCK_NONE;
    udom->media.overflow_inline = CSS_MEDIA_OVERFLOW_INLINE_NONE;

    udom->media.color = INTTOFIX(8);
    udom->media.color_index = INTTOFIX(256);
    udom->media.monochrome = INTTOFIX(0);
    udom->media.inverted_colors = INTTOFIX(0);

    udom->media.pointer = CSS_MEDIA_POINTER_NONE;
    udom->media.any_pointer = CSS_MEDIA_POINTER_NONE;
    udom->media.hover = CSS_MEDIA_HOVER_NONE;
    udom->media.any_hover = CSS_MEDIA_HOVER_NONE;

    udom->media.light_level = CSS_MEDIA_LIGHT_LEVEL_NORMAL;

    udom->media.scripting = CSS_MEDIA_SCRIPTING_NONE;

    udom->media.client_font_size = FLTTOFIX(14.4);   // 0.2 inch
    udom->media.client_line_height = INTTOFIX(FOIL_PX_PER_EM);

    return udom;

failed:
    if (udom) {
        udom_cleanup(udom);
        free(udom);
    }

    return NULL;
}

void foil_udom_delete(pcmcth_udom *udom)
{
    udom_cleanup(udom);
    free(udom);
}

pcmcth_udom *foil_udom_from_rdrbox(foil_rdrbox *box)
{
    foil_rdrbox *root = foil_rdrbox_get_root(box);
    assert(root->is_initial);
    return root->udom;
}

static void load_css(struct pcmcth_udom *udom, const char *href)
{
    char *css = NULL;
    size_t length;

    if (href[0] == '/' && href[1] != '/' && udom->base && udom->base->scheme &&
            strcasecmp(udom->base->scheme, "file") == 0) {

        LOG_DEBUG("Try to load CSS from file (absolute path): %s\n", href);
        css = purc_load_file_contents(href, &length);
    }
    else if (strchr(href, ':')) {
        /* href contains an absolute URL */
        struct purc_broken_down_url broken_down;

        memset(&broken_down, 0, sizeof(broken_down));
        pcutils_url_break_down(&broken_down, href);

        if (strcasecmp(broken_down.scheme, "file") == 0) {
            LOG_DEBUG("Try to load CSS from file (absolute path): %s\n",
                    broken_down.path);
            css = purc_load_file_contents(broken_down.path, &length);
        }
        else {
            LOG_WARN("Loading CSS from remote URL is not suppored: %s\n",
                    href);
            /* TODO: load from remote fetcher */
        }

        pcutils_broken_down_url_clear(&broken_down);
    }
    else if (udom->base && udom->base->scheme &&
            strcasecmp(udom->base->scheme, "file") == 0) {
        /* href contains a relative URL */
        char path[strlen(udom->base->path) + strlen(href) + 4];

        strcpy(path, udom->base->path);
        strcat(path, "/");
        strcat(path, href);

        LOG_DEBUG("Try to load CSS from file (relative path): %s\n", path);
        css = purc_load_file_contents(path, &length);
    }

    if (css) {
        css_error err;
        err = css_stylesheet_append_data(udom->author_sheet,
                (const unsigned char *)css, length);
        if (err != CSS_OK && err != CSS_NEEDDATA) {
            LOG_WARN("Failed to append css data from file: %d\n", err);
        }

        free(css);
    }
}

#define TAG_NAME_BASE           "base"
#define TAG_NAME_LINK           "link"
#define TAG_NAME_STYLE          "style"
#define TAG_NAME_TITLE          "title"

#define ATTR_NAME_STYLE         "style"
#define ATTR_NAME_HREF          "href"
#define ATTR_NAME_REL           "rel"
#define ATTR_NAME_TYPE          "type"
#define ATTR_NAME_LANG          "lang"

#define ATTR_VALUE_STYLESHEET   "stylesheet"
#define ATTR_VALUE_TEXT_CSS     "text/css"

static int
head_walker(purc_document_t doc, pcdoc_element_t element, void *ctxt)
{
    const char *name, *value;
    size_t len;
    char *css_href = NULL;

    struct pcmcth_udom *udom = (struct pcmcth_udom *)ctxt;
    pcdoc_element_get_tag_name(doc, element, &name, &len,
            NULL, NULL, NULL, NULL);

    if (len == (sizeof(TAG_NAME_BASE) - 1) &&
            strncasecmp(name, TAG_NAME_BASE, len) == 0) {

        if (pcdoc_element_get_attribute(doc, element, ATTR_NAME_HREF,
                &value, &len) == 0 && len > 0) {
            if (udom->base->scheme) {
                LOG_WARN("Multiple base element found; old base overridden\n");
                pcutils_broken_down_url_clear(udom->base);
            }

            char *base_url = strndup(value, len);
            if (!pcutils_url_break_down(udom->base, base_url)) {
                LOG_WARN("Bad href value for base element: %s\n", base_url);
            }
            free(base_url);
        }
    }
    else if (len == (sizeof(TAG_NAME_LINK) - 1) &&
            strncasecmp(name, TAG_NAME_LINK, len) == 0) {

        /* check if the value of attribute `rel` is `stylesheet` */
        if (pcdoc_element_get_attribute(doc, element, ATTR_NAME_REL,
                &value, &len) == 0 && len > 0) {
            if (len != (sizeof(ATTR_VALUE_STYLESHEET) - 1) ||
                    strncasecmp(value, ATTR_VALUE_STYLESHEET, len)) {
                goto done;
            }
        }

        /* check if the value of attribute `type` is `text/css`
        if (pcdoc_element_get_attribute(doc, element, ATTR_NAME_TYPE,
                &value, &len) == 0 && len > 0) {
            if (len != (sizeof(ATTR_VALUE_TEXT_CSS) - 1) ||
                    strncasecmp(value, ATTR_VALUE_TEXT_CSS, len)) {
                goto done;
            }
        }*/

        if (pcdoc_element_get_attribute(doc, element, ATTR_NAME_HREF,
                &value, &len) == 0 && len > 0) {
            css_href = strndup(value, len);
            load_css(udom, css_href);
        }
    }
    else if (len == (sizeof(TAG_NAME_STYLE) - 1) &&
            strncasecmp(name, TAG_NAME_STYLE, len) == 0) {

        pcdoc_node child = pcdoc_element_first_child(doc, element);
        while (child.data != NULL) {
            if (child.type == PCDOC_NODE_TEXT) {
                const char *text;
                size_t len;

                if (pcdoc_text_content_get_text(doc, child.text_node,
                            &text, &len) == 0 && len > 0) {
                    css_error err;
                    err = css_stylesheet_append_data(udom->author_sheet,
                            (const unsigned char *)text, len);
                    if (err != CSS_OK && err != CSS_NEEDDATA) {
                        LOG_ERROR("Failed to append css data: %d\n", err);
                        return -1;
                    }
                }
            }

            child = pcdoc_node_next_sibling(doc, child);
        }
    }
    else if (len == (sizeof(TAG_NAME_TITLE) - 1) &&
            strncasecmp(name, TAG_NAME_TITLE, len) == 0) {

        pcdoc_node child = pcdoc_element_first_child(doc, element);
        if (child.type == PCDOC_NODE_TEXT) {
            const char *text;
            size_t len;

            if (pcdoc_text_content_get_text(doc, child.text_node,
                        &text, &len) == 0 && len > 0) {
                LOG_DEBUG("title: %s\n", text);
                size_t consumed;
                consumed = foil_ustr_from_utf8_until_paragraph_boundary(
                        text, len, FOIL_WSR_NOWRAP,
                        &udom->title_ucs, &udom->title_len);
                if (consumed == 0) {
                    udom->title_ucs = NULL;
                    udom->title_len = 0;
                }
            }
        }
    }

done:
    if (css_href)
        free(css_href);
    return 0;
}

static css_select_results *
select_element_style(const css_media *media, css_select_ctx *select_ctx,
        pcmcth_udom *udom, pcdoc_element_t element,
        foil_rdrbox *parent_box)
{
    // prepare inline style
    css_error err;
    css_stylesheet* inline_sheet = NULL;
    const char* value;
    size_t len;

    if (pcdoc_element_get_attribute(udom->doc, element, ATTR_NAME_STYLE,
            &value, &len) == 0 && value) {
        css_stylesheet_params params;

        memset(&params, 0, sizeof(params));
        params.params_version = CSS_STYLESHEET_PARAMS_VERSION_1;
        params.level = CSS_LEVEL_DEFAULT;
        params.charset = FOIL_DEF_CHARSET;
        params.inline_style = true;
        params.url = "foo";
        params.title = "foo";
        params.resolve = resolve_url;
#if 0
        params.url = NULL;
        params.title = NULL;
        params.allow_quirks = false;
        params.resolve_pw = NULL;
        params.import = NULL;
        params.import_pw = NULL;
        params.color = NULL;
        params.color_pw = NULL;
        params.font = NULL;
        params.font_pw = NULL;
#endif

        err = css_stylesheet_create(&params, &inline_sheet);
        if (err == CSS_OK) {
            err = css_stylesheet_append_data(inline_sheet,
                    (const unsigned char *)value, len);
            if (err == CSS_OK || err == CSS_NEEDDATA) {
                css_stylesheet_data_done(inline_sheet);
            }
            else {
                LOG_WARN("Failed to append data to inline style sheet: %d\n",
                        err);
                css_stylesheet_destroy(inline_sheet);
                inline_sheet = NULL;
            }
        }
        else {
            LOG_WARN("Failed to create inline style sheet: %d\n", err);
        }
    }

    /* Select style for node */
    css_select_results *result = NULL;
    err = css_select_style(select_ctx, element, media, inline_sheet,
            &foil_css_select_handler, udom, &result);
    if (err != CSS_OK || result == NULL) {
        goto failed;
    }

    /* XXX: css_computed_style_compose() of CSSEng just clones the values for
       `inherit` for complex properties, e.g., `counter-reset` and
       `counter-increment`.

       This is not a smart way. One can optimize this by introducing
       reference count to the values of these complex properties. */
    css_computed_style *composed = NULL;
    if (parent_box && parent_box->computed_style) {
        err = css_computed_style_compose(
                parent_box->computed_style,
                result->styles[CSS_PSEUDO_ELEMENT_NONE],
                foil_css_select_handler.compute_font_size, NULL,
                &composed);
        if (err != CSS_OK) {
            goto failed;
        }

        css_computed_style_destroy(result->styles[CSS_PSEUDO_ELEMENT_NONE]);
        result->styles[CSS_PSEUDO_ELEMENT_NONE] = composed;
    }

    /* compose styles for pseudo elements */
    int pseudo_element;
    composed = NULL;
    for (pseudo_element = CSS_PSEUDO_ELEMENT_NONE + 1;
            pseudo_element < CSS_PSEUDO_ELEMENT_COUNT;
            pseudo_element++) {

        if (pseudo_element == CSS_PSEUDO_ELEMENT_FIRST_LETTER ||
                pseudo_element == CSS_PSEUDO_ELEMENT_FIRST_LINE)
            /* TODO: Handle first-line and first-letter pseudo
             *       element computed style completion */
            continue;

        if (result->styles[pseudo_element] == NULL)
            /* There were no rules concerning this pseudo element */
            continue;

        /* Complete the pseudo element's computed style, by composing
         * with the base element's style */
        err = css_computed_style_compose(
                result->styles[CSS_PSEUDO_ELEMENT_NONE],
                result->styles[pseudo_element],
                foil_css_select_handler.compute_font_size, NULL,
                &composed);
        if (err != CSS_OK) {
            /* TODO: perhaps this shouldn't be quite so
             * catastrophic? */
            goto failed;
        }

        /* Replace select_results style with composed style */
        css_computed_style_destroy(result->styles[pseudo_element]);
        result->styles[pseudo_element] = composed;
    }

    if (inline_sheet) {
        css_stylesheet_destroy(inline_sheet);
    }
    return result;

failed:
    if (inline_sheet) {
        css_stylesheet_destroy(inline_sheet);
    }

    if (result) {
        css_select_results_destroy(result);
    }

    return NULL;
}

static int
make_rdrtree(struct foil_create_ctxt *ctxt, pcdoc_element_t ancestor)
{
    char *tag_name = NULL;
    foil_rdrbox *box;
    css_select_results *result = NULL;

    result = select_element_style(&ctxt->udom->media,
            ctxt->udom->select_ctx, ctxt->udom, ancestor,
            ctxt->parent_box);
    if (result) {
        const char *name;
        size_t len;

        pcdoc_element_get_tag_name(ctxt->udom->doc, ancestor, &name, &len,
                NULL, NULL, NULL, NULL);
        assert(name != NULL && len > 0);
        tag_name = strndup(name, len);

        LOG_DEBUG("Creating boxes for element: %s\n", tag_name);

        ctxt->tag_name = tag_name;
        ctxt->elem = ancestor;
        ctxt->computed = result;

        /* skip descendants for "display: none" */
        if ((box = foil_rdrbox_create_principal(ctxt)) == NULL) {
            LOG_WARN("Non principal rdrbox created for element %s\n",
                    ctxt->tag_name);
            goto done;
        }

        sorted_array_add(ctxt->udom->elem2rdrbox, PTR2U64(ancestor), box);

        /* handle :before pseudo element */
        if (result->styles[CSS_PSEUDO_ELEMENT_BEFORE]) {
            if (foil_rdrbox_create_before(ctxt, box) == NULL) {
                LOG_WARN("Failed to create rdrbox for :before pseudo element\n");
                goto done;
            }
        }

    }
    else {
        goto failed;
    }

    pcdoc_node node;
    if (box->is_replaced || box->is_control) {
        /* skip contents if the element is a replaced one or a control */
        node.type = PCDOC_NODE_VOID;
        node.elem = NULL;
    }
    else {
        /* continue for the children */
        node = pcdoc_element_first_child(ctxt->udom->doc, ancestor);
    }

    while (node.type != PCDOC_NODE_VOID) {

        if (node.type == PCDOC_NODE_ELEMENT) {
            ctxt->parent_box = box;
            if (make_rdrtree(ctxt, node.elem))
                goto failed;
        }
        else if (node.type == PCDOC_NODE_TEXT) {
            const char *text = NULL;
            size_t len = 0;
            pcdoc_text_content_get_text(ctxt->udom->doc, node.text_node,
                    &text, &len);

            if (text && len > 0) {

                if (box->type == FOIL_RDRBOX_TYPE_INLINE &&
                        box->inline_data->nr_paras == 0) {
                    if (!foil_rdrbox_init_inline_data(ctxt, box, text, len))
                        goto done;
                }
                else {
                    foil_rdrbox *my_box;
                    if ((my_box = foil_rdrbox_create_anonymous_inline(ctxt,
                                    box)) == NULL)
                        goto done;

                    if (!foil_rdrbox_init_inline_data(ctxt, my_box, text, len))
                        goto done;
                }
            }
        }
        else if (node.type == PCDOC_NODE_CDATA_SECTION) {
            LOG_WARN("Node type 'PCDOC_NODE_CDATA_SECTION' skipped\n");
        }

        node = pcdoc_node_next_sibling(ctxt->udom->doc, node);
    }

    /* handle :after pseudo element */
    ctxt->tag_name = tag_name;
    ctxt->elem = ancestor;
    ctxt->computed = result;
    ctxt->parent_box = box->parent;
    if (result->styles[CSS_PSEUDO_ELEMENT_AFTER]) {
        if (foil_rdrbox_create_after(ctxt, box) == NULL) {
            LOG_WARN("Failed to create rdrbox for :after pseudo element\n");
            goto done;
        }
    }

done:
    if (result) {
        css_select_results_destroy(result);
    }
    if (tag_name)
        free(tag_name);
    return 0;

failed:
    if (result) {
        css_select_results_destroy(result);
    }
    if (tag_name)
        free(tag_name);
    return -1;
}

static int
make_rdrbox_subtree(struct foil_create_ctxt *ctxt, foil_rdrbox *box)
{
    char *tag_name = NULL;

    pcdoc_element *ancestor = box->owner;
    pcdoc_node node;
    if (box->is_replaced || box->is_control) {
        /* skip contents if the element is a replaced one or a control */
        node.type = PCDOC_NODE_VOID;
        node.elem = NULL;
    }
    else {
        /* continue for the children */
        node = pcdoc_element_first_child(ctxt->udom->doc, ancestor);
    }

    while (node.type != PCDOC_NODE_VOID) {

        if (node.type == PCDOC_NODE_ELEMENT) {
            ctxt->parent_box = box;
            if (make_rdrtree(ctxt, node.elem))
                goto failed;
        }
        else if (node.type == PCDOC_NODE_TEXT) {
            const char *text = NULL;
            size_t len = 0;
            pcdoc_text_content_get_text(ctxt->udom->doc, node.text_node,
                    &text, &len);

            if (text && len > 0) {
                if (box->type == FOIL_RDRBOX_TYPE_INLINE &&
                        box->inline_data->nr_paras == 0) {
                    if (!foil_rdrbox_init_inline_data(ctxt, box, text, len))
                        goto done;
                }
                else {
                    foil_rdrbox *my_box;
                    if ((my_box = foil_rdrbox_create_anonymous_inline(ctxt,
                                    box)) == NULL)
                        goto done;

                    if (!foil_rdrbox_init_inline_data(ctxt, my_box, text, len))
                        goto done;
                }
            }
        }
        else if (node.type == PCDOC_NODE_CDATA_SECTION) {
            LOG_WARN("Node type 'PCDOC_NODE_CDATA_SECTION' skipped\n");
        }

        node = pcdoc_node_next_sibling(ctxt->udom->doc, node);
    }

done:
    if (tag_name)
        free(tag_name);
    return 0;

failed:
    if (tag_name)
        free(tag_name);
    return -1;
}


static int
create_anonymous_blocks_for_block_container(struct foil_create_ctxt *ctxt,
        struct foil_rdrbox *box)
{
    assert(box->is_block_container);

    /* handle inline boxes before any block children */
    foil_rdrbox *child = box->first;
    foil_rdrbox *start = NULL;
    while (child) {

        if (child->is_inline_level && start == NULL)
            start = child;

        if (child->is_block_level && start != NULL) {

            foil_rdrbox *block;
            block = foil_rdrbox_create_anonymous_block(ctxt, box, start, NULL);
            if (block == NULL)
                goto failed;

            foil_rdrbox *inln = start;
            while (inln != NULL && inln != child) {
                foil_rdrbox *next = inln->next;

                if (inln->is_inline_level) {
                    foil_rdrbox_remove_from_tree(inln);
                    foil_rdrbox_append_child(block, inln);
                }
                inln = next;
            }

            start = NULL;
        }

        child = child->next;
    }

    /* handle left inline boxes */
    if (start) {
        foil_rdrbox *block;
        block = foil_rdrbox_create_anonymous_block(ctxt, box, start, NULL);
        if (block == NULL)
            goto failed;

        foil_rdrbox *inln = start;
        while (inln != NULL) {
            foil_rdrbox *next = inln->next;

            if (inln->is_inline_level) {
                foil_rdrbox_remove_from_tree(inln);
                foil_rdrbox_append_child(block, inln);
            }
            inln = next;
        }
    }

    return 0;

failed:
    return -1;
}

static int
create_anonymous_blocks_for_inline_box(struct foil_create_ctxt *ctxt,
        struct foil_rdrbox *box)
{
    assert(box->is_inline_box && box->parent);

    /* create a new anonymous block box and insert before box */
    foil_rdrbox *block;
    block = foil_rdrbox_create_anonymous_block(ctxt, box->parent, box, NULL);
    if (block == NULL)
        goto failed;

    /* move the current box as the child of the anonymous block box */
    foil_rdrbox_remove_from_tree(box);
    foil_rdrbox_append_child(block, box);

    /* travel for the children of the current box */
    foil_rdrbox *child = box->first;
    foil_rdrbox *last_sibling = block;
    while (child) {
        foil_rdrbox *next = child->next;

        if (child->is_inline_level) {
            if (block == NULL) {
                block = foil_rdrbox_create_anonymous_block(ctxt,
                        box->parent->parent, NULL, last_sibling);
                if (block == NULL)
                    goto failed;
                last_sibling = block;
            }

            foil_rdrbox_remove_from_tree(child);
            foil_rdrbox_append_child(block, child);
        }
        else if (child->is_block_level) {

            foil_rdrbox_remove_from_tree(child);
            foil_rdrbox_append_child(block->parent, child);
            last_sibling = child;
            block = NULL;   /* mark to create a new anonyouse block */
        }

        child = next;
    }

    return 0;

failed:
    return -1;
}

static int
normalize_rdrtree(struct foil_create_ctxt *ctxt,
        struct foil_rdrbox *box)
{
    unsigned nr_inlines = 0;
    unsigned nr_blocks = 0;

    /* continue for the children */
    foil_rdrbox *child = box->first;
    while (child) {

        // remove empty anonymous inline box
        if (child->type == FOIL_RDRBOX_TYPE_INLINE && child->is_anonymous &&
                child->first == NULL && child->inline_data->nr_paras == 0) {

            foil_rdrbox *tmp = child;
            child = child->next;

            foil_rdrbox_delete(tmp);
            LOG_WARN("an empty anonymous inline box removed\n");
            continue;
        }

        if (child->is_inline_level)
            nr_inlines++;
        else
            nr_blocks++;

        child = child->next;
    }

#ifndef NDEBUG
    char *name = foil_rdrbox_get_name(ctxt->udom->doc, box);
    LOG_DEBUG("box (%s) has %u inlines and %u blocks\n",
            name, nr_inlines, nr_blocks);
    free(name);
#endif

    if (box->type == FOIL_RDRBOX_TYPE_LIST_ITEM &&
            box->list_item_data->marker_box) {
        if (!foil_rdrbox_init_marker_data(ctxt,
                    box->list_item_data->marker_box, box)) {
            LOG_ERROR("Failed to initialize marker box\n");
            goto failed;
        }
    }

    if (box->is_block_container && nr_inlines > 0 && nr_blocks > 0) {
        /* force the box to have only block-level boxes
           by creating anonymous block box */
        if (create_anonymous_blocks_for_block_container(ctxt, box))
            goto failed;
    }
    else if (box->is_inline_box && nr_blocks > 0) {
        if (create_anonymous_blocks_for_inline_box(ctxt, box))
            goto failed;
    }

    /* continue for the children */
    child = box->first;
    while (child) {

        if (child->first)
            if (normalize_rdrtree(ctxt, child))
                goto failed;

        child = child->next;
    }

    return 0;

failed:
    return -1;
}

static void
pre_layout_rdrtree(struct foil_layout_ctxt *ctxt, struct foil_rdrbox *box)
{
    if (box != ctxt->initial_cblock)
        foil_rdrbox_pre_layout(ctxt, box);

    /* continue for the children */
    foil_rdrbox *child = box->first;
    while (child) {
        pre_layout_rdrtree(ctxt, child);
        child = child->next;
    }
}

static void
resolve_widths(struct foil_layout_ctxt *ctxt, struct foil_rdrbox *box)
{
    if (!box->is_width_resolved) {
        foil_rdrbox_resolve_width(ctxt, box);
    }

    /* continue for the children */
    foil_rdrbox *child = box->first;
    while (child) {
        resolve_widths(ctxt, child);
        child = child->next;
    }
}

static void
resolve_heights(struct foil_layout_ctxt *ctxt, struct foil_rdrbox *box)
{
    if (!box->is_height_resolved) {
        foil_rdrbox_resolve_height(ctxt, box);
    }

    /* continue for the children */
    foil_rdrbox *child = box->first;
    while (child) {
        resolve_heights(ctxt, child);
        child = child->next;
    }
}

static void
layout_rdrtree(struct foil_layout_ctxt *ctxt, struct foil_rdrbox *box)
{
    if ((box->is_block_level)
            && box->nr_inline_level_children > 0) {
        foil_rdrbox_lay_lines_in_block(ctxt, box);

        /* layout children */
        foil_rdrbox *child = box->first;
        while (child) {
            layout_rdrtree(ctxt, child);
            child = child->next;
        }
    }
    else if (box->is_block_container) {
        if (box->nr_inline_level_children > 0) {
            foil_rdrbox_lay_lines_in_block(ctxt, box);
        }

        foil_rdrbox *child = box->first;
        while (child) {
            if (child->is_block_level) {
                if (child->position && child->is_abs_positioned) {
                    foil_rdrbox_lay_abs_in_container(ctxt,
                                child->cblock_creator, child);
                }
                else if (child->floating) {
                    foil_rect_offset(&child->ctnt_rect,
                            box->ctnt_rect.left, box->ctnt_rect.top);
                }
                else {
                    foil_rdrbox_lay_block_in_container(ctxt, box, child);
                }
            }

            layout_rdrtree(ctxt, child);
            child = child->next;
        }
    }

    if (box->type == FOIL_RDRBOX_TYPE_LIST_ITEM &&
            box->list_item_data->marker_box) {
        foil_rdrbox_lay_marker_box(ctxt, box);
    }
}

uint8_t
foil_udom_get_langcode(purc_document_t doc, pcdoc_element_t elem)
{
    const char *value;
    size_t len;

    if (elem == NULL)
        elem = purc_document_root(doc);

    if (pcdoc_element_get_attribute(doc, elem, ATTR_NAME_LANG,
                &value, &len) == 0 && len == 2) {
        return (uint8_t)foil_langcode_from_iso639_1(value);
    }

    return (uint8_t)FOIL_LANGCODE_unknown;
}

#ifndef NDEBUG
static void
dump_rdrtree(struct foil_render_ctxt *ctxt, struct foil_rdrbox *ancestor,
        unsigned level)
{
    foil_rdrbox_dump(ancestor, ctxt->udom->doc, level);

    /* travel children */
    foil_rdrbox *child = ancestor->first;
    while (child) {

        dump_rdrtree(ctxt, child, level + 1);

        child = child->next;
    }
}

static void dump_udom(pcmcth_udom *udom)
{
    /* render the whole tree */
    foil_render_ctxt render_ctxt = { .udom = udom };

    /* dump the whole tree */
    LOG_DEBUG("Calling dump_rdrtree...\n");
    dump_rdrtree(&render_ctxt, udom->initial_cblock, 0);
}
#endif

pcmcth_udom *
foil_udom_load_edom(pcmcth_page *page, purc_variant_t edom, int *retv)
{
    purc_document_t edom_doc;
    purc_document_type_k doc_type;
    pcmcth_udom *udom = NULL;

    edom_doc = purc_variant_native_get_entity(edom);
    assert(edom_doc);

    void *_impl = purc_document_impl_entity(edom_doc, &doc_type);

    if (_impl == NULL) {
        *retv = PCRDR_SC_NO_CONTENT;
        goto failed;
    }
    else if (doc_type != PCDOC_K_TYPE_HTML && doc_type != PCDOC_K_TYPE_XML) {
        *retv = PCRDR_SC_NOT_ACCEPTABLE;
        goto failed;
    }

    udom = foil_udom_new(page);
    if (udom == NULL) {
        *retv = PCRDR_SC_INSUFFICIENT_STORAGE;
        goto failed;
    }

    /* save edom_doc for CSS select handlers */
    udom->doc = edom_doc;

    /* get default language code */
    udom->initial_cblock->lang_code = foil_udom_get_langcode(edom_doc, NULL);
    if (udom->initial_cblock->lang_code == FOIL_LANGCODE_unknown) {
        // XXX: default lang
        udom->initial_cblock->lang_code = FOIL_LANGCODE_en;
    }
    udom->initial_cblock->quotes =
        foil_quotes_get_initial(udom->initial_cblock->lang_code);

    // parse and append style sheets
    pcdoc_element_t head;
    head = purc_document_head(edom_doc);
    if (head) {
        css_error err;

        css_stylesheet_params params;
        memset(&params, 0, sizeof(params));
        params.params_version = CSS_STYLESHEET_PARAMS_VERSION_1;
        params.level = CSS_LEVEL_DEFAULT;
        params.charset = FOIL_DEF_CHARSET;
        params.url = "foo";
        params.title = "foo";
        params.resolve = resolve_url;

        err = css_stylesheet_create(&params, &udom->author_sheet);
        if (err != CSS_OK) {
            *retv = PCRDR_SC_INSUFFICIENT_STORAGE;
            LOG_ERROR("Failed to create default user agent sheet: %d\n", err);
            goto failed;
        }

        size_t n;
        pcdoc_travel_descendant_elements(edom_doc, head, head_walker,
                udom, &n);

        size_t sz;
        css_stylesheet_size(udom->author_sheet, &sz);
        if (sz == 0) {
            css_stylesheet_destroy(udom->author_sheet);
            udom->author_sheet = NULL;
        }
        else {
            css_stylesheet_data_done(udom->author_sheet);
            err = css_select_ctx_append_sheet(udom->select_ctx,
                    udom->author_sheet, CSS_ORIGIN_AUTHOR, NULL);
            if (err != CSS_OK) {
                *retv = PCRDR_SC_INSUFFICIENT_STORAGE;
                LOG_ERROR("Failed to append author style sheet: %d\n", err);
                goto failed;
            }
        }
    }

    /* create the box tree */
    foil_create_ctxt ctxt = { udom,
        udom->initial_cblock,           /* initial box */
        NULL,                           /* root box */
        udom->initial_cblock,           /* parent box */
        purc_document_root(edom_doc),   /* root element */
        purc_document_body(edom_doc),   /* body element */
        NULL, NULL, NULL, NULL };
    if (make_rdrtree(&ctxt, ctxt.root))
        goto failed;

    /* check and create anonymous block box if need */
    LOG_DEBUG("Calling normalize_rdrtree...\n");
    if (normalize_rdrtree(&ctxt, udom->initial_cblock))
        goto failed;

    /* determine the geometries of boxes and lay out the boxes */
    foil_layout_ctxt layout_ctxt = { udom, udom->initial_cblock };
    LOG_DEBUG("Calling pre_layout_rdrtree...\n");
    pre_layout_rdrtree(&layout_ctxt, udom->initial_cblock);

    LOG_DEBUG("Calling resolve_widths...\n");
    resolve_widths(&layout_ctxt, udom->initial_cblock);

    LOG_DEBUG("Calling resolve_heights...\n");
    resolve_heights(&layout_ctxt, udom->initial_cblock);

    LOG_DEBUG("Calling layout_rdrtree...\n");
    layout_rdrtree(&layout_ctxt, udom->initial_cblock);

#ifndef NDEBUG
    dump_udom(udom);
    foil_udom_render_to_file(udom, stdout);
#endif

    assert(udom->initial_cblock->width % FOIL_PX_GRID_CELL_W == 0);
    assert(udom->initial_cblock->height % FOIL_PX_GRID_CELL_H == 0);

    int cols = udom->initial_cblock->width / FOIL_PX_GRID_CELL_W;
    int rows = udom->initial_cblock->height / FOIL_PX_GRID_CELL_H;
    if (!foil_page_content_init(page, cols, rows,
                udom->initial_cblock->color,
                udom->initial_cblock->background_color)) {
        LOG_ERROR("Failed to initialize page content\n");
        goto failed;
    }

    foil_udom_render_to_page(udom);
    foil_page_expose(page);
    return udom;

failed:
    if (udom)
        foil_udom_delete(udom);
    page->udom = NULL;
    return NULL;
}

foil_rdrbox *foil_udom_find_rdrbox(pcmcth_udom *udom,
        uint64_t element_handle)
{
    void *data;

    if (sorted_array_find(udom->elem2rdrbox, element_handle, &data) < 0) {
        return NULL;
    }

    return data;
}

static void foil_rdrbox_delete_children(foil_rdrbox *root)
{
    foil_rdrbox *tmp;
    foil_rdrbox *box = root;

    while (box) {
        if (box->first) {
            box = box->first;
        }
        else {
            while (box != root && box->next == NULL) {
                tmp = box->parent;
                foil_rdrbox_delete(box);
                box = tmp;
            }

            if (box == root) {
                break;
            }

            tmp = box->next;
            foil_rdrbox_delete(box);
            box = tmp;
        }
    }
    box->nr_block_level_children = 0;
    box->nr_inline_level_children = 0;
    box->nr_floating_children = 0;
    box->nr_abspos_children = 0;
}

static int rebuild_subtree(pcmcth_udom *udom, foil_rdrbox *box)
{
    foil_rdrbox_delete_children(box);
    foil_create_ctxt ctxt = { udom,
        udom->initial_cblock,           /* initial box */
        NULL,                           /* root box */
        box->parent,                     /* parent box */
        purc_document_root(udom->doc),   /* root element */
        purc_document_body(udom->doc),   /* body element */
        NULL, NULL, NULL, NULL };
    if (make_rdrbox_subtree(&ctxt, box)) {
        goto failed;
    }

    if (normalize_rdrtree(&ctxt, box)) {
        goto failed;
    }

    return 0;

failed:
    return -1;
}

static void reset_rdrbox_layout_info(pcmcth_udom *udom, foil_rdrbox *box)
{
    (void)udom;
    box->is_width_resolved = 0;
    box->is_height_resolved = 0;
    box->is_in_normal_flow = 0;
    box->is_in_flow = 0;
    box->is_zidx_auto = 0;
    box->width = 0;
    box->height = 0;
    box->prop_for_width = 0;
    box->prop_for_height = 0;
    box->is_width_resolved = 0;
    box->is_height_resolved = 0;
    box->nr_block_level_children = 0;
    box->nr_inline_level_children = 0;
    box->nr_floating_children = 0;
    box->nr_abspos_children = 0;
    box->ml = box->mt = box->mr = box->mb = 0;
    box->pl = box->pt = box->pr = box->pb = 0;
    foil_rect_set(&box->ctnt_rect, 0, 0, 0, 0);
}

#define COMP_AND_UPDATE_CRUX(prop)                                            \
    if (box->prop != tmpbox->prop) {                                          \
        box->prop = tmpbox->prop;                                             \
        *crux_changed = 1;                                                    \
    }

#define COMP_AND_UPDATE(prop)                                                 \
    if (box->prop != tmpbox->prop) {                                          \
        box->prop = tmpbox->prop;                                             \
    }

#define VERIFY_PROP(prop_auto)                                                \
    if (value != tmp_value) {                                                 \
        *crux_changed = true;                                                 \
    }                                                                         \
    else if (value != prop_auto) {                                            \
        v = used_length(ctxt->udom, box, length, unit);                       \
        tmp_v = used_length(ctxt->udom, box, tmp_length, tmp_unit);           \
        if (v != tmp_v) {                                                     \
            *crux_changed = true;                                             \
        }                                                                     \
    }

#define FOIL_FPCT_TOFLOAT(v) (FIXTOFLT(FDIV(v, F_100)))

static float used_length(pcmcth_udom *udom, foil_rdrbox *box,
        css_unit unit, css_fixed length)
{
    float v = 0;

    switch (unit) {
    case CSS_UNIT_PCT:
        v = foil_rect_width(&box->cblock_creator->ctnt_rect);
        v = v * FOIL_FPCT_TOFLOAT(length);
        break;

    case CSS_UNIT_PX:
        v = FIXTOFLT(length);
        break;

    /* font-relative lengths */
    case CSS_UNIT_EX:
        // The x-height is so called because it is often
        // equal to the height of the lowercase "x".
        v = FIXTOFLT(length) * FOIL_PX_GRID_CELL_W;
        break;

    case CSS_UNIT_EM:
    case CSS_UNIT_CH:
    case CSS_UNIT_REM:
        // Equal to the used advance measure of the "0" glyph
        v = FIXTOFLT(length) * FOIL_PX_GRID_CELL_H;
        break;

    /* absolute lengths */
    case CSS_UNIT_CM:
        v = FIXTOFLT(length) * FOIL_DEF_DPI/2.54;
        break;
    case CSS_UNIT_IN:
        v = FIXTOFLT(length) * FOIL_DEF_DPI;
        break;
    case CSS_UNIT_MM:
        v = FIXTOFLT(length) * FOIL_DEF_DPI/2.54/10;
        break;
    case CSS_UNIT_PC:
        v = FIXTOFLT(length) * FOIL_DEF_DPI/6.0;
        break;
    case CSS_UNIT_PT:
        v = FIXTOFLT(length) * FOIL_DEF_DPI/72.0;
        break;
    case CSS_UNIT_Q:
        v = FIXTOFLT(length) * FOIL_DEF_DPI/2.54/40;
        break;

    /* viewport-relative lengths */
    case CSS_UNIT_VW:
        v = FIXTOFLT(length) * udom->vw / 100;
        break;
    case CSS_UNIT_VH:
        v = FIXTOFLT(length) * udom->vh / 100;
        break;
    case CSS_UNIT_VMAX:
        if (udom->vh > udom->vw)
            v = FIXTOFLT(length) * udom->vh / 100;
        else
            v = FIXTOFLT(length) * udom->vw / 100;
        break;
    case CSS_UNIT_VMIN:
        if (udom->vh > udom->vw)
            v = FIXTOFLT(length) * udom->vw / 100;
        else
            v = FIXTOFLT(length) * udom->vh / 100;
        break;

    default:
        // TODO: support more unit
        LOG_WARN("TODO: not supported unit: %d\n", unit);
        break;
    }

    return v;
}

static int compare_and_update_properties(foil_create_ctxt *ctxt,
        foil_rdrbox *box, int *crux_changed)
{
    (void) ctxt;
    (void) box;
    (void) crux_changed;
    foil_rdrbox *tmpbox;
    if ((tmpbox = foil_rdrbox_create_from_style(ctxt)) == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    if (box->type != tmpbox->type) {
        *crux_changed = 1;
        if (box->extra_data) {
            if (box->extra_data_cleaner) {
                box->extra_data_cleaner(box->extra_data);
            }
            free(box->extra_data);
        }
        box->extra_data = tmpbox->extra_data;
        box->extra_data_cleaner = tmpbox->extra_data_cleaner;
        tmpbox->extra_data = NULL;
        tmpbox->extra_data_cleaner = NULL;
    }

    COMP_AND_UPDATE_CRUX(position);
    COMP_AND_UPDATE_CRUX(is_abs_positioned);
    COMP_AND_UPDATE_CRUX(floating);
    COMP_AND_UPDATE_CRUX(clear);
    COMP_AND_UPDATE_CRUX(is_block_level);
    COMP_AND_UPDATE_CRUX(is_inline_level);

    /* foil_rdrbox_dtrm_common_properties */
    COMP_AND_UPDATE_CRUX(direction);
    COMP_AND_UPDATE_CRUX(visibility);
    COMP_AND_UPDATE(overflow_x);
    COMP_AND_UPDATE(overflow_y);
    COMP_AND_UPDATE(unicode_bidi);
    COMP_AND_UPDATE(text_transform);
    COMP_AND_UPDATE(white_space);
    COMP_AND_UPDATE(text_deco_blink);
    COMP_AND_UPDATE(text_deco_line_through);
    COMP_AND_UPDATE(text_deco_overline);
    COMP_AND_UPDATE(text_deco_underline);
    COMP_AND_UPDATE(word_break);
    COMP_AND_UPDATE(line_break);
    COMP_AND_UPDATE(word_wrap);
    COMP_AND_UPDATE(list_style_type);
    COMP_AND_UPDATE(list_style_position);
    COMP_AND_UPDATE(color.specified);
    COMP_AND_UPDATE(color.argb);
    COMP_AND_UPDATE(background_color.specified);
    COMP_AND_UPDATE(background_color.argb);
    COMP_AND_UPDATE(quotes);

    uint8_t value, tmp_value;
    css_fixed length, tmp_length;
    css_unit unit, tmp_unit;
    float v, tmp_v;
    value = css_computed_margin_left(box->computed_style, &length, &unit);
    tmp_value = css_computed_margin_left(ctxt->style, &tmp_length, &tmp_unit);
    VERIFY_PROP(CSS_MARGIN_AUTO);

    value = css_computed_margin_top(box->computed_style, &length, &unit);
    tmp_value = css_computed_margin_top(ctxt->style, &tmp_length, &tmp_unit);
    VERIFY_PROP(CSS_MARGIN_AUTO);

    value = css_computed_margin_right(box->computed_style, &length, &unit);
    tmp_value = css_computed_margin_right(ctxt->style, &tmp_length, &tmp_unit);
    VERIFY_PROP(CSS_MARGIN_AUTO);

    value = css_computed_margin_bottom(box->computed_style, &length, &unit);
    tmp_value = css_computed_margin_bottom(ctxt->style, &tmp_length, &tmp_unit);
    VERIFY_PROP(CSS_MARGIN_AUTO);

    value = css_computed_border_left_width(box->computed_style, &length, &unit);
    tmp_value = css_computed_border_left_width(ctxt->style, &tmp_length, &tmp_unit);
    if (value != tmp_value) {
        *crux_changed = true;
    }
    else if (value == CSS_BORDER_WIDTH_WIDTH) {
        v = used_length(ctxt->udom, box, length, unit);
        tmp_v = used_length(ctxt->udom, box, tmp_length, tmp_unit);
        if (v != tmp_v) {
            *crux_changed = true;
        }
    }

    value = css_computed_border_top_width(box->computed_style, &length, &unit);
    tmp_value = css_computed_border_top_width(ctxt->style, &tmp_length, &tmp_unit);
    if (value != tmp_value) {
        *crux_changed = true;
    }
    else if (value == CSS_BORDER_WIDTH_WIDTH) {
        v = used_length(ctxt->udom, box, length, unit);
        tmp_v = used_length(ctxt->udom, box, tmp_length, tmp_unit);
        if (v != tmp_v) {
            *crux_changed = true;
        }
    }

    value = css_computed_border_right_width(box->computed_style, &length, &unit);
    tmp_value = css_computed_border_right_width(ctxt->style, &tmp_length, &tmp_unit);
    if (value != tmp_value) {
        *crux_changed = true;
    }
    else if (value == CSS_BORDER_WIDTH_WIDTH) {
        v = used_length(ctxt->udom, box, length, unit);
        tmp_v = used_length(ctxt->udom, box, tmp_length, tmp_unit);
        if (v != tmp_v) {
            *crux_changed = true;
        }
    }

    value = css_computed_border_bottom_width(box->computed_style, &length, &unit);
    tmp_value = css_computed_border_bottom_width(ctxt->style, &tmp_length, &tmp_unit);
    if (value != tmp_value) {
        *crux_changed = true;
    }
    else if (value == CSS_BORDER_WIDTH_WIDTH) {
        v = used_length(ctxt->udom, box, length, unit);
        tmp_v = used_length(ctxt->udom, box, tmp_length, tmp_unit);
        if (v != tmp_v) {
            *crux_changed = true;
        }
    }

    value = css_computed_left(box->computed_style, &length, &unit);
    tmp_value = css_computed_left(ctxt->style, &tmp_length, &tmp_unit);
    VERIFY_PROP(CSS_LEFT_AUTO);

    value = css_computed_top(box->computed_style, &length, &unit);
    tmp_value = css_computed_top(ctxt->style, &tmp_length, &tmp_unit);
    VERIFY_PROP(CSS_TOP_AUTO);

    value = css_computed_right(box->computed_style, &length, &unit);
    tmp_value = css_computed_right(ctxt->style, &tmp_length, &tmp_unit);
    VERIFY_PROP(CSS_RIGHT_AUTO);

    value = css_computed_bottom(box->computed_style, &length, &unit);
    tmp_value = css_computed_bottom(ctxt->style, &tmp_length, &tmp_unit);
    VERIFY_PROP(CSS_BOTTOM_AUTO);

    value = css_computed_width(box->computed_style, &length, &unit);
    tmp_value = css_computed_width(ctxt->style, &tmp_length, &tmp_unit);
    VERIFY_PROP(CSS_WIDTH_AUTO);

    value = css_computed_height(box->computed_style, &length, &unit);
    tmp_value = css_computed_height(ctxt->style, &tmp_length, &tmp_unit);
    VERIFY_PROP(CSS_HEIGHT_AUTO);

    foil_rdrbox_delete(tmpbox);
    return 0;

failed:
    return -1;
}

static void reset_rdrbox_layout_deep(pcmcth_udom *udom, foil_rdrbox *box)
{
    reset_rdrbox_layout_info(udom, box);

    foil_rdrbox *child = box->first;
    while (child) {
        reset_rdrbox_layout_deep(udom, child);
        child = child->next;
    }
}

static void erase_bg(pcmcth_udom *udom, foil_rdrbox *box, foil_rect origin_rc)
{
    foil_render_ctxt ctxt = { .udom = udom, .fp = NULL };
    if (box->tailor_ops && box->tailor_ops->bgnd_painter) {
        box->tailor_ops->bgnd_painter(&ctxt, box);
    }
    else {
        foil_rect page_rc;
        const foil_rect *rc;
        if (box->is_root) {
            rc = NULL;
        }
        else {
            foil_rdrbox_map_rect_to_page(&origin_rc, &page_rc);
            rc = &page_rc;
        }
        foil_page_set_bgc(udom->page, box->background_color);
        foil_page_erase_rect(udom->page, rc);
    }
}

static struct foil_rdrbox *get_rdrbox_container(pcmcth_udom *udom,
        struct foil_rdrbox *rdrbox)
{
    struct foil_rdrbox *container = rdrbox->parent;
    while (container && container != rdrbox->cblock_creator) {
        container = container->parent;
    }

    if (!container) {
        container = udom->initial_cblock;
    }
    return container;
}

static struct foil_rdrbox *relayout_rdrtree(struct foil_layout_ctxt *ctxt,
        struct foil_rdrbox *rdrbox, foil_rect origin_rc)
{
again:
    reset_rdrbox_layout_deep(ctxt->udom, rdrbox);
    if (rdrbox == ctxt->udom->initial_cblock) {
        int width = ctxt->udom->vw;
        int height = ctxt->udom->vh;
        ctxt->udom->initial_cblock->is_initial = 1;
        ctxt->udom->initial_cblock->is_block_level = 1;
        ctxt->udom->initial_cblock->is_block_container = 1;
        ctxt->udom->initial_cblock->is_width_resolved = 1;

        ctxt->udom->initial_cblock->width = width;
        ctxt->udom->initial_cblock->height = height;

        ctxt->udom->initial_cblock->color.specified = 0;
        ctxt->udom->initial_cblock->color.argb = FOIL_DEF_FGC;
        ctxt->udom->initial_cblock->background_color.specified = 0;
        ctxt->udom->initial_cblock->background_color.argb = FOIL_DEF_BGC;

        ctxt->udom->initial_cblock->ctnt_rect.left = 0;
        ctxt->udom->initial_cblock->ctnt_rect.top = 0;
        ctxt->udom->initial_cblock->ctnt_rect.right = width;
        ctxt->udom->initial_cblock->ctnt_rect.bottom = height;
        ctxt->udom->initial_cblock->cblock_creator = NULL;
    }

    pre_layout_rdrtree(ctxt, rdrbox);
    resolve_widths(ctxt, rdrbox);
    resolve_heights(ctxt, rdrbox);

    if (rdrbox != ctxt->udom->initial_cblock) {
        int ow = foil_rect_width(&origin_rc);
        int oh = foil_rect_height(&origin_rc);
        int w = rdrbox->width;
        int h = rdrbox->height;

        if (ow != w || oh != h) {
            rdrbox = get_rdrbox_container(ctxt->udom, rdrbox);
            origin_rc = rdrbox->ctnt_rect;
            goto again;
        }

        foil_rect_set(&rdrbox->ctnt_rect, origin_rc.left, origin_rc.top,
                origin_rc.left + rdrbox->width, origin_rc.top + rdrbox->height);
    }

    layout_rdrtree(ctxt, rdrbox);
    erase_bg(ctxt->udom, rdrbox, origin_rc);

    if (rdrbox == ctxt->udom->initial_cblock) {
        int cols = ctxt->udom->initial_cblock->width / FOIL_PX_GRID_CELL_W;
        int rows = ctxt->udom->initial_cblock->height / FOIL_PX_GRID_CELL_H;

        if (rows > ctxt->udom->page->rows || cols > ctxt->udom->page->cols) {
            foil_page_set_row_col(ctxt->udom->page, cols, rows);
        }
        rdrbox = ctxt->udom->initial_cblock->first;
    }
    return rdrbox;
}


static int on_update_style(pcmcth_udom *udom, foil_rdrbox *rdrbox,
    pcdoc_element_t ref_elem, int op)
{
    (void)ref_elem;
    (void)op;
    int r = PCRDR_SC_NOT_IMPLEMENTED;
    foil_rdrbox *render_box = rdrbox;
    pcdoc_element *ancestor = rdrbox->owner;
    css_select_results *result = NULL;
    foil_layout_ctxt layout_ctxt = { udom, udom->initial_cblock };

    result = select_element_style(&udom->media,
            udom->select_ctx, udom, ancestor,
            rdrbox->parent);
    if (!result) {
        goto failed;
    }

    css_computed_style *style = result->styles[CSS_PSEUDO_ELEMENT_NONE];

    if (css_computed_style_is_equal(style, rdrbox->computed_style)) {
        r = PCRDR_SC_OK;
        goto done;
    }
    foil_create_ctxt ctxt = { udom,
        udom->initial_cblock,           /* initial box */
        NULL,                           /* root box */
        rdrbox->parent,                     /* parent box */
        purc_document_root(udom->doc),   /* root element */
        purc_document_body(udom->doc),   /* body element */
        ancestor,
        result,                          /* computed */
        style,                           /* style */
        NULL };

    int crux_changed = 0;
    if (compare_and_update_properties(&ctxt, rdrbox, &crux_changed)) {
        goto failed;
    }

    if (rdrbox->computed_style) {
        css_computed_style_destroy(rdrbox->computed_style);
    }
    rdrbox->computed_style = style;
    result->styles[CSS_PSEUDO_ELEMENT_NONE] = NULL;

    if (!crux_changed) {
        /* sizing, border property */
        pre_layout_rdrtree(&layout_ctxt, rdrbox);
        goto render;
    }

    render_box = get_rdrbox_container(udom, rdrbox);

    render_box = relayout_rdrtree(&layout_ctxt, render_box, render_box->ctnt_rect);

render:
    foil_udom_invalidate_rdrbox(udom, render_box);
    r = PCRDR_SC_OK;

done:
    if (result) {
        css_select_results_destroy(result);
    }
    return r;

failed:
    if (result) {
        css_select_results_destroy(result);
    }

    return PCRDR_SC_SERVICE_UNAVAILABLE;
}

static int on_rebuild_subtree(pcmcth_udom *udom, foil_rdrbox *rdrbox)
{
    foil_rect orc = rdrbox->ctnt_rect;

    rebuild_subtree(udom, rdrbox);

    foil_layout_ctxt layout_ctxt = { udom, udom->initial_cblock };
    rdrbox = relayout_rdrtree(&layout_ctxt, rdrbox, orc);

    foil_udom_invalidate_rdrbox(udom, rdrbox);
    return PCRDR_SC_OK;
}

static int on_displace_text_content(pcmcth_udom *udom, foil_rdrbox *rdrbox,
    pcdoc_element_t ref_elem)
{
    (void) ref_elem;
    return on_rebuild_subtree(udom, rdrbox);
}

static int on_erase_text_content(pcmcth_udom *udom, foil_rdrbox *rdrbox,
    pcdoc_element_t ref_elem)
{
    (void) ref_elem;
    return on_rebuild_subtree(udom, rdrbox);
}

static int on_clear_text_content(pcmcth_udom *udom, foil_rdrbox *rdrbox,
    pcdoc_element_t ref_elem)
{
    (void) ref_elem;
    return on_rebuild_subtree(udom, rdrbox);
}
static int on_append_text_content(pcmcth_udom *udom, foil_rdrbox *rdrbox,
    pcdoc_element_t ref_elem)
{
    (void) ref_elem;
    return on_rebuild_subtree(udom, rdrbox);
}
static int on_prepend_text_content(pcmcth_udom *udom, foil_rdrbox *rdrbox,
    pcdoc_element_t ref_elem)
{
    (void) ref_elem;
    return on_rebuild_subtree(udom, rdrbox);
}
static int on_insert_before_text_content(pcmcth_udom *udom, foil_rdrbox *rdrbox,
    pcdoc_element_t ref_elem)
{
    (void) ref_elem;
    rdrbox = get_rdrbox_container(udom, rdrbox);
    return on_rebuild_subtree(udom, rdrbox);
}
static int on_insert_after_text_content(pcmcth_udom *udom, foil_rdrbox *rdrbox,
    pcdoc_element_t ref_elem)
{
    (void) ref_elem;
    rdrbox = get_rdrbox_container(udom, rdrbox);
    return on_rebuild_subtree(udom, rdrbox);
}

static int on_update_text_content(pcmcth_udom *udom, foil_rdrbox *rdrbox,
    pcdoc_element_t ref_elem, int op)
{
    int r = PCRDR_SC_NOT_IMPLEMENTED;
    switch (op) {
    case PCRDR_K_OPERATION_DISPLACE:
    case PCRDR_K_OPERATION_UPDATE:
        r = on_displace_text_content(udom, rdrbox, ref_elem);
        break;

    case PCRDR_K_OPERATION_ERASE:
        r = on_erase_text_content(udom, rdrbox, ref_elem);
        break;

    case PCRDR_K_OPERATION_CLEAR:
        r = on_clear_text_content(udom, rdrbox, ref_elem);
        break;

    case PCRDR_K_OPERATION_APPEND:
        r = on_append_text_content(udom, rdrbox, ref_elem);
        break;

    case PCRDR_K_OPERATION_PREPEND:
        r = on_prepend_text_content(udom, rdrbox, ref_elem);
        break;

    case PCRDR_K_OPERATION_INSERTBEFORE:
        r = on_insert_before_text_content(udom, rdrbox, ref_elem);
        break;

    case PCRDR_K_OPERATION_INSERTAFTER:
        r = on_insert_after_text_content(udom, rdrbox, ref_elem);
        break;
    }
    return r;
}

static int on_displace_content(pcmcth_udom *udom, foil_rdrbox *rdrbox,
    pcdoc_element_t ref_elem)
{
    (void) ref_elem;
    return on_rebuild_subtree(udom, rdrbox);
}

static int on_erase_content(pcmcth_udom *udom, foil_rdrbox *rdrbox,
    pcdoc_element_t ref_elem)
{
    (void) ref_elem;
    return on_rebuild_subtree(udom, rdrbox);
}

static int on_clear_content(pcmcth_udom *udom, foil_rdrbox *rdrbox,
    pcdoc_element_t ref_elem)
{
    (void) ref_elem;
    return on_rebuild_subtree(udom, rdrbox);
}

static int on_append_content(pcmcth_udom *udom, foil_rdrbox *rdrbox,
    pcdoc_element_t ref_elem)
{
    (void) ref_elem;
    return on_rebuild_subtree(udom, rdrbox);
}

static int on_prepend_content(pcmcth_udom *udom, foil_rdrbox *rdrbox,
    pcdoc_element_t ref_elem)
{
    (void) ref_elem;
    return on_rebuild_subtree(udom, rdrbox);
}

static int on_insert_before_content(pcmcth_udom *udom, foil_rdrbox *rdrbox,
    pcdoc_element_t ref_elem)
{
    (void) ref_elem;
    rdrbox = get_rdrbox_container(udom, rdrbox);
    return on_rebuild_subtree(udom, rdrbox);
}

static int on_insert_after_content(pcmcth_udom *udom, foil_rdrbox *rdrbox,
    pcdoc_element_t ref_elem)
{
    (void) ref_elem;
    rdrbox = get_rdrbox_container(udom, rdrbox);
    return on_rebuild_subtree(udom, rdrbox);
}

static int on_update_content(pcmcth_udom *udom, foil_rdrbox *rdrbox,
    pcdoc_element_t ref_elem, int op)
{
    int r = PCRDR_SC_NOT_IMPLEMENTED;
    switch (op) {
    case PCRDR_K_OPERATION_DISPLACE:
    case PCRDR_K_OPERATION_UPDATE:
        r = on_displace_content(udom, rdrbox, ref_elem);
        break;

    case PCRDR_K_OPERATION_ERASE:
        r = on_erase_content(udom, rdrbox, ref_elem);
        break;

    case PCRDR_K_OPERATION_CLEAR:
        r = on_clear_content(udom, rdrbox, ref_elem);
        break;

    case PCRDR_K_OPERATION_APPEND:
        r = on_append_content(udom, rdrbox, ref_elem);
        break;

    case PCRDR_K_OPERATION_PREPEND:
        r = on_prepend_content(udom, rdrbox, ref_elem);
        break;

    case PCRDR_K_OPERATION_INSERTBEFORE:
        r = on_insert_before_content(udom, rdrbox, ref_elem);
        break;

    case PCRDR_K_OPERATION_INSERTAFTER:
        r = on_insert_after_content(udom, rdrbox, ref_elem);
        break;
    }
    return r;
}

/*
  - `append`: the last child element of the target element before this op.
  - `prepend`: the first child element of the target elment before this op.
  - `insertBefore`: the previous sibling of the target element before this op.
  - `insertAfter`: the next sibling of the target element before this op.
  - `displace`: the target element itself.
  - `update`: the target element itself.
  - `erase`: the target element itself.
  - `clear`: the target element itself.
*/
int foil_udom_update_rdrbox(pcmcth_udom *udom, foil_rdrbox *rdrbox,
        int op, const char *property, purc_variant_t ref_info)
{
    (void)op;
    int r = PCRDR_SC_NOT_IMPLEMENTED;

    pcdoc_element_t element;
    element = purc_variant_native_get_entity(ref_info);
    assert(element);

    if (strncasecmp(property, "attr.", 5) == 0) {
        const char *attr = property + 5;
        if (strcasecmp(attr, "style") == 0) {
            r = on_update_style(udom, rdrbox, element, op);
        }
        else if (strcasecmp(attr, "class") == 0) {
            r = on_update_style(udom, rdrbox, element, op);
        }
        else if (rdrbox->tailor_ops && rdrbox->tailor_ops->on_attr_changed) {
            foil_update_ctxt ctxt = { udom, element };
            rdrbox->tailor_ops->on_attr_changed(&ctxt, rdrbox);
            r = PCRDR_SC_OK;
        }
    }
    else if (strcasecmp(property, "textContent") == 0) {
        r = on_update_text_content(udom, rdrbox, element, op);
    }
    else if (strcasecmp(property, "content") == 0) {
        r = on_update_content(udom, rdrbox, element, op);
    }
    else {
        LOG_WARN("Unknown property: %s\n", property);
    }

    return r;
}

purc_variant_t foil_udom_call_method(pcmcth_udom *udom, foil_rdrbox *rdrbox,
        const char *method, purc_variant_t arg)
{
    LOG_DEBUG("rdrbox: %p, method: %s\n", rdrbox, method);

    if (rdrbox->tailor_ops && rdrbox->tailor_ops->call_method) {
        foil_update_ctxt ctxt = { udom, NULL };
        return rdrbox->tailor_ops->call_method(&ctxt, rdrbox, method, arg);
    }

    return PURC_VARIANT_INVALID;
}

purc_variant_t foil_udom_get_property(pcmcth_udom *udom, foil_rdrbox *rdrbox,
        const char *property)
{
    LOG_DEBUG("rdrbox: %p, property: %s\n", rdrbox, property);

    if (rdrbox->tailor_ops && rdrbox->tailor_ops->get_property) {
        foil_update_ctxt ctxt = { udom, NULL };
        return rdrbox->tailor_ops->get_property(&ctxt,
                rdrbox, property, PURC_VARIANT_INVALID);
    }

    return PURC_VARIANT_INVALID;
}

purc_variant_t foil_udom_set_property(pcmcth_udom *udom, foil_rdrbox *rdrbox,
        const char *property, purc_variant_t value)
{
    LOG_DEBUG("rdrbox: %p, property: %s\n", rdrbox, property);

    if (rdrbox->tailor_ops && rdrbox->tailor_ops->set_property) {
        foil_update_ctxt ctxt = { udom, NULL };
        return rdrbox->tailor_ops->set_property(&ctxt,
                rdrbox, property, value);
    }

    return PURC_VARIANT_INVALID;
}

