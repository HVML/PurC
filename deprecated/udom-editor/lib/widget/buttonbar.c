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

/** \file buttonbar.c
 *  \brief Source: WButtonBar widget
 */

#include <config.h>

#include <stdlib.h>
#include <string.h>

#include "lib/global.h"

#include "lib/tty/tty.h"
#include "lib/tty/key.h"        /* XCTRL and ALT macros  */
#include "lib/skin.h"
#include "lib/strutil.h"
#include "lib/util.h"
#include "lib/widget.h"

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

/* calculate positions of buttons; width is never less than 7 */
static void
buttonbar_init_button_positions (WButtonBar * bb)
{
    int i;
    int pos = 0;

    if (COLS < BUTTONBAR_LABELS_NUM * 7)
    {
        for (i = 0; i < BUTTONBAR_LABELS_NUM; i++)
        {
            if (pos + 7 <= COLS)
                pos += 7;

            bb->labels[i].end_coord = pos;
        }
    }
    else
    {
        /* Distribute the extra width in a way that the middle vertical line
           (between F5 and F6) aligns with the two panels. The extra width
           is distributed in this order: F10, F5, F9, F4, ..., F6, F1. */
        int dv, md;

        dv = COLS / BUTTONBAR_LABELS_NUM;
        md = COLS % BUTTONBAR_LABELS_NUM;

        for (i = 0; i < BUTTONBAR_LABELS_NUM / 2; i++)
        {
            pos += dv;
            if (BUTTONBAR_LABELS_NUM / 2 - 1 - i < md / 2)
                pos++;

            bb->labels[i].end_coord = pos;
        }

        for (; i < BUTTONBAR_LABELS_NUM; i++)
        {
            pos += dv;
            if (BUTTONBAR_LABELS_NUM - 1 - i < (md + 1) / 2)
                pos++;

            bb->labels[i].end_coord = pos;
        }
    }
}

/* --------------------------------------------------------------------------------------------- */

/* return width of one button */
static int
buttonbar_get_button_width (const WButtonBar * bb, int i)
{
    if (i == 0)
        return bb->labels[0].end_coord;
    return bb->labels[i].end_coord - bb->labels[i - 1].end_coord;
}

/* --------------------------------------------------------------------------------------------- */

static int
buttonbar_get_button_by_x_coord (const WButtonBar * bb, int x)
{
    int i;

    for (i = 0; i < BUTTONBAR_LABELS_NUM; i++)
        if (bb->labels[i].end_coord > x)
            return i;

    return (-1);
}

/* --------------------------------------------------------------------------------------------- */

static void
set_label_text (WButtonBar * bb, int idx, const char *text)
{
    g_free (bb->labels[idx - 1].text);
    bb->labels[idx - 1].text = g_strdup (text);
}

/* --------------------------------------------------------------------------------------------- */

/* returns TRUE if a function has been called, FALSE otherwise. */
static gboolean
buttonbar_call (WButtonBar * bb, int i)
{
    cb_ret_t ret = MSG_NOT_HANDLED;
    Widget *w = WIDGET (bb);
    Widget *target;

    if ((bb != NULL) && (bb->labels[i].command != CK_IgnoreKey))
    {
        target = (bb->labels[i].receiver != NULL) ? bb->labels[i].receiver : WIDGET (w->owner);
        ret = send_message (target, w, MSG_ACTION, bb->labels[i].command, NULL);
    }
    return ret;
}

/* --------------------------------------------------------------------------------------------- */

static cb_ret_t
buttonbar_callback (Widget * w, Widget * sender, widget_msg_t msg, int parm, void *data)
{
    WButtonBar *bb = BUTTONBAR (w);
    int i;

    switch (msg)
    {
    case MSG_HOTKEY:
        for (i = 0; i < BUTTONBAR_LABELS_NUM; i++)
            if (parm == KEY_F (i + 1) && buttonbar_call (bb, i))
                return MSG_HANDLED;
        return MSG_NOT_HANDLED;

    case MSG_DRAW:
        if (widget_get_state (w, WST_VISIBLE))
        {
            buttonbar_init_button_positions (bb);
            widget_gotoyx (w, 0, 0);
            tty_setcolor (DEFAULT_COLOR);
            tty_printf ("%-*s", w->cols, "");
            widget_gotoyx (w, 0, 0);

            for (i = 0; i < BUTTONBAR_LABELS_NUM; i++)
            {
                int width;
                const char *text;

                width = buttonbar_get_button_width (bb, i);
                if (width <= 0)
                    break;

                tty_setcolor (BUTTONBAR_HOTKEY_COLOR);
                tty_printf ("%2d", i + 1);

                tty_setcolor (BUTTONBAR_BUTTON_COLOR);
                text = (bb->labels[i].text != NULL) ? bb->labels[i].text : "";
                tty_print_string (str_fit_to_term (text, width - 2, J_LEFT_FIT));
            }
        }
        return MSG_HANDLED;

    case MSG_DESTROY:
        for (i = 0; i < BUTTONBAR_LABELS_NUM; i++)
            g_free (bb->labels[i].text);
        return MSG_HANDLED;

    default:
        return widget_default_callback (w, sender, msg, parm, data);
    }
}

/* --------------------------------------------------------------------------------------------- */

static void
buttonbar_mouse_callback (Widget * w, mouse_msg_t msg, mouse_event_t * event)
{
    switch (msg)
    {
    case MSG_MOUSE_CLICK:
        {
            WButtonBar *bb = BUTTONBAR (w);
            int button;

            button = buttonbar_get_button_by_x_coord (bb, event->x);
            if (button >= 0)
                buttonbar_call (bb, button);
            break;
        }

    default:
        break;
    }
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

WButtonBar *
buttonbar_new (void)
{
    WButtonBar *bb;
    Widget *w;

    bb = g_new0 (WButtonBar, 1);
    w = WIDGET (bb);
    widget_init (w, LINES - 1, 0, 1, COLS, buttonbar_callback, buttonbar_mouse_callback);

    w->pos_flags = WPOS_KEEP_HORZ | WPOS_KEEP_BOTTOM;
    widget_want_hotkey (w, TRUE);

    return bb;
}

/* --------------------------------------------------------------------------------------------- */

void
buttonbar_set_label (WButtonBar * bb, int idx, const char *text, const global_keymap_t * keymap,
                     Widget * receiver)
{
    if ((bb != NULL) && (idx >= 1) && (idx <= BUTTONBAR_LABELS_NUM))
    {
        long command = CK_IgnoreKey;

        if (keymap != NULL)
            command = keybind_lookup_keymap_command (keymap, KEY_F (idx));

        if ((text == NULL) || (text[0] == '\0'))
            set_label_text (bb, idx, "");
        else
            set_label_text (bb, idx, text);

        bb->labels[idx - 1].command = command;
        bb->labels[idx - 1].receiver = WIDGET (receiver);
    }
}

/* --------------------------------------------------------------------------------------------- */

/* Find ButtonBar widget in the dialog */
WButtonBar *
find_buttonbar (const WDialog * h)
{
    return BUTTONBAR (widget_find_by_type (CONST_WIDGET (h), buttonbar_callback));
}
