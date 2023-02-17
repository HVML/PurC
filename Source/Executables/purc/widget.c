/*
** @file widget.c
** @author Vincent Wei
** @date 2022/10/17
** @brief The implementation of widget.
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

#include "widget.h"
#include "workspace.h"
#include "timer.h"

#include "purc/purc-utils.h"
#include <assert.h>

foil_widget *foil_widget_new(foil_widget_type_k type,
        foil_widget_border_k border,
        const char *name, const char *title, const foil_rect *rect)
{
    assert(rect->right > rect->left && rect->bottom > rect->top);

    foil_widget *widget = calloc(1, sizeof(*widget));

    if (widget) {
        widget->type = type;
        widget->border = border;
        widget->name = name ? strdup(name) : NULL;
        widget->title = title ? strdup(title) : NULL;
        widget->rect = *rect;

        if (border == WSP_WIDGET_BORDER_NONE) {
            widget->client_rc.left = 0;
            widget->client_rc.right = 0;
            widget->client_rc.right = foil_rect_width(rect);
            widget->client_rc.bottom = foil_rect_height(rect);
        }
        else if (border == WSP_WIDGET_BORDER_SHADOW) {
            widget->client_rc.left = 0;
            widget->client_rc.right = 0;
            widget->client_rc.right = foil_rect_width(rect) - 1;
            widget->client_rc.bottom = foil_rect_height(rect) - 1;
        }
        else {
            widget->client_rc.left = 1;
            widget->client_rc.right = 1;
            widget->client_rc.right = foil_rect_width(rect) - 1;
            widget->client_rc.bottom = foil_rect_height(rect) - 1;
        }

        widget->vx = widget->client_rc.left;
        widget->vy = 0;
        widget->vw = foil_rect_width(&widget->client_rc);
        widget->vh = 0;
    }

    return widget;
}

void foil_widget_append_child(foil_widget *to, foil_widget *widget)
{
    if (to->last != NULL) {
        to->last->next = widget;
    }
    else {
        to->first = widget;
    }

    widget->parent = to;
    widget->next = NULL;
    widget->prev = to->last;

    to->last = widget;
}

void foil_widget_prepend_child(foil_widget *to, foil_widget *widget)
{
    if (to->first != NULL) {
        to->first->prev = widget;
    }
    else {
        to->last = widget;
    }

    widget->parent = to;
    widget->next = to->first;
    widget->prev = NULL;

    to->first = widget;
}

void foil_widget_insert_before(foil_widget *to, foil_widget *widget)
{
    if (to->prev != NULL) {
        to->prev->next = widget;
    }
    else {
        if (to->parent != NULL) {
            to->parent->first = widget;
        }
    }

    widget->parent = to->parent;
    widget->next = to;
    widget->prev = to->prev;

    to->prev = widget;
}

void foil_widget_insert_after(foil_widget *to, foil_widget *widget)
{
    if (to->next != NULL) {
        to->next->prev = widget;
    }
    else {
        if (to->parent != NULL) {
            to->parent->last = widget;
        }
    }

    widget->parent = to->parent;
    widget->next = to->next;
    widget->prev = to;
    to->next = widget;
}

void foil_widget_remove_from_tree(foil_widget *widget)
{
    if (widget->parent != NULL) {
        if (widget->parent->first == widget) {
            widget->parent->first = widget->next;
        }

        if (widget->parent->last == widget) {
            widget->parent->last = widget->prev;
        }
    }

    if (widget->next != NULL) {
        widget->next->prev = widget->prev;
    }

    if (widget->prev != NULL) {
        widget->prev->next = widget->next;
    }

    widget->parent = NULL;
    widget->next = NULL;
    widget->prev = NULL;
}

void foil_widget_delete(foil_widget *widget)
{
    foil_widget_remove_from_tree(widget);
    foil_page_content_cleanup(&widget->page);
    if (widget->name)
        free(widget->name);
    if (widget->title)
        free(widget->title);
    free(widget);
}

void foil_widget_delete_deep(foil_widget *root)
{
    foil_widget *tmp;
    foil_widget *widget = root;

    while (widget) {
        if (widget->first) {
            widget = widget->first;
        }
        else {
            while (widget != root && widget->next == NULL) {
                tmp = widget->parent;
                foil_widget_delete(widget);
                widget = tmp;
            }

            if (widget == root) {
                foil_widget_delete(widget);
                break;
            }

            tmp = widget->next;
            foil_widget_delete(widget);
            widget = tmp;
        }
    }
}

foil_widget *foil_widget_get_root(foil_widget *widget)
{
    foil_widget *parent = widget->parent;
    if (parent == NULL) /* an orphan widget */
        return parent;

    while (parent->parent) {
        parent = parent->parent;
    }

    return parent;
}

static void adjust_viewport_line_mode(foil_widget *widget)
{
    int widget_rows = foil_rect_height(&widget->client_rc);
    int rows = widget->vh;
    while (rows < widget->page.rows && rows < widget_rows) {
        fputs("\n", stdout);
        rows++;
    }

    widget->vh = rows;
    widget->vy = foil_rect_height(&widget->client_rc) - widget->page.rows;
}

static const char *escaped_bgc[] = {
    "\x1b[40m",  // FOIL_STD_COLOR_BLACK
    "\x1b[41m",  // FOIL_STD_COLOR_DARK_RED
    "\x1b[42m",  // FOIL_STD_COLOR_DARK_GREEN
    "\x1b[43m",  // FOIL_STD_COLOR_DARK_YELLOW
    "\x1b[44m",  // FOIL_STD_COLOR_DARK_BLUE
    "\x1b[45m",  // FOIL_STD_COLOR_DARK_MAGENTA
    "\x1b[46m",  // FOIL_STD_COLOR_DARK_CYAN
    "\x1b[47m",  // FOIL_STD_COLOR_GRAY
    "\x1b[100m", // FOIL_STD_COLOR_DARK_GRAY
    "\x1b[101m", // FOIL_STD_COLOR_RED
    "\x1b[102m", // FOIL_STD_COLOR_GREEN
    "\x1b[103m", // FOIL_STD_COLOR_YELLOW
    "\x1b[104m", // FOIL_STD_COLOR_BLUE
    "\x1b[105m", // FOIL_STD_COLOR_MAGENTA
    "\x1b[106m", // FOIL_STD_COLOR_CYAN
    "\x1b[107m", // FOIL_STD_COLOR_WHITE
};

static const char *escaped_fgc[] = {
    "\x1b[30m",  // FOIL_STD_COLOR_BLACK
    "\x1b[31m",  // FOIL_STD_COLOR_DARK_RED
    "\x1b[32m",  // FOIL_STD_COLOR_DARK_GREEN
    "\x1b[33m",  // FOIL_STD_COLOR_DARK_YELLOW
    "\x1b[34m",  // FOIL_STD_COLOR_DARK_BLUE
    "\x1b[35m",  // FOIL_STD_COLOR_DARK_MAGENTA
    "\x1b[36m",  // FOIL_STD_COLOR_DARK_CYAN
    "\x1b[37m",  // FOIL_STD_COLOR_GRAY
    "\x1b[90m",  // FOIL_STD_COLOR_DARK_GRAY
    "\x1b[91m",  // FOIL_STD_COLOR_RED
    "\x1b[92m",  // FOIL_STD_COLOR_GREEN
    "\x1b[93m",  // FOIL_STD_COLOR_YELLOW
    "\x1b[94m",  // FOIL_STD_COLOR_BLUE
    "\x1b[95m",  // FOIL_STD_COLOR_MAGENTA
    "\x1b[96m",  // FOIL_STD_COLOR_CYAN
    "\x1b[97m",  // FOIL_STD_COLOR_WHITE
};

static char *
make_escape_string_line_mode(const struct foil_tty_cell *cell, int n)
{
    struct pcutils_mystring mystr = { NULL, 0, 0 };

    uint8_t old_bgc = 255;
    uint8_t old_fgc = 255;

    unsigned count = 0;
    for (int i = 0; i < n; i++) {
        if (cell->latter_half) {
            cell++;
            continue;
        }

        if (i == 0) {
            pcutils_mystring_append_mchar(&mystr,
                    (const unsigned char *)escaped_bgc[cell->bgc], 0);
            pcutils_mystring_append_mchar(&mystr,
                    (const unsigned char *)escaped_fgc[cell->fgc], 0);

            old_bgc = cell->bgc;
            old_fgc = cell->fgc;
        }
        else {
            if (old_bgc != cell->bgc) {
                pcutils_mystring_append_mchar(&mystr,
                        (const unsigned char*)escaped_bgc[cell->bgc], 0);
                old_bgc = cell->bgc;
            }

            if (old_fgc != cell->fgc) {
                pcutils_mystring_append_mchar(&mystr,
                        (const unsigned char*)escaped_fgc[cell->fgc], 0);
                old_fgc = cell->fgc;
            }
        }

        pcutils_mystring_append_uchar(&mystr, cell->uc, 1);
        cell++;
        count++;
    }

    pcutils_mystring_done(&mystr);
    return mystr.buff;
}

static void print_dirty_page_area_line_mode(foil_widget *widget)
{
    pcmcth_page *page = &widget->page;

    if (foil_rect_is_empty(&page->dirty_rect)) {
        return;
    }

    int w = foil_rect_width(&page->dirty_rect);
    LOG_DEBUG("dirty rect: %d, %d, %d, %d\n",
            page->dirty_rect.left, page->dirty_rect.top,
            page->dirty_rect.right, page->dirty_rect.bottom);

    LOG_DEBUG("client rect: %d, %d, %d, %d\n",
            widget->client_rc.left, widget->client_rc.top,
            widget->client_rc.right, widget->client_rc.bottom);

    char buf[64];
    for (int y = page->dirty_rect.top; y < page->dirty_rect.bottom; y++) {
        int x = page->dirty_rect.left;

        int screen_col = x + widget->vx;
        int screen_row = y + widget->vy;
        if (screen_row < 0)
            continue;

        struct foil_tty_cell *cell = page->cells[y] + x;
        char *escaped_str = make_escape_string_line_mode(cell, w);

        LOG_DEBUG("screen col, row: %d, %d\n",
                screen_col, screen_row);
        assert(screen_col >= 0 &&
                screen_col < foil_rect_width(&widget->client_rc));
        assert(screen_row >= 0 &&
                screen_row < foil_rect_height(&widget->client_rc));

        snprintf(buf, sizeof(buf), "\x1b[%d;%dH", screen_row, screen_col);
        fputs(buf, stdout);
        fputs(escaped_str, stdout);
        free(escaped_str);
    }

    snprintf(buf, sizeof(buf), "\x1b[%d;%dH\x1b[39m\x1b[49m",
            widget->vy + page->rows + 1, 0);
    fputs(buf, stdout);
}

#define TIMER_FLUSHER_NAME          "flusher"
#define TIMER_FLUSHER_INTERVAL      20  // 50 fps

static int flush_contents(const char *name, void *ctxt)
{
    (void)name;
    foil_widget *widget = ctxt;
    print_dirty_page_area_line_mode(widget);
    fflush(stdout);

    foil_rect_empty(&widget->page.dirty_rect);
    return -1;
}

void foil_widget_expose(foil_widget *widget)
{
    foil_widget *root = foil_widget_get_root(widget);
    pcmcth_workspace *workspace = (pcmcth_workspace *)root->user_data;
    if (workspace->rdr->impl->term_mode == FOIL_TERM_MODE_LINE) {
        adjust_viewport_line_mode(widget);

        pcmcth_renderer *rdr = foil_get_renderer();

        if (foil_timer_find(rdr, TIMER_FLUSHER_NAME, flush_contents) == NULL) {
            foil_timer_new(rdr, TIMER_FLUSHER_NAME, flush_contents,
                    TIMER_FLUSHER_INTERVAL, widget);
        }
    }
    else {
        // TODO
    }
}

