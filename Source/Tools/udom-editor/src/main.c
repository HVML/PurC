/*
   Main program for the Midnight Commander

   Copyright (C) 1994-2021
   Free Software Foundation, Inc.

   Written by:
   Miguel de Icaza, 1994, 1995, 1996, 1997
   Janne Kukonlehto, 1994, 1995
   Norbert Warmuth, 1997

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

/** \file main.c
 *  \brief Source: this is a main module
 */

#include "ue-config.h"
#include "ue-features.h"

#include <ctype.h>
#include <errno.h>
#include <locale.h>
#include <pwd.h>                /* for username in xterm title */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>             /* getsid() */

#include "lib/global.h"

#include "lib/widget.h"
#include "lib/event.h"
#include "lib/tty/tty.h"
#include "lib/tty/key.h"        /* For init_key() */
#include "lib/tty/mouse.h"      /* init_mouse() */
#include "lib/skin.h"
#include "lib/filehighlight.h"
#include "lib/fileloc.h"
#include "lib/strutil.h"
#include "lib/util.h"
#include "lib/vfs/vfs.h"        /* vfs_init(), vfs_shut() */

#include "workingdir.h"

#include "events_init.h"
#include "args.h"
#include "subshell/subshell.h"
#include "keymap.h"
#include "setup.h"              /* load_setup() */

#include "consaver/cons.saver.h"        /* cons_saver_pid */

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

static void
check_codeset (void)
{
    const char *current_system_codepage = NULL;

    current_system_codepage = str_detect_termencoding ();

    mc_global.utf8_display = str_isutf8 (current_system_codepage);
}

/* --------------------------------------------------------------------------------------------- */
/** POSIX version.  The only version we support.  */

static void
OS_Setup (void)
{
    const char *datadir_env;

    mc_shell_init ();

    /* This is the directory, where MC was installed, on Unix this is DATADIR */
    /* and can be overriden by the MC_DATADIR environment variable */
    datadir_env = g_getenv ("MC_DATADIR");
    if (datadir_env != NULL)
        mc_global.sysconfig_dir = g_strdup (datadir_env);
    else
        mc_global.sysconfig_dir = g_strdup (SYSCONFDIR);

    mc_global.share_data_dir = g_strdup (DATADIR);
}

/* --------------------------------------------------------------------------------------------- */

static void
sigchld_handler_no_subshell (int sig)
{
#ifdef __linux__
    int pid, status;

    if (mc_global.tty.console_flag == '\0')
        return;

    /* COMMENT: if it were true that after the call to handle_console(..INIT)
       the value of mc_global.tty.console_flag never changed, we could simply not install
       this handler at all if (!mc_global.tty.console_flag && !mc_global.tty.use_subshell). */

    /* That comment is no longer true.  We need to wait() on a sigchld
       handler (that's at least what the tarfs code expects currently). */

    pid = waitpid (cons_saver_pid, &status, WUNTRACED | WNOHANG);

    if (pid == cons_saver_pid)
    {
        if (WIFSTOPPED (status))
        {
            /* Someone has stopped cons.saver - restart it */
            kill (pid, SIGCONT);
        }
        else
        {
            /* cons.saver has died - disable console saving */
            handle_console (CONSOLE_DONE);
            mc_global.tty.console_flag = '\0';
        }
    }
    /* If we got here, some other child exited; ignore it */
#endif /* __linux__ */

    (void) sig;
}

/* --------------------------------------------------------------------------------------------- */

static void
init_sigchld (void)
{
    struct sigaction sigchld_action;

    memset (&sigchld_action, 0, sizeof (sigchld_action));
    sigchld_action.sa_handler =
        mc_global.tty.use_subshell ? sigchld_handler :
        sigchld_handler_no_subshell;

    sigemptyset (&sigchld_action.sa_mask);

#ifdef SA_RESTART
    sigchld_action.sa_flags = SA_RESTART;
#endif /* !SA_RESTART */

    if (sigaction (SIGCHLD, &sigchld_action, NULL) == -1)
    {
        /*
         * This may happen on QNX Neutrino 6, where SA_RESTART
         * is defined but not implemented.  Fallback to no subshell.
         */
        mc_global.tty.use_subshell = FALSE;
    }
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Check MC_SID to prevent running one mc from another.
 *
 * @return TRUE if no parent mc in our session was found, FALSE otherwise.
 */

static gboolean
check_sid (void)
{
    pid_t my_sid, old_sid;
    const char *sid_str;

    sid_str = getenv ("MC_SID");
    if (sid_str == NULL)
        return TRUE;

    old_sid = (pid_t) strtol (sid_str, NULL, 0);
    if (old_sid == 0)
        return TRUE;

    my_sid = getsid (0);
    if (my_sid == -1)
        return TRUE;

    /* The parent mc is in a different session, it's OK */
    return (old_sid != my_sid);
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

int
main (int argc, char *argv[])
{
    GError *mcerror = NULL;
    int exit_code = EXIT_FAILURE;

    mc_global.run_from_parent_mc = !check_sid ();

    /* We had LC_CTYPE before, LC_ALL includs LC_TYPE as well */
#ifdef HAVE_SETLOCALE
    (void) setlocale (LC_ALL, "");
#endif
    (void) bindtextdomain (PACKAGE, LOCALEDIR);
    (void) textdomain (PACKAGE);

    /* do this before args parsing */
    str_init_strings (NULL);

    mc_setup_run_mode (argv);   /* are we mc? editor? viewer? etc... */

    if (!mc_args_parse (&argc, &argv, "mc", &mcerror))
    {
      startup_exit_falure:
        fprintf (stderr, _("Failed to run:\n%s\n"), mcerror->message);
        g_error_free (mcerror);
      startup_exit_ok:
        mc_shell_deinit ();
        str_uninit_strings ();
        return exit_code;
    }

    /* do this before mc_args_show_info () to view paths in the --datadir-info output */
    OS_Setup ();

    if (!g_path_is_absolute (mc_config_get_home_dir ()))
    {
        mc_propagate_error (&mcerror, 0, "%s: %s", _("Home directory path is not absolute"),
                            mc_config_get_home_dir ());
        mc_event_deinit (NULL);
        goto startup_exit_falure;
    }

    if (!mc_args_show_info ())
    {
        exit_code = EXIT_SUCCESS;
        goto startup_exit_ok;
    }

    if (!events_init (&mcerror))
        goto startup_exit_falure;

    mc_config_init_config_paths (&mcerror);
    if (mcerror != NULL)
    {
        mc_event_deinit (NULL);
        goto startup_exit_falure;
    }

    vfs_init ();
#if 0 // VW
    vfs_plugins_init ();
#endif

    load_setup ();

    /* Must be done after load_setup because depends on mc_global.vfs.cd_symlinks */
    vfs_setup_work_dir ();

    /* Set up temporary directory after VFS initialization */
    mc_tmpdir ();

    /* do this after vfs initialization and vfs working directory setup
       due to mc_setctl() and mcedit_arg_vpath_new() calls in mc_setup_by_args() */
    if (!mc_setup_by_args (argc, argv, &mcerror))
    {
        vfs_shut ();
        done_setup ();
        g_free (saved_other_dir);
        mc_event_deinit (NULL);
        goto startup_exit_falure;
    }

    /* Resolve the other_dir panel option.
     * 1. Must be done after vfs_setup_work_dir().
     * 2. Must be done after mc_setup_by_args() because of mc_run_mode.
     */
    if (mc_global.mc_run_mode == MC_RUN_FULL)
    {
        char *buffer;
        vfs_path_t *vpath;

        buffer = mc_config_get_string (mc_global.panels_config, "Dirs", "other_dir", ".");
        vpath = vfs_path_from_str (buffer);
        if (vfs_file_is_local (vpath))
            saved_other_dir = buffer;
        else
            g_free (buffer);
        vfs_path_free (vpath, TRUE);
    }

    /* check terminal type
     * $TERM must be set and not empty
     * mc_global.tty.xterm_flag is used in init_key() and tty_init()
     * Do this after mc_args_handle() where mc_args__force_xterm is set up.
     */
    mc_global.tty.xterm_flag = tty_check_term (mc_args__force_xterm);

    /* NOTE: This has to be called before tty_init or whatever routine
       calls any define_sequence */
    init_key ();

    /* Must be done before installing the SIGCHLD handler [[FIXME]] */
    handle_console (CONSOLE_INIT);

    /* Disallow subshell when invoked as standalone viewer or editor from running mc */
    if (mc_global.mc_run_mode != MC_RUN_FULL && mc_global.run_from_parent_mc)
        mc_global.tty.use_subshell = FALSE;

    if (mc_global.tty.use_subshell)
        subshell_get_console_attributes ();

    /* Install the SIGCHLD handler; must be done before init_subshell() */
    init_sigchld ();

    /* We need this, since ncurses endwin () doesn't restore the signals */
    save_stop_handler ();

    /* Must be done before init_subshell, to set up the terminal size: */
    /* FIXME: Should be removed and LINES and COLS computed on subshell */
    tty_init (!mc_args__nomouse, mc_global.tty.xterm_flag);

    /* start check mc_global.display_codepage and mc_global.source_codepage */
    check_codeset ();

    /* Removing this from the X code let's us type C-c */
    load_key_defs ();

    keymap_load (!mc_args__nokeymap);

#ifdef USE_INTERNAL_EDIT
    macros_list = g_array_new (TRUE, FALSE, sizeof (macros_t));
#endif /* USE_INTERNAL_EDIT */

    tty_init_colors (mc_global.tty.disable_colors, mc_args__force_colors);

    mc_skin_init (NULL, &mcerror);
#if 0 // VW
    dlg_set_default_colors ();
    input_set_default_colors ();
    if (mc_global.mc_run_mode == MC_RUN_FULL)
        command_set_default_colors ();
#endif

    mc_error_message (&mcerror, NULL);

    /* Done here to ensure that the subshell doesn't  */
    /* inherit the file descriptors opened below, etc */
    if (mc_global.tty.use_subshell && mc_global.run_from_parent_mc)
    {
        int r;

        r = query_dialog (_("Warning"),
                          _("GNU Midnight Commander\nis already running on this terminal.\n"
                            "Subshell support will be disabled."),
                          D_ERROR, 2, _("&OK"), _("&Quit"));
        if (r == 0)
        {
            /* parent mc was found and the user wants to continue */
            ;
        }
        else
        {
            /* parent mc was found and the user wants to quit mc */
            mc_global.midnight_shutdown = TRUE;
        }

        mc_global.tty.use_subshell = FALSE;
    }

    if (mc_global.tty.use_subshell)
        init_subshell ();

    if (!mc_global.midnight_shutdown)
    {
        /* Also done after init_subshell, to save any shell init file messages */
        if (mc_global.tty.console_flag != '\0')
            handle_console (CONSOLE_SAVE);

        if (mc_global.tty.alternate_plus_minus)
            application_keypad_mode ();

        /* Done after subshell initialization to allow select and paste text by mouse
           w/o Shift button in subshell in the native console */
        init_mouse ();

        /* Done after tty_enter_ca_mode (tty_init) because in VTE bracketed mode is
           separate for the normal and alternate screens */
        enable_bracketed_paste ();

        /* subshell_prompt is NULL here */
        mc_prompt = (geteuid () == 0) ? "# " : "$ ";
    }

    /* Program main loop */
    if (mc_global.midnight_shutdown)
        exit_code = EXIT_SUCCESS;
    else
        exit_code = do_nc ()? EXIT_SUCCESS : EXIT_FAILURE;

    disable_bracketed_paste ();

    disable_mouse ();

#if 0 // VW
    /* Save the tree store */
    (void) tree_store_save ();
#endif

    keymap_free ();

    /* Virtual File System shutdown */
    vfs_shut ();

#if 0 // VW
    flush_extension_file ();    /* does only free memory */
#endif

    mc_skin_deinit ();
    tty_colors_done ();

    tty_shutdown ();

    done_setup ();

    if (mc_global.tty.console_flag != '\0' && (quit & SUBSHELL_EXIT) == 0)
        handle_console (CONSOLE_RESTORE);
    if (mc_global.tty.alternate_plus_minus)
        numeric_keypad_mode ();

    (void) signal (SIGCHLD, SIG_DFL);   /* Disable the SIGCHLD handler */

    if (mc_global.tty.console_flag != '\0')
        handle_console (CONSOLE_DONE);

    if (mc_global.mc_run_mode == MC_RUN_FULL && mc_args__last_wd_file != NULL
        && last_wd_string != NULL && !print_last_revert)
    {
        int last_wd_fd;

        last_wd_fd = open (mc_args__last_wd_file, O_WRONLY | O_CREAT | O_TRUNC | O_EXCL,
                           S_IRUSR | S_IWUSR);
        if (last_wd_fd != -1)
        {
            ssize_t ret1;
            int ret2;
            ret1 = write (last_wd_fd, last_wd_string, strlen (last_wd_string));
            ret2 = close (last_wd_fd);
            (void) ret1;
            (void) ret2;
        }
    }
    g_free (last_wd_string);

    mc_shell_deinit ();

    done_key ();

#ifdef USE_INTERNAL_EDIT
    if (macros_list != NULL)
    {
        guint i;

        for (i = 0; i < macros_list->len; i++)
        {
            macros_t *macros;

            macros = &g_array_index (macros_list, struct macros_t, i);
            if (macros != NULL && macros->macro != NULL)
                (void) g_array_free (macros->macro, TRUE);
        }
        (void) g_array_free (macros_list, TRUE);
    }
#endif /* USE_INTERNAL_EDIT */

    str_uninit_strings ();

    if (mc_global.mc_run_mode != MC_RUN_EDITOR)
        g_free (mc_run_param0);
    else
        g_list_free_full ((GList *) mc_run_param0, (GDestroyNotify) mcedit_arg_free);

    g_free (mc_run_param1);
    g_free (saved_other_dir);

    mc_config_deinit_config_paths ();

    (void) mc_event_deinit (&mcerror);
    if (mcerror != NULL)
    {
        fprintf (stderr, _("\nFailed while close:\n%s\n"), mcerror->message);
        g_error_free (mcerror);
        exit_code = EXIT_FAILURE;
    }

    (void) putchar ('\n');      /* Hack to make shell's prompt start at left of screen */

    return exit_code;
}

/* --------------------------------------------------------------------------------------------- */
