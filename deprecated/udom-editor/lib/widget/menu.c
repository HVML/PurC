/*
   Pulldown menu code

   Copyright (C) 1994-2021
   Free Software Foundation, Inc.

   Written by:
   Andrew Borodin <aborodin@vmail.ru>, 2012, 2013, 2016

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

/** \file menu.c
 *  \brief Source: pulldown menu code
 */

#include <config.h>

#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>

#include "lib/global.h"

#include "lib/tty/tty.h"
#include "lib/skin.h"
#include "lib/tty/key.h"        /* key macros */
#include "lib/strutil.h"
#include "lib/widget.h"
#include "lib/event.h"          /* mc_event_raise() */

/*** global variables ****************************************************************************/

const global_keymap_t *menu_map = NULL;

/*** file scope macro definitions ****************************************************************/

#define MENUENTRY(x) ((menu_entry_t *)(x))
#define MENU(x) ((menu_t *)(x))

/*** file scope type declarations ****************************************************************/

struct menu_entry_t
{
    unsigned char first_letter;
    hotkey_t text;
    long command;
    char *shortcut;
};

struct menu_t
{
    int start_x;                /* position relative to menubar start */
    hotkey_t text;
    GList *entries;
    size_t max_entry_len;       /* cached max length of entry texts (text + shortcut) */
    size_t max_hotkey_len;      /* cached max length of shortcuts */
    unsigned int selected;      /* pointer to current menu entry */
    char *help_node;
};

/*** file scope variables ************************************************************************/

/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

static void
menu_arrange (menu_t * menu, dlg_shortcut_str get_shortcut)
{
    if (menu != NULL)
    {
        GList *i;
        size_t max_shortcut_len = 0;

        menu->max_entry_len = 1;
        menu->max_hotkey_len = 1;

        for (i = menu->entries; i != NULL; i = g_list_next (i))
        {
            menu_entry_t *entry = MENUENTRY (i->data);

            if (entry != NULL)
            {
                size_t len;

                len = (size_t) hotkey_width (entry->text);
                menu->max_hotkey_len = MAX (menu->max_hotkey_len, len);

                if (get_shortcut != NULL)
                    entry->shortcut = get_shortcut (entry->command);

                if (entry->shortcut != NULL)
                {
                    len = (size_t) str_term_width1 (entry->shortcut);
                    max_shortcut_len = MAX (max_shortcut_len, len);
                }
            }
        }

        menu->max_entry_len = menu->max_hotkey_len + max_shortcut_len;
    }
}

/* --------------------------------------------------------------------------------------------- */

static void
menubar_paint_idx (const WMenuBar * menubar, unsigned int idx, int color)
{
    const Widget *w = CONST_WIDGET (menubar);
    const menu_t *menu = MENU (g_list_nth_data (menubar->menu, menubar->selected));
    const menu_entry_t *entry = MENUENTRY (g_list_nth_data (menu->entries, idx));
    const int y = 2 + idx;
    int x = menu->start_x;

    if (x + menu->max_entry_len + 4 > (gsize) w->cols)
        x = w->cols - menu->max_entry_len - 4;

    if (entry == NULL)
    {
        /* menu separator */
        tty_setcolor (MENU_ENTRY_COLOR);

        widget_gotoyx (w, y, x - 1);
        tty_print_alt_char (ACS_LTEE, FALSE);
        tty_draw_hline (w->y + y, w->x + x, ACS_HLINE, menu->max_entry_len + 3);
        widget_gotoyx (w, y, x + menu->max_entry_len + 3);
        tty_print_alt_char (ACS_RTEE, FALSE);
    }
    else
    {
        int yt, xt;

        /* menu text */
        tty_setcolor (color);
        widget_gotoyx (w, y, x);
        tty_print_char ((unsigned char) entry->first_letter);
        tty_getyx (&yt, &xt);
        tty_draw_hline (yt, xt, ' ', menu->max_entry_len + 2);  /* clear line */
        tty_print_string (entry->text.start);

        if (entry->text.hotkey != NULL)
        {
            tty_setcolor (color == MENU_SELECTED_COLOR ? MENU_HOTSEL_COLOR : MENU_HOT_COLOR);
            tty_print_string (entry->text.hotkey);
            tty_setcolor (color);
        }

        if (entry->text.end != NULL)
            tty_print_string (entry->text.end);

        if (entry->shortcut != NULL)
        {
            widget_gotoyx (w, y, x + menu->max_hotkey_len + 3);
            tty_print_string (entry->shortcut);
        }

        /* move cursor to the start of entry text */
        widget_gotoyx (w, y, x + 1);
    }
}

/* --------------------------------------------------------------------------------------------- */

static void
menubar_draw_drop (const WMenuBar * menubar)
{
    const Widget *w = CONST_WIDGET (menubar);
    const menu_t *menu = MENU (g_list_nth_data (menubar->menu, menubar->selected));
    const unsigned int count = g_list_length (menu->entries);
    int column = menu->start_x - 1;
    unsigned int i;

    if (column + menu->max_entry_len + 5 > (gsize) w->cols)
        column = w->cols - menu->max_entry_len - 5;

    if (mc_global.tty.shadows)
        tty_draw_box_shadow (w->y + 1, w->x + column, count + 2, menu->max_entry_len + 5,
                             SHADOW_COLOR);

    tty_setcolor (MENU_ENTRY_COLOR);
    tty_draw_box (w->y + 1, w->x + column, count + 2, menu->max_entry_len + 5, FALSE);

    for (i = 0; i < count; i++)
        menubar_paint_idx (menubar, i,
                           i == menu->selected ? MENU_SELECTED_COLOR : MENU_ENTRY_COLOR);
}

/* --------------------------------------------------------------------------------------------- */

static void
menubar_set_color (const WMenuBar * menubar, gboolean current, gboolean hotkey)
{
    if (!widget_get_state (CONST_WIDGET (menubar), WST_FOCUSED))
        tty_setcolor (MENU_INACTIVE_COLOR);
    else if (current)
        tty_setcolor (hotkey ? MENU_HOTSEL_COLOR : MENU_SELECTED_COLOR);
    else
        tty_setcolor (hotkey ? MENU_HOT_COLOR : MENU_ENTRY_COLOR);
}

/* --------------------------------------------------------------------------------------------- */

static void
menubar_draw (const WMenuBar * menubar)
{
    const Widget *w = CONST_WIDGET (menubar);
    GList *i;

    /* First draw the complete menubar */
    tty_setcolor (widget_get_state (w, WST_FOCUSED) ? MENU_ENTRY_COLOR : MENU_INACTIVE_COLOR);
    tty_draw_hline (w->y, w->x, ' ', w->cols);

    /* Now each one of the entries */
    for (i = menubar->menu; i != NULL; i = g_list_next (i))
    {
        menu_t *menu = MENU (i->data);
        gboolean is_selected = (menubar->selected == (gsize) g_list_position (menubar->menu, i));

        menubar_set_color (menubar, is_selected, FALSE);
        widget_gotoyx (w, 0, menu->start_x);

        tty_print_char (' ');
        tty_print_string (menu->text.start);

        if (menu->text.hotkey != NULL)
        {
            menubar_set_color (menubar, is_selected, TRUE);
            tty_print_string (menu->text.hotkey);
            menubar_set_color (menubar, is_selected, FALSE);
        }

        if (menu->text.end != NULL)
            tty_print_string (menu->text.end);

        tty_print_char (' ');
    }

    if (menubar->is_dropped)
        menubar_draw_drop (menubar);
    else
        widget_gotoyx (w, 0, MENU (g_list_nth_data (menubar->menu, menubar->selected))->start_x);
}

/* --------------------------------------------------------------------------------------------- */

static void
menubar_remove (WMenuBar * menubar)
{
    Widget *g;

    if (!menubar->is_dropped)
        return;

    /* HACK: before refresh the dialog, change the current widget to keep the order
       of overlapped widgets. This is useful in multi-window editor.
       In general, menubar should be a special object, not an ordinary widget
       in the current dialog. */
    g = WIDGET (WIDGET (menubar)->owner);
    GROUP (g)->current = widget_find (g, widget_find_by_id (g, menubar->previous_widget));

    menubar->is_dropped = FALSE;
    do_refresh ();
    menubar->is_dropped = TRUE;

    /* restore current widget */
    GROUP (g)->current = widget_find (g, WIDGET (menubar));
}

/* --------------------------------------------------------------------------------------------- */

static void
menubar_left (WMenuBar * menubar)
{
    menubar_remove (menubar);
    if (menubar->selected == 0)
        menubar->selected = g_list_length (menubar->menu) - 1;
    else
        menubar->selected--;
    menubar_draw (menubar);
}

/* --------------------------------------------------------------------------------------------- */

static void
menubar_right (WMenuBar * menubar)
{
    menubar_remove (menubar);
    menubar->selected = (menubar->selected + 1) % g_list_length (menubar->menu);
    menubar_draw (menubar);
}

/* --------------------------------------------------------------------------------------------- */

static void
menubar_finish (WMenuBar * menubar)
{
    Widget *w = WIDGET (menubar);

    menubar->is_dropped = FALSE;
    w->lines = 1;
    widget_want_hotkey (w, FALSE);
    widget_set_options (w, WOP_SELECTABLE, FALSE);

    if (!mc_global.keybar_visible)
        widget_hide (w);
    else
    {
        /* Move the menubar to the bottom so that widgets displayed on top of
         * an "invisible" menubar get the first chance to respond to mouse events. */
        widget_set_bottom (w);
    }

    /* background must be bottom */
    if (DIALOG (w->owner)->bg != NULL)
        widget_set_bottom (WIDGET (DIALOG (w->owner)->bg));

    group_select_widget_by_id (w->owner, menubar->previous_widget);
    do_refresh ();
}

/* --------------------------------------------------------------------------------------------- */

static void
menubar_drop (WMenuBar * menubar, unsigned int selected)
{
    menubar->is_dropped = TRUE;
    menubar->selected = selected;
    menubar_draw (menubar);
}

/* --------------------------------------------------------------------------------------------- */

static void
menubar_execute (WMenuBar * menubar)
{
    const menu_t *menu = MENU (g_list_nth_data (menubar->menu, menubar->selected));
    const menu_entry_t *entry = MENUENTRY (g_list_nth_data (menu->entries, menu->selected));

    if ((entry != NULL) && (entry->command != CK_IgnoreKey))
    {
        Widget *w = WIDGET (menubar);

        mc_global.widget.is_right = (menubar->selected != 0);
        menubar_finish (menubar);
        send_message (w->owner, w, MSG_ACTION, entry->command, NULL);
        do_refresh ();
    }
}

/* --------------------------------------------------------------------------------------------- */

static void
menubar_down (WMenuBar * menubar)
{
    menu_t *menu = MENU (g_list_nth_data (menubar->menu, menubar->selected));
    const unsigned int len = g_list_length (menu->entries);
    menu_entry_t *entry;

    menubar_paint_idx (menubar, menu->selected, MENU_ENTRY_COLOR);

    do
    {
        menu->selected = (menu->selected + 1) % len;
        entry = MENUENTRY (g_list_nth_data (menu->entries, menu->selected));
    }
    while ((entry == NULL) || (entry->command == CK_IgnoreKey));

    menubar_paint_idx (menubar, menu->selected, MENU_SELECTED_COLOR);
}

/* --------------------------------------------------------------------------------------------- */

static void
menubar_up (WMenuBar * menubar)
{
    menu_t *menu = MENU (g_list_nth_data (menubar->menu, menubar->selected));
    const unsigned int len = g_list_length (menu->entries);
    menu_entry_t *entry;

    menubar_paint_idx (menubar, menu->selected, MENU_ENTRY_COLOR);

    do
    {
        if (menu->selected == 0)
            menu->selected = len - 1;
        else
            menu->selected--;
        entry = MENUENTRY (g_list_nth_data (menu->entries, menu->selected));
    }
    while ((entry == NULL) || (entry->command == CK_IgnoreKey));

    menubar_paint_idx (menubar, menu->selected, MENU_SELECTED_COLOR);
}

/* --------------------------------------------------------------------------------------------- */

static void
menubar_first (WMenuBar * menubar)
{
    if (menubar->is_dropped)
    {
        menu_t *menu = MENU (g_list_nth_data (menubar->menu, menubar->selected));

        if (menu->selected == 0)
            return;

        menubar_paint_idx (menubar, menu->selected, MENU_ENTRY_COLOR);

        menu->selected = 0;

        while (TRUE)
        {
            menu_entry_t *entry;

            entry = MENUENTRY (g_list_nth_data (menu->entries, menu->selected));

            if ((entry == NULL) || (entry->command == CK_IgnoreKey))
                menu->selected++;
            else
                break;
        }

        menubar_paint_idx (menubar, menu->selected, MENU_SELECTED_COLOR);
    }
    else
    {
        menubar->selected = 0;
        menubar_draw (menubar);
    }
}

/* --------------------------------------------------------------------------------------------- */

static void
menubar_last (WMenuBar * menubar)
{
    if (menubar->is_dropped)
    {
        menu_t *menu = MENU (g_list_nth_data (menubar->menu, menubar->selected));
        const unsigned int len = g_list_length (menu->entries);
        menu_entry_t *entry;

        if (menu->selected == len - 1)
            return;

        menubar_paint_idx (menubar, menu->selected, MENU_ENTRY_COLOR);

        menu->selected = len;

        do
        {
            menu->selected--;
            entry = MENUENTRY (g_list_nth_data (menu->entries, menu->selected));
        }
        while ((entry == NULL) || (entry->command == CK_IgnoreKey));

        menubar_paint_idx (menubar, menu->selected, MENU_SELECTED_COLOR);
    }
    else
    {
        menubar->selected = g_list_length (menubar->menu) - 1;
        menubar_draw (menubar);
    }
}

/* --------------------------------------------------------------------------------------------- */

static cb_ret_t
menubar_try_drop_menu (WMenuBar * menubar, int hotkey)
{
    GList *i;

    for (i = menubar->menu; i != NULL; i = g_list_next (i))
    {
        menu_t *menu = MENU (i->data);

        if (menu->text.hotkey != NULL && hotkey == g_ascii_tolower (menu->text.hotkey[0]))
        {
            menubar_drop (menubar, g_list_position (menubar->menu, i));
            return MSG_HANDLED;
        }
    }

    return MSG_NOT_HANDLED;
}

/* --------------------------------------------------------------------------------------------- */

static cb_ret_t
menubar_try_exec_menu (WMenuBar * menubar, int hotkey)
{
    menu_t *menu;
    GList *i;

    menu = g_list_nth_data (menubar->menu, menubar->selected);

    for (i = menu->entries; i != NULL; i = g_list_next (i))
    {
        const menu_entry_t *entry = MENUENTRY (i->data);

        if (entry != NULL && entry->text.hotkey != NULL
            && hotkey == g_ascii_tolower (entry->text.hotkey[0]))
        {
            menu->selected = g_list_position (menu->entries, i);
            menubar_execute (menubar);
            return MSG_HANDLED;
        }
    }

    return MSG_NOT_HANDLED;
}

/* --------------------------------------------------------------------------------------------- */

static cb_ret_t
menubar_execute_cmd (WMenuBar * menubar, long command)
{
    cb_ret_t ret = MSG_HANDLED;

    switch (command)
    {
    case CK_Help:
        {
            ev_help_t event_data = { NULL, NULL };

            if (menubar->is_dropped)
                event_data.node =
                    MENU (g_list_nth_data (menubar->menu, menubar->selected))->help_node;
            else
                event_data.node = "[Menu Bar]";

            mc_event_raise (MCEVENT_GROUP_CORE, "help", &event_data);
            menubar_draw (menubar);
        }
        break;

    case CK_Left:
        menubar_left (menubar);
        break;
    case CK_Right:
        menubar_right (menubar);
        break;
    case CK_Up:
        if (menubar->is_dropped)
            menubar_up (menubar);
        break;
    case CK_Down:
        if (menubar->is_dropped)
            menubar_down (menubar);
        else
            menubar_drop (menubar, menubar->selected);
        break;
    case CK_Home:
        menubar_first (menubar);
        break;
    case CK_End:
        menubar_last (menubar);
        break;

    case CK_Enter:
        if (menubar->is_dropped)
            menubar_execute (menubar);
        else
            menubar_drop (menubar, menubar->selected);
        break;
    case CK_Quit:
        menubar_finish (menubar);
        break;

    default:
        ret = MSG_NOT_HANDLED;
        break;
    }

    return ret;
}

/* --------------------------------------------------------------------------------------------- */

static int
menubar_handle_key (WMenuBar * menubar, int key)
{
    long cmd;
    cb_ret_t ret = MSG_NOT_HANDLED;

    cmd = widget_lookup_key (WIDGET (menubar), key);

    if (cmd != CK_IgnoreKey)
        ret = menubar_execute_cmd (menubar, cmd);

    if (ret != MSG_HANDLED)
    {
        if (menubar->is_dropped)
            ret = menubar_try_exec_menu (menubar, key);
        else
            ret = menubar_try_drop_menu (menubar, key);
    }

    return ret;
}

/* --------------------------------------------------------------------------------------------- */

static gboolean
menubar_refresh (WMenuBar * menubar)
{
    Widget *w = WIDGET (menubar);

    if (!widget_get_state (w, WST_FOCUSED))
        return FALSE;

    /* Trick to get all the mouse events */
    w->lines = LINES;

    /* Trick to get all of the hotkeys */
    widget_want_hotkey (w, TRUE);
    return TRUE;
}

/* --------------------------------------------------------------------------------------------- */

static inline void
menubar_free_menu (WMenuBar * menubar)
{
    g_clear_list (&menubar->menu, (GDestroyNotify) destroy_menu);
}

/* --------------------------------------------------------------------------------------------- */

static cb_ret_t
menubar_callback (Widget * w, Widget * sender, widget_msg_t msg, int parm, void *data)
{
    WMenuBar *menubar = MENUBAR (w);

    switch (msg)
    {
        /* We do not want the focus unless we have been activated */
    case MSG_FOCUS:
        if (menubar_refresh (menubar))
        {
            menubar_draw (menubar);
            return MSG_HANDLED;
        }
        return MSG_NOT_HANDLED;

    case MSG_UNFOCUS:
        return widget_get_state (w, WST_FOCUSED) ? MSG_NOT_HANDLED : MSG_HANDLED;

        /* We don't want the buttonbar to activate while using the menubar */
    case MSG_HOTKEY:
    case MSG_KEY:
        if (widget_get_state (w, WST_FOCUSED))
        {
            menubar_handle_key (menubar, parm);
            return MSG_HANDLED;
        }
        return MSG_NOT_HANDLED;

    case MSG_CURSOR:
        /* Put the cursor in a suitable place */
        return MSG_NOT_HANDLED;

    case MSG_DRAW:
        if (widget_get_state (w, WST_VISIBLE) || menubar_refresh (menubar))
            menubar_draw (menubar);
        return MSG_HANDLED;

    case MSG_RESIZE:
        /* try show menu after screen resize */
        widget_default_callback (w, NULL, MSG_RESIZE, 0, data);
        menubar_refresh (menubar);
        return MSG_HANDLED;

    case MSG_DESTROY:
        menubar_free_menu (menubar);
        return MSG_HANDLED;

    default:
        return widget_default_callback (w, sender, msg, parm, data);
    }
}

/* --------------------------------------------------------------------------------------------- */

static unsigned int
menubar_get_menu_by_x_coord (const WMenuBar * menubar, int x)
{
    unsigned int i;
    GList *menu;

    for (i = 0, menu = menubar->menu;
         menu != NULL && x >= MENU (menu->data)->start_x; i++, menu = g_list_next (menu))
        ;

    /* Don't set the invalid value -1 */
    if (i != 0)
        i--;

    return i;
}

/* --------------------------------------------------------------------------------------------- */

static gboolean
menubar_mouse_on_menu (const WMenuBar * menubar, int y, int x)
{
    const Widget *w = CONST_WIDGET (menubar);
    menu_t *menu;
    int left_x, right_x, bottom_y;

    if (!menubar->is_dropped)
        return FALSE;

    menu = MENU (g_list_nth_data (menubar->menu, menubar->selected));
    left_x = menu->start_x;
    right_x = left_x + menu->max_entry_len + 3;
    if (right_x > w->cols)
    {
        left_x = w->cols - (menu->max_entry_len + 3);
        right_x = w->cols;
    }

    bottom_y = g_list_length (menu->entries) + 2;       /* skip bar and top frame */

    return (x >= left_x && x < right_x && y > 1 && y < bottom_y);
}

/* --------------------------------------------------------------------------------------------- */

static void
menubar_change_selected_item (WMenuBar * menubar, int y)
{
    menu_t *menu;
    menu_entry_t *entry;

    y -= 2;                     /* skip bar and top frame */
    menu = MENU (g_list_nth_data (menubar->menu, menubar->selected));
    entry = MENUENTRY (g_list_nth_data (menu->entries, y));

    if (entry != NULL && entry->command != CK_IgnoreKey)
    {
        menubar_paint_idx (menubar, menu->selected, MENU_ENTRY_COLOR);
        menu->selected = y;
        menubar_paint_idx (menubar, menu->selected, MENU_SELECTED_COLOR);
    }
}

/* --------------------------------------------------------------------------------------------- */

static void
menubar_mouse_callback (Widget * w, mouse_msg_t msg, mouse_event_t * event)
{
    static gboolean was_drag = FALSE;

    WMenuBar *menubar = MENUBAR (w);
    gboolean mouse_on_drop;

    mouse_on_drop = menubar_mouse_on_menu (menubar, event->y, event->x);

    switch (msg)
    {
    case MSG_MOUSE_DOWN:
        was_drag = FALSE;

        if (event->y == 0)
        {
            /* events on menubar */
            unsigned int selected;

            selected = menubar_get_menu_by_x_coord (menubar, event->x);
            menubar_activate (menubar, TRUE, selected);
            menubar_remove (menubar);   /* if already shown */
            menubar_drop (menubar, selected);
        }
        else if (mouse_on_drop)
            menubar_change_selected_item (menubar, event->y);
        else
        {
            /* mouse click outside menubar or dropdown -- close menu */
            menubar_finish (menubar);

            /*
             * @FIXME.
             *
             * Unless we clear the 'capture' flag, we'll receive MSG_MOUSE_DRAG
             * events belonging to this click (in case the user drags the mouse,
             * of course).
             *
             * For the time being, we mark this with FIXME as this flag should
             * preferably be regarded as "implementation detail" and not be
             * touched by us. We should think of some other way of communicating
             * this to the system.
             */
            w->mouse.capture = FALSE;
        }
        break;

    case MSG_MOUSE_UP:
        if (was_drag && mouse_on_drop)
            menubar_execute (menubar);
        was_drag = FALSE;
        break;

    case MSG_MOUSE_CLICK:
        was_drag = FALSE;

        if ((event->buttons & GPM_B_MIDDLE) != 0 && event->y > 0 && menubar->is_dropped)
        {
            /* middle click -- everywhere */
            menubar_execute (menubar);
        }
        else if (mouse_on_drop)
            menubar_execute (menubar);
        else if (event->y > 0)
            /* releasing the mouse button outside the menu -- close menu */
            menubar_finish (menubar);
        break;

    case MSG_MOUSE_DRAG:
        if (event->y == 0)
        {
            menubar_remove (menubar);
            menubar_drop (menubar, menubar_get_menu_by_x_coord (menubar, event->x));
        }
        else if (mouse_on_drop)
            menubar_change_selected_item (menubar, event->y);

        was_drag = TRUE;
        break;

    case MSG_MOUSE_SCROLL_UP:
    case MSG_MOUSE_SCROLL_DOWN:
        was_drag = FALSE;

        if (widget_get_state (w, WST_FOCUSED))
        {
            if (event->y == 0)
            {
                /* menubar: left/right */
                if (msg == MSG_MOUSE_SCROLL_UP)
                    menubar_left (menubar);
                else
                    menubar_right (menubar);
            }
            else if (mouse_on_drop)
            {
                /* drop-down menu: up/down */
                if (msg == MSG_MOUSE_SCROLL_UP)
                    menubar_up (menubar);
                else
                    menubar_down (menubar);
            }
        }
        break;

    default:
        was_drag = FALSE;
        break;
    }
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

menu_entry_t *
menu_entry_create (const char *name, long command)
{
    menu_entry_t *entry;

    entry = g_new (menu_entry_t, 1);
    entry->first_letter = ' ';
    entry->text = hotkey_new (name);
    entry->command = command;
    entry->shortcut = NULL;

    return entry;
}

/* --------------------------------------------------------------------------------------------- */

void
menu_entry_free (menu_entry_t * entry)
{
    if (entry != NULL)
    {
        hotkey_free (entry->text);
        g_free (entry->shortcut);
        g_free (entry);
    }
}

/* --------------------------------------------------------------------------------------------- */

menu_t *
create_menu (const char *name, GList * entries, const char *help_node)
{
    menu_t *menu;

    menu = g_new (menu_t, 1);
    menu->start_x = 0;
    menu->text = hotkey_new (name);
    menu->entries = entries;
    menu->max_entry_len = 1;
    menu->max_hotkey_len = 0;
    menu->selected = 0;
    menu->help_node = g_strdup (help_node);

    return menu;
}

/* --------------------------------------------------------------------------------------------- */

void
menu_set_name (menu_t * menu, const char *name)
{
    hotkey_free (menu->text);
    menu->text = hotkey_new (name);
}

/* --------------------------------------------------------------------------------------------- */

void
destroy_menu (menu_t * menu)
{
    hotkey_free (menu->text);
    g_list_free_full (menu->entries, (GDestroyNotify) menu_entry_free);
    g_free (menu->help_node);
    g_free (menu);
}

/* --------------------------------------------------------------------------------------------- */

WMenuBar *
menubar_new (GList * menu)
{
    WMenuBar *menubar;
    Widget *w;

    menubar = g_new0 (WMenuBar, 1);
    w = WIDGET (menubar);
    widget_init (w, 0, 0, 1, COLS, menubar_callback, menubar_mouse_callback);
    w->pos_flags = WPOS_KEEP_HORZ | WPOS_KEEP_TOP;
    /* initially, menubar is not selectable */
    widget_set_options (w, WOP_SELECTABLE, FALSE);
    w->options |= WOP_TOP_SELECT;
    w->keymap = menu_map;
    menubar_set_menu (menubar, menu);

    return menubar;
}

/* --------------------------------------------------------------------------------------------- */

void
menubar_set_menu (WMenuBar * menubar, GList * menu)
{
    /* delete previous menu */
    menubar_free_menu (menubar);
    /* add new menu */
    menubar->is_dropped = FALSE;
    menubar->menu = menu;
    menubar->selected = 0;
    menubar_arrange (menubar);
    widget_set_state (WIDGET (menubar), WST_FOCUSED, FALSE);
}

/* --------------------------------------------------------------------------------------------- */

void
menubar_add_menu (WMenuBar * menubar, menu_t * menu)
{
    if (menu != NULL)
    {
        menu_arrange (menu, DIALOG (WIDGET (menubar)->owner)->get_shortcut);
        menubar->menu = g_list_append (menubar->menu, menu);
    }

    menubar_arrange (menubar);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Properly space menubar items. Should be called when menubar is created
 * and also when widget width is changed (i.e. upon xterm resize).
 */

void
menubar_arrange (WMenuBar * menubar)
{
    int start_x = 1;
    GList *i;
    int gap;

    if (menubar->menu == NULL)
        return;

    gap = WIDGET (menubar)->cols - 2;

    /* First, calculate gap between items... */
    for (i = menubar->menu; i != NULL; i = g_list_next (i))
    {
        menu_t *menu = MENU (i->data);

        /* preserve length here, to be used below */
        menu->start_x = hotkey_width (menu->text) + 2;
        gap -= menu->start_x;
    }

    if (g_list_next (menubar->menu) == NULL)
        gap = 1;
    else
        gap /= (g_list_length (menubar->menu) - 1);

    if (gap <= 0)
    {
        /* We are out of luck - window is too narrow... */
        gap = 1;
    }
    else if (gap >= 3)
        gap = 3;

    /* ...and now fix start positions of menubar items */
    for (i = menubar->menu; i != NULL; i = g_list_next (i))
    {
        menu_t *menu = MENU (i->data);
        int len = menu->start_x;

        menu->start_x = start_x;
        start_x += len + gap;
    }
}

/* --------------------------------------------------------------------------------------------- */
/** Find MenuBar widget in the dialog */

WMenuBar *
find_menubar (const WDialog * h)
{
    return MENUBAR (widget_find_by_type (CONST_WIDGET (h), menubar_callback));
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Activate menu bar.
 *
 * @param menubar menu bar object
 * @param dropped whether dropdown menus should be drooped or not
 * @which number of active dropdown menu
 */
void
menubar_activate (WMenuBar * menubar, gboolean dropped, int which)
{
    Widget *w = WIDGET (menubar);

    widget_show (w);

    if (!widget_get_state (w, WST_FOCUSED))
    {
        widget_set_options (w, WOP_SELECTABLE, TRUE);

        menubar->is_dropped = dropped;
        if (which >= 0)
            menubar->selected = (guint) which;

        menubar->previous_widget = group_get_current_widget_id (w->owner);

        /* Bring it to the top so it receives all mouse events before any other widget.
         * See also comment in menubar_finish(). */
        widget_select (w);
    }
}

/* --------------------------------------------------------------------------------------------- */
