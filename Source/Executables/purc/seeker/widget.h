/*
 * @file widget.h
 * @author Vincent Wei
 * @date 2023/10/20
 * @brief The global definitions for Seeker widget.
 *
 * Copyright (C) 2023 FMSoft <https://www.fmsoft.cn>
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

#ifndef purc_seeker_widget_h
#define purc_seeker_widget_h

#include "seeker.h"
#include "page.h"

#define WSP_WIDGET_CLASS_OFF_SCREEN     "-off-screen"

typedef enum {
    WSP_WIDGET_TYPE_ROOT = 0,       /* a virtual root window */
    WSP_WIDGET_TYPE_PLAINWINDOW,    /* a plain main window */
    WSP_WIDGET_TYPE_TABBEDWINDOW,   /* a tabbed main window */
    WSP_WIDGET_TYPE_CONTAINER,      /* A layout container widget */
    WSP_WIDGET_TYPE_PANEHOST,       /* the container of paned pages */
    WSP_WIDGET_TYPE_TABHOST,        /* the container of tabbed pages */
    WSP_WIDGET_TYPE_PANEDPAGE,      /* a paned page */
    WSP_WIDGET_TYPE_TABBEDPAGE,     /* a tabbed page */
} seeker_widget_type_k;

struct seeker_widget;
typedef struct seeker_widget seeker_widget;

struct seeker_widget {
    pcmcth_page         page;

    struct seeker_widget *parent;
    struct seeker_widget *first;
    struct seeker_widget *last;

    struct seeker_widget *prev;
    struct seeker_widget *next;

    seeker_widget_type_k  type;

    char               *name;
    char               *title;
    void               *user_data;

    void               *data;
};

#define WSP_WIDGET_FLAG_NAME      0x00000001
#define WSP_WIDGET_FLAG_TITLE     0x00000002
#define WSP_WIDGET_FLAG_GEOMETRY  0x00000004
#define WSP_WIDGET_FLAG_TOOLKIT   0x00000008

struct seeker_widget_info {
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

seeker_widget *seeker_widget_new(seeker_widget_type_k type,
        const char *name, const char *title);

void seeker_widget_append_child(seeker_widget *to, seeker_widget *widget);
void seeker_widget_prepend_child(seeker_widget *to, seeker_widget *widget);
void seeker_widget_insert_before(seeker_widget *to, seeker_widget *widget);
void seeker_widget_insert_after(seeker_widget *to, seeker_widget *widget);
void seeker_widget_remove_from_tree(seeker_widget *widget);

void seeker_widget_delete(seeker_widget *widget);
void seeker_widget_delete_deep(seeker_widget *widget);

seeker_widget *seeker_widget_get_root(seeker_widget *widget);

#ifdef __cplusplus
}
#endif

static inline seeker_widget *seeker_widget_from_page(pcmcth_page *page) {
    return container_of(page, seeker_widget, page);
}

#endif  /* purc_seeker_widget_h */

