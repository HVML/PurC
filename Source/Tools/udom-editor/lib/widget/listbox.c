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

/** \file listbox.c
 *  \brief Source: WListbox widget
 */

#include <config.h>

#include <stdlib.h>

#include "lib/global.h"

#include "lib/tty/tty.h"
#include "lib/skin.h"
#include "lib/strutil.h"
#include "lib/util.h"           /* Q_() */
#include "lib/widget.h"

/*** global variables ****************************************************************************/

const global_keymap_t *listbox_map = NULL;

/*** file scope macro definitions ****************************************************************/

/* Gives the position of the last item. */
#define LISTBOX_LAST(l) (listbox_is_empty (l) ? 0 : (int) g_queue_get_length ((l)->list) - 1)

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/*** file scope functions ************************************************************************/

static int
listbox_entry_cmp (const void *a, const void *b, void *user_data)
{
    const WLEntry *ea = (const WLEntry *) a;
    const WLEntry *eb = (const WLEntry *) b;

    (void) user_data;

    return strcmp (ea->text, eb->text);
}

/* --------------------------------------------------------------------------------------------- */

static void
listbox_entry_free (void *data)
{
    WLEntry *e = data;

    g_free (e->text);
    if (e->free_data)
        g_free (e->data);
    g_free (e);
}

/* --------------------------------------------------------------------------------------------- */

static void
listbox_drawscroll (WListbox * l)
{
    Widget *w = WIDGET (l);
    int max_line = w->lines - 1;
    int line = 0;
    int i;
    int length;

    /* Are we at the top? */
    widget_gotoyx (w, 0, w->cols);
    if (l->top == 0)
        tty_print_one_vline (TRUE);
    else
        tty_print_char ('^');

    length = g_queue_get_length (l->list);

    /* Are we at the bottom? */
    widget_gotoyx (w, max_line, w->cols);
    if (l->top + w->lines == length || w->lines >= length)
        tty_print_one_vline (TRUE);
    else
        tty_print_char ('v');

    /* Now draw the nice relative pointer */
    if (!g_queue_is_empty (l->list))
        line = 1 + ((l->pos * (w->lines - 2)) / length);

    for (i = 1; i < max_line; i++)
    {
        widget_gotoyx (w, i, w->cols);
        if (i != line)
            tty_print_one_vline (TRUE);
        else
            tty_print_char ('*');
    }
}

/* --------------------------------------------------------------------------------------------- */

static void
listbox_draw (WListbox * l, gboolean focused)
{
    Widget *w = WIDGET (l);
    const int *colors;
    gboolean disabled;
    int normalc, selc;
    int length = 0;
    GList *le = NULL;
    int pos;
    int i;
    int sel_line = -1;

    colors = widget_get_colors (w);

    disabled = widget_get_state (w, WST_DISABLED);
    normalc = disabled ? DISABLED_COLOR : colors[DLG_COLOR_NORMAL];
    selc = disabled ? DISABLED_COLOR : colors[focused ? DLG_COLOR_HOT_FOCUS : DLG_COLOR_FOCUS];

    if (l->list != NULL)
    {
        length = g_queue_get_length (l->list);
        le = g_queue_peek_nth_link (l->list, (guint) l->top);
    }

    /*    pos = (le == NULL) ? 0 : g_list_position (l->list, le); */
    pos = (le == NULL) ? 0 : l->top;

    for (i = 0; i < w->lines; i++)
    {
        const char *text = "";

        /* Display the entry */
        if (pos == l->pos && sel_line == -1)
        {
            sel_line = i;
            tty_setcolor (selc);
        }
        else
            tty_setcolor (normalc);

        widget_gotoyx (l, i, 1);

        if (l->list != NULL && le != NULL && (i == 0 || pos < length))
        {
            WLEntry *e = LENTRY (le->data);

            text = e->text;
            le = g_list_next (le);
            pos++;
        }

        tty_print_string (str_fit_to_term (text, w->cols - 2, J_LEFT_FIT));
    }

    l->cursor_y = sel_line;

    if (l->scrollbar && length > w->lines)
    {
        tty_setcolor (normalc);
        listbox_drawscroll (l);
    }
}

/* --------------------------------------------------------------------------------------------- */

static int
listbox_check_hotkey (WListbox * l, int key)
{
    if (!listbox_is_empty (l))
    {
        int i;
        GList *le;

        for (i = 0, le = g_queue_peek_head_link (l->list); le != NULL; i++, le = g_list_next (le))
        {
            WLEntry *e = LENTRY (le->data);

            if (e->hotkey == key)
                return i;
        }
    }

    return (-1);
}

/* --------------------------------------------------------------------------------------------- */

/* Calculates the item displayed at screen row 'y' (y==0 being the widget's 1st row). */
static int
listbox_y_pos (WListbox * l, int y)
{
    return MIN (l->top + y, LISTBOX_LAST (l));
}

/* --------------------------------------------------------------------------------------------- */

static void
listbox_fwd (WListbox * l, gboolean wrap)
{
    if (!listbox_is_empty (l))
    {
        if ((guint) l->pos + 1 < g_queue_get_length (l->list))
            listbox_select_entry (l, l->pos + 1);
        else if (wrap)
            listbox_select_first (l);
    }
}

/* --------------------------------------------------------------------------------------------- */

static void
listbox_fwd_n (WListbox * l, int n)
{
    listbox_select_entry (l, MIN (l->pos + n, LISTBOX_LAST (l)));
}

/* --------------------------------------------------------------------------------------------- */

static void
listbox_back (WListbox * l, gboolean wrap)
{
    if (!listbox_is_empty (l))
    {
        if (l->pos > 0)
            listbox_select_entry (l, l->pos - 1);
        else if (wrap)
            listbox_select_last (l);
    }
}

/* --------------------------------------------------------------------------------------------- */

static void
listbox_back_n (WListbox * l, int n)
{
    listbox_select_entry (l, MAX (l->pos - n, 0));
}

/* --------------------------------------------------------------------------------------------- */

static cb_ret_t
listbox_execute_cmd (WListbox * l, long command)
{
    cb_ret_t ret = MSG_HANDLED;
    Widget *w = WIDGET (l);

    if (l->list == NULL || g_queue_is_empty (l->list))
        return MSG_NOT_HANDLED;

    switch (command)
    {
    case CK_Up:
        listbox_back (l, TRUE);
        break;
    case CK_Down:
        listbox_fwd (l, TRUE);
        break;
    case CK_Top:
        listbox_select_first (l);
        break;
    case CK_Bottom:
        listbox_select_last (l);
        break;
    case CK_PageUp:
        listbox_back_n (l, w->lines - 1);
        break;
    case CK_PageDown:
        listbox_fwd_n (l, w->lines - 1);
        break;
    case CK_Delete:
        if (l->deletable)
        {
            gboolean is_last, is_more;
            int length;

            length = g_queue_get_length (l->list);

            is_last = (l->pos + 1 >= length);
            is_more = (l->top + w->lines >= length);

            listbox_remove_current (l);
            if ((l->top > 0) && (is_last || is_more))
                l->top--;
        }
        break;
    case CK_Clear:
        if (l->deletable && mc_global.widget.confirm_history_cleanup
            /* TRANSLATORS: no need to translate 'DialogTitle', it's just a context prefix */
            && (query_dialog (Q_ ("DialogTitle|History cleanup"),
                              _("Do you want clean this history?"),
                              D_ERROR, 2, _("&Yes"), _("&No")) == 0))
            listbox_remove_list (l);
        break;
    case CK_View:
    case CK_Edit:
    case CK_Enter:
        ret = send_message (WIDGET (l)->owner, l, MSG_NOTIFY, command, NULL);
        break;
    default:
        ret = MSG_NOT_HANDLED;
    }

    return ret;
}

/* --------------------------------------------------------------------------------------------- */

/* Return MSG_HANDLED if we want a redraw */
static cb_ret_t
listbox_key (WListbox * l, int key)
{
    long command;

    if (l->list == NULL)
        return MSG_NOT_HANDLED;

    /* focus on listbox item N by '0'..'9' keys */
    if (key >= '0' && key <= '9')
    {
        listbox_select_entry (l, key - '0');
        return MSG_HANDLED;
    }

    command = widget_lookup_key (WIDGET (l), key);
    if (command == CK_IgnoreKey)
        return MSG_NOT_HANDLED;
    return listbox_execute_cmd (l, command);
}

/* --------------------------------------------------------------------------------------------- */

/* Listbox item adding function */
static inline void
listbox_append_item (WListbox * l, WLEntry * e, listbox_append_t pos)
{
    if (l->list == NULL)
    {
        l->list = g_queue_new ();
        pos = LISTBOX_APPEND_AT_END;
    }

    switch (pos)
    {
    case LISTBOX_APPEND_AT_END:
        g_queue_push_tail (l->list, e);
        break;

    case LISTBOX_APPEND_BEFORE:
        g_queue_insert_before (l->list, g_queue_peek_nth_link (l->list, (guint) l->pos), e);
        break;

    case LISTBOX_APPEND_AFTER:
        g_queue_insert_after (l->list, g_queue_peek_nth_link (l->list, (guint) l->pos), e);
        break;

    case LISTBOX_APPEND_SORTED:
        g_queue_insert_sorted (l->list, e, (GCompareDataFunc) listbox_entry_cmp, NULL);
        break;

    default:
        break;
    }
}

/* --------------------------------------------------------------------------------------------- */

/* Call this whenever the user changes the selected item. */
static void
listbox_on_change (WListbox * l)
{
    listbox_draw (l, TRUE);
    send_message (WIDGET (l)->owner, l, MSG_NOTIFY, 0, NULL);
}

/* --------------------------------------------------------------------------------------------- */

static void
listbox_do_action (WListbox * l)
{
    int action;

    if (listbox_is_empty (l))
        return;

    if (l->callback != NULL)
        action = l->callback (l);
    else
        action = LISTBOX_DONE;

    if (action == LISTBOX_DONE)
    {
        WDialog *h = DIALOG (WIDGET (l)->owner);

        h->ret_value = B_ENTER;
        dlg_stop (h);
    }
}

/* --------------------------------------------------------------------------------------------- */

static void
listbox_run_hotkey (WListbox * l, int pos)
{
    listbox_select_entry (l, pos);
    listbox_on_change (l);
    listbox_do_action (l);
}

/* --------------------------------------------------------------------------------------------- */

static inline void
listbox_destroy (WListbox * l)
{
    listbox_remove_list (l);
}

/* --------------------------------------------------------------------------------------------- */

static cb_ret_t
listbox_callback (Widget * w, Widget * sender, widget_msg_t msg, int parm, void *data)
{
    WListbox *l = LISTBOX (w);

    switch (msg)
    {
    case MSG_HOTKEY:
        {
            int pos;

            pos = listbox_check_hotkey (l, parm);
            if (pos < 0)
                return MSG_NOT_HANDLED;

            listbox_run_hotkey (l, pos);

            return MSG_HANDLED;
        }

    case MSG_KEY:
        {
            cb_ret_t ret_code;

            ret_code = listbox_key (l, parm);
            if (ret_code != MSG_NOT_HANDLED)
                listbox_on_change (l);
            return ret_code;
        }

    case MSG_ACTION:
        return listbox_execute_cmd (l, parm);

    case MSG_CURSOR:
        widget_gotoyx (l, l->cursor_y, 0);
        return MSG_HANDLED;

    case MSG_DRAW:
        listbox_draw (l, widget_get_state (w, WST_FOCUSED));
        return MSG_HANDLED;

    case MSG_DESTROY:
        listbox_destroy (l);
        return MSG_HANDLED;

    default:
        return widget_default_callback (w, sender, msg, parm, data);
    }
}

/* --------------------------------------------------------------------------------------------- */

static void
listbox_mouse_callback (Widget * w, mouse_msg_t msg, mouse_event_t * event)
{
    WListbox *l = LISTBOX (w);
    int old_pos;

    old_pos = l->pos;

    switch (msg)
    {
    case MSG_MOUSE_DOWN:
        widget_select (w);
        listbox_select_entry (l, listbox_y_pos (l, event->y));
        break;

    case MSG_MOUSE_SCROLL_UP:
        listbox_back (l, FALSE);
        break;

    case MSG_MOUSE_SCROLL_DOWN:
        listbox_fwd (l, FALSE);
        break;

    case MSG_MOUSE_DRAG:
        event->result.repeat = TRUE;    /* It'd be functional even without this. */
        listbox_select_entry (l, listbox_y_pos (l, event->y));
        break;

    case MSG_MOUSE_CLICK:
        /* We don't call listbox_select_entry() here: MSG_MOUSE_DOWN/DRAG did this already. */
        if (event->count == GPM_DOUBLE) /* Double click */
            listbox_do_action (l);
        break;

    default:
        break;
    }

    /* If the selection has changed, we redraw the widget and notify the dialog. */
    if (l->pos != old_pos)
        listbox_on_change (l);
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

WListbox *
listbox_new (int y, int x, int height, int width, gboolean deletable, lcback_fn callback)
{
    WListbox *l;
    Widget *w;

    if (height <= 0)
        height = 1;

    l = g_new (WListbox, 1);
    w = WIDGET (l);
    widget_init (w, y, x, height, width, listbox_callback, listbox_mouse_callback);
    w->options |= WOP_SELECTABLE | WOP_WANT_HOTKEY;
    w->keymap = listbox_map;

    l->list = NULL;
    l->top = l->pos = 0;
    l->deletable = deletable;
    l->callback = callback;
    l->allow_duplicates = TRUE;
    l->scrollbar = !mc_global.tty.slow_terminal;

    return l;
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Finds item by its label.
 */
int
listbox_search_text (WListbox * l, const char *text)
{
    if (!listbox_is_empty (l))
    {
        int i;
        GList *le;

        for (i = 0, le = g_queue_peek_head_link (l->list); le != NULL; i++, le = g_list_next (le))
        {
            WLEntry *e = LENTRY (le->data);

            if (strcmp (e->text, text) == 0)
                return i;
        }
    }

    return (-1);
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Finds item by its 'data' slot.
 */
int
listbox_search_data (WListbox * l, const void *data)
{
    if (!listbox_is_empty (l))
    {
        int i;
        GList *le;

        for (i = 0, le = g_queue_peek_head_link (l->list); le != NULL; i++, le = g_list_next (le))
        {
            WLEntry *e = LENTRY (le->data);

            if (e->data == data)
                return i;
        }
    }

    return (-1);
}

/* --------------------------------------------------------------------------------------------- */

/* Selects the first entry and scrolls the list to the top */
void
listbox_select_first (WListbox * l)
{
    l->pos = l->top = 0;
}

/* --------------------------------------------------------------------------------------------- */

/* Selects the last entry and scrolls the list to the bottom */
void
listbox_select_last (WListbox * l)
{
    int lines = WIDGET (l)->lines;
    int length;

    length = listbox_get_length (l);

    l->pos = DOZ (length, 1);
    l->top = DOZ (length, lines);
}

/* --------------------------------------------------------------------------------------------- */

void
listbox_select_entry (WListbox * l, int dest)
{
    GList *le;
    int pos;
    gboolean top_seen = FALSE;

    if (listbox_is_empty (l) || dest < 0)
        return;

    /* Special case */
    for (pos = 0, le = g_queue_peek_head_link (l->list); le != NULL; pos++, le = g_list_next (le))
    {
        if (pos == l->top)
            top_seen = TRUE;

        if (pos == dest)
        {
            l->pos = dest;
            if (!top_seen)
                l->top = l->pos;
            else
            {
                int lines = WIDGET (l)->lines;

                if (l->pos - l->top >= lines)
                    l->top = l->pos - lines + 1;
            }
            return;
        }
    }

    /* If we are unable to find it, set decent values */
    l->pos = l->top = 0;
}

/* --------------------------------------------------------------------------------------------- */

int
listbox_get_length (const WListbox * l)
{
    return listbox_is_empty (l) ? 0 : (int) g_queue_get_length (l->list);
}

/* --------------------------------------------------------------------------------------------- */

/* Returns the current string text as well as the associated extra data */
void
listbox_get_current (WListbox * l, char **string, void **extra)
{
    WLEntry *e = NULL;
    gboolean ok;

    if (l != NULL)
        e = listbox_get_nth_item (l, l->pos);

    ok = (e != NULL);

    if (string != NULL)
        *string = ok ? e->text : NULL;

    if (extra != NULL)
        *extra = ok ? e->data : NULL;
}

/* --------------------------------------------------------------------------------------------- */

WLEntry *
listbox_get_nth_item (const WListbox * l, int pos)
{
    if (!listbox_is_empty (l) && pos >= 0)
    {
        GList *item;

        item = g_queue_peek_nth_link (l->list, (guint) pos);
        if (item != NULL)
            return LENTRY (item->data);
    }

    return NULL;
}

/* --------------------------------------------------------------------------------------------- */

GList *
listbox_get_first_link (const WListbox * l)
{
    return (l == NULL || l->list == NULL) ? NULL : g_queue_peek_head_link (l->list);
}

/* --------------------------------------------------------------------------------------------- */

void
listbox_remove_current (WListbox * l)
{
    if (!listbox_is_empty (l))
    {
        GList *current;
        int length;

        current = g_queue_peek_nth_link (l->list, (guint) l->pos);
        listbox_entry_free (current->data);
        g_queue_delete_link (l->list, current);

        length = g_queue_get_length (l->list);

        if (length == 0)
            l->top = l->pos = 0;
        else if (l->pos >= length)
            l->pos = length - 1;
    }
}

/* --------------------------------------------------------------------------------------------- */

gboolean
listbox_is_empty (const WListbox * l)
{
    return (l == NULL || l->list == NULL || g_queue_is_empty (l->list));
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Set new listbox items list.
 *
 * @param l WListbox object
 * @param list list of WLEntry objects
 */
void
listbox_set_list (WListbox * l, GQueue * list)
{
    listbox_remove_list (l);

    if (l != NULL)
        l->list = list;
}

/* --------------------------------------------------------------------------------------------- */

void
listbox_remove_list (WListbox * l)
{
    if (l != NULL)
    {
        if (l->list != NULL)
        {
            g_queue_free_full (l->list, (GDestroyNotify) listbox_entry_free);
            l->list = NULL;
        }

        l->pos = l->top = 0;
    }
}

/* --------------------------------------------------------------------------------------------- */

char *
listbox_add_item (WListbox * l, listbox_append_t pos, int hotkey, const char *text, void *data,
                  gboolean free_data)
{
    WLEntry *entry;

    if (l == NULL)
        return NULL;

    if (!l->allow_duplicates && (listbox_search_text (l, text) >= 0))
        return NULL;

    entry = g_new (WLEntry, 1);
    entry->text = g_strdup (text);
    entry->data = data;
    entry->free_data = free_data;
    entry->hotkey = hotkey;

    listbox_append_item (l, entry, pos);

    return entry->text;
}

/* --------------------------------------------------------------------------------------------- */
