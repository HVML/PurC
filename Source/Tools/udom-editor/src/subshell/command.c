/*
   Command line widget.
   This widget is derived from the WInput widget, it's used to cope
   with all the magic of the command input line, we depend on some
   help from the program's callback.

   Copyright (C) 1995-2021
   Free Software Foundation, Inc.

   Written by:
   Slava Zanko <slavazanko@gmail.com>, 2013
   Andrew Borodin <aborodin@vmail.ru>, 2020

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

/** \file command.c
 *  \brief Source: command line widget
 */

#include <config.h>

#include <stdlib.h>
#include <string.h>

#include "lib/global.h"
#include "lib/vfs/vfs.h"        /* vfs_current_is_local() */
#include "lib/skin.h"           /* DEFAULT_COLOR */
#include "lib/util.h"           /* whitespace() */
#include "lib/widget.h"

#include "src/setup.h"          /* quit */
#ifdef ENABLE_SUBSHELL
#include "src/subshell/subshell.h"
#endif
#include "src/execute.h"        /* shell_execute() */
#include "src/usermenu.h"       /* expand_format() */

#include "filemanager.h"        /* quiet_quit_cmd(), layout.h */
#include "cd.h"                 /* cd_to() */

#include "command.h"

/*** global variables ****************************************************************************/

/* This holds the command line */
WInput *cmdline;

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/* Color styles command line */
static input_colors_t command_colors;

/* --------------------------------------------------------------------------------------------- */
/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

/** Handle Enter on the command line
 *
 * @param lc_cmdline string for handling
 * @return MSG_HANDLED on sucsess else MSG_NOT_HANDLED
 */

static cb_ret_t
enter (WInput * lc_cmdline)
{
    char *cmd = lc_cmdline->buffer;

    if (!command_prompt)
        return MSG_HANDLED;

    /* Any initial whitespace should be removed at this point */
    while (whiteness (*cmd))
        cmd++;

    if (*cmd == '\0')
        return MSG_HANDLED;

    if (strncmp (cmd, "cd", 2) == 0 && (cmd[2] == '\0' || whitespace (cmd[2])))
    {
        cd_to (cmd + 2);
        input_clean (lc_cmdline);
        return MSG_HANDLED;
    }
    else if (strcmp (cmd, "exit") == 0)
    {
        input_assign_text (lc_cmdline, "");
        if (!quiet_quit_cmd ())
            return MSG_NOT_HANDLED;
    }
    else
    {
        GString *command;
        size_t i;

        if (!vfs_current_is_local ())
        {
            message (D_ERROR, MSG_ERROR, _("Cannot execute commands on non-local filesystems"));
            return MSG_NOT_HANDLED;
        }
#ifdef ENABLE_SUBSHELL
        /* Check this early before we clean command line
         * (will be checked again by shell_execute) */
        if (mc_global.tty.use_subshell && subshell_state != INACTIVE)
        {
            message (D_ERROR, MSG_ERROR, _("The shell is already running a command"));
            return MSG_NOT_HANDLED;
        }
#endif
        command = g_string_sized_new (32);

        for (i = 0; cmd[i] != '\0'; i++)
        {
            if (cmd[i] != '%')
                g_string_append_c (command, cmd[i]);
            else
            {
                char *s;

                s = expand_format (NULL, cmd[++i], TRUE);
                g_string_append (command, s);
                g_free (s);
            }
        }

        input_clean (lc_cmdline);
        shell_execute (command->str, 0);
        g_string_free (command, TRUE);

#ifdef ENABLE_SUBSHELL
        if ((quit & SUBSHELL_EXIT) != 0)
        {
            if (quiet_quit_cmd ())
                return MSG_HANDLED;

            quit = 0;
            /* restart subshell */
            if (mc_global.tty.use_subshell)
                init_subshell ();
        }

        if (mc_global.tty.use_subshell)
            do_load_prompt ();
#endif
    }
    return MSG_HANDLED;
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Default command line callback
 *
 * @param w Widget object
 * @param msg message for handling
 * @param parm extra parameter such as key code
 *
 * @return MSG_NOT_HANDLED on fail else MSG_HANDLED
 */

static cb_ret_t
command_callback (Widget * w, Widget * sender, widget_msg_t msg, int parm, void *data)
{
    switch (msg)
    {
    case MSG_KEY:
        /* Special case: we handle the enter key */
        if (parm == '\n')
            return enter (INPUT (w));
        MC_FALLTHROUGH;

    default:
        return input_callback (w, sender, msg, parm, data);
    }
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

WInput *
command_new (int y, int x, int cols)
{
    WInput *cmd;
    Widget *w;

    cmd = input_new (y, x, command_colors, cols, "", "cmdline",
                     INPUT_COMPLETE_FILENAMES | INPUT_COMPLETE_VARIABLES | INPUT_COMPLETE_USERNAMES
                     | INPUT_COMPLETE_HOSTNAMES | INPUT_COMPLETE_CD | INPUT_COMPLETE_COMMANDS |
                     INPUT_COMPLETE_SHELL_ESC);
    w = WIDGET (cmd);
    /* Don't set WOP_SELECTABLE up, otherwise panels will be unselected */
    widget_set_options (w, WOP_SELECTABLE, FALSE);
    /* Add our hooks */
    w->callback = command_callback;

    return cmd;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Set colors for the command line.
 */

void
command_set_default_colors (void)
{
    command_colors[WINPUTC_MAIN] = DEFAULT_COLOR;
    command_colors[WINPUTC_MARK] = COMMAND_MARK_COLOR;
    command_colors[WINPUTC_UNCHANGED] = DEFAULT_COLOR;
    command_colors[WINPUTC_HISTORY] = COMMAND_HISTORY_COLOR;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Insert quoted text in input line.  The function is meant for the
 * command line, so the percent sign is quoted as well.
 *
 * @param in WInput object
 * @param text string for insertion
 * @param insert_extra_space add extra space
 */

void
command_insert (WInput * in, const char *text, gboolean insert_extra_space)
{
    char *quoted_text;

    quoted_text = name_quote (text, TRUE);
    input_insert (in, quoted_text, insert_extra_space);
    g_free (quoted_text);
}

/* --------------------------------------------------------------------------------------------- */
