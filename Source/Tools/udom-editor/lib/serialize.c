/*
   Provides a serialize/unserialize functionality for INI-like formats.

   Copyright (C) 2011-2021
   Free Software Foundation, Inc.

   Written by:
   Slava Zanko <slavazanko@gmail.com>, 2011

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

/** \file lib/serialize.c
 *  \brief Source: serialize/unserialize functionality for INI-like formats.
 */

#include <config.h>

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "lib/global.h"

#include "lib/serialize.h"

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

#define SRLZ_DELIM_C ':'
#define SRLZ_DELIM_S ":"

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

static void
G_GNUC_PRINTF (2, 3)
prepend_error_message (GError ** error, const char *format, ...)
{
    char *prepend_str;
    char *split_str;
    va_list ap;

    if ((error == NULL) || (*error == NULL))
        return;

    va_start (ap, format);
    prepend_str = g_strdup_vprintf (format, ap);
    va_end (ap);

    split_str = g_strdup_printf ("%s: %s", prepend_str, (*error)->message);
    g_free (prepend_str);
    g_free ((*error)->message);
    (*error)->message = split_str;
}

/* --------------------------------------------------------------------------------------------- */

static const char *
go_to_end_of_serialized_string (const char *non_serialized_data,
                                const char *already_serialized_part, size_t * offset)
{
    size_t calculated_offset;
    const char *semi_ptr = strchr (non_serialized_data + 1, SRLZ_DELIM_C);

    calculated_offset = (semi_ptr - non_serialized_data) + 1 + strlen (already_serialized_part);
    if (calculated_offset >= strlen (non_serialized_data))
        return NULL;

    non_serialized_data += calculated_offset;
    *offset += calculated_offset;

    return non_serialized_data;
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

/**
 * Serialize some string object to string
 *
 * @param prefix prefix for serailization
 * @param data data for serialization
 * @param error contain pointer to object for handle error code and message
 *
 * @return serialized data as newly allocated string
 */

char *
mc_serialize_str (const char prefix, const char *data, GError ** error)
{
    if (data == NULL)
    {
        g_set_error (error, MC_ERROR, 0, "mc_serialize_str(): Input data is NULL.");
        return NULL;
    }
    return g_strdup_printf ("%c%zu" SRLZ_DELIM_S "%s", prefix, strlen (data), data);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Deserialize string to string object
 *
 * @param prefix prefix for deserailization
 * @param data data for deserialization
 * @param error contain pointer to object for handle error code and message
 *
 * @return newly allocated string
 */

#define FUNC_NAME "mc_serialize_str()"
char *
mc_deserialize_str (const char prefix, const char *data, GError ** error)
{
    size_t data_len;

    if ((data == NULL) || (*data == '\0'))
    {
        g_set_error (error, MC_ERROR, 0, FUNC_NAME ": Input data is NULL or empty.");
        return NULL;
    }

    if (*data != prefix)
    {
        g_set_error (error, MC_ERROR, 0, FUNC_NAME ": String prefix doesn't equal to '%c'", prefix);
        return NULL;
    }

    {
        char buffer[BUF_TINY];
        char *semi_ptr;
        size_t semi_offset;

        semi_ptr = strchr (data + 1, SRLZ_DELIM_C);
        if (semi_ptr == NULL)
        {
            g_set_error (error, MC_ERROR, 0,
                         FUNC_NAME ": Length delimiter '%c' doesn't exists", SRLZ_DELIM_C);
            return NULL;
        }
        semi_offset = semi_ptr - (data + 1);
        if (semi_offset >= BUF_TINY)
        {
            g_set_error (error, MC_ERROR, 0, FUNC_NAME ": Too big string length");
            return NULL;
        }
        strncpy (buffer, data + 1, semi_offset);
        buffer[semi_offset] = '\0';
        data_len = atol (buffer);
        data += semi_offset + 2;
    }

    if (data_len > strlen (data))
    {
        g_set_error (error, MC_ERROR, 0,
                     FUNC_NAME
                     ": Specified data length (%zu) is greater than actual data length (%zu)",
                     data_len, strlen (data));
        return NULL;
    }
    return g_strndup (data, data_len);
}

#undef FUNC_NAME

/* --------------------------------------------------------------------------------------------- */
/**
 * Serialize mc_config_t object to string
 *
 * @param data data for serialization
 * @param error contain pointer to object for handle error code and message
 *
 * @return serialized data as newly allocated string
 */

char *
mc_serialize_config (mc_config_t * data, GError ** error)
{
    gchar **groups, **group_iterator;
    GString *buffer;

    buffer = g_string_new ("");
    groups = mc_config_get_groups (data, NULL);

    for (group_iterator = groups; *group_iterator != NULL; group_iterator++)
    {
        char *serialized_str;
        gchar **params, **param_iterator;

        serialized_str = mc_serialize_str ('g', *group_iterator, error);
        if (serialized_str == NULL)
        {
            g_string_free (buffer, TRUE);
            g_strfreev (groups);
            return NULL;
        }
        g_string_append (buffer, serialized_str);
        g_free (serialized_str);

        params = mc_config_get_keys (data, *group_iterator, NULL);

        for (param_iterator = params; *param_iterator != NULL; param_iterator++)
        {
            char *value;

            serialized_str = mc_serialize_str ('p', *param_iterator, error);
            if (serialized_str == NULL)
            {
                g_string_free (buffer, TRUE);
                g_strfreev (params);
                g_strfreev (groups);
                return NULL;
            }
            g_string_append (buffer, serialized_str);
            g_free (serialized_str);

            value = mc_config_get_string_raw (data, *group_iterator, *param_iterator, "");
            serialized_str = mc_serialize_str ('v', value, error);
            g_free (value);

            if (serialized_str == NULL)
            {
                g_string_free (buffer, TRUE);
                g_strfreev (params);
                g_strfreev (groups);
                return NULL;
            }

            g_string_append (buffer, serialized_str);
            g_free (serialized_str);
        }

        g_strfreev (params);
    }

    g_strfreev (groups);

    return g_string_free (buffer, FALSE);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Deserialize string to mc_config_t object
 *
 * @param data data for serialization
 * @param error contain pointer to object for handle error code and message
 *
 * @return newly allocated mc_config_t object
 */

#define FUNC_NAME "mc_deserialize_config()"
#define prepend_error_and_exit() { \
    prepend_error_message (error, FUNC_NAME " at %zu", current_position + 1); \
                mc_config_deinit (ret_data); \
                return NULL; \
}

mc_config_t *
mc_deserialize_config (const char *data, GError ** error)
{
    char *current_group = NULL, *current_param = NULL, *current_value = NULL;
    size_t current_position = 0;
    mc_config_t *ret_data;
    enum automat_status
    {
        WAIT_GROUP,
        WAIT_PARAM,
        WAIT_VALUE
    } current_status = WAIT_GROUP;

    ret_data = mc_config_init (NULL, FALSE);

    while (data != NULL)
    {
        if ((current_status == WAIT_GROUP) && (*data == 'p') && (current_group != NULL))
            current_status = WAIT_PARAM;

        switch (current_status)
        {
        case WAIT_GROUP:
            g_free (current_group);

            current_group = mc_deserialize_str ('g', data, error);
            if (current_group == NULL)
                prepend_error_and_exit ();

            data = go_to_end_of_serialized_string (data, current_group, &current_position);
            current_status = WAIT_PARAM;
            break;
        case WAIT_PARAM:
            g_free (current_param);

            current_param = mc_deserialize_str ('p', data, error);
            if (current_param == NULL)
            {
                g_free (current_group);
                prepend_error_and_exit ();
            }

            data = go_to_end_of_serialized_string (data, current_param, &current_position);
            current_status = WAIT_VALUE;
            break;
        case WAIT_VALUE:
            current_value = mc_deserialize_str ('v', data, error);
            if (current_value == NULL)
            {
                g_free (current_group);
                g_free (current_param);
                prepend_error_and_exit ();
            }
            mc_config_set_string (ret_data, current_group, current_param, current_value);

            data = go_to_end_of_serialized_string (data, current_value, &current_position);
            g_free (current_value);
            current_status = WAIT_GROUP;
            break;
        default:
            break;
        }
    }
    g_free (current_group);
    g_free (current_param);

    return ret_data;
}

#undef FUNC_NAME

/* --------------------------------------------------------------------------------------------- */
