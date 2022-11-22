/*
** @file rdrbox-internal.h
** @author Vincent Wei
** @date 2022/10/10
** @brief The internal interface for renderring box.
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

#ifndef purc_foil_rdrbox_internal_h
#define purc_foil_rdrbox_internal_h

#include <purc/purc-utils.h>
#include <stdint.h>

#include "unicode/unicode.h"

#define LEN_BUF_INTEGER 128

struct text_paragraph {
    struct list_head ln;

    /* the code points of text in Unicode (logical order) */
    uint32_t *ucs;
    size_t nr_ucs;

    /* the break opportunities of the characters */
    foil_break_oppo_t *break_oppos;
};

struct _inline_box_data {
    foil_langcode_t lang;
    unsigned nr_paras;

    /* the text segments */
    struct list_head paras;
};

struct _block_box_data {
    int text_indent;

    uint8_t text_align:3;
};

struct _inline_block_data {
    int foo, bar;

    uint8_t text_align:3;
};

struct _list_item_data {
    unsigned index;           /* index in the parent box */
    foil_rdrbox *marker_box;  /* NULL for no marker */
};

struct _marker_box_data {
    uint32_t *ucs;
    size_t    nr_ucs;
};

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif /* not defined purc_foil_rdrbox_internal_h */

