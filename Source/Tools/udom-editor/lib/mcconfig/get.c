/*
   Configure module for the Midnight Commander

   Copyright (C) 1994-2021
   Free Software Foundation, Inc.

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

#include "lib/global.h"
#include "lib/strutil.h"

#include "lib/mcconfig.h"

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/*** file scope functions ************************************************************************/

/* --------------------------------------------------------------------------------------------- */
/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

gchar **
mc_config_get_groups (const mc_config_t * mc_config, gsize * len)
{
    gchar **ret = NULL;

    if (mc_config != NULL)
        ret = g_key_file_get_groups (mc_config->handle, len);

    if (ret == NULL)
    {
        ret = g_try_malloc0 (sizeof (gchar **));
        if (len != NULL)
            *len = 0;
    }

    return ret;
}

/* --------------------------------------------------------------------------------------------- */

gchar **
mc_config_get_keys (const mc_config_t * mc_config, const gchar * group, gsize * len)
{
    gchar **ret = NULL;

    if (mc_config != NULL && group != NULL)
        ret = g_key_file_get_keys (mc_config->handle, group, len, NULL);

    if (ret == NULL)
    {
        ret = g_try_malloc0 (sizeof (gchar **));
        if (len != NULL)
            *len = 0;
    }

    return ret;
}

/* --------------------------------------------------------------------------------------------- */

gchar *
mc_config_get_string (mc_config_t * mc_config, const gchar * group,
                      const gchar * param, const gchar * def)
{
    GIConv conv;
    GString *buffer;
    gchar *ret;
    estr_t conv_res;

    if (mc_config == NULL || group == NULL || param == NULL)
        return g_strdup (def);

    if (!mc_config_has_param (mc_config, group, param))
    {
        if (def != NULL)
            mc_config_set_string (mc_config, group, param, def);
        return g_strdup (def);
    }

    ret = g_key_file_get_string (mc_config->handle, group, param, NULL);
    if (ret == NULL)
        ret = g_strdup (def);

    if (mc_global.utf8_display)
        return ret;

    conv = str_crt_conv_from ("UTF-8");
    if (conv == INVALID_CONV)
        return ret;

    buffer = g_string_new ("");
    conv_res = str_convert (conv, ret, buffer);
    str_close_conv (conv);

    if (conv_res == ESTR_FAILURE)
    {
        g_string_free (buffer, TRUE);
        return ret;
    }

    g_free (ret);

    return g_string_free (buffer, FALSE);
}

/* --------------------------------------------------------------------------------------------- */

gchar *
mc_config_get_string_raw (mc_config_t * mc_config, const gchar * group,
                          const gchar * param, const gchar * def)
{
    gchar *ret;

    if (mc_config == NULL || group == NULL || param == NULL)
        return g_strdup (def);

    if (!mc_config_has_param (mc_config, group, param))
    {
        if (def != NULL)
            mc_config_set_string (mc_config, group, param, def);
        return g_strdup (def);
    }

    ret = g_key_file_get_string (mc_config->handle, group, param, NULL);

    return ret != NULL ? ret : g_strdup (def);
}

/* --------------------------------------------------------------------------------------------- */

gboolean
mc_config_get_bool (mc_config_t * mc_config, const gchar * group, const gchar * param, gboolean def)
{
    if (mc_config == NULL || group == NULL || param == NULL)
        return def;

    if (!mc_config_has_param (mc_config, group, param))
    {
        mc_config_set_bool (mc_config, group, param, def);
        return def;
    }

    return g_key_file_get_boolean (mc_config->handle, group, param, NULL);
}

/* --------------------------------------------------------------------------------------------- */

int
mc_config_get_int (mc_config_t * mc_config, const gchar * group, const gchar * param, int def)
{
    if (mc_config == NULL || group == NULL || param == NULL)
        return def;

    if (!mc_config_has_param (mc_config, group, param))
    {
        mc_config_set_int (mc_config, group, param, def);
        return def;
    }

    return g_key_file_get_integer (mc_config->handle, group, param, NULL);
}

/* --------------------------------------------------------------------------------------------- */

gchar **
mc_config_get_string_list (mc_config_t * mc_config, const gchar * group,
                           const gchar * param, gsize * length)
{
    if (mc_config == NULL || group == NULL || param == NULL)
        return NULL;

    return g_key_file_get_string_list (mc_config->handle, group, param, length, NULL);
}

/* --------------------------------------------------------------------------------------------- */

gboolean *
mc_config_get_bool_list (mc_config_t * mc_config, const gchar * group,
                         const gchar * param, gsize * length)
{
    if (mc_config == NULL || group == NULL || param == NULL)
        return NULL;

    return g_key_file_get_boolean_list (mc_config->handle, group, param, length, NULL);
}

/* --------------------------------------------------------------------------------------------- */

int *
mc_config_get_int_list (mc_config_t * mc_config, const gchar * group,
                        const gchar * param, gsize * length)
{
    if (mc_config == NULL || group == NULL || param == NULL)
        return NULL;

    return g_key_file_get_integer_list (mc_config->handle, group, param, length, NULL);
}

/* --------------------------------------------------------------------------------------------- */
