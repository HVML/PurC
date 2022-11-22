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

#include <assert.h>

foil_widget *foil_widget_new(foil_widget_type_t type, const char *name,
        const char *title)
{
    foil_widget *widget = calloc(1, sizeof(*widget));

    if (widget) {
        widget->type = type;
        widget->name = name ? strdup(name) : NULL;
        widget->title = title ? strdup(title) : NULL;
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

