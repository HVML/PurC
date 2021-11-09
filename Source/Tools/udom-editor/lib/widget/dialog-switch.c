/*
   Support of multiply editors and viewers.

   Original idea and code: Oleg "Olegarch" Konovalov <olegarch@linuxinside.com>

   Copyright (C) 2009-2021
   Free Software Foundation, Inc.

   Written by:
   Daniel Borca <dborca@yahoo.com>, 2007
   Andrew Borodin <aborodin@vmail.ru>, 2010, 2013

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

/** \file dialog-switch.c
 *  \brief Source: support of multiply editors and viewers.
 */

#include <config.h>

#include "lib/global.h"
#include "lib/tty/tty.h"        /* LINES, COLS */
#include "lib/tty/color.h"      /* tty_set_normal_attrs() */
#include "lib/widget.h"
#include "lib/event.h"

/*** global variables ****************************************************************************/

WDialog *filemanager = NULL;

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/* List of dialogs: filemanagers, editors, viewers */
static GList *mc_dialogs = NULL;
/* Currently active dialog */
static GList *mc_current = NULL;
/* Is there any dialogs that we have to run after returning to the manager from another dialog */
static gboolean dialog_switch_pending = FALSE;

/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

static unsigned char
get_hotkey (int n)
{
    return (n <= 9) ? '0' + n : 'a' + n - 10;
}

/* --------------------------------------------------------------------------------------------- */

static void
dialog_switch_suspend (void *data, void *user_data)
{
    (void) user_data;

    if (data != mc_current->data)
        widget_set_state (WIDGET (data), WST_SUSPENDED, TRUE);
}

/* --------------------------------------------------------------------------------------------- */

static void
dialog_switch_goto (GList * dlg)
{
    if (mc_current != dlg)
    {
        WDialog *old = DIALOG (mc_current->data);

        mc_current = dlg;

        if (old == filemanager)
        {
            /* switch from panels to another dialog (editor, viewer, etc) */
            dialog_switch_pending = TRUE;
            dialog_switch_process_pending ();
        }
        else
        {
            /* switch from editor, viewer, etc to another dialog */
            widget_set_state (WIDGET (old), WST_SUSPENDED, TRUE);

            if (DIALOG (dlg->data) != filemanager)
                /* switch to another editor, viewer, etc */
                /* return to panels before run the required dialog */
                dialog_switch_pending = TRUE;
            else
            {
                /* switch to panels */
                widget_set_state (WIDGET (filemanager), WST_ACTIVE, TRUE);
                do_refresh ();
            }
        }
    }
}

/* --------------------------------------------------------------------------------------------- */

static void
dialog_switch_resize (WDialog * d)
{
    if (widget_get_state (WIDGET (d), WST_ACTIVE))
        send_message (d, NULL, MSG_RESIZE, 0, NULL);
    else
        GROUP (d)->winch_pending = TRUE;
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

void
dialog_switch_add (WDialog * h)
{
    GList *dlg;

    dlg = g_list_find (mc_dialogs, h);

    if (dlg != NULL)
        mc_current = dlg;
    else
    {
        mc_dialogs = g_list_prepend (mc_dialogs, h);
        mc_current = mc_dialogs;
    }

    /* suspend forced all other screens */
    g_list_foreach (mc_dialogs, dialog_switch_suspend, NULL);
}

/* --------------------------------------------------------------------------------------------- */

void
dialog_switch_remove (WDialog * h)
{
    GList *this;

    if (DIALOG (mc_current->data) == h)
        this = mc_current;
    else
        this = g_list_find (mc_dialogs, h);

    mc_dialogs = g_list_delete_link (mc_dialogs, this);

    /* adjust current dialog */
    if (top_dlg != NULL)
        mc_current = g_list_find (mc_dialogs, DIALOG (top_dlg->data));
    else
        mc_current = mc_dialogs;

    /* resume forced the current screen */
    if (mc_current != NULL)
        widget_set_state (WIDGET (mc_current->data), WST_ACTIVE, TRUE);
}

/* --------------------------------------------------------------------------------------------- */

size_t
dialog_switch_num (void)
{
    return g_list_length (mc_dialogs);
}

/* --------------------------------------------------------------------------------------------- */

void
dialog_switch_next (void)
{
    GList *next;

    if (mc_global.midnight_shutdown || mc_current == NULL)
        return;

    next = g_list_next (mc_current);
    if (next == NULL)
        next = mc_dialogs;

    dialog_switch_goto (next);
}

/* --------------------------------------------------------------------------------------------- */

void
dialog_switch_prev (void)
{
    GList *prev;

    if (mc_global.midnight_shutdown || mc_current == NULL)
        return;

    prev = g_list_previous (mc_current);
    if (prev == NULL)
        prev = g_list_last (mc_dialogs);

    dialog_switch_goto (prev);
}

/* --------------------------------------------------------------------------------------------- */

void
dialog_switch_list (void)
{
    const size_t dlg_num = g_list_length (mc_dialogs);
    int lines, cols;
    Listbox *listbox;
    GList *h, *selected;
    int i = 0;

    if (mc_global.midnight_shutdown || mc_current == NULL)
        return;

    lines = MIN ((size_t) (LINES * 2 / 3), dlg_num);
    cols = COLS * 2 / 3;

    listbox = create_listbox_window (lines, cols, _("Screens"), "[Screen selector]");

    for (h = mc_dialogs; h != NULL; h = g_list_next (h))
    {
        WDialog *dlg = DIALOG (h->data);
        char *title;

        if (dlg->get_title != NULL)
            title = dlg->get_title (dlg, WIDGET (listbox->list)->cols - 2);
        else
            title = g_strdup ("");

        listbox_add_item (listbox->list, LISTBOX_APPEND_BEFORE, get_hotkey (i++), title, h, FALSE);

        g_free (title);
    }

    selected = run_listbox_with_data (listbox, mc_current);
    if (selected != NULL)
        dialog_switch_goto (selected);
}

/* --------------------------------------------------------------------------------------------- */

int
dialog_switch_process_pending (void)
{
    int ret = 0;

    while (dialog_switch_pending)
    {
        WDialog *h = DIALOG (mc_current->data);
        Widget *wh = WIDGET (h);

        dialog_switch_pending = FALSE;
        widget_set_state (wh, WST_SUSPENDED, TRUE);
        ret = dlg_run (h);
        if (widget_get_state (wh, WST_CLOSED))
        {
            widget_destroy (wh);

            /* return to panels */
            if (mc_global.mc_run_mode == MC_RUN_FULL)
            {
                mc_current = g_list_find (mc_dialogs, filemanager);
                mc_event_raise (MCEVENT_GROUP_FILEMANAGER, "update_panels", NULL);
            }
        }
    }

    repaint_screen ();

    return ret;
}

/* --------------------------------------------------------------------------------------------- */

void
dialog_switch_got_winch (void)
{
    GList *dlg;

    for (dlg = mc_dialogs; dlg != NULL; dlg = g_list_next (dlg))
        if (dlg != mc_current)
            GROUP (dlg->data)->winch_pending = TRUE;
}

/* --------------------------------------------------------------------------------------------- */

void
dialog_switch_shutdown (void)
{
    while (mc_dialogs != NULL)
    {
        WDialog *dlg = DIALOG (mc_dialogs->data);

        dlg_run (dlg);
        widget_destroy (WIDGET (dlg));
    }
}

/* --------------------------------------------------------------------------------------------- */

void
repaint_screen (void)
{
    do_refresh ();
    tty_refresh ();
}

/* --------------------------------------------------------------------------------------------- */

void
mc_refresh (void)
{
#ifdef ENABLE_BACKGROUND
    if (mc_global.we_are_background)
        return;
#endif /* ENABLE_BACKGROUND */

    if (!tty_got_winch ())
        tty_refresh ();
    else
    {
        /* if winch was caugth, we should do not only redraw screen, but
           reposition/resize all */
        dialog_change_screen_size ();
    }
}

/* --------------------------------------------------------------------------------------------- */

void
dialog_change_screen_size (void)
{
    GList *d;

    tty_flush_winch ();
    tty_change_screen_size ();

#ifdef HAVE_SLANG
    tty_keypad (TRUE);
    tty_nodelay (FALSE);
#endif

    /* Inform all suspending dialogs */
    dialog_switch_got_winch ();

    /* Inform all running dialogs from first to last */
    for (d = g_list_last (top_dlg); d != NULL; d = g_list_previous (d))
        dialog_switch_resize (DIALOG (d->data));

    /* Now, force the redraw */
    repaint_screen ();
}

/* --------------------------------------------------------------------------------------------- */
