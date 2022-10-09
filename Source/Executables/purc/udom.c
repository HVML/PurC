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

#include "foil.h"
#include "udom.h"
#include "util/sorted-array.h"
#include "util/list.h"

#include <assert.h>
#include <csseng/csseng.h>

typedef struct foil_rect {
    int left, top;
    int right, bottom;
} foil_rect;

typedef enum {
    PCTH_RDR_BOX_TYPE_INLINE,
    PCTH_RDR_BOX_TYPE_BLOCK,
    PCTH_RDR_BOX_TYPE_INLINE_BLOCK,
    PCTH_RDR_BOX_TYPE_MARKER,
} pcth_rdr_box_t;

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

struct _text_segment {
    struct list_head ln;

    unsigned i; // the index of first character
    unsigned n; // number of characters in this segment

    /* position of this segment in the containing block box */
    int x, y;

    /* rows taken by this segment (always be 1). */
    unsigned height;
    /* columns taken by this segment. */
    unsigned width;
};

struct _inline_box_data {
    /* the code points of text in Unicode (should be in visual order) */
    uint32_t *ucs;

    int letter_spacing;
    int word_spacing;

    /* text color */
    int color;
    /* text decoration */
    pcth_rdr_decoration_t deco;

    /* the text segments */
    struct list_head segs;
};

struct _block_box_data {
    // margins
    int ml, mt, mr, mb;
    // paddings
    int pl, pt, pr, pb;

    int              text_indent;
    pcth_rdr_align_t text_align;

    /* the code points of text in Unicode (should be in visual order) */
    uint32_t *ucs;

    /* text color and attributes */
    int color;

    /* the text segments */
    struct list_head segs;
};

struct purcth_rdrbox {
    struct purcth_rdrbox* parent;
    struct purcth_rdrbox* first;
    struct purcth_rdrbox* last;

    struct purcth_rdrbox* prev;
    struct purcth_rdrbox* next;

    /* type of box */
    pcth_rdr_box_t type;

    /* number of child boxes */
    unsigned nr_children;

    /* the visual region (rectangles) of the box */
    unsigned nr_rects;
    struct foil_rect *rects;

    /* the extra data if the box type is INLINE */
    union {
        void *data;     // aliases
        struct _inline_box_data *inline_data;
        struct _block_box_data *block_data;
    };
};

struct purcth_udom {
    /* the sorted array of eDOM element and the corresponding rendering box. */
    struct sorted_array *elem2rdrbox;

    /* author-defined style sheet */
    css_stylesheet *author_sheet;

    /* CSS selection context */
    css_select_ctx *select_ctx;

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
    "body            { margin: 8px }"
    "h1              { font-size: 2em; margin: .67em 0 }"
    "h2              { font-size: 1.5em; margin: .75em 0 }"
    "h3              { font-size: 1.17em; margin: .83em 0 }"
    "h4, p,"
    "blockquote, ul,"
    "fieldset, form,"
    "ol, dl, dir,"
    "menu            { margin: 1.12em 0 }"
    "h5              { font-size: .83em; margin: 1.5em 0 }"
    "h6              { font-size: .75em; margin: 1.67em 0 }"
    "h1, h2, h3, h4,"
    "h5, h6, b,"
    "strong          { font-weight: bolder }"
    "blockquote      { margin-left: 40px; margin-right: 40px }"
    "i, cite, em,"
    "var, address    { font-style: italic }"
    "pre, tt, code,"
    "kbd, samp       { font-family: monospace }"
    "pre             { white-space: pre }"
    "button, textarea,"
    "input, select   { display: inline-block }"
    "big             { font-size: 1.17em }"
    "small, sub, sup { font-size: .83em }"
    "sub             { vertical-align: sub }"
    "sup             { vertical-align: super }"
    "table           { border-spacing: 2px; }"
    "thead, tbody,"
    "tfoot           { vertical-align: middle }"
    "td, th, tr      { vertical-align: inherit }"
    "s, strike, del  { text-decoration: line-through }"
    "hr              { border: 1px inset }"
    "ol, ul, dir,"
    "menu, dd        { margin-left: 40px }"
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

int foil_udom_module_init(void)
{
    css_stylesheet_params params;
    css_error err;

    params.params_version = CSS_STYLESHEET_PARAMS_VERSION_1;
    params.level = CSS_LEVEL_DEFAULT;
    params.charset = FOIL_DEF_CHARSET;
    params.url = NULL;
    params.title = NULL;
    params.allow_quirks = false;
    params.inline_style = false;
    params.resolve = NULL;
    params.resolve_pw = NULL;
    params.import = NULL;
    params.import_pw = NULL;
    params.color = NULL;
    params.color_pw = NULL;
    params.font = NULL;
    params.font_pw = NULL;

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

    /* TODO */
    (void)page;

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

purcth_rdrbox *foil_udom_find_rdrbox(purcth_udom *udom,
        uint64_t element_handle)
{
    void *data;

    if (!sorted_array_find(udom->elem2rdrbox, element_handle, &data)) {
        return NULL;
    }

    return data;
}

static pchtml_action_t
append_style_walker(pcdom_node_t *node, void *ctxt)
{
    switch (node->type) {
    case PCDOM_NODE_TYPE_DOCUMENT_TYPE:
    case PCDOM_NODE_TYPE_TEXT:
    case PCDOM_NODE_TYPE_COMMENT:
    case PCDOM_NODE_TYPE_CDATA_SECTION:
        return PCHTML_ACTION_NEXT;

    case PCDOM_NODE_TYPE_ELEMENT: {
        const char *name;
        size_t len;

        pcdom_element_t *element;
        element = pcdom_interface_element(node);
        name = (const char *)pcdom_element_local_name(element, &len);
        if (strncasecmp(name, "style", len) == 0) {
            struct purcth_udom *udom = ctxt;

            pcdom_node_t *child = node->first_child;
            while (child) {
                if (child->type == PCDOM_NODE_TYPE_TEXT) {
                    pcdom_text_t *text;

                    text = pcdom_interface_text(child);

                    css_error err;
                    err = css_stylesheet_append_data(udom->author_sheet,
                            text->char_data.data.data,
                            text->char_data.data.length);
                    if (err != CSS_OK || err != CSS_NEEDDATA) {
                        LOG_ERROR("Failed to append css data: %d\n", err);
                        break;
                    }
                }

                child = child->next;
            }
        }

        /* walk to the siblings. */
        return PCHTML_ACTION_NEXT;
    }

    default:
        /* ignore any unknown node types */
        break;

    }

    return PCHTML_ACTION_OK;
}

struct rendering_ctxt {
    purcth_udom *udom;
};

static pchtml_action_t
rendering_walker(pcdom_node_t *node, void *ctxt)
{
    (void)ctxt;

    switch (node->type) {
    case PCDOM_NODE_TYPE_DOCUMENT_TYPE:
        return PCHTML_ACTION_NEXT;

    case PCDOM_NODE_TYPE_TEXT:
    case PCDOM_NODE_TYPE_COMMENT:
    case PCDOM_NODE_TYPE_CDATA_SECTION:
        return PCHTML_ACTION_NEXT;

    case PCDOM_NODE_TYPE_ELEMENT: {
        // struct rendering_ctxt *my_ctxt = ctxt;

        /* walk to the siblings. */
        return PCHTML_ACTION_NEXT;
    }

    default:
        /* ignore any unknown node types */
        break;
    }

    return PCHTML_ACTION_NEXT;
}

purcth_udom *
foil_udom_load_edom(purcth_page *page, purc_variant_t edom, int *retv)
{
    pcdom_document_t *edom_doc;

    edom_doc = purc_variant_native_get_entity(edom);
    assert(edom_doc);

    size_t len;
    const char *doctype = (const char *)pcdom_document_type_name(
            edom_doc->doctype, &len);

    if (len == 0 || strcasecmp(doctype, "html")) {
        *retv = PCRDR_SC_NOT_ACCEPTABLE;
        goto failed;
    }

    purcth_udom *udom;
    udom = foil_udom_new(page);
    if (udom == NULL) {
        *retv = PCRDR_SC_INSUFFICIENT_STORAGE;
        goto failed;
    }

    // parse and append style sheets
    pcdom_element_t *head;
    head = pchtml_doc_get_head(pchtml_interface_document(edom_doc));
    if (head) {
        css_error err;

        css_stylesheet_params params;
        params.params_version = CSS_STYLESHEET_PARAMS_VERSION_1;
        params.level = CSS_LEVEL_DEFAULT;
        params.charset = FOIL_DEF_CHARSET;
        params.url = NULL;
        params.title = NULL;
        params.allow_quirks = false;
        params.inline_style = false;
        params.resolve = NULL;
        params.resolve_pw = NULL;
        params.import = NULL;
        params.import_pw = NULL;
        params.color = NULL;
        params.color_pw = NULL;
        params.font = NULL;
        params.font_pw = NULL;

        err = css_stylesheet_create(&params, &udom->author_sheet);
        if (err != CSS_OK) {
            *retv = PCRDR_SC_INSUFFICIENT_STORAGE;
            LOG_ERROR("Failed to create default user agent sheet: %d\n", err);
            goto failed;
        }

        pcdom_node_simple_walk(pcdom_interface_node(head),
                append_style_walker, udom);

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

    pcdom_element_t *root = edom_doc->element;
    struct rendering_ctxt ctxt = { udom, };
    pcdom_node_simple_walk(pcdom_interface_node(root), rendering_walker, &ctxt);
    return udom;

failed:
    foil_udom_delete(udom);
    return NULL;
}

int foil_udom_update_rdrbox(purcth_udom *udom, purcth_rdrbox *rdrbox,
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

purc_variant_t foil_udom_call_method(purcth_udom *udom, purcth_rdrbox *rdrbox,
        const char *method, purc_variant_t arg)
{
    (void)udom;
    (void)rdrbox;
    (void)method;
    (void)arg;

    /* TODO */
    return PURC_VARIANT_INVALID;
}

purc_variant_t foil_udom_get_property(purcth_udom *udom, purcth_rdrbox *rdrbox,
        const char *property)
{
    (void)udom;
    (void)rdrbox;
    (void)property;

    /* TODO */
    return PURC_VARIANT_INVALID;
}

purc_variant_t foil_udom_set_property(purcth_udom *udom, purcth_rdrbox *rdrbox,
        const char *property, purc_variant_t value)
{
    (void)udom;
    (void)rdrbox;
    (void)property;
    (void)value;

    /* TODO */
    return PURC_VARIANT_INVALID;
}

