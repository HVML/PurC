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
   Andrew Borodin <aborodin@vmail.ru>, 2009, 2010, 2013

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

/** \file groupbox.c
 *  \brief Source: WGroupbox widget
 */

#include <config.h>

#include "lib/global.h"

#include "lib/tty/tty.h"
#include "lib/tty/color.h"
#include "lib/skin.h"
#include "lib/util.h"
#include "lib/widget.h"

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/*** file scope functions ************************************************************************/

static cb_ret_t
groupbox_callback (Widget * w, Widget * sender, widget_msg_t msg, int parm, void *data)
{
    WGroupbox *g = GROUPBOX (w);

    switch (msg)
    {
    case MSG_DRAW:
        {
            gboolean disabled;
            const int *colors;

            colors = widget_get_colors (w);

            disabled = widget_get_state (w, WST_DISABLED);
            tty_setcolor (disabled ? DISABLED_COLOR : colors[DLG_COLOR_NORMAL]);
            tty_draw_box (w->y, w->x, w->lines, w->cols, TRUE);

            if (g->title != NULL)
            {
                tty_setcolor (disabled ? DISABLED_COLOR : colors[DLG_COLOR_TITLE]);
                widget_gotoyx (w, 0, 1);
                tty_print_string (g->title);
            }
            return MSG_HANDLED;
        }

    case MSG_DESTROY:
        g_free (g->title);
        return MSG_HANDLED;

    default:
        return widget_default_callback (w, sender, msg, parm, data);
    }
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

WGroupbox *
groupbox_new (int y, int x, int height, int width, const char *title)
{
    WGroupbox *g;
    Widget *w;

    g = g_new (WGroupbox, 1);
    w = WIDGET (g);
    widget_init (w, y, x, height, width, groupbox_callback, NULL);

    g->title = NULL;
    groupbox_set_title (g, title);

    return g;
}

/* --------------------------------------------------------------------------------------------- */

void
groupbox_set_title (WGroupbox * g, const char *title)
{
    MC_PTR_FREE (g->title);

    /* Strip existing spaces, add one space before and after the title */
    if (title != NULL && *title != '\0')
    {
        char *t;

        t = g_strstrip (g_strdup (title));
        g->title = g_strconcat (" ", t, " ", (char *) NULL);
        g_free (t);
    }

    widget_draw (WIDGET (g));
}

/* --------------------------------------------------------------------------------------------- */
