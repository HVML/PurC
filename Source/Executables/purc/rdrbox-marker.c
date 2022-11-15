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

#include <assert.h>

static unsigned
numbering_decimal(unsigned u, char *buf, size_t sz_buf)
{
    unsigned len = 0;
    unsigned tmp = u;

    do {
        len++;
        tmp = tmp / 10;
    } while (tmp);

    if (len > sz_buf)
        return 0;

    buf[len] = '\0';

    ssize_t pos = len - 1;
    while (u) {
        unsigned r = u % 10;
        u = u / 10;

        assert(pos >= 0);
        buf[pos] = '0' + r;
        pos--;
    }

    return len;
}

static unsigned
numbering_decimal_leading_zero(unsigned u, unsigned max,
        char *buf, size_t sz_buf)
{
    if (u > max) {
        return numbering_decimal(u, buf, sz_buf);
    }

    unsigned len = 0;
    unsigned tmp = max;

    do {
        len++;
        tmp = tmp / 10;
    } while (tmp);

    if (len > sz_buf) {
        return 0;
    }

    buf[len] = '\0';

    ssize_t pos = len - 1;
    while (u && pos > 0) {
        unsigned r = u % 10;
        u = u / 10;

        assert(pos >= 0);
        buf[pos] = '0' + r;
        pos--;
    }

    while (pos > 0) {
        buf[pos] = '0';
        pos--;
    }

    return len;
}

static unsigned
numbering_lower_roman(unsigned u, char *buf, size_t sz_buf)
{
    (void)u;
    (void)sz_buf;
    strcpy(buf, "TODO/lower-roman");
    return strlen(buf);
}

static unsigned
numbering_upper_roman(unsigned u, char *buf, size_t sz_buf)
{
    (void)u;
    (void)sz_buf;
    strcpy(buf, "TODO/upper-roman");
    return strlen(buf);
}

static unsigned
numbering_georgian(unsigned u, char *buf, size_t sz_buf)
{
    (void)u;
    (void)sz_buf;
    strcpy(buf, "TODO/numbering georgian");
    return strlen(buf);
}

static unsigned
numbering_armenian(unsigned u, char *buf, size_t sz_buf)
{
    (void)u;
    (void)sz_buf;
    strcpy(buf, "TODO/numbering armenian");
    return strlen(buf);
}

static unsigned
alphabetic_lower_latin(unsigned u, char *buf, size_t sz_buf)
{
    unsigned len = 0;
    unsigned tmp = u;

    do {
        len++;
        tmp = tmp / 26;
    } while (tmp);

    if (len > sz_buf)
        return 0;

    buf[len] = '\0';

    ssize_t pos = len - 1;
    while (u) {
        unsigned r = u % 26;
        u = u / 26;

        assert(pos >= 0);
        buf[pos] = 'a' + r;
        pos--;
    }

    return len;
}

static unsigned
alphabetic_upper_latin(unsigned u, char *buf, size_t sz_buf)
{
    unsigned len = 0;
    unsigned tmp = u;

    do {
        len++;
        tmp = tmp / 26;
    } while (tmp);

    if (len > sz_buf)
        return 0;

    buf[len] = '\0';

    ssize_t pos = len - 1;
    while (u) {
        unsigned r = u % 26;
        u = u / 26;

        assert(pos >= 0);
        buf[pos] = 'A' + r;
        pos--;
    }

    return len;
}

static unsigned
alphabetic_lower_greek(unsigned u, char *buf, size_t sz_buf)
{
    unsigned len = 0;
    unsigned tmp = u;
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
    if (len > sz_buf)
        return 0;

    buf[len] = '\0';

    ssize_t pos = len - 2;
    while (u) {
        unsigned r = u % nr_greek_letters;
        u = u / nr_greek_letters;

        assert(pos >= 0);
        pcutils_unichar_to_utf8(uchar_lower_greek_first + r,
                (unsigned char *)buf + pos);
        pos -= 2;
    }

    return len;
}

#define UTF8STR_OF_DISC_CHAR    "●"
#define UTF8STR_OF_CIRCLE_CHAR  "○"
#define UTF8STR_OF_SQUARE_CHAR  "□"

purc_atom_t foil_rdrbox_list_number(const unsigned nr_items,
        const unsigned index, uint8_t type, const char *tail)
{
    unsigned tail_len = tail ? strlen(tail) : 0;
    char buff[LEN_BUF_INTEGER + tail_len + 4];
    unsigned len;
    purc_atom_t atom = 0;

    buff[0] = '\0';
    switch (type) {
    case FOIL_RDRBOX_LIST_STYLE_TYPE_DISC:
        strcpy(buff, UTF8STR_OF_DISC_CHAR);
        len = sizeof(UTF8STR_OF_DISC_CHAR) - 1;
        break;

    case FOIL_RDRBOX_LIST_STYLE_TYPE_CIRCLE:
        strcpy(buff, UTF8STR_OF_CIRCLE_CHAR);
        len = sizeof(UTF8STR_OF_CIRCLE_CHAR) - 1;
        break;

    case FOIL_RDRBOX_LIST_STYLE_TYPE_SQUARE:
        strcpy(buff, UTF8STR_OF_SQUARE_CHAR);
        len = sizeof(UTF8STR_OF_SQUARE_CHAR) - 1;
        break;

    case FOIL_RDRBOX_LIST_STYLE_TYPE_DECIMAL:
        len = numbering_decimal(index + 1, buff, sizeof(buff));
        break;

    case FOIL_RDRBOX_LIST_STYLE_TYPE_DECIMAL_LEADING_ZERO:
        len = numbering_decimal_leading_zero(index, nr_items,
                buff, sizeof(buff));
        break;

    case FOIL_RDRBOX_LIST_STYLE_TYPE_LOWER_ROMAN:
        len = numbering_lower_roman(index, buff, sizeof(buff));
        break;

    case FOIL_RDRBOX_LIST_STYLE_TYPE_UPPER_ROMAN:
        len = numbering_upper_roman(index, buff, sizeof(buff));
        break;

    case FOIL_RDRBOX_LIST_STYLE_TYPE_ARMENIAN:
        len = numbering_armenian(index, buff, sizeof(buff));
        break;

    case FOIL_RDRBOX_LIST_STYLE_TYPE_GEORGIAN:
        len = numbering_georgian(index, buff, sizeof(buff));
        break;

    case FOIL_RDRBOX_LIST_STYLE_TYPE_LOWER_GREEK:
        len = alphabetic_lower_greek(index, buff, sizeof(buff));
        break;

    case FOIL_RDRBOX_LIST_STYLE_TYPE_LOWER_LATIN:
        len = alphabetic_lower_latin(index, buff, sizeof(buff));
        break;

    case FOIL_RDRBOX_LIST_STYLE_TYPE_UPPER_LATIN:
        len = alphabetic_upper_latin(index, buff, sizeof(buff));
        break;
    }

    if (buff[0]) {
        unsigned i = 0;
        if (tail && (len + tail_len) < sizeof(buff)) {
            while (tail[i]) {
                buff[len + i] = tail[i];
                i++;
            }
        }
        buff[len + i] = '\0';

        atom = purc_atom_from_string_ex(PURC_ATOM_BUCKET_RDR, buff);
    }

    return atom;
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

    data->atom = foil_rdrbox_list_number(nr_items, index,
            list_item->list_style_type, tail);
    return (data->atom != 0);
}

