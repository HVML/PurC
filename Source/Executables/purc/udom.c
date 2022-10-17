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

#include "udom.h"
#include "page.h"
#include "rdrbox.h"
#include "util/sorted-array.h"
#include "util/list.h"

#include <assert.h>

typedef enum {
    PCTH_RDR_ALIGN_LEFT,
    PCTH_RDR_ALIGN_RIGHT,
    PCTH_RDR_ALIGN_CENTER,
    PCTH_RDR_ALIGN_JUSTIFY,
} pcth_rdr_align_t;

typedef enum {
    PCTH_RDR_DECORATION_NONE,
    PCTH_RDR_DECORATION_UNDERLINE,
    PCTH_RDR_DECORATION_OVERLINE,
    PCTH_RDR_DECORATION_LINE_THROUGH,
    PCTH_RDR_DECORATION_BLINK,
} pcth_rdr_decoration_t;

typedef enum {
    PCTH_RDR_WHITE_SPACE_NORMAL,
    PCTH_RDR_WHITE_SPACE_PRE,
    PCTH_RDR_WHITE_SPACE_NOWRAP,
    PCTH_RDR_WHITE_SPACE_PRE_WRAP,
    PCTH_RDR_WHITE_SPACE_PRE_LINE,
} pcth_rdr_white_space_t;

struct purcth_udom {
    /* the sorted array of eDOM element and the corresponding rendering box. */
    struct sorted_array *elem2rdrbox;

    /* author-defined style sheet */
    css_stylesheet *author_sheet;

    /* CSS selection context */
    css_select_ctx *select_ctx;

    /* the initial containing block */
    struct foil_rdrbox *initial_cblock;

    /* the CSS media */
    css_media media;

    /* size of whole page in pixels */
    unsigned width, height;

    /* size of page in rows and columns */
    unsigned cols, rows;
};

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
    "dir, hr, menu, pre   { display: block; unicode-bidi: embed }"
    "li              { display: list-item }"
    "head            { display: none }"
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
    "body            { margin: 1em 1ex }"
    "h1              { margin: 3em 0 1em 0 }"
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
    "input, select   { display: inline-block }"
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
    "/* Begin bidirectionality settings (do not change) */"
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

int foil_udom_module_init(void)
{
    css_stylesheet_params params;
    css_error err;

    memset(&params, 0, sizeof(params));
    params.params_version = CSS_STYLESHEET_PARAMS_VERSION_1;
    params.level = CSS_LEVEL_DEFAULT;
    params.charset = FOIL_DEF_CHARSET;
    params.resolve = resolve_url;
#if 0
    params.url = NULL;
    params.title = NULL;
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
    if (err != CSS_OK) {
        LOG_ERROR("Failed to append data to UA style sheet: %d\n", err);
        css_stylesheet_destroy(def_ua_sheet);
        return -1;
    }

    css_stylesheet_data_done(def_ua_sheet);

    return 0;
}

void foil_udom_module_cleanup(void)
{
    if (def_ua_sheet)
        css_stylesheet_destroy(def_ua_sheet);
}

static void udom_cleanup(purcth_udom *udom)
{
    if (udom->elem2rdrbox)
        sorted_array_cleanup(udom->elem2rdrbox);
    if (udom->author_sheet)
        css_stylesheet_destroy(udom->author_sheet);
    if (udom->select_ctx)
        css_select_ctx_destroy(udom->select_ctx);
    if (udom->initial_cblock)
        foil_rdrbox_delete_deep(udom->initial_cblock);
}

purcth_udom *foil_udom_new(purcth_page *page)
{
    purcth_udom* udom = calloc(1, sizeof(purcth_udom));
    if (udom == NULL) {
        goto failed;
    }

    udom->elem2rdrbox = sorted_array_create(SAFLAG_DEFAULT, 8, NULL, NULL);
    if (udom->elem2rdrbox == NULL) {
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

    /* create the initial containing block */
    int cols = foil_page_cols(page);
    int rows = foil_page_rows(page);
    int width = cols * FOIL_PX_PER_EX;
    int height = rows * FOIL_PX_PER_EM;

    udom->initial_cblock = foil_rdrbox_new_block();
    if (udom->initial_cblock == NULL) {
        goto failed;
    }

    udom->initial_cblock->rect.left = 0;
    udom->initial_cblock->rect.top = 0;
    udom->initial_cblock->rect.right = width;
    udom->initial_cblock->rect.bottom = height;

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

void foil_udom_delete(purcth_udom *udom)
{
    udom_cleanup(udom);
    free(udom);
}

foil_rdrbox *foil_udom_find_rdrbox(purcth_udom *udom,
        uint64_t element_handle)
{
    void *data;

    if (!sorted_array_find(udom->elem2rdrbox, element_handle, &data)) {
        return NULL;
    }

    return data;
}

static int append_style_walker(purc_document_t doc,
        pcdoc_element_t element, void *ctxt)
{
    const char *name;
    size_t len;

    pcdoc_element_get_tag_name(doc, element, &name, &len,
            NULL, NULL, NULL, NULL);
    if (strncasecmp(name, "style", len) == 0) {
        struct purcth_udom *udom = ctxt;

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
                    if (err != CSS_OK || err != CSS_NEEDDATA) {
                        LOG_ERROR("Failed to append css data: %d\n", err);
                        return -1;
                    }
                }
            }

            child = pcdoc_node_next_sibling(doc, child);
        }
    }

    return 0;
}

#define ATTR_NAME_STYLE     "style"

extern css_select_handler foil_css_select_handler;

static css_select_results *
select_element_style(const css_media *media, css_select_ctx *select_ctx,
        purc_document_t doc, pcdoc_element_t element)
{
    // prepare inline style
    css_error err;
    css_stylesheet* inline_sheet = NULL;
    const char* value;
    size_t len;

    if (pcdoc_element_get_attribute(doc, element, ATTR_NAME_STYLE,
            &value, &len) == 0 && value) {
        css_stylesheet_params params;

        memset(&params, 0, sizeof(params));
        params.params_version = CSS_STYLESHEET_PARAMS_VERSION_1;
        params.level = CSS_LEVEL_DEFAULT;
        params.charset = FOIL_DEF_CHARSET;
        params.inline_style = true;
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
            if (err == CSS_OK) {
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
            &foil_css_select_handler, NULL, &result);
    if (err != CSS_OK || result == NULL) {
        goto failed;
    }

    /* TODO: handle styles for pseudo elements */
    int pseudo_element;
    css_computed_style *composed = NULL;
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

static int udom_maker(purc_document_t doc,
        pcdoc_element_t element, void *ctxt)
{
    struct purcth_rendering_ctxt *my_ctxt = ctxt;

    css_select_results *result;
    result = select_element_style(&my_ctxt->udom->media,
            my_ctxt->udom->select_ctx, doc, element);
    if (result) {
        foil_create_rdrbox(my_ctxt, element, result);
        css_select_results_destroy(result);
    }

    return 0;
}

purcth_udom *
foil_udom_load_edom(purcth_page *page, purc_variant_t edom, int *retv)
{
    purc_document_t edom_doc;
    purc_document_type doc_type;

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

    purcth_udom *udom;
    udom = foil_udom_new(page);
    if (udom == NULL) {
        *retv = PCRDR_SC_INSUFFICIENT_STORAGE;
        goto failed;
    }

    size_t n;

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

        err = css_stylesheet_create(&params, &udom->author_sheet);
        if (err != CSS_OK) {
            *retv = PCRDR_SC_INSUFFICIENT_STORAGE;
            LOG_ERROR("Failed to create default user agent sheet: %d\n", err);
            goto failed;
        }

        pcdoc_travel_descendant_elements(edom_doc, head,
                append_style_walker, udom, &n);

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

    struct purcth_rendering_ctxt ctxt = { edom_doc, udom,
        udom->initial_cblock };
    pcdoc_travel_descendant_elements(edom_doc, purc_document_root(edom_doc),
            udom_maker, &ctxt, &n);
    return udom;

failed:
    foil_udom_delete(udom);
    return NULL;
}

int foil_udom_update_rdrbox(purcth_udom *udom, foil_rdrbox *rdrbox,
        int op, const char *property, purc_variant_t ref_info)
{
    (void)udom;
    (void)rdrbox;
    (void)op;
    (void)property;
    (void)ref_info;

    /* TODO */
    return PCRDR_SC_NOT_IMPLEMENTED;
}

purc_variant_t foil_udom_call_method(purcth_udom *udom, foil_rdrbox *rdrbox,
        const char *method, purc_variant_t arg)
{
    (void)udom;
    (void)rdrbox;
    (void)method;
    (void)arg;

    /* TODO */
    return PURC_VARIANT_INVALID;
}

purc_variant_t foil_udom_get_property(purcth_udom *udom, foil_rdrbox *rdrbox,
        const char *property)
{
    (void)udom;
    (void)rdrbox;
    (void)property;

    /* TODO */
    return PURC_VARIANT_INVALID;
}

purc_variant_t foil_udom_set_property(purcth_udom *udom, foil_rdrbox *rdrbox,
        const char *property, purc_variant_t value)
{
    (void)udom;
    (void)rdrbox;
    (void)property;
    (void)value;

    /* TODO */
    return PURC_VARIANT_INVALID;
}

