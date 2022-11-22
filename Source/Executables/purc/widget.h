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

typedef enum {
    WSP_WIDGET_TYPE_NONE  = 0,       /* not-existing */
    WSP_WIDGET_TYPE_ROOT,            /* a virtual root window */
    WSP_WIDGET_TYPE_PLAINWINDOW,     /* a plain main window */
    WSP_WIDGET_TYPE_TABBEDWINDOW,    /* a tabbed main window */
    WSP_WIDGET_TYPE_CONTAINER,       /* A layout container widget */
    WSP_WIDGET_TYPE_PANEHOST,        /* the container of paned pages */
    WSP_WIDGET_TYPE_TABHOST,         /* the container of tabbed pages */
    WSP_WIDGET_TYPE_PANEDPAGE,       /* a paned page */
    WSP_WIDGET_TYPE_TABBEDPAGE,      /* a tabbed page */
} foil_widget_type_t;

struct foil_widget;
typedef struct foil_widget foil_widget;

typedef int  (*foil_widget_create_cb)(foil_widget *);
typedef int  (*foil_widget_moved_cb)(foil_widget *);
typedef int  (*foil_widget_resized_cb)(foil_widget *);
typedef void (*foil_widget_destroy_cb)(foil_widget *);

struct foil_widget {
    struct foil_widget *parent;
    struct foil_widget *first;
    struct foil_widget *last;

    struct foil_widget *prev;
    struct foil_widget *next;

    foil_widget_type_t  type;
    foil_rect           rect;

    char               *name;
    char               *title;

    foil_widget_create_cb   on_create;
    foil_widget_moved_cb    on_moved;
    foil_widget_resized_cb  on_resized;
    foil_widget_destroy_cb  on_destroy;

    pcmcth_page         page;

    void               *user_data;
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
    unsigned    w, h;

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

#define foil_page_to_widget(page_ptr)   \
    container_of(page_ptr, foil_widget, page)

#ifdef __cplusplus
extern "C" {
#endif

foil_widget *foil_widget_new(foil_widget_type_t type,
        const char *name, const char *title);

void foil_widget_append_child(foil_widget *to, foil_widget *widget);
void foil_widget_prepend_child(foil_widget *to, foil_widget *widget);
void foil_widget_insert_before(foil_widget *to, foil_widget *widget);
void foil_widget_insert_after(foil_widget *to, foil_widget *widget);
void foil_widget_remove_from_tree(foil_widget *widget);

void foil_widget_delete(foil_widget *widget);
void foil_widget_delete_deep(foil_widget *widget);


#ifdef __cplusplus
}
#endif

#endif  /* purc_foil_widget_h */

