/*
   Execution routines for GNU Midnight Commander

   Copyright (C) 2003-2021
   Free Software Foundation, Inc.

   Written by:
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

/** \file  execute.c
 *  \brief Source: execution routines
 */

#include "ue-config.h"

#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "lib/global.h"

#include "lib/tty/tty.h"
#include "lib/tty/key.h"
#include "lib/tty/win.h"
#include "lib/vfs/vfs.h"
#include "lib/mcconfig.h"
#include "lib/util.h"
#include "lib/strutil.h"        /* str_replace_all_substrings() */
#include "lib/widget.h"

#include "consaver/cons.saver.h"
#include "subshell/subshell.h"
#include "setup.h"              /* clear_before_exec */

#include "execute.h"
#include "workingdir.h"

/*** global variables ****************************************************************************/

int pause_after_run = pause_on_dumb_terminals;

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/*** file scope functions ************************************************************************/

void do_execute (const char *shell, const char *command, int flags);
void do_executev (const char *shell, int flags, char *const argv[]);
char *execute_get_external_cmd_opts_from_config (const char *command,
                                                 const vfs_path_t * filename_vpath,
                                                 long start_line);

/* --------------------------------------------------------------------------------------------- */

static void
edition_post_exec (void)
{
    tty_enter_ca_mode ();

    /* FIXME: Missing on slang endwin? */
    tty_reset_prog_mode ();
    tty_flush_input ();

    tty_keypad (TRUE);
    tty_raw_mode ();
    channels_up ();
    enable_mouse ();
    enable_bracketed_paste ();
    if (mc_global.tty.alternate_plus_minus)
        application_keypad_mode ();
}

/* --------------------------------------------------------------------------------------------- */

static void
edition_pre_exec (void)
{
    if (clear_before_exec)
        tty_clear_screen ();
    else
    {
        if (!(mc_global.tty.console_flag != '\0' || mc_global.tty.xterm_flag))
            printf ("\n\n");
    }

    channels_down ();
    disable_mouse ();
    disable_bracketed_paste ();

    tty_reset_shell_mode ();
    tty_keypad (FALSE);
    tty_reset_screen ();

    numeric_keypad_mode ();

    /* on xterms: maybe endwin did not leave the terminal on the shell
     * screen page: do it now.
     *
     * Do not move this before endwin: in some systems rmcup includes
     * a call to clear screen, so it will end up clearing the shell screen.
     */
    tty_exit_ca_mode ();
}

/* --------------------------------------------------------------------------------------------- */

static void
do_possible_cd (const vfs_path_t * new_dir_vpath)
{
    if (!workingdir_cd (new_dir_vpath, cd_exact))
        message (D_ERROR, _("Warning"), "%s",
                 _("The Commander can't change to the directory that\n"
                   "the subshell claims you are in. Perhaps you have\n"
                   "deleted your working directory, or given yourself\n"
                   "extra access permissions with the \"su\" command?"));
}

/* --------------------------------------------------------------------------------------------- */

static void
do_suspend_cmd (void)
{
    pre_exec ();

    if (mc_global.tty.console_flag != '\0' && !mc_global.tty.use_subshell)
        handle_console (CONSOLE_RESTORE);

#ifdef SIGTSTP
    {
        struct sigaction sigtstp_action;

        memset (&sigtstp_action, 0, sizeof (sigtstp_action));
        /* Make sure that the SIGTSTP below will suspend us directly,
           without calling ncurses' SIGTSTP handler; we *don't* want
           ncurses to redraw the screen immediately after the SIGCONT */
        sigaction (SIGTSTP, &startup_handler, &sigtstp_action);

        kill (getpid (), SIGTSTP);

        /* Restore previous SIGTSTP action */
        sigaction (SIGTSTP, &sigtstp_action, NULL);
    }
#endif /* SIGTSTP */

    if (mc_global.tty.console_flag != '\0' && !mc_global.tty.use_subshell)
        handle_console (CONSOLE_SAVE);

    edition_post_exec ();
}

/* --------------------------------------------------------------------------------------------- */

static gboolean
execute_prepare_with_vfs_arg (const vfs_path_t * filename_vpath, vfs_path_t ** localcopy_vpath,
                              time_t * mtime)
{
    struct stat st;

    /* Simplest case, this file is local */
    if ((filename_vpath == NULL && vfs_file_is_local (vfs_get_raw_current_dir ()))
        || vfs_file_is_local (filename_vpath))
        return TRUE;

    /* FIXME: Creation of new files on VFS is not supported */
    if (filename_vpath == NULL)
        return FALSE;

    *localcopy_vpath = mc_getlocalcopy (filename_vpath);
    if (*localcopy_vpath == NULL)
    {
        message (D_ERROR, MSG_ERROR, _("Cannot fetch a local copy of %s"),
                 vfs_path_as_str (filename_vpath));
        return FALSE;
    }

    mc_stat (*localcopy_vpath, &st);
    *mtime = st.st_mtime;
    return TRUE;
}

/* --------------------------------------------------------------------------------------------- */

static void
execute_cleanup_with_vfs_arg (const vfs_path_t * filename_vpath, vfs_path_t ** localcopy_vpath,
                              time_t * mtime)
{
    if (*localcopy_vpath != NULL)
    {
        struct stat st;

        /*
         * filename can be an entry on panel, it can be changed by executing
         * the command, so make a copy.  Smarter VFS code would make the code
         * below unnecessary.
         */
        mc_stat (*localcopy_vpath, &st);
        mc_ungetlocalcopy (filename_vpath, *localcopy_vpath, *mtime != st.st_mtime);
        vfs_path_free (*localcopy_vpath, TRUE);
        *localcopy_vpath = NULL;
    }
}

/* --------------------------------------------------------------------------------------------- */

static char *
execute_get_opts_from_cfg (const char *command, const char *default_str)
{
    char *str_from_config;

    str_from_config =
        mc_config_get_string_raw (mc_global.main_config, CONFIG_EXT_EDITOR_VIEWER_SECTION, command,
                                  NULL);

    if (str_from_config == NULL)
    {
        mc_config_t *cfg;

        cfg = mc_config_init (global_profile_name, TRUE);
        if (cfg == NULL)
            return g_strdup (default_str);

        str_from_config =
            mc_config_get_string_raw (cfg, CONFIG_EXT_EDITOR_VIEWER_SECTION, command, default_str);

        mc_config_deinit (cfg);
    }

    return str_from_config;
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

char *
execute_get_external_cmd_opts_from_config (const char *command, const vfs_path_t * filename_vpath,
                                           long start_line)
{
    char *str_from_config, *return_str;
    char *parameter;

    if (filename_vpath == NULL)
        return g_strdup ("");

    parameter = g_shell_quote (vfs_path_get_last_path_str (filename_vpath));

    if (start_line <= 0)
        return parameter;

    str_from_config = execute_get_opts_from_cfg (command, "%filename");

    return_str = str_replace_all (str_from_config, "%filename", parameter);
    g_free (parameter);
    g_free (str_from_config);
    str_from_config = return_str;

    parameter = g_strdup_printf ("%ld", start_line);
    return_str = str_replace_all (str_from_config, "%lineno", parameter);
    g_free (parameter);
    g_free (str_from_config);

    return return_str;
}

/* --------------------------------------------------------------------------------------------- */

void
do_executev (const char *shell, int flags, char *const argv[])
{
    vfs_path_t *new_dir_vpath = NULL;

    vfs_path_t *old_vfs_dir_vpath = NULL;

    if (!vfs_current_is_local ())
        old_vfs_dir_vpath = vfs_path_clone (vfs_get_raw_current_dir ());

#if 0 // VW
    if (mc_global.mc_run_mode == MC_RUN_FULL)
        save_cwds_stat ();
#endif
    pre_exec ();
    if (mc_global.tty.console_flag != '\0')
        handle_console (CONSOLE_RESTORE);

    if (!mc_global.tty.use_subshell && *argv != NULL && (flags & EXECUTE_INTERNAL) == 0)
    {
        printf ("%s%s\n", mc_prompt, *argv);
        fflush (stdout);
    }
    if (mc_global.tty.use_subshell && (flags & EXECUTE_INTERNAL) == 0)
    {
        do_update_prompt ();

        /* We don't care if it died, higher level takes care of this */
        invoke_subshell (*argv, VISIBLY, old_vfs_dir_vpath != NULL ? NULL : &new_dir_vpath);
    }
    else
        my_systemv_flags (flags, shell, argv);

    if ((flags & EXECUTE_INTERNAL) == 0)
    {
        if ((pause_after_run == pause_always
             || (pause_after_run == pause_on_dumb_terminals && !mc_global.tty.xterm_flag
                 && mc_global.tty.console_flag == '\0')) && quit == 0
            && subshell_state != RUNNING_COMMAND
            )
        {
            printf ("%s", _("Press any key to continue..."));
            fflush (stdout);
            tty_raw_mode ();
            get_key_code (0);
            printf ("\r\n");
            fflush (stdout);
        }
        if (mc_global.tty.console_flag != '\0' && output_lines != 0 && mc_global.keybar_visible)
        {
            putchar ('\n');
            fflush (stdout);
        }
    }

    if (mc_global.tty.console_flag != '\0')
        handle_console (CONSOLE_SAVE);
    edition_post_exec ();

    if (new_dir_vpath != NULL)
    {
        do_possible_cd (new_dir_vpath);
        vfs_path_free (new_dir_vpath, TRUE);
    }

    if (old_vfs_dir_vpath != NULL)
    {
        mc_chdir (old_vfs_dir_vpath);
        vfs_path_free (old_vfs_dir_vpath, TRUE);
    }

    if (mc_global.mc_run_mode == MC_RUN_FULL)
    {
#if 0 // VW
        update_panels (UP_OPTIMIZE, UP_KEEPSEL);
#endif
        update_xterm_title_path ();
    }

    do_refresh ();
    use_dash (TRUE);
}

/* --------------------------------------------------------------------------------------------- */

void
do_execute (const char *shell, const char *command, int flags)
{
    GPtrArray *args_array;

    args_array = g_ptr_array_new ();
    g_ptr_array_add (args_array, (char *) command);
    g_ptr_array_add (args_array, NULL);

    do_executev (shell, flags, (char *const *) args_array->pdata);

    g_ptr_array_free (args_array, TRUE);
}

/* --------------------------------------------------------------------------------------------- */

/** Set up the terminal before executing a program */

void
pre_exec (void)
{
    use_dash (FALSE);
    edition_pre_exec ();
}

/* --------------------------------------------------------------------------------------------- */
/** Hide the terminal after executing a program */
void
post_exec (void)
{
    edition_post_exec ();
    use_dash (TRUE);
    repaint_screen ();
}

/* --------------------------------------------------------------------------------------------- */
/* Executes a command */

void
shell_execute (const char *command, int flags)
{
    char *cmd = NULL;

    if (flags & EXECUTE_HIDE)
    {
        cmd = g_strconcat (" ", command, (char *) NULL);
        flags ^= EXECUTE_HIDE;
    }

    if (mc_global.tty.use_subshell)
    {
        if (subshell_state == INACTIVE)
            do_execute (mc_global.shell->path, cmd ? cmd : command, flags | EXECUTE_AS_SHELL);
        else
            message (D_ERROR, MSG_ERROR, "%s", _("The shell is already running a command"));
    }
    else
        do_execute (mc_global.shell->path, cmd ? cmd : command, flags | EXECUTE_AS_SHELL);

    g_free (cmd);
}

/* --------------------------------------------------------------------------------------------- */

void
toggle_subshell (void)
{
    static gboolean message_flag = TRUE;

    vfs_path_t *new_dir_vpath = NULL;

    SIG_ATOMIC_VOLATILE_T was_sigwinch = 0;

    if (!(mc_global.tty.xterm_flag || mc_global.tty.console_flag != '\0'
          || mc_global.tty.use_subshell || output_starts_shell))
    {
        if (message_flag)
            message (D_ERROR, MSG_ERROR,
                     _("Not an xterm or Linux console;\nthe subshell cannot be toggled."));
        message_flag = FALSE;
        return;
    }

    channels_down ();
    disable_mouse ();
    disable_bracketed_paste ();
    if (clear_before_exec)
        tty_clear_screen ();
    if (mc_global.tty.alternate_plus_minus)
        numeric_keypad_mode ();
#ifndef HAVE_SLANG
    /* With slang we don't want any of this, since there
     * is no raw_mode supported
     */
    tty_reset_shell_mode ();
#endif /* !HAVE_SLANG */
    tty_noecho ();
    tty_keypad (FALSE);
    tty_reset_screen ();
    tty_exit_ca_mode ();
    tty_raw_mode ();
    if (mc_global.tty.console_flag != '\0')
        handle_console (CONSOLE_RESTORE);

    if (mc_global.tty.use_subshell)
    {
        vfs_path_t **new_dir_p;

        new_dir_p = vfs_current_is_local ()? &new_dir_vpath : NULL;
        invoke_subshell (NULL, VISIBLY, new_dir_p);
    }
    else
    {
        if (output_starts_shell)
        {
            fputs (_("Type 'exit' to return to the Midnight Commander"), stderr);
            fputs ("\n\r\n\r", stderr);

            my_system (EXECUTE_INTERNAL, mc_global.shell->path, NULL);
        }
        else
            get_key_code (0);
    }

    if (mc_global.tty.console_flag != '\0')
        handle_console (CONSOLE_SAVE);

    tty_enter_ca_mode ();

    tty_reset_prog_mode ();
    tty_keypad (TRUE);

    /* Prevent screen flash when user did 'exit' or 'logout' within
       subshell */
    if ((quit & SUBSHELL_EXIT) != 0)
    {
        /* User did 'exit' or 'logout': quit MC */
        if (quiet_quit_cmd ())
            return;

        quit = 0;
        /* restart subshell */
        if (mc_global.tty.use_subshell)
            init_subshell ();
    }

    enable_mouse ();
    enable_bracketed_paste ();
    channels_up ();
    if (mc_global.tty.alternate_plus_minus)
        application_keypad_mode ();

    /* HACK:
     * Save sigwinch flag that will be reset in mc_refresh() called via update_panels().
     * There is some problem with screen redraw in ncurses-based mc in this situation.
     */
    was_sigwinch = tty_got_winch ();
    tty_flush_winch ();

    if (mc_global.tty.use_subshell)
    {
        if (mc_global.mc_run_mode == MC_RUN_FULL)
        {
            if (new_dir_vpath != NULL)
                do_possible_cd (new_dir_vpath);
        }
        else if (new_dir_vpath != NULL && mc_chdir (new_dir_vpath) != -1)
            vfs_setup_cwd ();
    }

    vfs_path_free (new_dir_vpath, TRUE);

    if (mc_global.mc_run_mode == MC_RUN_FULL)
    {
#if 0 // VW
        update_panels (UP_OPTIMIZE, UP_KEEPSEL);
#endif
        update_xterm_title_path ();
    }

    if (was_sigwinch != 0 || tty_got_winch ())
        dialog_change_screen_size ();
    else
        repaint_screen ();
}

/* --------------------------------------------------------------------------------------------- */

/* event callback */
gboolean
execute_suspend (const gchar * event_group_name, const gchar * event_name,
                 gpointer init_data, gpointer data)
{
    (void) event_group_name;
    (void) event_name;
    (void) init_data;
    (void) data;

#if 0 // VW
    if (mc_global.mc_run_mode == MC_RUN_FULL)
        save_cwds_stat ();
#endif
    do_suspend_cmd ();
#if 0 // VW
    if (mc_global.mc_run_mode == MC_RUN_FULL)
        update_panels (UP_OPTIMIZE, UP_KEEPSEL);
#endif
    do_refresh ();

    return TRUE;
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Execute command on a filename that can be on VFS.
 * Errors are reported to the user.
 */

void
execute_with_vfs_arg (const char *command, const vfs_path_t * filename_vpath)
{
    vfs_path_t *localcopy_vpath = NULL;
    const vfs_path_t *do_execute_vpath;
    time_t mtime;

    if (!execute_prepare_with_vfs_arg (filename_vpath, &localcopy_vpath, &mtime))
        return;

    do_execute_vpath = (localcopy_vpath == NULL) ? filename_vpath : localcopy_vpath;

    do_execute (command, vfs_path_get_last_path_str (do_execute_vpath), EXECUTE_INTERNAL);

    execute_cleanup_with_vfs_arg (filename_vpath, &localcopy_vpath, &mtime);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Execute external editor or viewer.
 *
 * @param command editor/viewer to run
 * @param filename_vpath path for edit/view
 * @param start_line cursor will be placed at the 'start_line' position after opening file
 *        if start_line is 0 or negative, no start line will be passed to editor/viewer
 */

void
execute_external_editor_or_viewer (const char *command, const vfs_path_t * filename_vpath,
                                   long start_line)
{
    vfs_path_t *localcopy_vpath = NULL;
    const vfs_path_t *do_execute_vpath;
    char *extern_cmd_options;
    time_t mtime = 0;

    if (!execute_prepare_with_vfs_arg (filename_vpath, &localcopy_vpath, &mtime))
        return;

    do_execute_vpath = (localcopy_vpath == NULL) ? filename_vpath : localcopy_vpath;

    extern_cmd_options =
        execute_get_external_cmd_opts_from_config (command, do_execute_vpath, start_line);

    if (extern_cmd_options != NULL)
    {
        char **argv_cmd_options;
        int argv_count;

        if (g_shell_parse_argv (extern_cmd_options, &argv_count, &argv_cmd_options, NULL))
        {
            do_executev (command, EXECUTE_INTERNAL, argv_cmd_options);
            g_strfreev (argv_cmd_options);
        }
        else
            do_executev (command, EXECUTE_INTERNAL, NULL);

        g_free (extern_cmd_options);
    }

    execute_cleanup_with_vfs_arg (filename_vpath, &localcopy_vpath, &mtime);
}

/* --------------------------------------------------------------------------------------------- */
