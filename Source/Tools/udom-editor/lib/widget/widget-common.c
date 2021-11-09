/*
   Widgets for the Midnight Commander

   Copyright (C) 1994-2021
   Free Software Foundation, Inc.

   Authors:
   Radek Doulik, 1994, 1995
   Miguel de Icaza, 1994, 1995
   Jakub Jelinek, 1995
   Andrej Borsenkow, 1996
   Norbert Warmuth, 1997
   Andrew Borodin <aborodin@vmail.ru>, 2009, 2010, 2011, 2012, 2013

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

/** \file widget-common.c
 *  \brief Source: shared stuff of widgets
 */

#include <config.h>

#include <stdlib.h>
#include <string.h>

#include "lib/global.h"

#include "lib/tty/tty.h"
#include "lib/tty/color.h"
#include "lib/skin.h"
#include "lib/strutil.h"
#include "lib/widget.h"

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/* maximum value of used widget ID */
static unsigned long widget_id = 0;

/* --------------------------------------------------------------------------------------------- */
/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

/**
 * Calc widget ID,
 * Widget ID is uniq for each widget created during MC session (like PID in OS).
 *
 * @return widget ID.
 */
static unsigned long
widget_set_id (void)
{
    unsigned long id;

    id = widget_id++;
    /* TODO IF NEEDED: if id is already used, find next free id. */

    return id;
}

/* --------------------------------------------------------------------------------------------- */

static cb_ret_t
widget_default_resize (Widget * w, const WRect * r)
{
    if (r == NULL)
        return MSG_NOT_HANDLED;

    w->y = r->y;
    w->x = r->x;
    w->lines = r->lines;
    w->cols = r->cols;

    return MSG_HANDLED;
}

/* --------------------------------------------------------------------------------------------- */

static void
widget_do_focus (Widget * w, gboolean enable)
{
    if (w != NULL && widget_get_state (WIDGET (w->owner), WST_VISIBLE | WST_FOCUSED))
        widget_set_state (w, WST_FOCUSED, enable);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Focus specified widget in it's owner.
 *
 * @param w widget to be focused.
 */

static void
widget_focus (Widget * w)
{
    WGroup *g = w->owner;

    if (g == NULL)
        return;

    if (WIDGET (g->current->data) != w)
    {
        widget_do_focus (WIDGET (g->current->data), FALSE);
        /* Test if focus lost was allowed and focus has really been loose */
        if (g->current == NULL || !widget_get_state (WIDGET (g->current->data), WST_FOCUSED))
        {
            widget_do_focus (w, TRUE);
            g->current = widget_find (WIDGET (g), w);
        }
    }
    else if (!widget_get_state (w, WST_FOCUSED))
        widget_do_focus (w, TRUE);
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Put widget on top or bottom of Z-order.
 */
static void
widget_reorder (GList * l, gboolean set_top)
{
    WGroup *g = WIDGET (l->data)->owner;

    g->widgets = g_list_remove_link (g->widgets, l);
    if (set_top)
        g->widgets = g_list_concat (g->widgets, l);
    else
        g->widgets = g_list_concat (l, g->widgets);
}

/* --------------------------------------------------------------------------------------------- */

static gboolean
hotkey_cmp (const char *s1, const char *s2)
{
    gboolean n1, n2;

    n1 = s1 != NULL;
    n2 = s2 != NULL;

    if (n1 != n2)
        return FALSE;

    if (n1 && n2 && strcmp (s1, s2) != 0)
        return FALSE;

    return TRUE;
}

/* --------------------------------------------------------------------------------------------- */

static void
widget_default_mouse_callback (Widget * w, mouse_msg_t msg, mouse_event_t * event)
{
    /* do nothing */
    (void) w;
    (void) msg;
    (void) event;
}

/* --------------------------------------------------------------------------------------------- */

static const int *
widget_default_get_colors (const Widget * w)
{
    const Widget *owner = CONST_WIDGET (w->owner);

    return (owner == NULL ? NULL : widget_get_colors (owner));
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

struct hotkey_t
hotkey_new (const char *text)
{
    hotkey_t result;
    const char *cp, *p;

    if (text == NULL)
        text = "";

    /* search for '&', that is not on the of text */
    cp = strchr (text, '&');
    if (cp != NULL && cp[1] != '\0')
    {
        result.start = g_strndup (text, cp - text);

        /* skip '&' */
        cp++;
        p = str_cget_next_char (cp);
        result.hotkey = g_strndup (cp, p - cp);

        cp = p;
        result.end = g_strdup (cp);
    }
    else
    {
        result.start = g_strdup (text);
        result.hotkey = NULL;
        result.end = NULL;
    }

    return result;
}

/* --------------------------------------------------------------------------------------------- */

void
hotkey_free (const hotkey_t hotkey)
{
    g_free (hotkey.start);
    g_free (hotkey.hotkey);
    g_free (hotkey.end);
}

/* --------------------------------------------------------------------------------------------- */

int
hotkey_width (const hotkey_t hotkey)
{
    int result;

    result = str_term_width1 (hotkey.start);
    result += (hotkey.hotkey != NULL) ? str_term_width1 (hotkey.hotkey) : 0;
    result += (hotkey.end != NULL) ? str_term_width1 (hotkey.end) : 0;
    return result;
}

/* --------------------------------------------------------------------------------------------- */

gboolean
hotkey_equal (const hotkey_t hotkey1, const hotkey_t hotkey2)
{
    /* *INDENT-OFF* */
    return (strcmp (hotkey1.start, hotkey2.start) == 0) &&
           hotkey_cmp (hotkey1.hotkey, hotkey2.hotkey) &&
           hotkey_cmp (hotkey1.end, hotkey2.end);
    /* *INDENT-ON* */
}

/* --------------------------------------------------------------------------------------------- */

void
hotkey_draw (Widget * w, const hotkey_t hotkey, gboolean focused)
{
    if (hotkey.start[0] != '\0')
    {
        widget_selectcolor (w, focused, FALSE);
        tty_print_string (hotkey.start);
    }

    if (hotkey.hotkey != NULL)
    {
        widget_selectcolor (w, focused, TRUE);
        tty_print_string (hotkey.hotkey);
    }

    if (hotkey.end != NULL)
    {
        widget_selectcolor (w, focused, FALSE);
        tty_print_string (hotkey.end);
    }
}

/* --------------------------------------------------------------------------------------------- */

char *
hotkey_get_text (const hotkey_t hotkey)
{
    GString *text;

    text = g_string_new (hotkey.start);

    if (hotkey.hotkey != NULL)
    {
        g_string_append_c (text, '&');
        g_string_append (text, hotkey.hotkey);
    }

    if (hotkey.end != NULL)
        g_string_append (text, hotkey.end);

    return g_string_free (text, FALSE);
}

/* --------------------------------------------------------------------------------------------- */

void
widget_init (Widget * w, int y, int x, int lines, int cols,
             widget_cb_fn callback, widget_mouse_cb_fn mouse_callback)
{
    w->id = widget_set_id ();
    w->x = x;
    w->y = y;
    w->cols = cols;
    w->lines = lines;
    w->pos_flags = WPOS_KEEP_DEFAULT;
    w->callback = callback;

    w->keymap = NULL;
    w->ext_keymap = NULL;
    w->ext_mode = FALSE;

    w->mouse_callback = mouse_callback != NULL ? mouse_callback : widget_default_mouse_callback;
    w->owner = NULL;
    w->mouse_handler = mouse_handle_event;
    w->mouse.forced_capture = FALSE;
    w->mouse.capture = FALSE;
    w->mouse.last_msg = MSG_MOUSE_NONE;
    w->mouse.last_buttons_down = 0;

    w->options = WOP_DEFAULT;
    w->state = WST_CONSTRUCT | WST_VISIBLE;

    w->make_global = widget_default_make_global;
    w->make_local = widget_default_make_local;

    w->make_global = widget_default_make_global;
    w->make_local = widget_default_make_local;

    w->find = widget_default_find;
    w->find_by_type = widget_default_find_by_type;
    w->find_by_id = widget_default_find_by_id;

    w->set_state = widget_default_set_state;
    w->destroy = widget_default_destroy;
    w->get_colors = widget_default_get_colors;
}

/* --------------------------------------------------------------------------------------------- */

/* Default callback for widgets */
cb_ret_t
widget_default_callback (Widget * w, Widget * sender, widget_msg_t msg, int parm, void *data)
{
    (void) sender;
    (void) parm;

    switch (msg)
    {
    case MSG_INIT:
    case MSG_FOCUS:
    case MSG_UNFOCUS:
    case MSG_ENABLE:
    case MSG_DISABLE:
    case MSG_DRAW:
    case MSG_DESTROY:
    case MSG_CURSOR:
    case MSG_IDLE:
        return MSG_HANDLED;

    case MSG_RESIZE:
        return widget_default_resize (w, CONST_RECT (data));

    default:
        return MSG_NOT_HANDLED;
    }
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Apply new options to widget.
 *
 * @param w       widget
 * @param options widget option flags to modify. Several flags per call can be modified.
 * @param enable  TRUE if specified options should be added, FALSE if options should be removed
 */
void
widget_set_options (Widget * w, widget_options_t options, gboolean enable)
{
    if (enable)
        w->options |= options;
    else
        w->options &= ~options;
}

/* --------------------------------------------------------------------------------------------- */

void
widget_adjust_position (widget_pos_flags_t pos_flags, int *y, int *x, int *lines, int *cols)
{
    if ((pos_flags & WPOS_FULLSCREEN) != 0)
    {
        *y = 0;
        *x = 0;
        *lines = LINES;
        *cols = COLS;
    }
    else
    {
        if ((pos_flags & WPOS_CENTER_HORZ) != 0)
            *x = (COLS - *cols) / 2;

        if ((pos_flags & WPOS_CENTER_VERT) != 0)
            *y = (LINES - *lines) / 2;

        if ((pos_flags & WPOS_TRYUP) != 0)
        {
            if (*y > 3)
                *y -= 2;
            else if (*y == 3)
                *y = 2;
        }
    }
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Change widget position and size.
 *
 * @param w widget
 * @param y y coordinate of top-left corner
 * @param x x coordinate of top-left corner
 * @param lines width
 * @param cols height
 */

void
widget_set_size (Widget * w, int y, int x, int lines, int cols)
{
    WRect r = { y, x, lines, cols };

    send_message (w, NULL, MSG_RESIZE, 0, &r);
    widget_draw (w);
}

/* --------------------------------------------------------------------------------------------- */

void
widget_selectcolor (Widget * w, gboolean focused, gboolean hotkey)
{
    int color;
    const int *colors;

    colors = widget_get_colors (w);

    if (widget_get_state (w, WST_DISABLED))
        color = DISABLED_COLOR;
    else if (hotkey)
        color = colors[focused ? DLG_COLOR_HOT_FOCUS : DLG_COLOR_HOT_NORMAL];
    else
        color = colors[focused ? DLG_COLOR_FOCUS : DLG_COLOR_NORMAL];

    tty_setcolor (color);
}

/* --------------------------------------------------------------------------------------------- */

void
widget_erase (Widget * w)
{
    if (w != NULL)
        tty_fill_region (w->y, w->x, w->lines, w->cols, ' ');
}

/* --------------------------------------------------------------------------------------------- */

void
widget_set_visibility (Widget * w, gboolean make_visible)
{
    if (widget_get_state (w, WST_VISIBLE) != make_visible)
        widget_set_state (w, WST_VISIBLE, make_visible);
}

/* --------------------------------------------------------------------------------------------- */
/**
  * Check whether widget is active or not.
  * Widget is active if it's current in the its owner and each owner in the chain is current too.
  *
  * @param w the widget
  *
  * @return TRUE if the widget is active, FALSE otherwise
  */

gboolean
widget_is_active (const void *w)
{
    const WGroup *owner;

    /* Is group top? */
    if (w == top_dlg->data)
        return TRUE;

    owner = CONST_WIDGET (w)->owner;

    /* Is widget in any group? */
    if (owner == NULL)
        return FALSE;

    if (w != owner->current->data)
        return FALSE;

    return widget_is_active (owner);
}

/* --------------------------------------------------------------------------------------------- */

cb_ret_t
widget_draw (Widget * w)
{
    cb_ret_t ret = MSG_NOT_HANDLED;

    if (w != NULL && widget_get_state (w, WST_VISIBLE))
    {
        WGroup *g = w->owner;

        if (g != NULL && widget_get_state (WIDGET (g), WST_ACTIVE))
            ret = w->callback (w, NULL, MSG_DRAW, 0, NULL);
    }

    return ret;
}

/* --------------------------------------------------------------------------------------------- */
/**
  * Replace widget in the dialog.
  *
  * @param old_w old widget that need to be replaced
  * @param new_w new widget that will replace @old_w
  */

void
widget_replace (Widget * old_w, Widget * new_w)
{
    WGroup *g = old_w->owner;
    gboolean should_focus = FALSE;
    GList *holder;

    if (g->widgets == NULL)
        return;

    if (g->current == NULL)
        g->current = g->widgets;

    /* locate widget position in the list */
    if (old_w == g->current->data)
        holder = g->current;
    else
        holder = g_list_find (g->widgets, old_w);

    /* if old widget is focused, we should focus the new one... */
    if (widget_get_state (old_w, WST_FOCUSED))
        should_focus = TRUE;
    /* ...but if new widget isn't selectable, we cannot focus it */
    if (!widget_get_options (new_w, WOP_SELECTABLE))
        should_focus = FALSE;

    /* if new widget isn't selectable, select other widget before replace */
    if (!should_focus)
    {
        GList *l;

        for (l = group_get_widget_next_of (holder); widget_is_focusable (WIDGET (l->data));
             l = group_get_widget_next_of (l))
            ;

        widget_select (WIDGET (l->data));
    }

    /* replace widget */
    new_w->owner = g;
    new_w->id = old_w->id;
    holder->data = new_w;

    send_message (old_w, NULL, MSG_DESTROY, 0, NULL);
    send_message (new_w, NULL, MSG_INIT, 0, NULL);

    if (should_focus)
        widget_select (new_w);
    else
        widget_draw (new_w);
}

/* --------------------------------------------------------------------------------------------- */

gboolean
widget_is_focusable (const Widget * w)
{
    return (widget_get_options (w, WOP_SELECTABLE) && widget_get_state (w, WST_VISIBLE) &&
            !widget_get_state (w, WST_DISABLED));
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Select specified widget in it's owner.
 *
 * Note: this function (and widget_focus(), which it calls) is a no-op
 * if the widget is already selected.
 *
 * @param w widget to be selected
 */

void
widget_select (Widget * w)
{
    WGroup *g;

    if (!widget_get_options (w, WOP_SELECTABLE))
        return;

    g = GROUP (w->owner);
    if (g != NULL)
    {
        if (widget_get_options (w, WOP_TOP_SELECT))
        {
            GList *l;

            l = widget_find (WIDGET (g), w);
            widget_reorder (l, TRUE);
        }

        widget_focus (w);
    }
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Set widget at bottom of widget list.
 */

void
widget_set_bottom (Widget * w)
{
    widget_reorder (widget_find (WIDGET (w->owner), w), FALSE);
}

/* --------------------------------------------------------------------------------------------- */
/**
  * Check whether two widgets are overlapped or not.
  * @param a 1st widget
  * @param b 2nd widget
  *
  * @return TRUE if widgets are overlapped, FALSE otherwise.
  */

gboolean
widget_overlapped (const Widget * a, const Widget * b)
{
    return !((b->x >= a->x + a->cols)
             || (a->x >= b->x + b->cols) || (b->y >= a->y + a->lines) || (a->y >= b->y + b->lines));
}

/* --------------------------------------------------------------------------------------------- */
/**
  * Look up key event of widget and translate it to command ID.
  * @param w   widget
  * @param key key event
  *
  * @return command ID binded with @key.
  */

long
widget_lookup_key (Widget * w, int key)
{
    if (w->ext_mode)
    {
        w->ext_mode = FALSE;
        return keybind_lookup_keymap_command (w->ext_keymap, key);
    }

    return keybind_lookup_keymap_command (w->keymap, key);
}

/* --------------------------------------------------------------------------------------------- */

/**
  * Default widget callback to convert widget coordinates from local (relative to owner) to global
  * (relative to screen).
  *
  * @param w widget
  * @delta offset for top-left corner coordinates. Used for child widgets of WGroup
  */

void
widget_default_make_global (Widget * w, const WRect * delta)
{
    if (delta != NULL)
    {
        w->y += delta->y;
        w->x += delta->x;
    }
    else if (w->owner != NULL)
    {
        w->y += WIDGET (w->owner)->y;
        w->x += WIDGET (w->owner)->x;
    }
}

/* --------------------------------------------------------------------------------------------- */

/**
  * Default widget callback to convert widget coordinates from global (relative to screen) to local
  * (relative to owner).
  *
  * @param w widget
  * @delta offset for top-left corner coordinates. Used for child widgets of WGroup
  */

void
widget_default_make_local (Widget * w, const WRect * delta)
{
    if (delta != NULL)
    {
        w->y -= delta->y;
        w->x -= delta->x;
    }
    else if (w->owner != NULL)
    {
        w->y -= WIDGET (w->owner)->y;
        w->x -= WIDGET (w->owner)->x;
    }
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Default callback function to find widget.
 *
 * @param w widget
 * @param what widget to find
 *
 * @return holder of @what if widget is @what, NULL otherwise
 */

GList *
widget_default_find (const Widget * w, const Widget * what)
{
    return (w != what
            || w->owner == NULL) ? NULL : g_list_find (CONST_GROUP (w->owner)->widgets, what);
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Default callback function to find widget by widget type using widget callback.
 *
 * @param w widget
 * @param cb widget callback
 *
 * @return @w if widget callback is @cb, NULL otherwise
 */

Widget *
widget_default_find_by_type (const Widget * w, widget_cb_fn cb)
{
    return (w->callback == cb ? WIDGET (w) : NULL);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Default callback function to find widget by widget ID.
 *
 * @param w widget
 * @param id widget ID
 *
 * @return @w if widget id is equal to @id, NULL otherwise
 */

Widget *
widget_default_find_by_id (const Widget * w, unsigned long id)
{
    return (w->id == id ? WIDGET (w) : NULL);
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Default callback function to modify state of widget.
 *
 * @param w      widget
 * @param state  widget state flag to modify
 * @param enable specifies whether to turn the flag on (TRUE) or off (FALSE).
 *               Only one flag per call can be modified.
 * @return       MSG_HANDLED if set was handled successfully, MSG_NOT_HANDLED otherwise.
 */

cb_ret_t
widget_default_set_state (Widget * w, widget_state_t state, gboolean enable)
{
    gboolean ret = MSG_HANDLED;
    Widget *owner = WIDGET (GROUP (w->owner));

    if (enable)
        w->state |= state;
    else
        w->state &= ~state;

    if (enable)
    {
        /* exclusive bits */
        if ((state & WST_CONSTRUCT) != 0)
            w->state &= ~(WST_ACTIVE | WST_SUSPENDED | WST_CLOSED);
        else if ((state & WST_ACTIVE) != 0)
            w->state &= ~(WST_CONSTRUCT | WST_SUSPENDED | WST_CLOSED);
        else if ((state & WST_SUSPENDED) != 0)
            w->state &= ~(WST_CONSTRUCT | WST_ACTIVE | WST_CLOSED);
        else if ((state & WST_CLOSED) != 0)
            w->state &= ~(WST_CONSTRUCT | WST_ACTIVE | WST_SUSPENDED);
    }

    if (owner == NULL)
        return MSG_NOT_HANDLED;

    switch (state)
    {
    case WST_VISIBLE:
        if (widget_get_state (owner, WST_ACTIVE))
        {
            /* redraw owner to show/hide widget */
            widget_draw (owner);

            if (!enable)
            {
                /* try select another widget if current one got hidden */
                if (w == GROUP (owner)->current->data)
                    group_select_next_widget (GROUP (owner));

                widget_update_cursor (owner);   /* FIXME: unneeded? */
            }
        }
        break;


    case WST_DISABLED:
        ret = send_message (w, NULL, enable ? MSG_DISABLE : MSG_ENABLE, 0, NULL);
        if (ret == MSG_HANDLED && widget_get_state (owner, WST_ACTIVE))
            ret = widget_draw (w);
        break;

    case WST_FOCUSED:
        {
            widget_msg_t msg;

            msg = enable ? MSG_FOCUS : MSG_UNFOCUS;
            ret = send_message (w, NULL, msg, 0, NULL);
            if (ret == MSG_HANDLED && widget_get_state (owner, WST_ACTIVE))
            {
                widget_draw (w);
                /* Notify owner that focus was moved from one widget to another */
                send_message (owner, w, MSG_CHANGED_FOCUS, 0, NULL);
            }
        }
        break;

    default:
        break;
    }

    return ret;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Default callback function to destroy widget.
 *
 * @param w widget
 */

void
widget_default_destroy (Widget * w)
{
    send_message (w, NULL, MSG_DESTROY, 0, NULL);
    g_free (w);
}

/* --------------------------------------------------------------------------------------------- */
/* get mouse pointer location within widget */

Gpm_Event
mouse_get_local (const Gpm_Event * global, const Widget * w)
{
    Gpm_Event local;

    memset (&local, 0, sizeof (local));

    local.buttons = global->buttons;
    local.x = global->x - w->x;
    local.y = global->y - w->y;
    local.type = global->type;

    return local;
}

/* --------------------------------------------------------------------------------------------- */

gboolean
mouse_global_in_widget (const Gpm_Event * event, const Widget * w)
{
    return (event->x > w->x) && (event->y > w->y) && (event->x <= w->x + w->cols)
        && (event->y <= w->y + w->lines);
}

/* --------------------------------------------------------------------------------------------- */
