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

#include "widget.h"
#include "workspace.h"

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

        widget->vx = 0;
        widget->vy = 0;
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
    while (parent->parent) {
        parent = parent->parent;
    }

    return parent;
}

void foil_widget_expose(foil_widget *widget)
{
    foil_widget *root = foil_widget_get_root(widget);
    pcmcth_workspace *workspace = (pcmcth_workspace *)root->user_data;
    if (workspace->rdr->impl->term_mode == FOIL_TERM_MODE_LINE) {
    }
    else {
        // TODO
    }
}

