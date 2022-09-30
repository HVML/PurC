/*
   Color setup for NCurses screen library

   Copyright (C) 1994-2021
   Free Software Foundation, Inc.

   Written by:
   Andrew Borodin <aborodin@vmail.ru>, 2009
   Slava Zanko <slavazanko@gmail.com>, 2010
   Egmont Koblinger <egmont@gmail.com>, 2010

   This file is part of purc, an HVML interperter with CLI.

   Purc is free software: you can redistribute it
   and/or modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, either version 3 of the License,
   or (at your option) any later version.

   Purc is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/** \file color-ncurses.c
 *  \brief Source: NCUrses-specific color setup
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>          /* size_t */
#include <glib.h>

#include "global.h"

#include "tty-ncurses.h"
#include "color.h"              /* variables */
#include "color-internal.h"

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

static GHashTable *mc_tty_color_color_pair_attrs = NULL;

/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

static inline void
mc_tty_color_attr_destroy_cb (gpointer data)
{
    g_free (data);
}

/* --------------------------------------------------------------------------------------------- */

static void
mc_tty_color_save_attr (int color_pair, int color_attr)
{
    int *attr, *key;

    attr = g_try_new0 (int, 1);
    if (attr == NULL)
        return;

    key = g_try_new (int, 1);
    if (key == NULL)
    {
        g_free (attr);
        return;
    }

    *key = color_pair;
    *attr = color_attr;

    g_hash_table_replace (mc_tty_color_color_pair_attrs, (gpointer) key, (gpointer) attr);
}

/* --------------------------------------------------------------------------------------------- */

static int
color_get_attr (int color_pair)
{
    int *fnd = NULL;

    if (mc_tty_color_color_pair_attrs != NULL)
        fnd = (int *) g_hash_table_lookup (mc_tty_color_color_pair_attrs, (gpointer) & color_pair);
    return (fnd != NULL) ? *fnd : 0;
}

/* --------------------------------------------------------------------------------------------- */

static void
mc_tty_color_pair_init_special (tty_color_pair_t * mc_color_pair,
                                int fg1, int bg1, int fg2, int bg2, int attr)
{
    if (has_colors () && !mc_tty_color_disable)
        init_pair (mc_color_pair->pair_index, fg1, bg1);
    else
        init_pair (mc_color_pair->pair_index, fg2, bg2);
    mc_tty_color_save_attr (mc_color_pair->pair_index, attr);
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

void
tty_color_init_lib (bool disable, bool force)
{
    (void) force;

    if (has_colors () && !disable)
    {
        use_colors = TRUE;
        start_color ();
        use_default_colors ();
    }

    mc_tty_color_color_pair_attrs = g_hash_table_new_full
        (g_int_hash, g_int_equal, mc_tty_color_attr_destroy_cb, mc_tty_color_attr_destroy_cb);
}

/* --------------------------------------------------------------------------------------------- */

void
tty_color_deinit_lib (void)
{
    g_hash_table_destroy (mc_tty_color_color_pair_attrs);
    mc_tty_color_color_pair_attrs = NULL;
}

/* --------------------------------------------------------------------------------------------- */

void
tty_color_try_alloc_pair_lib (tty_color_pair_t * mc_color_pair)
{
    if (mc_color_pair->ifg <= (int) SPEC_A_REVERSE)
    {
        switch (mc_color_pair->ifg)
        {
        case SPEC_A_REVERSE:
            mc_tty_color_pair_init_special (mc_color_pair,
                                            COLOR_BLACK, COLOR_WHITE,
                                            COLOR_BLACK, COLOR_WHITE | A_BOLD, A_REVERSE);
            break;
        case SPEC_A_BOLD:
            mc_tty_color_pair_init_special (mc_color_pair,
                                            COLOR_WHITE, COLOR_BLACK,
                                            COLOR_WHITE, COLOR_BLACK, A_BOLD);
            break;
        case SPEC_A_BOLD_REVERSE:
            mc_tty_color_pair_init_special (mc_color_pair,
                                            COLOR_WHITE, COLOR_WHITE,
                                            COLOR_WHITE, COLOR_WHITE, A_BOLD | A_REVERSE);
            break;
        case SPEC_A_UNDERLINE:
            mc_tty_color_pair_init_special (mc_color_pair,
                                            COLOR_WHITE, COLOR_BLACK,
                                            COLOR_WHITE, COLOR_BLACK, A_UNDERLINE);
            break;
        default:
            break;
        }
    }
    else
    {
        int ifg, ibg, attr;

        ifg = mc_color_pair->ifg;
        ibg = mc_color_pair->ibg;
        attr = mc_color_pair->attr;

        /* In legacy color mode, change bright colors into bold */
        if (!tty_use_256colors (NULL) && !tty_use_truecolors (NULL))
        {
            if (ifg >= 8 && ifg < 16)
            {
                ifg &= 0x07;
                attr |= A_BOLD;
            }

            if (ibg >= 8 && ibg < 16)
            {
                ibg &= 0x07;
                /* attr | = A_BOLD | A_REVERSE ; */
            }
        }

        init_pair (mc_color_pair->pair_index, ifg, ibg);
        mc_tty_color_save_attr (mc_color_pair->pair_index, attr);
    }
}

/* --------------------------------------------------------------------------------------------- */

void
tty_setcolor (int color)
{
    attrset (COLOR_PAIR (color) | color_get_attr (color));
}

/* --------------------------------------------------------------------------------------------- */

void
tty_lowlevel_setcolor (int color)
{
    tty_setcolor (color);
}

/* --------------------------------------------------------------------------------------------- */

void
tty_set_normal_attrs (void)
{
    standend ();
}

/* --------------------------------------------------------------------------------------------- */

bool
tty_use_256colors (GError ** error)
{
    (void) error;

    return (COLORS == 256);
}

/* --------------------------------------------------------------------------------------------- */

bool
tty_use_truecolors (GError ** error)
{
    /* Not yet supported in ncurses */
    g_set_error (error, MC_ERROR, -1, _("True color not supported with ncurses."));
    return FALSE;
}

/* --------------------------------------------------------------------------------------------- */
