/*
** @file rdrbox.c
** @author Vincent Wei
** @date 2022/10/10
** @brief The implementation of renderring box.
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

#include "rdrbox.h"
#include "rdrbox-internal.h"
#include "udom.h"

#include <stdio.h>
#include <assert.h>

int foil_rdrbox_module_init(pcmcth_renderer *rdr)
{
    (void)rdr;
    return 0;
}

void foil_rdrbox_module_cleanup(pcmcth_renderer *rdr)
{
    (void)rdr;
}

foil_rdrbox *foil_rdrbox_new(uint8_t type)
{
    foil_rdrbox *box = calloc(1, sizeof(*box));
    if (box == NULL)
        goto failed;

    box->type = type;
    switch (type) {
    case FOIL_RDRBOX_TYPE_BLOCK:
        box->block_data = calloc(1, sizeof(*box->block_data));
        if (box->block_data == NULL) {
            goto failed;
        }
        break;

    case FOIL_RDRBOX_TYPE_INLINE:
        box->inline_data = calloc(1, sizeof(*box->inline_data));
        if (box->inline_data == NULL) {
            goto failed;
        }
        INIT_LIST_HEAD(&box->inline_data->paras);
        break;

    case FOIL_RDRBOX_TYPE_LIST_ITEM:
        box->list_item_data = calloc(1, sizeof(*box->list_item_data));
        if (box->list_item_data == NULL) {
            goto failed;
        }
        break;

    case FOIL_RDRBOX_TYPE_MARKER:
        box->marker_data = calloc(1, sizeof(*box->marker_data));
        if (box->marker_data == NULL) {
            goto failed;
        }
        break;

    case FOIL_RDRBOX_TYPE_INLINE_BLOCK:
        box->inline_block_data = calloc(1, sizeof(*box->inline_block_data));
        if (box->inline_block_data == NULL) {
            goto failed;
        }
        break;

    default:
        // TODO:
        LOG_WARN("Not supported box type: %d\n", type);
        goto failed;
    }

    return box;

failed:
    if (box)
        free(box);
    return NULL;
}

foil_quotes *foil_quotes_new(unsigned nr_strings, const char **strings)
{
    foil_quotes *quotes;

    quotes = calloc(1, sizeof(*quotes));
    if (G_LIKELY(quotes)) {
        quotes->strings = calloc(nr_strings, sizeof(lwc_string *));

        if (quotes->strings == NULL) {
            LOG_ERROR("Failed to allocate space for quote strings\n");
            goto failed;
        }

        quotes->nr_strings = 0;
        for (unsigned i = 0; i < nr_strings; i++) {
            if (lwc_intern_string(strings[i], strlen(strings[i]),
                        &quotes->strings[i])) {
                LOG_ERROR("Failed to intern quote string\n");
                goto failed;
            }

            quotes->nr_strings++;
            LOG_DEBUG("Interned quote string: %p\n", quotes->strings[i]);
        }

        quotes->refc = 1;
    }

    return quotes;

failed:
    foil_quotes_delete(quotes);
    return NULL;
}

foil_quotes *foil_quotes_new_lwc(unsigned nr_strings, lwc_string **strings)
{
    foil_quotes *quotes;

    quotes = calloc(1, sizeof(*quotes));
    if (G_LIKELY(quotes)) {
        quotes->strings = calloc(nr_strings, sizeof(lwc_string *));

        if (quotes->strings == NULL) {
            LOG_ERROR("Failed to allocate space for quote strings\n");
            goto failed;
        }

        quotes->nr_strings = 0;
        for (unsigned i = 0; i < nr_strings && strings[i] != NULL; i++) {
            quotes->strings[i] = lwc_string_ref(strings[i]);
            quotes->nr_strings++;
        }

        quotes->refc = 1;
    }

    return quotes;

failed:
    foil_quotes_delete(quotes);
    return NULL;
}

static const char *quotes_en[] = {
    "\"",
    "\"",
    "'",
    "'",
};

static const char *quotes_zh[] = {
    "“",
    "”",
    "‘",
    "’",
};

/* must be sorted against langcode */
static struct lang_quotes {
    uint8_t         code;
    uint8_t         nr_strings;
    const char    **strings;
    foil_quotes    *quotes;
} lang_quotes [] = {
    { FOIL_LANGCODE_en, (uint8_t)PCA_TABLESIZE(quotes_en), quotes_en, NULL },
    { FOIL_LANGCODE_zh, (uint8_t)PCA_TABLESIZE(quotes_zh), quotes_zh, NULL },
};

/* get the initial qutoes for specific language code */
foil_quotes *foil_quotes_get_initial(uint8_t lang_code)
{
    static ssize_t max = PCA_TABLESIZE(lang_quotes) - 1;

    struct lang_quotes *found = NULL;
    ssize_t low = 0, high = max, mid;
    while (low <= high) {
        mid = (low + high) / 2;
        found = lang_quotes + mid;
        if (lang_code == found->code) {
            goto found;
        }
        else {
            if (lang_code < found->code) {
                high = mid - 1;
            }
            else {
                low = mid + 1;
            }
        }
    }

    /* use `en` as the default */
    return foil_quotes_get_initial(FOIL_LANGCODE_en);

found:
    if (found->quotes)
        return foil_quotes_ref(found->quotes);

    found->quotes = foil_quotes_new(found->nr_strings, found->strings);
    return found->quotes;
}

void foil_quotes_delete(foil_quotes *quotes)
{
    if (quotes->strings) {
        for (unsigned i = 0; i < quotes->nr_strings; i++) {
            LOG_DEBUG("unref interned quote string: %p\n", quotes->strings[i]);
            lwc_string_unref(quotes->strings[i]);
        }

        free(quotes->strings);
    }

    free(quotes);
}

foil_counters *foil_counters_new(const css_computed_counter *css_counters)
{
    foil_counters *counters;

    size_t n = 0;
    while (css_counters[n].name) {
        n++;
    }

    if (n == 0)
        return NULL;

    counters = calloc(1, sizeof(*counters));
    if (G_LIKELY(counters)) {
        counters->counters = calloc(n, sizeof(foil_named_counter));
        if (counters->counters == NULL) {
            LOG_ERROR("Failed to allocate space for named counters\n");
            goto failed;
        }

        counters->nr_counters = 0;
        for (unsigned i = 0; i < n; i++) {
            counters->counters[i].name = lwc_string_ref(css_counters[i].name);
            counters->counters[i].value = FIXTOINT(css_counters[i].value);

            counters->nr_counters++;
        }

        counters->refc = 1;
    }

    return counters;

failed:
    foil_counters_delete(counters);
    return NULL;
}

void foil_counters_delete(foil_counters *counters)
{
    if (counters->counters) {
        for (unsigned i = 0; i < counters->nr_counters; i++) {
            LOG_DEBUG("unref interned counter name string: %p\n",
                    counters->counters[i].name);
            lwc_string_unref(counters->counters[i].name);
        }

        free(counters->counters);
    }

    free(counters);
}

void foil_rdrbox_delete(foil_rdrbox *box)
{
    foil_rdrbox_remove_from_tree(box);

    if (box->quotes) {
        foil_quotes_unref(box->quotes);
    }

    if (box->counter_reset) {
        foil_counters_unref(box->counter_reset);
    }

    if (box->counter_incrm) {
        foil_counters_unref(box->counter_incrm);
    }

    if (box->counters_table)
        g_hash_table_destroy(box->counters_table);

    if (box->data) {
        if (box->cb_data_cleanup) {
            box->cb_data_cleanup(box->data);
        }

        free(box->data);
    }

    free(box);
}

void foil_rdrbox_append_child(foil_rdrbox *to, foil_rdrbox *box)
{
    if (to->last != NULL) {
        to->last->next = box;
    }
    else {
        to->first = box;
    }

    box->parent = to;
    box->next = NULL;
    box->prev = to->last;

    to->last = box;
}

void foil_rdrbox_prepend_child(foil_rdrbox *to, foil_rdrbox *box)
{
    if (to->first != NULL) {
        to->first->prev = box;
    }
    else {
        to->last = box;
    }

    box->parent = to;
    box->next = to->first;
    box->prev = NULL;

    to->first = box;
}

void foil_rdrbox_insert_before(foil_rdrbox *to, foil_rdrbox *box)
{
    if (to->prev != NULL) {
        to->prev->next = box;
    }
    else {
        if (to->parent != NULL) {
            to->parent->first = box;
        }
    }

    box->parent = to->parent;
    box->next = to;
    box->prev = to->prev;

    to->prev = box;
}

void foil_rdrbox_insert_after(foil_rdrbox *to, foil_rdrbox *box)
{
    if (to->next != NULL) {
        to->next->prev = box;
    }
    else {
        if (to->parent != NULL) {
            to->parent->last = box;
        }
    }

    box->parent = to->parent;
    box->next = to->next;
    box->prev = to;
    to->next = box;
}

void foil_rdrbox_remove_from_tree(foil_rdrbox *box)
{
    if (box->parent != NULL) {
        if (box->parent->first == box) {
            box->parent->first = box->next;
        }

        if (box->parent->last == box) {
            box->parent->last = box->prev;
        }
    }

    if (box->next != NULL) {
        box->next->prev = box->prev;
    }

    if (box->prev != NULL) {
        box->prev->next = box->next;
    }

    box->parent = NULL;
    box->next = NULL;
    box->prev = NULL;
}

void foil_rdrbox_delete_deep(foil_rdrbox *root)
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
                foil_rdrbox_delete(box);
                break;
            }

            tmp = box->next;
            foil_rdrbox_delete(box);
            box = tmp;
        }
    }
}

static const char *literal_values_boxtype[] = {
    "inline",
    "block",
    "list-item",
    "marker",
    "inline-block",
    "table",
    "inline-table",
    "table-row_group",
    "table-header-group",
    "table-footer-group",
    "table-row",
    "table-column-group",
    "table-column",
    "table-cell",
    "table-caption",
};

static const char *literal_values_position[] = {
    "static",
    "relative",
    "absolute",
    "fixed",
    "sticky",
};

static const char *literal_values_float[] = {
    "none",
    "left",
    "right",
};

#ifndef NDEBUG
static const char *literal_values_direction[] = {
    "ltr",
    "rtl",
};

static const char *literal_values_visibility[] = {
    "visible",
    "hidden",
    "collapse",
};

static const char *literal_values_overflow[] = {
    "visible",
    "hidden",
    "scroll",
    "auto",
};

static const char *literal_values_unicode_bidi[] = {
    "normal",
    "embed",
    "isolate",
    "bidi_override",
    "isolate_override",
    "plaintext",
};

static const char *literal_values_text_transform[] = {
    "none",
    "capitalize",
    "uppercase",
    "lowercase",
};

static const char *literal_values_white_space[] = {
    "normal",
    "pre",
    "nowrap",
    "pre-wrap",
    "pre-line",
};

static const char *literal_values_text_align[] = {
    "left",
    "right",
    "center",
    "justify",
};

static const char *literal_values_text_overflow[] = {
    "clip",
    "ellipsis",
};

static const char *literal_values_word_break[] = {
    "normal",
    "keep-all",
    "break-all",
    "break-word",
};

static const char *literal_values_line_break[] = {
    "auto",
    "loose",
    "normal",
    "strict",
    "anywhere",
};

static const char *literal_values_word_wrap[] = {
    "normal",
    "break-word",
    "anywhere",
};

static const char *literal_values_list_style_type[] = {
    "disc",
    "circle",
    "square",
    "decimal",
    "decimal-leading-zero",
    "lower-roman",
    "upper-roman",
    "lower-greek",
    "lower-latin",
    "upper-latin",
    "armenian",
    "georgian",
    "none",
};

static const char *literal_values_list_style_position[] = {
    "outside",
    "inside",
};

#endif /* not defined NDEBUG */

#define INVALID_USED_VALUE_UINT8     0xFF

static uint8_t
display_to_type(foil_create_ctxt *ctxt, uint8_t computed)
{
    (void)ctxt;
    assert(ctxt->parent_box);

    assert(computed != CSS_DISPLAY_INHERIT);

    static const struct uint8_values_map {
        uint8_t from;
        uint8_t to;
    } display_value_map[] = {
        { CSS_DISPLAY_INLINE, FOIL_RDRBOX_TYPE_INLINE },
        { CSS_DISPLAY_BLOCK, FOIL_RDRBOX_TYPE_BLOCK },
        { CSS_DISPLAY_LIST_ITEM, FOIL_RDRBOX_TYPE_LIST_ITEM },
        { CSS_DISPLAY_RUN_IN, FOIL_RDRBOX_TYPE_INLINE_BLOCK },
        { CSS_DISPLAY_INLINE_BLOCK, FOIL_RDRBOX_TYPE_INLINE_BLOCK },
        { CSS_DISPLAY_TABLE, FOIL_RDRBOX_TYPE_TABLE },
        { CSS_DISPLAY_INLINE_TABLE, FOIL_RDRBOX_TYPE_INLINE_TABLE },
        { CSS_DISPLAY_TABLE_ROW_GROUP,  FOIL_RDRBOX_TYPE_TABLE_ROW_GROUP },
        { CSS_DISPLAY_TABLE_HEADER_GROUP, FOIL_RDRBOX_TYPE_TABLE_HEADER_GROUP },
        { CSS_DISPLAY_TABLE_FOOTER_GROUP, FOIL_RDRBOX_TYPE_TABLE_FOOTER_GROUP },
        { CSS_DISPLAY_TABLE_ROW, FOIL_RDRBOX_TYPE_TABLE_ROW },
        { CSS_DISPLAY_TABLE_COLUMN_GROUP, FOIL_RDRBOX_TYPE_TABLE_COLUMN_GROUP },
        { CSS_DISPLAY_TABLE_COLUMN, FOIL_RDRBOX_TYPE_TABLE_COLUMN },
        { CSS_DISPLAY_TABLE_CELL, FOIL_RDRBOX_TYPE_TABLE_CELL },
        { CSS_DISPLAY_TABLE_CAPTION, FOIL_RDRBOX_TYPE_TABLE_CAPTION },
        { CSS_DISPLAY_NONE, INVALID_USED_VALUE_UINT8 },

        // TODO
        { CSS_DISPLAY_FLEX, FOIL_RDRBOX_TYPE_BLOCK },
        { CSS_DISPLAY_INLINE_FLEX, FOIL_RDRBOX_TYPE_INLINE_BLOCK },
        { CSS_DISPLAY_GRID, FOIL_RDRBOX_TYPE_BLOCK },
        { CSS_DISPLAY_INLINE_GRID, FOIL_RDRBOX_TYPE_INLINE_BLOCK },
    };

    int lower = 0;
    int upper = PCA_TABLESIZE(display_value_map) - 1;

    while (lower <= upper) {
        int mid = (lower + upper) >> 1;

        if (computed < display_value_map[mid].from)
            upper = mid - 1;
        else if (computed > display_value_map[mid].from)
            lower = mid + 1;
        else
            return display_value_map[mid].to;
    }

    return FOIL_RDRBOX_TYPE_INLINE;
}

static bool is_table_box(foil_rdrbox *box)
{
    return box->type >= FOIL_RDRBOX_TYPE_TABLE &&
        box->type <= FOIL_RDRBOX_TYPE_TABLE_CAPTION;
}

static int round_width(float w)
{
    if (w > 0)
        return (int)(w / FOIL_PX_GRID_CELL_W + 0.5) * FOIL_PX_GRID_CELL_W;

    return (int)(w / FOIL_PX_GRID_CELL_W - 0.5) * FOIL_PX_GRID_CELL_W;
}

static int round_height(float h)
{
    if (h > 0)
        return (int)(h / FOIL_PX_GRID_CELL_H + 0.5) * FOIL_PX_GRID_CELL_H;
    return (int)(h / FOIL_PX_GRID_CELL_H - 0.5) * FOIL_PX_GRID_CELL_H;
}

static int calc_used_value_widths(foil_rdrbox *box,
        css_unit unit, css_fixed length)
{
    int v = 0;

    switch (unit) {
    case CSS_UNIT_PCT:
        v = foil_rect_width(&box->cblock_rect);
        v = round_width(v * FIXTOFLT(length));
        break;

    case CSS_UNIT_PX:
        v = round_width(FIXTOFLT(length));
        break;

    case CSS_UNIT_EX:
        v = round_width(FIXTOFLT(length) * FOIL_PX_GRID_CELL_W);
        break;

    case CSS_UNIT_EM:
        v = round_width(FIXTOFLT(length) * FOIL_PX_GRID_CELL_H);
        break;

    default:
        // TODO: support more unit
        LOG_WARN("TODO: support unit: %d\n", unit);
        break;
    }

    return v;
}

static int calc_used_value_heights(foil_rdrbox *box,
        css_unit unit, css_fixed length)
{
    int v = 0;

    switch (unit) {
    case CSS_UNIT_PCT:
        v = foil_rect_height(&box->cblock_rect);
        v = round_height(v * FIXTOFLT(length));
        break;

    case CSS_UNIT_PX:
        v = round_height(FIXTOFLT(length));
        break;

    case CSS_UNIT_EX:
        v = round_height(FIXTOFLT(length) * FOIL_PX_GRID_CELL_W);
        break;

    case CSS_UNIT_EM:
        v = round_height(FIXTOFLT(length) * FOIL_PX_GRID_CELL_H);
        break;

    default:
        // TODO: support more unit
        LOG_WARN("TODO: support unit: %d\n", unit);
        break;
    }

    return v;
}

static guint cb_lwc_string_hash(gconstpointer v)
{
    return lwc_string_hash_value((lwc_string *)v);
#if 0
    const char *str;
    size_t len;

    str = lwc_string_data((lwc_string *)v);
    len = lwc_string_length((lwc_string *)v);

    char buff[len + 1];
    strncpy(buff, str, len);
    buff[len] = 0;

    return g_str_hash(buff);
#endif
}

static gboolean
cb_lwc_string_equal(gconstpointer a, gconstpointer b)
{
    bool is_equal;
    lwc_error error;
    error = lwc_string_isequal((lwc_string *)a, (lwc_string *)b, &is_equal);
    (void)error;
    return is_equal;
}

static void
cb_lwc_string_key_destroy(gpointer data)
{
    lwc_string_unref((lwc_string *)data);
}

#if 0
static void
cb_clone_counter(gpointer key, gpointer value, gpointer user_data)
{
    GHashTable *dst_table = (GHashTable *)user_data;
    lwc_string_ref((lwc_string *)key);

    g_hash_table_insert(dst_table, key, value);
}
#endif

static GHashTable *create_counters_table(foil_create_ctxt *ctxt)
{
    (void)ctxt;
    return g_hash_table_new_full(cb_lwc_string_hash,
            cb_lwc_string_equal, cb_lwc_string_key_destroy, NULL);

#if 0
    GHashTable *dst_table;
    if (G_UNLIKELY(dst_table == NULL)) {
        return NULL;
    }

    if (ctxt->scoped_counters_table) {
        g_hash_table_foreach(ctxt->scoped_counters_table,
                cb_clone_counter, dst_table);
    }
#endif
}

static GHashTable *
find_counters_table_in_prev_siblings(foil_create_ctxt *ctxt,
        foil_rdrbox *box, const lwc_string *name)
{
    (void)ctxt;
    foil_rdrbox *prev = box->prev;

    while (prev) {
        if (prev->counters_table) {
            if (g_hash_table_lookup_extended(prev->counters_table,
                        name, NULL, NULL)) {
                return prev->counters_table;
            }
        }

        prev = prev->prev;
    }

    return NULL;
}

static GHashTable *find_counters_table(foil_create_ctxt *ctxt,
        foil_rdrbox *box, const lwc_string *name, intptr_t *value)
{
    foil_rdrbox *parent = box->parent;

    while (box) {
        if (box->counters_table) {
            if (g_hash_table_lookup_extended(box->counters_table,
                        name, NULL, (gpointer *)value)) {
                return box->counters_table;
            }
        }

        box = box->prev;
    }

    if (parent) {
        return find_counters_table(ctxt, parent, name, value);
    }

    return NULL;
}

static uint8_t normalize_list_style_type(uint8_t v)
{
    switch (v) {
    default:
    case CSS_LIST_STYLE_TYPE_DISC:
        v = FOIL_RDRBOX_LIST_STYLE_TYPE_DISC;
        break;

    case CSS_LIST_STYLE_TYPE_CIRCLE:
        v = FOIL_RDRBOX_LIST_STYLE_TYPE_CIRCLE;
        break;

    case CSS_LIST_STYLE_TYPE_SQUARE:
        v = FOIL_RDRBOX_LIST_STYLE_TYPE_SQUARE;
        break;

    case CSS_LIST_STYLE_TYPE_DECIMAL:
        v = FOIL_RDRBOX_LIST_STYLE_TYPE_DECIMAL;
        break;

    case CSS_LIST_STYLE_TYPE_DECIMAL_LEADING_ZERO:
        v = FOIL_RDRBOX_LIST_STYLE_TYPE_DECIMAL_LEADING_ZERO;
        break;

    case CSS_LIST_STYLE_TYPE_LOWER_ROMAN:
        v = FOIL_RDRBOX_LIST_STYLE_TYPE_LOWER_ROMAN;
        break;

    case CSS_LIST_STYLE_TYPE_UPPER_ROMAN:
        v = FOIL_RDRBOX_LIST_STYLE_TYPE_UPPER_ROMAN;
        break;

    case CSS_LIST_STYLE_TYPE_LOWER_GREEK:
        v = FOIL_RDRBOX_LIST_STYLE_TYPE_LOWER_GREEK;
        break;

    case CSS_LIST_STYLE_TYPE_LOWER_ALPHA:
    case CSS_LIST_STYLE_TYPE_LOWER_LATIN:
        v = FOIL_RDRBOX_LIST_STYLE_TYPE_LOWER_LATIN;
        break;

    case CSS_LIST_STYLE_TYPE_UPPER_ALPHA:
    case CSS_LIST_STYLE_TYPE_UPPER_LATIN:
        v = FOIL_RDRBOX_LIST_STYLE_TYPE_UPPER_LATIN;
        break;

    case CSS_LIST_STYLE_TYPE_ARMENIAN:
    case CSS_LIST_STYLE_TYPE_UPPER_ARMENIAN:
        v = FOIL_RDRBOX_LIST_STYLE_TYPE_UPPER_ARMENIAN;
        break;

    case CSS_LIST_STYLE_TYPE_LOWER_ARMENIAN:
        v = FOIL_RDRBOX_LIST_STYLE_TYPE_LOWER_ARMENIAN;
        break;

    case CSS_LIST_STYLE_TYPE_GEORGIAN:
        v = FOIL_RDRBOX_LIST_STYLE_TYPE_GEORGIAN;
        break;

    case CSS_LIST_STYLE_TYPE_CJK_DECIMAL:
        v = FOIL_RDRBOX_LIST_STYLE_TYPE_CJK_DECIMAL;
        break;

    case CSS_LIST_STYLE_TYPE_TIBETAN:
        v = FOIL_RDRBOX_LIST_STYLE_TYPE_TIBETAN;
        break;

    case CSS_LIST_STYLE_TYPE_NONE:
        v = FOIL_RDRBOX_LIST_STYLE_TYPE_NONE;
        break;
    }

    return v;
}

typedef void (*cb_matched_counter)(void *ctxt, foil_rdrbox *box,
        lwc_string *name, gpointer value);

static void
travel_box_up_for_counter(foil_rdrbox *box, lwc_string *name,
        cb_matched_counter func, void *ctxt)
{
    foil_rdrbox *parent = box->parent;

    while (box) {
        if (box->counters_table) {
            gpointer value;
            if (g_hash_table_lookup_extended(box->counters_table,
                        name, NULL, &value)) {
                func(ctxt, box, name, value);
            }
        }

        box = box->prev;
    }

    if (parent) {
        return travel_box_up_for_counter(parent, name, func, ctxt);
    }
}

struct counters_ctxt {
    const css_computed_content_item *ctnt_item;
    GString *text;

    uint8_t type;
    const char *sep_str;
    size_t sep_len;
};

static void on_matched_counter(void *ctxt, foil_rdrbox *box,
        lwc_string *name, gpointer value)
{
    struct counters_ctxt *my_ctxt = ctxt;
    (void)box;
    (void)name;

    intptr_t counter = (intptr_t)value;
    char *number = foil_rdrbox_list_number(0, (int)counter,
            my_ctxt->type, NULL);
    if (number) {
        g_string_prepend(my_ctxt->text, number);
        free(number);
    }

    if (my_ctxt->sep_str && my_ctxt->sep_len > 0) {
        g_string_prepend_len(my_ctxt->text, my_ctxt->sep_str, my_ctxt->sep_len);
    }
}

static void
generate_content_from_counters(foil_create_ctxt *ctxt, foil_rdrbox *box,
        const css_computed_content_item *ctnt_item, GString *text)
{
    (void)ctxt;

    struct counters_ctxt my_ctxt;
    my_ctxt.ctnt_item = ctnt_item;
    my_ctxt.text = text;

    // must be called for a pseudo element
    assert(box->principal);

    my_ctxt.type = ctnt_item->data.counters.style;
    if (my_ctxt.type == 0)
        my_ctxt.type = CSS_LIST_STYLE_TYPE_DECIMAL;
    my_ctxt.type = normalize_list_style_type(my_ctxt.type);

    if (ctnt_item->data.counters.sep) {
        my_ctxt.sep_str = lwc_string_data(ctnt_item->data.counters.sep);
        my_ctxt.sep_len = lwc_string_length(ctnt_item->data.counters.sep);
    }
    else {
        my_ctxt.sep_str = NULL;
        my_ctxt.sep_len = 0;
    }

    travel_box_up_for_counter(box, ctnt_item->data.counters.name,
            on_matched_counter, &my_ctxt);

    if (my_ctxt.sep_len > 0 && text->len > my_ctxt.sep_len) {
        g_string_erase(text, 0, my_ctxt.sep_len);
    }
}

/* display, positionn, and float must be determined
   before calling this function */
static void dtrm_common_properties(foil_create_ctxt *ctxt,
        foil_rdrbox *box)
{
    uint8_t v;

    LOG_DEBUG("Common style properties of element (%s):\n", ctxt->tag_name);

    /* determine direction */
    v = css_computed_direction(ctxt->style);
    assert(v != CSS_DIRECTION_INHERIT);
    if (v == CSS_DIRECTION_RTL) {
        box->direction = FOIL_RDRBOX_DIRECTION_RTL;
    }
    else {
        box->direction = FOIL_RDRBOX_DIRECTION_LTR;
    }

    LOG_DEBUG("\tdirection: %s\n", literal_values_direction[box->direction]);

    /* determine visibility */
    v = css_computed_visibility(
            ctxt->style);
    assert(v != CSS_VISIBILITY_INHERIT);
    switch (v) {
    case CSS_VISIBILITY_HIDDEN:
        box->visibility = FOIL_RDRBOX_VISIBILITY_HIDDEN;
        break;
    case CSS_VISIBILITY_COLLAPSE:
        box->visibility = FOIL_RDRBOX_VISIBILITY_COLLAPSE;
        break;
    case CSS_VISIBILITY_VISIBLE:
    default:
        box->visibility = FOIL_RDRBOX_VISIBILITY_VISIBLE;
        break;
    }

    LOG_DEBUG("\tvisibility: %s\n",
            literal_values_visibility[box->visibility]);

    /* determine overflow_x */
    v = css_computed_overflow_x(
            ctxt->style);
    assert(v != CSS_OVERFLOW_INHERIT);
    switch (v) {
    case CSS_OVERFLOW_HIDDEN:
        box->overflow_x = FOIL_RDRBOX_OVERFLOW_HIDDEN;
        break;
    case CSS_OVERFLOW_SCROLL:
        box->overflow_x = FOIL_RDRBOX_OVERFLOW_SCROLL;
        break;
    case CSS_OVERFLOW_AUTO:
        if (is_table_box(box))
            box->overflow_x = FOIL_RDRBOX_OVERFLOW_AUTO;
        else
            box->overflow_x = FOIL_RDRBOX_OVERFLOW_AUTO;
        break;
    default:
        box->overflow_x = FOIL_RDRBOX_OVERFLOW_VISIBLE;
        break;
    }

    LOG_DEBUG("\toverflow-x: %s\n", literal_values_overflow[box->overflow_x]);

    /* determine overflow_y */
    v = css_computed_overflow_y(
            ctxt->style);
    assert(v != CSS_OVERFLOW_INHERIT);
    switch (v) {
    case CSS_OVERFLOW_HIDDEN:
        box->overflow_y = FOIL_RDRBOX_OVERFLOW_HIDDEN;
        break;
    case CSS_OVERFLOW_SCROLL:
        box->overflow_y = FOIL_RDRBOX_OVERFLOW_SCROLL;
        break;
    case CSS_OVERFLOW_AUTO:
        if (is_table_box(box))
            box->overflow_x = FOIL_RDRBOX_OVERFLOW_AUTO;
        else
            box->overflow_x = FOIL_RDRBOX_OVERFLOW_AUTO;
        break;
    default:
        box->overflow_x = FOIL_RDRBOX_OVERFLOW_VISIBLE;
        break;
    }

    LOG_DEBUG("\toverflow-y: %s\n", literal_values_overflow[box->overflow_y]);

    /* determine unicode_bidi */
    v = css_computed_unicode_bidi(
            ctxt->style);
    assert(v != CSS_UNICODE_BIDI_INHERIT);
    switch (v) {
    case CSS_UNICODE_BIDI_EMBED:
        box->unicode_bidi = FOIL_RDRBOX_UNICODE_BIDI_EMBED;
        break;
    case CSS_UNICODE_BIDI_ISOLATE:
        box->unicode_bidi = FOIL_RDRBOX_UNICODE_BIDI_ISOLATE;
        break;
    case CSS_UNICODE_BIDI_BIDI_OVERRIDE:
        box->unicode_bidi = FOIL_RDRBOX_UNICODE_BIDI_BIDI_OVERRIDE;
        break;
    case CSS_UNICODE_BIDI_ISOLATE_OVERRIDE:
        box->unicode_bidi = FOIL_RDRBOX_UNICODE_BIDI_ISOLATE_OVERRIDE;
        break;
    case CSS_UNICODE_BIDI_PLAINTEXT:
        box->unicode_bidi = FOIL_RDRBOX_UNICODE_BIDI_PLAINTEXT;
        break;
    case CSS_UNICODE_BIDI_NORMAL:
    default:
        box->unicode_bidi = FOIL_RDRBOX_UNICODE_BIDI_NORMAL;
        break;
    }

    LOG_DEBUG("\tunicode-bidi: %s\n",
            literal_values_unicode_bidi[box->unicode_bidi]);

    /* determine text_transform */
    v = css_computed_text_transform(
            ctxt->style);
    assert(v != CSS_TEXT_TRANSFORM_INHERIT);
    switch (v) {
    case CSS_TEXT_TRANSFORM_CAPITALIZE:
        box->text_transform = FOIL_RDRBOX_TEXT_TRANSFORM_CAPITALIZE;
        break;
    case CSS_TEXT_TRANSFORM_UPPERCASE:
        box->text_transform = FOIL_RDRBOX_TEXT_TRANSFORM_UPPERCASE;
        break;
    case CSS_TEXT_TRANSFORM_LOWERCASE:
        box->text_transform = FOIL_RDRBOX_TEXT_TRANSFORM_LOWERCASE;
        break;
    case CSS_TEXT_TRANSFORM_NONE:
    default:
        box->text_transform = FOIL_RDRBOX_TEXT_TRANSFORM_NONE;
        break;
    }

    LOG_DEBUG("\ttext-transform: %s\n",
            literal_values_text_transform[box->text_transform]);

    /* determine white-space */
    v = css_computed_white_space(
            ctxt->style);
    assert(v != CSS_WHITE_SPACE_INHERIT);
    switch (v) {
    case CSS_WHITE_SPACE_PRE:
        box->white_space = FOIL_RDRBOX_WHITE_SPACE_PRE;
        break;
    case CSS_WHITE_SPACE_NOWRAP:
        box->white_space = FOIL_RDRBOX_WHITE_SPACE_NOWRAP;
        break;
    case CSS_WHITE_SPACE_PRE_WRAP:
        box->white_space = FOIL_RDRBOX_WHITE_SPACE_PRE_WRAP;
        break;
    case CSS_WHITE_SPACE_PRE_LINE:
        box->white_space = FOIL_RDRBOX_WHITE_SPACE_PRE_LINE;
        break;
    case CSS_WHITE_SPACE_BREAK_SPACES:
        box->white_space = FOIL_RDRBOX_WHITE_SPACE_BREAK_SPACES;
        break;
    case CSS_WHITE_SPACE_NORMAL:
    default:
        box->white_space = FOIL_RDRBOX_WHITE_SPACE_NORMAL;
        break;
    }

    LOG_DEBUG("\twhite-space: %s\n",
            literal_values_white_space[box->white_space]);

    /* determine text-decoration */
    v = css_computed_text_decoration(
            ctxt->style);
    assert(v != CSS_TEXT_DECORATION_INHERIT);
    if (v != CSS_TEXT_DECORATION_NONE) {
        if (v & CSS_TEXT_DECORATION_BLINK)
            box->text_deco_blink = 1;
        if (v & CSS_TEXT_DECORATION_LINE_THROUGH)
            box->text_deco_line_through = 1;
        if (v & CSS_TEXT_DECORATION_OVERLINE)
            box->text_deco_overline = 1;
        if (v & CSS_TEXT_DECORATION_UNDERLINE)
            box->text_deco_underline = 1;
    }

    LOG_DEBUG("\ttext-decoration: blink/%s, line-through/%s, "
            "overline/%s, underline/%s\n",
            box->text_deco_blink ? "yes" : "no",
            box->text_deco_line_through ? "yes" : "no",
            box->text_deco_overline ? "yes" : "no",
            box->text_deco_underline ? "yes" : "no");

    css_fixed length;
    css_unit unit;

    /* determine letter-spacing */
    v = css_computed_letter_spacing(
            ctxt->style,
            &length, &unit);
    assert(v != CSS_LETTER_SPACING_INHERIT);
    if (v == CSS_LETTER_SPACING_SET) {
        box->letter_spacing = calc_used_value_widths(box, unit, length);
    }
    /* CSS_LETTER_SPACING_NORMAL */

    if (box->letter_spacing < 0) {
        box->letter_spacing = 0;
    }

    LOG_DEBUG("\tletter-spacing: %d\n", box->letter_spacing);
    assert(box->letter_spacing >= 0);

    /* determine word-spacing */
    v = css_computed_word_spacing(
            ctxt->style,
            &length, &unit);
    assert(v != CSS_WORD_SPACING_INHERIT);
    if (v == CSS_WORD_SPACING_SET) {
        box->word_spacing = calc_used_value_widths(box, unit, length);
    }
    /* CSS_WORD_SPACING_NORMAL */

    if (box->word_spacing < 0) {
        box->word_spacing = 0;
    }

    LOG_DEBUG("\tword-spacing: %d\n", box->word_spacing);

    if (box->is_block_container) {
        /* determine text-indent */
        v = css_computed_text_indent(ctxt->style, &length, &unit);
        assert(v != CSS_TEXT_INDENT_INHERIT);
        box->text_indent = calc_used_value_widths(box, unit, length);

        if (box->text_indent < 0)
            box->text_indent = 0;

        LOG_DEBUG("\ttext-indent: %d\n", box->text_indent);

        /* determine text-align */
        v = css_computed_text_align(ctxt->style);
        assert(v != CSS_TEXT_ALIGN_INHERIT);
        switch (v) {
        case CSS_TEXT_ALIGN_RIGHT:
            box->text_align = FOIL_RDRBOX_TEXT_ALIGN_RIGHT;
            break;
        case CSS_TEXT_ALIGN_CENTER:
            box->text_align = FOIL_RDRBOX_TEXT_ALIGN_CENTER;
            break;
        case CSS_TEXT_ALIGN_JUSTIFY:
            box->text_align = FOIL_RDRBOX_TEXT_ALIGN_JUSTIFY;
            break;
        case CSS_TEXT_ALIGN_LEFT:
        default:
            box->text_align = FOIL_RDRBOX_TEXT_ALIGN_LEFT;
            break;
        }

        LOG_DEBUG("\ttext-align: %s\n",
                literal_values_text_align[box->text_align]);

        /* determine text-overflow.
           Note that CSSEng has a wrong interface for text-overflow */
        lwc_string *string;
        v = css_computed_text_overflow(
            ctxt->style, &string);
        if (v == CSS_TEXT_OVERFLOW_ELLIPSIS)
            box->text_overflow = FOIL_RDRBOX_TEXT_OVERFLOW_ELLIPSIS;
        else
            box->text_overflow = FOIL_RDRBOX_TEXT_OVERFLOW_CLIP;

        LOG_DEBUG("\ttext-overflow: %s\n",
                literal_values_text_overflow[box->text_overflow]);
    }

    /* determine word-break */
    v = css_computed_word_break(ctxt->style);
    assert(v != CSS_WORD_BREAK_INHERIT);
    switch (v) {
    case CSS_WORD_BREAK_BREAK_WORD:
        /* this is a legacy keyword */
        box->word_break = FOIL_RDRBOX_WORD_BREAK_NORMAL;
        break;

    case CSS_WORD_BREAK_BREAK_ALL:
        box->word_break = FOIL_RDRBOX_WORD_BREAK_BREAK_ALL;
        break;

    case CSS_WORD_BREAK_KEEP_ALL:
        box->word_break = FOIL_RDRBOX_WORD_BREAK_KEEP_ALL;
        break;

    case CSS_WORD_BREAK_NORMAL:
    default:
        box->word_break = FOIL_RDRBOX_WORD_BREAK_NORMAL;
        break;
    }

    LOG_DEBUG("\tword-break: %s\n",
            literal_values_word_break[box->word_break]);

    /* determine line-break */
    v = css_computed_line_break(ctxt->style);
    assert(v != CSS_LINE_BREAK_INHERIT);
    switch (v) {
    case CSS_LINE_BREAK_LOOSE:
        box->line_break = FOIL_RDRBOX_LINE_BREAK_LOOSE;
        break;

    case CSS_LINE_BREAK_NORMAL:
        box->line_break = FOIL_RDRBOX_LINE_BREAK_NORMAL;
        break;

    case CSS_LINE_BREAK_STRICT:
        box->line_break = FOIL_RDRBOX_LINE_BREAK_STRICT;
        break;

    case CSS_LINE_BREAK_ANYWHERE:
        box->line_break = FOIL_RDRBOX_LINE_BREAK_ANYWHERE;
        break;

    case CSS_LINE_BREAK_AUTO:
    default:
        box->line_break = FOIL_RDRBOX_LINE_BREAK_AUTO;
        break;
    }

    LOG_DEBUG("\tline-break: %s\n",
            literal_values_line_break[box->line_break]);

    /* determine word-wrap */
    v = css_computed_word_wrap(ctxt->style);
    assert(v != CSS_WORD_WRAP_INHERIT);
    switch (v) {
    case CSS_WORD_WRAP_BREAK_WORD:
        box->word_wrap = FOIL_RDRBOX_WORD_WRAP_BREAK_WORD;
        break;

    case CSS_WORD_WRAP_ANYWHERE:
        box->word_wrap = FOIL_RDRBOX_WORD_WRAP_ANYWHERE;
        break;

    case CSS_WORD_WRAP_NORMAL:
    default:
        box->word_wrap = FOIL_RDRBOX_WORD_WRAP_NORMAL;
        break;
    }

    LOG_DEBUG("\tword-wrap: %s\n",
            literal_values_word_wrap[box->word_wrap]);

    /* determine list-style-type
       (Foil always assumes list-style-image is `none`) */
    v = css_computed_list_style_type(ctxt->style);
    assert(v != CSS_LIST_STYLE_TYPE_INHERIT);
    box->list_style_type = normalize_list_style_type(v);

    LOG_DEBUG("\tlist-style-type: %s\n",
            literal_values_list_style_type[box->list_style_type]);

    /* determine list-style-position */
    v = css_computed_list_style_position(ctxt->style);
    assert(v != CSS_LIST_STYLE_POSITION_INHERIT);
    switch (v) {
    default:
    case CSS_LIST_STYLE_POSITION_OUTSIDE:
        box->list_style_position = FOIL_RDRBOX_LIST_STYLE_POSITION_OUTSIDE;
        break;

    case CSS_LIST_STYLE_POSITION_INSIDE:
        box->list_style_position = FOIL_RDRBOX_LIST_STYLE_POSITION_INSIDE;
        break;
    }

    LOG_DEBUG("\tlist-style-position: %s\n",
            literal_values_list_style_position[box->list_style_position]);

    /* determine foreground color */
    css_color color_argb;
    v = css_computed_color(ctxt->style, &color_argb);
    assert(v != CSS_COLOR_INHERIT);
    box->color = color_argb;

    LOG_DEBUG("\tcolor: 0x%08x\n", box->color);

    /* determine background color */
    v = css_computed_background_color(ctxt->style, &color_argb);
    assert(v != CSS_COLOR_INHERIT);
    box->background_color = color_argb;

    LOG_DEBUG("\tbackground-color: 0x%08x\n", box->background_color);

    /* determine quotes */
    lwc_string **strings = NULL;
    v = css_computed_quotes(ctxt->style, &strings);
    if (v == CSS_QUOTES_INHERIT) {
        if (ctxt->parent_box->quotes) {
            box->quotes = foil_quotes_ref(ctxt->parent_box->quotes);
        }
    }
    else if (v == CSS_QUOTES_STRING && strings != NULL) {
        size_t n = 0;
        while (strings[n]) {
            n++;
        }

        /* if n is odd */
        if ((n % 2)) {
            LOG_WARN("Bad number of quote strings: %u\n", (unsigned)n);
            n = (n >> 1) << 1;
        }

        if (n > 0 && strings != NULL) {
            box->quotes = foil_quotes_new_lwc(n, strings);
        }
    }
    else {
        // keep NULL
    }

    LOG_DEBUG("\tquotes: %p\n", box->quotes);

}

static void
dtrm_margin_left_right(foil_create_ctxt *ctxt, foil_rdrbox *box)
{
    uint8_t value;
    css_fixed length;
    css_unit unit;
    value = css_computed_margin_left(ctxt->style, &length, &unit);
    assert(value != CSS_MARGIN_INHERIT);
    switch (value) {
        case CSS_MARGIN_AUTO:
            box->ml = 0;
            break;
        case CSS_MARGIN_SET:
            box->ml = calc_used_value_widths(box, unit, length);
            break;
        default:
            assert(0);  // must be a bug
            break;
    }

    value = css_computed_margin_right(ctxt->style, &length, &unit);
    assert(value != CSS_MARGIN_INHERIT);
    switch (value) {
        case CSS_MARGIN_AUTO:
            box->mr = 0;
            break;
        case CSS_MARGIN_SET:
            box->mr = calc_used_value_widths(box, unit, length);
            break;
        default:
            assert(0);  // must be a bug
            break;
    }
}

static void
dtrm_margin_top_bottom(foil_create_ctxt *ctxt, foil_rdrbox *box)
{
    uint8_t value;
    css_fixed length;
    css_unit unit;

    value = css_computed_margin_top(ctxt->style, &length, &unit);
    assert(value != CSS_MARGIN_INHERIT);
    switch (value) {
        case CSS_MARGIN_AUTO:
            box->mt = 0;
            break;
        case CSS_MARGIN_SET:
            box->mr = calc_used_value_heights(box, unit, length);
            break;
        default:
            assert(0);  // must be a bug
            break;
    }

    value = css_computed_margin_bottom(ctxt->style, &length, &unit);
    assert(value != CSS_MARGIN_INHERIT);
    switch (value) {
        case CSS_MARGIN_AUTO:
            box->mb = 0;
            break;
        case CSS_MARGIN_SET:
            box->mb = calc_used_value_heights(box, unit, length);
            break;
        default:
            assert(0);  // must be a bug
            break;
    }
}

static int
get_intrinsic_width(foil_create_ctxt *ctxt)
{
    int l = 0;
    const char *value;
    size_t len;

    if (pcdoc_element_get_attribute(ctxt->doc, ctxt->elem,
            "width", &value, &len) == 0) {

        char *v = strndup(value, len);
        l = strtol(v, NULL, 10);
        free(v);
    }

    return (int)l;
}

static int
get_intrinsic_height(foil_create_ctxt *ctxt)
{
    int l = 0;
    const char *value;
    size_t len;

    if (pcdoc_element_get_attribute(ctxt->doc, ctxt->elem,
            "height", &value, &len) == 0) {

        char *v = strndup(value, len);
        l = strtol(v, NULL, 10);
        free(v);
    }

    return (int)l;
}

static int
get_intrinsic_ratio(foil_create_ctxt *ctxt)
{
    (void)ctxt;

    return 2.0f; // always assume the instrinsic ratio is 2:1
}

static uint8_t
dtrm_width_replaced(foil_create_ctxt *ctxt, foil_rdrbox *box)
{
    uint8_t width_v, height_v;
    css_fixed width_l, height_l;
    css_unit width_u, height_u;

    assert(box->is_replaced);

    width_v = css_computed_width(ctxt->style, &width_l, &width_u);
    assert(width_v != CSS_WIDTH_INHERIT);

    if (width_v != CSS_WIDTH_AUTO) {
        box->width = calc_used_value_widths(box, width_u, width_l);

        return width_v;
    }

    height_v = css_computed_height(ctxt->style, &height_l, &height_u);
    assert(height_v != CSS_HEIGHT_INHERIT);

    int intrinsic_width = get_intrinsic_width(ctxt);
    int intrinsic_height = get_intrinsic_height(ctxt);
    float intrinsic_ratio = get_intrinsic_ratio(ctxt);

    if (width_v == CSS_WIDTH_AUTO && height_v == CSS_HEIGHT_AUTO
            && intrinsic_width > 0) {
        box->width = round_width(intrinsic_width);
    }
    else if (width_v == CSS_WIDTH_AUTO && height_v == CSS_HEIGHT_AUTO
            && intrinsic_height > 0) {
        box->width = round_width(intrinsic_height * intrinsic_ratio);
    }
    else if (width_v == CSS_WIDTH_AUTO && height_v != CSS_HEIGHT_AUTO) {
        int height = 0;
        height = calc_used_value_heights(box, height_u, height_l);
        box->width = round_width(height * intrinsic_ratio);
    }
    else if (width_v == CSS_WIDTH_AUTO && height_v == CSS_HEIGHT_AUTO
            && intrinsic_ratio > 0) {
        LOG_WARN("`width` is undefined\n");
    }
    else if (width_v == CSS_WIDTH_AUTO && intrinsic_width > 0) {
        box->width = round_width(intrinsic_width);
    }
    else if (width_v == CSS_WIDTH_AUTO) {
        box->width = FOIL_PX_REPLACED_W;
    }

    return width_v;
}

static uint8_t
dtrm_height_replaced(foil_create_ctxt *ctxt, foil_rdrbox *box)
{
    uint8_t width_v, height_v;
    css_fixed width_l, height_l;
    css_unit width_u, height_u;

    assert(box->is_replaced);

    height_v = css_computed_height(ctxt->style, &height_l, &height_u);
    assert(height_v != CSS_HEIGHT_INHERIT);
    if (height_v != CSS_WIDTH_AUTO) {
        box->height = calc_used_value_heights(box, height_u, height_l);

        return height_v;
    }

    width_v = css_computed_width(ctxt->style, &width_l, &width_u);

    int intrinsic_height = get_intrinsic_height(ctxt);
    float intrinsic_ratio = get_intrinsic_ratio(ctxt);

    if (width_v == CSS_WIDTH_AUTO && height_v == CSS_HEIGHT_AUTO
            && intrinsic_height > 0) {
        box->height = round_height(intrinsic_height);
    }
    else if (height_v == CSS_HEIGHT_AUTO && intrinsic_ratio > 0) {
        box->height = round_height(box->width / intrinsic_ratio);
    }
    else if (height_v == CSS_HEIGHT_AUTO && intrinsic_height > 0) {
        box->height = round_height(intrinsic_height);
    }
    else if (height_v == CSS_WIDTH_AUTO) {
        box->height = FOIL_PX_REPLACED_H;
    }

    return width_v;
}

static uint8_t
dtrm_width_shrink_to_fit(foil_create_ctxt *ctxt, foil_rdrbox *box)
{
    uint8_t width_v;
    css_fixed width_l;
    css_unit width_u;

    width_v = css_computed_width(ctxt->style, &width_l, &width_u);
    assert(width_v != CSS_WIDTH_INHERIT);

    if (width_v != CSS_WIDTH_AUTO) {
        box->width = calc_used_value_widths(box, width_u, width_l);

        return width_v;
    }

    /* TODO */
    LOG_WARN("Not implemented\n");
    box->width = FOIL_PX_GRID_CELL_W * 10;
    return CSS_WIDTH_SET;
}

static void
dtrm_margin_left_right_block_normal(foil_create_ctxt *ctxt,
        foil_rdrbox *box, uint8_t width_v)
{
    int nr_autos = 0;
    int cblock_width = foil_rect_width(&box->cblock_rect);

    if (width_v == CSS_WIDTH_AUTO)
        nr_autos++;

    uint8_t v;
    css_fixed padding_left_l;
    css_unit padding_left_u;
    v = css_computed_padding_left(ctxt->style,
            &padding_left_l, &padding_left_u);
    assert(v != CSS_PADDING_INHERIT);
    box->pl = calc_used_value_widths(box, padding_left_u, padding_left_l);

    css_fixed padding_right_l;
    css_unit padding_right_u;
    v = css_computed_padding_right(ctxt->style,
            &padding_right_l, &padding_right_u);
    assert(v != CSS_PADDING_INHERIT);
    box->pr = calc_used_value_widths(box, padding_right_u, padding_right_l);
    (void)v;

    uint8_t margin_left_v;
    css_fixed margin_left_l;
    css_unit margin_left_u;
    margin_left_v = css_computed_margin_left(ctxt->style,
            &margin_left_l, &margin_left_u);
    assert(margin_left_v != CSS_MARGIN_INHERIT);

    if (margin_left_v == CSS_MARGIN_AUTO) {
        nr_autos++;
    }
    else {
        box->ml = calc_used_value_widths(box, margin_left_u, margin_left_l);
    }

    uint8_t margin_right_v;
    css_fixed margin_right_l;
    css_unit margin_right_u;
    margin_right_v = css_computed_margin_right(ctxt->style,
            &margin_right_l, &margin_right_u);
    assert(margin_right_v != CSS_MARGIN_INHERIT);
    if (margin_right_v == CSS_MARGIN_AUTO) {
        nr_autos++;
    }
    else {
        box->mr = calc_used_value_widths(box, margin_right_u, margin_right_l);
    }

    if (width_v != CSS_WIDTH_AUTO) {
        int tmp = box->ml + box->bl + box->pl + box->width +
            box->pr + box->br + box->mr;
        if (tmp > cblock_width) {
            if (margin_left_v == CSS_MARGIN_AUTO) {
                box->ml = 0;
            }

            if (margin_right_v == CSS_MARGIN_AUTO) {
                box->mr = 0;
            }
        }
    }

    if (nr_autos == 0) {
        if (box->width < 0) {
            LOG_WARN("Computed width is negative: %d\n", box->width);
            box->width = 0;
        }

        if (box->cblock_creator->direction == FOIL_RDRBOX_DIRECTION_LTR) {
            box->mr = cblock_width -
                box->width - box->pl - box->bl - box->pr - box->br - box->ml;
        }
        else {
            box->ml = cblock_width -
                box->width - box->pl - box->bl - box->pr - box->br - box->mr;
        }
    }
    else if (nr_autos == 1) {
        if (width_v == CSS_WIDTH_AUTO)
            box->width = cblock_width -
                box->ml - box->bl - box->pl - box->pr - box->br - box->mr;
        else if (margin_left_v == CSS_MARGIN_AUTO)
            box->ml = cblock_width -
                box->width - box->bl - box->pl - box->pr - box->br - box->mr;
        else if (margin_right_v == CSS_MARGIN_AUTO)
            box->mr = cblock_width -
                box->width - box->bl - box->pl - box->pr - box->br - box->ml;
    }

    if (width_v == CSS_WIDTH_AUTO) {
        if (margin_left_v == CSS_MARGIN_AUTO)
            box->ml = 0;

        if (margin_right_v == CSS_MARGIN_AUTO)
            box->mr = 0;

        box->width = cblock_width -
            box->ml - box->bl - box->pl - box->pr - box->br - box->mr;
    }

    if (margin_left_v == CSS_MARGIN_AUTO &&
            margin_right_v == CSS_MARGIN_AUTO) {
        int margins = cblock_width -
                box->width - box->bl - box->pl - box->pr - box->br;
        box->ml = margins >> 1;
        box->ml = round_width(box->ml);
        box->mr = margins - box->ml;
    }
}

/* calculate widths and margins */
static void
calc_widths_margins(foil_create_ctxt *ctxt, foil_rdrbox *box)
{
    if (box->type == FOIL_RDRBOX_TYPE_INLINE) {
        if (box->is_replaced) {
            /* CSS 2.2 Section 10.3.2 */

            dtrm_width_replaced(ctxt, box);
        }
        else {
            /* CSS 2.2 Section 10.3.1 */
            box->width = -1; // not apply
        }

        dtrm_margin_left_right(ctxt, box);
    }
    else if (box->type == FOIL_RDRBOX_TYPE_BLOCK && ctxt->in_normal_flow) {
        uint8_t width_v;

        if (box->is_replaced) {
            width_v = dtrm_width_replaced(ctxt, box);
        }
        else {
            css_fixed width_l;
            css_unit width_u;
            width_v = css_computed_width(ctxt->style, &width_l, &width_u);
            assert(width_v != CSS_WIDTH_INHERIT);
            if (width_v == CSS_WIDTH_AUTO) {
                box->width = 0;
            }
            else {
                box->width = calc_used_value_widths(box, width_u, width_l);
            }
        }

        dtrm_margin_left_right_block_normal(ctxt, box, width_v);
    }
    else if (ctxt->pos_schema == FOIL_RDRBOX_POSSCHEMA_FLOATS) {
        if (box->is_replaced) {
            dtrm_width_replaced(ctxt, box);
        }
        else {
            dtrm_width_shrink_to_fit(ctxt, box);
        }

        dtrm_margin_left_right(ctxt, box);
    }
    else if (ctxt->pos_schema == FOIL_RDRBOX_POSSCHEMA_ABSOLUTE) {
        LOG_WARN("Not implemented for absolutely positioned\n");
    }
    else if (box->type == FOIL_RDRBOX_TYPE_INLINE_BLOCK &&
            ctxt->in_normal_flow) {
        if (box->is_replaced) {
            dtrm_width_replaced(ctxt, box);
        }
        else {
            dtrm_width_shrink_to_fit(ctxt, box);
        }

        dtrm_margin_left_right(ctxt, box);
    }
    else {
        LOG_ERROR("Should not be here\n");
    }
}

/* calculate heights and margins */
static void
calc_heights_margins(foil_create_ctxt *ctxt, foil_rdrbox *box)
{
    if (box->type == FOIL_RDRBOX_TYPE_INLINE && !box->is_replaced) {
        box->height = -1; // not apply
    }
    else if (box->is_replaced && (box->type == FOIL_RDRBOX_TYPE_INLINE ||
                (box->type == FOIL_RDRBOX_TYPE_BLOCK &&
                    ctxt->in_normal_flow) ||
                (box->type == FOIL_RDRBOX_TYPE_INLINE_BLOCK &&
                    ctxt->in_normal_flow) ||
                ctxt->pos_schema == FOIL_RDRBOX_POSSCHEMA_FLOATS)) {

        dtrm_margin_top_bottom(ctxt, box);
        dtrm_height_replaced(ctxt, box);
    }
    else if (box->type == FOIL_RDRBOX_TYPE_BLOCK && !box->is_replaced &&
            ctxt->in_normal_flow) {

        css_fixed height_l;
        css_unit height_u;
        uint8_t height_v = css_computed_height(ctxt->style,
                &height_l, &height_u);
        assert(height_v != CSS_HEIGHT_INHERIT);

        if (height_v != CSS_WIDTH_AUTO) {
            box->height = calc_used_value_heights(box, height_u, height_l);
        }
        else if (box->overflow_y == CSS_OVERFLOW_VISIBLE) {
            dtrm_margin_top_bottom(ctxt, box);

            // set height is pending.
            box->height_pending = 1;
        }
    }
}

/* adjust position according to 'vertical-align' */
static void
adjust_position_vertically(foil_create_ctxt *ctxt, foil_rdrbox *box)
{
    (void)ctxt;
    (void)box;
}

/* TODO: check whether an element is replaced or non-replaced */
static int
is_replaced_element(pcdoc_element_t elem, const char *tag_name)
{
    (void)elem;
    (void)tag_name;
    return 0;
}

static foil_rdrbox *
create_rdrbox_from_style(foil_create_ctxt *ctxt)
{
    pcdoc_node node = { PCDOC_NODE_ELEMENT, { ctxt->elem } };
    foil_rdrbox *box = NULL;

    /* determine the box type */
    uint8_t display = css_computed_display(ctxt->style,
            pcdoc_node_get_parent(ctxt->doc, node) == NULL);

    // return INVALID_USED_VALUE_UINT8 for 'display:none;'
    uint8_t type = display_to_type(ctxt, display);
    if (type == INVALID_USED_VALUE_UINT8) {
        LOG_DEBUG("\tdisplay: %s\n", "none");
        goto failed;
    }

    LOG_DEBUG("\ttype: %s\n", literal_values_boxtype[type]);

    /* allocate the principal box */
    box = foil_rdrbox_new(type);
    if (box == NULL)
        goto failed;

    box->owner = ctxt->elem;

    /* whether is a block level box */
    if (box->type == FOIL_RDRBOX_TYPE_BLOCK ||
            box->type == FOIL_RDRBOX_TYPE_LIST_ITEM ||
            box->type == FOIL_RDRBOX_TYPE_TABLE)
        box->is_block_level = 1;
    else if (box->type == FOIL_RDRBOX_TYPE_INLINE ||
            box->type == FOIL_RDRBOX_TYPE_INLINE_BLOCK ||
            box->type == FOIL_RDRBOX_TYPE_INLINE_TABLE) {
        box->is_inline_level = 1;
    }

    /* whether is a block contianer */
    if (box->type == FOIL_RDRBOX_TYPE_BLOCK ||
            box->type == FOIL_RDRBOX_TYPE_LIST_ITEM)
        box->is_block_container = 1;
    else if (!box->is_replaced && box->type == FOIL_RDRBOX_TYPE_INLINE_BLOCK)
        box->is_block_container = 1;

    uint8_t v;
    v = css_computed_position(ctxt->style);
    assert(v != CSS_POSITION_INHERIT);
    switch (v) {
    case CSS_POSITION_RELATIVE:
        box->position = FOIL_RDRBOX_POSITION_RELATIVE;
        break;

    case CSS_POSITION_ABSOLUTE:
        box->position = FOIL_RDRBOX_POSITION_ABSOLUTE;
        break;

    case CSS_POSITION_FIXED:
        box->position = FOIL_RDRBOX_POSITION_FIXED;
        break;

    /* CSSEng does not support position: sticky so far
    case CSS_POSITION_STICKY:
        box->position = FOIL_RDRBOX_POSITION_STICKY;
        break; */

    case CSS_POSITION_STATIC:
    default:
        box->position = FOIL_RDRBOX_POSITION_STATIC;
        break;
    }

    LOG_DEBUG("\tposition: %s\n", literal_values_position[box->position]);

    /* determine float */
    v = css_computed_float(ctxt->style);
    assert(v != CSS_FLOAT_INHERIT);
    switch (v) {
    case CSS_FLOAT_LEFT:
        box->floating = FOIL_RDRBOX_FLOAT_LEFT;
        break;

    case CSS_FLOAT_RIGHT:
        box->floating = FOIL_RDRBOX_FLOAT_RIGHT;
        break;

    case CSS_FLOAT_NONE:
    default:
        box->floating = FOIL_RDRBOX_FLOAT_NONE;
        break;
    }

    LOG_DEBUG("\tfloat: %s\n", literal_values_float[box->floating]);

    /* TODO: determine the positioning schema */

    /* determine the containing block */
    if (purc_document_root(ctxt->doc) == ctxt->elem) {
        box->cblock_rect.left = 0;
        box->cblock_rect.top = 0;
        box->cblock_rect.right = ctxt->initial_cblock->width;
        box->cblock_rect.bottom = ctxt->initial_cblock->height;
        box->cblock_creator = ctxt->initial_cblock;
    }
    else if (box->position == FOIL_RDRBOX_POSITION_STATIC ||
            box->position == FOIL_RDRBOX_POSITION_RELATIVE) {
        /* the containing block is formed by the content edge of
           the nearest ancestor box that is a block container or
           which establishes a formatting context. */

        const foil_rdrbox *container;
        container = foil_rdrbox_find_container_for_relative(ctxt,
                ctxt->parent_box);
        assert(container);
        foil_rdrbox_content_box(container, &box->cblock_rect);
        box->cblock_creator = container;
    }
    else if (box->position == FOIL_RDRBOX_POSITION_FIXED) {
        box->cblock_rect.left = 0;
        box->cblock_rect.top = 0;
        box->cblock_rect.right = ctxt->initial_cblock->width;
        box->cblock_rect.bottom = ctxt->initial_cblock->height;
        box->cblock_creator = ctxt->initial_cblock;
    }
    else if (box->position == FOIL_RDRBOX_POSITION_ABSOLUTE) {
        /* the containing block is established by the nearest ancestor
           with a 'position' of 'absolute', 'relative' or 'fixed',
           in the following way:

           In the case that the ancestor is an inline element,
           the containing block is the bounding box around the padding boxes
           of the first and the last inline boxes generated for that element.

           Otherwise, the containing block is formed by the padding edge of
           the ancestor.

           If there is no such ancestor, the containing block is
           the initial containing block. */

        const foil_rdrbox *container;
        container = foil_rdrbox_find_container_for_absolute(ctxt,
                ctxt->parent_box);
        if (container) {
            if (container->type == FOIL_RDRBOX_TYPE_INLINE)
                foil_rdrbox_form_containing_block(container,
                        &box->cblock_rect);
            else
                foil_rdrbox_padding_box(container, &box->cblock_rect);
        }
        else {
            box->cblock_rect.left = 0;
            box->cblock_rect.top = 0;
            box->cblock_rect.right = ctxt->initial_cblock->width;
            box->cblock_rect.bottom = ctxt->initial_cblock->height;
        }
        box->cblock_creator = ctxt->initial_cblock;
    }

    /* determine the used values for common properties */
    dtrm_common_properties(ctxt, box);

    /* calculate widths and margins */
    calc_widths_margins(ctxt, box);

    /* calculate heights and margins */
    calc_heights_margins(ctxt, box);

    /* adjust position according to 'vertical-align' */
    adjust_position_vertically(ctxt, box);

    if (type == FOIL_RDRBOX_TYPE_LIST_ITEM) {
        box->list_item_data->index = ctxt->parent_box->nr_child_list_items;
        ctxt->parent_box->nr_child_list_items++;

        if (box->list_style_type != FOIL_RDRBOX_LIST_STYLE_TYPE_NONE) {
            // allocate the marker box
            foil_rdrbox *marker_box = foil_rdrbox_new(FOIL_RDRBOX_TYPE_MARKER);
            if (marker_box == NULL) {
                LOG_ERROR("Failed to create marker box\n");
                goto failed;
            }

            marker_box->owner = ctxt->elem;
            marker_box->is_anonymous = 1;
            box->list_item_data->marker_box = marker_box;
            foil_rdrbox_insert_before(box, marker_box);
        }
    }

    return box;

failed:
    if (box)
        foil_rdrbox_delete(box);

    return NULL;
}

/* call this function after attaching the box to the rendering tree */
static void
dtrm_counter_properties(foil_create_ctxt *ctxt, foil_rdrbox *box)
{
    const css_computed_counter *counters;
    uint8_t v;

    /* determine counter-reset */
    v = css_computed_counter_reset(ctxt->style, &counters);
    if (v == CSS_COUNTER_RESET_INHERIT) {
        if (ctxt->parent_box->counter_reset)
            box->counter_reset =
                foil_counters_ref(ctxt->parent_box->counter_reset);
    }
    else if (v == CSS_COUNTER_RESET_NAMED && counters != NULL) {
        box->counter_reset = foil_counters_new(counters);
        if (box->counter_reset == NULL) {
            LOG_WARN("Failed to create foil_counters for counter-reset\n");
        }
    }
    else {
        // do nothing.
    }

    if (box->counter_reset) {
        if ((box->counters_table = create_counters_table(ctxt)) == NULL) {
            LOG_WARN("Failed to create new hash table for counters\n");
        }
        else {
            for (size_t i = 0; i < box->counter_reset->nr_counters; i++) {
                lwc_string *name =
                    lwc_string_ref(box->counter_reset->counters[i].name);
                GHashTable *counters_table =
                    find_counters_table_in_prev_siblings(ctxt, box, name);

                if (counters_table == NULL)
                    counters_table = box->counters_table;

                g_hash_table_replace(counters_table,
                        name, (gpointer)box->counter_reset->counters[i].value);
            }
        }
    }

    LOG_DEBUG("counter-reset for %s: %d; counters: %p; counters table: %p\n",
            ctxt->tag_name, v, counters, box->counters_table);

    /* determine counter-increment */
    v = css_computed_counter_increment(ctxt->style, &counters);
    if (v == CSS_COUNTER_INCREMENT_INHERIT) {
        if (ctxt->parent_box->counter_incrm)
            box->counter_incrm =
                foil_counters_ref(ctxt->parent_box->counter_incrm);
    }
    else if (v == CSS_COUNTER_INCREMENT_NAMED && counters != NULL) {
        box->counter_incrm = foil_counters_new(counters);
        if (box->counter_incrm == NULL) {
            LOG_WARN("Failed to create foil_counters for counter-increment\n");
        }
    }
    else {
        // do nothing
    }

    if (box->counter_incrm) {
        for (size_t i = 0; i < box->counter_incrm->nr_counters; i++) {
            intptr_t old_value;
            GHashTable *counters_table =
                find_counters_table(ctxt, box,
                        box->counter_incrm->counters[i].name, &old_value);

            if (counters_table) {
                intptr_t new_value;
                new_value = old_value + box->counter_incrm->counters[i].value;
                lwc_string *name =
                    lwc_string_ref(box->counter_incrm->counters[i].name);
                g_hash_table_replace(counters_table, name, (gpointer)new_value);
            }
            else {
                /* behave as though a counter-reset had reset the counter to 0
                   on that element or pseudo-element. */
                if (box->counters_table == NULL) {
                    if ((box->counters_table = create_counters_table(ctxt)) == NULL) {
                        LOG_WARN("Failed to create new hash table for counters\n");
                    }
                }

                if (box->counters_table) {
                    lwc_string *name =
                        lwc_string_ref(box->counter_incrm->counters[i].name);
                    g_hash_table_replace(box->counters_table,
                            name, (gpointer)0);
                }
            }
        }
    }

    LOG_DEBUG("counter-increment for %s: %d; counters: %p; counter_incrm: %p\n",
            ctxt->tag_name, v, counters, box->counter_incrm);
}

foil_rdrbox *foil_rdrbox_create_principal(foil_create_ctxt *ctxt)
{
    foil_rdrbox *box;

    assert(ctxt->tag_name);

    ctxt->style = ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE];
    if ((box = create_rdrbox_from_style(ctxt))) {
        box->is_principal = 1;
        box->is_replaced = is_replaced_element(ctxt->elem, ctxt->tag_name);
        if (!box->is_replaced && box->type == FOIL_RDRBOX_TYPE_INLINE)
            box->is_inline_box = 1;

        foil_rdrbox_append_child(ctxt->parent_box, box);

        dtrm_counter_properties(ctxt, box);
    }

    return box;
}

static foil_rdrbox *
create_pseudo_box(foil_create_ctxt *ctxt, foil_rdrbox *principal)
{
    foil_rdrbox *box;

    if ((box = create_rdrbox_from_style(ctxt))) {
        box->principal = principal;
        box->is_anonymous = 1;
        box->is_pseudo = 1;
    }

    return box;
}

static void
init_pseudo_box_content(foil_create_ctxt *ctxt, foil_rdrbox *box)
{
    GString *text = NULL;
    const css_computed_content_item *ctnt_item;
    uint8_t v = css_computed_content(ctxt->style, &ctnt_item);
    assert(v != CSS_CONTENT_INHERIT);

    if (v == CSS_CONTENT_NONE || v == CSS_CONTENT_NORMAL) {
        // do nothing
    }
    else {
        assert(v == CSS_CONTENT_SET);
        text = g_string_new("");

        const char *str;
        size_t len;

        while (ctnt_item) {
            LOG_ERROR("content item type: %d\n", (int)ctnt_item->type);

            switch (ctnt_item->type) {
            case CSS_COMPUTED_CONTENT_NONE:
                ctnt_item = NULL;
                continue;

            case CSS_COMPUTED_CONTENT_STRING:
                str = lwc_string_data(ctnt_item->data.string);
                len = lwc_string_length(ctnt_item->data.string);
                g_string_append_len(text, str, len);
                break;

            case CSS_COMPUTED_CONTENT_ATTR: {
                const char *v;
                size_t l;
                v = lwc_string_data(ctnt_item->data.attr);
                l = lwc_string_length(ctnt_item->data.attr);

                if (l > 0) {
                    char *attr_name = strndup(v, l);
                    pcdoc_element_get_attribute(ctxt->doc, ctxt->elem,
                                attr_name, &str, &len);
                    free(attr_name);

                    g_string_append_len(text, str, len);
                }
                break;
            }

            case CSS_COMPUTED_CONTENT_URI:
                g_string_append(text, "<URI>");
                break;

            case CSS_COMPUTED_CONTENT_COUNTER: {
                intptr_t value;
                if (find_counters_table(ctxt, box,
                            ctnt_item->data.counter.name, &value)) {

                    uint8_t type = ctnt_item->data.counter.style;
                    if (type == 0)
                        type = CSS_LIST_STYLE_TYPE_DECIMAL;
                    type = normalize_list_style_type(type);

                    char *number = foil_rdrbox_list_number(0, (int)value,
                            type, NULL);
                    if (number) {
                        g_string_append(text, number);
                        free(number);
                    }
                }
                else {
                    LOG_ERROR("Could not find counters table for counter: %s\n",
                            lwc_string_data(ctnt_item->data.counter.name));
                }

                break;
            }

            case CSS_COMPUTED_CONTENT_COUNTERS: {
                generate_content_from_counters(ctxt, box, ctnt_item, text);
                break;
            }

            case CSS_COMPUTED_CONTENT_OPEN_QUOTE: {
                int quoting_depth =
                    ctxt->udom->nr_open_quotes - ctxt->udom->nr_close_quotes;
                if (quoting_depth < 0)
                    quoting_depth = 0;

                if (box->quotes && box->quotes->nr_strings > 0) {
                    size_t i = quoting_depth * 2;
                    if (i >= box->quotes->nr_strings) {
                        i = box->quotes->nr_strings - 2;
                    }

                    str = lwc_string_data(box->quotes->strings[i]);
                    len = lwc_string_length(box->quotes->strings[i]);
                    g_string_append_len(text, str, len);
                }

                ctxt->udom->nr_open_quotes++;
                break;
            }

            case CSS_COMPUTED_CONTENT_CLOSE_QUOTE: {
                ctxt->udom->nr_close_quotes++;

                int quoting_depth =
                    ctxt->udom->nr_open_quotes - ctxt->udom->nr_close_quotes;
                if (quoting_depth < 0)
                    quoting_depth = 0;

                if (box->quotes && box->quotes->nr_strings > 0) {
                    size_t i = quoting_depth * 2 + 1;
                    if (i >= box->quotes->nr_strings) {
                        i = box->quotes->nr_strings - 1;
                    }

                    str = lwc_string_data(box->quotes->strings[i]);
                    len = lwc_string_length(box->quotes->strings[i]);
                    g_string_append_len(text, str, len);
                }
                break;
            }

            case CSS_COMPUTED_CONTENT_NO_OPEN_QUOTE:
                ctxt->udom->nr_open_quotes++;
                break;

            case CSS_COMPUTED_CONTENT_NO_CLOSE_QUOTE:
                ctxt->udom->nr_close_quotes++;
                break;

            default:
                LOG_ERROR("bad content item type: %d\n", (int)ctnt_item->type);
                assert(0);
                break;
            }

            ctnt_item++;
        }

        if (text && text->len > 0) {
            foil_rdrbox *inline_box;
            if (box->is_block_level) {
                if ((inline_box = foil_rdrbox_create_anonymous_inline(ctxt,
                                box)) == NULL)
                    goto failed;
            }
            else {
                inline_box = box;
            }

            LOG_DEBUG("inline content: %s\n", text->str);
            if (!foil_rdrbox_init_inline_data(ctxt, inline_box,
                        text->str, text->len))
                goto failed;

            g_string_free(text, TRUE);
        }
    }

    return;

failed:
    if (text)
        g_string_free(text, TRUE);
}

foil_rdrbox *foil_rdrbox_create_before(foil_create_ctxt *ctxt,
        foil_rdrbox *principal)
{
    foil_rdrbox *box;

    ctxt->style = ctxt->computed->styles[CSS_PSEUDO_ELEMENT_BEFORE];
    if ((box = create_pseudo_box(ctxt, principal))) {
        LOG_DEBUG("created a box for :before pseudo element for %s\n",
                ctxt->tag_name);
        if (principal->is_block_level && box->is_inline_level) {
            foil_rdrbox_prepend_child(principal, box);
        }
        else {
            foil_rdrbox_insert_before(principal, box);
        }

        dtrm_counter_properties(ctxt, box);
        init_pseudo_box_content(ctxt, box);
    }

    return box;
}

foil_rdrbox *foil_rdrbox_create_after(foil_create_ctxt *ctxt,
        foil_rdrbox *principal)
{
    foil_rdrbox *box;

    ctxt->style = ctxt->computed->styles[CSS_PSEUDO_ELEMENT_AFTER];
    if ((box = create_pseudo_box(ctxt, principal))) {
        LOG_DEBUG("created a box for :after pseudo element for %s\n",
                ctxt->tag_name);
        if (principal->is_block_level && box->is_inline_level) {
            foil_rdrbox_append_child(principal, box);
        }
        else {
            foil_rdrbox_insert_after(principal, box);
        }
        dtrm_counter_properties(ctxt, box);
        init_pseudo_box_content(ctxt, box);
    }

    return box;
}

foil_rdrbox *foil_rdrbox_create_anonymous_block(foil_create_ctxt *ctxt,
        foil_rdrbox *parent, foil_rdrbox *before, foil_rdrbox *after)
{
    foil_rdrbox *box;

    box = foil_rdrbox_new(FOIL_RDRBOX_TYPE_BLOCK);
    if (box == NULL)
        goto failed;

    box->owner = ctxt->elem;
    box->is_anonymous = 1;
    box->is_block_level = 1;
    box->is_block_container = 1;

    if (before) {
        foil_rdrbox_insert_before(before, box);
    }
    else if (after) {
        foil_rdrbox_insert_after(after, box);
    }
    else if (parent) {
        foil_rdrbox_append_child(parent, box);
    }
    return box;

failed:
    return NULL;
}

/* create an anonymous inline box */
foil_rdrbox *foil_rdrbox_create_anonymous_inline(foil_create_ctxt *ctxt,
        foil_rdrbox *parent)
{
    foil_rdrbox *box;

    box = foil_rdrbox_new(FOIL_RDRBOX_TYPE_INLINE);
    if (box == NULL)
        goto failed;

    box->owner = ctxt->elem;
    box->is_anonymous = 1;
    box->is_inline_level = 1;
    box->is_inline_box = 1;

    foil_rdrbox_append_child(parent, box);
    return box;

failed:
    return NULL;
}

bool foil_rdrbox_content_box(const foil_rdrbox *box, foil_rect *rc)
{
    if (box->type == FOIL_RDRBOX_TYPE_INLINE)
        return false;

    rc->left   = box->cblock_rect.left   + box->left +
        box->ml + box->bl + box->pl;
    rc->top    = box->cblock_rect.top    + box->top  +
        box->mt + box->bt + box->pt;
    rc->right  = box->cblock_rect.right  + box->left +
        box->mr + box->br + box->pr;
    rc->bottom = box->cblock_rect.bottom + box->top  +
        box->mb + box->bb + box->pb;
    return true;
}

bool foil_rdrbox_padding_box(const foil_rdrbox *box, foil_rect *rc)
{
    if (box->type == FOIL_RDRBOX_TYPE_INLINE)
        return false;

    rc->left   = box->cblock_rect.left   + box->left + box->ml + box->bl;
    rc->top    = box->cblock_rect.top    + box->top  + box->mt + box->bt;
    rc->right  = box->cblock_rect.right  + box->left + box->mr + box->br;
    rc->bottom = box->cblock_rect.bottom + box->top  + box->mb + box->bb;

    return true;
}

bool foil_rdrbox_border_box(const foil_rdrbox *box, foil_rect *rc)
{
    if (box->type == FOIL_RDRBOX_TYPE_INLINE)
        return false;

    rc->left   = box->cblock_rect.left   + box->left + box->ml;
    rc->top    = box->cblock_rect.top    + box->top  + box->mt;
    rc->right  = box->cblock_rect.right  + box->left + box->mr;
    rc->bottom = box->cblock_rect.bottom + box->top  + box->mb;

    return true;
}

bool foil_rdrbox_margin_box(const foil_rdrbox *box, foil_rect *rc)
{
    if (box->type == FOIL_RDRBOX_TYPE_INLINE)
        return false;

    rc->left   = box->cblock_rect.left   + box->left;
    rc->top    = box->cblock_rect.top    + box->top;
    rc->right  = box->cblock_rect.right  + box->left;
    rc->bottom = box->cblock_rect.bottom + box->top;

    return true;
}

bool foil_rdrbox_form_containing_block(const foil_rdrbox *box, foil_rect *rc)
{
    (void)box;
    (void)rc;

    return false;
}

const foil_rdrbox *
foil_rdrbox_find_container_for_relative(foil_create_ctxt *ctxt,
        const foil_rdrbox *box)
{
    (void)ctxt;
    (void)box;

    return ctxt->initial_cblock;
}

const foil_rdrbox *
foil_rdrbox_find_container_for_absolute(foil_create_ctxt *ctxt,
        const foil_rdrbox *box)
{
    (void)ctxt;
    (void)box;

    return NULL;
}

char *foil_rdrbox_get_name(purc_document_t doc, const foil_rdrbox *box)
{
    char *name = NULL;

    if (box->is_initial) {
        name = strdup("initial");
    }
    else if (box->is_principal) {
        if (doc) {
            const char *tag_name;
            size_t len;
            pcdoc_element_get_tag_name(doc, box->owner, &tag_name, &len,
                    NULL, NULL, NULL, NULL);
            name = strndup(tag_name, len);
        }
        else {
            name = strdup("principal");
        }
    }
    else if (box->type == FOIL_RDRBOX_TYPE_MARKER) {
        name = strdup("marker");
    }
    else if (box->is_pseudo) {
        name = strdup("pseudo");
    }
    else {
        name = strdup("anonymous");
    }

    return name;
}

static void dump_ucs(const uint32_t *ucs, size_t nr_ucs)
{
    for (size_t i = 0; i < nr_ucs; i++) {
        char utf8[16];
        if (g_unichar_isgraph(ucs[i])) {
            unsigned len = pcutils_unichar_to_utf8(ucs[i],
                    (unsigned char *)utf8);
            utf8[len] = 0;
        }
        else {
            sprintf(utf8, "<U+%X>", ucs[i]);
        }
        fputs(utf8, stdout);
    }
}

void foil_rdrbox_dump(const foil_rdrbox *box,
        purc_document_t doc, unsigned level)
{
    unsigned n = 0;
    char indent[level * 2 + 1];

    indent[level * 2] = '\0';
    while (n < level) {
        indent[n * 2] = ' ';
        indent[n * 2 + 1] = ' ';
        n++;
    }

    char *name = foil_rdrbox_get_name(doc, box);

    fputs(indent, stdout);
    fprintf(stdout, "box for %s: "
            "(type: %s; position: %s; float: %s; block container: %s; "
            "block level: %s; inline level: %s; replaced: %s)\n",
            name,
            literal_values_boxtype[box->type],
            literal_values_position[box->position],
            literal_values_float[box->floating],
            box->is_block_container ? "yes" : "no",
            box->is_block_level ? "yes" : "no",
            box->is_inline_level ? "yes" : "no",
            box->is_replaced ? "yes" : "no");

    if (box->type == FOIL_RDRBOX_TYPE_MARKER) {
        fputs(indent, stdout);
        fputs(" content: ", stdout);
        dump_ucs(box->marker_data->ucs, box->marker_data->nr_ucs);
        fputs("\n", stdout);
    }
    else if (box->type == FOIL_RDRBOX_TYPE_INLINE) {
        fputs(indent, stdout);

        struct _inline_box_data *inline_data = box->inline_data;

        size_t nr_ucs = 0;
        struct text_paragraph *p;
        list_for_each_entry(p, &inline_data->paras, ln) {
            nr_ucs += p->nr_ucs;
        }

        fprintf(stdout, " content (paras: %u, chars: %u, ws: %d): ",
                inline_data->nr_paras, (unsigned)nr_ucs, box->white_space);

        list_for_each_entry(p, &inline_data->paras, ln) {
            dump_ucs(p->ucs, p->nr_ucs);
            // strcat(utf8, "…");
        }
        fputs("\n", stdout);
    }

    if (name)
        free(name);
}

void foil_rdrbox_render_before(foil_render_ctxt *ctxt,
        const foil_rdrbox *box, unsigned level)
{
    /* print title */
    if (level == 0) {
        if (ctxt->udom->title_ucs) {
            for (size_t i = 0; i < ctxt->udom->title_len; i++) {
                char utf8[10];
                unsigned len = pcutils_unichar_to_utf8(ctxt->udom->title_ucs[i],
                        (unsigned char *)utf8);
                utf8[len] = 0;
                fputs(utf8, stdout);
            }
            fputs("\n", stdout);
        }
    }

    if (box->is_block_level && box->first && box->first->is_inline_level) {
        fputs("\n", stdout);
    }
}

static void render_ucs(const uint32_t *ucs, size_t nr_ucs)
{
    for (size_t i = 0; i < nr_ucs; i++) {
        char utf8[16];
        unsigned len = pcutils_unichar_to_utf8(ucs[i],
                (unsigned char *)utf8);
        utf8[len] = 0;
        fputs(utf8, stdout);
    }
}

void foil_rdrbox_render_content(foil_render_ctxt *ctxt,
        const foil_rdrbox *box, unsigned level)
{
    (void)ctxt;
    (void)level;

    if (box->type == FOIL_RDRBOX_TYPE_LIST_ITEM) {
        if (box->list_item_data->marker_box) {
            foil_rdrbox *marker = box->list_item_data->marker_box;
            render_ucs(marker->marker_data->ucs, marker->marker_data->nr_ucs);
        }
    }
    else if (box->type == FOIL_RDRBOX_TYPE_INLINE) {
        struct _inline_box_data *inline_data = box->inline_data;
        struct text_paragraph *p;
        list_for_each_entry(p, &inline_data->paras, ln) {
            render_ucs(p->ucs, p->nr_ucs);
        }
    }
}

void foil_rdrbox_render_after(foil_render_ctxt *ctxt,
        const foil_rdrbox *box, unsigned level)
{
    (void)ctxt;
    (void)level;

    if (box->is_block_level && box->first && box->first->is_inline_level) {
        fputs("\n", stdout);
    }
}

