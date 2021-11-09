/*
   Hooks.

   Slavaz: Warning! this file is deprecated and should be replaced
   by mcevents functional.

   Copyright (C) 1994-2021
   Free Software Foundation, Inc.

   Written by:
   Miguel de Icaza, 1994, 1995, 1996
   Janne Kukonlehto, 1994, 1995, 1996
   Dugan Porter, 1994, 1995, 1996
   Jakub Jelinek, 1994, 1995, 1996
   Mauricio Plaza, 1994, 1995, 1996

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

/** \file
 *  \brief Source: hooks
 */

#include <config.h>

#include "lib/global.h"
#include "lib/hook.h"

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/*** file scope functions ************************************************************************/

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

void
add_hook (hook_t ** hook_list, void (*hook_fn) (void *), void *data)
{
    hook_t *new_hook = g_new (hook_t, 1);

    new_hook->hook_fn = hook_fn;
    new_hook->next = *hook_list;
    new_hook->hook_data = data;

    *hook_list = new_hook;
}

/* --------------------------------------------------------------------------------------------- */

void
execute_hooks (hook_t * hook_list)
{
    hook_t *new_hook = NULL;
    hook_t *p;

    /* We copy the hook list first so tahat we let the hook
     * function call delete_hook
     */

    while (hook_list != NULL)
    {
        add_hook (&new_hook, hook_list->hook_fn, hook_list->hook_data);
        hook_list = hook_list->next;
    }
    p = new_hook;

    while (new_hook != NULL)
    {
        new_hook->hook_fn (new_hook->hook_data);
        new_hook = new_hook->next;
    }

    for (hook_list = p; hook_list != NULL;)
    {
        p = hook_list;
        hook_list = hook_list->next;
        g_free (p);
    }
}

/* --------------------------------------------------------------------------------------------- */

void
delete_hook (hook_t ** hook_list, void (*hook_fn) (void *))
{
    hook_t *new_list = NULL;
    hook_t *current, *next;

    for (current = *hook_list; current != NULL; current = next)
    {
        next = current->next;
        if (current->hook_fn == hook_fn)
            g_free (current);
        else
            add_hook (&new_list, current->hook_fn, current->hook_data);
    }
    *hook_list = new_list;
}

/* --------------------------------------------------------------------------------------------- */

gboolean
hook_present (hook_t * hook_list, void (*hook_fn) (void *))
{
    hook_t *p;

    for (p = hook_list; p != NULL; p = p->next)
        if (p->hook_fn == hook_fn)
            return TRUE;
    return FALSE;
}

/* --------------------------------------------------------------------------------------------- */
