/*
   Editor initialisation and callback handler.

   Copyright (C) 1996-2021
   Free Software Foundation, Inc.

   Written by:
   Paul Sheer, 1996, 1997
   Andrew Borodin <aborodin@vmail.ru> 2012, 2013

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

/** \file
 *  \brief Source: editor initialisation and callback handler
 *  \author Paul Sheer
 *  \date 1996, 1997
 */

#include <config.h>

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "lib/global.h"

#include "lib/tty/tty.h"        /* LINES, COLS */
#include "lib/tty/key.h"        /* is_idle() */
#include "lib/tty/color.h"      /* tty_setcolor() */
#include "lib/skin.h"
#include "lib/fileloc.h"        /* EDIT_HOME_DIR */
#include "lib/strutil.h"        /* str_term_trim() */
#include "lib/util.h"           /* mc_build_filename() */
#include "lib/widget.h"
#include "lib/mcconfig.h"
#include "lib/event.h"          /* mc_event_raise() */
#ifdef HAVE_CHARSET
#include "lib/charsets.h"
#endif

#include "src/keymap.h"         /* keybind_lookup_keymap_command() */
#include "src/setup.h"          /* home_dir */
#include "src/execute.h"        /* toggle_subshell()  */
#include "src/learn.h"          /* learn_keys() */
#include "src/args.h"           /* mcedit_arg_t */

#include "edit-impl.h"
#include "editwidget.h"
#include "editmacros.h"         /* edit_execute_macro() */
#ifdef HAVE_ASPELL
#include "spell.h"
#endif

/*** global variables ****************************************************************************/

char *edit_window_state_char = NULL;
char *edit_window_close_char = NULL;

/*** file scope macro definitions ****************************************************************/

#define WINDOW_MIN_LINES (2 + 2)
#define WINDOW_MIN_COLS (2 + LINE_STATE_WIDTH + 2)

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/
static unsigned int edit_dlg_init_refcounter = 0;

/*** file scope functions ************************************************************************/

static cb_ret_t edit_dialog_callback (Widget * w, Widget * sender, widget_msg_t msg, int parm,
                                      void *data);

/* --------------------------------------------------------------------------------------------- */
/**
 * Init the 'edit' subsystem
 */

static void
edit_dlg_init (void)
{
    edit_dlg_init_refcounter++;

    if (edit_dlg_init_refcounter == 1)
    {
        edit_window_state_char = mc_skin_get ("widget-editor", "window-state-char", "*");
        edit_window_close_char = mc_skin_get ("widget-editor", "window-close-char", "X");

#ifdef HAVE_ASPELL
        aspell_init ();
#endif
    }
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Deinit the 'edit' subsystem
 */

static void
edit_dlg_deinit (void)
{
    if (edit_dlg_init_refcounter == 1)
    {
        g_free (edit_window_state_char);
        g_free (edit_window_close_char);

#ifdef HAVE_ASPELL
        aspell_clean ();
#endif
    }

    if (edit_dlg_init_refcounter != 0)
        edit_dlg_init_refcounter--;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Show info about editor
 */

static void
edit_about (void)
{
    char *ver;

    ver = g_strdup_printf ("MCEdit %s", mc_global.mc_version);

    {
        quick_widget_t quick_widgets[] = {
            /* *INDENT-OFF* */
            QUICK_LABEL (ver, NULL),
            QUICK_SEPARATOR (TRUE),
            QUICK_LABEL (N_("A user friendly text editor\n"
                            "written for the Midnight Commander."), NULL),
            QUICK_SEPARATOR (FALSE),
            QUICK_LABEL (N_("Copyright (C) 1996-2021 the Free Software Foundation"), NULL),
            QUICK_START_BUTTONS (TRUE, TRUE),
            QUICK_BUTTON (N_("&OK"), B_ENTER, NULL, NULL),
            QUICK_END
            /* *INDENT-ON* */
        };

        quick_dialog_t qdlg = {
            -1, -1, 40,
            N_("About"), "[Internal File Editor]",
            quick_widgets, NULL, NULL
        };

        quick_widgets[0].pos_flags = WPOS_KEEP_TOP | WPOS_CENTER_HORZ;
        quick_widgets[2].pos_flags = WPOS_KEEP_TOP | WPOS_CENTER_HORZ;
        quick_widgets[4].pos_flags = WPOS_KEEP_TOP | WPOS_CENTER_HORZ;

        (void) quick_dialog (&qdlg);
    }

    g_free (ver);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Show a help window
 */

static void
edit_help (void)
{
    ev_help_t event_data = { NULL, "[Internal File Editor]" };
    mc_event_raise (MCEVENT_GROUP_CORE, "help", &event_data);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Restore saved window size.
 *
 * @param edit editor object
 */

static void
edit_restore_size (WEdit * edit)
{
    Widget *w = WIDGET (edit);

    edit->drag_state = MCEDIT_DRAG_NONE;
    w->mouse.forced_capture = FALSE;
    widget_set_size_rect (w, &edit->loc_prev);
    widget_draw (WIDGET (w->owner));
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Move window by one row or column in any direction.
 *
 * @param edit    editor object
 * @param command direction (CK_Up, CK_Down, CK_Left, CK_Right)
 */

static void
edit_window_move (WEdit * edit, long command)
{
    Widget *w = WIDGET (edit);
    Widget *wh = WIDGET (w->owner);

    switch (command)
    {
    case CK_Up:
        if (w->y > wh->y + 1)   /* menubar */
            w->y--;
        break;
    case CK_Down:
        if (w->y < wh->y + wh->lines - 2)       /* buttonbar */
            w->y++;
        break;
    case CK_Left:
        if (w->x + w->cols > wh->x)
            w->x--;
        break;
    case CK_Right:
        if (w->x < wh->x + wh->cols)
            w->x++;
        break;
    default:
        return;
    }

    edit->force |= REDRAW_PAGE;
    widget_draw (WIDGET (w->owner));
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Resize window by one row or column in any direction.
 *
 * @param edit    editor object
 * @param command direction (CK_Up, CK_Down, CK_Left, CK_Right)
 */

static void
edit_window_resize (WEdit * edit, long command)
{
    Widget *w = WIDGET (edit);
    Widget *wh = WIDGET (w->owner);

    switch (command)
    {
    case CK_Up:
        if (w->lines > WINDOW_MIN_LINES)
            w->lines--;
        break;
    case CK_Down:
        if (w->y + w->lines < wh->y + wh->lines - 1)    /* buttonbar */
            w->lines++;
        break;
    case CK_Left:
        if (w->cols > WINDOW_MIN_COLS)
            w->cols--;
        break;
    case CK_Right:
        if (w->x + w->cols < wh->x + wh->cols)
            w->cols++;
        break;
    default:
        return;
    }

    edit->force |= REDRAW_COMPLETELY;
    widget_draw (WIDGET (w->owner));
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Get hotkey by number.
 *
 * @param n number
 * @return hotkey
 */

static unsigned char
get_hotkey (int n)
{
    return (n <= 9) ? '0' + n : 'a' + n - 10;
}

/* --------------------------------------------------------------------------------------------- */

static void
edit_window_list (const WDialog * h)
{
    const WGroup *g = CONST_GROUP (h);
    const size_t offset = 2;    /* skip menu and buttonbar */
    const size_t dlg_num = g_list_length (g->widgets) - offset;
    int lines, cols;
    Listbox *listbox;
    GList *w;
    WEdit *selected;
    int i = 0;

    lines = MIN ((size_t) (LINES * 2 / 3), dlg_num);
    cols = COLS * 2 / 3;

    listbox = create_listbox_window (lines, cols, _("Open files"), "[Open files]");

    for (w = g->widgets; w != NULL; w = g_list_next (w))
        if (edit_widget_is_editor (CONST_WIDGET (w->data)))
        {
            WEdit *e = (WEdit *) w->data;
            char *fname;

            if (e->filename_vpath == NULL)
                fname = g_strdup_printf ("%c [%s]", e->modified ? '*' : ' ', _("NoName"));
            else
                fname =
                    g_strdup_printf ("%c%s", e->modified ? '*' : ' ',
                                     vfs_path_as_str (e->filename_vpath));

            listbox_add_item (listbox->list, LISTBOX_APPEND_AT_END, get_hotkey (i++),
                              str_term_trim (fname, WIDGET (listbox->list)->cols - 2), e, FALSE);
            g_free (fname);
        }

    selected = run_listbox_with_data (listbox, g->current->data);
    if (selected != NULL)
        widget_select (WIDGET (selected));
}

/* --------------------------------------------------------------------------------------------- */

static char *
edit_get_shortcut (long command)
{
    const char *ext_map;
    const char *shortcut = NULL;

    shortcut = keybind_lookup_keymap_shortcut (editor_map, command);
    if (shortcut != NULL)
        return g_strdup (shortcut);

    ext_map = keybind_lookup_keymap_shortcut (editor_map, CK_ExtendedKeyMap);
    if (ext_map != NULL)
        shortcut = keybind_lookup_keymap_shortcut (editor_x_map, command);
    if (shortcut != NULL)
        return g_strdup_printf ("%s %s", ext_map, shortcut);

    return NULL;
}

/* --------------------------------------------------------------------------------------------- */

static char *
edit_get_title (const WDialog * h, size_t len)
{
    const WEdit *edit = find_editor (h);
    const char *modified = edit->modified ? "(*) " : "    ";
    const char *file_label;
    char *filename;

    len -= 4;

    if (edit->filename_vpath == NULL)
        filename = g_strdup (_("[NoName]"));
    else
        filename = g_strdup (vfs_path_as_str (edit->filename_vpath));

    file_label = str_term_trim (filename, len - str_term_width1 (_("Edit: ")));
    g_free (filename);

    return g_strconcat (_("Edit: "), modified, file_label, (char *) NULL);
}

/* --------------------------------------------------------------------------------------------- */

static cb_ret_t
edit_dialog_command_execute (WDialog * h, long command)
{
    WGroup *g = GROUP (h);
    Widget *wh = WIDGET (h);
    cb_ret_t ret = MSG_HANDLED;

    switch (command)
    {
    case CK_EditNew:
        edit_add_window (h, wh->y + 1, wh->x, wh->lines - 2, wh->cols, NULL, 0);
        break;
    case CK_EditFile:
        edit_load_cmd (h);
        break;
    case CK_History:
        edit_load_file_from_history (h);
        break;
    case CK_EditSyntaxFile:
        edit_load_syntax_file (h);
        break;
    case CK_EditUserMenu:
        edit_load_menu_file (h);
        break;
    case CK_Close:
        /* if there are no opened files anymore, close MC editor */
        if (edit_widget_is_editor (CONST_WIDGET (g->current->data)) &&
            edit_close_cmd ((WEdit *) g->current->data) && find_editor (h) == NULL)
            dlg_stop (h);
        break;
    case CK_Help:
        edit_help ();
        /* edit->force |= REDRAW_COMPLETELY; */
        break;
    case CK_Menu:
        edit_menu_cmd (h);
        break;
    case CK_Quit:
    case CK_Cancel:
        /* don't close editor due to SIGINT, but stop move/resize window */
        {
            Widget *w = WIDGET (g->current->data);

            if (edit_widget_is_editor (w) && ((WEdit *) w)->drag_state != MCEDIT_DRAG_NONE)
                edit_restore_size ((WEdit *) w);
            else if (command == CK_Quit)
                dlg_stop (h);
        }
        break;
    case CK_About:
        edit_about ();
        break;
    case CK_SyntaxOnOff:
        edit_syntax_onoff_cmd (h);
        break;
    case CK_ShowTabTws:
        edit_show_tabs_tws_cmd (h);
        break;
    case CK_ShowMargin:
        edit_show_margin_cmd (h);
        break;
    case CK_ShowNumbers:
        edit_show_numbers_cmd (h);
        break;
    case CK_Refresh:
        edit_refresh_cmd ();
        break;
    case CK_Shell:
        toggle_subshell ();
        break;
    case CK_LearnKeys:
        learn_keys ();
        break;
    case CK_WindowMove:
    case CK_WindowResize:
        if (edit_widget_is_editor (CONST_WIDGET (g->current->data)))
            edit_handle_move_resize ((WEdit *) g->current->data, command);
        break;
    case CK_WindowList:
        edit_window_list (h);
        break;
    case CK_WindowNext:
        group_select_next_widget (g);
        break;
    case CK_WindowPrev:
        group_select_prev_widget (g);
        break;
    case CK_Options:
        edit_options_dialog (h);
        break;
    case CK_OptionsSaveMode:
        edit_save_mode_cmd ();
        break;
    case CK_SaveSetup:
        // VW: save_setup_cmd ();
        break;
    default:
        ret = MSG_NOT_HANDLED;
        break;
    }

    return ret;
}

/* --------------------------------------------------------------------------------------------- */
/*
 * Translate the keycode into either 'command' or 'char_for_insertion'.
 * 'command' is one of the editor commands from cmddef.h.
 */

static gboolean
edit_translate_key (WEdit * edit, long x_key, int *cmd, int *ch)
{
    Widget *w = WIDGET (edit);
    long command = CK_InsertChar;
    int char_for_insertion = -1;

    /* an ordinary insertable character */
    if (!w->ext_mode && x_key < 256)
    {
#ifndef HAVE_CHARSET
        if (is_printable (x_key))
        {
            char_for_insertion = x_key;
            goto fin;
        }
#else
        int c;

        if (edit->charpoint >= 4)
        {
            edit->charpoint = 0;
            edit->charbuf[edit->charpoint] = '\0';
        }
        if (edit->charpoint < 4)
        {
            edit->charbuf[edit->charpoint++] = x_key;
            edit->charbuf[edit->charpoint] = '\0';
        }

        /* input from 8-bit locale */
        if (!mc_global.utf8_display)
        {
            /* source in 8-bit codeset */
            c = convert_from_input_c (x_key);

            if (is_printable (c))
            {
                if (!edit->utf8)
                    char_for_insertion = c;
                else
                    char_for_insertion = convert_from_8bit_to_utf_c2 ((char) x_key);
                goto fin;
            }
        }
        else
        {
            /* UTF-8 locale */
            int res;

            res = str_is_valid_char (edit->charbuf, edit->charpoint);
            if (res < 0 && res != -2)
            {
                edit->charpoint = 0;    /* broken multibyte char, skip */
                goto fin;
            }

            if (edit->utf8)
            {
                /* source in UTF-8 codeset */
                if (res < 0)
                {
                    char_for_insertion = x_key;
                    goto fin;
                }

                edit->charbuf[edit->charpoint] = '\0';
                edit->charpoint = 0;
                if (g_unichar_isprint (g_utf8_get_char (edit->charbuf)))
                {
                    char_for_insertion = x_key;
                    goto fin;
                }
            }
            else
            {
                /* 8-bit source */
                if (res < 0)
                {
                    /* not finised multibyte input (in meddle multibyte utf-8 char) */
                    goto fin;
                }

                if (g_unichar_isprint (g_utf8_get_char (edit->charbuf)))
                {
                    c = convert_from_utf_to_current (edit->charbuf);
                    edit->charbuf[0] = '\0';
                    edit->charpoint = 0;
                    char_for_insertion = c;
                    goto fin;
                }

                /* unprinteble utf input, skip it */
                edit->charbuf[0] = '\0';
                edit->charpoint = 0;
            }
        }
#endif /* HAVE_CHARSET */
    }

    /* Commands specific to the key emulation */
    command = widget_lookup_key (w, x_key);
    if (command == CK_IgnoreKey)
        command = CK_InsertChar;

  fin:
    *cmd = (int) command;       /* FIXME */
    *ch = char_for_insertion;

    return !(command == CK_InsertChar && char_for_insertion == -1);
}


/* --------------------------------------------------------------------------------------------- */

static inline void
edit_quit (WDialog * h)
{
    GList *l;
    WEdit *e = NULL;
    GSList *m = NULL;
    GSList *me;

    /* don't stop the dialog before final decision */
    widget_set_state (WIDGET (h), WST_ACTIVE, TRUE);

    /* check window state and get modified files */
    for (l = GROUP (h)->widgets; l != NULL; l = g_list_next (l))
        if (edit_widget_is_editor (CONST_WIDGET (l->data)))
        {
            e = (WEdit *) l->data;

            if (e->drag_state != MCEDIT_DRAG_NONE)
            {
                edit_restore_size (e);
                g_slist_free (m);
                return;
            }

            /* create separate list because widget_select()
               changes the window position in Z order */
            if (e->modified)
                m = g_slist_prepend (m, l->data);
        }

    for (me = m; me != NULL; me = g_slist_next (me))
    {
        e = (WEdit *) me->data;

        widget_select (WIDGET (e));

        if (!edit_ok_to_exit (e))
            break;
    }

    /* if all files were checked, quit editor */
    if (me == NULL)
        dlg_stop (h);

    g_slist_free (m);
}

/* --------------------------------------------------------------------------------------------- */

static inline void
edit_set_buttonbar (WEdit * edit, WButtonBar * bb)
{
    Widget *w = WIDGET (edit);

    buttonbar_set_label (bb, 1, Q_ ("ButtonBar|Help"), w->keymap, NULL);
    buttonbar_set_label (bb, 2, Q_ ("ButtonBar|Save"), w->keymap, w);
    buttonbar_set_label (bb, 3, Q_ ("ButtonBar|Mark"), w->keymap, w);
    buttonbar_set_label (bb, 4, Q_ ("ButtonBar|Replac"), w->keymap, w);
    buttonbar_set_label (bb, 5, Q_ ("ButtonBar|Copy"), w->keymap, w);
    buttonbar_set_label (bb, 6, Q_ ("ButtonBar|Move"), w->keymap, w);
    buttonbar_set_label (bb, 7, Q_ ("ButtonBar|Search"), w->keymap, w);
    buttonbar_set_label (bb, 8, Q_ ("ButtonBar|Delete"), w->keymap, w);
    buttonbar_set_label (bb, 9, Q_ ("ButtonBar|PullDn"), w->keymap, NULL);
    buttonbar_set_label (bb, 10, Q_ ("ButtonBar|Quit"), w->keymap, NULL);
}

/* --------------------------------------------------------------------------------------------- */

static void
edit_total_update (WEdit * edit)
{
    edit_find_bracket (edit);
    edit->force |= REDRAW_COMPLETELY;
    edit_update_curs_row (edit);
    edit_update_screen (edit);
}

/* --------------------------------------------------------------------------------------------- */

static gboolean
edit_update_cursor (WEdit * edit, const mouse_event_t * event)
{
    int x, y;
    gboolean done;

    x = event->x - (edit->fullscreen ? 0 : 1);
    y = event->y - (edit->fullscreen ? 0 : 1);

    if (edit->mark2 != -1 && event->msg == MSG_MOUSE_UP)
        return TRUE;            /* don't do anything */

    if (event->msg == MSG_MOUSE_DOWN || event->msg == MSG_MOUSE_UP)
        edit_push_key_press (edit);

    if (!option_cursor_beyond_eol)
        edit->prev_col = x - edit->start_col - option_line_state_width;
    else
    {
        long line_len;

        line_len =
            edit_move_forward3 (edit, edit_buffer_get_current_bol (&edit->buffer), 0,
                                edit_buffer_get_current_eol (&edit->buffer));

        if (x > line_len - 1)
        {
            edit->over_col = x - line_len - edit->start_col - option_line_state_width;
            edit->prev_col = line_len;
        }
        else
        {
            edit->over_col = 0;
            edit->prev_col = x - option_line_state_width - edit->start_col;
        }
    }

    if (y > edit->curs_row)
        edit_move_down (edit, y - edit->curs_row, FALSE);
    else if (y < edit->curs_row)
        edit_move_up (edit, edit->curs_row - y, FALSE);
    else
        edit_move_to_prev_col (edit, edit_buffer_get_current_bol (&edit->buffer));

    if (event->msg == MSG_MOUSE_CLICK)
    {
        edit_mark_cmd (edit, TRUE);     /* reset */
        edit->highlight = 0;
    }

    done = (event->msg != MSG_MOUSE_DRAG);
    if (done)
        edit_mark_cmd (edit, FALSE);

    return done;
}

/* --------------------------------------------------------------------------------------------- */
/** Callback for the edit dialog */

static cb_ret_t
edit_dialog_callback (Widget * w, Widget * sender, widget_msg_t msg, int parm, void *data)
{
    WGroup *g = GROUP (w);
    WDialog *h = DIALOG (w);

    switch (msg)
    {
    case MSG_INIT:
        edit_dlg_init ();
        return MSG_HANDLED;

    case MSG_RESIZE:
        dlg_default_callback (w, NULL, MSG_RESIZE, 0, NULL);
        menubar_arrange (find_menubar (h));
        return MSG_HANDLED;

    case MSG_ACTION:
        {
            /* Handle shortcuts, menu, and buttonbar. */

            cb_ret_t result;

            result = edit_dialog_command_execute (h, parm);

            /* We forward any commands coming from the menu, and which haven't been
               handled by the dialog, to the focused WEdit window. */
            if (result == MSG_NOT_HANDLED && sender == WIDGET (find_menubar (h)))
                result = send_message (g->current->data, NULL, MSG_ACTION, parm, NULL);

            return result;
        }

    case MSG_KEY:
        {
            Widget *we = WIDGET (g->current->data);
            cb_ret_t ret = MSG_NOT_HANDLED;

            if (edit_widget_is_editor (we))
            {
                gboolean ext_mode;
                long command;

                /* keep and then extmod flag */
                ext_mode = we->ext_mode;
                command = widget_lookup_key (we, parm);
                we->ext_mode = ext_mode;

                if (command == CK_IgnoreKey)
                    we->ext_mode = FALSE;
                else
                {
                    ret = edit_dialog_command_execute (h, command);
                    /* if command was not handled, keep the extended mode
                       for the further key processing */
                    if (ret == MSG_HANDLED)
                        we->ext_mode = FALSE;
                }
            }

            /*
             * Due to the "end of bracket" escape the editor sees input with is_idle() == false
             * (expects more characters) and hence doesn't yet refresh the screen, but then
             * no further characters arrive (there's only an "end of bracket" which is swallowed
             * by tty_get_event()), so you end up with a screen that's not refreshed after pasting.
             * So let's trigger an IDLE signal.
             */
            if (!is_idle ())
                widget_idle (w, TRUE);
            return ret;
        }

        /* hardcoded menu hotkeys (see edit_drop_hotkey_menu) */
    case MSG_UNHANDLED_KEY:
        return edit_drop_hotkey_menu (h, parm) ? MSG_HANDLED : MSG_NOT_HANDLED;

    case MSG_VALIDATE:
        edit_quit (h);
        return MSG_HANDLED;

    case MSG_DESTROY:
        edit_dlg_deinit ();
        return MSG_HANDLED;

    case MSG_IDLE:
        widget_idle (w, FALSE);
        return send_message (g->current->data, NULL, MSG_IDLE, 0, NULL);

    default:
        return dlg_default_callback (w, sender, msg, parm, data);
    }
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Handle mouse events of editor screen.
 *
 * @param w Widget object (the editor)
 * @param msg mouse event message
 * @param event mouse event data
 */
static void
edit_dialog_mouse_callback (Widget * w, mouse_msg_t msg, mouse_event_t * event)
{
    gboolean unhandled = TRUE;

    if (msg == MSG_MOUSE_DOWN && event->y == 0)
    {
        WGroup *g = GROUP (w);
        WDialog *h = DIALOG (w);
        WMenuBar *b;

        b = find_menubar (h);

        if (!widget_get_state (WIDGET (b), WST_FOCUSED))
        {
            /* menubar */

            GList *l;
            GList *top = NULL;
            int x;

            /* Try find top fullscreen window */
            for (l = g->widgets; l != NULL; l = g_list_next (l))
                if (edit_widget_is_editor (CONST_WIDGET (l->data))
                    && ((WEdit *) l->data)->fullscreen)
                    top = l;

            /* Handle fullscreen/close buttons in the top line */
            x = w->cols - 6;

            if (top != NULL && event->x >= x)
            {
                WEdit *e = (WEdit *) top->data;

                if (top != g->current)
                {
                    /* Window is not active. Activate it */
                    widget_select (WIDGET (e));
                }

                /* Handle buttons */
                if (event->x - x <= 2)
                    edit_toggle_fullscreen (e);
                else
                    send_message (h, NULL, MSG_ACTION, CK_Close, NULL);

                unhandled = FALSE;
            }

            if (unhandled)
                menubar_activate (b, drop_menus, -1);
        }
    }

    /* Continue handling of unhandled event in window or menu */
    event->result.abort = unhandled;
}

/* --------------------------------------------------------------------------------------------- */

static cb_ret_t
edit_dialog_bg_callback (Widget * w, Widget * sender, widget_msg_t msg, int parm, void *data)
{
    switch (msg)
    {
    case MSG_INIT:
        {
            Widget *wo = WIDGET (w->owner);

            w->y = wo->y + 1;
            w->x = wo->x;
            w->lines = wo->lines - 2;
            w->cols = wo->cols;
            w->pos_flags |= WPOS_KEEP_ALL;

            return MSG_HANDLED;
        }

    default:
        return background_callback (w, sender, msg, parm, data);
    }
}

/* --------------------------------------------------------------------------------------------- */

static cb_ret_t
edit_callback (Widget * w, Widget * sender, widget_msg_t msg, int parm, void *data)
{
    WEdit *e = (WEdit *) w;

    switch (msg)
    {
    case MSG_FOCUS:
        edit_set_buttonbar (e, find_buttonbar (DIALOG (w->owner)));
        return MSG_HANDLED;

    case MSG_DRAW:
        e->force |= REDRAW_COMPLETELY;
        edit_update_screen (e);
        return MSG_HANDLED;

    case MSG_KEY:
        {
            int cmd, ch;
            cb_ret_t ret = MSG_NOT_HANDLED;

            /* The user may override the access-keys for the menu bar. */
            if (macro_index == -1 && edit_execute_macro (e, parm))
            {
                edit_update_screen (e);
                ret = MSG_HANDLED;
            }
            else if (edit_translate_key (e, parm, &cmd, &ch))
            {
                edit_execute_key_command (e, cmd, ch);
                edit_update_screen (e);
                ret = MSG_HANDLED;
            }

            return ret;
        }

    case MSG_ACTION:
        /* command from menubar or buttonbar */
        edit_execute_key_command (e, parm, -1);
        edit_update_screen (e);
        return MSG_HANDLED;

    case MSG_CURSOR:
        {
            int y, x;

            y = (e->fullscreen ? 0 : 1) + EDIT_TEXT_VERTICAL_OFFSET + e->curs_row;
            x = (e->fullscreen ? 0 : 1) + EDIT_TEXT_HORIZONTAL_OFFSET + option_line_state_width +
                e->curs_col + e->start_col + e->over_col;

            widget_gotoyx (w, y, x);
            return MSG_HANDLED;
        }

    case MSG_IDLE:
        edit_update_screen (e);
        return MSG_HANDLED;

    case MSG_DESTROY:
        edit_clean (e);
        return MSG_HANDLED;

    default:
        return widget_default_callback (w, sender, msg, parm, data);
    }
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Handle move/resize mouse events.
 */
static void
edit_mouse_handle_move_resize (Widget * w, mouse_msg_t msg, mouse_event_t * event)
{
    WEdit *edit = (WEdit *) (w);
    Widget *h = WIDGET (w->owner);
    int global_x, global_y;

    if (msg == MSG_MOUSE_UP)
    {
        /* Exit move/resize mode. */
        edit_execute_cmd (edit, CK_Enter, -1);
        edit_update_screen (edit);      /* Paint the buttonbar over our possibly overlapping frame. */
        return;
    }

    if (msg != MSG_MOUSE_DRAG)
        /**
         * We ignore any other events. Specifically, MSG_MOUSE_DOWN.
         *
         * When the move/resize is initiated by the menu, we let the user
         * stop it by clicking with the mouse. Which is why we don't want
         * a mouse down to affect the window.
         */
        return;

    /* Convert point to global coordinates for easier calculations. */
    global_x = event->x + w->x;
    global_y = event->y + w->y;

    /* Clamp the point to the dialog's client area. */
    global_y = CLAMP (global_y, h->y + 1, h->y + h->lines - 2); /* Status line, buttonbar */
    global_x = CLAMP (global_x, h->x, h->x + h->cols - 1);      /* Currently a no-op, as the dialog has no left/right margins. */

    if (edit->drag_state == MCEDIT_DRAG_MOVE)
    {
        w->y = global_y;
        w->x = global_x - edit->drag_state_start;
    }
    else if (edit->drag_state == MCEDIT_DRAG_RESIZE)
    {
        w->lines = MAX (WINDOW_MIN_LINES, global_y - w->y + 1);
        w->cols = MAX (WINDOW_MIN_COLS, global_x - w->x + 1);
    }

    edit->force |= REDRAW_COMPLETELY;   /* Not really needed as WEdit's MSG_DRAW already does this. */

    /* We draw the whole dialog because dragging/resizing exposes area beneath. */
    widget_draw (WIDGET (w->owner));
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Handle mouse events of editor window
 *
 * @param w Widget object (the editor window)
 * @param msg mouse event message
 * @param event mouse event data
 */
static void
edit_mouse_callback (Widget * w, mouse_msg_t msg, mouse_event_t * event)
{
    WEdit *edit = (WEdit *) w;
    /* buttons' distance from right edge */
    int dx = edit->fullscreen ? 0 : 2;
    /* location of 'Close' and 'Toggle fullscreen' pictograms */
    int close_x, toggle_fullscreen_x;

    close_x = (w->cols - 1) - dx - 1;
    toggle_fullscreen_x = close_x - 3;

    if (edit->drag_state != MCEDIT_DRAG_NONE)
    {
        /* window is being resized/moved */
        edit_mouse_handle_move_resize (w, msg, event);
        return;
    }

    /* If it's the last line on the screen, we abort the event to make the
     * system channel it to the overlapping buttonbar instead. We have to do
     * this because a WEdit has the WOP_TOP_SELECT flag, which makes it above
     * the buttonbar in Z-order. */
    if (msg == MSG_MOUSE_DOWN && (event->y + w->y == LINES - 1))
    {
        event->result.abort = TRUE;
        return;
    }

    switch (msg)
    {
    case MSG_MOUSE_DOWN:
        widget_select (w);
        edit_update_curs_row (edit);
        edit_update_curs_col (edit);

        if (!edit->fullscreen)
        {
            if (event->y == 0)
            {
                if (event->x >= close_x - 1 && event->x <= close_x + 1)
                    ;           /* do nothing (see MSG_MOUSE_CLICK) */
                else if (event->x >= toggle_fullscreen_x - 1 && event->x <= toggle_fullscreen_x + 1)
                    ;           /* do nothing (see MSG_MOUSE_CLICK) */
                else
                {
                    /* start window move */
                    edit_execute_cmd (edit, CK_WindowMove, -1);
                    edit_update_screen (edit);  /* Paint the buttonbar over our possibly overlapping frame. */
                    edit->drag_state_start = event->x;
                }
                break;
            }

            if (event->y == w->lines - 1 && event->x == w->cols - 1)
            {
                /* bottom-right corner -- start window resize */
                edit_execute_cmd (edit, CK_WindowResize, -1);
                break;
            }
        }

        MC_FALLTHROUGH;         /* to start/stop text selection */

    case MSG_MOUSE_UP:
        edit_update_cursor (edit, event);
        edit_total_update (edit);
        break;

    case MSG_MOUSE_CLICK:
        if (event->y == 0)
        {
            if (event->x >= close_x - 1 && event->x <= close_x + 1)
                send_message (w->owner, NULL, MSG_ACTION, CK_Close, NULL);
            else if (event->x >= toggle_fullscreen_x - 1 && event->x <= toggle_fullscreen_x + 1)
                edit_toggle_fullscreen (edit);
            else if (!edit->fullscreen && event->count == GPM_DOUBLE)
                /* double click on top line (toggle fullscreen) */
                edit_toggle_fullscreen (edit);
        }
        else if (event->count == GPM_DOUBLE)
        {
            /* double click */
            edit_mark_current_word_cmd (edit);
            edit_total_update (edit);
        }
        else if (event->count == GPM_TRIPLE)
        {
            /* triple click: works in GPM only, not in xterm */
            edit_mark_current_line_cmd (edit);
            edit_total_update (edit);
        }
        break;

    case MSG_MOUSE_DRAG:
        edit_update_cursor (edit, event);
        edit_total_update (edit);
        break;

    case MSG_MOUSE_SCROLL_UP:
        edit_move_up (edit, 2, TRUE);
        edit_total_update (edit);
        break;

    case MSG_MOUSE_SCROLL_DOWN:
        edit_move_down (edit, 2, TRUE);
        edit_total_update (edit);
        break;

    default:
        break;
    }
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */
/**
 * Edit one file.
 *
 * @param file_vpath file object
 * @param line       line number
 * @return TRUE if no errors was occurred, FALSE otherwise
 */

gboolean
edit_file (const vfs_path_t * file_vpath, long line)
{
    mcedit_arg_t arg = { (vfs_path_t *) file_vpath, line };
    GList *files;
    gboolean ok;

    files = g_list_prepend (NULL, &arg);
    ok = edit_files (files);
    g_list_free (files);

    return ok;
}

/* --------------------------------------------------------------------------------------------- */

gboolean
edit_files (const GList * files)
{
    static gboolean made_directory = FALSE;
    WDialog *edit_dlg;
    WGroup *g;
    WMenuBar *menubar;
    Widget *w, *wd;
    const GList *file;
    gboolean ok = FALSE;

    if (!made_directory)
    {
        char *dir;

        dir = mc_build_filename (mc_config_get_cache_path (), EDIT_HOME_DIR, (char *) NULL);
        made_directory = (mkdir (dir, 0700) != -1 || errno == EEXIST);
        g_free (dir);

        dir = mc_build_filename (mc_config_get_path (), EDIT_HOME_DIR, (char *) NULL);
        made_directory = (mkdir (dir, 0700) != -1 || errno == EEXIST);
        g_free (dir);

        dir = mc_build_filename (mc_config_get_data_path (), EDIT_HOME_DIR, (char *) NULL);
        made_directory = (mkdir (dir, 0700) != -1 || errno == EEXIST);
        g_free (dir);
    }

    /* Create a new dialog and add it widgets to it */
    edit_dlg =
        dlg_create (FALSE, 0, 0, 1, 1, WPOS_FULLSCREEN, FALSE, NULL, edit_dialog_callback,
                    edit_dialog_mouse_callback, "[Internal File Editor]", NULL);
    wd = WIDGET (edit_dlg);
    widget_want_tab (wd, TRUE);
    wd->keymap = editor_map;
    wd->ext_keymap = editor_x_map;

    edit_dlg->get_shortcut = edit_get_shortcut;
    edit_dlg->get_title = edit_get_title;

    g = GROUP (edit_dlg);

    edit_dlg->bg =
        WIDGET (background_new
                (1, 0, wd->lines - 2, wd->cols, EDITOR_BACKGROUND, ' ', edit_dialog_bg_callback));
    group_add_widget (g, edit_dlg->bg);

    menubar = menubar_new (NULL);
    w = WIDGET (menubar);
    group_add_widget_autopos (g, w, w->pos_flags, NULL);
    edit_init_menu (menubar);

    w = WIDGET (buttonbar_new ());
    group_add_widget_autopos (g, w, w->pos_flags, NULL);

    for (file = files; file != NULL; file = g_list_next (file))
    {
        mcedit_arg_t *f = (mcedit_arg_t *) file->data;
        gboolean f_ok;

        f_ok = edit_add_window (edit_dlg, wd->y + 1, wd->x, wd->lines - 2, wd->cols, f->file_vpath,
                                f->line_number);
        /* at least one file has been opened succefully */
        ok = ok || f_ok;
    }

    if (ok)
        dlg_run (edit_dlg);

    if (!ok || widget_get_state (wd, WST_CLOSED))
        widget_destroy (wd);

    return ok;
}

/* --------------------------------------------------------------------------------------------- */

const char *
edit_get_file_name (const WEdit * edit)
{
    return vfs_path_as_str (edit->filename_vpath);
}

/* --------------------------------------------------------------------------------------------- */

WEdit *
find_editor (const WDialog * h)
{
    const WGroup *g = CONST_GROUP (h);

    if (edit_widget_is_editor (CONST_WIDGET (g->current->data)))
        return (WEdit *) g->current->data;
    return (WEdit *) widget_find_by_type (CONST_WIDGET (h), edit_callback);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Check if widget is an WEdit class.
 *
 * @param w probably editor object
 * @return TRUE if widget is an WEdit class, FALSE otherwise
 */

gboolean
edit_widget_is_editor (const Widget * w)
{
    return (w != NULL && w->callback == edit_callback);
}

/* --------------------------------------------------------------------------------------------- */

void
edit_update_screen (WEdit * e)
{
    edit_scroll_screen_over_cursor (e);
    edit_update_curs_col (e);
    edit_status (e, widget_get_state (WIDGET (e), WST_FOCUSED));

    /* pop all events for this window for internal handling */
    if (!is_idle ())
        e->force |= REDRAW_PAGE;
    else
    {
        if ((e->force & REDRAW_COMPLETELY) != 0)
            e->force |= REDRAW_PAGE;
        edit_render_keypress (e);
    }

    widget_draw (WIDGET (find_buttonbar (DIALOG (WIDGET (e)->owner))));
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Save current window size.
 *
 * @param edit editor object
 */

void
edit_save_size (WEdit * edit)
{
    Widget *w = WIDGET (edit);

    rect_init (&edit->loc_prev, w->y, w->x, w->lines, w->cols);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Create new editor window and insert it into editor screen.
 *
 * @param h     editor dialog (screen)
 * @param y     y coordinate
 * @param x     x coordinate
 * @param lines window height
 * @param cols  window width
 * @param f     file object
 * @param fline line number in file
 * @return TRUE if new window was successfully created and inserted into editor screen,
 *         FALSE otherwise
 */

gboolean
edit_add_window (WDialog * h, int y, int x, int lines, int cols, const vfs_path_t * f, long fline)
{
    WEdit *edit;
    Widget *w;

    edit = edit_init (NULL, y, x, lines, cols, f, fline);
    if (edit == NULL)
        return FALSE;

    w = WIDGET (edit);
    w->callback = edit_callback;
    w->mouse_callback = edit_mouse_callback;

    group_add_widget_autopos (GROUP (h), w, WPOS_KEEP_ALL, NULL);
    edit_set_buttonbar (edit, find_buttonbar (h));
    widget_draw (WIDGET (h));

    return TRUE;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Handle move/resize events.
 *
 * @param edit    editor object
 * @param command action id
 * @return TRUE if the action was handled, FALSE otherwise
 */

gboolean
edit_handle_move_resize (WEdit * edit, long command)
{
    Widget *w = WIDGET (edit);
    gboolean ret = FALSE;

    if (edit->fullscreen)
    {
        edit->drag_state = MCEDIT_DRAG_NONE;
        w->mouse.forced_capture = FALSE;
        return ret;
    }

    switch (edit->drag_state)
    {
    case MCEDIT_DRAG_NONE:
        /* possible start move/resize */
        switch (command)
        {
        case CK_WindowMove:
            edit->drag_state = MCEDIT_DRAG_MOVE;
            edit_save_size (edit);
            edit_status (edit, TRUE);   /* redraw frame and status */
            /**
             * If a user initiates a move by the menu, not by the mouse, we
             * make a subsequent mouse drag pull the frame from its middle.
             * (We can instead choose '0' to pull it from the corner.)
             */
            edit->drag_state_start = w->cols / 2;
            ret = TRUE;
            break;
        case CK_WindowResize:
            edit->drag_state = MCEDIT_DRAG_RESIZE;
            edit_save_size (edit);
            edit_status (edit, TRUE);   /* redraw frame and status */
            ret = TRUE;
            break;
        default:
            break;
        }
        break;

    case MCEDIT_DRAG_MOVE:
        switch (command)
        {
        case CK_WindowResize:
            edit->drag_state = MCEDIT_DRAG_RESIZE;
            ret = TRUE;
            break;
        case CK_Up:
        case CK_Down:
        case CK_Left:
        case CK_Right:
            edit_window_move (edit, command);
            ret = TRUE;
            break;
        case CK_Enter:
        case CK_WindowMove:
            edit->drag_state = MCEDIT_DRAG_NONE;
            edit_status (edit, TRUE);   /* redraw frame and status */
            MC_FALLTHROUGH;
        default:
            ret = TRUE;
            break;
        }
        break;

    case MCEDIT_DRAG_RESIZE:
        switch (command)
        {
        case CK_WindowMove:
            edit->drag_state = MCEDIT_DRAG_MOVE;
            ret = TRUE;
            break;
        case CK_Up:
        case CK_Down:
        case CK_Left:
        case CK_Right:
            edit_window_resize (edit, command);
            ret = TRUE;
            break;
        case CK_Enter:
        case CK_WindowResize:
            edit->drag_state = MCEDIT_DRAG_NONE;
            edit_status (edit, TRUE);   /* redraw frame and status */
            MC_FALLTHROUGH;
        default:
            ret = TRUE;
            break;
        }
        break;

    default:
        break;
    }

    /**
     * - We let the user stop a resize/move operation by clicking with the
     *   mouse anywhere. ("clicking" = pressing and releasing a button.)
     * - We let the user perform a resize/move operation by a mouse drag
     *   initiated anywhere.
     *
     * "Anywhere" means: inside or outside the window. We make this happen
     * with the 'forced_capture' flag.
     */
    w->mouse.forced_capture = (edit->drag_state != MCEDIT_DRAG_NONE);

    return ret;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Toggle window fuulscreen mode.
 *
 * @param edit editor object
 */

void
edit_toggle_fullscreen (WEdit * edit)
{
    Widget *w = WIDGET (edit);

    edit->fullscreen = !edit->fullscreen;
    edit->force = REDRAW_COMPLETELY;

    if (!edit->fullscreen)
    {
        edit_restore_size (edit);
        /* do not follow screen size on resize */
        w->pos_flags = WPOS_KEEP_DEFAULT;
    }
    else
    {
        Widget *h = WIDGET (w->owner);

        edit_save_size (edit);
        widget_set_size (w, h->y + 1, h->x, h->lines - 2, h->cols);
        /* follow screen size on resize */
        w->pos_flags = WPOS_KEEP_ALL;
        edit->force |= REDRAW_PAGE;
        edit_update_screen (edit);
    }
}

/* --------------------------------------------------------------------------------------------- */
