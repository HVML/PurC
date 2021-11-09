/*
   Input line filename/username/hostname/variable/command completion.
   (Let mc type for you...)

   Copyright (C) 1995-2021
   Free Software Foundation, Inc.

   Written by:
   Jakub Jelinek, 1995
   Slava Zanko <slavazanko@gmail.com>, 2013
   Andrew Borodin <aborodin@vmail.ru>, 2013

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

/** \file lib/widget/input_complete.c
 *  \brief Source: Input line filename/username/hostname/variable/command completion
 */

#include <config.h>

#include <ctype.h>
#include <limits.h>             /* MB_LEN_MAX */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <unistd.h>

#include "lib/global.h"

#include "lib/tty/tty.h"
#include "lib/tty/key.h"        /* XCTRL and ALT macros */
#include "lib/vfs/vfs.h"
#include "lib/strescape.h"
#include "lib/strutil.h"
#include "lib/util.h"
#include "lib/widget.h"

/*** global variables ****************************************************************************/

/* Linux declares environ in <unistd.h>, so don't repeat it here. */
#if (!(defined(__linux__) && defined (__USE_GNU)) && !defined(__CYGWIN__))
extern char **environ;
#endif

/*** file scope macro definitions ****************************************************************/

/* #define DO_COMPLETION_DEBUG */
#ifdef DO_COMPLETION_DEBUG
#define SHOW_C_CTX(func) fprintf(stderr, "%s: text='%s' flags=%s\n", func, text, show_c_flags(flags))
#else
#define SHOW_C_CTX(func)
#endif /* DO_CMPLETION_DEBUG */

#define DO_INSERTION 1
#define DO_QUERY     2

/*** file scope type declarations ****************************************************************/

typedef char *CompletionFunction (const char *text, int state, input_complete_t flags);

typedef struct
{
    size_t in_command_position;
    char *word;
    char *p;
    char *q;
    char *r;
    gboolean is_cd;
    input_complete_t flags;
} try_complete_automation_state_t;

/*** file scope variables ************************************************************************/

static char **hosts = NULL;
static char **hosts_p = NULL;
static int hosts_alloclen = 0;

static int complete_height, complete_width;
static WInput *input;
static int min_end;
static int start = 0;
static int end = 0;

/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

char **try_complete (char *text, int *lc_start, int *lc_end, input_complete_t flags);
void complete_engine_fill_completions (WInput * in);

#ifdef DO_COMPLETION_DEBUG
/**
 * Useful to print/debug completion flags
 */
static const char *
show_c_flags (input_complete_t flags)
{
    static char s_cf[] = "FHCVUDS";

    s_cf[0] = (flags & INPUT_COMPLETE_FILENAMES) != 0 ? 'F' : ' ';
    s_cf[1] = (flags & INPUT_COMPLETE_HOSTNAMES) != 0 ? 'H' : ' ';
    s_cf[2] = (flags & INPUT_COMPLETE_COMMANDS) != 0 ? 'C' : ' ';
    s_cf[3] = (flags & INPUT_COMPLETE_VARIABLES) != 0 ? 'V' : ' ';
    s_cf[4] = (flags & INPUT_COMPLETE_USERNAMES) != 0 ? 'U' : ' ';
    s_cf[5] = (flags & INPUT_COMPLETE_CD) != 0 ? 'D' : ' ';
    s_cf[6] = (flags & INPUT_COMPLETE_SHELL_ESC) != 0 ? 'S' : ' ';

    return s_cf;
}
#endif /* DO_CMPLETION_DEBUG */

/* --------------------------------------------------------------------------------------------- */

static char *
filename_completion_function (const char *text, int state, input_complete_t flags)
{
    static DIR *directory = NULL;
    static char *filename = NULL;
    static char *dirname = NULL;
    static char *users_dirname = NULL;
    static size_t filename_len = 0;
    static vfs_path_t *dirname_vpath = NULL;

    gboolean isdir = TRUE, isexec = FALSE;
    struct vfs_dirent *entry = NULL;

    SHOW_C_CTX ("filename_completion_function");

    if (text != NULL && (flags & INPUT_COMPLETE_SHELL_ESC) != 0)
    {
        char *u_text;
        char *result;
        char *e_result;

        u_text = strutils_shell_unescape (text);

        result = filename_completion_function (u_text, state, flags & (~INPUT_COMPLETE_SHELL_ESC));
        g_free (u_text);

        e_result = strutils_shell_escape (result);
        g_free (result);

        return e_result;
    }

    /* If we're starting the match process, initialize us a bit. */
    if (state == 0)
    {
        const char *temp;

        g_free (dirname);
        g_free (filename);
        g_free (users_dirname);
        vfs_path_free (dirname_vpath, TRUE);

        if ((*text != '\0') && (temp = strrchr (text, PATH_SEP)) != NULL)
        {
            filename = g_strdup (++temp);
            dirname = g_strndup (text, temp - text);
        }
        else
        {
            dirname = g_strdup (".");
            filename = g_strdup (text);
        }

        /* We aren't done yet.  We also support the "~user" syntax. */

        /* Save the version of the directory that the user typed. */
        users_dirname = dirname;
        dirname = tilde_expand (dirname);
        canonicalize_pathname (dirname);
        dirname_vpath = vfs_path_from_str (dirname);

        /* Here we should do something with variable expansion
           and `command`.
           Maybe a dream - UNIMPLEMENTED yet. */

        directory = mc_opendir (dirname_vpath);
        filename_len = strlen (filename);
    }

    /* Now that we have some state, we can read the directory. */

    while (directory != NULL && (entry = mc_readdir (directory)) != NULL)
    {
        if (!str_is_valid_string (entry->d_name))
            continue;

        /* Special case for no filename.
           All entries except "." and ".." match. */
        if (filename_len == 0)
        {
            if (DIR_IS_DOT (entry->d_name) || DIR_IS_DOTDOT (entry->d_name))
                continue;
        }
        else
        {
            /* Otherwise, if these match up to the length of filename, then
               it may be a match. */
            if ((entry->d_name[0] != filename[0]) ||
                ((NLENGTH (entry)) < filename_len) ||
                strncmp (filename, entry->d_name, filename_len) != 0)
                continue;
        }

        isdir = TRUE;
        isexec = FALSE;

        {
            struct stat tempstat;
            vfs_path_t *tmp_vpath;

            tmp_vpath = vfs_path_build_filename (dirname, entry->d_name, (char *) NULL);

            /* Unix version */
            if (mc_stat (tmp_vpath, &tempstat) == 0)
            {
                uid_t my_uid;
                gid_t my_gid;

                my_uid = getuid ();
                my_gid = getgid ();

                if (!S_ISDIR (tempstat.st_mode))
                {
                    isdir = FALSE;

                    if ((my_uid == 0 && (tempstat.st_mode & 0111) != 0) ||
                        (my_uid == tempstat.st_uid && (tempstat.st_mode & 0100) != 0) ||
                        (my_gid == tempstat.st_gid && (tempstat.st_mode & 0010) != 0) ||
                        (tempstat.st_mode & 0001) != 0)
                        isexec = TRUE;
                }
            }
            else
            {
                /* stat failed, strange. not a dir in any case */
                isdir = FALSE;
            }
            vfs_path_free (tmp_vpath, TRUE);
        }

        if ((flags & INPUT_COMPLETE_COMMANDS) != 0 && (isexec || isdir))
            break;
        if ((flags & INPUT_COMPLETE_CD) != 0 && isdir)
            break;
        if ((flags & INPUT_COMPLETE_FILENAMES) != 0)
            break;
    }

    if (entry == NULL)
    {
        if (directory != NULL)
        {
            mc_closedir (directory);
            directory = NULL;
        }
        MC_PTR_FREE (dirname);
        vfs_path_free (dirname_vpath, TRUE);
        dirname_vpath = NULL;
        MC_PTR_FREE (filename);
        MC_PTR_FREE (users_dirname);
        return NULL;
    }

    {
        GString *temp;

        temp = g_string_sized_new (16);

        if (users_dirname != NULL && (users_dirname[0] != '.' || users_dirname[1] != '\0'))
        {
            g_string_append (temp, users_dirname);

            /* We need a '/' at the end. */
            if (!IS_PATH_SEP (temp->str[temp->len - 1]))
                g_string_append_c (temp, PATH_SEP);
        }
        g_string_append (temp, entry->d_name);
        if (isdir)
            g_string_append_c (temp, PATH_SEP);

        return g_string_free (temp, FALSE);
    }
}

/* --------------------------------------------------------------------------------------------- */
/** We assume here that text[0] == '~' , if you want to call it in another way,
   you have to change the code */

static char *
username_completion_function (const char *text, int state, input_complete_t flags)
{
    static struct passwd *entry = NULL;
    static size_t userlen = 0;

    (void) flags;
    SHOW_C_CTX ("username_completion_function");

    if (text[0] == '\\' && text[1] == '~')
        text++;
    if (state == 0)
    {                           /* Initialization stuff */
        setpwent ();
        userlen = strlen (text + 1);
    }

    while ((entry = getpwent ()) != NULL)
    {
        /* Null usernames should result in all users as possible completions. */
        if (userlen == 0)
            break;
        if (text[1] == entry->pw_name[0] && strncmp (text + 1, entry->pw_name, userlen) == 0)
            break;
    }

    if (entry != NULL)
        return g_strconcat ("~", entry->pw_name, PATH_SEP_STR, (char *) NULL);

    endpwent ();
    return NULL;
}

/* --------------------------------------------------------------------------------------------- */
/** We assume text [0] == '$' and want to have a look at text [1], if it is
   equal to '{', so that we should append '}' at the end */

static char *
variable_completion_function (const char *text, int state, input_complete_t flags)
{
    static char **env_p = NULL;
    static gboolean isbrace = FALSE;
    static size_t varlen = 0;
    const char *p = NULL;

    (void) flags;
    SHOW_C_CTX ("variable_completion_function");

    if (state == 0)
    {                           /* Initialization stuff */
        isbrace = (text[1] == '{');
        varlen = strlen (text + 1 + isbrace);
        env_p = environ;
    }

    while (*env_p != NULL)
    {
        p = strchr (*env_p, '=');
        if (p != NULL && ((size_t) (p - *env_p) >= varlen)
            && strncmp (text + 1 + isbrace, *env_p, varlen) == 0)
            break;
        env_p++;
    }

    if (*env_p == NULL)
        return NULL;

    {
        GString *temp;

        temp = g_string_new_len (*env_p, p - *env_p);

        if (isbrace)
        {
            g_string_prepend_c (temp, '{');
            g_string_append_c (temp, '}');
        }
        g_string_prepend_c (temp, '$');

        env_p++;

        return g_string_free (temp, FALSE);
    }
}

/* --------------------------------------------------------------------------------------------- */

static void
fetch_hosts (const char *filename)
{
    FILE *file;
    char buffer[256];
    char *name;
    char *lc_start;
    char *bi;

    file = fopen (filename, "r");
    if (file == NULL)
        return;

    while (fgets (buffer, sizeof (buffer) - 1, file) != NULL)
    {
        /* Skip to first character. */
        for (bi = buffer; bi[0] != '\0' && str_isspace (bi); str_next_char (&bi))
            ;

        /* Ignore comments... */
        if (bi[0] == '#')
            continue;

        /* Handle $include. */
        if (strncmp (bi, "$include ", 9) == 0)
        {
            char *includefile, *t;

            /* Find start of filename. */
            includefile = bi + 9;
            while (*includefile != '\0' && whitespace (*includefile))
                includefile++;
            t = includefile;

            /* Find end of filename. */
            while (t[0] != '\0' && !str_isspace (t))
                str_next_char (&t);
            *t = '\0';

            fetch_hosts (includefile);
            continue;
        }

        /* Skip IP #s. */
        while (bi[0] != '\0' && !str_isspace (bi))
            str_next_char (&bi);

        /* Get the host names separated by white space. */
        while (bi[0] != '\0' && bi[0] != '#')
        {
            while (bi[0] != '\0' && str_isspace (bi))
                str_next_char (&bi);
            if (bi[0] == '#')
                continue;
            for (lc_start = bi; bi[0] != '\0' && !str_isspace (bi); str_next_char (&bi))
                ;

            if (bi == lc_start)
                continue;

            name = g_strndup (lc_start, bi - lc_start);

            {
                char **host_p;
                int j;

                j = hosts_p - hosts;

                if (j >= hosts_alloclen)
                {
                    hosts_alloclen += 30;
                    hosts = g_renew (char *, hosts, hosts_alloclen + 1);
                    hosts_p = hosts + j;
                }

                for (host_p = hosts; host_p < hosts_p; host_p++)
                    if (strcmp (name, *host_p) == 0)
                        break;  /* We do not want any duplicates */

                if (host_p == hosts_p)
                {
                    *(hosts_p++) = name;
                    *hosts_p = NULL;
                }
                else
                    g_free (name);
            }
        }
    }

    fclose (file);
}

/* --------------------------------------------------------------------------------------------- */

static char *
hostname_completion_function (const char *text, int state, input_complete_t flags)
{
    static char **host_p = NULL;
    static size_t textstart = 0;
    static size_t textlen = 0;

    (void) flags;
    SHOW_C_CTX ("hostname_completion_function");

    if (state == 0)
    {                           /* Initialization stuff */
        const char *p;

        g_strfreev (hosts);
        hosts_alloclen = 30;
        hosts = g_new (char *, hosts_alloclen + 1);
        *hosts = NULL;
        hosts_p = hosts;
        p = getenv ("HOSTFILE");
        fetch_hosts (p != NULL ? p : "/etc/hosts");
        host_p = hosts;
        textstart = (*text == '@') ? 1 : 0;
        textlen = strlen (text + textstart);
    }

    for (; *host_p != NULL; host_p++)
    {
        if (textlen == 0)
            break;              /* Match all of them */
        if (strncmp (text + textstart, *host_p, textlen) == 0)
            break;
    }

    if (*host_p == NULL)
    {
        g_strfreev (hosts);
        hosts = NULL;
        return NULL;
    }

    {
        GString *temp;

        temp = g_string_sized_new (8);

        if (textstart != 0)
            g_string_append_c (temp, '@');
        g_string_append (temp, *host_p);
        host_p++;

        return g_string_free (temp, FALSE);
    }
}

/* --------------------------------------------------------------------------------------------- */
/**
 * This is the function to call when the word to complete is in a position
 * where a command word can be found. It looks around $PATH, looking for
 * commands that match. It also scans aliases, function names, and the
 * table of shell built-ins.
 */

static char *
command_completion_function (const char *text, int state, input_complete_t flags)
{
    static const char *path_end = NULL;
    static gboolean isabsolute = FALSE;
    static int phase = 0;
    static size_t text_len = 0;
    static const char *const *words = NULL;
    static char *path = NULL;
    static char *cur_path = NULL;
    static char *cur_word = NULL;
    static int init_state = 0;
    static const char *const bash_reserved[] = {
        "if", "then", "else", "elif", "fi", "case", "esac", "for",
        "select", "while", "until", "do", "done", "in", "function", 0
    };
    static const char *const bash_builtins[] = {
        "alias", "bg", "bind", "break", "builtin", "cd", "command",
        "continue", "declare", "dirs", "echo", "enable", "eval",
        "exec", "exit", "export", "fc", "fg", "getopts", "hash",
        "help", "history", "jobs", "kill", "let", "local", "logout",
        "popd", "pushd", "pwd", "read", "readonly", "return", "set",
        "shift", "source", "suspend", "test", "times", "trap", "type",
        "typeset", "ulimit", "umask", "unalias", "unset", "wait", 0
    };

    char *u_text;
    char *p, *found;

    SHOW_C_CTX ("command_completion_function");

    if ((flags & INPUT_COMPLETE_COMMANDS) == 0)
        return NULL;

    u_text = strutils_shell_unescape (text);
    flags &= ~INPUT_COMPLETE_SHELL_ESC;

    if (state == 0)
    {                           /* Initialize us a little bit */
        isabsolute = strchr (u_text, PATH_SEP) != NULL;
        if (!isabsolute)
        {
            words = bash_reserved;
            phase = 0;
            text_len = strlen (u_text);

            if (path == NULL)
            {
                path = g_strdup (getenv ("PATH"));
                if (path != NULL)
                {
                    p = path;
                    path_end = strchr (p, '\0');
                    while ((p = strchr (p, PATH_ENV_SEP)) != NULL)
                        *p++ = '\0';
                }
            }
        }
    }

    if (isabsolute)
    {
        p = filename_completion_function (u_text, state, flags);

        if (p != NULL)
        {
            char *temp_p = p;

            p = strutils_shell_escape (p);
            g_free (temp_p);
        }

        g_free (u_text);
        return p;
    }

    found = NULL;
    switch (phase)
    {
    case 0:                    /* Reserved words */
        for (; *words != NULL; words++)
            if (strncmp (*words, u_text, text_len) == 0)
            {
                g_free (u_text);
                return g_strdup (*(words++));
            }
        phase++;
        words = bash_builtins;
        MC_FALLTHROUGH;
    case 1:                    /* Builtin commands */
        for (; *words != NULL; words++)
            if (strncmp (*words, u_text, text_len) == 0)
            {
                g_free (u_text);
                return g_strdup (*(words++));
            }
        phase++;
        if (path == NULL)
            break;
        cur_path = path;
        cur_word = NULL;
        MC_FALLTHROUGH;
    case 2:                    /* And looking through the $PATH */
        while (found == NULL)
        {
            if (cur_word == NULL)
            {
                char *expanded;

                if (cur_path >= path_end)
                    break;
                expanded = tilde_expand (*cur_path != '\0' ? cur_path : ".");
                cur_word = mc_build_filename (expanded, u_text, (char *) NULL);
                g_free (expanded);
                canonicalize_pathname (cur_word);
                cur_path = strchr (cur_path, '\0') + 1;
                init_state = state;
            }
            found = filename_completion_function (cur_word, state - init_state, flags);
            if (found == NULL)
                MC_PTR_FREE (cur_word);
        }
        MC_FALLTHROUGH;
    default:
        break;
    }

    if (found == NULL)
        MC_PTR_FREE (path);
    else
    {
        p = strrchr (found, PATH_SEP);
        if (p != NULL)
        {
            char *tmp = found;

            found = strutils_shell_escape (p + 1);
            g_free (tmp);
        }
    }

    g_free (u_text);
    return found;
}

/* --------------------------------------------------------------------------------------------- */

static int
match_compare (const void *a, const void *b)
{
    return strcmp (*(char *const *) a, *(char *const *) b);
}

/* --------------------------------------------------------------------------------------------- */
/** Returns an array of char * matches with the longest common denominator
   in the 1st entry. Then a NULL terminated list of different possible
   completions follows.
   You have to supply your own CompletionFunction with the word you
   want to complete as the first argument and an count of previous matches
   as the second. 
   In case no matches were found we return NULL. */

static char **
completion_matches (const char *text, CompletionFunction entry_function, input_complete_t flags)
{
    /* Number of slots in match_list. */
    size_t match_list_size = 30;
    /* The list of matches. */
    char **match_list;
    /* Number of matches actually found. */
    size_t matches = 0;

    /* Temporary string binder. */
    char *string;

    match_list = g_new (char *, match_list_size + 1);
    match_list[1] = NULL;

    while ((string = (*entry_function) (text, matches, flags)) != NULL)
    {
        if (matches + 1 == match_list_size)
        {
            match_list_size += 30;
            match_list = (char **) g_renew (char *, match_list, match_list_size + 1);
        }
        match_list[++matches] = string;
        match_list[matches + 1] = NULL;
    }

    /* If there were any matches, then look through them finding out the
       lowest common denominator.  That then becomes match_list[0]. */
    if (matches == 0)
        MC_PTR_FREE (match_list);       /* There were no matches. */
    else
    {
        /* If only one match, just use that. */
        if (matches == 1)
        {
            match_list[0] = match_list[1];
            match_list[1] = NULL;
        }
        else
        {
            size_t i = 1;
            int low = 4096;     /* Count of max-matched characters. */
            size_t j;

            qsort (match_list + 1, matches, sizeof (char *), match_compare);

            /* And compare each member of the list with
               the next, finding out where they stop matching. 
               If we find two equal strings, we have to put one away... */

            j = i + 1;
            while (j < matches + 1)
            {
                char *si, *sj;
                char *ni, *nj;

                for (si = match_list[i], sj = match_list[j]; si[0] != '\0' && sj[0] != '\0';)
                {

                    ni = str_get_next_char (si);
                    nj = str_get_next_char (sj);

                    if (ni - si != nj - sj)
                        break;
                    if (strncmp (si, sj, ni - si) != 0)
                        break;

                    si = ni;
                    sj = nj;
                }

                if (si[0] == '\0' && sj[0] == '\0')
                {               /* Two equal strings */
                    g_free (match_list[j]);
                    j++;
                    if (j > matches)
                        break;
                    continue;   /* Look for a run of equal strings */
                }
                else if (low > si - match_list[i])
                    low = si - match_list[i];
                if (i + 1 != j) /* So there's some gap */
                    match_list[i + 1] = match_list[j];
                i++;
                j++;
            }
            matches = i;
            match_list[matches + 1] = NULL;
            match_list[0] = g_strndup (match_list[1], low);
        }
    }

    return match_list;
}

/* --------------------------------------------------------------------------------------------- */
/** Check if directory completion is needed */
static gboolean
check_is_cd (const char *text, int lc_start, input_complete_t flags)
{
    const char *p, *q;

    SHOW_C_CTX ("check_is_cd");

    if ((flags & INPUT_COMPLETE_CD) == 0)
        return FALSE;

    /* Skip initial spaces */
    p = text;
    q = text + lc_start;
    while (p < q && p[0] != '\0' && str_isspace (p))
        str_cnext_char (&p);

    /* Check if the command is "cd" and the cursor is after it */
    return (p[0] == 'c' && p[1] == 'd' && str_isspace (p + 2) && p + 2 < q);
}

/* --------------------------------------------------------------------------------------------- */

static void
try_complete_commands_prepare (try_complete_automation_state_t * state, char *text, int *lc_start)
{
    const char *command_separator_chars = ";|&{(`";
    char *ti;

    if (*lc_start == 0)
        ti = text;
    else
    {
        ti = str_get_prev_char (&text[*lc_start]);
        while (ti > text && whitespace (ti[0]))
            str_prev_char (&ti);
    }

    if (ti == text)
        state->in_command_position++;
    else if (strchr (command_separator_chars, ti[0]) != NULL)
    {
        state->in_command_position++;
        if (ti != text)
        {
            int this_char, prev_char;

            /* Handle the two character tokens '>&', '<&', and '>|'.
               We are not in a command position after one of these. */
            this_char = ti[0];
            prev_char = str_get_prev_char (ti)[0];

            /* Quoted */
            if ((this_char == '&' && (prev_char == '<' || prev_char == '>'))
                || (this_char == '|' && prev_char == '>') || (ti != text
                                                              && str_get_prev_char (ti)[0] == '\\'))
                state->in_command_position = 0;
        }
    }
}

/* --------------------------------------------------------------------------------------------- */

static void
try_complete_find_start_sign (try_complete_automation_state_t * state)
{
    if ((state->flags & INPUT_COMPLETE_COMMANDS) != 0)
        state->p = strrchr (state->word, '`');
    if ((state->flags & (INPUT_COMPLETE_COMMANDS | INPUT_COMPLETE_VARIABLES)) != 0)
    {
        state->q = strrchr (state->word, '$');

        /* don't substitute variable in \$ case */
        if (strutils_is_char_escaped (state->word, state->q))
        {
            /* drop '\\' */
            str_move (state->q - 1, state->q);
            /* adjust flags */
            state->flags &= ~INPUT_COMPLETE_VARIABLES;
            state->q = NULL;
        }
    }
    if ((state->flags & INPUT_COMPLETE_HOSTNAMES) != 0)
        state->r = strrchr (state->word, '@');
    if (state->q != NULL && state->q[1] == '(' && (state->flags & INPUT_COMPLETE_COMMANDS) != 0)
    {
        if (state->q > state->p)
            state->p = str_get_next_char (state->q);
        state->q = NULL;
    }
}

/* --------------------------------------------------------------------------------------------- */

static char **
try_complete_all_possible (try_complete_automation_state_t * state, char *text, int *lc_start)
{
    char **matches = NULL;

    if (state->in_command_position != 0)
    {
        SHOW_C_CTX ("try_complete:cmd_subst");
        matches =
            completion_matches (state->word, command_completion_function,
                                state->flags & (~INPUT_COMPLETE_FILENAMES));
    }
    else if ((state->flags & INPUT_COMPLETE_FILENAMES) != 0)
    {
        if (state->is_cd)
            state->flags &= ~(INPUT_COMPLETE_FILENAMES | INPUT_COMPLETE_COMMANDS);
        SHOW_C_CTX ("try_complete:filename_subst_1");
        matches = completion_matches (state->word, filename_completion_function, state->flags);

        if (matches == NULL && state->is_cd && !IS_PATH_SEP (*state->word) && *state->word != '~')
        {
            state->q = text + *lc_start;
            for (state->p = text;
                 *state->p != '\0' && state->p < state->q && whitespace (*state->p);
                 str_next_char (&state->p))
                ;
            if (strncmp (state->p, "cd", 2) == 0)
                for (state->p += 2;
                     *state->p != '\0' && state->p < state->q && whitespace (*state->p);
                     str_next_char (&state->p))
                    ;
            if (state->p == state->q)
            {
                char *cdpath_ref, *cdpath;
                char c;

                cdpath_ref = g_strdup (getenv ("CDPATH"));
                cdpath = cdpath_ref;
                c = (cdpath == NULL) ? '\0' : ':';

                while (matches == NULL && c == ':')
                {
                    char *s;

                    s = strchr (cdpath, ':');
                    /* cppcheck-suppress nullPointer */
                    if (s == NULL)
                        s = strchr (cdpath, '\0');
                    c = *s;
                    *s = '\0';
                    if (*cdpath != '\0')
                    {
                        state->r = mc_build_filename (cdpath, state->word, (char *) NULL);
                        SHOW_C_CTX ("try_complete:filename_subst_2");
                        matches =
                            completion_matches (state->r, filename_completion_function,
                                                state->flags);
                        g_free (state->r);
                    }
                    *s = c;
                    cdpath = str_get_next_char (s);
                }
                g_free (cdpath_ref);
            }
        }
    }
    return matches;
}

/* --------------------------------------------------------------------------------------------- */

static gboolean
insert_text (WInput * in, char *text, ssize_t size)
{
    size_t text_len;
    int buff_len;

    text_len = strlen (text);
    buff_len = str_length (in->buffer);
    size = MIN (size, (ssize_t) text_len) + start - end;
    if (strlen (in->buffer) + size >= (size_t) in->current_max_size)
    {
        /* Expand the buffer */
        char *narea;
        Widget *w = WIDGET (in);

        narea = g_try_realloc (in->buffer, in->current_max_size + size + w->cols);
        if (narea != NULL)
        {
            in->buffer = narea;
            in->current_max_size += size + w->cols;
        }
    }
    if (strlen (in->buffer) + 1 < (size_t) in->current_max_size)
    {
        if (size != 0)
            memmove (in->buffer + end + size, in->buffer + end, strlen (&in->buffer[end]) + 1);
        memmove (in->buffer + start, text, size - (start - end));
        in->point += str_length (in->buffer) - buff_len;
        input_update (in, TRUE);
        end += size;
    }

    return size != 0;
}

/* --------------------------------------------------------------------------------------------- */

static cb_ret_t
complete_callback (Widget * w, Widget * sender, widget_msg_t msg, int parm, void *data)
{
    static int bl = 0;

    WGroup *g = GROUP (w);
    WDialog *h = DIALOG (w);

    switch (msg)
    {
    case MSG_KEY:
        switch (parm)
        {
        case KEY_LEFT:
        case KEY_RIGHT:
            bl = 0;
            h->ret_value = 0;
            dlg_stop (h);
            return MSG_HANDLED;

        case KEY_BACKSPACE:
            bl = 0;
            /* exit from completion list if input line is empty */
            if (end == 0)
            {
                h->ret_value = 0;
                dlg_stop (h);
            }
            /* Refill the list box and start again */
            else if (end == min_end)
            {
                end = str_get_prev_char (&input->buffer[end]) - input->buffer;
                input_handle_char (input, parm);
                h->ret_value = B_USER;
                dlg_stop (h);
            }
            else
            {
                int new_end;
                int i;
                GList *e;

                new_end = str_get_prev_char (&input->buffer[end]) - input->buffer;

                for (i = 0, e = listbox_get_first_link (LISTBOX (g->current->data));
                     e != NULL; i++, e = g_list_next (e))
                {
                    WLEntry *le = LENTRY (e->data);

                    if (strncmp (input->buffer + start, le->text, new_end - start) == 0)
                    {
                        listbox_select_entry (LISTBOX (g->current->data), i);
                        end = new_end;
                        input_handle_char (input, parm);
                        widget_draw (WIDGET (g->current->data));
                        break;
                    }
                }
            }
            return MSG_HANDLED;

        default:
            if (parm < 32 || parm > 255)
            {
                bl = 0;
                if (widget_lookup_key (WIDGET (input), parm) != CK_Complete)
                    return MSG_NOT_HANDLED;

                if (end == min_end)
                    return MSG_HANDLED;

                /* This means we want to refill the list box and start again */
                h->ret_value = B_USER;
                dlg_stop (h);
            }
            else
            {
                static char buff[MB_LEN_MAX] = "";
                GList *e;
                int i;
                int need_redraw = 0;
                int low = 4096;
                char *last_text = NULL;

                buff[bl++] = (char) parm;
                buff[bl] = '\0';

                switch (str_is_valid_char (buff, bl))
                {
                case -1:
                    bl = 0;
                    MC_FALLTHROUGH;
                case -2:
                    return MSG_HANDLED;
                default:
                    break;
                }

                for (i = 0, e = listbox_get_first_link (LISTBOX (g->current->data));
                     e != NULL; i++, e = g_list_next (e))
                {
                    WLEntry *le = LENTRY (e->data);

                    if (strncmp (input->buffer + start, le->text, end - start) == 0
                        && strncmp (&le->text[end - start], buff, bl) == 0)
                    {
                        if (need_redraw == 0)
                        {
                            need_redraw = 1;
                            listbox_select_entry (LISTBOX (g->current->data), i);
                            last_text = le->text;
                        }
                        else
                        {
                            char *si, *sl;
                            int si_num = 0;
                            int sl_num = 0;

                            /* count symbols between start and end */
                            for (si = le->text + start; si < le->text + end;
                                 str_next_char (&si), si_num++)
                                ;
                            for (sl = last_text + start; sl < last_text + end;
                                 str_next_char (&sl), sl_num++)
                                ;

                            /* pointers to next symbols */
                            si = &le->text[str_offset_to_pos (le->text, ++si_num)];
                            sl = &last_text[str_offset_to_pos (last_text, ++sl_num)];

                            while (si[0] != '\0' && sl[0] != '\0')
                            {
                                char *nexti, *nextl;

                                nexti = str_get_next_char (si);
                                nextl = str_get_next_char (sl);

                                if (nexti - si != nextl - sl || strncmp (si, sl, nexti - si) != 0)
                                    break;

                                si = nexti;
                                sl = nextl;

                                si_num++;
                            }

                            last_text = le->text;

                            si = &last_text[str_offset_to_pos (last_text, si_num)];
                            if (low > si - last_text)
                                low = si - last_text;

                            need_redraw = 2;
                        }
                    }
                }

                if (need_redraw == 2)
                {
                    insert_text (input, last_text, low);
                    widget_draw (WIDGET (g->current->data));
                }
                else if (need_redraw == 1)
                {
                    h->ret_value = B_ENTER;
                    dlg_stop (h);
                }
                bl = 0;
            }
        }
        return MSG_HANDLED;

    default:
        return dlg_default_callback (w, sender, msg, parm, data);
    }
}

/* --------------------------------------------------------------------------------------------- */

/** Returns TRUE if the user would like to see us again */
static gboolean
complete_engine (WInput * in, int what_to_do)
{
    if (in->completions != NULL && str_offset_to_pos (in->buffer, in->point) != end)
        input_complete_free (in);

    if (in->completions == NULL)
        complete_engine_fill_completions (in);

    if (in->completions == NULL)
        tty_beep ();
    else
    {
        if ((what_to_do & DO_INSERTION) != 0
            || ((what_to_do & DO_QUERY) != 0 && in->completions[1] == NULL))
        {
            char *lc_complete = in->completions[0];

            if (!insert_text (in, lc_complete, strlen (lc_complete)) || in->completions[1] != NULL)
                tty_beep ();
            else
                input_complete_free (in);
        }

        if ((what_to_do & DO_QUERY) != 0 && in->completions != NULL && in->completions[1] != NULL)
        {
            int maxlen = 0, count = 0, i;
            int x, y, w, h;
            int start_x, start_y;
            char **p, *q;
            WDialog *complete_dlg;
            WListbox *complete_list;

            for (p = in->completions + 1; *p != NULL; count++, p++)
            {
                i = str_term_width1 (*p);
                if (i > maxlen)
                    maxlen = i;
            }

            start_x = WIDGET (in)->x;
            start_y = WIDGET (in)->y;
            if (start_y - 2 >= count)
            {
                y = start_y - 2 - count;
                h = 2 + count;
            }
            else if (start_y >= LINES - start_y - 1)
            {
                y = 0;
                h = start_y;
            }
            else
            {
                y = start_y + 1;
                h = LINES - start_y - 1;
            }
            x = start - in->term_first_shown - 2 + start_x;
            w = maxlen + 4;
            if (x + w > COLS)
                x = COLS - w;
            if (x < 0)
                x = 0;
            if (x + w > COLS)
                w = COLS;

            input = in;
            min_end = end;
            complete_height = h;
            complete_width = w;

            complete_dlg =
                dlg_create (TRUE, y, x, complete_height, complete_width, WPOS_KEEP_DEFAULT, TRUE,
                            dialog_colors, complete_callback, NULL, "[Completion]", NULL);
            complete_list = listbox_new (1, 1, h - 2, w - 2, FALSE, NULL);
            group_add_widget (GROUP (complete_dlg), complete_list);

            for (p = in->completions + 1; *p != NULL; p++)
                listbox_add_item (complete_list, LISTBOX_APPEND_AT_END, 0, *p, NULL, FALSE);

            i = dlg_run (complete_dlg);
            q = NULL;
            if (i == B_ENTER)
            {
                listbox_get_current (complete_list, &q, NULL);
                if (q != NULL)
                    insert_text (in, q, strlen (q));
            }
            if (q != NULL || end != min_end)
                input_complete_free (in);
            widget_destroy (WIDGET (complete_dlg));

            /* B_USER if user wants to start over again */
            return (i == B_USER);
        }
    }

    return FALSE;
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

/** Returns an array of matches, or NULL if none. */
char **
try_complete (char *text, int *lc_start, int *lc_end, input_complete_t flags)
{
    try_complete_automation_state_t state;
    char **matches = NULL;

    memset (&state, 0, sizeof (state));
    state.flags = flags;

    SHOW_C_CTX ("try_complete");
    state.word = g_strndup (text + *lc_start, *lc_end - *lc_start);

    state.is_cd = check_is_cd (text, *lc_start, state.flags);

    /* Determine if this could be a command word. It is if it appears at
       the start of the line (ignoring preceding whitespace), or if it
       appears after a character that separates commands. And we have to
       be in a INPUT_COMPLETE_COMMANDS flagged Input line. */
    if (!state.is_cd && (flags & INPUT_COMPLETE_COMMANDS) != 0)
        try_complete_commands_prepare (&state, text, lc_start);

    try_complete_find_start_sign (&state);

    /* Command substitution? */
    if (state.p > state.q && state.p > state.r)
    {
        SHOW_C_CTX ("try_complete:cmd_backq_subst");
        matches = completion_matches (str_cget_next_char (state.p),
                                      command_completion_function,
                                      state.flags & (~INPUT_COMPLETE_FILENAMES));
        if (matches != NULL)
            *lc_start += str_get_next_char (state.p) - state.word;
    }

    /* Variable name? */
    else if (state.q > state.p && state.q > state.r)
    {
        SHOW_C_CTX ("try_complete:var_subst");
        matches = completion_matches (state.q, variable_completion_function, state.flags);
        if (matches != NULL)
            *lc_start += state.q - state.word;
    }

    /* Starts with '@', then look through the known hostnames for 
       completion first. */
    else if (state.r > state.p && state.r > state.q)
    {
        SHOW_C_CTX ("try_complete:host_subst");
        matches = completion_matches (state.r, hostname_completion_function, state.flags);
        if (matches != NULL)
            *lc_start += state.r - state.word;
    }

    /* Starts with '~' and there is no slash in the word, then
       try completing this word as a username. */
    if (matches == NULL && *state.word == '~' && (state.flags & INPUT_COMPLETE_USERNAMES) != 0
        && strchr (state.word, PATH_SEP) == NULL)
    {
        SHOW_C_CTX ("try_complete:user_subst");
        matches = completion_matches (state.word, username_completion_function, state.flags);
    }

    /* If this word is in a command position, then
       complete over possible command names, including aliases, functions,
       and command names. */
    if (matches == NULL)
        matches = try_complete_all_possible (&state, text, lc_start);

    /* And finally if nothing found, try complete directory name */
    if (matches == NULL)
    {
        state.in_command_position = 0;
        matches = try_complete_all_possible (&state, text, lc_start);
    }

    g_free (state.word);

    if (matches != NULL &&
        (flags & (INPUT_COMPLETE_FILENAMES | INPUT_COMPLETE_SHELL_ESC)) !=
        (INPUT_COMPLETE_FILENAMES | INPUT_COMPLETE_SHELL_ESC))
    {
        /* FIXME: HACK? INPUT_COMPLETE_SHELL_ESC is used only in command line. */
        char **m;

        for (m = matches; *m != NULL; m++)
        {
            char *p;

            p = *m;
            *m = strutils_shell_escape (*m);
            g_free (p);
        }
    }

    return matches;
}

/* --------------------------------------------------------------------------------------------- */

void
complete_engine_fill_completions (WInput * in)
{
    char *s;
    const char *word_separators;

    word_separators = (in->completion_flags & INPUT_COMPLETE_SHELL_ESC) ? " \t;|<>" : "\t;|<>";

    end = str_offset_to_pos (in->buffer, in->point);

    s = in->buffer;
    if (in->point != 0)
    {
        /* get symbol before in->point */
        size_t i;

        for (i = in->point - 1; i > 0; i--)
            str_next_char (&s);
    }

    for (; s >= in->buffer; str_prev_char (&s))
    {
        start = s - in->buffer;
        if (strchr (word_separators, *s) != NULL && !strutils_is_char_escaped (in->buffer, s))
            break;
    }

    if (start < end)
    {
        str_next_char (&s);
        start = s - in->buffer;
    }

    in->completions = try_complete (in->buffer, &start, &end, in->completion_flags);
}

/* --------------------------------------------------------------------------------------------- */

/* declared in lib/widget/input.h */
void
input_complete (WInput * in)
{
    int engine_flags;

    if (!str_is_valid_string (in->buffer))
        return;

    if (in->completions != NULL)
        engine_flags = DO_QUERY;
    else
    {
        engine_flags = DO_INSERTION;

        if (mc_global.widget.show_all_if_ambiguous)
            engine_flags |= DO_QUERY;
    }

    while (complete_engine (in, engine_flags))
        ;
}

/* --------------------------------------------------------------------------------------------- */

void
input_complete_free (WInput * in)
{
    g_strfreev (in->completions);
    in->completions = NULL;
}

/* --------------------------------------------------------------------------------------------- */
