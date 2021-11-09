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

/* --------------------------------------------------------------------------------------------- */
/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

static gchar *
mc_config_normalize_before_save (const gchar * value)
{
    GIConv conv;
    GString *buffer;
    gboolean ok;

    if (mc_global.utf8_display)
        return g_strdup (value);

    conv = str_crt_conv_to ("UTF-8");
    if (conv == INVALID_CONV)
        return g_strdup (value);

    buffer = g_string_new ("");

    ok = (str_convert (conv, value, buffer) != ESTR_FAILURE);
    str_close_conv (conv);

    if (!ok)
    {
        g_string_free (buffer, TRUE);
        return g_strdup (value);
    }

    return g_string_free (buffer, FALSE);
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

void
mc_config_set_string_raw (mc_config_t * mc_config, const gchar * group,
                          const gchar * param, const gchar * value)
{
    if (mc_config != NULL && group != NULL && param != NULL && value != NULL)
        g_key_file_set_string (mc_config->handle, group, param, value);
}

/* --------------------------------------------------------------------------------------------- */

void
mc_config_set_string_raw_value (mc_config_t * mc_config, const gchar * group,
                                const gchar * param, const gchar * value)
{
    if (mc_config != NULL && group != NULL && param != NULL && value != NULL)
        g_key_file_set_value (mc_config->handle, group, param, value);
}

/* --------------------------------------------------------------------------------------------- */

void
mc_config_set_string (mc_config_t * mc_config, const gchar * group,
                      const gchar * param, const gchar * value)
{
    if (mc_config != NULL && group != NULL && param != NULL && value != NULL)
    {
        gchar *buffer;

        buffer = mc_config_normalize_before_save (value);
        g_key_file_set_string (mc_config->handle, group, param, buffer);
        g_free (buffer);
    }
}

/* --------------------------------------------------------------------------------------------- */

void
mc_config_set_bool (mc_config_t * mc_config, const gchar * group,
                    const gchar * param, gboolean value)
{
    if (mc_config != NULL && group != NULL && param != NULL)
        g_key_file_set_boolean (mc_config->handle, group, param, value);
}

/* --------------------------------------------------------------------------------------------- */

void
mc_config_set_int (mc_config_t * mc_config, const gchar * group, const gchar * param, int value)
{
    if (mc_config != NULL && group != NULL && param != NULL)
        g_key_file_set_integer (mc_config->handle, group, param, value);
}

/* --------------------------------------------------------------------------------------------- */

void
mc_config_set_string_list (mc_config_t * mc_config, const gchar * group,
                           const gchar * param, const gchar * const value[], gsize length)
{
    if (mc_config != NULL && group != NULL && param != NULL && value != NULL && length != 0)
        g_key_file_set_string_list (mc_config->handle, group, param, value, length);
}

/* --------------------------------------------------------------------------------------------- */

void
mc_config_set_bool_list (mc_config_t * mc_config, const gchar * group,
                         const gchar * param, gboolean value[], gsize length)
{
    if (mc_config != NULL && group != NULL && param != NULL && value != NULL && length != 0)
        g_key_file_set_boolean_list (mc_config->handle, group, param, value, length);
}

/* --------------------------------------------------------------------------------------------- */

void
mc_config_set_int_list (mc_config_t * mc_config, const gchar * group,
                        const gchar * param, int value[], gsize length)
{
    if (mc_config != NULL && group != NULL && param != NULL && value != NULL && length != 0)
        g_key_file_set_integer_list (mc_config->handle, group, param, value, length);
}

/* --------------------------------------------------------------------------------------------- */
