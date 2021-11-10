/*
   Widgets for the Midnight Commander

   Copyright (C) 2020-2021
   The Free Software Foundation, Inc.

   Authors:
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

/** \file frame.c
 *  \brief Source: WFrame widget (frame of dialogs)
 */

#include <config.h>

#include <stdlib.h>

#include "lib/global.h"
#include "lib/tty/tty.h"
#include "lib/tty/color.h"
#include "lib/skin.h"
#include "lib/strutil.h"
#include "lib/util.h"           /* MC_PTR_FREE */
#include "lib/widget.h"

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/* --------------------------------------------------------------------------------------------- */
/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

static void
frame_adjust (WFrame * f)
{
    Widget *w = WIDGET (f);
    Widget *wo = WIDGET (w->owner);

    w->y = wo->y;
    w->x = wo->x;
    w->lines = wo->lines;
    w->cols = wo->cols;

    w->pos_flags |= WPOS_KEEP_ALL;
}

/* --------------------------------------------------------------------------------------------- */

static void
frame_draw (const WFrame * f)
{
    const Widget *w = CONST_WIDGET (f);
    int d = f->compact ? 0 : 1;
    const int *colors;

    colors = widget_get_colors (w);

    if (mc_global.tty.shadows)
        tty_draw_box_shadow (w->y, w->x, w->lines, w->cols, SHADOW_COLOR);

    tty_setcolor (colors[FRAME_COLOR_NORMAL]);
    tty_fill_region (w->y, w->x, w->lines, w->cols, ' ');
    tty_draw_box (w->y + d, w->x + d, w->lines - 2 * d, w->cols - 2 * d, f->single);

    if (f->title != NULL)
    {
        /* TODO: truncate long title */
        tty_setcolor (colors[FRAME_COLOR_TITLE]);
        widget_gotoyx (w, d, (w->cols - str_term_width1 (f->title)) / 2);
        tty_print_string (f->title);
    }
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

WFrame *
frame_new (int y, int x, int lines, int cols, const char *title, gboolean single, gboolean compact)
{
    WFrame *f;
    Widget *w;

    f = g_new (WFrame, 1);
    w = WIDGET (f);
    widget_init (w, y, x, lines, cols, frame_callback, NULL);

    f->single = single;
    f->compact = compact;

    f->title = NULL;
    frame_set_title (f, title);

    return f;
}

/* --------------------------------------------------------------------------------------------- */

cb_ret_t
frame_callback (Widget * w, Widget * sender, widget_msg_t msg, int parm, void *data)
{
    WFrame *f = FRAME (w);

    switch (msg)
    {
    case MSG_INIT:
        frame_adjust (f);
        return MSG_HANDLED;

    case MSG_DRAW:
        frame_draw (f);
        return MSG_HANDLED;

    case MSG_DESTROY:
        g_free (f->title);
        return MSG_HANDLED;

    default:
        return widget_default_callback (w, sender, msg, parm, data);
    }
}

/* --------------------------------------------------------------------------------------------- */

void
frame_set_title (WFrame * f, const char *title)
{
    MC_PTR_FREE (f->title);

    /* Strip existing spaces, add one space before and after the title */
    if (title != NULL && *title != '\0')
    {
        char *t;

        t = g_strstrip (g_strdup (title));
        if (*t != '\0')
            f->title = g_strdup_printf (" %s ", t);
        g_free (t);
    }

    widget_draw (WIDGET (f));
}

/* --------------------------------------------------------------------------------------------- */
