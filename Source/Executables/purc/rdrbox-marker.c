/*
** @file rdrbox-marker.c
** @author Vincent Wei
** @date 2022/10/29
** @brief The implementation of marker box.
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

#undef NDEBUG

#include "rdrbox.h"
#include "rdrbox-internal.h"

#include <glib.h>

#include <assert.h>

static unsigned
numbering_decimal(GString *text, int u)
{
    unsigned len = 0;
    int tmp = u;

    if (u <= 0) {
        g_string_printf(text, "%d", u);
        return text->len;
    }

    do {
        len++;
        tmp = tmp / 10;
    } while (tmp);

    g_string_set_size(text, len);

    ssize_t pos = len - 1;
    while (u) {
        unsigned r = u % 10;
        u = u / 10;

        assert(pos >= 0);
        text->str[pos] = '0' + r;
        pos--;
    }

    return len;
}

static unsigned
numbering_decimal_leading_zero(GString *text, int u, int max)
{
    if (u <= 0 || u > max) {
        return numbering_decimal(text, u);
    }

    unsigned len = 0;
    int tmp = max;

    do {
        len++;
        tmp = tmp / 10;
    } while (tmp);

    g_string_set_size(text, len);

    ssize_t pos = len - 1;
    while (u && pos > 0) {
        unsigned r = u % 10;
        u = u / 10;

        assert(pos >= 0);
        text->str[pos] = '0' + r;
        pos--;
    }

    while (pos >= 0) {
        text->str[pos] = '0';
        pos--;
    }

    return len;
}

static unsigned
numbering_lower_roman(GString *text, int u)
{
    if (u <= 0) {
        return numbering_decimal(text, u);
    }

    g_string_assign(text, "TODO/lower-roman");
    return text->len;
}

static unsigned
numbering_upper_roman(GString *text, int u)
{
    if (u <= 0) {
        return numbering_decimal(text, u);
    }

    g_string_assign(text, "TODO/upper-roman");
    return text->len;
}

static unsigned
numbering_georgian(GString *text, int u)
{
    if (u <= 0) {
        return numbering_decimal(text, u);
    }

    g_string_assign(text, "TODO/numbering georgian");
    return text->len;
}

static unsigned
numbering_armenian(GString *text, int u)
{
    if (u <= 0) {
        return numbering_decimal(text, u);
    }

    g_string_assign(text, "TODO/numbering armenian");
    return text->len;
}

static unsigned
alphabetic_lower_latin(GString *text, int u)
{
    if (u <= 0) {
        return numbering_decimal(text, u);
    }

    unsigned len = 0;
    int tmp = u;

    do {
        len++;
        tmp = tmp / 26;
    } while (tmp);

    g_string_set_size(text, len);

    ssize_t pos = len - 1;
    while (u) {
        unsigned r = u % 26;
        u = u / 26;

        assert(pos >= 0);
        text->str[pos] = 'a' + r - 1;
        pos--;
    }

    return len;
}

static unsigned
alphabetic_upper_latin(GString *text, int u)
{
    if (u <= 0) {
        return numbering_decimal(text, u);
    }

    unsigned len = 0;
    int tmp = u;

    do {
        len++;
        tmp = tmp / 26;
    } while (tmp);

    g_string_set_size(text, len);

    ssize_t pos = len - 1;
    while (u) {
        unsigned r = u % 26;
        u = u / 26;

        assert(pos >= 0);
        text->str[pos] = 'A' + r - 1;
        pos--;
    }

    return len;
}

static unsigned
alphabetic_lower_greek(GString *text, int u)
{
    if (u <= 0) {
        return numbering_decimal(text, u);
    }

    unsigned len = 0;
    int tmp = u;
    static const uint32_t uchar_lower_greek_first = 0x03B1;  // α
    static const uint32_t uchar_upper_greek_last  = 0x03C9;  // ω
    static const unsigned nr_greek_letters =
        uchar_upper_greek_last - uchar_lower_greek_first + 1;

    do {
        len++;
        tmp = tmp / nr_greek_letters;
    } while (tmp);

    /* The lenght of UTF-8 encoding of a greek letter is 2:
       U+03B1 -> CE B1 */
    len *= 2;
    g_string_set_size(text, len);

    ssize_t pos = len - 2;
    while (u) {
        unsigned r = u % nr_greek_letters;
        u = u / nr_greek_letters;

        assert(pos >= 0);
        pcutils_unichar_to_utf8(uchar_lower_greek_first + r - 1,
                (unsigned char *)text->str + pos);
        pos -= 2;
    }

    return len;
}

#define UTF8STR_OF_DISC_CHAR    "●"
#define UTF8STR_OF_CIRCLE_CHAR  "○"
#define UTF8STR_OF_SQUARE_CHAR  "□"

char *foil_rdrbox_list_number(const int max,
        const int number, uint8_t type, const char *tail)
{
    GString *text = g_string_new("");

    switch (type) {
    case FOIL_RDRBOX_LIST_STYLE_TYPE_DISC:
        g_string_append(text, UTF8STR_OF_DISC_CHAR);
        break;

    case FOIL_RDRBOX_LIST_STYLE_TYPE_CIRCLE:
        g_string_append(text, UTF8STR_OF_CIRCLE_CHAR);
        break;

    case FOIL_RDRBOX_LIST_STYLE_TYPE_SQUARE:
        g_string_append(text, UTF8STR_OF_SQUARE_CHAR);
        break;

    case FOIL_RDRBOX_LIST_STYLE_TYPE_DECIMAL:
        numbering_decimal(text, number);
        break;

    case FOIL_RDRBOX_LIST_STYLE_TYPE_DECIMAL_LEADING_ZERO:
        numbering_decimal_leading_zero(text, number, max);
        break;

    case FOIL_RDRBOX_LIST_STYLE_TYPE_LOWER_ROMAN:
        numbering_lower_roman(text, number);
        break;

    case FOIL_RDRBOX_LIST_STYLE_TYPE_UPPER_ROMAN:
        numbering_upper_roman(text, number);
        break;

    case FOIL_RDRBOX_LIST_STYLE_TYPE_ARMENIAN:
        numbering_armenian(text, number);
        break;

    case FOIL_RDRBOX_LIST_STYLE_TYPE_GEORGIAN:
        numbering_georgian(text, number);
        break;

    case FOIL_RDRBOX_LIST_STYLE_TYPE_LOWER_GREEK:
        alphabetic_lower_greek(text, number);
        break;

    case FOIL_RDRBOX_LIST_STYLE_TYPE_LOWER_LATIN:
        alphabetic_lower_latin(text, number);
        break;

    case FOIL_RDRBOX_LIST_STYLE_TYPE_UPPER_LATIN:
        alphabetic_upper_latin(text, number);
        break;
    }

    if (tail) {
        g_string_append(text, tail);
    }

    return g_string_free(text, FALSE);
}

static void marker_data_cleaner(void *data)
{
    struct _marker_box_data *marker_data = (struct _marker_box_data *)data;
    if (marker_data && marker_data->text)
        free(marker_data->text);
}

bool foil_rdrbox_init_marker_data(foil_create_ctxt *ctxt,
        foil_rdrbox *marker, const foil_rdrbox *list_item)
{
    assert(list_item->list_style_type != FOIL_RDRBOX_LIST_STYLE_TYPE_NONE);

    marker->owner = ctxt->elem;
    marker->is_anonymous = 1;

    /* copy some properties from the principal box */
    marker->fgc = list_item->fgc;
    marker->bgc = list_item->bgc;

    const unsigned nr_items = list_item->parent->nr_child_list_items;
    const unsigned index = list_item->list_item_data->index;
    struct _marker_box_data *data = marker->marker_data;
    const char *tail = NULL;

    switch (list_item->list_style_type) {
    case FOIL_RDRBOX_LIST_STYLE_TYPE_DECIMAL:
    case FOIL_RDRBOX_LIST_STYLE_TYPE_DECIMAL_LEADING_ZERO:
        tail = ". ";
        break;
    case FOIL_RDRBOX_LIST_STYLE_TYPE_LOWER_ROMAN:
    case FOIL_RDRBOX_LIST_STYLE_TYPE_UPPER_ROMAN:
    case FOIL_RDRBOX_LIST_STYLE_TYPE_ARMENIAN:
    case FOIL_RDRBOX_LIST_STYLE_TYPE_GEORGIAN:
    case FOIL_RDRBOX_LIST_STYLE_TYPE_LOWER_GREEK:
    case FOIL_RDRBOX_LIST_STYLE_TYPE_LOWER_LATIN:
    case FOIL_RDRBOX_LIST_STYLE_TYPE_UPPER_LATIN:
        tail = ") ";
        break;
    }

    data->text = foil_rdrbox_list_number(nr_items, index + 1,
            list_item->list_style_type, tail);
    marker->cb_data_cleanup = marker_data_cleaner;
    return (data->text != NULL);
}

