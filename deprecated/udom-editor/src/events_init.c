/*
   Event callbacks initialization

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

#ifdef ENABLE_BACKGROUND
#include "background.h"         /* (background_parent_call), background_parent_call_string() */
#endif /* ENABLE_BACKGROUND */
#include "clipboard.h"          /* clipboard events */
#include "execute.h"            /* execute_suspend() */
#include "help.h"               /* help_interactive_display() */

#include "events_init.h"

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/


/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

gboolean
events_init (GError ** mcerror)
{
    /* *INDENT-OFF* */
    static const event_init_t standard_events[] =
    {
        {MCEVENT_GROUP_CORE, "clipboard_file_to_ext_clip", clipboard_file_to_ext_clip, NULL},
        {MCEVENT_GROUP_CORE, "clipboard_file_from_ext_clip", clipboard_file_from_ext_clip, NULL},
        {MCEVENT_GROUP_CORE, "clipboard_text_to_file", clipboard_text_to_file, NULL},
        {MCEVENT_GROUP_CORE, "clipboard_text_from_file", clipboard_text_from_file, NULL},

        {MCEVENT_GROUP_CORE, "help", help_interactive_display, NULL},
        {MCEVENT_GROUP_CORE, "suspend", execute_suspend, NULL},

#ifdef ENABLE_BACKGROUND
        {MCEVENT_GROUP_CORE, "background_parent_call", background_parent_call, NULL},
        {MCEVENT_GROUP_CORE, "background_parent_call_string", background_parent_call_string, NULL},
#endif /* ENABLE_BACKGROUND */

        {NULL, NULL, NULL, NULL}
    };
    /* *INDENT-ON* */

    if (!mc_event_init (mcerror))
        return FALSE;

    return mc_event_mass_add (standard_events, mcerror);
}

/* --------------------------------------------------------------------------------------------- */
