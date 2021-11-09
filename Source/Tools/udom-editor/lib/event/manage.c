/*
   Handle any events in application.
   Manage events: add, delete, destroy, search

   Copyright (C) 2011-2021
   Free Software Foundation, Inc.

   Written by:
   Slava Zanko <slavazanko@gmail.com>, 2011.

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
#include "lib/util.h"
#include "lib/event.h"

#include "internal.h"

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

static void
mc_event_group_destroy_value (gpointer data)
{
    GPtrArray *callbacks;

    callbacks = (GPtrArray *) data;
    g_ptr_array_foreach (callbacks, (GFunc) g_free, NULL);
    g_ptr_array_free (callbacks, TRUE);
}

/* --------------------------------------------------------------------------------------------- */


/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

gboolean
mc_event_add (const gchar * event_group_name, const gchar * event_name,
              mc_event_callback_func_t event_callback, gpointer event_init_data, GError ** mcerror)
{

    GTree *event_group;
    GPtrArray *callbacks;
    mc_event_callback_t *cb;

    mc_return_val_if_error (mcerror, FALSE);

    if (mc_event_grouplist == NULL || event_group_name == NULL || event_name == NULL
        || event_callback == NULL)
    {
        mc_propagate_error (mcerror, 0, "%s", _("Check input data! Some of parameters are NULL!"));
        return FALSE;
    }

    event_group = mc_event_get_event_group_by_name (event_group_name, TRUE, mcerror);
    if (event_group == NULL)
        return FALSE;

    callbacks = mc_event_get_event_by_name (event_group, event_name, TRUE, mcerror);
    if (callbacks == NULL)
        return FALSE;

    cb = mc_event_is_callback_in_array (callbacks, event_callback, event_init_data);
    if (cb == NULL)
    {
        cb = g_new0 (mc_event_callback_t, 1);
        cb->callback = event_callback;
        g_ptr_array_add (callbacks, (gpointer) cb);
    }
    cb->init_data = event_init_data;
    return TRUE;
}

/* --------------------------------------------------------------------------------------------- */

void
mc_event_del (const gchar * event_group_name, const gchar * event_name,
              mc_event_callback_func_t event_callback, gpointer event_init_data)
{
    GTree *event_group;
    GPtrArray *callbacks;
    mc_event_callback_t *cb;

    if (mc_event_grouplist == NULL || event_group_name == NULL || event_name == NULL
        || event_callback == NULL)
        return;

    event_group = mc_event_get_event_group_by_name (event_group_name, FALSE, NULL);
    if (event_group == NULL)
        return;

    callbacks = mc_event_get_event_by_name (event_group, event_name, FALSE, NULL);
    if (callbacks == NULL)
        return;

    cb = mc_event_is_callback_in_array (callbacks, event_callback, event_init_data);

    if (cb == NULL)
        return;

    g_ptr_array_remove (callbacks, (gpointer) cb);
    g_free ((gpointer) cb);
}

/* --------------------------------------------------------------------------------------------- */

void
mc_event_destroy (const gchar * event_group_name, const gchar * event_name)
{
    GTree *event_group;

    if (mc_event_grouplist == NULL || event_group_name == NULL || event_name == NULL)
        return;

    event_group = mc_event_get_event_group_by_name (event_group_name, FALSE, NULL);
    g_tree_remove (event_group, (gconstpointer) event_name);
}

/* --------------------------------------------------------------------------------------------- */

void
mc_event_group_del (const gchar * event_group_name)
{

    if (mc_event_grouplist != NULL && event_group_name != NULL)
        g_tree_remove (mc_event_grouplist, (gconstpointer) event_group_name);
}

/* --------------------------------------------------------------------------------------------- */

GTree *
mc_event_get_event_group_by_name (const gchar * event_group_name, gboolean create_new,
                                  GError ** mcerror)
{
    GTree *event_group;

    mc_return_val_if_error (mcerror, FALSE);

    event_group = (GTree *) g_tree_lookup (mc_event_grouplist, (gconstpointer) event_group_name);
    if (event_group == NULL && create_new)
    {
        event_group =
            g_tree_new_full ((GCompareDataFunc) g_ascii_strcasecmp,
                             NULL,
                             (GDestroyNotify) g_free,
                             (GDestroyNotify) mc_event_group_destroy_value);
        if (event_group == NULL)
        {
            mc_propagate_error (mcerror, 0, _("Unable to create group '%s' for events!"),
                                event_group_name);
            return NULL;
        }
        g_tree_insert (mc_event_grouplist, g_strdup (event_group_name), (gpointer) event_group);
    }
    return event_group;
}

/* --------------------------------------------------------------------------------------------- */

GPtrArray *
mc_event_get_event_by_name (GTree * event_group, const gchar * event_name, gboolean create_new,
                            GError ** mcerror)
{
    GPtrArray *callbacks;

    mc_return_val_if_error (mcerror, FALSE);

    callbacks = (GPtrArray *) g_tree_lookup (event_group, (gconstpointer) event_name);
    if (callbacks == NULL && create_new)
    {
        callbacks = g_ptr_array_new ();
        if (callbacks == NULL)
        {
            mc_propagate_error (mcerror, 0, _("Unable to create event '%s'!"), event_name);
            return NULL;
        }
        g_tree_insert (event_group, g_strdup (event_name), (gpointer) callbacks);
    }
    return callbacks;
}

/* --------------------------------------------------------------------------------------------- */

mc_event_callback_t *
mc_event_is_callback_in_array (GPtrArray * callbacks, mc_event_callback_func_t event_callback,
                               gpointer event_init_data)
{
    guint array_index;

    for (array_index = 0; array_index < callbacks->len; array_index++)
    {
        mc_event_callback_t *cb = g_ptr_array_index (callbacks, array_index);
        if (cb->callback == event_callback && cb->init_data == event_init_data)
            return cb;
    }
    return NULL;
}

/* --------------------------------------------------------------------------------------------- */
