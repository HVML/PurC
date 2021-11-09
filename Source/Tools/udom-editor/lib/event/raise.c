/*
   Handle any events in application.
   Raise events.

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
#include "lib/event.h"

#include "internal.h"

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/*** file scope functions ************************************************************************/

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

gboolean
mc_event_raise (const gchar * event_group_name, const gchar * event_name, gpointer event_data)
{
    GTree *event_group;
    GPtrArray *callbacks;
    guint array_index;

    if (mc_event_grouplist == NULL || event_group_name == NULL || event_name == NULL)
        return FALSE;

    event_group = mc_event_get_event_group_by_name (event_group_name, FALSE, NULL);
    if (event_group == NULL)
        return FALSE;

    callbacks = mc_event_get_event_by_name (event_group, event_name, FALSE, NULL);
    if (callbacks == NULL)
        return FALSE;

    for (array_index = callbacks->len; array_index > 0; array_index--)
    {
        mc_event_callback_t *cb = g_ptr_array_index (callbacks, array_index - 1);
        if (!(*cb->callback) (event_group_name, event_name, cb->init_data, event_data))
            break;
    }
    return TRUE;
}

/* --------------------------------------------------------------------------------------------- */
