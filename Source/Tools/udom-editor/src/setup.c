/*
   Setup loading/saving.

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

/** \file setup.c
 *  \brief Source: setup loading/saving
 */

#include <config.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "lib/global.h"

#include "lib/widget.h"
#include "lib/tty/tty.h"
#include "lib/tty/key.h"
#include "lib/mcconfig.h"       /* num_history_items_recorded */
#include "lib/fileloc.h"
#include "lib/timefmt.h"
#include "lib/util.h"

#if 0 // VW
#include "filemanager/dir.h"
#include "filemanager/filemanager.h"
#include "filemanager/tree.h"   /* xtree_mode */
#include "filemanager/hotlist.h"        /* load/save/done hotlist */
#include "filemanager/panelize.h"       /* load/save/done panelize */
#include "filemanager/layout.h"
#include "filemanager/cmd.h"
#endif

#include "args.h"
#include "execute.h"            /* pause_after_run */
#include "clipboard.h"

#ifdef USE_INTERNAL_EDIT
#include "src/editor/edit.h"
#endif

#include "setup.h"

/*** global variables ****************************************************************************/

char *global_profile_name;      /* mc.lib */

/* Only used at program boot */
gboolean boot_current_is_left = TRUE;

/* If on, default for "No" in delete operations */
gboolean safe_delete = FALSE;
/* If on, default for "No" in overwrite files */
gboolean safe_overwrite = FALSE;

/* Controls screen clearing before an exec */
gboolean clear_before_exec = TRUE;

/* Asks for confirmation before deleting a file */
gboolean confirm_delete = TRUE;
/* Asks for confirmation before deleting a hotlist entry */
gboolean confirm_directory_hotlist_delete = FALSE;
/* Asks for confirmation before overwriting a file */
gboolean confirm_overwrite = TRUE;
/* Asks for confirmation before executing a program by pressing enter */
gboolean confirm_execute = FALSE;
/* Asks for confirmation before leaving the program */
gboolean confirm_exit = FALSE;

/* If true, at startup the user-menu is invoked */
gboolean auto_menu = FALSE;
/* This flag indicates if the pull down menus by default drop down */
gboolean drop_menus = FALSE;

/* Asks for confirmation when using F3 to view a directory and there
   are tagged files */
gboolean confirm_view_dir = FALSE;

/* Ask file name before start the editor */
gboolean editor_ask_filename_before_edit = FALSE;

gboolean copymove_persistent_attr = TRUE;

/* Tab size */
int option_tab_spacing = DEFAULT_TAB_SPACING;

/* Ugly hack to allow panel_save_setup to work as a place holder for */
/* default panel values */
int saving_setup;

gboolean easy_patterns = TRUE;

/* It true saves the setup when quitting */
gboolean auto_save_setup = TRUE;

/* If true, then the +, - and \ keys have their special meaning only if the
 * command line is empty, otherwise they behave like regular letters
 */
gboolean only_leading_plus_minus = TRUE;

/* Automatically fills name with current selected item name on mkdir */
gboolean auto_fill_mkdir_name = TRUE;

/* If set and you don't have subshell support, then C-o will give you a shell */
gboolean output_starts_shell = FALSE;

/* If set, we execute the file command to check the file type */
gboolean use_file_to_check_type = TRUE;

gboolean verbose = TRUE;

/*
 * Whether the Midnight Commander tries to provide more
 * information about copy/move sizes and bytes transferred
 * at the expense of some speed
 */
gboolean file_op_compute_totals = TRUE;

/* If true use the internal viewer */
gboolean use_internal_view = TRUE;
/* If set, use the builtin editor */
gboolean use_internal_edit = TRUE;

/* Value of "other_dir" key in ini file */
char *saved_other_dir = NULL;

/* If set, then print to the given file the last directory we were at */
char *last_wd_string = NULL;

/* Set when main loop should be terminated */
int quit = 0;

/* Set to TRUE to suppress printing the last directory */
int print_last_revert = FALSE;

#ifdef USE_INTERNAL_EDIT
/* index to record_macro_buf[], -1 if not recording a macro */
int macro_index = -1;

/* macro stuff */
struct macro_action_t record_macro_buf[MAX_MACRO_LENGTH];

GArray *macros_list;
#endif /* USE_INTERNAL_EDIT */

/*** file scope macro definitions ****************************************************************/

/* In order to use everywhere the same setup for the locale we use defines */
#define FMTYEAR _("%b %e  %Y")
#define FMTTIME _("%b %e %H:%M")

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

static char *profile_name = NULL;       /* ${XDG_CONFIG_HOME}/mc/ini */

static const struct
{
    const char *opt_name;
    gboolean *opt_addr;
} bool_options [] = {
    { "verbose", &verbose },
    { "shell_patterns", &easy_patterns },
    { "auto_save_setup", &auto_save_setup },
    { "preallocate_space", &mc_global.vfs.preallocate_space },
    { "auto_menu", &auto_menu },
    { "use_internal_view", &use_internal_view },
    { "use_internal_edit", &use_internal_edit },
    { "clear_before_exec", &clear_before_exec },
    { "confirm_delete", &confirm_delete },
    { "confirm_overwrite", &confirm_overwrite },
    { "confirm_execute", &confirm_execute },
    { "confirm_history_cleanup", &mc_global.widget.confirm_history_cleanup },
    { "confirm_exit", &confirm_exit },
    { "confirm_directory_hotlist_delete", &confirm_directory_hotlist_delete },
    { "confirm_view_dir", &confirm_view_dir },
    { "safe_delete", &safe_delete },
    { "safe_overwrite", &safe_overwrite },
    { "use_8th_bit_as_meta", &use_8th_bit_as_meta },
    { "drop_menus", &drop_menus },
    { "old_esc_mode", &old_esc_mode },
    { "cd_symlinks", &mc_global.vfs.cd_symlinks },
    { "show_all_if_ambiguous", &mc_global.widget.show_all_if_ambiguous },
#ifdef USE_FILE_CMD
    { "use_file_to_guess_type", &use_file_to_check_type },
#endif
    { "alternate_plus_minus", &mc_global.tty.alternate_plus_minus },
    { "only_leading_plus_minus", &only_leading_plus_minus },
    { "show_output_starts_shell", &output_starts_shell },
    { "file_op_compute_totals", &file_op_compute_totals },
    { "classic_progressbar", &classic_progressbar },
#ifdef USE_INTERNAL_EDIT
    { "editor_fill_tabs_with_spaces", &option_fill_tabs_with_spaces },
    { "editor_return_does_auto_indent", &option_return_does_auto_indent },
    { "editor_backspace_through_tabs", &option_backspace_through_tabs },
    { "editor_fake_half_tabs", &option_fake_half_tabs },
    { "editor_option_save_position", &option_save_position },
    { "editor_option_auto_para_formatting", &option_auto_para_formatting },
    { "editor_option_typewriter_wrap", &option_typewriter_wrap },
    { "editor_edit_confirm_save", &edit_confirm_save },
    { "editor_syntax_highlighting", &option_syntax_highlighting },
    { "editor_persistent_selections", &option_persistent_selections },
    { "editor_drop_selection_on_copy", &option_drop_selection_on_copy },
    { "editor_cursor_beyond_eol", &option_cursor_beyond_eol },
    { "editor_cursor_after_inserted_block", &option_cursor_after_inserted_block },
    { "editor_visible_tabs", &visible_tabs },
    { "editor_visible_spaces", &visible_tws },
    { "editor_line_state", &option_line_state },
    { "editor_simple_statusbar", &simple_statusbar },
    { "editor_check_new_line", &option_check_nl_at_eof },
    { "editor_show_right_margin", &show_right_margin },
    { "editor_group_undo", &option_group_undo },
    { "editor_state_full_filename", &option_state_full_filename },
#endif /* USE_INTERNAL_EDIT */
    { "editor_ask_filename_before_edit", &editor_ask_filename_before_edit },
    { "shadows", &mc_global.tty.shadows },
    { "auto_fill_mkdir_name", &auto_fill_mkdir_name },
    { "copymove_persistent_attr", &copymove_persistent_attr },
    { NULL, NULL }
};

static const struct
{
    const char *opt_name;
    int *opt_addr;
} int_options [] = {
    { "pause_after_run", &pause_after_run },
    { "mouse_repeat_rate", &mou_auto_repeat },
    { "double_click_speed", &double_click_speed },
    { "old_esc_mode_timeout", &old_esc_mode_timeout },
    { "num_history_items_recorded", &num_history_items_recorded },
    /* option_tab_spacing is used in internal viewer */
    { "editor_tab_spacing", &option_tab_spacing },
#ifdef USE_INTERNAL_EDIT
    { "editor_word_wrap_line_length", &option_word_wrap_line_length },
    { "editor_option_save_mode", &option_save_mode },
#endif /* USE_INTERNAL_EDIT */
    { NULL, NULL }
};

static const struct
{
    const char *opt_name;
    char **opt_addr;
    const char *opt_defval;
} str_options[] = {
#ifdef USE_INTERNAL_EDIT
    { "editor_backup_extension", &option_backup_ext, "~" },
    { "editor_filesize_threshold", &option_filesize_threshold, "64M" },
    { "editor_stop_format_chars", &option_stop_format_chars, "-+*\\,.;:&>" },
#endif
    {  NULL, NULL, NULL }
};

/* --------------------------------------------------------------------------------------------- */
/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------------------------- */

static void
load_config (void)
{
    size_t i;
    const char *kt;

    /* Load boolean options */
    for (i = 0; bool_options[i].opt_name != NULL; i++)
        *bool_options[i].opt_addr =
            mc_config_get_bool (mc_global.main_config, CONFIG_APP_SECTION, bool_options[i].opt_name,
                                *bool_options[i].opt_addr);

    /* Load integer options */
    for (i = 0; int_options[i].opt_name != NULL; i++)
        *int_options[i].opt_addr =
            mc_config_get_int (mc_global.main_config, CONFIG_APP_SECTION, int_options[i].opt_name,
                               *int_options[i].opt_addr);

    /* Load string options */
    for (i = 0; str_options[i].opt_name != NULL; i++)
        *str_options[i].opt_addr =
            mc_config_get_string (mc_global.main_config, CONFIG_APP_SECTION,
                                  str_options[i].opt_name, str_options[i].opt_defval);

    /* Overwrite some options */
#ifdef USE_INTERNAL_EDIT
    if (option_word_wrap_line_length <= 0)
        option_word_wrap_line_length = DEFAULT_WRAP_LINE_LENGTH;
#else
    /* Reset forced in case of build without internal editor */
    use_internal_edit = FALSE;
#endif /* USE_INTERNAL_EDIT */

    if (option_tab_spacing <= 0)
        option_tab_spacing = DEFAULT_TAB_SPACING;

    kt = getenv ("KEYBOARD_KEY_TIMEOUT_US");
    if (kt != NULL && kt[0] != '\0')
        old_esc_mode_timeout = atoi (kt);
}

/* --------------------------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------------------------- */

static void
load_keys_from_section (const char *terminal, mc_config_t * cfg)
{
    char *section_name;
    gchar **profile_keys, **keys;
    char *valcopy, *value;
    long key_code;

    if (terminal == NULL)
        return;

    section_name = g_strconcat ("terminal:", terminal, (char *) NULL);
    keys = mc_config_get_keys (cfg, section_name, NULL);

    for (profile_keys = keys; *profile_keys != NULL; profile_keys++)
    {
        /* copy=other causes all keys from [terminal:other] to be loaded. */
        if (g_ascii_strcasecmp (*profile_keys, "copy") == 0)
        {
            valcopy = mc_config_get_string (cfg, section_name, *profile_keys, "");
            load_keys_from_section (valcopy, cfg);
            g_free (valcopy);
            continue;
        }

        key_code = lookup_key (*profile_keys, NULL);
        if (key_code != 0)
        {
            gchar **values;

            values = mc_config_get_string_list (cfg, section_name, *profile_keys, NULL);
            if (values != NULL)
            {
                gchar **curr_values;

                for (curr_values = values; *curr_values != NULL; curr_values++)
                {
                    valcopy = convert_controls (*curr_values);
                    define_sequence (key_code, valcopy, MCKEY_NOACTION);
                    g_free (valcopy);
                }

                g_strfreev (values);
            }
            else
            {
                value = mc_config_get_string (cfg, section_name, *profile_keys, "");
                valcopy = convert_controls (value);
                define_sequence (key_code, valcopy, MCKEY_NOACTION);
                g_free (valcopy);
                g_free (value);
            }
        }
    }
    g_strfreev (keys);
    g_free (section_name);
}

/* --------------------------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------------------------- */

static void
save_config (void)
{
    size_t i;

    /* Save boolean options */
    for (i = 0; bool_options[i].opt_name != NULL; i++)
        mc_config_set_bool (mc_global.main_config, CONFIG_APP_SECTION, bool_options[i].opt_name,
                            *bool_options[i].opt_addr);

    /* Save integer options */
    for (i = 0; int_options[i].opt_name != NULL; i++)
        mc_config_set_int (mc_global.main_config, CONFIG_APP_SECTION, int_options[i].opt_name,
                           *int_options[i].opt_addr);

    /* Save string options */
    for (i = 0; str_options[i].opt_name != NULL; i++)
        mc_config_set_string (mc_global.main_config, CONFIG_APP_SECTION, str_options[i].opt_name,
                              *str_options[i].opt_addr);
}

/* --------------------------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

const char *
setup_init (void)
{
    if (profile_name == NULL)
    {
        char *profile;

        profile = mc_config_get_full_path (MC_CONFIG_FILE);
        if (!exist_file (profile))
        {
            char *inifile;

            inifile = mc_build_filename (mc_global.sysconfig_dir, "mc.ini", (char *) NULL);
            if (exist_file (inifile))
            {
                g_free (profile);
                profile = inifile;
            }
            else
            {
                g_free (inifile);
                inifile = mc_build_filename (mc_global.share_data_dir, "mc.ini", (char *) NULL);
                if (!exist_file (inifile))
                    g_free (inifile);
                else
                {
                    g_free (profile);
                    profile = inifile;
                }
            }
        }

        profile_name = profile;
    }

    return profile_name;
}

/* --------------------------------------------------------------------------------------------- */

void
load_setup (void)
{
    const char *profile;

    profile = setup_init ();

    /* mc.lib is common for all users, but has priority lower than
       ${XDG_CONFIG_HOME}/mc/ini.  FIXME: it's only used for keys and treestore now */
    global_profile_name =
        g_build_filename (mc_global.sysconfig_dir, MC_GLOBAL_CONFIG_FILE, (char *) NULL);
    if (!exist_file (global_profile_name))
    {
        g_free (global_profile_name);
        global_profile_name =
            g_build_filename (mc_global.share_data_dir, MC_GLOBAL_CONFIG_FILE, (char *) NULL);
    }

    mc_global.main_config = mc_config_init (profile, FALSE);

    load_config ();

    /* Load time formats */
    user_recent_timeformat =
        mc_config_get_string (mc_global.main_config, CONFIG_MISC_SECTION, "timeformat_recent",
                              FMTTIME);
    user_old_timeformat =
        mc_config_get_string (mc_global.main_config, CONFIG_MISC_SECTION, "timeformat_old",
                              FMTYEAR);

    /* The default color and the terminal dependent color */
    mc_global.tty.setup_color_string =
        mc_config_get_string (mc_global.main_config, "Colors", "base_color", "");
    mc_global.tty.term_color_string =
        mc_config_get_string (mc_global.main_config, "Colors", getenv ("TERM"), "");
    mc_global.tty.color_terminal_string =
        mc_config_get_string (mc_global.main_config, "Colors", "color_terminals", "");

    /* Load the directory history */
    /*    directory_history_load (); */
    /* Remove the temporal entries */

    clipboard_store_path =
        mc_config_get_string (mc_global.main_config, CONFIG_MISC_SECTION, "clipboard_store", "");
    clipboard_paste_path =
        mc_config_get_string (mc_global.main_config, CONFIG_MISC_SECTION, "clipboard_paste", "");
}

/* --------------------------------------------------------------------------------------------- */

gboolean
save_setup (gboolean save_options, gboolean save_panel_options)
{
    gboolean ret = TRUE;

    (void)save_panel_options;

    saving_setup = 1;

    if (save_options)
    {
        char *tmp_profile;

        save_config ();
        /* directory_history_save (); */

        mc_config_set_string (mc_global.main_config, CONFIG_MISC_SECTION, "clipboard_store",
                              clipboard_store_path);
        mc_config_set_string (mc_global.main_config, CONFIG_MISC_SECTION, "clipboard_paste",
                              clipboard_paste_path);

        tmp_profile = mc_config_get_full_path (MC_CONFIG_FILE);
        ret = mc_config_save_to_file (mc_global.main_config, tmp_profile, NULL);
        g_free (tmp_profile);
    }

    saving_setup = 0;

    return ret;
}

/* --------------------------------------------------------------------------------------------- */

void
done_setup (void)
{
    size_t i;

    g_free (clipboard_store_path);
    g_free (clipboard_paste_path);
    g_free (global_profile_name);
    g_free (mc_global.tty.color_terminal_string);
    g_free (mc_global.tty.term_color_string);
    g_free (mc_global.tty.setup_color_string);
    g_free (profile_name);
    mc_config_deinit (mc_global.main_config);

    g_free (user_recent_timeformat);
    g_free (user_old_timeformat);

    for (i = 0; str_options[i].opt_name != NULL; i++)
        g_free (*str_options[i].opt_addr);
}


/* --------------------------------------------------------------------------------------------- */

void
setup_save_config_show_error (const char *filename, GError ** mcerror)
{
    if (mcerror != NULL && *mcerror != NULL)
    {
        message (D_ERROR, MSG_ERROR, _("Cannot save file %s:\n%s"), filename, (*mcerror)->message);
        g_error_free (*mcerror);
        *mcerror = NULL;
    }
}

/* --------------------------------------------------------------------------------------------- */

void
load_key_defs (void)
{
    /*
     * Load keys from mc.lib before ${XDG_CONFIG_HOME}/mc/ini, so that the user
     * definitions override global settings.
     */
    mc_config_t *mc_global_config;

    mc_global_config = mc_config_init (global_profile_name, FALSE);
    if (mc_global_config != NULL)
    {
        load_keys_from_section ("general", mc_global_config);
        load_keys_from_section (getenv ("TERM"), mc_global_config);
        mc_config_deinit (mc_global_config);
    }

    load_keys_from_section ("general", mc_global.main_config);
    load_keys_from_section (getenv ("TERM"), mc_global.main_config);
}

/* --------------------------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------------------------- */
