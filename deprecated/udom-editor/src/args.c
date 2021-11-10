/*
   Handle command line arguments.

   Copyright (C) 2009-2021
   Free Software Foundation, Inc.

   Written by:
   Slava Zanko <slavazanko@gmail.com>, 2009.
   Andrew Borodin <aborodin@vmail.ru>, 2011, 2012.

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

#include <config.h>

#include <stdlib.h>
#include <stdio.h>

#include "lib/global.h"
#include "lib/tty/tty.h"
#include "lib/strutil.h"
#include "lib/vfs/vfs.h"
#include "lib/util.h"           /* x_basename() */

#include "src/textconf.h"

#include "src/args.h"

/*** external variables **************************************************************************/

/*** global variables ****************************************************************************/

/* If true, assume we are running on an xterm terminal */
gboolean mc_args__force_xterm = FALSE;

gboolean mc_args__nomouse = FALSE;

/* Force colors, only used by Slang */
gboolean mc_args__force_colors = FALSE;

/* Don't load keymap from file and use default one */
gboolean mc_args__nokeymap = FALSE;

char *mc_args__last_wd_file = NULL;

/* when enabled NETCODE, use folowing file as logfile */
char *mc_args__netfs_logfile = NULL;

/* keymap file */
char *mc_args__keymap_file = NULL;

void *mc_run_param0 = NULL;
char *mc_run_param1 = NULL;

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/* If true, show version info and exit */
static gboolean mc_args__show_version = FALSE;

/* forward declarations */
static gboolean parse_mc_e_argument (const gchar * option_name, const gchar * value,
                                     gpointer data, GError ** mcerror);
static gboolean parse_mc_v_argument (const gchar * option_name, const gchar * value,
                                     gpointer data, GError ** mcerror);

static GOptionContext *context;

#ifdef ENABLE_SUBSHELL
static gboolean mc_args__nouse_subshell = FALSE;
#endif /* ENABLE_SUBSHELL */
static gboolean mc_args__show_datadirs = FALSE;
static gboolean mc_args__show_datadirs_extended = FALSE;
#ifdef ENABLE_CONFIGURE_ARGS
static gboolean mc_args__show_configure_opts = FALSE;
#endif

static GOptionGroup *main_group;

static const GOptionEntry argument_main_table[] = {
    /* *INDENT-OFF* */
    /* generic options */
    {
     "version", 'V', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE,
     &mc_args__show_version,
     N_("Displays the current version"),
     NULL
    },

    /* options for wrappers */
    {
     "datadir", 'f', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE,
     &mc_args__show_datadirs,
     N_("Print data directory"),
     NULL
    },

    /* show extended information about used data directories */
    {
     "datadir-info", 'F', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE,
     &mc_args__show_datadirs_extended,
     N_("Print extended info about used data directories"),
     NULL
    },

#ifdef ENABLE_CONFIGURE_ARGS
    /* show configure options */
    {
     "configure-options", '\0', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE,
     &mc_args__show_configure_opts,
     N_("Print configure options"),
     NULL
    },
#endif

    {
     "printwd", 'P', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_STRING,
     &mc_args__last_wd_file,
     N_("Print last working directory to specified file"),
     N_("<file>")
    },

#ifdef ENABLE_SUBSHELL
    {
     "subshell", 'U', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE,
     &mc_global.tty.use_subshell,
     N_("Enables subshell support (default)"),
     NULL
    },

    {
     "nosubshell", 'u', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE,
     &mc_args__nouse_subshell,
     N_("Disables subshell support"),
     NULL
    },
#endif

    /* debug options */
#ifdef ENABLE_VFS_FTP
    {
     "ftplog", 'l', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_STRING,
     &mc_args__netfs_logfile,
     N_("Log ftp dialog to specified file"),
     N_("<file>")
    },
#endif /* ENABLE_VFS_FTP */

    {
     /* handle arguments manually */
     "view", 'v', G_OPTION_FLAG_IN_MAIN | G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
     (gpointer) parse_mc_v_argument,
     N_("Launches the file viewer on a file"),
     N_("<file>")
    },

    {
     /* handle arguments manually */
     "edit", 'e', G_OPTION_FLAG_IN_MAIN | G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
     (gpointer) parse_mc_e_argument,
     N_("Edit files"),
     N_("<file> ...")
    },

    G_OPTION_ENTRY_NULL
    /* *INDENT-ON* */
};

static GOptionGroup *terminal_group;
#define ARGS_TERM_OPTIONS 0
static const GOptionEntry argument_terminal_table[] = {
    /* *INDENT-OFF* */
    /* terminal options */
    {
     "xterm", 'x', ARGS_TERM_OPTIONS, G_OPTION_ARG_NONE,
     &mc_args__force_xterm,
     N_("Forces xterm features"),
     NULL
    },

    {
     "no-x11", 'X', ARGS_TERM_OPTIONS, G_OPTION_ARG_NONE,
     &mc_global.tty.disable_x11,
     N_("Disable X11 support"),
     NULL
    },

    {
     "oldmouse", 'g', ARGS_TERM_OPTIONS, G_OPTION_ARG_NONE,
     &mc_global.tty.old_mouse,
     N_("Tries to use an old highlight mouse tracking"),
     NULL
    },

    {
     "nomouse", 'd', ARGS_TERM_OPTIONS, G_OPTION_ARG_NONE,
     &mc_args__nomouse,
     N_("Disable mouse support in text version"),
     NULL
    },

#ifdef HAVE_SLANG
    {
     "termcap", 't', ARGS_TERM_OPTIONS, G_OPTION_ARG_NONE,
     &SLtt_Try_Termcap,
     N_("Tries to use termcap instead of terminfo"),
     NULL
    },
#endif

    {
     "slow", 's', ARGS_TERM_OPTIONS, G_OPTION_ARG_NONE,
     &mc_global.tty.slow_terminal,
     N_("To run on slow terminals"),
     NULL
    },

    {
     "stickchars", 'a', ARGS_TERM_OPTIONS, G_OPTION_ARG_NONE,
     &mc_global.tty.ugly_line_drawing,
     N_("Use stickchars to draw"),
     NULL
    },

#ifdef HAVE_SLANG
    {
     "resetsoft", 'k', ARGS_TERM_OPTIONS, G_OPTION_ARG_NONE,
     &reset_hp_softkeys,
     N_("Resets soft keys on HP terminals"),
     NULL
    },
#endif

    {
     "keymap", 'K', ARGS_TERM_OPTIONS, G_OPTION_ARG_STRING,
     &mc_args__keymap_file,
     N_("Load definitions of key bindings from specified file"),
     N_("<file>")
    },

    {
     "nokeymap", '\0', ARGS_TERM_OPTIONS, G_OPTION_ARG_NONE,
     &mc_args__nokeymap,
     N_("Don't load definitions of key bindings from file, use defaults"),
     NULL
    },

    G_OPTION_ENTRY_NULL
    /* *INDENT-ON* */
};

#undef ARGS_TERM_OPTIONS

static GOptionGroup *color_group;
#define ARGS_COLOR_OPTIONS 0
/* #define ARGS_COLOR_OPTIONS G_OPTION_FLAG_IN_MAIN */
static const GOptionEntry argument_color_table[] = {
    /* *INDENT-OFF* */
    /* color options */
    {
     "nocolor", 'b', ARGS_COLOR_OPTIONS, G_OPTION_ARG_NONE,
     &mc_global.tty.disable_colors,
     N_("Requests to run in black and white"),
     NULL
    },

    {
     "color", 'c', ARGS_COLOR_OPTIONS, G_OPTION_ARG_NONE,
     &mc_args__force_colors,
     N_("Request to run in color mode"),
     NULL
    },

    {
     "colors", 'C', ARGS_COLOR_OPTIONS, G_OPTION_ARG_STRING,
     &mc_global.tty.command_line_colors,
     N_("Specifies a color configuration"),
     N_("<string>")
    },

    {
     "skin", 'S', ARGS_COLOR_OPTIONS, G_OPTION_ARG_STRING,
     &mc_global.tty.skin,
     N_("Show mc with specified skin"),
     N_("<string>")
    },

    G_OPTION_ENTRY_NULL
    /* *INDENT-ON* */
};

#undef ARGS_COLOR_OPTIONS

static gchar *mc_args__loc__colors_string = NULL;
static gchar *mc_args__loc__footer_string = NULL;
static gchar *mc_args__loc__header_string = NULL;
static gchar *mc_args__loc__usage_string = NULL;

/* --------------------------------------------------------------------------------------------- */
/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

static void
mc_args_clean_temp_help_strings (void)
{
    MC_PTR_FREE (mc_args__loc__colors_string);
    MC_PTR_FREE (mc_args__loc__footer_string);
    MC_PTR_FREE (mc_args__loc__header_string);
    MC_PTR_FREE (mc_args__loc__usage_string);
}

/* --------------------------------------------------------------------------------------------- */

static GOptionGroup *
mc_args_new_color_group (void)
{
    /* *INDENT-OFF* */
    /* FIXME: to preserve translations, lines should be split. */
    mc_args__loc__colors_string = g_strdup_printf ("%s\n%s",
                                                   /* TRANSLATORS: don't translate keywords */
                                                   _("--colors KEYWORD={FORE},{BACK},{ATTR}:KEYWORD2=...\n\n"
                                                     "{FORE}, {BACK} and {ATTR} can be omitted, and the default will be used\n"
                                                     "\n Keywords:\n"
                                                     "   Global:       errors, disabled, reverse, gauge, header\n"
                                                     "                 input, inputmark, inputunchanged, commandlinemark\n"
                                                     "                 bbarhotkey, bbarbutton, statusbar\n"
                                                     "   File display: normal, selected, marked, markselect\n"
                                                     "   Dialog boxes: dnormal, dfocus, dhotnormal, dhotfocus, errdhotnormal,\n"
                                                     "                 errdhotfocus\n"
                                                     "   Menus:        menunormal, menuhot, menusel, menuhotsel, menuinactive\n"
                                                     "   Popup menus:  pmenunormal, pmenusel, pmenutitle\n"
                                                     "   Editor:       editnormal, editbold, editmarked, editwhitespace,\n"
                                                     "                 editlinestate, editbg, editframe, editframeactive\n"
                                                     "                 editframedrag\n"
                                                     "   Viewer:       viewnormal,viewbold, viewunderline, viewselected\n"
                                                     "   Help:         helpnormal, helpitalic, helpbold, helplink, helpslink\n"),
                                                   /* TRANSLATORS: don't translate color names and attributes */
                                                   _("Standard Colors:\n"
                                                    "   black, gray, red, brightred, green, brightgreen, brown,\n"
                                                    "   yellow, blue, brightblue, magenta, brightmagenta, cyan,\n"
                                                    "   brightcyan, lightgray and white\n\n"
                                                    "Extended colors, when 256 colors are available:\n"
                                                    "   color16 to color255, or rgb000 to rgb555 and gray0 to gray23\n\n"
                                                    "Attributes:\n"
                                                    "   bold, italic, underline, reverse, blink; append more with '+'\n")
                                                    );
    /* *INDENT-ON* */

    return g_option_group_new ("color", mc_args__loc__colors_string,
                               _("Color options"), NULL, NULL);

}

/* --------------------------------------------------------------------------------------------- */

static gchar *
mc_args_add_usage_info (void)
{
    gchar *s;

    switch (mc_global.mc_run_mode)
    {
    case MC_RUN_EDITOR:
        s = g_strdup_printf ("%s\n", _("[+lineno] file1[:lineno] [file2[:lineno]...]"));
        break;
    case MC_RUN_VIEWER:
        s = g_strdup_printf ("%s\n", _("file"));
        break;
#ifdef USE_DIFF_VIEW
    case MC_RUN_DIFFVIEWER:
        s = g_strdup_printf ("%s\n", _("file1 file2"));
        break;
#endif /* USE_DIFF_VIEW */
    case MC_RUN_FULL:
    default:
        s = g_strdup_printf ("%s\n", _("[this_dir] [other_panel_dir]"));
    }

    mc_args__loc__usage_string = s;

    return mc_args__loc__usage_string;
}

/* --------------------------------------------------------------------------------------------- */

static void
mc_args_add_extended_info_to_help (void)
{
    mc_args__loc__footer_string = g_strdup_printf ("%s",
                                                   _
                                                   ("\n"
                                                    "Please send any bug reports (including the output of 'mc -V')\n"
                                                    "as tickets at www.midnight-commander.org\n"));
    mc_args__loc__header_string =
        g_strdup_printf (_("GNU Midnight Commander %s\n"), mc_global.mc_version);

    g_option_context_set_description (context, mc_args__loc__footer_string);
    g_option_context_set_summary (context, mc_args__loc__header_string);
}

/* --------------------------------------------------------------------------------------------- */

static gchar *
mc_args__convert_help_to_syscharset (const gchar * charset, const gchar * error_message_str,
                                     const gchar * help_str)
{
    GString *buffer;
    GIConv conv;
    gchar *full_help_str;

    buffer = g_string_new ("");
    conv = g_iconv_open (charset, "UTF-8");
    full_help_str = g_strdup_printf ("%s\n\n%s\n", error_message_str, help_str);

    str_convert (conv, full_help_str, buffer);

    g_free (full_help_str);
    g_iconv_close (conv);

    return g_string_free (buffer, FALSE);
}

/* --------------------------------------------------------------------------------------------- */

static gboolean
parse_mc_e_argument (const gchar * option_name, const gchar * value, gpointer data,
                     GError ** mcerror)
{
    (void) option_name;
    (void) value;
    (void) data;

    mc_return_val_if_error (mcerror, FALSE);

    mc_global.mc_run_mode = MC_RUN_EDITOR;

    return TRUE;
}

/* --------------------------------------------------------------------------------------------- */

static gboolean
parse_mc_v_argument (const gchar * option_name, const gchar * value, gpointer data,
                     GError ** mcerror)
{
    (void) option_name;
    (void) value;
    (void) data;

    mc_return_val_if_error (mcerror, FALSE);

    mc_global.mc_run_mode = MC_RUN_VIEWER;

    return TRUE;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Create mcedit_arg_t object from vfs_path_t object and the line number.
 *
 * @param file_vpath  file path object
 * @param line_number line number. If value is 0, try to restore saved position.
 * @return mcedit_arg_t object
 */

static mcedit_arg_t *
mcedit_arg_vpath_new (vfs_path_t * file_vpath, long line_number)
{
    mcedit_arg_t *arg;

    arg = g_new (mcedit_arg_t, 1);
    arg->file_vpath = file_vpath;
    arg->line_number = line_number;

    return arg;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Create mcedit_arg_t object from file name and the line number.
 *
 * @param file_name   file name
 * @param line_number line number. If value is 0, try to restore saved position.
 * @return mcedit_arg_t object
 */

static mcedit_arg_t *
mcedit_arg_new (const char *file_name, long line_number)
{
    return mcedit_arg_vpath_new (vfs_path_from_str (file_name), line_number);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Get list of filenames (and line numbers) from command line, when mc called as editor
 *
 * @param argc count of all arguments
 * @param argv array of strings, contains arguments
 * @return list of mcedit_arg_t objects
 */

static GList *
parse_mcedit_arguments (int argc, char **argv)
{
    GList *flist = NULL;
    int i;
    long first_line_number = -1;

    for (i = 0; i < argc; i++)
    {
        char *tmp;
        char *end, *p;
        mcedit_arg_t *arg;

        tmp = argv[i];

        /*
         * First, try to get line number as +lineno.
         */
        if (*tmp == '+')
        {
            long lineno;
            char *error;

            lineno = strtol (tmp + 1, &error, 10);

            if (*error == '\0')
            {
                /* this is line number */
                first_line_number = lineno;
                continue;
            }
            /* this is file name */
        }

        /*
         * Check for filename:lineno, followed by an optional colon.
         * This format is used by many programs (especially compilers)
         * in error messages and warnings. It is supported so that
         * users can quickly copy and paste file locations.
         */
        end = tmp + strlen (tmp);
        p = end;

        if (p > tmp && p[-1] == ':')
            p--;
        while (p > tmp && g_ascii_isdigit ((gchar) p[-1]))
            p--;

        if (tmp < p && p < end && p[-1] == ':')
        {
            char *fname;
            vfs_path_t *tmp_vpath, *fname_vpath;
            struct stat st;

            fname = g_strndup (tmp, p - 1 - tmp);
            tmp_vpath = vfs_path_from_str (tmp);
            fname_vpath = vfs_path_from_str (fname);

            /*
             * Check that the file before the colon actually exists.
             * If it doesn't exist, create new file.
             */
            if (mc_stat (tmp_vpath, &st) == -1 && mc_stat (fname_vpath, &st) != -1)
            {
                arg = mcedit_arg_vpath_new (fname_vpath, atoi (p));
                vfs_path_free (tmp_vpath, TRUE);
            }
            else
            {
                arg = mcedit_arg_vpath_new (tmp_vpath, 0);
                vfs_path_free (fname_vpath, TRUE);
            }

            g_free (fname);
        }
        else
            arg = mcedit_arg_new (tmp, 0);

        flist = g_list_prepend (flist, arg);
    }

    if (flist == NULL)
        flist = g_list_prepend (flist, mcedit_arg_new (NULL, 0));
    else if (first_line_number != -1)
    {
        /* overwrite line number for first file */
        GList *l;

        l = g_list_last (flist);
        ((mcedit_arg_t *) l->data)->line_number = first_line_number;
    }

    return flist;
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

void
mc_setup_run_mode (char **argv)
{
    const char *base;

    base = x_basename (argv[0]);

    if (strncmp (base, "mce", 3) == 0 || strcmp (base, "vi") == 0)
    {
        /* mce* or vi is link to mc */
        mc_global.mc_run_mode = MC_RUN_EDITOR;
    }
    else if (strncmp (base, "mcv", 3) == 0 || strcmp (base, "view") == 0)
    {
        /* mcv* or view is link to mc */
        mc_global.mc_run_mode = MC_RUN_VIEWER;
    }
#ifdef USE_DIFF_VIEW
    else if (strncmp (base, "mcd", 3) == 0 || strcmp (base, "diff") == 0)
    {
        /* mcd* or diff is link to mc */
        mc_global.mc_run_mode = MC_RUN_DIFFVIEWER;
    }
#endif /* USE_DIFF_VIEW */
}

gboolean
mc_args_parse (int *argc, char ***argv, const char *translation_domain, GError ** mcerror)
{
    const gchar *_system_codepage;
    gboolean ok = TRUE;

    mc_return_val_if_error (mcerror, FALSE);

    _system_codepage = str_detect_termencoding ();

#ifdef ENABLE_NLS
    if (!str_isutf8 (_system_codepage))
        bind_textdomain_codeset ("mc", "UTF-8");
#endif

    context = g_option_context_new (mc_args_add_usage_info ());

    g_option_context_set_ignore_unknown_options (context, FALSE);

    mc_args_add_extended_info_to_help ();

    main_group = g_option_group_new ("main", _("Main options"), _("Main options"), NULL, NULL);

    g_option_group_add_entries (main_group, argument_main_table);
    g_option_context_set_main_group (context, main_group);
    g_option_group_set_translation_domain (main_group, translation_domain);

    terminal_group = g_option_group_new ("terminal", _("Terminal options"),
                                         _("Terminal options"), NULL, NULL);

    g_option_group_add_entries (terminal_group, argument_terminal_table);
    g_option_context_add_group (context, terminal_group);
    g_option_group_set_translation_domain (terminal_group, translation_domain);

    color_group = mc_args_new_color_group ();

    g_option_group_add_entries (color_group, argument_color_table);
    g_option_context_add_group (context, color_group);
    g_option_group_set_translation_domain (color_group, translation_domain);

    if (!g_option_context_parse (context, argc, argv, mcerror))
    {
        if (*mcerror == NULL)
            mc_propagate_error (mcerror, 0, "%s\n", _("Arguments parse error!"));
        else
        {
            gchar *help_str;

            help_str = g_option_context_get_help (context, TRUE, NULL);

            if (str_isutf8 (_system_codepage))
                mc_replace_error (mcerror, (*mcerror)->code, "%s\n\n%s\n", (*mcerror)->message,
                                  help_str);
            else
            {
                gchar *full_help_str;

                full_help_str =
                    mc_args__convert_help_to_syscharset (_system_codepage, (*mcerror)->message,
                                                         help_str);
                mc_replace_error (mcerror, (*mcerror)->code, "%s", full_help_str);
                g_free (full_help_str);
            }
            g_free (help_str);
        }

        ok = FALSE;
    }

    g_option_context_free (context);
    mc_args_clean_temp_help_strings ();

#ifdef ENABLE_NLS
    if (!str_isutf8 (_system_codepage))
        bind_textdomain_codeset ("mc", _system_codepage);
#endif

    return ok;
}

/* --------------------------------------------------------------------------------------------- */

gboolean
mc_args_show_info (void)
{
    if (mc_args__show_version)
    {
        show_version ();
        return FALSE;
    }

    if (mc_args__show_datadirs)
    {
        printf ("%s (%s)\n", mc_global.sysconfig_dir, mc_global.share_data_dir);
        return FALSE;
    }

    if (mc_args__show_datadirs_extended)
    {
        show_datadirs_extended ();
        return FALSE;
    }

#ifdef ENABLE_CONFIGURE_ARGS
    if (mc_args__show_configure_opts)
    {
        show_configure_options ();
        return FALSE;
    }
#endif

    return TRUE;
}

/* --------------------------------------------------------------------------------------------- */

gboolean
mc_setup_by_args (int argc, char **argv, GError ** mcerror)
{
    char *tmp;

    mc_return_val_if_error (mcerror, FALSE);

    if (mc_args__force_colors)
        mc_global.tty.disable_colors = FALSE;

#ifdef ENABLE_SUBSHELL
    if (mc_args__nouse_subshell)
        mc_global.tty.use_subshell = FALSE;
#endif /* ENABLE_SUBSHELL */

    if (mc_args__netfs_logfile != NULL)
    {
        vfs_path_t *vpath;
#ifdef ENABLE_VFS_FTP
        vpath = vfs_path_from_str ("ftp://");
        mc_setctl (vpath, VFS_SETCTL_LOGFILE, (void *) mc_args__netfs_logfile);
        vfs_path_free (vpath, TRUE);
#endif /* ENABLE_VFS_FTP */
        (void) vpath;
    }

    tmp = (argc > 0) ? argv[1] : NULL;

    switch (mc_global.mc_run_mode)
    {
    case MC_RUN_EDITOR:
        mc_run_param0 = parse_mcedit_arguments (argc - 1, &argv[1]);
        break;

    case MC_RUN_VIEWER:
        if (tmp == NULL)
        {
            mc_propagate_error (mcerror, 0, "%s\n", _("No arguments given to the viewer."));
            return FALSE;
        }

        mc_run_param0 = g_strdup (tmp);
        break;

#ifdef USE_DIFF_VIEW
    case MC_RUN_DIFFVIEWER:
        if (argc < 3)
        {
            mc_propagate_error (mcerror, 0, "%s\n",
                                _("Two files are required to envoke the diffviewer."));
            return FALSE;
        }
        MC_FALLTHROUGH;
#endif /* USE_DIFF_VIEW */

    case MC_RUN_FULL:
    default:
        /* set the current dir and the other dir for filemanager,
           or two files for diff viewer */
        if (tmp != NULL)
        {
            mc_run_param0 = g_strdup (tmp);
            tmp = (argc > 1) ? argv[2] : NULL;
            if (tmp != NULL)
                mc_run_param1 = g_strdup (tmp);
        }
        break;
    }

    return TRUE;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Free the mcedit_arg_t object.
 *
 * @param arg mcedit_arg_t object
 */

void
mcedit_arg_free (mcedit_arg_t * arg)
{
    vfs_path_free (arg->file_vpath, TRUE);
    g_free (arg);
}

/* --------------------------------------------------------------------------------------------- */
