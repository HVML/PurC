/*
   Widget based utility functions.

   Copyright (C) 1994-2021
   Free Software Foundation, Inc.

   Authors:
   Miguel de Icaza, 1994, 1995, 1996
   Radek Doulik, 1994, 1995
   Jakub Jelinek, 1995
   Andrej Borsenkow, 1995
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

/** \file listbox-window.c
 *  \brief Source: Listbox widget, a listbox within dialog window
 */

#include <config.h>

#include <stdlib.h>

#include "lib/global.h"
#include "lib/tty/tty.h"        /* COLS */
#include "lib/skin.h"
#include "lib/strutil.h"        /* str_term_width1() */
#include "lib/widget.h"

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/*** file scope functions ************************************************************************/

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

Listbox *
create_listbox_window_centered (int center_y, int center_x, int lines, int cols,
                                const char *title, const char *help)
{
    const int space = 4;

    int xpos = 0, ypos = 0;
    Listbox *listbox;
    widget_pos_flags_t pos_flags = WPOS_TRYUP;

    /* Adjust sizes */
    lines = MIN (lines, LINES - 6);

    if (title != NULL)
    {
        int len;

        len = str_term_width1 (title) + 4;
        cols = MAX (cols, len);
    }

    cols = MIN (cols, COLS - 6);

    /* adjust position */
    if ((center_y < 0) || (center_x < 0))
        pos_flags |= WPOS_CENTER;
    else
    {
        /* Actually, this this is not used in MC. */

        ypos = center_y;
        xpos = center_x;

        ypos -= lines / 2;
        xpos -= cols / 2;

        if (ypos + lines >= LINES)
            ypos = LINES - lines - space;
        if (ypos < 0)
            ypos = 0;

        if (xpos + cols >= COLS)
            xpos = COLS - cols - space;
        if (xpos < 0)
            xpos = 0;
    }

    listbox = g_new (Listbox, 1);

    listbox->dlg =
        dlg_create (TRUE, ypos, xpos, lines + space, cols + space, pos_flags, FALSE, listbox_colors,
                    NULL, NULL, help, title);

    listbox->list = listbox_new (2, 2, lines, cols, FALSE, NULL);
    group_add_widget (GROUP (listbox->dlg), listbox->list);

    return listbox;
}

/* --------------------------------------------------------------------------------------------- */

Listbox *
create_listbox_window (int lines, int cols, const char *title, const char *help)
{
    return create_listbox_window_centered (-1, -1, lines, cols, title, help);
}

/* --------------------------------------------------------------------------------------------- */

/** Returns the number of the item selected */
int
run_listbox (Listbox * l)
{
    int val = -1;

    if (dlg_run (l->dlg) != B_CANCEL)
        val = l->list->pos;
    widget_destroy (WIDGET (l->dlg));
    g_free (l);
    return val;
}

/* --------------------------------------------------------------------------------------------- */

/**
 * A variant of run_listbox() which is more convenient to use when we
 * need to select arbitrary 'data'.
 *
 * @param select  the item to select initially, by its 'data'. Optional.
 * @return        the 'data' of the item selected, or NULL if none selected.
 */
void *
run_listbox_with_data (Listbox * l, const void *select)
{
    void *val = NULL;

    if (select != NULL)
        listbox_select_entry (l->list, listbox_search_data (l->list, select));

    if (dlg_run (l->dlg) != B_CANCEL)
    {
        WLEntry *e;
        e = listbox_get_nth_item (l->list, l->list->pos);
        if (e != NULL)
        {
            /* The assert guards against returning a soon-to-be deallocated
             * pointer (as in listbox_add_item(..., TRUE)). */
            g_assert (!e->free_data);
            val = e->data;
        }
    }

    widget_destroy (WIDGET (l->dlg));
    g_free (l);
    return val;
}

/* --------------------------------------------------------------------------------------------- */
