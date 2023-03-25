/*
 * @file widget.h
 * @author Vincent Wei
 * @date 2022/10/02
 * @brief The global definitions for terminal widget.
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

#ifndef purc_foil_widget_h
#define purc_foil_widget_h

#include "foil.h"
#include "page.h"
#include "region/rect.h"

#define WSP_WIDGET_CLASS_OFF_SCREEN     "-off-screen"

typedef enum {
    WSP_WIDGET_TYPE_OFFSCREEN = 0,  /* an off-screen plain window */
    WSP_WIDGET_TYPE_ROOT,           /* a virtual root window */
    WSP_WIDGET_TYPE_PLAINWINDOW,    /* a plain main window */
    WSP_WIDGET_TYPE_TABBEDWINDOW,   /* a tabbed main window */
    WSP_WIDGET_TYPE_CONTAINER,      /* A layout container widget */
    WSP_WIDGET_TYPE_PANEHOST,       /* the container of paned pages */
    WSP_WIDGET_TYPE_TABHOST,        /* the container of tabbed pages */
    WSP_WIDGET_TYPE_PANEDPAGE,      /* a paned page */
    WSP_WIDGET_TYPE_TABBEDPAGE,     /* a tabbed page */
} foil_widget_type_k;

typedef enum {
    WSP_WIDGET_BORDER_NONE = 0,                 /* no border */
    WSP_WIDGET_BORDER_LIGHT_LINES,              /* light lines */
    WSP_WIDGET_BORDER_HEAVY_LINES,              /* heavy lines */
    WSP_WIDGET_BORDER_DOUBLE_LINES,             /* double lines */
    WSP_WIDGET_BORDER_LIGHT_LINES_WITH_ARCS,    /* light lines with arcs */
    WSP_WIDGET_BORDER_SHADOW,                   /* shadows */
} foil_widget_border_k;

struct foil_widget;
typedef struct foil_widget foil_widget;

typedef int  (*foil_widget_create_cb)(foil_widget *);
typedef int  (*foil_widget_moved_cb)(foil_widget *);
typedef int  (*foil_widget_resized_cb)(foil_widget *);
typedef void (*foil_widget_destroy_cb)(foil_widget *);

struct foil_widget_ops {
    int   (*init)(foil_widget *);
    void  (*expose)(foil_widget *);
    int   (*dump)(foil_widget *, const char *);
    void  (*clean)(foil_widget *);
};

struct foil_widget {
    pcmcth_page         page;

    struct foil_widget *parent;
    struct foil_widget *first;
    struct foil_widget *last;

    struct foil_widget *prev;
    struct foil_widget *next;

    foil_widget_type_k  type;
    foil_widget_border_k border;

    /* the rectangle of this widget in parent */
    foil_rect           rect;

    /* the client rectangle in this widget */
    foil_rect           client_rc;

    /* the coordinate of the page origin in viewport */
    int                 vx, vy;
    /* the columns and rows of the viewport */
    int                 vw, vh;

    char               *name;
    char               *title;
    void               *user_data;

    void                   *data;
    struct foil_widget_ops *ops;
};

#define WSP_WIDGET_FLAG_NAME      0x00000001
#define WSP_WIDGET_FLAG_TITLE     0x00000002
#define WSP_WIDGET_FLAG_GEOMETRY  0x00000004
#define WSP_WIDGET_FLAG_TOOLKIT   0x00000008

struct foil_widget_info {
    unsigned int flags;

    const char *name;
    const char *title;
    const char *klass;

    /* geometry */
    int         x, y;
    int         w, h;

    /* other styles */
    const char *backgroundColor;
    bool        darkMode;
    bool        fullScreen;
    bool        withToolbar;

#if 0
    int         ml, mt, mr, mb; /* margins */
    int         pl, pt, pr, pb; /* paddings */
    float       bl, bt, br, bb; /* borders */
    float       brlt, brtr, brrb, brbl; /* border radius */

    float       opacity;
#endif
};

#ifndef container_of
#define container_of(ptr, type, member)                                 \
    ({                                                                  \
        const __typeof__(((type *) NULL)->member) *__mptr = (ptr);      \
        (type *) (void *) ((char *) __mptr - offsetof(type, member));   \
    })
#endif

#ifdef __cplusplus
extern "C" {
#endif

foil_widget *foil_widget_new(foil_widget_type_k type,
        foil_widget_border_k border,
        const char *name, const char *title, const foil_rect *rc);

void foil_widget_append_child(foil_widget *to, foil_widget *widget);
void foil_widget_prepend_child(foil_widget *to, foil_widget *widget);
void foil_widget_insert_before(foil_widget *to, foil_widget *widget);
void foil_widget_insert_after(foil_widget *to, foil_widget *widget);
void foil_widget_remove_from_tree(foil_widget *widget);

void foil_widget_delete(foil_widget *widget);
void foil_widget_delete_deep(foil_widget *widget);

foil_widget *foil_widget_get_root(foil_widget *widget);

void foil_widget_expose(foil_widget *widget);
int foil_widget_dump(foil_widget *widget, const char *fname);

#ifdef __cplusplus
}
#endif

static inline foil_widget *foil_widget_from_page(pcmcth_page *page) {
    return container_of(page, foil_widget, page);
}

static inline int foil_widget_width(const foil_widget *widget) {
    return foil_rect_width(&widget->rect);
}

static inline int foil_widget_height(const foil_widget *widget) {
    return foil_rect_width(&widget->rect);
}

static inline int foil_widget_client_width(const foil_widget *widget) {
    return foil_rect_width(&widget->client_rc);
}

static inline int foil_widget_client_height(const foil_widget *widget)
{
    return foil_rect_height(&widget->client_rc);
}

static inline int foil_widget_viewport_x(const foil_widget *widget) {
    return widget->vx;
}

static inline int foil_widget_viewport_y(const foil_widget *widget) {
    return widget->vy;
}

#endif  /* purc_foil_widget_h */

