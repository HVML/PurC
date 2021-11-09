/*
   Handle  events in application.
   Interface functions: init/deinit; start/stop

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

GTree *mc_event_grouplist = NULL;

/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

gboolean
mc_event_init (GError ** mcerror)
{
    mc_return_val_if_error (mcerror, FALSE);

    if (mc_event_grouplist != NULL)
    {
        mc_propagate_error (mcerror, 0, "%s", _("Event system already initialized"));
        return FALSE;
    }

    mc_event_grouplist =
        g_tree_new_full ((GCompareDataFunc) g_ascii_strcasecmp,
                         NULL, (GDestroyNotify) g_free, (GDestroyNotify) g_tree_destroy);

    if (mc_event_grouplist == NULL)
    {
        mc_propagate_error (mcerror, 0, "%s", _("Failed to initialize event system"));
        return FALSE;
    }

    return TRUE;
}

/* --------------------------------------------------------------------------------------------- */

gboolean
mc_event_deinit (GError ** mcerror)
{
    mc_return_val_if_error (mcerror, FALSE);

    if (mc_event_grouplist == NULL)
    {
        mc_propagate_error (mcerror, 0, "%s", _("Event system not initialized"));
        return FALSE;
    }

    g_tree_destroy (mc_event_grouplist);
    mc_event_grouplist = NULL;
    return TRUE;
}

/* --------------------------------------------------------------------------------------------- */

gboolean
mc_event_mass_add (const event_init_t * events, GError ** mcerror)
{
    size_t array_index;

    mc_return_val_if_error (mcerror, FALSE);

    for (array_index = 0; events[array_index].event_group_name != NULL; array_index++)
    {
        if (!mc_event_add (events[array_index].event_group_name,
                           events[array_index].event_name,
                           events[array_index].cb, events[array_index].init_data, mcerror))
        {
            return FALSE;
        }
    }
    return TRUE;
}

/* --------------------------------------------------------------------------------------------- */

gboolean
mc_event_present (const gchar * event_group_name, const gchar * event_name)
{
    GTree *event_group;
    GPtrArray *callbacks;

    if (mc_event_grouplist == NULL || event_group_name == NULL || event_name == NULL)
        return FALSE;

    event_group = mc_event_get_event_group_by_name (event_group_name, FALSE, NULL);
    if (event_group == NULL)
        return FALSE;

    callbacks = mc_event_get_event_by_name (event_group, event_name, FALSE, NULL);
    if (callbacks == NULL)
        return FALSE;

    return TRUE;
}

/* --------------------------------------------------------------------------------------------- */
