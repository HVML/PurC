/* A more useful interface to strtol.

   Copyright (C) 1995-2021
   Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Written by Jim Meyering. */

#include <config.h>

/* Some pre-ANSI implementations (e.g. SunOS 4)
   need stderr defined if assertion checking is enabled.  */
#include <stdio.h>

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "strutil.h"

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/* --------------------------------------------------------------------------------------------- */
/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

static strtol_error_t
bkm_scale (uintmax_t * x, int scale_factor)
{
    if (UINTMAX_MAX / scale_factor < *x)
    {
        *x = UINTMAX_MAX;
        return LONGINT_OVERFLOW;
    }

    *x *= scale_factor;
    return LONGINT_OK;
}

/* --------------------------------------------------------------------------------------------- */

static strtol_error_t
bkm_scale_by_power (uintmax_t * x, int base, int power)
{
    strtol_error_t err = LONGINT_OK;
    while (power-- != 0)
        err |= bkm_scale (x, base);
    return err;
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

strtol_error_t
xstrtoumax (const char *s, char **ptr, int base, uintmax_t * val, const char *valid_suffixes)
{
    char *t_ptr;
    char **p;
    uintmax_t tmp;
    strtol_error_t err = LONGINT_OK;

    g_assert (0 <= base && base <= 36);

    p = (ptr != NULL ? ptr : &t_ptr);

    {
        const char *q = s;
        unsigned char ch = *q;

        while (isspace (ch))
            ch = *++q;

        if (ch == '-')
            return LONGINT_INVALID;
    }

    errno = 0;
    tmp = strtol (s, p, base);

    if (*p == s)
    {
        /* If there is no number but there is a valid suffix, assume the
           number is 1.  The string is invalid otherwise.  */
        if (valid_suffixes != NULL && **p != '\0' && strchr (valid_suffixes, **p) != NULL)
            tmp = 1;
        else
            return LONGINT_INVALID;
    }
    else if (errno != 0)
    {
        if (errno != ERANGE)
            return LONGINT_INVALID;
        err = LONGINT_OVERFLOW;
    }

    /* Let valid_suffixes == NULL mean "allow any suffix".  */
    /* FIXME: update all callers except the ones that allow suffixes
       after the number, changing last parameter NULL to "".  */
    if (valid_suffixes == NULL)
    {
        *val = tmp;
        return err;
    }

    if (**p != '\0')
    {
        int suffixes = 1;
        strtol_error_t overflow;

        if (strchr (valid_suffixes, **p) == NULL)
        {
            *val = tmp;
            return err | LONGINT_INVALID_SUFFIX_CHAR;
        }

        base = 1024;

        switch (**p)
        {
        case 'E':
        case 'G':
        case 'g':
        case 'k':
        case 'K':
        case 'M':
        case 'm':
        case 'P':
        case 'T':
        case 't':
        case 'Y':
        case 'Z':
            if (strchr (valid_suffixes, '0') != NULL)
            {
                /* The "valid suffix" '0' is a special flag meaning that
                   an optional second suffix is allowed, which can change
                   the base.  A suffix "B" (e.g. "100MB") stands for a power
                   of 1000, whereas a suffix "iB" (e.g. "100MiB") stands for
                   a power of 1024.  If no suffix (e.g. "100M"), assume
                   power-of-1024.  */

                switch (p[0][1])
                {
                case 'i':
                    if (p[0][2] == 'B')
                        suffixes += 2;
                    break;

                case 'B':
                case 'D':      /* 'D' is obsolescent */
                    base = 1000;
                    suffixes++;
                    break;
                default:
                    break;
                }
            }
            break;
        default:
            break;
        }

        switch (**p)
        {
        case 'b':
            overflow = bkm_scale (&tmp, 512);
            break;

        case 'B':
            /* This obsolescent first suffix is distinct from the 'B'
               second suffix above.  E.g., 'tar -L 1000B' means change
               the tape after writing 1000 KiB of data.  */
            overflow = bkm_scale (&tmp, 1024);
            break;

        case 'c':
            overflow = LONGINT_OK;
            break;

        case 'E':              /* exa or exbi */
            overflow = bkm_scale_by_power (&tmp, base, 6);
            break;

        case 'G':              /* giga or gibi */
        case 'g':              /* 'g' is undocumented; for compatibility only */
            overflow = bkm_scale_by_power (&tmp, base, 3);
            break;

        case 'k':              /* kilo */
        case 'K':              /* kibi */
            overflow = bkm_scale_by_power (&tmp, base, 1);
            break;

        case 'M':              /* mega or mebi */
        case 'm':              /* 'm' is undocumented; for compatibility only */
            overflow = bkm_scale_by_power (&tmp, base, 2);
            break;

        case 'P':              /* peta or pebi */
            overflow = bkm_scale_by_power (&tmp, base, 5);
            break;

        case 'T':              /* tera or tebi */
        case 't':              /* 't' is undocumented; for compatibility only */
            overflow = bkm_scale_by_power (&tmp, base, 4);
            break;

        case 'w':
            overflow = bkm_scale (&tmp, 2);
            break;

        case 'Y':              /* yotta or 2**80 */
            overflow = bkm_scale_by_power (&tmp, base, 8);
            break;

        case 'Z':              /* zetta or 2**70 */
            overflow = bkm_scale_by_power (&tmp, base, 7);
            break;

        default:
            *val = tmp;
            return err | LONGINT_INVALID_SUFFIX_CHAR;
        }

        err |= overflow;
        *p += suffixes;
        if (**p != '\0')
            err |= LONGINT_INVALID_SUFFIX_CHAR;
    }

    *val = tmp;
    return err;
}

/* --------------------------------------------------------------------------------------------- */
