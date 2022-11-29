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
numbering_decimal(GString *text, int number)
{
    unsigned len = 0;
    int tmp = number;

    if (number <= 0) {
        g_string_printf(text, "%d", number);
        return text->len;
    }

    do {
        len++;
        tmp = tmp / 10;
    } while (tmp);

    g_string_set_size(text, len);

    ssize_t pos = len - 1;
    while (number) {
        unsigned r = number % 10;
        number = number / 10;

        assert(pos >= 0);
        text->str[pos] = '0' + r;
        pos--;
    }

    return len;
}

static unsigned
numbering_decimal_leading_zero(GString *text, int number, int max)
{
    if (number <= 0 || number > max) {
        return numbering_decimal(text, number);
    }

    unsigned len = 0;
    int tmp = max;

    do {
        len++;
        tmp = tmp / 10;
    } while (tmp);

    g_string_set_size(text, len);

    ssize_t pos = len - 1;
    while (number && pos >= 0) {
        unsigned r = number % 10;
        number = number / 10;

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

/* Note that this function is copied from LGPL'd WebKit */
static unsigned
numbering_roman(GString *text, int number, bool upper_lower)
{
    // Big enough to store largest roman number less than 3999 which
    // is 3888 (MMMDCCCLXXXVIII)
    if (number < 1 || number > 3999) {
        return numbering_decimal(text, number);
    }

#if 0
    const gunichar udigits[] = {
        0x2160, /* Ⅰ */
        0x2161, /* Ⅱ */
        0x2162, /* Ⅲ */
        0x2163, /* Ⅳ */
        0x2164, /* Ⅴ */
        0x2165, /* Ⅵ */
        0x2166, /* Ⅶ */
        0x2167, /* Ⅷ */
        0x2168, /* Ⅸ */
        0x2169, /* Ⅹ */
        0x216A, /* Ⅺ */
        0x216B, /* Ⅻ */
        0x216C, /* Ⅼ */
        0x216D, /* Ⅽ */
        0x216E, /* Ⅾ */
        0x216F, /* Ⅿ */
    };
    unsigned upper_off = upper_lower ? 0 : (0x2170 - 0x2160);
#endif

    const unsigned size = 15;
    g_string_set_size(text, size);

    unsigned length = 0;
    const char ldigits[] = { 'i', 'v', 'x', 'l', 'c', 'd', 'm' };
    const char udigits[] = { 'I', 'V', 'X', 'L', 'C', 'D', 'M' };
    const char *digits = upper_lower ? udigits : ldigits;
    int d = 0;

    do {
        int num = number % 10;
        if (num % 5 < 4)
            for (int i = num % 5; i > 0; i--)
                text->str[size - ++length] = digits[d];
        if (num >= 4 && num <= 8)
            text->str[size - ++length] = digits[d + 1];
        if (num == 9)
            text->str[size - ++length] = digits[d + 2];
        if (num % 5 == 4)
            text->str[size - ++length] = digits[d];
        number /= 10;
        d += 2;
    } while (number);

    assert(length <= size);
    if (length < size)
        g_string_erase(text, 0, size - length);

    return text->len;
}

/* Note that this function is copied from LGPL'd WebKit */
static unsigned
numbering_georgian(GString *text, int number)
{
    if (number < 1 || number > 19999) {
        return numbering_decimal(text, number);
    }

    if (number > 9999)
        g_string_append_unichar(text, 0x10F5);

    int thousands;
    if ((thousands = (number / 1000) % 10)) {
        static const gunichar georgianThousands[9] = {
            0x10E9, 0x10EA, 0x10EB, 0x10EC, 0x10ED, 0x10EE, 0x10F4, 0x10EF, 0x10F0
        };
        g_string_append_unichar(text, georgianThousands[thousands - 1]);
    }

    int hundreds;
    if ((hundreds = (number / 100) % 10)) {
        static const gunichar georgianHundreds[9] = {
            0x10E0, 0x10E1, 0x10E2, 0x10F3, 0x10E4, 0x10E5, 0x10E6, 0x10E7, 0x10E8
        };
        g_string_append_unichar(text, georgianHundreds[hundreds - 1]);
    }

    int tens;
    if ((tens = (number / 10) % 10)) {
        static const gunichar georgianTens[9] = {
            0x10D8, 0x10D9, 0x10DA, 0x10DB, 0x10DC, 0x10F2, 0x10DD, 0x10DE, 0x10DF
        };
        g_string_append_unichar(text, georgianTens[tens - 1]);
    }

    int ones;
    if ((ones = number % 10)) {
        static const gunichar georgianOnes[9] = {
            0x10D0, 0x10D1, 0x10D2, 0x10D3, 0x10D4, 0x10D5, 0x10D6, 0x10F1, 0x10D7
        };
        g_string_append_unichar(text, georgianOnes[ones - 1]);
    }

    return text->len;
}

/* Note that this function is copied from LGPL'd WebKit */
static void to_armenian_under_10000(GString *text, int number,
        bool upper, bool circumflex)
{
    assert(number >= 0 && number < 10000);

    int lower_off = upper ? 0 : 0x0030;
    int thousands;
    if ((thousands = number / 1000)) {
        if (thousands == 7) {
            g_string_append_unichar(text, 0x0552 + lower_off);
            if (circumflex)
                g_string_append_unichar(text, 0x0302);
        } else {
            g_string_append_unichar(text, (0x054C - 1 + lower_off) + thousands);
            if (circumflex)
                g_string_append_unichar(text, 0x0302);
        }
    }

    int hundreds;
    if ((hundreds = (number / 100) % 10)) {
        g_string_append_unichar(text, (0x0543 - 1 + lower_off) + hundreds);
        if (circumflex)
            g_string_append_unichar(text, 0x0302);
    }

    int tens;
    if ((tens = (number / 10) % 10)) {
        g_string_append_unichar(text, (0x053A - 1 + lower_off) + tens);
        if (circumflex)
            g_string_append_unichar(text, 0x0302);
    }

    int ones;
    if ((ones = number % 10)) {
        g_string_append_unichar(text, (0x531 - 1 + lower_off) + ones);
        if (circumflex)
            g_string_append_unichar(text, 0x0302);
    }
}

/* Note that this function is copied from LGPL'd WebKit */
static unsigned
numbering_armenian(GString *text, int number, bool upper_lower)
{
    if (number < 0 || number > 99999999) {
        return numbering_decimal(text, number);
    }

    to_armenian_under_10000(text, number / 10000, upper_lower, true);
    to_armenian_under_10000(text, number % 10000, upper_lower, false);
    return text->len;
}

static unsigned
alphabetic_lower_latin(GString *text, int number)
{
    if (number <= 0) {
        return numbering_decimal(text, number);
    }

    unsigned len = 0;
    int tmp = number;

    do {
        len++;
        tmp = tmp / 26;
    } while (tmp);

    g_string_set_size(text, len);

    ssize_t pos = len - 1;
    while (number) {
        unsigned r = number % 26;
        number = number / 26;

        assert(pos >= 0);
        text->str[pos] = 'a' + r - 1;
        pos--;
    }

    return len;
}

static unsigned
alphabetic_upper_latin(GString *text, int number)
{
    if (number <= 0) {
        return numbering_decimal(text, number);
    }

    unsigned len = 0;
    int tmp = number;

    do {
        len++;
        tmp = tmp / 26;
    } while (tmp);

    g_string_set_size(text, len);

    ssize_t pos = len - 1;
    while (number) {
        unsigned r = number % 26;
        number = number / 26;

        assert(pos >= 0);
        text->str[pos] = 'A' + r - 1;
        pos--;
    }

    return len;
}

static unsigned
alphabetic_lower_greek(GString *text, int number)
{
    if (number <= 0) {
        return numbering_decimal(text, number);
    }

    unsigned len = 0;
    int tmp = number;
    static const uint32_t uchar_lower_greek_first = 0x03B1;  // α
    static const uint32_t uchar_lower_greek_last  = 0x03C9;  // ω
    static const unsigned nr_greek_letters =
        uchar_lower_greek_last - uchar_lower_greek_first + 1;

    do {
        len++;
        tmp = tmp / nr_greek_letters;
    } while (tmp);

    /* The lenght of UTF-8 encoding of a greek letter is 2:
       U+03B1 -> CE B1 */
    len *= 2;
    g_string_set_size(text, len);

    ssize_t pos = len - 2;
    while (number) {
        unsigned r = number % nr_greek_letters;
        number = number / nr_greek_letters;

        assert(pos >= 0);
        pcutils_unichar_to_utf8(uchar_lower_greek_first + r - 1,
                (unsigned char *)text->str + pos);
        pos -= 2;
    }

    return len;
}

static unsigned numbering_numeric(GString *text, int number,
        const char *numeric_symbols[])
{
    if (number < 0)
        return numbering_decimal(text, number);

    if (number == 0) {
        g_string_assign(text, numeric_symbols[0]);
        return text->len;
    }

    while (number) {
        unsigned r = number % 10;
        number = number / 10;

        g_string_prepend(text, numeric_symbols[r]);
    }

    return text->len;
}

#define UTF8STR_OF_DISC_CHAR    "●"
#define UTF8STR_OF_CIRCLE_CHAR  "○"
#define UTF8STR_OF_SQUARE_CHAR  "□"

static const char *cjk_numeric_symbols[] = {
    "〇", "一", "二", "三", "四", "五", "六", "七", "八", "九",
};

static const char *tibetan_numeric_symbols[] = {
    "༠", "༡", "༢", "༣", "༤", "༥", "༦", "༧", "༨", "༩",
};

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
        numbering_roman(text, number, false);
        break;

    case FOIL_RDRBOX_LIST_STYLE_TYPE_UPPER_ROMAN:
        numbering_roman(text, number, true);
        break;

    case FOIL_RDRBOX_LIST_STYLE_TYPE_LOWER_ARMENIAN:
        numbering_armenian(text, number, false);
        break;

    case FOIL_RDRBOX_LIST_STYLE_TYPE_UPPER_ARMENIAN:
        numbering_armenian(text, number, true);
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

    case FOIL_RDRBOX_LIST_STYLE_TYPE_CJK_DECIMAL:
        numbering_numeric(text, number, cjk_numeric_symbols);
        break;

    case FOIL_RDRBOX_LIST_STYLE_TYPE_TIBETAN:
        numbering_numeric(text, number, tibetan_numeric_symbols);
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
    if (marker_data && marker_data->ucs)
        free(marker_data->ucs);
}

bool foil_rdrbox_init_marker_data(foil_create_ctxt *ctxt,
        foil_rdrbox *marker, const foil_rdrbox *list_item)
{
    (void)ctxt;
    assert(list_item->list_style_type != FOIL_RDRBOX_LIST_STYLE_TYPE_NONE);

    /* TODO: copy values of inheritable properties from the principal box */
    marker->color = list_item->color;
    marker->background_color = list_item->background_color;

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
    case FOIL_RDRBOX_LIST_STYLE_TYPE_UPPER_ARMENIAN:
    case FOIL_RDRBOX_LIST_STYLE_TYPE_LOWER_ARMENIAN:
    case FOIL_RDRBOX_LIST_STYLE_TYPE_GEORGIAN:
    case FOIL_RDRBOX_LIST_STYLE_TYPE_LOWER_GREEK:
    case FOIL_RDRBOX_LIST_STYLE_TYPE_LOWER_LATIN:
    case FOIL_RDRBOX_LIST_STYLE_TYPE_UPPER_LATIN:
        tail = ") ";
        break;

    case FOIL_RDRBOX_LIST_STYLE_TYPE_CJK_DECIMAL:
        tail = "、";
        break;
    }

    char *text = foil_rdrbox_list_number(nr_items, index + 1,
            list_item->list_style_type, tail);
    data->ucs = pcutils_string_decode_utf8_alloc(text, -1, &data->nr_ucs);
    free(text);

    data->width = foil_ucs_calc_width_nowrap(data->ucs, data->nr_ucs);
    marker->cb_data_cleanup = marker_data_cleaner;
    return (data->ucs != NULL);
}

