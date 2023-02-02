/*
** @file page.c
** @author Vincent Wei
** @date 2022/10/10
** @brief The implementation of page (the view for showing the rendered tree).
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

#include "page.h"
#include "udom.h"
#include "widget.h"

#include "unicode/unicode.h"

#include <assert.h>

int foil_page_module_init(pcmcth_renderer *rdr)
{
    return foil_udom_module_init(rdr);
}

void foil_page_module_cleanup(pcmcth_renderer *rdr)
{
    foil_udom_module_cleanup(rdr);
}

bool foil_page_content_init(pcmcth_page *page, int cols, int rows,
        uint8_t fgc, uint8_t bgc)
{
    if (page->cells)
        foil_page_content_cleanup(page);

    page->cells = calloc(rows, sizeof(struct foil_tty_cell *));
    if (page->cells == NULL)
        return false;

    for (int i = 0; i < rows; i++) {
        page->cells[i] = calloc(cols, sizeof(struct foil_tty_cell));
        if (page->cells[i] == NULL) {
            foil_page_content_cleanup(page);
        }
    }

    page->rows = rows;
    page->cols = cols;
    page->udom = NULL;

    page->attrs = FOIL_CHAR_ATTR_NULL;
    page->fgc   = fgc;
    page->bgc   = bgc;

    foil_page_fill_rect(page, NULL, FOIL_UCHAR_SPACE);
    return true;
}

void foil_page_content_cleanup(pcmcth_page *page)
{
    if (page->cells) {
        for (int i = 0; i < page->rows; i++) {
            if (page->cells[i])
                free(page->cells[i]);
        }
        free(page->cells);
    }

    page->rows = 0;
    page->cols = 0;
    page->cells = NULL;
}

/* An anonymous page resides in an orphan widget */
pcmcth_page *foil_page_new(pcmcth_workspace *workspace)
{
    struct foil_widget *orphan;
    foil_rect rc;
    foil_rect_set(&rc, 0, 0, 80, 25);
    orphan = foil_widget_new(
            WSP_WIDGET_TYPE_PLAINWINDOW, WSP_WIDGET_BORDER_NONE,
            "orphan", "An orphan widget", &rc);

    if (orphan) {
        orphan->user_data = workspace;
        return &orphan->page;
    }
    return NULL;
}

pcmcth_udom *foil_page_delete(pcmcth_page *page)
{
    struct foil_widget *orphan = foil_widget_from_page(page);
    assert(orphan->parent == NULL);

    pcmcth_udom *udom = page->udom;
    foil_page_content_cleanup(page);
    foil_widget_delete(orphan);

    return udom;
}

pcmcth_udom *foil_page_set_udom(pcmcth_page *page, pcmcth_udom *udom)
{
    pcmcth_udom *old_udom = page->udom;
    page->udom = udom;

    return old_udom;
}

uint8_t foil_page_set_fgc(pcmcth_page *page, uint8_t color)
{
    uint8_t old = page->fgc;
    page->fgc = color;
    return old;
}

uint8_t foil_page_set_bgc(pcmcth_page *page, uint8_t color)
{
    uint8_t old = page->bgc;
    page->bgc = color;
    return old;
}

uint8_t foil_page_set_attrs(pcmcth_page *page, uint8_t attrs)
{
    uint8_t old = page->attrs;
    page->attrs = attrs;
    return old;
}

/* use the foreground color of the page but reserve the background color */
int foil_page_draw_uchar(pcmcth_page *page, int x, int y,
        uint32_t uc, size_t count)
{
    if (x < 0 || y < 0 || x >= page->cols || y >= page->rows || count == 0)
        return 0;

    struct foil_tty_cell *dst_cell = page->cells[y];
    dst_cell += x;

    int nr_cells = 0;
    foil_rect dirty = { x, y, x, y + 1 };
    if (dst_cell->latter_half) {
        assert( x > 0);

        /* reset this cell and the previous cell to space */
        dst_cell[0].uc = FOIL_UCHAR_SPACE;
        dst_cell[0].latter_half = 0;
        dst_cell[-1].uc = FOIL_UCHAR_SPACE;
        dst_cell[-1].latter_half = 0;

        dirty.left--;
        nr_cells++;
    }

    size_t my_count = 0;
    if (g_unichar_iswide(uc)) {
        while (x < page->cols - 1 && my_count < count) {
            dst_cell->uc = uc;
            dst_cell->attrs = page->attrs;
            dst_cell->fgc = page->fgc;
            dst_cell->latter_half = 0;
            dst_cell++;

            dst_cell->latter_half = 1;
            dst_cell++;

            x += 2;
            nr_cells += 2;
            my_count++;
        }
    }
    else {
        while (x < page->cols && my_count < count) {
            dst_cell->uc = uc;
            dst_cell->latter_half = 0;
            dst_cell->attrs = page->attrs;
            dst_cell->fgc = page->fgc;
            dst_cell++;

            x++;
            nr_cells++;
            my_count++;
        }
    }
    dirty.right = x;

    if (nr_cells > 0 && dst_cell->latter_half) {
        assert(x < page->cols);

        /* reset this cell to space */
        dst_cell[0].uc = FOIL_UCHAR_SPACE;
        dst_cell[0].latter_half = 0;

        dirty.right++;
    }

    foil_rect_get_bound(&page->dirty_rect, &page->dirty_rect, &dirty);
    return nr_cells;
}

int foil_page_draw_ustring(pcmcth_page *page, int x, int y,
        uint32_t *ucs, size_t nr_ucs)
{
    if (y < 0 || x >= page->cols || y >= page->rows || nr_ucs == 0)
        return 0;

    /* skip characters beyond the left bound */
    while (x < 0 && nr_ucs > 0) {
        if (g_unichar_iswide(*ucs))
            x += 2;
        else
            x += 1;
        ucs++;
        nr_ucs--;
    }

    if (nr_ucs == 0)
        return 0;

    assert(x >= 0);

    struct foil_tty_cell *dst_cell = page->cells[y];
    dst_cell += x;

    int nr_cells = 0;
    foil_rect dirty = { x, y, x, y + 1 };
    if (dst_cell->latter_half) {
        assert(x > 0);

        /* reset this cell and the previous cell to space */
        dst_cell[0].uc = FOIL_UCHAR_SPACE;
        dst_cell[0].latter_half = 0;
        dst_cell[-1].uc = FOIL_UCHAR_SPACE;
        dst_cell[-1].latter_half = 0;

        dirty.left--;
        nr_cells++;
    }

    size_t my_count = 0;
    while (x < page->cols && my_count < nr_ucs) {
        uint32_t uc = ucs[my_count];

        if (g_unichar_iswide(uc)) {
            if (x == page->cols - 1)
                break;

            dst_cell->uc = uc;
            dst_cell->attrs = page->attrs;
            dst_cell->fgc = page->fgc;
            dst_cell->latter_half = 0;
            dst_cell++;

            dst_cell->latter_half = 1;
            dst_cell++;

            x += 2;
            nr_cells += 2;
            my_count++;
        }
        else {
            dst_cell->uc = uc;
            dst_cell->attrs = page->attrs;
            dst_cell->fgc = page->fgc;
            dst_cell->latter_half = 0;
            dst_cell++;

            x++;
            nr_cells++;
            my_count++;
        }
    }
    dirty.right = x;

    if (nr_cells > 0 && dst_cell->latter_half) {
        assert(x < page->cols);

        /* reset this cell to space */
        dst_cell[0].uc = FOIL_UCHAR_SPACE;
        dst_cell[0].latter_half = 0;

        dirty.right++;
    }

    foil_rect_get_bound(&page->dirty_rect, &page->dirty_rect, &dirty);
    return nr_cells;
}

bool foil_page_fill_rect(pcmcth_page *page, const foil_rect *rc, uint32_t uc)
{
    if (rc == NULL) {
        struct foil_tty_cell cell;
        cell.uc = uc;
        cell.attrs = page->attrs;
        cell.fgc = page->fgc;
        cell.bgc = page->bgc;
        cell.latter_half = 0;

        if (g_unichar_iswide(uc)) {
            for (int y = 0; y < page->rows; y++) {
                struct foil_tty_cell *line = page->cells[y];
                cell.latter_half = 0;
                for (int x = 0; x < page->cols; x += 2) {
                    memcpy(line + x, &cell, sizeof(cell));
                }

                cell.latter_half = 1;
                for (int x = 1; x < page->cols; x += 2) {
                    memcpy(line + x, &cell, sizeof(cell));
                }
            }
        }
        else {
            for (int y = 0; y < page->rows; y++) {
                struct foil_tty_cell *line = page->cells[y];
                for (int x = 0; x < page->cols; x++) {
                    memcpy(line + x, &cell, sizeof(cell));
                }
            }
        }

        foil_rect_set(&page->dirty_rect, 0, 0, page->cols, page->rows);
    }
    else {
        foil_rect my_rc;
        foil_rect_set(&my_rc, 0, 0, page->cols, page->rows);
        if (!foil_rect_intersect(&my_rc, &my_rc, rc))
            goto done;

        size_t count = my_rc.right - my_rc.left;
        for (int y = my_rc.top; y < my_rc.bottom; y++) {
            foil_page_draw_uchar(page, my_rc.left, y, uc, count);
        }
    }

    return true;

done:
    return false;
}

bool foil_page_erase_rect(pcmcth_page *page, const foil_rect *rc)
{
    struct foil_tty_cell cell;
    cell.uc = FOIL_UCHAR_SPACE;
    cell.attrs = page->attrs;
    cell.fgc = page->fgc;
    cell.bgc = page->bgc;
    cell.latter_half = 0;

    foil_rect my_rc;
    if (rc == NULL) {
        foil_rect_set(&my_rc, 0, 0, page->cols, page->rows);
    }
    else {
        foil_rect_set(&my_rc, 0, 0, page->cols, page->rows);
        if (!foil_rect_intersect(&my_rc, &my_rc, rc))
            goto done;
    }

    foil_rect dirty = my_rc;
    for (int y = my_rc.top; y < my_rc.bottom; y++) {
        struct foil_tty_cell *line = page->cells[y];

        for (int x = my_rc.left; x < my_rc.right; x++) {
            memcpy(line + x, &cell, sizeof(cell));
        }

        if (line[my_rc.left].latter_half) {
            assert(my_rc.left > 0);
            line[my_rc.left - 1].uc = FOIL_UCHAR_SPACE;
            line[my_rc.left - 1].latter_half = 0;
            dirty.left = my_rc.left - 1;
        }

        if (my_rc.right < page->cols && line[my_rc.right].latter_half) {
            line[my_rc.right].uc = FOIL_UCHAR_SPACE;
            line[my_rc.left].latter_half = 0;
            dirty.right = my_rc.right + 1;
        }
    }

    foil_rect_get_bound(&page->dirty_rect, &page->dirty_rect, &dirty);
    return true;

done:
    return false;
}

bool foil_page_expose(pcmcth_page *page)
{
    foil_widget *widget = foil_widget_from_page(page);
    if (foil_rect_is_empty(&page->dirty_rect)) {
        return false;
    }

    foil_widget_expose(widget);
    foil_rect_empty(&page->dirty_rect);
    return true;
}

pcmcth_workspace *foil_page_get_workspace(pcmcth_page *page)
{
    pcmcth_workspace *workspace = NULL;

    foil_widget *widget = foil_widget_from_page(page);

    foil_widget *root = foil_widget_get_root(widget);
    if (root) {
        workspace = (pcmcth_workspace *)root->user_data;
    }
    else {
        /* For anonymous pages */
        workspace = (pcmcth_workspace *)widget->user_data;
    }

    return workspace;
}

