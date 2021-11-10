/*
   Widgets for the Midnight Commander

   Copyright (C) 1994-2021
   Free Software Foundation, Inc.

   Authors:
   Radek Doulik, 1994, 1995
   Miguel de Icaza, 1994, 1995
   Jakub Jelinek, 1995
   Andrej Borsenkow, 1996
   Norbert Warmuth, 1997
   Andrew Borodin <aborodin@vmail.ru>, 2009-2019

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

/** \file history.c
 *  \brief Source: save and load history
 */

#include <config.h>

#include <stdlib.h>
#include <sys/types.h>

#include "lib/global.h"

#include "lib/fileloc.h"        /* MC_HISTORY_FILE */
#include "lib/strutil.h"
#include "lib/util.h"           /* list_append_unique */

#include "lib/mcconfig.h"

/*** global variables ****************************************************************************/

/* how much history items are used */
int num_history_items_recorded = 60;

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/* --------------------------------------------------------------------------------------------- */
/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

/**
 * Load the history from the ${XDG_CACHE_HOME}/mc/history file.
 * It is called with the widgets history name and returns the GList list.
 */

GList *
mc_config_history_get (const char *name)
{
    GList *hist = NULL;
    char *profile;
    mc_config_t *cfg;

    if (num_history_items_recorded == 0)        /* this is how to disable */
        return NULL;
    if (name == NULL || *name == '\0')
        return NULL;

    profile = mc_config_get_full_path (MC_HISTORY_FILE);
    cfg = mc_config_init (profile, TRUE);

    hist = mc_config_history_load (cfg, name);

    mc_config_deinit (cfg);
    g_free (profile);

    return hist;
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Load history from the mc_config
 */
GList *
mc_config_history_load (mc_config_t * cfg, const char *name)
{
    size_t i;
    GList *hist = NULL;
    char **keys;
    size_t keys_num = 0;
    GIConv conv = INVALID_CONV;
    GString *buffer;

    if (name == NULL || *name == '\0')
        return NULL;

    /* get number of keys */
    keys = mc_config_get_keys (cfg, name, &keys_num);
    g_strfreev (keys);

    /* create charset conversion handler to convert strings
       from utf-8 to system codepage */
    if (!mc_global.utf8_display)
        conv = str_crt_conv_from ("UTF-8");

    buffer = g_string_sized_new (64);

    for (i = 0; i < keys_num; i++)
    {
        char key[BUF_TINY];
        char *this_entry;

        g_snprintf (key, sizeof (key), "%lu", (unsigned long) i);
        this_entry = mc_config_get_string_raw (cfg, name, key, "");

        if (this_entry == NULL)
            continue;

        if (conv == INVALID_CONV)
            hist = list_append_unique (hist, this_entry);
        else
        {
            g_string_set_size (buffer, 0);
            if (str_convert (conv, this_entry, buffer) == ESTR_FAILURE)
                hist = list_append_unique (hist, this_entry);
            else
            {
                hist = list_append_unique (hist, g_strndup (buffer->str, buffer->len));
                g_free (this_entry);
            }
        }
    }

    g_string_free (buffer, TRUE);
    if (conv != INVALID_CONV)
        str_close_conv (conv);

    /* return pointer to the last entry in the list */
    return g_list_last (hist);
}

/* --------------------------------------------------------------------------------------------- */

/**
  * Save history to the mc_config, but don't save config to file
  */
void
mc_config_history_save (mc_config_t * cfg, const char *name, GList * h)
{
    GIConv conv = INVALID_CONV;
    GString *buffer;
    int i;

    if (name == NULL || *name == '\0' || h == NULL)
        return;

    /* go to end of list */
    h = g_list_last (h);

    /* go back 60 places */
    for (i = 0; (i < num_history_items_recorded - 1) && (h->prev != NULL); i++)
        h = g_list_previous (h);

    if (name != NULL)
        mc_config_del_group (cfg, name);

    /* create charset conversion handler to convert strings
       from system codepage to UTF-8 */
    if (!mc_global.utf8_display)
        conv = str_crt_conv_to ("UTF-8");

    buffer = g_string_sized_new (64);

    /* dump history into profile */
    for (i = 0; h != NULL; h = g_list_next (h))
    {
        char key[BUF_TINY];
        char *text = (char *) h->data;

        /* We shouldn't have null entries, but let's be sure */
        if (text == NULL)
            continue;

        g_snprintf (key, sizeof (key), "%d", i++);

        if (conv == INVALID_CONV)
            mc_config_set_string_raw (cfg, name, key, text);
        else
        {
            g_string_set_size (buffer, 0);
            if (str_convert (conv, text, buffer) == ESTR_FAILURE)
                mc_config_set_string_raw (cfg, name, key, text);
            else
                mc_config_set_string_raw (cfg, name, key, buffer->str);
        }
    }

    g_string_free (buffer, TRUE);
    if (conv != INVALID_CONV)
        str_close_conv (conv);
}

/* --------------------------------------------------------------------------------------------- */
