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

static bool
numbering_decimal(unsigned u, char *buf, size_t sz_buf)
{
    unsigned len = 0;
    unsigned tmp = u;

    do {
        len++;
        tmp = tmp / 10;
    } while (tmp);

    if (len + 2 > sz_buf)
        return false;

    ssize_t pos = len - 1;
    while (u) {
        unsigned r = u % 10;
        u = u / 10;

        assert(pos >= 0);
        buf[pos] = '0' + r;
        pos--;
    }

    buf[len] = '.';
    buf[len + 1] = '\0';
    return true;
}

static bool
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

    if (len + 2 > sz_buf) {
        return false;
    }

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

    buf[len] = '.';
    buf[len + 1] = '\0';
    return true;
}

static bool
numbering_lower_roman(unsigned u, char *buf, size_t sz_buf)
{
    (void)u;
    (void)sz_buf;
    strcpy(buf, "TODO/lower-roman");
    return true;
}

static bool
numbering_upper_roman(unsigned u, char *buf, size_t sz_buf)
{
    (void)u;
    (void)sz_buf;
    strcpy(buf, "TODO/upper-roman");
    return true;
}

static bool
numbering_georgian(unsigned u, char *buf, size_t sz_buf)
{
    (void)u;
    (void)sz_buf;
    strcpy(buf, "TODO/numbering georgian");
    return true;
}

static bool
numbering_armenian(unsigned u, char *buf, size_t sz_buf)
{
    (void)u;
    (void)sz_buf;
    strcpy(buf, "TODO/numbering armenian");
    return true;
}

static bool
alphabetic_lower_latin(unsigned u, char *buf, size_t sz_buf)
{
    unsigned len = 0;
    unsigned tmp = u;

    do {
        len++;
        tmp = tmp / 26;
    } while (tmp);

    if (len + 2 > sz_buf)
        return false;

    ssize_t pos = len - 1;
    while (u) {
        unsigned r = u % 26;
        u = u / 26;

        assert(pos >= 0);
        buf[pos] = 'a' + r;
        pos--;
    }

    buf[len] = '.';
    buf[len + 1] = '\0';
    return true;
}

static bool
alphabetic_upper_latin(unsigned u, char *buf, size_t sz_buf)
{
    unsigned len = 0;
    unsigned tmp = u;

    do {
        len++;
        tmp = tmp / 26;
    } while (tmp);

    if (len + 2 > sz_buf)
        return false;

    ssize_t pos = len - 1;
    while (u) {
        unsigned r = u % 26;
        u = u / 26;

        assert(pos >= 0);
        buf[pos] = 'A' + r;
        pos--;
    }

    buf[len] = '.';
    buf[len + 1] = '\0';
    return true;
}

static bool
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
    if ((len + 1) * 2 > sz_buf)
        return false;

    ssize_t pos = (len - 1) * 2;
    while (u) {
        unsigned r = u % nr_greek_letters;
        u = u / nr_greek_letters;

        assert(pos >= 0);
        pcutils_unichar_to_utf8(uchar_lower_greek_first + r,
                (unsigned char *)buf + pos);
        pos -= 2;
    }

    buf[len * 2] = '.';
    buf[len * 2 + 1] = '\0';
    return true;
}

bool foil_rdrbox_init_marker_data(foil_create_ctxt *ctxt,
        foil_rdrbox *marker, const foil_rdrbox *list_item)
{
    char buff[128];

    assert(list_item->list_style_type != FOIL_RDRBOX_LIST_STYLE_TYPE_NONE);

    marker->owner = ctxt->elem;
    marker->is_anonymous = 1;

    /* copy some properties from the principal box */
    marker->fgc = list_item->fgc;
    marker->bgc = list_item->bgc;

    buff[0] = '\0';
    const unsigned nr_items = list_item->parent->nr_child_list_items;
    const unsigned index = list_item->list_item_data->index;
    struct _marker_box_data *data = marker->marker_data;
    switch (list_item->list_style_type) {
    case FOIL_RDRBOX_LIST_STYLE_TYPE_DISC:
        data->atom = purc_atom_from_static_string_ex(
                PURC_ATOM_BUCKET_RDR, "●");
        break;

    case FOIL_RDRBOX_LIST_STYLE_TYPE_CIRCLE:
        data->atom = purc_atom_from_static_string_ex(
                PURC_ATOM_BUCKET_RDR, "○");
        break;

    case FOIL_RDRBOX_LIST_STYLE_TYPE_SQUARE:
        data->atom = purc_atom_from_static_string_ex(
                PURC_ATOM_BUCKET_RDR, "□");
        break;

    case FOIL_RDRBOX_LIST_STYLE_TYPE_DECIMAL:
        numbering_decimal(index + 1, buff, sizeof(buff));
        LOG_DEBUG("%s\n", buff);
        break;

    case FOIL_RDRBOX_LIST_STYLE_TYPE_DECIMAL_LEADING_ZERO:
        numbering_decimal_leading_zero(index, nr_items,
                buff, sizeof(buff));
        break;

    case FOIL_RDRBOX_LIST_STYLE_TYPE_LOWER_ROMAN:
        numbering_lower_roman(index, buff, sizeof(buff));
        break;

    case FOIL_RDRBOX_LIST_STYLE_TYPE_UPPER_ROMAN:
        numbering_upper_roman(index, buff, sizeof(buff));
        break;

    case FOIL_RDRBOX_LIST_STYLE_TYPE_ARMENIAN:
        numbering_armenian(index, buff, sizeof(buff));
        break;

    case FOIL_RDRBOX_LIST_STYLE_TYPE_GEORGIAN:
        numbering_georgian(index, buff, sizeof(buff));
        break;

    case FOIL_RDRBOX_LIST_STYLE_TYPE_LOWER_GREEK:
        alphabetic_lower_greek(index, buff, sizeof(buff));
        break;

    case FOIL_RDRBOX_LIST_STYLE_TYPE_LOWER_LATIN:
        alphabetic_lower_latin(index, buff, sizeof(buff));
        break;

    case FOIL_RDRBOX_LIST_STYLE_TYPE_UPPER_LATIN:
        alphabetic_upper_latin(index, buff, sizeof(buff));
        break;

    }

    if (buff[0]) {
        data->atom = purc_atom_from_string_ex(PURC_ATOM_BUCKET_RDR, buff);
    }

    return (data->atom != 0);
}

