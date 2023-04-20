/*
 * @file page.h
 * @author Vincent Wei
 * @date 2022/10/10
 * @brief The header for page.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of purc, which is an HVML interpreter with
 * a command line interface (CLI).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef purc_foil_page_h
#define purc_foil_page_h

#include "foil.h"
#include "region/rect.h"
#include "unicode/unicode.h"
#include "util/list.h"

#define FOIL_DEFCLR_MASK    0x80000000

enum {
    FOIL_TTY_COLOR_STD_16C = 0,
    FOIL_TTY_COLOR_XTERM_256C,
    FOIL_TTY_COLOR_TRUE_COLOR,
};

struct foil_tty_cell {
    /* the Unicode code point of the character */
    uint32_t uc;

    /* the character attributes */
    uint8_t attrs;

    /* Indicate whether the cell is the latter half of a wide character. */
    uint8_t latter_half:1;

    /* the foreground color (index or true color x8r8g8b8);
       the most significant bit indicates whether it's the default color. */
    int fgc;
    /* the background color (index or true color x8r8g8b8);
       the most significant bit indicates whether it's the default color. */
    int bgc;
};

/* a page is the client area of a window or widget,
   which is used to render the content. */
struct pcmcth_page {
    /* rows and columns of the whole page */
    int rows, cols;

    /* the current character attributes */
    uint8_t attrs;
    uint8_t color_mode;

    /* the current foreground color;
       the most significant bit indicates using the default color. */
    int fgc;
    /* the current background color;
       the most significant bit indicates using the default color. */
    int bgc;

    /* the dirty rectangle;
       TODO: use region in the future. */
    foil_rect dirty_rect;

    /* Since PURCMC-120 */
    purc_page_ostack_t ostack;

    pcmcth_udom *udom;
    struct foil_tty_cell **cells;
};

#ifdef __cplusplus
extern "C" {
#endif

int foil_page_module_init(pcmcth_renderer *rdr);
void foil_page_module_cleanup(pcmcth_renderer *rdr);

/* Allocates an anonymous page. */
pcmcth_page *foil_page_new(pcmcth_workspace *workspace);
/* Deletes an anonymous page and returns the uDOM set for it. */
pcmcth_udom *foil_page_delete(pcmcth_page *page);

/* Returns the workspace to which the page belongs */
pcmcth_workspace *foil_page_get_workspace(pcmcth_page *page);

bool foil_page_content_init(pcmcth_page *page, int cols, int rows,
        foil_color fgc, foil_color bgc);
void foil_page_content_cleanup(pcmcth_page *page);

/* Sets uDOM and return the old one */
pcmcth_udom *foil_page_set_udom(pcmcth_page *page, pcmcth_udom *udom);

void foil_page_set_fgc(pcmcth_page *page, foil_color color);
void foil_page_set_bgc(pcmcth_page *page, foil_color color);
void foil_page_set_attrs(pcmcth_page *page, uint8_t attrs);

int foil_page_draw_uchar(pcmcth_page *page, int x, int y,
        uint32_t uc, size_t count);
int foil_page_draw_ustring(pcmcth_page *page, int x, int y,
        uint32_t *ucs, size_t nr_ucs);

bool foil_page_fill_rect(pcmcth_page *page, const foil_rect *rc, uint32_t uc);
bool foil_page_erase_rect(pcmcth_page *page, const foil_rect *rc);
bool foil_page_expose(pcmcth_page *page);

#ifdef __cplusplus
}
#endif

static inline int foil_page_cols(const pcmcth_page *page) {
    return page->cols;
}

static inline int foil_page_cell(const pcmcth_page *page) {
    return page->rows;
}

#if 0
/* a span is a group of continuous characters which are all with
   same foreground color and background color and decorations in a row */
struct foil_row_span_line_mode {
    /* the code of the foreground color; e.g., "30m" for black */
    const char *fgc_code;

    /* the code of the background color; e.g., "40m" for black */
    const char *bgc_code;

    /* the code for the decoration; e.g., "1" for bold */
    const char *decoration;

    /* the pointer to the text in Unicode codepoints */
    const uint32_t *ucs;

    /* the number of characters in this span */
    int n;

    /* the width (columns) of this span */
    int cols;
};

struct foil_line_line_mode {
    unsigned nr_spans;
    struct foil_row_span_line_mode *spans;
};

struct foil_contents_line_mode {
    unsigned nr_lines;
    struct foil_line_line_mode *lines;
};
#endif /* deprecated */

#endif  /* purc_foil_page_h */

