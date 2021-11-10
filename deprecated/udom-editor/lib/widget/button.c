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
   Andrew Borodin <aborodin@vmail.ru>, 2009, 2010, 2013, 2016

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

/** \file button.c
 *  \brief Source: WButton widget
 */

#include <config.h>

#include <stdlib.h>

#include "lib/global.h"

#include "lib/tty/tty.h"
#include "lib/strutil.h"
#include "lib/widget.h"

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/* --------------------------------------------------------------------------------------------- */
/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

cb_ret_t
button_default_callback (Widget * w, Widget * sender, widget_msg_t msg, int parm, void *data)
{
    WButton *b = BUTTON (w);
    WGroup *g = w->owner;
    WDialog *h = DIALOG (g);
    int off = 0;

    switch (msg)
    {
    case MSG_HOTKEY:
        /*
         * Don't let the default button steal Enter from the current
         * button.  This is a workaround for the flawed event model
         * when hotkeys are sent to all widgets before the key is
         * handled by the current widget.
         */
        if (parm == '\n' && WIDGET (g->current->data) == w)
        {
            send_message (w, sender, MSG_KEY, ' ', data);
            return MSG_HANDLED;
        }

        if (parm == '\n' && b->flags == DEFPUSH_BUTTON)
        {
            send_message (w, sender, MSG_KEY, ' ', data);
            return MSG_HANDLED;
        }

        if (b->text.hotkey != NULL && g_ascii_tolower ((gchar) b->text.hotkey[0]) == parm)
        {
            send_message (w, sender, MSG_KEY, ' ', data);
            return MSG_HANDLED;
        }
        return MSG_NOT_HANDLED;

    case MSG_KEY:
        if (parm != ' ' && parm != '\n')
            return MSG_NOT_HANDLED;

        h->ret_value = b->action;
        if (b->callback == NULL || b->callback (b, b->action) != 0)
            dlg_stop (h);

        return MSG_HANDLED;

    case MSG_CURSOR:
        switch (b->flags)
        {
        case DEFPUSH_BUTTON:
            off = 3;
            break;
        case NORMAL_BUTTON:
            off = 2;
            break;
        case NARROW_BUTTON:
            off = 1;
            break;
        case HIDDEN_BUTTON:
        default:
            off = 0;
            break;
        }
        widget_gotoyx (w, 0, b->hotpos + off);
        return MSG_HANDLED;

    case MSG_DRAW:
        {
            gboolean focused;

            focused = widget_get_state (w, WST_FOCUSED);

            widget_selectcolor (w, focused, FALSE);
            widget_gotoyx (w, 0, 0);

            switch (b->flags)
            {
            case DEFPUSH_BUTTON:
                tty_print_string ("[< ");
                break;
            case NORMAL_BUTTON:
                tty_print_string ("[ ");
                break;
            case NARROW_BUTTON:
                tty_print_string ("[");
                break;
            case HIDDEN_BUTTON:
            default:
                return MSG_HANDLED;
            }

            hotkey_draw (w, b->text, focused);

            switch (b->flags)
            {
            case DEFPUSH_BUTTON:
                tty_print_string (" >]");
                break;
            case NORMAL_BUTTON:
                tty_print_string (" ]");
                break;
            case NARROW_BUTTON:
                tty_print_string ("]");
                break;
            default:
                break;
            }

            return MSG_HANDLED;
        }

    case MSG_DESTROY:
        hotkey_free (b->text);
        return MSG_HANDLED;

    default:
        return widget_default_callback (w, sender, msg, parm, data);
    }
}

/* --------------------------------------------------------------------------------------------- */

void
button_mouse_default_callback (Widget * w, mouse_msg_t msg, mouse_event_t * event)
{
    (void) event;

    switch (msg)
    {
    case MSG_MOUSE_DOWN:
        widget_select (w);
        break;

    case MSG_MOUSE_CLICK:
        send_message (w, NULL, MSG_KEY, ' ', NULL);
        send_message (w->owner, w, MSG_POST_KEY, ' ', NULL);
        break;

    default:
        break;
    }
}

/* --------------------------------------------------------------------------------------------- */

WButton *
button_new (int y, int x, int action, button_flags_t flags, const char *text, bcback_fn callback)
{
    WButton *b;
    Widget *w;

    b = g_new (WButton, 1);
    w = WIDGET (b);

    b->action = action;
    b->flags = flags;
    b->text = hotkey_new (text);
    widget_init (w, y, x, 1, button_get_len (b), button_default_callback,
                 button_mouse_default_callback);
    w->options |= WOP_SELECTABLE | WOP_WANT_CURSOR | WOP_WANT_HOTKEY;
    b->callback = callback;
    b->hotpos = (b->text.hotkey != NULL) ? str_term_width1 (b->text.start) : -1;

    return b;
}

/* --------------------------------------------------------------------------------------------- */

char *
button_get_text (const WButton * b)
{
    return hotkey_get_text (b->text);
}

/* --------------------------------------------------------------------------------------------- */

void
button_set_text (WButton * b, const char *text)
{
    Widget *w = WIDGET (b);
    hotkey_t hk;

    hk = hotkey_new (text);
    if (hotkey_equal (b->text, hk))
    {
        hotkey_free (hk);
        return;
    }

    hotkey_free (b->text);
    b->text = hk;
    b->hotpos = (b->text.hotkey != NULL) ? str_term_width1 (b->text.start) : -1;
    w->cols = button_get_len (b);
    widget_draw (w);
}

/* --------------------------------------------------------------------------------------------- */

int
button_get_len (const WButton * b)
{
    int ret = hotkey_width (b->text);

    switch (b->flags)
    {
    case DEFPUSH_BUTTON:
        ret += 6;
        break;
    case NORMAL_BUTTON:
        ret += 4;
        break;
    case NARROW_BUTTON:
        ret += 2;
        break;
    case HIDDEN_BUTTON:
    default:
        return 0;
    }

    return ret;
}

/* --------------------------------------------------------------------------------------------- */
