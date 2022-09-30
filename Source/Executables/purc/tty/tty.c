/*
   Interface to the terminal controlling library.

   Copyright (C) 2005-2021
   Free Software Foundation, Inc.

   Written by:
   Roland Illig <roland.illig@gmx.de>, 2005.
   Andrew Borodin <aborodin@vmail.ru>, 2009.

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

/** \file tty.c
 *  \brief Source: %interface to the terminal controlling library
 */

#include "config.h"

#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>             /* memset() */

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#else
#include <sys/time.h>
#include <sys/types.h>
#endif
#include <unistd.h>             /* exit() */

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

/* In some systems (like Solaris 11.4 SPARC), TIOCSWINSZ is defined in termios.h */
#include <termios.h>

#include "global.h"
#include "strutil.h"

#include "tty.h"
#include "tty-internal.h"
#include "color.h"              /* tty_set_normal_attrs() */
#include "mouse.h"              /* use_mouse_p */
#include "win.h"

/*** global variables ****************************************************************************/

int mc_tty_frm[MC_TTY_FRM_MAX];

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

static SIG_ATOMIC_VOLATILE_T got_interrupt = 0;

/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

static void
sigintr_handler (int signo)
{
    (void) &signo;
    got_interrupt = 1;
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

/**
 * Check terminal type. If $TERM is not set or value is empty, mc finishes with EXIT_FAILURE.
 *
 * @param force_xterm Set forced the XTerm type
 *
 * @return true if @param force_xterm is true or value of $TERM is one of following:
 *         term*
 *         konsole*
 *         rxvt*
 *         Eterm
 *         dtterm
 *         alacritty*
 *         foot*
 *         screen*
 *         tmux*
 */
bool
tty_check_term (bool force_xterm)
{
    const char *termvalue;

    termvalue = getenv ("TERM");
    if (termvalue == NULL || *termvalue == '\0')
    {
        fputs (_("The TERM environment variable is unset!\n"), stderr);
        exit (EXIT_FAILURE);
    }

    /* *INDENT-OFF* */
    return force_xterm
        || strncmp (termvalue, "xterm", 5) == 0
        || strncmp (termvalue, "konsole", 7) == 0
        || strncmp (termvalue, "rxvt", 4) == 0
        || strcmp (termvalue, "Eterm") == 0
        || strcmp (termvalue, "dtterm") == 0
        || strncmp (termvalue, "alacritty", 9) == 0
        || strncmp (termvalue, "foot", 4) == 0
        || strncmp (termvalue, "screen", 6) == 0
        || strncmp (termvalue, "tmux", 4) == 0;
    /* *INDENT-ON* */
}

/* --------------------------------------------------------------------------------------------- */

extern void
tty_start_interrupt_key (void)
{
    struct sigaction act;

    memset (&act, 0, sizeof (act));
    act.sa_handler = sigintr_handler;
    sigemptyset (&act.sa_mask);
#ifdef SA_RESTART
    act.sa_flags = SA_RESTART;
#endif /* SA_RESTART */
    sigaction (SIGINT, &act, NULL);
}

/* --------------------------------------------------------------------------------------------- */

extern void
tty_enable_interrupt_key (void)
{
    struct sigaction act;

    memset (&act, 0, sizeof (act));
    act.sa_handler = sigintr_handler;
    sigemptyset (&act.sa_mask);
    sigaction (SIGINT, &act, NULL);
    got_interrupt = 0;
}

/* --------------------------------------------------------------------------------------------- */

extern void
tty_disable_interrupt_key (void)
{
    struct sigaction act;

    memset (&act, 0, sizeof (act));
    act.sa_handler = SIG_IGN;
    sigemptyset (&act.sa_mask);
    sigaction (SIGINT, &act, NULL);
}

/* --------------------------------------------------------------------------------------------- */

extern bool
tty_got_interrupt (void)
{
    bool rv;

    rv = (got_interrupt != 0);
    got_interrupt = 0;
    return rv;
}

/* --------------------------------------------------------------------------------------------- */

bool
tty_got_winch (void)
{
    fd_set fdset;
    /* *INDENT-OFF* */
    /* instant timeout */
    struct timeval timeout = { .tv_sec = 0, .tv_usec = 0 };
    /* *INDENT-ON* */
    int ok;

    FD_ZERO (&fdset);
    FD_SET (sigwinch_pipe[0], &fdset);

    while ((ok = select (sigwinch_pipe[0] + 1, &fdset, NULL, NULL, &timeout)) < 0)
        if (errno != EINTR)
        {
            perror (_("Cannot check SIGWINCH pipe"));
            exit (EXIT_FAILURE);
        }

    return (ok != 0 && FD_ISSET (sigwinch_pipe[0], &fdset));
}

/* --------------------------------------------------------------------------------------------- */

void
tty_flush_winch (void)
{
    ssize_t n;

    /* merge all SIGWINCH events raised to this moment */
    do
    {
        char x[16];

        /* read multiple events at a time  */
        n = read (sigwinch_pipe[0], &x, sizeof (x));
    }
    while (n > 0 || (n == -1 && errno == EINTR));
}

/* --------------------------------------------------------------------------------------------- */

void
tty_print_one_hline (bool single)
{
    tty_print_alt_char (ACS_HLINE, single);
}

/* --------------------------------------------------------------------------------------------- */

void
tty_print_one_vline (bool single)
{
    tty_print_alt_char (ACS_VLINE, single);
}

/* --------------------------------------------------------------------------------------------- */

void
tty_draw_box (int y, int x, int ys, int xs, bool single)
{
    int y2, x2;

    if (ys <= 0 || xs <= 0)
        return;

    ys--;
    xs--;

    y2 = y + ys;
    x2 = x + xs;

    tty_draw_vline (y, x, mc_tty_frm[single ? MC_TTY_FRM_VERT : MC_TTY_FRM_DVERT], ys);
    tty_draw_vline (y, x2, mc_tty_frm[single ? MC_TTY_FRM_VERT : MC_TTY_FRM_DVERT], ys);
    tty_draw_hline (y, x, mc_tty_frm[single ? MC_TTY_FRM_HORIZ : MC_TTY_FRM_DHORIZ], xs);
    tty_draw_hline (y2, x, mc_tty_frm[single ? MC_TTY_FRM_HORIZ : MC_TTY_FRM_DHORIZ], xs);
    tty_gotoyx (y, x);
    tty_print_alt_char (ACS_ULCORNER, single);
    tty_gotoyx (y2, x);
    tty_print_alt_char (ACS_LLCORNER, single);
    tty_gotoyx (y, x2);
    tty_print_alt_char (ACS_URCORNER, single);
    tty_gotoyx (y2, x2);
    tty_print_alt_char (ACS_LRCORNER, single);
}

/* --------------------------------------------------------------------------------------------- */

void
tty_draw_box_shadow (int y, int x, int rows, int cols, int shadow_color)
{
    /* draw right shadow */
    tty_colorize_area (y + 1, x + cols, rows - 1, 2, shadow_color);
    /* draw bottom shadow */
    tty_colorize_area (y + rows, x + 2, 1, cols, shadow_color);
}

/* --------------------------------------------------------------------------------------------- */

char *
mc_tty_normalize_from_utf8 (const char *str)
{
    GIConv conv;
    GString *buffer;
    const char *_system_codepage = str_detect_termencoding ();

    if (str_isutf8 (_system_codepage))
        return g_strdup (str);

    conv = g_iconv_open (_system_codepage, "UTF-8");
    if (conv == INVALID_CONV)
        return g_strdup (str);

    buffer = g_string_new ("");

    if (str_convert (conv, str, buffer) == ESTR_FAILURE)
    {
        g_string_free (buffer, TRUE);
        str_close_conv (conv);
        return g_strdup (str);
    }
    str_close_conv (conv);

    return g_string_free (buffer, FALSE);
}

/* --------------------------------------------------------------------------------------------- */

/** Resize given terminal using TIOCSWINSZ, return ioctl() result */
int
tty_resize (int fd)
{
#if defined TIOCSWINSZ
    struct winsize tty_size;

    tty_size.ws_row = LINES;
    tty_size.ws_col = COLS;
    tty_size.ws_xpixel = tty_size.ws_ypixel = 0;

    return ioctl (fd, TIOCSWINSZ, &tty_size);
#else
    return 0;
#endif
}

/* --------------------------------------------------------------------------------------------- */

/** Clear screen */
void
tty_clear_screen (void)
{
    tty_set_normal_attrs ();
    tty_fill_region (0, 0, LINES, COLS, ' ');
    tty_refresh ();
}

/* --------------------------------------------------------------------------------------------- */

void
tty_init_xterm_support (bool is_xterm)
{
    const char *termvalue;

    termvalue = getenv ("TERM");

    /* Check mouse and ca capabilities */
    /* terminfo/termcap structures have been already initialized,
       in slang_init() or/and init_curses()  */
    /* Check terminfo at first, then check termcap */
    xmouse_seq = tty_tgetstr ("kmous");
    if (xmouse_seq == NULL)
        xmouse_seq = tty_tgetstr ("Km");
    smcup = tty_tgetstr ("smcup");
    if (smcup == NULL)
        smcup = tty_tgetstr ("ti");
    rmcup = tty_tgetstr ("rmcup");
    if (rmcup == NULL)
        rmcup = tty_tgetstr ("te");

    if (strcmp (termvalue, "cygwin") == 0)
    {
        is_xterm = TRUE;
        use_mouse_p = MOUSE_DISABLED;
    }

    if (is_xterm)
    {
        /* Default to the standard xterm sequence */
        if (xmouse_seq == NULL)
            xmouse_seq = ESC_STR "[M";

        /* Enable mouse unless explicitly disabled by --nomouse */
        if (use_mouse_p != MOUSE_DISABLED)
        {
            if (mc_global.tty.old_mouse)
                use_mouse_p = MOUSE_XTERM_NORMAL_TRACKING;
            else
            {
                /* FIXME: this dirty hack to set supported type of tracking the mouse */
                const char *color_term = getenv ("COLORTERM");
                if (strncmp (termvalue, "rxvt", 4) == 0 ||
                    (color_term != NULL && strncmp (color_term, "rxvt", 4) == 0) ||
                    strcmp (termvalue, "Eterm") == 0)
                    use_mouse_p = MOUSE_XTERM_NORMAL_TRACKING;
                else
                    use_mouse_p = MOUSE_XTERM_BUTTON_EVENT_TRACKING;
            }
        }
    }

    /* There's only one termcap entry "kmous", typically containing "\E[M" or "\E[<".
     * We need the former in xmouse_seq, the latter in xmouse_extended_seq.
     * See tickets 2956, 3954, and 4063 for details. */
    if (xmouse_seq != NULL)
    {
        if (strcmp (xmouse_seq, ESC_STR "[<") == 0)
            xmouse_seq = ESC_STR "[M";

        xmouse_extended_seq = ESC_STR "[<";
    }
}

/* --------------------------------------------------------------------------------------------- */
