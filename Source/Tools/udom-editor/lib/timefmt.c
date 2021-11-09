/*
   Time formatting functions

   Copyright (C) 1994-2021
   Free Software Foundation, Inc.

   Written by:
   Miguel de Icaza, 1994, 1995, 1996
   Janne Kukonlehto, 1994, 1995, 1996
   Dugan Porter, 1994, 1995, 1996
   Jakub Jelinek, 1994, 1995, 1996
   Mauricio Plaza, 1994, 1995, 1996

   The file_date routine is mostly from GNU's fileutils package,
   written by Richard Stallman and David MacKenzie.

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

/** \file
 *  \brief Source: time formatting functions
 */

#include <config.h>

#include <stdlib.h>
#include <limits.h>             /* MB_LEN_MAX */

#include "lib/global.h"
#include "lib/strutil.h"

#include "lib/timefmt.h"

/*** global variables ****************************************************************************/

char *user_recent_timeformat = NULL;    /* time format string for recent dates */
char *user_old_timeformat = NULL;       /* time format string for older dates */

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/*
 * Cache variable for the i18n_checktimelength function,
 * initially set to a clearly invalid value to show that
 * it hasn't been initialized yet.
 */
static size_t i18n_timelength_cache = MAX_I18NTIMELENGTH + 1;

/*** file scope functions ************************************************************************/

/*** public functions ****************************************************************************/

/* --------------------------------------------------------------------------------------------- */
/**
 * Check strftime() results. Some systems (i.e. Solaris) have different
 *  short-month and month name sizes for different locales
 */
size_t
i18n_checktimelength (void)
{
    size_t length = 0;
    time_t testtime;
    struct tm *lt;

    if (i18n_timelength_cache <= MAX_I18NTIMELENGTH)
        return i18n_timelength_cache;

    testtime = time (NULL);
    lt = localtime (&testtime);

    if (lt == NULL)
    {
        /* huh, localtime() doesnt seem to work ... falling back to "(invalid)" */
        length = str_term_width1 (_(INVALID_TIME_TEXT));
    }
    else
    {
        char buf[MB_LEN_MAX * MAX_I18NTIMELENGTH + 1];
        size_t tlen;

        /* We are interested in the longest possible date */
        lt->tm_sec = lt->tm_min = lt->tm_hour = lt->tm_mday = 10;

        /* Loop through all months to find out the longest one */
        for (lt->tm_mon = 0; lt->tm_mon < 12; lt->tm_mon++)
        {
            strftime (buf, sizeof (buf) - 1, user_recent_timeformat, lt);
            tlen = (size_t) str_term_width1 (buf);
            length = MAX (tlen, length);
            strftime (buf, sizeof (buf) - 1, user_old_timeformat, lt);
            tlen = (size_t) str_term_width1 (buf);
            length = MAX (tlen, length);
        }

        tlen = (size_t) str_term_width1 (_(INVALID_TIME_TEXT));
        length = MAX (tlen, length);
    }

    /* Don't handle big differences. Use standard value (email bug, please) */
    if (length > MAX_I18NTIMELENGTH || length < MIN_I18NTIMELENGTH)
        length = STD_I18NTIMELENGTH;

    /* Save obtained value to the cache */
    i18n_timelength_cache = length;

    return i18n_timelength_cache;
}

/* --------------------------------------------------------------------------------------------- */

const char *
file_date (time_t when)
{
    static char timebuf[MB_LEN_MAX * MAX_I18NTIMELENGTH + 1];
    time_t current_time = time (NULL);
    const char *fmt;

    if (current_time > when + 6L * 30L * 24L * 60L * 60L        /* Old. */
        || current_time < when - 60L * 60L)     /* In the future. */
        /* The file is fairly old or in the future.
           POSIX says the cutoff is 6 months old;
           approximate this by 6*30 days.
           Allow a 1 hour slop factor for what is considered "the future",
           to allow for NFS server/client clock disagreement.
           Show the year instead of the time of day.  */

        fmt = user_old_timeformat;
    else
        fmt = user_recent_timeformat;

    FMT_LOCALTIME (timebuf, sizeof (timebuf), fmt, when);

    return timebuf;
}

/* --------------------------------------------------------------------------------------------- */
