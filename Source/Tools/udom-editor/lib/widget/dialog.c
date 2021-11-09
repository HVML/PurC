/*
   Dialog box features module for the Midnight Commander

   Copyright (C) 1994-2021
   Free Software Foundation, Inc.

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

/** \file dialog.c
 *  \brief Source: dialog box features module
 */

#include <config.h>

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "lib/global.h"

#include "lib/tty/tty.h"
#include "lib/skin.h"
#include "lib/tty/key.h"
#include "lib/strutil.h"
#include "lib/fileloc.h"        /* MC_HISTORY_FILE */
#include "lib/event.h"          /* mc_event_raise() */
#include "lib/util.h"           /* MC_PTR_FREE */
#include "lib/mcconfig.h"       /* num_history_items_recorded */

#include "lib/widget.h"
#include "lib/widget/mouse.h"

/*** global variables ****************************************************************************/

/* Color styles for normal and error dialogs */
dlg_colors_t dialog_colors;
dlg_colors_t alarm_colors;
dlg_colors_t listbox_colors;

/* Primitive way to check if the the current dialog is our dialog */
/* This is needed by async routines like load_prompt */
GList *top_dlg = NULL;

/* A hook list for idle events */
hook_t *idle_hook = NULL;

/* If set then dialogs just clean the screen when refreshing, else */
/* they do a complete refresh, refreshing all the parts of the program */
gboolean fast_refresh = FALSE;

/* left click outside of dialog closes it */
gboolean mouse_close_dialog = FALSE;

const global_keymap_t *dialog_map = NULL;

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/* --------------------------------------------------------------------------------------------- */
/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

static const int *
dlg_default_get_colors (const Widget * w)
{
    return CONST_DIALOG (w)->colors;
}

/* --------------------------------------------------------------------------------------------- */
/**
  * Read histories from the ${XDG_CACHE_HOME}/mc/history file
  */
static void
dlg_read_history (WDialog * h)
{
    char *profile;
    ev_history_load_save_t event_data;

    if (num_history_items_recorded == 0)        /* this is how to disable */
        return;

    profile = mc_config_get_full_path (MC_HISTORY_FILE);
    event_data.cfg = mc_config_init (profile, TRUE);
    event_data.receiver = NULL;

    /* create all histories in dialog */
    mc_event_raise (h->event_group, MCEVENT_HISTORY_LOAD, &event_data);

    mc_config_deinit (event_data.cfg);
    g_free (profile);
}

/* --------------------------------------------------------------------------------------------- */

static void
refresh_cmd (void)
{
#ifdef HAVE_SLANG
    tty_touch_screen ();
    mc_refresh ();
#else
    /* Use this if the refreshes fail */
    tty_clear_screen ();
    repaint_screen ();
#endif /* HAVE_SLANG */
}

/* --------------------------------------------------------------------------------------------- */

static cb_ret_t
dlg_execute_cmd (WDialog * h, long command)
{
    WGroup *g = GROUP (h);
    cb_ret_t ret = MSG_HANDLED;

    if (send_message (h, NULL, MSG_ACTION, command, NULL) == MSG_HANDLED)
        return MSG_HANDLED;

    switch (command)
    {
    case CK_Ok:
        h->ret_value = B_ENTER;
        dlg_stop (h);
        break;
    case CK_Cancel:
        h->ret_value = B_CANCEL;
        dlg_stop (h);
        break;

    case CK_Up:
    case CK_Left:
        group_select_prev_widget (g);
        break;
    case CK_Down:
    case CK_Right:
        group_select_next_widget (g);
        break;

    case CK_Help:
        {
            ev_help_t event_data = { NULL, h->help_ctx };
            mc_event_raise (MCEVENT_GROUP_CORE, "help", &event_data);
        }
        break;

    case CK_Suspend:
        mc_event_raise (MCEVENT_GROUP_CORE, "suspend", NULL);
        refresh_cmd ();
        break;
    case CK_Refresh:
        refresh_cmd ();
        break;

    case CK_ScreenList:
        if (!widget_get_state (WIDGET (h), WST_MODAL))
            dialog_switch_list ();
        else
            ret = MSG_NOT_HANDLED;
        break;
    case CK_ScreenNext:
        if (!widget_get_state (WIDGET (h), WST_MODAL))
            dialog_switch_next ();
        else
            ret = MSG_NOT_HANDLED;
        break;
    case CK_ScreenPrev:
        if (!widget_get_state (WIDGET (h), WST_MODAL))
            dialog_switch_prev ();
        else
            ret = MSG_NOT_HANDLED;
        break;

    default:
        ret = MSG_NOT_HANDLED;
    }

    return ret;
}

/* --------------------------------------------------------------------------------------------- */

static cb_ret_t
dlg_handle_key (WDialog * h, int d_key)
{
    long command;

    command = widget_lookup_key (WIDGET (h), d_key);
    if (command == CK_IgnoreKey)
        command = keybind_lookup_keymap_command (dialog_map, d_key);
    if (command != CK_IgnoreKey)
        return dlg_execute_cmd (h, command);

    return MSG_NOT_HANDLED;
}

/* --------------------------------------------------------------------------------------------- */

static void
dlg_key_event (WDialog * h, int d_key)
{
    Widget *w = WIDGET (h);
    WGroup *g = GROUP (h);
    cb_ret_t handled;

    if (g->widgets == NULL)
        return;

    if (g->current == NULL)
        g->current = g->widgets;

    /* TAB used to cycle */
    if (!widget_get_options (w, WOP_WANT_TAB))
    {
        if (d_key == '\t')
        {
            group_select_next_widget (g);
            return;
        }
        else if ((d_key & ~(KEY_M_SHIFT | KEY_M_CTRL)) == '\t')
        {
            group_select_prev_widget (g);
            return;
        }
    }

    /* first can dlalog handle the key itself */
    handled = send_message (h, NULL, MSG_KEY, d_key, NULL);

    if (handled == MSG_NOT_HANDLED)
        handled = group_default_callback (w, NULL, MSG_KEY, d_key, NULL);

    if (handled == MSG_NOT_HANDLED)
        handled = dlg_handle_key (h, d_key);

    (void) handled;
    send_message (h, NULL, MSG_POST_KEY, d_key, NULL);
}

/* --------------------------------------------------------------------------------------------- */

static int
dlg_handle_mouse_event (Widget * w, Gpm_Event * event)
{
    if (w->mouse_callback != NULL)
    {
        int mou;

        mou = mouse_handle_event (w, event);
        if (mou != MOU_UNHANDLED)
            return mou;
    }

    return group_handle_mouse_event (w, event);
}

/* --------------------------------------------------------------------------------------------- */

static void
frontend_dlg_run (WDialog * h)
{
    Widget *wh = WIDGET (h);
    Gpm_Event event;

    event.x = -1;

    /* close opened editors, viewers, etc */
    if (!widget_get_state (wh, WST_MODAL) && mc_global.midnight_shutdown)
    {
        send_message (h, NULL, MSG_VALIDATE, 0, NULL);
        return;
    }

    while (widget_get_state (wh, WST_ACTIVE))
    {
        int d_key;

        if (tty_got_winch ())
            dialog_change_screen_size ();

        if (is_idle ())
        {
            if (idle_hook)
                execute_hooks (idle_hook);

            while (widget_get_state (wh, WST_IDLE) && is_idle ())
                send_message (wh, NULL, MSG_IDLE, 0, NULL);

            /* Allow terminating the dialog from the idle handler */
            if (!widget_get_state (wh, WST_ACTIVE))
                break;
        }

        widget_update_cursor (wh);

        /* Clear interrupt flag */
        tty_got_interrupt ();
        d_key = tty_get_event (&event, GROUP (h)->mouse_status == MOU_REPEAT, TRUE);

        dlg_process_event (h, d_key, &event);

        if (widget_get_state (wh, WST_CLOSED))
            send_message (h, NULL, MSG_VALIDATE, 0, NULL);
    }
}

/* --------------------------------------------------------------------------------------------- */

static void
dlg_default_destroy (Widget * w)
{
    WDialog *h = DIALOG (w);

    /* if some widgets have history, save all histories at one moment here */
    dlg_save_history (h);
    group_default_callback (w, NULL, MSG_DESTROY, 0, NULL);
    mc_event_group_del (h->event_group);
    g_free (h->event_group);
    g_free (h);

    do_refresh ();
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */
/** Default dialog callback */

cb_ret_t
dlg_default_callback (Widget * w, Widget * sender, widget_msg_t msg, int parm, void *data)
{
    switch (msg)
    {
    case MSG_INIT:
        /* nothing to init in dialog itself */
        return MSG_HANDLED;

    case MSG_IDLE:
        /* we don't want endless loop */
        widget_idle (w, FALSE);
        return MSG_HANDLED;

    case MSG_DESTROY:
        /* nothing to deinit in dialog itself */
        return MSG_HANDLED;

    default:
        return group_default_callback (w, sender, msg, parm, data);
    }
}

/* --------------------------------------------------------------------------------------------- */

void
dlg_default_mouse_callback (Widget * w, mouse_msg_t msg, mouse_event_t * event)
{
    switch (msg)
    {
    case MSG_MOUSE_CLICK:
        if (event->y < 0 || event->y >= w->lines || event->x < 0 || event->x >= w->cols)
        {
            DIALOG (w)->ret_value = B_CANCEL;
            dlg_stop (DIALOG (w));
        }
        break;

    default:
        /* return MOU_UNHANDLED */
        event->result.abort = TRUE;
        break;
    }
}

/* --------------------------------------------------------------------------------------------- */

WDialog *
dlg_create (gboolean modal, int y1, int x1, int lines, int cols, widget_pos_flags_t pos_flags,
            gboolean compact, const int *colors, widget_cb_fn callback,
            widget_mouse_cb_fn mouse_callback, const char *help_ctx, const char *title)
{
    WDialog *new_d;
    Widget *w;
    WGroup *g;

    new_d = g_new0 (WDialog, 1);
    w = WIDGET (new_d);
    g = GROUP (new_d);
    widget_adjust_position (pos_flags, &y1, &x1, &lines, &cols);
    group_init (g, y1, x1, lines, cols, callback != NULL ? callback : dlg_default_callback,
                mouse_callback != NULL ? mouse_callback : dlg_default_mouse_callback);

    w->pos_flags = pos_flags;
    w->options |= WOP_SELECTABLE | WOP_TOP_SELECT;
    w->state |= WST_FOCUSED;
    /* Temporary hack: dialog doesn't have an owner, own itself. */
    w->owner = g;

    w->keymap = dialog_map;

    w->mouse_handler = dlg_handle_mouse_event;
    w->mouse.forced_capture = mouse_close_dialog && (w->pos_flags & WPOS_FULLSCREEN) == 0;

    w->destroy = dlg_default_destroy;
    w->get_colors = dlg_default_get_colors;

    new_d->colors = colors;
    new_d->help_ctx = help_ctx;
    new_d->compact = compact;
    new_d->data = NULL;

    if (modal)
    {
        w->state |= WST_MODAL;

        new_d->bg = WIDGET (frame_new (0, 0, w->lines, w->cols, title, FALSE, new_d->compact));
        group_add_widget (g, new_d->bg);
        frame_set_title (FRAME (new_d->bg), title);
    }

    /* unique name of event group for this dialog */
    new_d->event_group = g_strdup_printf ("%s_%p", MCEVENT_GROUP_DIALOG, (void *) new_d);

    return new_d;
}

/* --------------------------------------------------------------------------------------------- */

void
dlg_set_default_colors (void)
{
    dialog_colors[DLG_COLOR_NORMAL] = COLOR_NORMAL;
    dialog_colors[DLG_COLOR_FOCUS] = COLOR_FOCUS;
    dialog_colors[DLG_COLOR_HOT_NORMAL] = COLOR_HOT_NORMAL;
    dialog_colors[DLG_COLOR_HOT_FOCUS] = COLOR_HOT_FOCUS;
    dialog_colors[DLG_COLOR_TITLE] = COLOR_TITLE;

    alarm_colors[DLG_COLOR_NORMAL] = ERROR_COLOR;
    alarm_colors[DLG_COLOR_FOCUS] = ERROR_FOCUS;
    alarm_colors[DLG_COLOR_HOT_NORMAL] = ERROR_HOT_NORMAL;
    alarm_colors[DLG_COLOR_HOT_FOCUS] = ERROR_HOT_FOCUS;
    alarm_colors[DLG_COLOR_TITLE] = ERROR_TITLE;

    listbox_colors[DLG_COLOR_NORMAL] = PMENU_ENTRY_COLOR;
    listbox_colors[DLG_COLOR_FOCUS] = PMENU_SELECTED_COLOR;
    listbox_colors[DLG_COLOR_HOT_NORMAL] = PMENU_ENTRY_COLOR;
    listbox_colors[DLG_COLOR_HOT_FOCUS] = PMENU_SELECTED_COLOR;
    listbox_colors[DLG_COLOR_TITLE] = PMENU_TITLE_COLOR;
}

/* --------------------------------------------------------------------------------------------- */

void
do_refresh (void)
{
    GList *d = top_dlg;

    if (fast_refresh)
    {
        if (d != NULL)
            widget_draw (WIDGET (d->data));
    }
    else
    {
        /* Search first fullscreen dialog */
        for (; d != NULL; d = g_list_next (d))
            if ((WIDGET (d->data)->pos_flags & WPOS_FULLSCREEN) != 0)
                break;

        /* when small dialog (i.e. error message) is created first,
           there is no fullscreen dialog in the stack */
        if (d == NULL)
            d = g_list_last (top_dlg);

        /* back to top dialog */
        for (; d != NULL; d = g_list_previous (d))
            widget_draw (WIDGET (d->data));
    }
}

/* --------------------------------------------------------------------------------------------- */

void
dlg_stop (WDialog * h)
{
    widget_set_state (WIDGET (h), WST_CLOSED, TRUE);
}

/* --------------------------------------------------------------------------------------------- */
/** Init the process */

void
dlg_init (WDialog * h)
{
    WGroup *g = GROUP (h);
    Widget *wh = WIDGET (h);

    if (top_dlg != NULL && widget_get_state (WIDGET (top_dlg->data), WST_MODAL))
        widget_set_state (wh, WST_MODAL, TRUE);

    /* add dialog to the stack */
    top_dlg = g_list_prepend (top_dlg, h);

    /* Initialize dialog manager and widgets */
    if (widget_get_state (wh, WST_CONSTRUCT))
    {
        if (!widget_get_state (wh, WST_MODAL))
            dialog_switch_add (h);

        send_message (h, NULL, MSG_INIT, 0, NULL);
        group_default_callback (wh, NULL, MSG_INIT, 0, NULL);
        dlg_read_history (h);
    }

    /* Select the first widget that takes focus */
    while (g->current != NULL && !widget_is_focusable (g->current->data))
        group_set_current_widget_next (g);

    widget_set_state (wh, WST_ACTIVE, TRUE);
    /* draw dialog and focus found widget */
    widget_set_state (wh, WST_FOCUSED, TRUE);

    h->ret_value = 0;
}

/* --------------------------------------------------------------------------------------------- */

void
dlg_process_event (WDialog * h, int key, Gpm_Event * event)
{
    switch (key)
    {
    case EV_NONE:
        if (tty_got_interrupt ())
            dlg_execute_cmd (h, CK_Cancel);
        break;

    case EV_MOUSE:
        {
            Widget *w = WIDGET (h);

            GROUP (h)->mouse_status = w->mouse_handler (w, event);
            break;
        }

    default:
        dlg_key_event (h, key);
        break;
    }
}

/* --------------------------------------------------------------------------------------------- */
/** Shutdown the dlg_run */

void
dlg_run_done (WDialog * h)
{
    top_dlg = g_list_remove (top_dlg, h);

    if (widget_get_state (WIDGET (h), WST_CLOSED))
    {
        send_message (h, GROUP (h)->current == NULL ? NULL : WIDGET (GROUP (h)->current->data),
                      MSG_END, 0, NULL);
        if (!widget_get_state (WIDGET (h), WST_MODAL))
            dialog_switch_remove (h);
    }
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Standard run dialog routine
 * We have to keep this routine small so that we can duplicate it's
 * behavior on complex routines like the file routines, this way,
 * they can call the dlg_process_event without rewriting all the code
 */

int
dlg_run (WDialog * h)
{
    dlg_init (h);
    frontend_dlg_run (h);
    dlg_run_done (h);
    return h->ret_value;
}

/* --------------------------------------------------------------------------------------------- */

/**
  * Write history to the ${XDG_CACHE_HOME}/mc/history file
  */
void
dlg_save_history (WDialog * h)
{
    char *profile;
    int i;

    if (num_history_items_recorded == 0)        /* this is how to disable */
        return;

    profile = mc_config_get_full_path (MC_HISTORY_FILE);
    i = open (profile, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if (i != -1)
        close (i);

    /* Make sure the history is only readable by the user */
    if (chmod (profile, S_IRUSR | S_IWUSR) != -1 || errno == ENOENT)
    {
        ev_history_load_save_t event_data;

        event_data.cfg = mc_config_init (profile, FALSE);
        event_data.receiver = NULL;

        /* get all histories in dialog */
        mc_event_raise (h->event_group, MCEVENT_HISTORY_SAVE, &event_data);

        mc_config_save_file (event_data.cfg, NULL);
        mc_config_deinit (event_data.cfg);
    }

    g_free (profile);
}

/* --------------------------------------------------------------------------------------------- */

char *
dlg_get_title (const WDialog * h, size_t len)
{
    char *t;

    if (h == NULL)
        abort ();

    if (h->get_title != NULL)
        t = h->get_title (h, len);
    else
        t = g_strdup ("");

    return t;
}

/* --------------------------------------------------------------------------------------------- */
