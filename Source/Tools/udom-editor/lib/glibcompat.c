/*
   GLIB - Library of useful routines for C programming

   Copyright (C) 2009-2021
   Free Software Foundation, Inc.

   Written by:
   Slava Zanko <slavazanko@gmail.com>, 2009, 2013.

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

/** \file glibcompat.c
 *  \brief Source: compatibility with older versions of glib
 *
 *  Following code was copied from glib to GNU Midnight Commander to
 *  provide compatibility with older versions of glib.
 */

#include <config.h>
#include <string.h>

#include "global.h"
#include "glibcompat.h"

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/*** file scope functions ************************************************************************/

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

#if ! GLIB_CHECK_VERSION (2, 63, 3)
/**
 * g_clear_slist: (skip)
 * @slist_ptr: (not nullable): a #GSList return location
 * @destroy: (nullable): the function to pass to g_slist_free_full() or NULL to not free elements
 *
 * Clears a pointer to a #GSList, freeing it and, optionally, freeing its elements using @destroy.
 *
 * @slist_ptr must be a valid pointer. If @slist_ptr points to a null #GSList, this does nothing.
 *
 * Since: 2.64
 */
void
g_clear_slist (GSList ** slist_ptr, GDestroyNotify destroy)
{
    GSList *slist;

    slist = *slist_ptr;

    if (slist != NULL)
    {
        *slist_ptr = NULL;

        if (destroy != NULL)
            g_slist_free_full (slist, destroy);
        else
            g_slist_free (slist);
    }
}

/* --------------------------------------------------------------------------------------------- */

/**
 * g_clear_list:
 * @list_ptr: (not nullable): a #GList return location
 * @destroy: (nullable): the function to pass to g_list_free_full() or NULL to not free elements
 *
 * Clears a pointer to a #GList, freeing it and, optionally, freeing its elements using @destroy.
 *
 * @list_ptr must be a valid pointer. If @list_ptr points to a null #GList, this does nothing.
 *
 * Since: 2.64
 */
void
g_clear_list (GList ** list_ptr, GDestroyNotify destroy)
{
    GList *list;

    list = *list_ptr;

    if (list != NULL)
    {
        *list_ptr = NULL;

        if (destroy != NULL)
            g_list_free_full (list, destroy);
        else
            g_list_free (list);
    }
}

/* --------------------------------------------------------------------------------------------- */

#endif /* ! GLIB_CHECK_VERSION (2, 63, 3) */

#if ! GLIB_CHECK_VERSION (2, 32, 0)
/**
 * g_queue_free_full:
 * @queue: a pointer to a #GQueue
 * @free_func: the function to be called to free each element's data
 *
 * Convenience method, which frees all the memory used by a #GQueue,
 * and calls the specified destroy function on every element's data.
 *
 * Since: 2.32
 */
void
g_queue_free_full (GQueue * queue, GDestroyNotify free_func)
{
    g_queue_foreach (queue, (GFunc) free_func, NULL);
    g_queue_free (queue);
}
#endif /* ! GLIB_CHECK_VERSION (2, 32, 0) */

/* --------------------------------------------------------------------------------------------- */

#if ! GLIB_CHECK_VERSION (2, 60, 0)
/**
 * g_queue_clear_full:
 * @queue: a pointer to a #GQueue
 * @free_func: (nullable): the function to be called to free memory allocated
 *
 * Convenience method, which frees all the memory used by a #GQueue,
 * and calls the provided @free_func on each item in the #GQueue.
 *
 * Since: 2.60
 */
void
g_queue_clear_full (GQueue * queue, GDestroyNotify free_func)
{
    g_return_if_fail (queue != NULL);

    if (free_func != NULL)
        g_queue_foreach (queue, (GFunc) free_func, NULL);

    g_queue_clear (queue);
}
#endif /* ! GLIB_CHECK_VERSION (2, 60, 0) */

/* --------------------------------------------------------------------------------------------- */

/**
 * mc_g_string_copy:
 * @dest: (not nullable): the destination #GString. Its current contents are destroyed
 * @src: (not nullable): the source #GString
 * @return: @dest
 *
 * Copies the bytes from a #GString into a #GString, destroying any previous contents.
 * It is rather like the standard strcpy() function, except that you do not have to worry about
 * having enough space to copy the string.
 *
 * There is no such API in GLib2.
 */
GString *
mc_g_string_copy (GString * dest, const GString * src)
{
    g_return_val_if_fail (src != NULL, NULL);
    g_return_val_if_fail (dest != NULL, NULL);

    g_string_set_size (dest, 0);
    g_string_append_len (dest, src->str, src->len);

    return dest;
}

/* --------------------------------------------------------------------------------------------- */

/**
 * mc_g_string_dup:
 * @s: (nullable): the source #GString
 * @return: @copy of @s
 *
 * Copies the bytes from one #GString to another.
 *
 * There is no such API in GLib2.
 */
GString *
mc_g_string_dup (const GString * s)
{
    GString *ret = NULL;

    if (s != NULL)
        ret = g_string_new_len (s->str, s->len);

    return ret;
}

/* --------------------------------------------------------------------------------------------- */
