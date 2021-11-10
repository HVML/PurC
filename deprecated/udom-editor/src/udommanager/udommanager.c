/*
   Main dialog (file panels) of the Midnight Commander

   Copyright (C) 1994-2021
   Free Software Foundation, Inc.

   Written by:
   Miguel de Icaza, 1994, 1995, 1996, 1997
   Janne Kukonlehto, 1994, 1995
   Norbert Warmuth, 1997
   Andrew Borodin <aborodin@vmail.ru>, 2009, 2010, 2012, 2013, 2020
   Slava Zanko <slavazanko@gmail.com>, 2013

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

/** \file filemanager.c
 *  \brief Source: main dialog (file panels) of the Midnight Commander
 */

#include <config.h>

#include <ctype.h>
#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <pwd.h>                /* for username in xterm title */

#include "lib/global.h"
#include "lib/fileloc.h"        /* MC_HINT */

#include "lib/tty/tty.h"
#include "lib/tty/key.h"        /* KEY_M_* masks */
#include "lib/skin.h"
#include "lib/util.h"

#include "lib/vfs/vfs.h"

#include "src/args.h"
#include "src/subshell/subshell.h"
#include "src/execute.h"        /* toggle_subshell */
#include "src/setup.h"          /* variables */
#include "src/learn.h"          /* learn_keys() */
#include "src/keymap.h"
#include "lib/fileloc.h"        /* MC_FILEPOS_FILE */
#include "lib/keybind.h"
#include "lib/event.h"

#include "layout.h"
#include "command.h"            /* cmdline */

#include "src/editor/edit.h"

#include "src/consaver/cons.saver.h"    /* show_console_contents */
#include "src/file_history.h"   /* show_file_history() */

#include "udommanager.h"

/*** global variables ****************************************************************************/

/* When the modes are active, left_panel, right_panel and tree_panel */
/* point to a proper data structure.  You should check with the functions */
/* get_current_type and get_other_type the types of the panels before using */
/* this pointer variables */

/* The structures for the panels */
WPanel *left_panel = NULL;
WPanel *right_panel = NULL;
/* Pointer to the selected and unselected panel */
WPanel *current_panel = NULL;

/* The Menubar */
WMenuBar *the_menubar = NULL;
/* The widget where we draw the prompt */
WLabel *the_prompt;
/* The hint bar */
WLabel *the_hint;
/* The button bar */
WButtonBar *the_bar;

/* The prompt */
const char *mc_prompt = NULL;

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

static menu_t *left_menu, *right_menu;

/*** file scope functions ************************************************************************/

/** Stop MC main dialog and the current dialog if it exists.
  * Needed to provide fast exit from MC viewer or editor on shell exit */
static void
stop_dialogs (void)
{
    dlg_stop (filemanager);

    if (top_dlg != NULL)
        dlg_stop (DIALOG (top_dlg->data));
}

/* --------------------------------------------------------------------------------------------- */

static GList *
create_panel_menu (void)
{
    GList *entries = NULL;

    entries = g_list_prepend (entries, menu_entry_create (_("File listin&g"), CK_PanelListing));
    entries = g_list_prepend (entries, menu_entry_create (_("&Quick view"), CK_PanelQuickView));
    entries = g_list_prepend (entries, menu_entry_create (_("&Info"), CK_PanelInfo));
    entries = g_list_prepend (entries, menu_entry_create (_("&Tree"), CK_PanelTree));
    entries = g_list_prepend (entries, menu_separator_create ());
    entries =
        g_list_prepend (entries,
                        menu_entry_create (_("&Listing format..."), CK_SetupListingFormat));
    entries = g_list_prepend (entries, menu_entry_create (_("&Sort order..."), CK_Sort));
    entries = g_list_prepend (entries, menu_entry_create (_("&Filter..."), CK_Filter));
    entries = g_list_prepend (entries, menu_separator_create ());
    entries = g_list_prepend (entries, menu_entry_create (_("Paneli&ze"), CK_Panelize));
    entries = g_list_prepend (entries, menu_separator_create ());
    entries = g_list_prepend (entries, menu_entry_create (_("&Rescan"), CK_Reread));

    return g_list_reverse (entries);
}

/* --------------------------------------------------------------------------------------------- */

static GList *
create_file_menu (void)
{
    GList *entries = NULL;

    entries = g_list_prepend (entries, menu_entry_create (_("&View"), CK_View));
    entries = g_list_prepend (entries, menu_entry_create (_("Vie&w file..."), CK_ViewFile));
    entries = g_list_prepend (entries, menu_entry_create (_("&Filtered view"), CK_ViewFiltered));
    entries = g_list_prepend (entries, menu_entry_create (_("&Edit"), CK_Edit));
    entries = g_list_prepend (entries, menu_entry_create (_("&Copy"), CK_Copy));
    entries = g_list_prepend (entries, menu_entry_create (_("C&hmod"), CK_ChangeMode));
    entries = g_list_prepend (entries, menu_entry_create (_("&Link"), CK_Link));
    entries = g_list_prepend (entries, menu_entry_create (_("&Symlink"), CK_LinkSymbolic));
    entries =
        g_list_prepend (entries,
                        menu_entry_create (_("Relative symlin&k"), CK_LinkSymbolicRelative));
    entries = g_list_prepend (entries, menu_entry_create (_("Edit s&ymlink"), CK_LinkSymbolicEdit));
    entries = g_list_prepend (entries, menu_entry_create (_("Ch&own"), CK_ChangeOwn));
    entries =
        g_list_prepend (entries, menu_entry_create (_("&Advanced chown"), CK_ChangeOwnAdvanced));
    entries = g_list_prepend (entries, menu_entry_create (_("&Rename/Move"), CK_Move));
    entries = g_list_prepend (entries, menu_entry_create (_("&Mkdir"), CK_MakeDir));
    entries = g_list_prepend (entries, menu_entry_create (_("&Delete"), CK_Delete));
    entries = g_list_prepend (entries, menu_entry_create (_("&Quick cd"), CK_CdQuick));
    entries = g_list_prepend (entries, menu_separator_create ());
    entries = g_list_prepend (entries, menu_entry_create (_("Select &group"), CK_Select));
    entries = g_list_prepend (entries, menu_entry_create (_("U&nselect group"), CK_Unselect));
    entries = g_list_prepend (entries, menu_entry_create (_("&Invert selection"), CK_SelectInvert));
    entries = g_list_prepend (entries, menu_separator_create ());
    entries = g_list_prepend (entries, menu_entry_create (_("E&xit"), CK_Quit));

    return g_list_reverse (entries);
}

/* --------------------------------------------------------------------------------------------- */

static GList *
create_command_menu (void)
{
    /* I know, I'm lazy, but the tree widget when it's not running
     * as a panel still has some problems, I have not yet finished
     * the WTree widget port, sorry.
     */
    GList *entries = NULL;

    entries = g_list_prepend (entries, menu_entry_create (_("&User menu"), CK_UserMenu));
    entries = g_list_prepend (entries, menu_entry_create (_("&Directory tree"), CK_Tree));
    entries = g_list_prepend (entries, menu_entry_create (_("&Find file"), CK_Find));
    entries = g_list_prepend (entries, menu_entry_create (_("S&wap panels"), CK_Swap));
    entries = g_list_prepend (entries, menu_entry_create (_("Switch &panels on/off"), CK_Shell));
    entries =
        g_list_prepend (entries, menu_entry_create (_("&Compare directories"), CK_CompareDirs));
    entries =
        g_list_prepend (entries, menu_entry_create (_("E&xternal panelize"), CK_ExternalPanelize));
    entries = g_list_prepend (entries, menu_entry_create (_("Show directory s&izes"), CK_DirSize));
    entries = g_list_prepend (entries, menu_separator_create ());
    entries = g_list_prepend (entries, menu_entry_create (_("Command &history"), CK_History));
    entries =
        g_list_prepend (entries,
                        menu_entry_create (_("Viewed/edited files hi&story"),
                                           CK_EditorViewerHistory));
    entries = g_list_prepend (entries, menu_entry_create (_("Di&rectory hotlist"), CK_HotList));
#ifdef ENABLE_VFS
    entries = g_list_prepend (entries, menu_entry_create (_("&Active VFS list"), CK_VfsList));
#endif
#ifdef ENABLE_BACKGROUND
    entries = g_list_prepend (entries, menu_entry_create (_("&Background jobs"), CK_Jobs));
#endif
    entries = g_list_prepend (entries, menu_entry_create (_("Screen lis&t"), CK_ScreenList));
    entries = g_list_prepend (entries, menu_separator_create ());
#ifdef LISTMODE_EDITOR
    entries = g_list_prepend (entries, menu_entry_create (_("&Listing format edit"), CK_ListMode));
#endif
#if defined (ENABLE_VFS_UNDELFS) || defined (LISTMODE_EDITOR)
    entries = g_list_prepend (entries, menu_separator_create ());
#endif
    entries =
        g_list_prepend (entries,
                        menu_entry_create (_("Edit &extension file"), CK_EditExtensionsFile));
    entries = g_list_prepend (entries, menu_entry_create (_("Edit &menu file"), CK_EditUserMenu));
    entries =
        g_list_prepend (entries,
                        menu_entry_create (_("Edit hi&ghlighting group file"),
                                           CK_EditFileHighlightFile));

    return g_list_reverse (entries);
}

/* --------------------------------------------------------------------------------------------- */

static GList *
create_options_menu (void)
{
    GList *entries = NULL;

    entries = g_list_prepend (entries, menu_entry_create (_("&Configuration..."), CK_Options));
    entries = g_list_prepend (entries, menu_entry_create (_("&Layout..."), CK_OptionsLayout));
    entries = g_list_prepend (entries, menu_entry_create (_("&Panel options..."), CK_OptionsPanel));
    entries =
        g_list_prepend (entries, menu_entry_create (_("C&onfirmation..."), CK_OptionsConfirm));
    entries =
        g_list_prepend (entries, menu_entry_create (_("&Appearance..."), CK_OptionsAppearance));
    entries =
        g_list_prepend (entries, menu_entry_create (_("&Display bits..."), CK_OptionsDisplayBits));
    entries = g_list_prepend (entries, menu_entry_create (_("Learn &keys..."), CK_LearnKeys));
#ifdef ENABLE_VFS
    entries = g_list_prepend (entries, menu_entry_create (_("&Virtual FS..."), CK_OptionsVfs));
#endif
    entries = g_list_prepend (entries, menu_separator_create ());
    entries = g_list_prepend (entries, menu_entry_create (_("&Save setup"), CK_SaveSetup));

    return g_list_reverse (entries);
}

/* --------------------------------------------------------------------------------------------- */

static void
init_menu (void)
{
    left_menu = create_menu ("", create_panel_menu (), "[Left and Right Menus]");
    menubar_add_menu (the_menubar, left_menu);
    menubar_add_menu (the_menubar, create_menu (_("&File"), create_file_menu (), "[File Menu]"));
    menubar_add_menu (the_menubar,
                      create_menu (_("&Command"), create_command_menu (), "[Command Menu]"));
    menubar_add_menu (the_menubar,
                      create_menu (_("&Options"), create_options_menu (), "[Options Menu]"));
    right_menu = create_menu ("", create_panel_menu (), "[Left and Right Menus]");
    menubar_add_menu (the_menubar, right_menu);
    update_menu ();
}

/* --------------------------------------------------------------------------------------------- */

static void
menu_last_selected_cmd (void)
{
    menubar_activate (the_menubar, drop_menus, -1);
}

/* --------------------------------------------------------------------------------------------- */

static void
menu_cmd (void)
{
    int selected;

    if ((get_current_index () == 0) == current_panel->active)
        selected = 0;
    else
        selected = g_list_length (the_menubar->menu) - 1;

    menubar_activate (the_menubar, drop_menus, selected);
}

/* --------------------------------------------------------------------------------------------- */

static char *
midnight_get_shortcut (long command)
{
    const char *ext_map;
    const char *shortcut = NULL;

    shortcut = keybind_lookup_keymap_shortcut (filemanager_map, command);
    if (shortcut != NULL)
        return g_strdup (shortcut);

    shortcut = keybind_lookup_keymap_shortcut (panel_map, command);
    if (shortcut != NULL)
        return g_strdup (shortcut);

    ext_map = keybind_lookup_keymap_shortcut (filemanager_map, CK_ExtendedKeyMap);
    if (ext_map != NULL)
        shortcut = keybind_lookup_keymap_shortcut (filemanager_x_map, command);
    if (shortcut != NULL)
        return g_strdup_printf ("%s %s", ext_map, shortcut);

    return NULL;
}

/* --------------------------------------------------------------------------------------------- */

static char *
midnight_get_title (const WDialog * h, size_t len)
{
    char *path;
    char *login;
    char *p;

    (void) h;

    title_path_prepare (&path, &login);

    p = g_strdup_printf ("%s [%s]:%s", _("Panels:"), login, path);
    g_free (path);
    g_free (login);
    path = g_strdup (str_trunc (p, len - 4));
    g_free (p);

    return path;
}

/* --------------------------------------------------------------------------------------------- */

static void
toggle_panels_split (void)
{
    panels_layout.horizontal_split = !panels_layout.horizontal_split;
    layout_change ();
    do_refresh ();
}

/* --------------------------------------------------------------------------------------------- */

#ifdef ENABLE_VFS
/* event helper */
static gboolean
check_panel_timestamp (const WPanel * panel, panel_view_mode_t mode, struct vfs_class *vclass,
                       vfsid id)
{
    if (mode == view_listing)
    {
        const vfs_path_element_t *path_element;

        path_element = vfs_path_get_by_index (panel->cwd_vpath, -1);

        if (path_element->class != vclass)
            return FALSE;

        if (vfs_getid (panel->cwd_vpath) != id)
            return FALSE;
    }
    return TRUE;
}

/* --------------------------------------------------------------------------------------------- */

/* event callback */
static gboolean
check_current_panel_timestamp (const gchar * event_group_name, const gchar * event_name,
                               gpointer init_data, gpointer data)
{
    ev_vfs_stamp_create_t *event_data = (ev_vfs_stamp_create_t *) data;

    (void) event_group_name;
    (void) event_name;
    (void) init_data;

    event_data->ret =
        check_panel_timestamp (current_panel, get_current_type (), event_data->vclass,
                               event_data->id);
    return !event_data->ret;
}

/* --------------------------------------------------------------------------------------------- */

/* event callback */
static gboolean
check_other_panel_timestamp (const gchar * event_group_name, const gchar * event_name,
                             gpointer init_data, gpointer data)
{
    ev_vfs_stamp_create_t *event_data = (ev_vfs_stamp_create_t *) data;

    (void) event_group_name;
    (void) event_name;
    (void) init_data;

    event_data->ret =
        check_panel_timestamp (other_panel, get_other_type (), event_data->vclass, event_data->id);
    return !event_data->ret;
}
#endif /* ENABLE_VFS */

/* --------------------------------------------------------------------------------------------- */

/* event callback */
static gboolean
print_vfs_message (const gchar * event_group_name, const gchar * event_name,
                   gpointer init_data, gpointer data)
{
    ev_vfs_print_message_t *event_data = (ev_vfs_print_message_t *) data;

    (void) event_group_name;
    (void) event_name;
    (void) init_data;

    if (mc_global.midnight_shutdown)
        goto ret;

    if (!mc_global.message_visible || the_hint == NULL || WIDGET (the_hint)->owner == NULL)
    {
        int col, row;

        if (!nice_rotating_dash || (ok_to_refresh <= 0))
            goto ret;

        /* Preserve current cursor position */
        tty_getyx (&row, &col);

        tty_gotoyx (0, 0);
        tty_setcolor (NORMAL_COLOR);
        tty_print_string (str_fit_to_term (event_data->msg, COLS - 1, J_LEFT));

        /* Restore cursor position */
        tty_gotoyx (row, col);
        mc_refresh ();
        goto ret;
    }

    if (mc_global.message_visible)
        set_hintbar (event_data->msg);

  ret:
    MC_PTR_FREE (event_data->msg);
    return TRUE;
}

/* --------------------------------------------------------------------------------------------- */

static void
create_panels (void)
{
}

/* --------------------------------------------------------------------------------------------- */

static void
setup_mc (void)
{
    tty_display_8bit (mc_global.eight_bit_clean);

    if ((tty_baudrate () < 9600) || mc_global.tty.slow_terminal)
        verbose = FALSE;
}

/* --------------------------------------------------------------------------------------------- */

static void
setup_dummy_mc (void)
{
    vfs_path_t *vpath;
    char *d;
    int ret;

    d = _vfs_get_cwd ();
    setup_mc ();
    vpath = vfs_path_from_str (d);
    ret = mc_chdir (vpath);
    (void) ret;
    vfs_path_free (vpath, TRUE);
    g_free (d);
}

/* --------------------------------------------------------------------------------------------- */

static void
done_mc (void)
{
    /* Setup shutdown
     *
     * We sync the profiles since the hotlist may have changed, while
     * we only change the setup data if we have the auto save feature set
     */

    save_setup (auto_save_setup, panels_options.auto_save_setup);

    vfs_stamp_path (vfs_get_raw_current_dir ());
}

/* --------------------------------------------------------------------------------------------- */

static void
create_file_manager (void)
{
    Widget *w = WIDGET (filemanager);
    WGroup *g = GROUP (filemanager);

    w->keymap = filemanager_map;
    w->ext_keymap = filemanager_x_map;

    filemanager->get_shortcut = midnight_get_shortcut;
    filemanager->get_title = midnight_get_title;
    /* allow rebind tab */
    widget_want_tab (w, TRUE);

    the_menubar = menubar_new (NULL);
    group_add_widget (g, the_menubar);
    init_menu ();

    create_panels ();
    group_add_widget (g, get_panel_widget (0));
    group_add_widget (g, get_panel_widget (1));

    the_hint = label_new (0, 0, 0);
    the_hint->transparent = TRUE;
    the_hint->auto_adjust_cols = 0;
    WIDGET (the_hint)->cols = COLS;
    group_add_widget (g, the_hint);

    cmdline = command_new (0, 0, 0);
    group_add_widget (g, cmdline);

    the_prompt = label_new (0, 0, mc_prompt);
    the_prompt->transparent = TRUE;
    group_add_widget (g, the_prompt);

    the_bar = buttonbar_new ();
    group_add_widget (g, the_bar);
    midnight_set_buttonbar (the_bar);

    /* Must be done after creation of cmdline and promt widgets to avoid potential
       NULL dereference in load_prompt() -> ... -> setup_cmdline() -> label_set_text(). */
    if (mc_global.tty.use_subshell)
        add_select_channel (mc_global.tty.subshell_pty, load_prompt, NULL);
}

/* --------------------------------------------------------------------------------------------- */

/** result must be free'd (I think this should go in util.c) */
static vfs_path_t *
prepend_cwd_on_local (const char *filename)
{
    vfs_path_t *vpath;

    vpath = vfs_path_from_str (filename);
    if (!vfs_file_is_local (vpath) || g_path_is_absolute (filename))
        return vpath;

    vfs_path_free (vpath, TRUE);

    return vfs_path_append_new (vfs_get_raw_current_dir (), filename, (char *) NULL);
}

/* --------------------------------------------------------------------------------------------- */

static gboolean
quit_cmd_internal (int quiet)
{
    int q = quit;
    size_t n;

    n = dialog_switch_num () - 1;
    if (n != 0)
    {
        char msg[BUF_MEDIUM];

        g_snprintf (msg, sizeof (msg),
                    ngettext ("You have %zu opened screen. Quit anyway?",
                              "You have %zu opened screens. Quit anyway?", n), n);

        if (query_dialog (_("The Midnight Commander"), msg, D_NORMAL, 2, _("&Yes"), _("&No")) != 0)
            return FALSE;
        q = 1;
    }
    else if (quiet || !confirm_exit)
        q = 1;
    else if (query_dialog (_("The Midnight Commander"),
                           _("Do you really want to quit the Midnight Commander?"),
                           D_NORMAL, 2, _("&Yes"), _("&No")) == 0)
        q = 1;

    if (q != 0)
    {
        if (!mc_global.tty.use_subshell)
            stop_dialogs ();
        else if ((q = exit_subshell ()? 1 : 0) != 0)
            stop_dialogs ();
    }

    if (q != 0)
        quit |= 1;
    return (quit != 0);
}

/* --------------------------------------------------------------------------------------------- */

static gboolean
quit_cmd (void)
{
    return quit_cmd_internal (0);
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Repaint the contents of the panels without frames.  To schedule panel
 * for repainting, set panel->dirty to TRUE.  There are many reasons why
 * the panels need to be repainted, and this is a costly operation, so
 * it's done once per event.
 */

static void
update_dirty_panels (void)
{
    if (get_current_type () == view_listing && current_panel->dirty)
        widget_draw (WIDGET (current_panel));

    if (get_other_type () == view_listing && other_panel->dirty)
        widget_draw (WIDGET (other_panel));
}

/* --------------------------------------------------------------------------------------------- */

static void
toggle_show_hidden (void)
{
    panels_options.show_dot_files = !panels_options.show_dot_files;
    update_panels (UP_RELOAD, UP_KEEPSEL);
    /* redraw panels forced */
    update_dirty_panels ();
}

/* --------------------------------------------------------------------------------------------- */

static cb_ret_t
midnight_execute_cmd (Widget * sender, long command)
{
    cb_ret_t res = MSG_HANDLED;

    (void) sender;

    /* stop quick search before executing any command */
    send_message (current_panel, NULL, MSG_ACTION, CK_SearchStop, NULL);

    switch (command)
    {
    case CK_QuitQuiet:
        quiet_quit_cmd ();
        break;
    case CK_Quit:
        quit_cmd ();
        break;
    case CK_Shell:
        toggle_subshell ();
        break;
    case CK_Suspend:
        mc_event_raise (MCEVENT_GROUP_CORE, "suspend", NULL);
        break;
    case CK_Cancel:
        /* don't close panels due to SIGINT */
        break;
    default:
        res = MSG_NOT_HANDLED;
    }

    return res;
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Whether the command-line should not respond to key events.
 *
 * This is TRUE if a QuickView or TreeView have the focus, as they're going
 * to consume some keys and there's no sense in passing to the command-line
 * just the leftovers.
 */
static gboolean
is_cmdline_mute (void)
{
    /* When one of panels is other than view_listing,
       current_panel points to view_listing panel all time independently of
       it's activity. Thus, we can't use get_current_type() here.
       current_panel should point to actualy current active panel
       independently of it's type. */
    return (!current_panel->active
            && (get_other_type () == view_quick || get_other_type () == view_tree));
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Handles the Enter key on the command-line.
 *
 * Returns TRUE if non-whitespace was indeed processed.
 */
static gboolean
handle_cmdline_enter (void)
{
    size_t i;

    for (i = 0; cmdline->buffer[i] != '\0' && whitespace (cmdline->buffer[i]); i++)
        ;

    if (cmdline->buffer[i] != '\0')
    {
        send_message (cmdline, NULL, MSG_KEY, '\n', NULL);
        return TRUE;
    }

    input_insert (cmdline, "", FALSE);
    cmdline->point = 0;

    return FALSE;
}

/* --------------------------------------------------------------------------------------------- */

static cb_ret_t
midnight_callback (Widget * w, Widget * sender, widget_msg_t msg, int parm, void *data)
{
    long command;

    switch (msg)
    {
    case MSG_INIT:
        panel_init ();
        setup_panels ();
        return MSG_HANDLED;

    case MSG_DRAW:
        load_hint (TRUE);
        group_default_callback (w, NULL, MSG_DRAW, 0, NULL);
        /* We handle the special case of the output lines */
        if (mc_global.tty.console_flag != '\0' && output_lines != 0)
        {
            unsigned char end_line;

            end_line = LINES - (mc_global.keybar_visible ? 1 : 0) - 1;
            show_console_contents (output_start_y, end_line - output_lines, end_line);
        }
        return MSG_HANDLED;

    case MSG_RESIZE:
        widget_adjust_position (w->pos_flags, &w->y, &w->x, &w->lines, &w->cols);
        setup_panels ();
        menubar_arrange (the_menubar);
        return MSG_HANDLED;

    case MSG_IDLE:
        /* We only need the first idle event to show user menu after start */
        widget_idle (w, FALSE);

        if (boot_current_is_left)
            widget_select (get_panel_widget (0));
        else
            widget_select (get_panel_widget (1));

        if (auto_menu)
            midnight_execute_cmd (NULL, CK_UserMenu);
        return MSG_HANDLED;

    case MSG_KEY:
        if (w->ext_mode)
        {
            command = widget_lookup_key (w, parm);
            if (command != CK_IgnoreKey)
                return midnight_execute_cmd (NULL, command);
        }

        /* FIXME: should handle all menu shortcuts before this point */
        if (widget_get_state (WIDGET (the_menubar), WST_FOCUSED))
            return MSG_NOT_HANDLED;

        if (parm == '\n' && !is_cmdline_mute ())
        {
            if (handle_cmdline_enter ())
                return MSG_HANDLED;
            /* Else: the panel will handle it. */
        }

        if ((!mc_global.tty.alternate_plus_minus
             || !(mc_global.tty.console_flag != '\0' || mc_global.tty.xterm_flag)) && !quote)
        {
            if (!only_leading_plus_minus)
            {
                /* Special treatement, since the input line will eat them */
                if (parm == '+')
                    return send_message (current_panel, filemanager, MSG_ACTION, CK_Select, NULL);

                if (parm == '\\' || parm == '-')
                    return send_message (current_panel, filemanager, MSG_ACTION, CK_Unselect, NULL);

                if (parm == '*')
                    return send_message (current_panel, filemanager, MSG_ACTION, CK_SelectInvert,
                                         NULL);
            }
            else if (!command_prompt || input_is_empty (cmdline))
            {
                /* Special treatement '+', '-', '\', '*' only when this is
                 * first char on input line
                 */
                if (parm == '+')
                    return send_message (current_panel, filemanager, MSG_ACTION, CK_Select, NULL);

                if (parm == '\\' || parm == '-')
                    return send_message (current_panel, filemanager, MSG_ACTION, CK_Unselect, NULL);

                if (parm == '*')
                    return send_message (current_panel, filemanager, MSG_ACTION, CK_SelectInvert,
                                         NULL);
            }
        }
        return MSG_NOT_HANDLED;

    case MSG_HOTKEY_HANDLED:
        if ((get_current_type () == view_listing))
        {
            current_panel->dirty = TRUE;        /* FIXME: unneeded? */
            send_message (current_panel, NULL, MSG_ACTION, CK_SearchStop, NULL);
        }
        return MSG_HANDLED;

    case MSG_UNHANDLED_KEY:
        {
            cb_ret_t v = MSG_NOT_HANDLED;

            command = widget_lookup_key (w, parm);
            if (command != CK_IgnoreKey)
                v = midnight_execute_cmd (NULL, command);

            if (v == MSG_NOT_HANDLED && command_prompt && !is_cmdline_mute ())
                v = send_message (cmdline, NULL, MSG_KEY, parm, NULL);

            return v;
        }

    case MSG_POST_KEY:
        if (!widget_get_state (WIDGET (the_menubar), WST_FOCUSED))
            update_dirty_panels ();
        return MSG_HANDLED;

    case MSG_ACTION:
        /* Handle shortcuts, menu, and buttonbar. */
        return midnight_execute_cmd (sender, parm);

    case MSG_DESTROY:
        panel_deinit ();
        return MSG_HANDLED;

    default:
        return dlg_default_callback (w, sender, msg, parm, data);
    }
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

void
update_menu (void)
{
    menu_set_name (left_menu, panels_layout.horizontal_split ? _("&Above") : _("&Left"));
    menu_set_name (right_menu, panels_layout.horizontal_split ? _("&Below") : _("&Right"));
    menubar_arrange (the_menubar);
    widget_set_visibility (WIDGET (the_menubar), menubar_visible);
}

/* --------------------------------------------------------------------------------------------- */

void
midnight_set_buttonbar (WButtonBar * b)
{
    Widget *w = WIDGET (filemanager);

    buttonbar_set_label (b, 1, Q_ ("ButtonBar|Help"), w->keymap, NULL);
    buttonbar_set_label (b, 2, Q_ ("ButtonBar|Menu"), w->keymap, NULL);
    buttonbar_set_label (b, 3, Q_ ("ButtonBar|View"), w->keymap, NULL);
    buttonbar_set_label (b, 4, Q_ ("ButtonBar|Edit"), w->keymap, NULL);
    buttonbar_set_label (b, 5, Q_ ("ButtonBar|Copy"), w->keymap, NULL);
    buttonbar_set_label (b, 6, Q_ ("ButtonBar|RenMov"), w->keymap, NULL);
    buttonbar_set_label (b, 7, Q_ ("ButtonBar|Mkdir"), w->keymap, NULL);
    buttonbar_set_label (b, 8, Q_ ("ButtonBar|Delete"), w->keymap, NULL);
    buttonbar_set_label (b, 9, Q_ ("ButtonBar|PullDn"), w->keymap, NULL);
    buttonbar_set_label (b, 10, Q_ ("ButtonBar|Quit"), w->keymap, NULL);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Return a random hint.  If force is TRUE, ignore the timeout.
 */

char *
get_random_hint (gboolean force)
{
    static const gint64 update_period = 60 * G_USEC_PER_SEC;
    static gint64 tv = 0;

    char *data, *result = NULL, *eop;
    size_t len, start;
    GIConv conv;

    /* Do not change hints more often than one minute */
    if (!force && !mc_time_elapsed (&tv, update_period))
        return g_strdup ("");

    data = load_mc_home_file (mc_global.share_data_dir, MC_HINT, NULL, &len);
    if (data == NULL)
        return NULL;

    /* get a random entry */
    srand ((unsigned int) (tv / G_USEC_PER_SEC));
    start = ((size_t) rand ()) % (len - 1);

    /* Search the start of paragraph */
    for (; start != 0; start--)
        if (data[start] == '\n' && data[start + 1] == '\n')
        {
            start += 2;
            break;
        }

    /* Search the end of paragraph */
    for (eop = data + start; *eop != '\0'; eop++)
    {
        if (*eop == '\n' && *(eop + 1) == '\n')
        {
            *eop = '\0';
            break;
        }
        if (*eop == '\n')
            *eop = ' ';
    }

    /* hint files are stored in utf-8 */
    /* try convert hint file from utf-8 to terminal encoding */
    conv = str_crt_conv_from ("UTF-8");
    if (conv != INVALID_CONV)
    {
        GString *buffer;

        buffer = g_string_sized_new (len - start);
        if (str_convert (conv, &data[start], buffer) != ESTR_FAILURE)
            result = g_string_free (buffer, FALSE);
        else
            g_string_free (buffer, TRUE);
        str_close_conv (conv);
    }
    else
        result = g_strndup (data + start, len - start);

    g_free (data);
    return result;
}


/* --------------------------------------------------------------------------------------------- */
/**
 * Load new hint and display it.
 * IF force is not 0, ignore the timeout.
 */

void
load_hint (gboolean force)
{
    char *hint;

    if (WIDGET (the_hint)->owner == NULL)
        return;

    if (!mc_global.message_visible)
    {
        label_set_text (the_hint, NULL);
        return;
    }

    hint = get_random_hint (force);

    if (hint != NULL)
    {
        if (*hint != '\0')
            set_hintbar (hint);
        g_free (hint);
    }
    else
    {
        char text[BUF_SMALL];

        g_snprintf (text, sizeof (text), _("GNU Midnight Commander %s\n"), mc_global.mc_version);
        set_hintbar (text);
    }
}

/* --------------------------------------------------------------------------------------------- */
/**
  * Change current panel in the file manager.
  *
  * @return current_panel
  */

WPanel *
change_panel (void)
{
    input_complete_free (cmdline);
    group_select_next_widget (GROUP (filemanager));
    return current_panel;
}

/* --------------------------------------------------------------------------------------------- */

/** Save current stat of directories to avoid reloading the panels
 * when no modifications have taken place
 */
void
save_cwds_stat (void)
{
    if (panels_options.fast_reload)
    {
        mc_stat (current_panel->cwd_vpath, &(current_panel->dir_stat));
        if (get_other_type () == view_listing)
            mc_stat (other_panel->cwd_vpath, &(other_panel->dir_stat));
    }
}

/* --------------------------------------------------------------------------------------------- */

gboolean
quiet_quit_cmd (void)
{
    print_last_revert = TRUE;
    return quit_cmd_internal (1);
}

/* --------------------------------------------------------------------------------------------- */

/** Run the main dialog that occupies the whole screen */
gboolean
do_nc (void)
{
    gboolean ret;

    edit_stack_init ();

    filemanager = dlg_create (FALSE, 0, 0, 1, 1, WPOS_FULLSCREEN, FALSE, dialog_colors,
                              midnight_callback, NULL, "[main]", NULL);

    /* Check if we were invoked as an editor or file viewer */
    if (mc_global.mc_run_mode != MC_RUN_FULL)
    {
        setup_dummy_mc ();
    }
    else
    {
        /* We only need the first idle event to show user menu after start */
        widget_idle (WIDGET (filemanager), TRUE);

        setup_mc ();
        mc_filehighlight = mc_fhl_new (TRUE);

        create_file_manager ();
        (void) dlg_run (filemanager);

        mc_fhl_free (&mc_filehighlight);

        ret = TRUE;

        /* widget_destroy destroys even current_panel->cwd_vpath, so we have to save a copy :) */
        if (mc_args__last_wd_file != NULL && vfs_current_is_local ())
            last_wd_string = g_strdup (vfs_path_as_str (current_panel->cwd_vpath));

        /* don't handle VFS timestamps for dirs opened in panels */
        mc_event_destroy (MCEVENT_GROUP_CORE, "vfs_timestamp");
    }

    /* Program end */
    mc_global.midnight_shutdown = TRUE;
    dialog_switch_shutdown ();
    done_mc ();
    widget_destroy (WIDGET (filemanager));
    current_panel = NULL;

    edit_stack_free ();

    if ((quit & SUBSHELL_EXIT) == 0)
        tty_clear_screen ();

    return ret;
}

/* --------------------------------------------------------------------------------------------- */
