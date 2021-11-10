/*
   Panel managing.

   Copyright (C) 1994-2021
   Free Software Foundation, Inc.

   Written by:
   Miguel de Icaza, 1995
   Timur Bakeyev, 1997, 1999
   Slava Zanko <slavazanko@gmail.com>, 2013
   Andrew Borodin <aborodin@vmail.ru>, 2013-2016

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

/** \file panel.c
 *  \brief Source: panel managin module
 */

#include <config.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib/global.h"

#include "lib/tty/tty.h"
#include "lib/tty/key.h"        /* XCTRL and ALT macros  */
#include "lib/skin.h"
#include "lib/strescape.h"
#include "lib/mcconfig.h"
#include "lib/vfs/vfs.h"
#include "lib/unixcompat.h"
#include "lib/search.h"
#include "lib/timefmt.h"        /* file_date() */
#include "lib/util.h"
#include "lib/widget.h"
#include "lib/event.h"

#include "src/setup.h"          /* For loading/saving panel options */
#include "src/execute.h"
#include "src/keymap.h"         /* global_keymap_t */
#include "src/subshell/subshell.h"      /* do_subshell_chdir() */

#include "src/usermenu.h"

#include "command.h"            /* cmdline */
#include "udommanager.h"

#include "panel.h"

/*** global variables ****************************************************************************/

/* --------------------------------------------------------------------------------------------- */
/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------------------------- */

static void
panel_destroy (WPanel * p)
{
    // VW TODO
    (void) p;
}

static cb_ret_t
panel_callback (Widget * w, Widget * sender, widget_msg_t msg, int parm, void *data)
{
    WPanel *panel = PANEL (w);

    switch (msg)
    {
    case MSG_INIT:
        return MSG_HANDLED;

    case MSG_DRAW:
        /* Repaint everything, including frame and separator */
        widget_erase (w);
#if 0 // VW: TODO
        show_dir (panel);
        panel_print_header (panel);
        adjust_top_file (panel);
        paint_dir (panel);
        mini_info_separator (panel);
        display_mini_info (panel);
#endif
        panel->dirty = FALSE;
        return MSG_HANDLED;

    case MSG_FOCUS:
        current_panel = panel;
        panel->active = TRUE;

        update_xterm_title_path ();
        return MSG_HANDLED;

    case MSG_UNFOCUS:
        panel->active = FALSE;
        return MSG_HANDLED;

    case MSG_DESTROY:
        panel_destroy (panel);
        return MSG_HANDLED;

    default:
        return widget_default_callback (w, sender, msg, parm, data);
    }
}

/* --------------------------------------------------------------------------------------------- */

static void
panel_mouse_callback (Widget * w, mouse_msg_t msg, mouse_event_t * event)
{
    WPanel *panel = PANEL (w);
    gboolean is_active;

    (void) event;
    is_active = widget_is_active (w);

    switch (msg)
    {
    case MSG_MOUSE_DOWN:
        break;

    case MSG_MOUSE_DRAG:
        break;

    case MSG_MOUSE_UP:
        break;

    case MSG_MOUSE_CLICK:
        break;

    case MSG_MOUSE_MOVE:
        break;

    case MSG_MOUSE_SCROLL_UP:
        break;

    case MSG_MOUSE_SCROLL_DOWN:
        break;

    default:
        break;
    }

    if (panel->dirty)
        widget_draw (w);
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

/**
 * Creatie an empty panel with specified size.
 *
 * @param panel_name name of panel for setup retieving
 *
 * @return new instance of WPanel
 */

WPanel *
panel_sized_empty_new (const char *panel_name, int y, int x, int lines, int cols)
{
    WPanel *panel;
    Widget *w;

    panel = g_new0 (WPanel, 1);
    w = WIDGET (panel);
    widget_init (w, y, x, lines, cols, panel_callback, panel_mouse_callback);
    w->options |= WOP_SELECTABLE | WOP_TOP_SELECT;
    w->keymap = panel_map;

    panel->name = g_strdup (panel_name);

    return panel;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Panel creation for specified size and directory.
 *
 * @param panel_name name of panel for setup retieving
 * @param y y coordinate of top-left corner
 * @param x x coordinate of top-left corner
 * @param lines vertical size
 * @param cols horizontal size
 * @param vpath working panel directory. If NULL then current directory is used
 *
 * @return new instance of WPanel
 */

void
panel_init (void)
{
}

/* --------------------------------------------------------------------------------------------- */

void
panel_deinit (void)
{
}

/* --------------------------------------------------------------------------------------------- */
