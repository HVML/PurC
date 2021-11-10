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

/** \file gauge.c
 *  \brief Source: WGauge widget (progress indicator)
 */

#include <config.h>

#include <stdlib.h>
#include <string.h>

#include "lib/global.h"

#include "lib/tty/tty.h"
#include "lib/tty/color.h"
#include "lib/skin.h"
#include "lib/widget.h"

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/*** file scope functions ************************************************************************/

static cb_ret_t
gauge_callback (Widget * w, Widget * sender, widget_msg_t msg, int parm, void *data)
{
    WGauge *g = GAUGE (w);
    const int *colors;

    switch (msg)
    {
    case MSG_DRAW:
        colors = widget_get_colors (w);
        widget_gotoyx (w, 0, 0);
        if (!g->shown)
        {
            tty_setcolor (colors[DLG_COLOR_NORMAL]);
            tty_printf ("%*s", w->cols, "");
        }
        else
        {
            int gauge_len;
            int percentage, columns;
            int total = g->max;
            int done = g->current;

            if (total <= 0 || done < 0)
            {
                done = 0;
                total = 100;
            }
            if (done > total)
                done = total;
            while (total > 65535)
            {
                total /= 256;
                done /= 256;
            }

            gauge_len = w->cols - 7;    /* 7 positions for percentage */

            percentage = (200 * done / total + 1) / 2;
            columns = (2 * gauge_len * done / total + 1) / 2;
            tty_print_char ('[');
            if (g->from_left_to_right)
            {
                tty_setcolor (GAUGE_COLOR);
                tty_printf ("%*s", columns, "");
                tty_setcolor (colors[DLG_COLOR_NORMAL]);
                tty_printf ("%*s] %3d%%", gauge_len - columns, "", percentage);
            }
            else
            {
                tty_setcolor (colors[DLG_COLOR_NORMAL]);
                tty_printf ("%*s", gauge_len - columns, "");
                tty_setcolor (GAUGE_COLOR);
                tty_printf ("%*s", columns, "");
                tty_setcolor (colors[DLG_COLOR_NORMAL]);
                tty_printf ("] %3d%%", percentage);
            }
        }
        return MSG_HANDLED;

    default:
        return widget_default_callback (w, sender, msg, parm, data);
    }
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

WGauge *
gauge_new (int y, int x, int cols, gboolean shown, int max, int current)
{
    WGauge *g;
    Widget *w;

    g = g_new (WGauge, 1);
    w = WIDGET (g);
    widget_init (w, y, x, 1, cols, gauge_callback, NULL);

    g->shown = shown;
    if (max == 0)
        max = 1;                /* I do not like division by zero :) */
    g->max = max;
    g->current = current;
    g->from_left_to_right = TRUE;

    return g;
}

/* --------------------------------------------------------------------------------------------- */

void
gauge_set_value (WGauge * g, int max, int current)
{
    if (g->current == current && g->max == max)
        return;                 /* Do not flicker */

    if (max == 0)
        max = 1;                /* I do not like division by zero :) */
    g->current = current;
    g->max = max;
    widget_draw (WIDGET (g));
}

/* --------------------------------------------------------------------------------------------- */

void
gauge_show (WGauge * g, gboolean shown)
{
    if (g->shown != shown)
    {
        g->shown = shown;
        widget_draw (WIDGET (g));
    }
}

/* --------------------------------------------------------------------------------------------- */
