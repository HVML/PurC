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
   Andrew Borodin <aborodin@vmail.ru>, 2009-2019

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

/** \file history.c
 *  \brief Source: show history
 */

#include <config.h>

#include <stdlib.h>
#include <sys/types.h>

#include "lib/global.h"

#include "lib/tty/tty.h"        /* LINES, COLS */
#include "lib/strutil.h"
#include "lib/widget.h"
#include "lib/keybind.h"        /* CK_* */

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

#define B_VIEW (B_USER + 1)
#define B_EDIT (B_USER + 2)

/*** file scope type declarations ****************************************************************/

typedef struct
{
    int y;
    int x;
    size_t count;
    size_t max_width;
} history_dlg_data;

/*** file scope variables ************************************************************************/

/*** file scope functions ************************************************************************/

static cb_ret_t
history_dlg_reposition (WDialog * dlg_head)
{
    history_dlg_data *data;
    int x = 0, y, he, wi;
    WRect r;

    /* guard checks */
    if ((dlg_head == NULL) || (dlg_head->data == NULL))
        return MSG_NOT_HANDLED;

    data = (history_dlg_data *) dlg_head->data;

    y = data->y;
    he = data->count + 2;

    if (he <= y || y > (LINES - 6))
    {
        he = MIN (he, y - 1);
        y -= he;
    }
    else
    {
        y++;
        he = MIN (he, LINES - y);
    }

    if (data->x > 2)
        x = data->x - 2;

    wi = data->max_width + 4;

    if ((wi + x) > COLS)
    {
        wi = MIN (wi, COLS);
        x = COLS - wi;
    }

    rect_init (&r, y, x, he, wi);

    return dlg_default_callback (WIDGET (dlg_head), NULL, MSG_RESIZE, 0, &r);
}

/* --------------------------------------------------------------------------------------------- */

static cb_ret_t
history_dlg_callback (Widget * w, Widget * sender, widget_msg_t msg, int parm, void *data)
{
    switch (msg)
    {
    case MSG_RESIZE:
        return history_dlg_reposition (DIALOG (w));

    case MSG_NOTIFY:
        {
            /* message from listbox */
            WDialog *d = DIALOG (w);

            switch (parm)
            {
            case CK_View:
                d->ret_value = B_VIEW;
                break;
            case CK_Edit:
                d->ret_value = B_EDIT;
                break;
            case CK_Enter:
                d->ret_value = B_ENTER;
                break;
            default:
                return MSG_NOT_HANDLED;
            }

            dlg_stop (d);
            return MSG_HANDLED;
        }

    default:
        return dlg_default_callback (w, sender, msg, parm, data);
    }
}

/* --------------------------------------------------------------------------------------------- */

static void
history_create_item (history_descriptor_t * hd, void *data)
{
    char *text = (char *) data;
    size_t width;

    width = str_term_width1 (text);
    hd->max_width = MAX (width, hd->max_width);

    listbox_add_item (hd->listbox, LISTBOX_APPEND_AT_END, 0, text, NULL, TRUE);
}

/* --------------------------------------------------------------------------------------------- */

static void *
history_release_item (history_descriptor_t * hd, WLEntry * le)
{
    void *text;

    (void) hd;

    text = le->text;
    le->text = NULL;

    return text;
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

void
history_descriptor_init (history_descriptor_t * hd, int y, int x, GList * history, int current)
{
    hd->list = history;
    hd->y = y;
    hd->x = x;
    hd->current = current;
    hd->action = CK_IgnoreKey;
    hd->text = NULL;
    hd->max_width = 0;
    hd->listbox = listbox_new (1, 1, 2, 2, TRUE, NULL);
    /* in most cases history list contains string only and no any other data */
    hd->create = history_create_item;
    hd->release = history_release_item;
    hd->free = g_free;
}

/* --------------------------------------------------------------------------------------------- */

void
history_show (history_descriptor_t * hd)
{
    GList *z, *hi;
    size_t count;
    WDialog *query_dlg;
    history_dlg_data hist_data;
    int dlg_ret;

    if (hd == NULL || hd->list == NULL)
        return;

    hd->max_width = str_term_width1 (_("History")) + 2;

    for (z = hd->list; z != NULL; z = g_list_previous (z))
        hd->create (hd, z->data);
    /* after this, the order of history items is following: recent at begin, oldest at end */

    count = listbox_get_length (hd->listbox);

    hist_data.y = hd->y;
    hist_data.x = hd->x;
    hist_data.count = count;
    hist_data.max_width = hd->max_width;

    query_dlg =
        dlg_create (TRUE, 0, 0, 4, 4, WPOS_KEEP_DEFAULT, TRUE, dialog_colors, history_dlg_callback,
                    NULL, "[History-query]", _("History"));
    query_dlg->data = &hist_data;

    /* this call makes list stick to all sides of dialog, effectively make
       it be resized with dialog */
    group_add_widget_autopos (GROUP (query_dlg), hd->listbox, WPOS_KEEP_ALL, NULL);

    /* to avoid diplicating of (calculating sizes in two places)
       code, call history_dlg_callback function here, to set dialog and
       controls positions.
       The main idea - create 4x4 dialog and add 2x2 list in
       center of it, and let dialog function resize it to needed size. */
    send_message (query_dlg, NULL, MSG_RESIZE, 0, NULL);

    if (WIDGET (query_dlg)->y < hd->y)
    {
        /* history is above base widget -- revert order to place recent item at bottom */
        /* revert history direction */
        g_queue_reverse (hd->listbox->list);
        if (hd->current < 0 || (size_t) hd->current >= count)
            listbox_select_last (hd->listbox);
        else
            listbox_select_entry (hd->listbox, count - 1 - (size_t) hd->current);
    }
    else
    {
        /* history is below base widget -- keep order to place recent item on top  */
        if (hd->current > 0)
            listbox_select_entry (hd->listbox, hd->current);
    }

    dlg_ret = dlg_run (query_dlg);
    if (dlg_ret != B_CANCEL)
    {
        char *q;

        switch (dlg_ret)
        {
        case B_EDIT:
            hd->action = CK_Edit;
            break;
        case B_VIEW:
            hd->action = CK_View;
            break;
        default:
            hd->action = CK_Enter;
        }

        listbox_get_current (hd->listbox, &q, NULL);
        hd->text = g_strdup (q);
    }

    /* get modified history from dialog */
    z = NULL;
    for (hi = listbox_get_first_link (hd->listbox); hi != NULL; hi = g_list_next (hi))
        /* history is being reverted here again */
        z = g_list_prepend (z, hd->release (hd, LENTRY (hi->data)));

    /* restore history direction */
    if (WIDGET (query_dlg)->y < hd->y)
        z = g_list_reverse (z);

    widget_destroy (WIDGET (query_dlg));

    hd->list = g_list_first (hd->list);
    g_list_free_full (hd->list, hd->free);
    hd->list = g_list_last (z);
}

/* --------------------------------------------------------------------------------------------- */
