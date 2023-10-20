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

seeker_widget *seeker_widget_new(seeker_widget_type_k type,
        const char *name, const char *title)
{
    seeker_widget *widget = calloc(1, sizeof(*widget));

    if (widget) {
        widget->type = type;
        widget->name = name ? strdup(name) : NULL;
        widget->title = title ? strdup(title) : NULL;

        if (widget->name == NULL || widget->title == NULL)
            goto failed;
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

void seeker_widget_append_child(seeker_widget *to, seeker_widget *widget)
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

void seeker_widget_prepend_child(seeker_widget *to, seeker_widget *widget)
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

void seeker_widget_insert_before(seeker_widget *to, seeker_widget *widget)
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

void seeker_widget_insert_after(seeker_widget *to, seeker_widget *widget)
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

void seeker_widget_remove_from_tree(seeker_widget *widget)
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

void seeker_widget_delete(seeker_widget *widget)
{
    if (widget->data)
        free(widget->data);
    seeker_widget_remove_from_tree(widget);
    seeker_page_content_cleanup(&widget->page);
    if (widget->name)
        free(widget->name);
    if (widget->title)
        free(widget->title);
    free(widget);
}

void seeker_widget_delete_deep(seeker_widget *root)
{
    seeker_widget *tmp;
    seeker_widget *widget = root;

    while (widget) {
        if (widget->first) {
            widget = widget->first;
        }
        else {
            while (widget != root && widget->next == NULL) {
                tmp = widget->parent;
                seeker_widget_delete(widget);
                widget = tmp;
            }

            if (widget == root) {
                seeker_widget_delete(widget);
                break;
            }

            tmp = widget->next;
            seeker_widget_delete(widget);
            widget = tmp;
        }
    }
}

seeker_widget *seeker_widget_get_root(seeker_widget *widget)
{
    seeker_widget *parent = widget->parent;
    if (parent == NULL) /* an orphan widget */
        return parent;

    while (parent->parent) {
        parent = parent->parent;
    }

    return parent;
}

