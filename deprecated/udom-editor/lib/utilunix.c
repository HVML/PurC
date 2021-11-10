/*
   Various utilities - Unix variants

   Copyright (C) 1994-2021
   Free Software Foundation, Inc.

   Written by:
   Miguel de Icaza, 1994, 1995, 1996
   Janne Kukonlehto, 1994, 1995, 1996
   Dugan Porter, 1994, 1995, 1996
   Jakub Jelinek, 1994, 1995, 1996
   Mauricio Plaza, 1994, 1995, 1996

   The mc_realpath routine is mostly from uClibc package, written
   by Rick Sladkey <jrs@world.std.com>

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

/** \file utilunix.c
 *  \brief Source: various utilities - Unix variant
 */

#include <config.h>

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#include <sys/wait.h>
#include <pwd.h>
#include <grp.h>

#include "lib/global.h"

#include "lib/unixcompat.h"
#include "lib/vfs/vfs.h"        /* VFS_ENCODING_PREFIX */
#include "lib/strutil.h"        /* str_move() */
#include "lib/util.h"
#include "lib/widget.h"         /* message() */
#include "lib/vfs/xdirentry.h"

#ifdef HAVE_CHARSET
#include "lib/charsets.h"
#endif

#include "utilunix.h"

/*** global variables ****************************************************************************/

struct sigaction startup_handler;

/*** file scope macro definitions ****************************************************************/

#define UID_CACHE_SIZE 200
#define GID_CACHE_SIZE 30

/* Pipes are guaranteed to be able to hold at least 4096 bytes */
/* More than that would be unportable */
#define MAX_PIPE_SIZE 4096

/*** file scope type declarations ****************************************************************/

typedef struct
{
    int index;
    char *string;
} int_cache;

typedef enum
{
    FORK_ERROR = -1,
    FORK_CHILD,
    FORK_PARENT,
} my_fork_state_t;

typedef struct
{
    struct sigaction intr;
    struct sigaction quit;
    struct sigaction stop;
} my_system_sigactions_t;

/*** file scope variables ************************************************************************/

static int_cache uid_cache[UID_CACHE_SIZE];
static int_cache gid_cache[GID_CACHE_SIZE];

/* --------------------------------------------------------------------------------------------- */
/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

static char *
i_cache_match (int id, int_cache * cache, int size)
{
    int i;

    for (i = 0; i < size; i++)
        if (cache[i].index == id)
            return cache[i].string;
    return 0;
}

/* --------------------------------------------------------------------------------------------- */

static void
i_cache_add (int id, int_cache * cache, int size, char *text, int *last)
{
    g_free (cache[*last].string);
    cache[*last].string = g_strdup (text);
    cache[*last].index = id;
    *last = ((*last) + 1) % size;
}

/* --------------------------------------------------------------------------------------------- */

static my_fork_state_t
my_fork (void)
{
    pid_t pid;

    pid = fork ();

    if (pid < 0)
    {
        fprintf (stderr, "\n\nfork () = -1\n");
        return FORK_ERROR;
    }

    if (pid == 0)
        return FORK_CHILD;

    while (TRUE)
    {
        int status = 0;

        if (waitpid (pid, &status, 0) > 0)
            return WEXITSTATUS (status) == 0 ? FORK_PARENT : FORK_ERROR;

        if (errno != EINTR)
            return FORK_ERROR;
    }
}

/* --------------------------------------------------------------------------------------------- */

static void
my_system__save_sigaction_handlers (my_system_sigactions_t * sigactions)
{
    struct sigaction ignore;

    memset (&ignore, 0, sizeof (ignore));
    ignore.sa_handler = SIG_IGN;
    sigemptyset (&ignore.sa_mask);

    sigaction (SIGINT, &ignore, &sigactions->intr);
    sigaction (SIGQUIT, &ignore, &sigactions->quit);

    /* Restore the original SIGTSTP handler, we don't want ncurses' */
    /* handler messing the screen after the SIGCONT */
    sigaction (SIGTSTP, &startup_handler, &sigactions->stop);
}

/* --------------------------------------------------------------------------------------------- */

static void
my_system__restore_sigaction_handlers (my_system_sigactions_t * sigactions)
{
    sigaction (SIGINT, &sigactions->intr, NULL);
    sigaction (SIGQUIT, &sigactions->quit, NULL);
    sigaction (SIGTSTP, &sigactions->stop, NULL);
}

/* --------------------------------------------------------------------------------------------- */

static GPtrArray *
my_system_make_arg_array (int flags, const char *shell, char **execute_name)
{
    GPtrArray *args_array;

    args_array = g_ptr_array_new ();

    if ((flags & EXECUTE_AS_SHELL) != 0)
    {
        g_ptr_array_add (args_array, (gpointer) shell);
        g_ptr_array_add (args_array, (gpointer) "-c");
        *execute_name = g_strdup (shell);
    }
    else
    {
        char *shell_token;

        shell_token = shell != NULL ? strchr (shell, ' ') : NULL;
        if (shell_token == NULL)
            *execute_name = g_strdup (shell);
        else
            *execute_name = g_strndup (shell, (gsize) (shell_token - shell));

        g_ptr_array_add (args_array, (gpointer) shell);
    }
    return args_array;
}

/* --------------------------------------------------------------------------------------------- */

static void
mc_pread_stream (mc_pipe_stream_t * ps, const fd_set * fds)
{
    size_t buf_len;
    ssize_t read_len;

    if (!FD_ISSET (ps->fd, fds))
    {
        ps->len = MC_PIPE_STREAM_UNREAD;
        return;
    }

    buf_len = (size_t) ps->len;

    if (buf_len >= MC_PIPE_BUFSIZE)
        buf_len = ps->null_term ? MC_PIPE_BUFSIZE - 1 : MC_PIPE_BUFSIZE;

    do
    {
        read_len = read (ps->fd, ps->buf, buf_len);
    }
    while (read_len < 0 && errno == EINTR);

    if (read_len < 0)
    {
        /* reading error */
        ps->len = MC_PIPE_ERROR_READ;
        ps->error = errno;
    }
    else if (read_len == 0)
        /* EOF */
        ps->len = MC_PIPE_STREAM_EOF;
    else
    {
        /* success */
        ps->len = read_len;

        if (ps->null_term)
            ps->buf[(size_t) ps->len] = '\0';
    }

    ps->pos = 0;
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

const char *
get_owner (uid_t uid)
{
    struct passwd *pwd;
    char *name;
    static uid_t uid_last;

    name = i_cache_match ((int) uid, uid_cache, UID_CACHE_SIZE);
    if (name != NULL)
        return name;

    pwd = getpwuid (uid);
    if (pwd != NULL)
    {
        i_cache_add ((int) uid, uid_cache, UID_CACHE_SIZE, pwd->pw_name, (int *) &uid_last);
        return pwd->pw_name;
    }
    else
    {
        static char ibuf[10];

        g_snprintf (ibuf, sizeof (ibuf), "%d", (int) uid);
        return ibuf;
    }
}

/* --------------------------------------------------------------------------------------------- */

const char *
get_group (gid_t gid)
{
    struct group *grp;
    char *name;
    static gid_t gid_last;

    name = i_cache_match ((int) gid, gid_cache, GID_CACHE_SIZE);
    if (name != NULL)
        return name;

    grp = getgrgid (gid);
    if (grp != NULL)
    {
        i_cache_add ((int) gid, gid_cache, GID_CACHE_SIZE, grp->gr_name, (int *) &gid_last);
        return grp->gr_name;
    }
    else
    {
        static char gbuf[10];

        g_snprintf (gbuf, sizeof (gbuf), "%d", (int) gid);
        return gbuf;
    }
}

/* --------------------------------------------------------------------------------------------- */
/* Since ncurses uses a handler that automatically refreshes the */
/* screen after a SIGCONT, and we don't want this behavior when */
/* spawning a child, we save the original handler here */

void
save_stop_handler (void)
{
    sigaction (SIGTSTP, NULL, &startup_handler);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Wrapper for _exit() system call.
 * The _exit() function has gcc's attribute 'noreturn', and this is reason why we can't
 * mock the call.
 *
 * @param status exit code
 */

void
/* __attribute__ ((noreturn)) */
my_exit (int status)
{
    _exit (status);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Call external programs.
 *
 * @parameter flags   addition conditions for running external programs.
 * @parameter shell   shell (if flags contain EXECUTE_AS_SHELL), command to run otherwise.
 *                    Shell (or command) will be found in paths described in PATH variable
 *                    (if shell parameter doesn't begin from path delimiter)
 * @parameter command Command for shell (or first parameter for command, if flags contain EXECUTE_AS_SHELL)
 * @return 0 if successfull, -1 otherwise
 */

int
my_system (int flags, const char *shell, const char *command)
{
    return my_systeml (flags, shell, command, NULL);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Call external programs with various parameters number.
 *
 * @parameter flags addition conditions for running external programs.
 * @parameter shell shell (if flags contain EXECUTE_AS_SHELL), command to run otherwise.
 *                  Shell (or command) will be found in pathes described in PATH variable
 *                  (if shell parameter doesn't begin from path delimiter)
 * @parameter ...   Command for shell with addition parameters for shell
 *                  (or parameters for command, if flags contain EXECUTE_AS_SHELL).
 *                  Should be NULL terminated.
 * @return 0 if successfull, -1 otherwise
 */

int
my_systeml (int flags, const char *shell, ...)
{
    GPtrArray *args_array;
    int status = 0;
    va_list vargs;
    char *one_arg;

    args_array = g_ptr_array_new ();

    va_start (vargs, shell);
    while ((one_arg = va_arg (vargs, char *)) != NULL)
          g_ptr_array_add (args_array, one_arg);
    va_end (vargs);

    g_ptr_array_add (args_array, NULL);
    status = my_systemv_flags (flags, shell, (char *const *) args_array->pdata);

    g_ptr_array_free (args_array, TRUE);

    return status;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Call external programs with array of strings as parameters.
 *
 * @parameter command command to run. Command will be found in paths described in PATH variable
 *                    (if command parameter doesn't begin from path delimiter)
 * @parameter argv    Array of strings (NULL-terminated) with parameters for command
 * @return 0 if successfull, -1 otherwise
 */

int
my_systemv (const char *command, char *const argv[])
{
    my_fork_state_t fork_state;
    int status = 0;
    my_system_sigactions_t sigactions;

    my_system__save_sigaction_handlers (&sigactions);

    fork_state = my_fork ();
    switch (fork_state)
    {
    case FORK_ERROR:
        status = -1;
        break;
    case FORK_CHILD:
        {
            signal (SIGINT, SIG_DFL);
            signal (SIGQUIT, SIG_DFL);
            signal (SIGTSTP, SIG_DFL);
            signal (SIGCHLD, SIG_DFL);

            execvp (command, argv);
            my_exit (127);      /* Exec error */
        }
        MC_FALLTHROUGH;
        /* no break here, or unreachable-code warning by no returning my_exit() */
    default:
        status = 0;
        break;
    }
    my_system__restore_sigaction_handlers (&sigactions);

    return status;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Call external programs with flags and with array of strings as parameters.
 *
 * @parameter flags   addition conditions for running external programs.
 * @parameter command shell (if flags contain EXECUTE_AS_SHELL), command to run otherwise.
 *                    Shell (or command) will be found in paths described in PATH variable
 *                    (if shell parameter doesn't begin from path delimiter)
 * @parameter argv    Array of strings (NULL-terminated) with parameters for command
 * @return 0 if successfull, -1 otherwise
 */

int
my_systemv_flags (int flags, const char *command, char *const argv[])
{
    char *execute_name = NULL;
    GPtrArray *args_array;
    int status = 0;

    args_array = my_system_make_arg_array (flags, command, &execute_name);

    for (; argv != NULL && *argv != NULL; argv++)
        g_ptr_array_add (args_array, *argv);

    g_ptr_array_add (args_array, NULL);
    status = my_systemv (execute_name, (char *const *) args_array->pdata);

    g_free (execute_name);
    g_ptr_array_free (args_array, TRUE);

    return status;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Create pipe and run child process.
 *
 * @parameter command command line of child process
 * @parameter read_out do or don't read the stdout of child process
 * @parameter read_err do or don't read the stderr of child process
 * @paremeter error contains pointer to object to handle error code and message
 *
 * @return newly created object of mc_pipe_t class in success, NULL otherwise
 */

mc_pipe_t *
mc_popen (const char *command, gboolean read_out, gboolean read_err, GError ** error)
{
    mc_pipe_t *p;
    const char *const argv[] = { "/bin/sh", "sh", "-c", command, NULL };

    p = g_try_new (mc_pipe_t, 1);
    if (p == NULL)
    {
        mc_replace_error (error, MC_PIPE_ERROR_CREATE_PIPE, "%s",
                          _("Cannot create pipe descriptor"));
        goto ret_err;
    }

    p->out.fd = -1;
    p->err.fd = -1;

    if (!g_spawn_async_with_pipes
        (NULL, (gchar **) argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_FILE_AND_ARGV_ZERO, NULL,
         NULL, &p->child_pid, NULL, read_out ? &p->out.fd : NULL, read_err ? &p->err.fd : NULL,
         error))
    {
        mc_replace_error (error, MC_PIPE_ERROR_CREATE_PIPE_STREAM, "%s",
                          _("Cannot create pipe streams"));
        goto ret_err;
    }

    p->out.buf[0] = '\0';
    p->out.len = MC_PIPE_BUFSIZE;
    p->out.null_term = FALSE;

    p->err.buf[0] = '\0';
    p->err.len = MC_PIPE_BUFSIZE;
    p->err.null_term = FALSE;

    return p;

  ret_err:
    g_free (p);
    return NULL;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Read stdout and stderr of pipe asynchronously.
 *
 * @parameter p pipe descriptor
 *
 * The lengths of read data contain in p->out.len and p->err.len.
 *
 * Before read, p->xxx.len is an input. It defines the number of data to read.
 * Should not be greater than MC_PIPE_BUFSIZE.
 *
 * After read, p->xxx.len is an output and contains the following:
 *   p->xxx.len > 0: an actual length of read data stored in p->xxx.buf;
 *   p->xxx.len == MC_PIPE_STREAM_EOF: EOF of stream p->xxx;
 *   p->xxx.len == MC_PIPE_STREAM_UNREAD: stream p->xxx was not read;
 *   p->xxx.len == MC_PIPE_ERROR_READ: reading error, and p->xxx.errno is set appropriately.
 *
 * @paremeter error contains pointer to object to handle error code and message
 */

void
mc_pread (mc_pipe_t * p, GError ** error)
{
    gboolean read_out, read_err;
    fd_set fds;
    int maxfd = 0;
    int res;

    if (error != NULL)
        *error = NULL;

    read_out = p->out.fd >= 0;
    read_err = p->err.fd >= 0;

    if (!read_out && !read_err)
    {
        p->out.len = MC_PIPE_STREAM_UNREAD;
        p->err.len = MC_PIPE_STREAM_UNREAD;
        return;
    }

    FD_ZERO (&fds);
    if (read_out)
    {
        FD_SET (p->out.fd, &fds);
        maxfd = p->out.fd;
    }

    if (read_err)
    {
        FD_SET (p->err.fd, &fds);
        maxfd = MAX (maxfd, p->err.fd);
    }

    /* no timeout */
    res = select (maxfd + 1, &fds, NULL, NULL, NULL);
    if (res < 0 && errno != EINTR)
    {
        mc_propagate_error (error, MC_PIPE_ERROR_READ,
                            _
                            ("Unexpected error in select() reading data from a child process:\n%s"),
                            unix_error_string (errno));
        return;
    }

    if (read_out)
        mc_pread_stream (&p->out, &fds);
    else
        p->out.len = MC_PIPE_STREAM_UNREAD;

    if (read_err)
        mc_pread_stream (&p->err, &fds);
    else
        p->err.len = MC_PIPE_STREAM_UNREAD;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Reads a line from @stream. Reading stops after an EOL or a newline. If a newline is read,
 * it is appended to the line.
 *
 * @stream mc_pipe_stream_t object
 *
 * @return newly created GString or NULL in case of EOL;
 */

GString *
mc_pstream_get_string (mc_pipe_stream_t * ps)
{
    char *s;
    size_t size, i;
    gboolean escape = FALSE;

    g_return_val_if_fail (ps != NULL, NULL);

    if (ps->len < 0)
        return NULL;

    size = ps->len - ps->pos;

    if (size == 0)
        return NULL;

    s = ps->buf + ps->pos;

    if (s[0] == '\0')
        return NULL;

    /* find '\0' or unescaped '\n' */
    for (i = 0; i < size && !(s[i] == '\0' || (s[i] == '\n' && !escape)); i++)
        escape = s[i] == '\\' ? !escape : FALSE;

    if (i != size && s[i] == '\n')
        i++;

    ps->pos += i;

    return g_string_new_len (s, i);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Close pipe and destroy pipe descriptor.
 *
 * @paremeter p pipe descriptor
 * @paremeter error contains pointer to object to handle error code and message
 */

void
mc_pclose (mc_pipe_t * p, GError ** error)
{
    int res;

    if (p->out.fd >= 0)
        res = close (p->out.fd);
    if (p->err.fd >= 0)
        res = close (p->err.fd);

    do
    {
        int status;

        res = waitpid (p->child_pid, &status, 0);
    }
    while (res < 0 && errno == EINTR);

    if (res < 0)
        mc_replace_error (error, MC_PIPE_ERROR_READ, _("Unexpected error in waitpid():\n%s"),
                          unix_error_string (errno));

    g_free (p);
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Perform tilde expansion if possible.
 *
 * @param directory pointer to the path
 *
 * @return newly allocated string, even if it's unchanged.
 */

char *
tilde_expand (const char *directory)
{
    struct passwd *passwd;
    const char *p, *q;

    if (*directory != '~')
        return g_strdup (directory);

    p = directory + 1;

    /* d = "~" or d = "~/" */
    if (*p == '\0' || IS_PATH_SEP (*p))
    {
        passwd = getpwuid (geteuid ());
        q = IS_PATH_SEP (*p) ? p + 1 : "";
    }
    else
    {
        q = strchr (p, PATH_SEP);
        if (q == NULL)
            passwd = getpwnam (p);
        else
        {
            char *name;

            name = g_strndup (p, q - p);
            passwd = getpwnam (name);
            q++;
            g_free (name);
        }
    }

    /* If we can't figure the user name, leave tilde unexpanded */
    if (passwd == NULL)
        return g_strdup (directory);

    return g_strconcat (passwd->pw_dir, PATH_SEP_STR, q, (char *) NULL);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Canonicalize path, and return a new path.  Do everything in place.
 * The new path differs from path in:
 *      Multiple '/'s are collapsed to a single '/'.
 *      Leading './'s and trailing '/.'s are removed.
 *      Trailing '/'s are removed.
 *      Non-leading '../'s and trailing '..'s are handled by removing
 *      portions of the path.
 * Well formed UNC paths are modified only in the local part.
 */

void
custom_canonicalize_pathname (char *path, CANON_PATH_FLAGS flags)
{
    char *p, *s;
    char *lpath = path;         /* path without leading UNC part */
    const size_t url_delim_len = strlen (VFS_PATH_URL_DELIMITER);

    /* Detect and preserve UNC paths: //server/... */
    if ((flags & CANON_PATH_GUARDUNC) != 0 && IS_PATH_SEP (path[0]) && IS_PATH_SEP (path[1]))
    {
        for (p = path + 2; p[0] != '\0' && !IS_PATH_SEP (p[0]); p++)
            ;
        if (IS_PATH_SEP (p[0]) && p > path + 2)
            lpath = p;
    }

    if (lpath[0] == '\0' || lpath[1] == '\0')
        return;

    if ((flags & CANON_PATH_JOINSLASHES) != 0)
    {
        /* Collapse multiple slashes */
        for (p = lpath; *p != '\0'; p++)
            if (IS_PATH_SEP (p[0]) && IS_PATH_SEP (p[1]) && (p == lpath || *(p - 1) != ':'))
            {
                s = p + 1;
                while (IS_PATH_SEP (*(++s)))
                    ;
                str_move (p + 1, s);
            }

        /* Collapse "/./" -> "/" */
        for (p = lpath; *p != '\0';)
            if (IS_PATH_SEP (p[0]) && p[1] == '.' && IS_PATH_SEP (p[2]))
                str_move (p, p + 2);
            else
                p++;
    }

    if ((flags & CANON_PATH_REMSLASHDOTS) != 0)
    {
        size_t len;

        /* Remove trailing slashes */
        for (p = lpath + strlen (lpath) - 1; p > lpath && IS_PATH_SEP (*p); p--)
        {
            if (p >= lpath + url_delim_len - 1
                && strncmp (p - url_delim_len + 1, VFS_PATH_URL_DELIMITER, url_delim_len) == 0)
                break;
            *p = '\0';
        }

        /* Remove leading "./" */
        if (lpath[0] == '.' && IS_PATH_SEP (lpath[1]))
        {
            if (lpath[2] == '\0')
            {
                lpath[1] = '\0';
                return;
            }

            str_move (lpath, lpath + 2);
        }

        /* Remove trailing "/" or "/." */
        len = strlen (lpath);
        if (len < 2)
            return;

        if (IS_PATH_SEP (lpath[len - 1])
            && (len < url_delim_len
                || strncmp (lpath + len - url_delim_len, VFS_PATH_URL_DELIMITER,
                            url_delim_len) != 0))
            lpath[len - 1] = '\0';
        else if (lpath[len - 1] == '.' && IS_PATH_SEP (lpath[len - 2]))
        {
            if (len == 2)
            {
                lpath[1] = '\0';
                return;
            }

            lpath[len - 2] = '\0';
        }
    }

    /* Collapse "/.." with the previous part of path */
    if ((flags & CANON_PATH_REMDOUBLEDOTS) != 0)
    {
#ifdef HAVE_CHARSET
        const size_t enc_prefix_len = strlen (VFS_ENCODING_PREFIX);
#endif /* HAVE_CHARSET */

        for (p = lpath; p[0] != '\0' && p[1] != '\0' && p[2] != '\0';)
        {
            if (!IS_PATH_SEP (p[0]) || p[1] != '.' || p[2] != '.'
                || (!IS_PATH_SEP (p[3]) && p[3] != '\0'))
            {
                p++;
                continue;
            }

            /* search for the previous token */
            s = p - 1;
            if (s >= lpath + url_delim_len - 2
                && strncmp (s - url_delim_len + 2, VFS_PATH_URL_DELIMITER, url_delim_len) == 0)
            {
                s -= (url_delim_len - 2);
                while (s >= lpath && !IS_PATH_SEP (*s--))
                    ;
            }

            while (s >= lpath)
            {
                if (s - url_delim_len > lpath
                    && strncmp (s - url_delim_len, VFS_PATH_URL_DELIMITER, url_delim_len) == 0)
                {
                    char *vfs_prefix = s - url_delim_len;
                    vfs_class *vclass;

                    while (vfs_prefix > lpath && !IS_PATH_SEP (*--vfs_prefix))
                        ;
                    if (IS_PATH_SEP (*vfs_prefix))
                        vfs_prefix++;
                    *(s - url_delim_len) = '\0';

                    vclass = vfs_prefix_to_class (vfs_prefix);
                    *(s - url_delim_len) = *VFS_PATH_URL_DELIMITER;

                    if (vclass != NULL && (vclass->flags & VFSF_REMOTE) != 0)
                    {
                        s = vfs_prefix;
                        continue;
                    }
                }

                if (IS_PATH_SEP (*s))
                    break;

                s--;
            }

            s++;

            /* If the previous token is "..", we cannot collapse it */
            if (s[0] == '.' && s[1] == '.' && s + 2 == p)
            {
                p += 3;
                continue;
            }

            if (p[3] != '\0')
            {
                if (s == lpath && IS_PATH_SEP (*s))
                {
                    /* "/../foo" -> "/foo" */
                    str_move (s + 1, p + 4);
                }
                else
                {
                    /* "token/../foo" -> "foo" */
#ifdef HAVE_CHARSET
                    if ((strncmp (s, VFS_ENCODING_PREFIX, enc_prefix_len) == 0)
                        && (is_supported_encoding (s + enc_prefix_len)))
                        /* special case: remove encoding */
                        str_move (s, p + 1);
                    else
#endif /* HAVE_CHARSET */
                        str_move (s, p + 4);
                }

                p = s > lpath ? s - 1 : s;
                continue;
            }

            /* trailing ".." */
            if (s == lpath)
            {
                /* "token/.." -> "." */
                if (!IS_PATH_SEP (lpath[0]))
                    lpath[0] = '.';
                lpath[1] = '\0';
            }
            else
            {
                /* "foo/token/.." -> "foo" */
                if (s == lpath + 1)
                    s[0] = '\0';
#ifdef HAVE_CHARSET
                else if ((strncmp (s, VFS_ENCODING_PREFIX, enc_prefix_len) == 0)
                         && (is_supported_encoding (s + enc_prefix_len)))
                {
                    /* special case: remove encoding */
                    s[0] = '.';
                    s[1] = '.';
                    s[2] = '\0';

                    /* search for the previous token */
                    /* IS_PATH_SEP (s[-1]) */
                    for (p = s - 1; p >= lpath && !IS_PATH_SEP (*p); p--)
                        ;

                    if (p >= lpath)
                        continue;
                }
#endif /* HAVE_CHARSET */
                else
                {
                    if (s >= lpath + url_delim_len
                        && strncmp (s - url_delim_len, VFS_PATH_URL_DELIMITER, url_delim_len) == 0)
                        *s = '\0';
                    else
                        s[-1] = '\0';
                }
            }

            break;
        }
    }
}

/* --------------------------------------------------------------------------------------------- */

void
canonicalize_pathname (char *path)
{
    custom_canonicalize_pathname (path, CANON_PATH_ALL);
}

/* --------------------------------------------------------------------------------------------- */

#ifdef HAVE_GET_PROCESS_STATS
int
gettimeofday (struct timeval *tp, void *tzp)
{
    (void) tzp;

    return get_process_stats (tp, PS_SELF, 0, 0);
}
#endif /* HAVE_GET_PROCESS_STATS */

/* --------------------------------------------------------------------------------------------- */

char *
mc_realpath (const char *path, char *resolved_path)
{
#ifdef HAVE_CHARSET
    const char *p = path;
    gboolean absolute_path = FALSE;

    if (IS_PATH_SEP (*p))
    {
        absolute_path = TRUE;
        p++;
    }

    /* ignore encoding: skip "#enc:" */
    if (g_str_has_prefix (p, VFS_ENCODING_PREFIX))
    {
        p += strlen (VFS_ENCODING_PREFIX);
        p = strchr (p, PATH_SEP);
        if (p != NULL)
        {
            if (!absolute_path && p[1] != '\0')
                p++;

            path = p;
        }
    }
#endif /* HAVE_CHARSET */

#ifdef HAVE_REALPATH
    return realpath (path, resolved_path);
#else
    {
        char copy_path[PATH_MAX];
        char got_path[PATH_MAX];
        char *new_path = got_path;
        char *max_path;
#ifdef S_IFLNK
        char link_path[PATH_MAX];
        int readlinks = 0;
        int n;
#endif /* S_IFLNK */

        /* Make a copy of the source path since we may need to modify it. */
        if (strlen (path) >= PATH_MAX - 2)
        {
            errno = ENAMETOOLONG;
            return NULL;
        }

        strcpy (copy_path, path);
        path = copy_path;
        max_path = copy_path + PATH_MAX - 2;
        /* If it's a relative pathname use getwd for starters. */
        if (!IS_PATH_SEP (*path))
        {
            new_path = g_get_current_dir ();
            if (new_path == NULL)
                strcpy (got_path, "");
            else
            {
                g_snprintf (got_path, sizeof (got_path), "%s", new_path);
                g_free (new_path);
                new_path = got_path;
            }

            new_path += strlen (got_path);
            if (!IS_PATH_SEP (new_path[-1]))
                *new_path++ = PATH_SEP;
        }
        else
        {
            *new_path++ = PATH_SEP;
            path++;
        }
        /* Expand each slash-separated pathname component. */
        while (*path != '\0')
        {
            /* Ignore stray "/". */
            if (IS_PATH_SEP (*path))
            {
                path++;
                continue;
            }
            if (*path == '.')
            {
                /* Ignore ".". */
                if (path[1] == '\0' || IS_PATH_SEP (path[1]))
                {
                    path++;
                    continue;
                }
                if (path[1] == '.')
                {
                    if (path[2] == '\0' || IS_PATH_SEP (path[2]))
                    {
                        path += 2;
                        /* Ignore ".." at root. */
                        if (new_path == got_path + 1)
                            continue;
                        /* Handle ".." by backing up. */
                        while (!IS_PATH_SEP ((--new_path)[-1]))
                            ;
                        continue;
                    }
                }
            }
            /* Safely copy the next pathname component. */
            while (*path != '\0' && !IS_PATH_SEP (*path))
            {
                if (path > max_path)
                {
                    errno = ENAMETOOLONG;
                    return NULL;
                }
                *new_path++ = *path++;
            }
#ifdef S_IFLNK
            /* Protect against infinite loops. */
            if (readlinks++ > MAXSYMLINKS)
            {
                errno = ELOOP;
                return NULL;
            }
            /* See if latest pathname component is a symlink. */
            *new_path = '\0';
            n = readlink (got_path, link_path, PATH_MAX - 1);
            if (n < 0)
            {
                /* EINVAL means the file exists but isn't a symlink. */
                if (errno != EINVAL)
                {
                    /* Make sure it's null terminated. */
                    *new_path = '\0';
                    strcpy (resolved_path, got_path);
                    return NULL;
                }
            }
            else
            {
                /* Note: readlink doesn't add the null byte. */
                link_path[n] = '\0';
                if (IS_PATH_SEP (*link_path))
                    /* Start over for an absolute symlink. */
                    new_path = got_path;
                else
                    /* Otherwise back up over this component. */
                    while (!IS_PATH_SEP (*(--new_path)))
                        ;
                /* Safe sex check. */
                if (strlen (path) + n >= PATH_MAX - 2)
                {
                    errno = ENAMETOOLONG;
                    return NULL;
                }
                /* Insert symlink contents into path. */
                strcat (link_path, path);
                strcpy (copy_path, link_path);
                path = copy_path;
            }
#endif /* S_IFLNK */
            *new_path++ = PATH_SEP;
        }
        /* Delete trailing slash but don't whomp a lone slash. */
        if (new_path != got_path + 1 && IS_PATH_SEP (new_path[-1]))
            new_path--;
        /* Make sure it's null terminated. */
        *new_path = '\0';
        strcpy (resolved_path, got_path);
        return resolved_path;
    }
#endif /* HAVE_REALPATH */
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Return the index of the permissions triplet
 *
 */

int
get_user_permissions (struct stat *st)
{
    static gboolean initialized = FALSE;
    static gid_t *groups;
    static int ngroups;
    static uid_t uid;
    int i;

    if (!initialized)
    {
        uid = geteuid ();

        ngroups = getgroups (0, NULL);
        if (ngroups == -1)
            ngroups = 0;        /* ignore errors */

        /* allocate space for one element in addition to what
         * will be filled by getgroups(). */
        groups = g_new (gid_t, ngroups + 1);

        if (ngroups != 0)
        {
            ngroups = getgroups (ngroups, groups);
            if (ngroups == -1)
                ngroups = 0;    /* ignore errors */
        }

        /* getgroups() may or may not return the effective group ID,
         * so we always include it at the end of the list. */
        groups[ngroups++] = getegid ();

        initialized = TRUE;
    }

    if (st->st_uid == uid || uid == 0)
        return 0;

    for (i = 0; i < ngroups; i++)
        if (st->st_gid == groups[i])
            return 1;

    return 2;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Build filename from arguments.
 * Like to g_build_filename(), but respect VFS_PATH_URL_DELIMITER
 */

char *
mc_build_filenamev (const char *first_element, va_list args)
{
    gboolean absolute;
    const char *element = first_element;
    GString *path;
    char *ret;

    if (element == NULL)
        return NULL;

    path = g_string_new ("");

    absolute = IS_PATH_SEP (*first_element);

    do
    {
        if (*element == '\0')
            element = va_arg (args, char *);
        else
        {
            char *tmp_element;
            size_t len;
            const char *start;

            tmp_element = g_strdup (element);

            element = va_arg (args, char *);

            canonicalize_pathname (tmp_element);
            len = strlen (tmp_element);
            start = IS_PATH_SEP (tmp_element[0]) ? tmp_element + 1 : tmp_element;

            g_string_append (path, start);
            if (!IS_PATH_SEP (tmp_element[len - 1]) && element != NULL)
                g_string_append_c (path, PATH_SEP);

            g_free (tmp_element);
        }
    }
    while (element != NULL);

    if (absolute)
        g_string_prepend_c (path, PATH_SEP);

    ret = g_string_free (path, FALSE);
    canonicalize_pathname (ret);

    return ret;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Build filename from arguments.
 * Like to g_build_filename(), but respect VFS_PATH_URL_DELIMITER
 */

char *
mc_build_filename (const char *first_element, ...)
{
    va_list args;
    char *ret;

    if (first_element == NULL)
        return NULL;

    va_start (args, first_element);
    ret = mc_build_filenamev (first_element, args);
    va_end (args);
    return ret;
}

/* --------------------------------------------------------------------------------------------- */
