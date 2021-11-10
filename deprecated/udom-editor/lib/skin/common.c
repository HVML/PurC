/*
   Skins engine.
   Interface functions

   Copyright (C) 2009-2021
   Free Software Foundation, Inc.

   Written by:
   Slava Zanko <slavazanko@gmail.com>, 2009
   Egmont Koblinger <egmont@gmail.com>, 2010

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

#include <config.h>
#include <stdlib.h>

#include "internal.h"
#include "lib/util.h"

#include "lib/tty/color.h"      /* tty_use_256colors(); */

/*** global variables ****************************************************************************/

mc_skin_t mc_skin__default;

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

static gboolean mc_skin_is_init = FALSE;

/* --------------------------------------------------------------------------------------------- */
/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

static void
mc_skin_hash_destroy_value (gpointer data)
{
    mc_skin_color_t *mc_skin_color = (mc_skin_color_t *) data;
    g_free (mc_skin_color->fgcolor);
    g_free (mc_skin_color->bgcolor);
    g_free (mc_skin_color->attrs);
    g_free (mc_skin_color);
}

/* --------------------------------------------------------------------------------------------- */

static char *
mc_skin_get_default_name (void)
{
    char *tmp_str;

    /* from command line */
    if (mc_global.tty.skin != NULL)
        return g_strdup (mc_global.tty.skin);

    /* from envirovement variable */
    tmp_str = getenv ("MC_SKIN");
    if (tmp_str != NULL)
        return g_strdup (tmp_str);

    /*  from config. Or 'default' if no present in config */
    return mc_config_get_string (mc_global.main_config, CONFIG_APP_SECTION, "skin", "default");
}

/* --------------------------------------------------------------------------------------------- */

static void
mc_skin_reinit (void)
{
    mc_skin_deinit ();
    mc_skin__default.name = mc_skin_get_default_name ();
    mc_skin__default.colors = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                     g_free, mc_skin_hash_destroy_value);
}

/* --------------------------------------------------------------------------------------------- */

static void
mc_skin_try_to_load_default (void)
{
    mc_skin_reinit ();
    g_free (mc_skin__default.name);
    mc_skin__default.name = g_strdup ("default");
    if (!mc_skin_ini_file_load (&mc_skin__default))
    {
        mc_skin_reinit ();
        mc_skin_set_hardcoded_skin (&mc_skin__default);
    }

}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

gboolean
mc_skin_init (const gchar * skin_override, GError ** mcerror)
{
    gboolean is_good_init = TRUE;
    GError *error = NULL;

    mc_return_val_if_error (mcerror, FALSE);

    mc_skin__default.have_256_colors = FALSE;
    mc_skin__default.have_true_colors = FALSE;

    mc_skin__default.name =
        skin_override != NULL ? g_strdup (skin_override) : mc_skin_get_default_name ();

    mc_skin__default.colors = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                     g_free, mc_skin_hash_destroy_value);
    if (!mc_skin_ini_file_load (&mc_skin__default))
    {
        mc_propagate_error (mcerror, 0,
                            _("Unable to load '%s' skin.\nDefault skin has been loaded"),
                            mc_skin__default.name);
        mc_skin_try_to_load_default ();
        is_good_init = FALSE;
    }
    mc_skin_colors_old_configure (&mc_skin__default);

    if (!mc_skin_ini_file_parse (&mc_skin__default))
    {
        mc_propagate_error (mcerror, 0,
                            _("Unable to parse '%s' skin.\nDefault skin has been loaded"),
                            mc_skin__default.name);

        mc_skin_try_to_load_default ();
        mc_skin_colors_old_configure (&mc_skin__default);
        (void) mc_skin_ini_file_parse (&mc_skin__default);
        is_good_init = FALSE;
    }
    if (is_good_init && mc_skin__default.have_true_colors && !tty_use_truecolors (&error))
    {
        mc_propagate_error (mcerror, 0,
                            _
                            ("Unable to use '%s' skin with true colors support:\n%s\nDefault skin has been loaded"),
                            mc_skin__default.name, error->message);
        g_error_free (error);
        mc_skin_try_to_load_default ();
        mc_skin_colors_old_configure (&mc_skin__default);
        (void) mc_skin_ini_file_parse (&mc_skin__default);
        is_good_init = FALSE;
    }
    if (is_good_init && mc_skin__default.have_256_colors && !tty_use_256colors (&error))
    {
        mc_propagate_error (mcerror, 0,
                            _
                            ("Unable to use '%s' skin with 256 colors support\non non-256 colors terminal.\nDefault skin has been loaded"),
                            mc_skin__default.name);
        mc_skin_try_to_load_default ();
        mc_skin_colors_old_configure (&mc_skin__default);
        (void) mc_skin_ini_file_parse (&mc_skin__default);
        is_good_init = FALSE;
    }
    mc_skin_is_init = TRUE;
    return is_good_init;
}

/* --------------------------------------------------------------------------------------------- */

void
mc_skin_deinit (void)
{
    tty_color_free_all_tmp ();
    tty_color_free_all_non_tmp ();

    MC_PTR_FREE (mc_skin__default.name);
    g_hash_table_destroy (mc_skin__default.colors);
    mc_skin__default.colors = NULL;

    MC_PTR_FREE (mc_skin__default.description);

    mc_config_deinit (mc_skin__default.config);
    mc_skin__default.config = NULL;

    mc_skin_is_init = FALSE;
}

/* --------------------------------------------------------------------------------------------- */

gchar *
mc_skin_get (const gchar * group, const gchar * key, const gchar * default_value)
{
    if (mc_global.tty.ugly_line_drawing)
    {
        return g_strdup (default_value);
    }
    return mc_config_get_string (mc_skin__default.config, group, key, default_value);
}

/* --------------------------------------------------------------------------------------------- */
