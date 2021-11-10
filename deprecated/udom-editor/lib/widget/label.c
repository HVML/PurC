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

/** \file label.c
 *  \brief Source: WLabel widget
 */

#include <config.h>

#include <stdarg.h>
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

/*** file scope functions ************************************************************************/

static cb_ret_t
label_callback (Widget * w, Widget * sender, widget_msg_t msg, int parm, void *data)
{
    WLabel *l = LABEL (w);

    switch (msg)
    {
    case MSG_DRAW:
        {
            char *p = l->text;
            int y = 0;
            gboolean disabled;
            align_crt_t align;

            if (l->text == NULL)
                return MSG_HANDLED;

            disabled = widget_get_state (w, WST_DISABLED);

            if (l->transparent)
                tty_setcolor (disabled ? DISABLED_COLOR : DEFAULT_COLOR);
            else
            {
                const int *colors;

                colors = widget_get_colors (w);
                tty_setcolor (disabled ? DISABLED_COLOR : colors[DLG_COLOR_NORMAL]);
            }

            align = (w->pos_flags & WPOS_CENTER_HORZ) != 0 ? J_CENTER_LEFT : J_LEFT;

            while (TRUE)
            {
                char *q;
                char c = '\0';


                q = strchr (p, '\n');
                if (q != NULL)
                {
                    c = q[0];
                    q[0] = '\0';
                }

                widget_gotoyx (w, y, 0);
                tty_print_string (str_fit_to_term (p, w->cols, align));

                if (q == NULL)
                    break;

                q[0] = c;
                p = q + 1;
                y++;
            }
            return MSG_HANDLED;
        }

    case MSG_DESTROY:
        g_free (l->text);
        return MSG_HANDLED;

    default:
        return widget_default_callback (w, sender, msg, parm, data);
    }
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

WLabel *
label_new (int y, int x, const char *text)
{
    WLabel *l;
    Widget *w;
    int cols = 1;
    int lines = 1;

    if (text != NULL)
        str_msg_term_size (text, &lines, &cols);

    l = g_new (WLabel, 1);
    w = WIDGET (l);
    widget_init (w, y, x, lines, cols, label_callback, NULL);

    l->text = g_strdup (text);
    l->auto_adjust_cols = TRUE;
    l->transparent = FALSE;

    return l;
}

/* --------------------------------------------------------------------------------------------- */

void
label_set_text (WLabel * label, const char *text)
{
    Widget *w = WIDGET (label);
    int newcols = w->cols;
    int newlines;

    if (label->text != NULL && text != NULL && strcmp (label->text, text) == 0)
        return;                 /* Flickering is not nice */

    g_free (label->text);

    if (text == NULL)
        label->text = NULL;
    else
    {
        label->text = g_strdup (text);
        if (label->auto_adjust_cols)
        {
            str_msg_term_size (text, &newlines, &newcols);
            if (newcols > w->cols)
                w->cols = newcols;
            if (newlines > w->lines)
                w->lines = newlines;
        }
    }

    widget_draw (w);

    if (newcols < w->cols)
        w->cols = newcols;
}

/* --------------------------------------------------------------------------------------------- */

void
label_set_textv (WLabel * label, const char *format, ...)
{
    va_list args;
    char buf[BUF_1K];           /* FIXME: is it enough? */

    va_start (args, format);
    g_vsnprintf (buf, sizeof (buf), format, args);
    va_end (args);

    label_set_text (label, buf);
}

/* --------------------------------------------------------------------------------------------- */
