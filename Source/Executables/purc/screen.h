/*
 * @file screen.h
 * @author Vincent Wei
 * @date 2022/10/02
 * @brief The global definitions for terminal screen.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of purc, which is an HVML interpreter with
 * a command line interface (CLI).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef purc_screen_h
#define purc_screen_h

#if defined(HAVE_STRING_H)
#include <string.h>
   /* An ANSI string.h and pre-ANSI memory.h might conflict */
#elif defined(HAVE_MEMORY_H)
#include <memory.h>
#else
#include <strings.h>
    /* memory and strings.h conflict on other systems */
#endif /* !STDC_HEADERS & !HAVE_STRING_H */

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

/* for O_* macros */
#include <fcntl.h>

/* for sig_atomic_t */
#include <signal.h>

#define SIG_ATOMIC_VOLATILE_T       sig_atomic_t

#define DEFAULT_CHARSET "ASCII"

#ifndef PATH_MAX
#ifdef _POSIX_VERSION
#define PATH_MAX _POSIX_PATH_MAX
#else
#ifdef MAXPATHLEN
#define PATH_MAX MAXPATHLEN
#else
#define PATH_MAX 1024
#endif
#endif
#endif

#ifndef MAXPATHLEN
#define MC_MAXPATHLEN 4096
#else
#define MC_MAXPATHLEN MAXPATHLEN
#endif

#define DIR_IS_DOT(x) ((x)[0] == '.' && (x)[1] == '\0')
#define DIR_IS_DOTDOT(x) ((x)[0] == '.' && (x)[1] == '.' && (x)[2] == '\0')

#if defined(__GNUC__) && defined(__has_attribute) && \
    __has_attribute(fallthrough)
#define purc_FALLTHROUGH __attribute__ ((fallthrough))
#else
#define purc_FALLTHROUGH
#endif

/*** typedefs(not structures) and defined constants */

/* The O_BINARY definition was taken from gettext */
#if !defined O_BINARY && defined _O_BINARY
  /* For MSC-compatible compilers.  */
#define O_BINARY _O_BINARY
#endif
#ifdef __BEOS__
  /* BeOS 5 has O_BINARY, but is has no effect.  */
#undef O_BINARY
#endif
/* On reasonable systems, binary I/O is the default.  */
#ifndef O_BINARY
#define O_BINARY 0
#endif

/* Replacement for O_NONBLOCK */
#ifndef O_NONBLOCK
#ifdef O_NDELAY                 /* SYSV */
#define O_NONBLOCK O_NDELAY
#else /* BSD */
#define O_NONBLOCK FNDELAY
#endif /* !O_NDELAY */
#endif /* !O_NONBLOCK */

#if defined(__QNX__) && !defined(__QNXNTO__)
/* exec*() from <process.h> */
#include <unix.h>
#endif

#include <glib.h>

/* Solaris9 doesn't have PRIXMAX */
#ifndef PRIXMAX
#define PRIXMAX PRIxMAX
#endif

#ifdef ENABLE_NLS
#include <libintl.h>
#define _(String) gettext (String)
#ifdef gettext_noop
#define N_(String) gettext_noop (String)
#else
#define N_(String) (String)
#endif
#else /* Stubs that do something close enough.  */
#define textdomain(String) 1
#define gettext(String) (String)
#define ngettext(String1,String2,Num) (((Num) == 1) ? (String1) : (String2))
#define dgettext(Domain,Message) (Message)
#define dcgettext(Domain,Message,Type) (Message)
#define bindtextdomain(Domain,Directory) 1
#define _(String) (String)
#define N_(String) (String)
#endif /* !ENABLE_NLS */

/* Just for keeping Your's brains from invention a proper size of the buffer :-) */
#define BUF_10K 10240L
#define BUF_8K  8192L
#define BUF_4K  4096L
#define BUF_1K  1024L

#define BUF_LARGE  BUF_1K
#define BUF_MEDIUM 512
#define BUF_SMALL 128
#define BUF_TINY 64

/* ESC_CHAR is defined in /usr/include/langinfo.h in some systems */
#ifdef ESC_CHAR
#undef ESC_CHAR
#endif
/* AIX compiler doesn't understand '\e' */
#define ESC_CHAR '\033'
#define ESC_STR  "\033"

/* OS specific defines */
#define PATH_SEP '/'
#define PATH_SEP_STR "/"
#define IS_PATH_SEP(c) ((c) == PATH_SEP)
#define PATH_ENV_SEP ':'
#define TMPDIR_DEFAULT "/tmp"
#define SCRIPT_SUFFIX ""
#define get_default_editor() "vi"
#define OS_SORT_CASE_SENSITIVE_DEFAULT TRUE
#define UTF8_CHAR_LEN 6

/* struct stat members */
#ifdef __APPLE__
#define st_atim st_atimespec
#define st_ctim st_ctimespec
#define st_mtim st_mtimespec
#endif

/* Used to distinguish between a normal MC termination and */
/* one caused by typing 'exit' or 'logout' in the subshell */
#define SUBSHELL_EXIT 128

#define MC_ERROR g_quark_from_static_string("purc")

/*** structures declarations (and typedefs of structures) ********************/

typedef struct {
    gboolean keybar_visible;

    /* If true, allow characters in the range 160-255 */
    gboolean eight_bit_clean;
    /*
     * If true, also allow characters in the range 128-159.
     * This is reported to break on many terminals (xterm, qansi-m).
     */
    gboolean full_eight_bits;
    struct {
        /* Use the specified skin */
        char *skin;
        /* Dialog window and frop down menu have a shadow */
        gboolean shadows;

        char *setup_color_string;
        char *term_color_string;
        char *color_terminal_string;
        /* colors specified on the command line: they override any other setting */
        char *command_line_colors;
        char console_flag;

        /* If using a subshell for evaluating commands this is true */
        gboolean use_subshell;

#ifdef ENABLE_SUBSHELL
        /* File descriptors of the pseudoterminal used by the subshell */
        int subshell_pty;
#endif                          /* !ENABLE_SUBSHELL */

        /* This flag is set by xterm detection routine in function main() */
        /* It is used by function toggle_subshell() */
        gboolean xterm_flag;

        /* disable x11 support */
        gboolean disable_x11;

        /* For slow terminals */
        /* If true lines are shown by spaces */
        gboolean slow_terminal;

        /* Set to force black and white display at program startup */
        gboolean disable_colors;

        /* If true use +, -, | for line drawing */
        gboolean ugly_line_drawing;

        /* Tries to use old highlight mouse tracking */
        gboolean old_mouse;

        /* If true, use + and \ keys normally and select/unselect do if M-+ / M-\.
           and M-- and keypad + / - */
        gboolean alternate_plus_minus;
    } tty;

} mc_global_t;

/*** global variables defined in .c file */

extern mc_global_t mc_global;

void mc_refresh(void);
int vfs_timeouts(void);
void vfs_timeout_handler(void);

#endif  /* purc_screen_h */

