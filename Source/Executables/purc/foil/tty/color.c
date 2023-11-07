/*
   Color setup.
   Interface functions.

   Copyright (C) 1994-2021
   Free Software Foundation, Inc.

   Written by:
   Andrew Borodin <aborodin@vmail.ru>, 2009
   Slava Zanko <slavazanko@gmail.com>, 2009
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

/** \file color.c
 *  \brief Source: color setup
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>          /* size_t */

#include "../screen.h"

#include "tty.h"
#include "color.h"

#include "color-internal.h"

/*** global variables ****************************************************************************/

static char *tty_color_defaults__fg = NULL;
static char *tty_color_defaults__bg = NULL;
static char *tty_color_defaults__attrs = NULL;

/* Set if we are actually using colors */
gboolean use_colors = FALSE;

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

static GHashTable *mc_tty_color__hashtable = NULL;

/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

static gboolean
tty_color_free_condition_cb (gpointer key, gpointer value, gpointer user_data)
{
    gboolean is_temp_color;
    tty_color_pair_t *mc_color_pair;
    (void) key;

    is_temp_color = user_data != NULL;
    mc_color_pair = (tty_color_pair_t *) value;
    return (mc_color_pair->is_temp == is_temp_color);
}

/* --------------------------------------------------------------------------------------------- */

static void
tty_color_free_all (gboolean is_temp_color)
{
    g_hash_table_foreach_remove (mc_tty_color__hashtable,
            (GHRFunc)(void *)tty_color_free_condition_cb,
            is_temp_color ? GSIZE_TO_POINTER (1) : NULL);
}

/* --------------------------------------------------------------------------------------------- */

static gboolean
tty_color_get_next_cpn_cb (gpointer key, gpointer value, gpointer user_data)
{
    size_t cp;
    tty_color_pair_t *mc_color_pair;
    (void) key;

    cp = GPOINTER_TO_SIZE (user_data);
    mc_color_pair = (tty_color_pair_t *) value;

    return (cp == mc_color_pair->pair_index);
}

/* --------------------------------------------------------------------------------------------- */

static size_t
tty_color_get_next__color_pair_number (void)
{
    size_t cp_count, cp;

    cp_count = g_hash_table_size (mc_tty_color__hashtable);
    for (cp = 0; cp < cp_count; cp++)
        if (g_hash_table_find (mc_tty_color__hashtable,
                    (GHRFunc)(void *)tty_color_get_next_cpn_cb,
                    GSIZE_TO_POINTER (cp)) == NULL)
            break;

    return cp;
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

int
tty_init_colors (gboolean disable, gboolean force)
{
    int colors = tty_color_init_lib (disable, force);
    mc_tty_color__hashtable = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
    return colors;
}

/* --------------------------------------------------------------------------------------------- */

void
tty_colors_done (void)
{
    tty_color_deinit_lib ();
    g_free (tty_color_defaults__fg);
    g_free (tty_color_defaults__bg);
    g_free (tty_color_defaults__attrs);

    g_hash_table_destroy (mc_tty_color__hashtable);
}

/* --------------------------------------------------------------------------------------------- */

gboolean
tty_use_colors (void)
{
    return use_colors;
}

/* --------------------------------------------------------------------------------------------- */

int
tty_try_alloc_color_pair2 (const char *fg, const char *bg, const char *attrs,
                           gboolean is_temp_color)
{
    gchar *color_pair;
    tty_color_pair_t *mc_color_pair;
    int ifg, ibg, attr;

    if (fg == NULL || !strcmp (fg, "base"))
        fg = tty_color_defaults__fg;
    if (bg == NULL || !strcmp (bg, "base"))
        bg = tty_color_defaults__bg;
    if (attrs == NULL || !strcmp (attrs, "base"))
        attrs = tty_color_defaults__attrs;

    ifg = tty_color_get_index_by_name (fg);
    ibg = tty_color_get_index_by_name (bg);
    attr = tty_attr_get_bits (attrs);

    color_pair = g_strdup_printf ("%d.%d.%d", ifg, ibg, attr);
    if (color_pair == NULL)
        return 0;

    mc_color_pair =
        (tty_color_pair_t *) g_hash_table_lookup (mc_tty_color__hashtable, (gpointer) color_pair);

    if (mc_color_pair != NULL)
    {
        g_free (color_pair);
        return mc_color_pair->pair_index;
    }

    mc_color_pair = g_try_new0 (tty_color_pair_t, 1);
    if (mc_color_pair == NULL)
    {
        g_free (color_pair);
        return 0;
    }

    mc_color_pair->is_temp = is_temp_color;
    mc_color_pair->ifg = ifg;
    mc_color_pair->ibg = ibg;
    mc_color_pair->attr = attr;
    mc_color_pair->pair_index = tty_color_get_next__color_pair_number ();

    tty_color_try_alloc_pair_lib (mc_color_pair);

    g_hash_table_insert (mc_tty_color__hashtable, (gpointer) color_pair, (gpointer) mc_color_pair);

    return mc_color_pair->pair_index;
}

/* --------------------------------------------------------------------------------------------- */

int
tty_try_alloc_color_pair (const char *fg, const char *bg, const char *attrs)
{
    return tty_try_alloc_color_pair2 (fg, bg, attrs, TRUE);
}

/* --------------------------------------------------------------------------------------------- */

void
tty_color_free_all_tmp (void)
{
    tty_color_free_all (TRUE);
}

/* --------------------------------------------------------------------------------------------- */

void
tty_color_free_all_non_tmp (void)
{
    tty_color_free_all (FALSE);
}

/* --------------------------------------------------------------------------------------------- */

void
tty_color_set_defaults (const char *fgcolor, const char *bgcolor, const char *attrs)
{
    g_free (tty_color_defaults__fg);
    g_free (tty_color_defaults__bg);
    g_free (tty_color_defaults__attrs);

    tty_color_defaults__fg = g_strdup (fgcolor);
    tty_color_defaults__bg = g_strdup (bgcolor);
    tty_color_defaults__attrs = g_strdup (attrs);
}

/* --------------------------------------------------------------------------------------------- */
