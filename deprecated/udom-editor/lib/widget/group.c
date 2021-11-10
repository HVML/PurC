/*
   Widget group features module for the Midnight Commander

   Copyright (C) 2020-2021
   The Free Software Foundation, Inc.

   Written by:
   Andrew Borodin <aborodin@vmail.ru>, 2020

   This file is part of the Midnight Commander.

   The Midnight Commander is free software: you can redistribute it
   and/or modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, either version 3 of the License,
   or (at your option) any later version.

   The Midnight Commander is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/** \file group.c
 *  \brief Source: widget group features module
 */

#include <config.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "lib/global.h"

#include "lib/tty/key.h"        /* ALT() */

#include "lib/widget.h"

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/* Control widget positions in a group */
typedef struct
{
    int shift_x;
    int scale_x;
    int shift_y;
    int scale_y;
} widget_shift_scale_t;

typedef struct
{
    widget_state_t state;
    gboolean enable;
} widget_state_info_t;

/*** file scope variables ************************************************************************/

/* --------------------------------------------------------------------------------------------- */
/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

static void
group_widget_init (void *data, void *user_data)
{
    (void) user_data;

    send_message (WIDGET (data), NULL, MSG_INIT, 0, NULL);
}

/* --------------------------------------------------------------------------------------------- */

static GList *
group_get_next_or_prev_of (GList * list, gboolean next)
{
    GList *l = NULL;

    if (list != NULL)
    {
        WGroup *owner = WIDGET (list->data)->owner;

        if (owner != NULL)
        {
            if (next)
            {
                l = g_list_next (list);
                if (l == NULL)
                    l = owner->widgets;
            }
            else
            {
                l = g_list_previous (list);
                if (l == NULL)
                    l = g_list_last (owner->widgets);
            }
        }
    }

    return l;
}

/* --------------------------------------------------------------------------------------------- */

static void
group_select_next_or_prev (WGroup * g, gboolean next)
{
    if (g->widgets != NULL && g->current != NULL)
    {
        GList *l = g->current;

        do
        {
            l = group_get_next_or_prev_of (l, next);
        }
        while (!widget_is_focusable (l->data) && l != g->current);

        widget_select (l->data);
    }
}

/* --------------------------------------------------------------------------------------------- */

static void
group_widget_set_state (gpointer data, gpointer user_data)
{
    widget_state_info_t *state = (widget_state_info_t *) user_data;

    widget_set_state (WIDGET (data), state->state, state->enable);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Send broadcast message to all widgets in the group that have specified options.
 *
 * @param g WGroup object
 * @param msg message sent to widgets
 * @param reverse if TRUE, send message in reverse order, FALSE -- in direct one.
 * @param options if WOP_DEFAULT, the message is sent to all widgets. Else message is sent to widgets
 *                that have specified options.
 */

static void
group_send_broadcast_msg_custom (WGroup * g, widget_msg_t msg, gboolean reverse,
                                 widget_options_t options)
{
    GList *p, *first;

    if (g->widgets == NULL)
        return;

    if (g->current == NULL)
        g->current = g->widgets;

    p = group_get_next_or_prev_of (g->current, !reverse);
    first = p;

    do
    {
        Widget *w = WIDGET (p->data);

        p = group_get_next_or_prev_of (p, !reverse);

        if (options == WOP_DEFAULT || (options & w->options) != 0)
            /* special case: don't draw invisible widgets */
            if (msg != MSG_DRAW || widget_get_state (w, WST_VISIBLE))
                send_message (w, NULL, msg, 0, NULL);
    }
    while (first != p);
}

/* --------------------------------------------------------------------------------------------- */

/**
  * Default group callback to convert group coordinates from local (relative to owner) to global
  * (relative to screen).
  *
  * @param w widget
  */

static void
group_default_make_global (Widget * w, const WRect * delta)
{
    GList *iter;

    if (delta != NULL)
    {
        /* change own coordinates */
        widget_default_make_global (w, delta);
        /* change child widget coordinates */
        for (iter = GROUP (w)->widgets; iter != NULL; iter = g_list_next (iter))
            WIDGET (iter->data)->make_global (WIDGET (iter->data), delta);
    }
    else if (w->owner != NULL)
    {
        WRect r = { WIDGET (w->owner)->y, WIDGET (w->owner)->x, 0, 0 };

        /* change own coordinates */
        widget_default_make_global (w, &r);
        /* change child widget coordinates */
        for (iter = GROUP (w)->widgets; iter != NULL; iter = g_list_next (iter))
            WIDGET (iter->data)->make_global (WIDGET (iter->data), &r);
    }
}

/* --------------------------------------------------------------------------------------------- */

/**
  * Default group callback to convert group coordinates from global (relative to screen) to local
  * (relative to owner).
  *
  * @param w widget
  */

static void
group_default_make_local (Widget * w, const WRect * delta)
{
    GList *iter;

    if (delta != NULL)
    {
        /* change own coordinates */
        widget_default_make_local (w, delta);
        /* change child widget coordinates */
        for (iter = GROUP (w)->widgets; iter != NULL; iter = g_list_next (iter))
            WIDGET (iter->data)->make_local (WIDGET (iter->data), delta);
    }
    else if (w->owner != NULL)
    {
        WRect r = { WIDGET (w->owner)->y, WIDGET (w->owner)->x, 0, 0 };

        /* change own coordinates */
        widget_default_make_local (w, &r);
        /* change child widget coordinates */
        for (iter = GROUP (w)->widgets; iter != NULL; iter = g_list_next (iter))
            WIDGET (iter->data)->make_local (WIDGET (iter->data), &r);
    }
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Default group callback function to find widget in the group.
 *
 * @param w WGroup object
 * @param what widget to find
 *
 * @return holder of @what if found, NULL otherwise
 */

static GList *
group_default_find (const Widget * w, const Widget * what)
{
    GList *w0;

    w0 = widget_default_find (w, what);
    if (w0 == NULL)
    {
        GList *iter;

        for (iter = CONST_GROUP (w)->widgets; iter != NULL; iter = g_list_next (iter))
        {
            w0 = widget_find (WIDGET (iter->data), what);
            if (w0 != NULL)
                break;
        }
    }

    return w0;
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Default group callback function to find widget in the group using widget callback.
 *
 * @param w WGroup object
 * @param cb widget callback
 *
 * @return widget object if found, NULL otherwise
 */

static Widget *
group_default_find_by_type (const Widget * w, widget_cb_fn cb)
{
    Widget *w0;

    w0 = widget_default_find_by_type (w, cb);
    if (w0 == NULL)
    {
        GList *iter;

        for (iter = CONST_GROUP (w)->widgets; iter != NULL; iter = g_list_next (iter))
        {
            w0 = widget_find_by_type (WIDGET (iter->data), cb);
            if (w0 != NULL)
                break;
        }
    }

    return w0;
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Default group callback function to find widget by widget ID in the group.
 *
 * @param w WGroup object
 * @param id widget ID
 *
 * @return widget object if widget with specified id is found in group, NULL otherwise
 */

static Widget *
group_default_find_by_id (const Widget * w, unsigned long id)
{
    Widget *w0;

    w0 = widget_default_find_by_id (w, id);
    if (w0 == NULL)
    {
        GList *iter;

        for (iter = CONST_GROUP (w)->widgets; iter != NULL; iter = g_list_next (iter))
        {
            w0 = widget_find_by_id (WIDGET (iter->data), id);
            if (w0 != NULL)
                break;
        }
    }

    return w0;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Update cursor position in the active widget of the group.
 *
 * @param g WGroup object
 *
 * @return MSG_HANDLED if cursor was updated in the specified group, MSG_NOT_HANDLED otherwise
 */

static cb_ret_t
group_update_cursor (WGroup * g)
{
    GList *p = g->current;

    if (p != NULL && widget_get_state (WIDGET (g), WST_ACTIVE))
        do
        {
            Widget *w = WIDGET (p->data);

            /* Don't use widget_is_selectable() here.
               If WOP_SELECTABLE option is not set, widget can handle mouse events.
               For example, commandl line in file manager */
            if (widget_get_options (w, WOP_WANT_CURSOR) && widget_get_state (w, WST_VISIBLE)
                && !widget_get_state (w, WST_DISABLED) && widget_update_cursor (WIDGET (p->data)))
                return MSG_HANDLED;

            p = group_get_widget_next_of (p);
        }
        while (p != g->current);

    return MSG_NOT_HANDLED;
}

/* --------------------------------------------------------------------------------------------- */

static void
group_widget_set_position (gpointer data, gpointer user_data)
{
    /* there are, mainly, 2 generally possible situations:
     * 1. control sticks to one side - it should be moved
     * 2. control sticks to two sides of one direction - it should be sized
     */

    Widget *c = WIDGET (data);
    Widget *g = WIDGET (c->owner);
    const widget_shift_scale_t *wss = (const widget_shift_scale_t *) user_data;
    WRect r = { c->y, c->x, c->lines, c->cols };

    if ((c->pos_flags & WPOS_CENTER_HORZ) != 0)
        r.x = g->x + (g->cols - c->cols) / 2;
    else if ((c->pos_flags & WPOS_KEEP_LEFT) != 0 && (c->pos_flags & WPOS_KEEP_RIGHT) != 0)
    {
        r.x += wss->shift_x;
        r.cols += wss->scale_x;
    }
    else if ((c->pos_flags & WPOS_KEEP_LEFT) != 0)
        r.x += wss->shift_x;
    else if ((c->pos_flags & WPOS_KEEP_RIGHT) != 0)
        r.x += wss->shift_x + wss->scale_x;

    if ((c->pos_flags & WPOS_CENTER_VERT) != 0)
        r.y = g->y + (g->lines - c->lines) / 2;
    else if ((c->pos_flags & WPOS_KEEP_TOP) != 0 && (c->pos_flags & WPOS_KEEP_BOTTOM) != 0)
    {
        r.y += wss->shift_y;
        r.lines += wss->scale_y;
    }
    else if ((c->pos_flags & WPOS_KEEP_TOP) != 0)
        r.y += wss->shift_y;
    else if ((c->pos_flags & WPOS_KEEP_BOTTOM) != 0)
        r.y += wss->shift_y + wss->scale_y;

    send_message (c, NULL, MSG_RESIZE, 0, &r);
}

/* --------------------------------------------------------------------------------------------- */

static void
group_set_position (WGroup * g, const WRect * r)
{
    Widget *w = WIDGET (g);
    widget_shift_scale_t wss;
    /* save old positions, will be used to reposition childs */
    WRect or = { w->y, w->x, w->lines, w->cols };

    w->x = r->x;
    w->y = r->y;
    w->lines = r->lines;
    w->cols = r->cols;

    /* dialog is empty */
    if (g->widgets == NULL)
        return;

    if (g->current == NULL)
        g->current = g->widgets;

    /* values by which controls should be moved */
    wss.shift_x = w->x - or.x;
    wss.scale_x = w->cols - or.cols;
    wss.shift_y = w->y - or.y;
    wss.scale_y = w->lines - or.lines;

    if (wss.shift_x != 0 || wss.shift_y != 0 || wss.scale_x != 0 || wss.scale_y != 0)
        g_list_foreach (g->widgets, group_widget_set_position, &wss);
}

/* --------------------------------------------------------------------------------------------- */

static void
group_default_resize (WGroup * g, WRect * r)
{
    /* This is default resizing mechanism.
     * The main idea of this code is to resize dialog according to flags
     * (if any of flags require automatic resizing, like WPOS_CENTER,
     * end after that reposition controls in dialog according to flags of widget)
     */

    Widget *w = WIDGET (g);
    WRect r0;

    if (r == NULL)
        rect_init (&r0, w->y, w->x, w->lines, w->cols);
    else
        r0 = *r;

    widget_adjust_position (w->pos_flags, &r0.y, &r0.x, &r0.lines, &r0.cols);
    group_set_position (g, &r0);
}

/* --------------------------------------------------------------------------------------------- */

static void
group_draw (WGroup * g)
{
    Widget *wg = WIDGET (g);

    /* draw all widgets in Z-order, from first to last */
    if (widget_get_state (wg, WST_ACTIVE))
    {
        GList *p;

        if (g->winch_pending)
        {
            g->winch_pending = FALSE;
            send_message (wg, NULL, MSG_RESIZE, 0, NULL);
        }

        for (p = g->widgets; p != NULL; p = g_list_next (p))
            widget_draw (WIDGET (p->data));

        widget_update_cursor (wg);
    }
}

/* --------------------------------------------------------------------------------------------- */

static cb_ret_t
group_handle_key (WGroup * g, int key)
{
    cb_ret_t handled;

    /* first try the hotkey */
    handled = send_message (g, NULL, MSG_HOTKEY, key, NULL);

    /* not used - then try widget_callback */
    if (handled == MSG_NOT_HANDLED)
        handled = send_message (g->current->data, NULL, MSG_KEY, key, NULL);

    /* not used - try to use the unhandled case */
    if (handled == MSG_NOT_HANDLED)
        handled = send_message (g, g->current->data, MSG_UNHANDLED_KEY, key, NULL);

    return handled;
}

/* --------------------------------------------------------------------------------------------- */

static cb_ret_t
group_handle_hotkey (WGroup * g, int key)
{
    GList *current;
    Widget *w;
    cb_ret_t handled = MSG_NOT_HANDLED;
    int c;

    if (g->widgets == NULL)
        return MSG_NOT_HANDLED;

    if (g->current == NULL)
        g->current = g->widgets;

    w = WIDGET (g->current->data);

    if (!widget_get_state (w, WST_VISIBLE) || widget_get_state (w, WST_DISABLED))
        return MSG_NOT_HANDLED;

    /* Explanation: we don't send letter hotkeys to other widgets
     * if the currently selected widget is an input line */
    if (widget_get_options (w, WOP_IS_INPUT))
    {
        /* skip ascii control characters, anything else can valid character in some encoding */
        if (key >= 32 && key < 256)
            return MSG_NOT_HANDLED;
    }

    /* If it's an alt key, send the message */
    c = key & ~ALT (0);
    if (key & ALT (0) && g_ascii_isalpha (c))
        key = g_ascii_tolower (c);

    if (widget_get_options (w, WOP_WANT_HOTKEY))
        handled = send_message (w, NULL, MSG_HOTKEY, key, NULL);

    /* If not used, send hotkey to other widgets */
    if (handled == MSG_HANDLED)
        return MSG_HANDLED;

    current = group_get_widget_next_of (g->current);

    /* send it to all widgets */
    while (g->current != current && handled == MSG_NOT_HANDLED)
    {
        w = WIDGET (current->data);

        if (widget_get_options (w, WOP_WANT_HOTKEY) && !widget_get_state (w, WST_DISABLED))
            handled = send_message (w, NULL, MSG_HOTKEY, key, NULL);

        if (handled == MSG_NOT_HANDLED)
            current = group_get_widget_next_of (current);
    }

    if (handled == MSG_HANDLED)
    {
        w = WIDGET (current->data);
        widget_select (w);
        send_message (g, w, MSG_HOTKEY_HANDLED, 0, NULL);
    }

    return handled;
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

/**
 * Initialize group.
 *
 * @param g WGroup widget
 * @param y1 y-coordinate of top-left corner
 * @param x1 x-coordinate of top-left corner
 * @param lines group height
 * @param cols group width
 * @param callback group callback
 * @param mouse_callback group mouse handler
 */

void
group_init (WGroup * g, int y1, int x1, int lines, int cols, widget_cb_fn callback,
            widget_mouse_cb_fn mouse_callback)
{
    Widget *w = WIDGET (g);

    widget_init (w, y1, x1, lines, cols, callback != NULL ? callback : group_default_callback,
                 mouse_callback);

    w->mouse_handler = group_handle_mouse_event;

    w->make_global = group_default_make_global;
    w->make_local = group_default_make_local;

    w->find = group_default_find;
    w->find_by_type = group_default_find_by_type;
    w->find_by_id = group_default_find_by_id;

    w->set_state = group_default_set_state;

    g->mouse_status = MOU_UNHANDLED;
}

/* --------------------------------------------------------------------------------------------- */

cb_ret_t
group_default_callback (Widget * w, Widget * sender, widget_msg_t msg, int parm, void *data)
{
    WGroup *g = GROUP (w);

    switch (msg)
    {
    case MSG_INIT:
        g_list_foreach (g->widgets, group_widget_init, NULL);
        return MSG_HANDLED;

    case MSG_DRAW:
        group_draw (g);
        return MSG_HANDLED;

    case MSG_KEY:
        return group_handle_key (g, parm);

    case MSG_HOTKEY:
        return group_handle_hotkey (g, parm);

    case MSG_CURSOR:
        return group_update_cursor (g);

    case MSG_RESIZE:
        group_default_resize (g, RECT (data));
        return MSG_HANDLED;

    case MSG_DESTROY:
        g_list_foreach (g->widgets, (GFunc) widget_destroy, NULL);
        g_list_free (g->widgets);
        g->widgets = NULL;
        return MSG_HANDLED;

    default:
        return widget_default_callback (w, sender, msg, parm, data);
    }
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Change state of group.
 *
 * @param w      group
 * @param state  widget state flag to modify
 * @param enable specifies whether to turn the flag on (TRUE) or off (FALSE).
 *               Only one flag per call can be modified.
 * @return       MSG_HANDLED if set was handled successfully, MSG_NOT_HANDLED otherwise.
 */
cb_ret_t
group_default_set_state (Widget * w, widget_state_t state, gboolean enable)
{
    gboolean ret = MSG_HANDLED;
    WGroup *g = GROUP (w);
    widget_state_info_t st = {
        .state = state,
        .enable = enable
    };

    ret = widget_default_set_state (w, state, enable);

    if (state == WST_ACTIVE || state == WST_SUSPENDED || state == WST_CLOSED)
        /* inform all child widgets */
        g_list_foreach (g->widgets, group_widget_set_state, &st);

    if ((w->state & WST_ACTIVE) != 0)
    {
        if ((w->state & WST_FOCUSED) != 0)
        {
            /* update current widget */
            if (g->current != NULL)
                widget_set_state (WIDGET (g->current->data), WST_FOCUSED, enable);
        }
        else
            /* inform all child widgets */
            g_list_foreach (g->widgets, group_widget_set_state, &st);
    }

    return ret;
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Handling mouse events.
 *
 * @param g WGroup object
 * @param event GPM mouse event
 *
 * @return result of mouse event handling
 */
int
group_handle_mouse_event (Widget * w, Gpm_Event * event)
{
    WGroup *g = GROUP (w);

    if (g->widgets != NULL)
    {
        GList *p;

        /* send the event to widgets in reverse Z-order */
        p = g_list_last (g->widgets);
        do
        {
            Widget *wp = WIDGET (p->data);

            /* Don't use widget_is_selectable() here.
               If WOP_SELECTABLE option is not set, widget can handle mouse events.
               For example, commandl line in file manager */
            if (widget_get_state (w, WST_VISIBLE) && !widget_get_state (wp, WST_DISABLED))
            {
                /* put global cursor position to the widget */
                int ret;

                ret = wp->mouse_handler (wp, event);
                if (ret != MOU_UNHANDLED)
                    return ret;
            }

            p = g_list_previous (p);
        }
        while (p != NULL);
    }

    return MOU_UNHANDLED;
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Insert widget to group before specified widget with specified positioning.
 * Make the inserted widget current.
 *
 * @param g WGroup object
 * @param w widget to be added
 * @pos positioning flags
 * @param before add @w before this widget
 *
 * @return widget ID
 */

unsigned long
group_add_widget_autopos (WGroup * g, void *w, widget_pos_flags_t pos_flags, const void *before)
{
    Widget *wg = WIDGET (g);
    Widget *ww = WIDGET (w);
    GList *new_current;

    /* Don't accept NULL widget. This shouldn't happen */
    assert (ww != NULL);

    if ((pos_flags & WPOS_CENTER_HORZ) != 0)
        ww->x = (wg->cols - ww->cols) / 2;

    if ((pos_flags & WPOS_CENTER_VERT) != 0)
        ww->y = (wg->lines - ww->lines) / 2;

    ww->owner = g;
    ww->pos_flags = pos_flags;
    widget_make_global (ww);

    if (g->widgets == NULL || before == NULL)
    {
        g->widgets = g_list_append (g->widgets, ww);
        new_current = g_list_last (g->widgets);
    }
    else
    {
        GList *b;

        b = g_list_find (g->widgets, before);

        /* don't accept widget not from group. This shouldn't happen */
        assert (b != NULL);

        b = g_list_next (b);
        g->widgets = g_list_insert_before (g->widgets, b, ww);
        if (b != NULL)
            new_current = g_list_previous (b);
        else
            new_current = g_list_last (g->widgets);
    }

    /* widget has been added at runtime */
    if (widget_get_state (wg, WST_ACTIVE))
    {
        group_widget_init (ww, NULL);
        widget_select (ww);
    }
    else
        g->current = new_current;

    return ww->id;
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Remove widget from group.
 *
 * @param w Widget object
 */
void
group_remove_widget (void *w)
{
    Widget *ww = WIDGET (w);
    WGroup *g;
    GList *d;

    /* Don't accept NULL widget. This shouldn't happen */
    assert (w != NULL);

    g = ww->owner;

    d = g_list_find (g->widgets, ww);
    if (d == g->current)
        group_set_current_widget_next (g);

    g->widgets = g_list_delete_link (g->widgets, d);
    if (g->widgets == NULL)
        g->current = NULL;

    /* widget has been deleted at runtime */
    if (widget_get_state (WIDGET (g), WST_ACTIVE))
    {
        group_draw (g);
        group_select_current_widget (g);
    }

    widget_make_local (ww);
    ww->owner = NULL;
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Switch current widget to widget after current in group.
 *
 * @param g WGroup object
 */

void
group_set_current_widget_next (WGroup * g)
{
    g->current = group_get_next_or_prev_of (g->current, TRUE);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Switch current widget to widget before current in group.
 *
 * @param g WGroup object
 */

void
group_set_current_widget_prev (WGroup * g)
{
    g->current = group_get_next_or_prev_of (g->current, FALSE);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Get widget that is after specified widget in group.
 *
 * @param w widget holder
 *
 * @return widget that is after "w" or NULL if "w" is NULL or widget doesn't have owner
 */

GList *
group_get_widget_next_of (GList * w)
{
    return group_get_next_or_prev_of (w, TRUE);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Get widget that is before specified widget in group.
 *
 * @param w widget holder
 *
 * @return widget that is before "w" or NULL if "w" is NULL or widget doesn't have owner
 */

GList *
group_get_widget_prev_of (GList * w)
{
    return group_get_next_or_prev_of (w, FALSE);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Try to select next widget in the Z order.
 *
 * @param g WGroup object
 */

void
group_select_next_widget (WGroup * g)
{
    group_select_next_or_prev (g, TRUE);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Try to select previous widget in the Z order.
 *
 * @param g WGroup object
 */

void
group_select_prev_widget (WGroup * g)
{
    group_select_next_or_prev (g, FALSE);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Find the widget with the specified ID in the group and select it
 *
 * @param g WGroup object
 * @param id widget ID
 */

void
group_select_widget_by_id (const WGroup * g, unsigned long id)
{
    Widget *w;

    w = widget_find_by_id (CONST_WIDGET (g), id);
    if (w != NULL)
        widget_select (w);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Send broadcast message to all widgets in the group.
 *
 * @param g WGroup object
 * @param msg message sent to widgets
 */

void
group_send_broadcast_msg (WGroup * g, widget_msg_t msg)
{
    group_send_broadcast_msg_custom (g, msg, FALSE, WOP_DEFAULT);
}

/* --------------------------------------------------------------------------------------------- */
