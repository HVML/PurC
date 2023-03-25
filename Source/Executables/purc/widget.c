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

#undef NDEBUG

#include "widget.h"
#include "page.h"
#include "workspace.h"
#include "timer.h"

#include "purc/purc-utils.h"
#include <assert.h>

static struct foil_widget_ops *get_widget_ops(foil_widget_type_k type);

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

        widget->vx = 0;
        widget->vy = 0;
        widget->vw = foil_rect_width(&widget->client_rc);
        widget->vh = 0;

        widget->ops = get_widget_ops(type);
        assert(widget->ops);

        if (widget->ops->init) {
            if (widget->ops->init(widget))
                goto failed;
        }
    }

    return widget;

failed:
    if (widget->name)
        free(widget->name);
    if (widget->title)
        free(widget->title);
    free(widget);
    return NULL;
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
    if (widget->ops->clean)
        widget->ops->clean(widget);
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

void foil_widget_expose(foil_widget *widget)
{
    if (widget->ops->expose)
        widget->ops->expose(widget);
}

purc_variant_t foil_widget_call_method(foil_widget *widget,
        const char *method, purc_variant_t arg)
{
    if (strcmp(method, "dumpContents") == 0) {
        const char *fname = purc_variant_get_string_const(arg);

        int retv = -1;
        if (fname && widget->ops->dump) {
            retv = widget->ops->dump(widget, fname);
        }

        if (retv)
            goto failed;
    }

    return purc_variant_make_boolean(true);

failed:
    return PURC_VARIANT_INVALID;
}

static const char *escaped_bgc[] = {
    "\x1b[49m",  // Default
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
    "\x1b[39m",  // Default
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

static unsigned char *
escape_bgc(char *buf, const struct pcmcth_page *page, int bgc)
{
    if (bgc & FOIL_DEFCLR_MASK) {
        strcpy(buf, escaped_bgc[0]);
    }
    else {
        switch (page->color_mode) {
        case FOIL_TTY_COLOR_STD_16C:
            strcpy(buf, escaped_bgc[bgc + 1]);
            break;

        case FOIL_TTY_COLOR_XTERM_256C:
            sprintf(buf, "\033[48;5;%dm", bgc);
            break;

        case FOIL_TTY_COLOR_TRUE_COLOR:
            sprintf(buf, "\033[48;2;%d;%d;%dm",
                        (int)((uint8_t)((uint32_t)bgc >> 16)),
                        (int)((uint8_t)((uint32_t)bgc >> 8)),
                        (int)((uint8_t)((uint32_t)bgc)));
            break;

        default:
            assert(0);
            break;
        }
    }

    return (unsigned char *)buf;
}

static unsigned char *
escape_fgc(char *buf, const struct pcmcth_page *page, int fgc)
{
    if (fgc & FOIL_DEFCLR_MASK) {
        strcpy(buf, escaped_fgc[0]);
    }
    else {
        switch (page->color_mode) {
        case FOIL_TTY_COLOR_STD_16C:
            strcpy(buf, escaped_fgc[fgc + 1]);
            break;


        case FOIL_TTY_COLOR_XTERM_256C:
            sprintf(buf, "\033[38;5;%dm", fgc);
            break;

        case FOIL_TTY_COLOR_TRUE_COLOR:
            sprintf(buf, "\033[38;2;%d;%d;%dm",
                        (int)((uint8_t)((uint32_t)fgc >> 16)),
                        (int)((uint8_t)((uint32_t)fgc >> 8)),
                        (int)((uint8_t)((uint32_t)fgc)));
            break;

        default:
            assert(0);
            break;
        }
    }

    return (unsigned char *)buf;
}

static char *
make_escape_string_line_mode(const struct pcmcth_page *page,
        const struct foil_tty_cell *cell, int n)
{
    struct pcutils_mystring mystr = { NULL, 0, 0 };

    int old_bgc = -1;
    int old_fgc = -1;

    char buf[64];

    unsigned count = 0;
    for (int i = 0; i < n; i++) {
        if (cell->latter_half) {
            cell++;
            continue;
        }

        if (i == 0) {
            pcutils_mystring_append_mchar(&mystr,
                    escape_bgc(buf, page, cell->bgc), 0);
            pcutils_mystring_append_mchar(&mystr,
                    escape_fgc(buf, page, cell->fgc), 0);

            old_bgc = cell->bgc;
            old_fgc = cell->fgc;
        }
        else {
            if (old_bgc != cell->bgc) {
                pcutils_mystring_append_mchar(&mystr,
                        escape_bgc(buf, page, cell->bgc), 0);
                old_bgc = cell->bgc;
            }

            if (old_fgc != cell->fgc) {
                pcutils_mystring_append_mchar(&mystr,
                        escape_fgc(buf, page, cell->fgc), 0);
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

    LOG_DEBUG("dirty rect: %d, %d, %d, %d\n",
            page->dirty_rect.left, page->dirty_rect.top,
            page->dirty_rect.right, page->dirty_rect.bottom);

    LOG_DEBUG("client rect: %d, %d, %d, %d\n",
            widget->client_rc.left, widget->client_rc.top,
            widget->client_rc.right, widget->client_rc.bottom);

    foil_rect viewport, dirty;
    foil_rect_set(&viewport, widget->vx, widget->vy,
            widget->vx + widget->vw, widget->vy + widget->vh);

    if (!foil_rect_intersect(&dirty, &page->dirty_rect, &viewport)) {
        return;
    }

    char buf[64];
    int w = foil_rect_width(&dirty);
    for (int y = dirty.top; y < dirty.bottom; y++) {
        int x = dirty.left;

        int rel_col = x - widget->vx;
        int rel_row = widget->vh - y + widget->vy;
        if (rel_row > widget->vh)
            continue;

        struct foil_tty_cell *cell = page->cells[y] + x;
        char *escaped_str = make_escape_string_line_mode(page, cell, w);

        LOG_DEBUG("move curosr %d rows up and %d colunms right\n",
                rel_row, rel_col);

        /* restore curosr and move cursor rel_row up lines,
           move curosr rel_col right columns */
        snprintf(buf, sizeof(buf), "\0338\x1b[%dA\x1b[%dC", rel_row, rel_col + 1);
        fputs(buf, stdout);
        fputs(escaped_str, stdout);
        free(escaped_str);
    }

#if 0
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH\x1b[39m\x1b[49m",
            widget->vy + page->rows + 1, 0);
    fputs(buf, stdout);
#else
    /* restore cursor position (bottom-left corner of the page). */
    fputs("\0338", stdout);
#endif
}

static void adjust_viewport_line_mode(foil_widget *widget)
{
    int widget_rows = foil_rect_height(&widget->client_rc);
    int rows = widget->vh;

    if (rows < widget->page.rows) {
        while (rows < widget->page.rows) {
            int vy = widget_rows - widget->page.rows + rows;
            /* Writes the contents of lines off the scrolled screen. */
            if (vy < 0) {
                struct foil_tty_cell *cell = widget->page.cells[rows];
                char *escaped_str = make_escape_string_line_mode(
                        &widget->page, cell, widget->page.cols);
                fputs(escaped_str, stdout);
            }

            fputs("\n", stdout);
            rows++;
        }
        widget->vh = rows;

        if (widget->page.rows > widget_rows) {
            widget->vy = widget->page.rows - widget_rows;
            widget->vh = widget_rows;
        }

        LOG_DEBUG("widget viewport (rows: %d): %d, %d, %d, %d\n",
                rows,
                widget->vx, widget->vy, widget->vw, widget->vh);

        /* Save cursor position */
        fputs("\0337", stdout);
        fflush(stdout);
    }
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

static void expose_on_screen(foil_widget *widget)
{
    foil_widget *root = foil_widget_get_root(widget);
    pcmcth_workspace *workspace = (pcmcth_workspace *)root->user_data;
    if (workspace->rdr->impl->term_mode == FOIL_TERM_MODE_LINE) {
        adjust_viewport_line_mode(widget);

        pcmcth_renderer *rdr = foil_get_renderer();

        if (foil_timer_find(rdr,
                    TIMER_FLUSHER_NAME, flush_contents, widget) == NULL) {
            foil_timer_new(rdr, TIMER_FLUSHER_NAME, flush_contents,
                    TIMER_FLUSHER_INTERVAL, widget);
        }
    }
    else {
        // TODO
    }
}

struct off_screen_lines {
    int cols, rows;
    char *lines[];
};

static int init_off_screen(foil_widget *widget)
{
    int rows = foil_rect_height(&widget->rect);
    char **lines = calloc(rows, sizeof(char *));
    if (lines) {
        widget->data = lines;
        return 0;
    }

    return -1;
}

static void expose_off_screen(foil_widget *widget)
{
    pcmcth_page *page = &widget->page;

    if (foil_rect_is_empty(&page->dirty_rect)) {
        return;
    }

    /* always make the last line visible */
    if (page->rows > widget->vh) {
        int cli_height = foil_rect_height(&widget->client_rc);
        widget->vh = page->rows;
        if (widget->vh > cli_height)
            widget->vh = cli_height;
        widget->vy = page->rows - cli_height;
    }

    foil_rect viewport, dirty;
    foil_rect_set(&viewport, widget->vx, widget->vy,
            widget->vx + widget->vw, widget->vy + widget->vh);

    if (!foil_rect_intersect(&dirty, &page->dirty_rect, &viewport)) {
        return;
    }

    /* handle border of widget here */
    char **lines = widget->data;
    int w = foil_rect_width(&widget->client_rc);
    for (int y = dirty.top; y < dirty.bottom; y++) {
        int rel_row = y - widget->vy;
        if (rel_row > widget->vh)
            continue;

        if (lines[y])
            free(lines[y]);
        lines[y] = make_escape_string_line_mode(page, page->cells[y], w);
    }
}

static int dump_off_screen(foil_widget *widget, const char *fname)
{
    assert(widget->data);

    FILE* fp = fopen(fname, "w+");
    if (fp == NULL)
        return -1;

    char **lines = widget->data;
    int rows = foil_rect_height(&widget->rect);

    for (int y = 0; y < rows; y++) {
        if (lines[y]) {
            fputs(lines[y], fp);
        }

        /* always write a new line character */
        fputs("\n", fp);
    }

    fclose(fp);
    return 0;
}

static void clean_off_screen(foil_widget *widget)
{
    assert(widget->data);
    char **lines = widget->data;
    int rows = foil_rect_height(&widget->rect);

    for (int y = 0; y < rows; y++) {
        if (lines[y])
            free(lines[y]);
    }
}

static struct foil_widget_ops *get_widget_ops(foil_widget_type_k type)
{
    static struct foil_widget_ops ops_for_on_scrn = {
        .init = NULL,
        .expose = expose_on_screen,
        .dump = NULL,
        .clean = NULL,
    };

    static struct foil_widget_ops ops_for_off_scrn = {
        .init = init_off_screen,
        .expose = expose_off_screen,
        .dump = dump_off_screen,
        .clean = clean_off_screen,
    };

    if (type == WSP_WIDGET_TYPE_OFFSCREEN) {
        return &ops_for_off_scrn;
    }

    return &ops_for_on_scrn;
}


