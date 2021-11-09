/*
   Virtual File System garbage collection code

   Copyright (C) 2003-2021
   Free Software Foundation, Inc.

   Written by:
   Miguel de Icaza, 1995
   Jakub Jelinek, 1995
   Pavel Machek, 1998
   Pavel Roskin, 2003

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

/**
 * \file
 * \brief Source: Virtual File System: garbage collection code
 * \author Miguel de Icaza
 * \author Jakub Jelinek
 * \author Pavel Machek
 * \author Pavel Roskin
 * \date 1995, 1998, 2003
 */


#include <config.h>

#include <stdlib.h>

#include "lib/global.h"
#include "lib/event.h"
#include "lib/util.h"           /* MC_PTR_FREE */

#include "vfs.h"
#include "utilvfs.h"

#include "gc.h"

/*
 * The garbage collection mechanism is based on "stamps".
 *
 * A stamp is a record that says "I'm a filesystem which is no longer in
 * use. Free me when you get a chance."
 *
 * This file contains a set of functions used for managing this stamp. You
 * should use them when you write your own filesystem. Here are some rules
 * of thumb:
 *
 * (1) When the last open file in your filesystem gets closed, conditionally
 *     create a stamp. You do this with vfs_stamp_create(). (The meaning
 *     of "conditionaly" is explained below.)
 *
 * (2) When a file in your filesystem is opened, delete the stamp. You do
 *     this with vfs_rmstamp().
 *
 * (3) When a path inside your filesystem is invoked, call vfs_stamp() to
 *     postpone the free'ing of your filesystem a bit. (This simply updates
 *     a timestamp variable inside the stamp.)
 *
 * Additionally, when a user navigates to a new directory in a panel (or a
 * programmer uses mc_chdir()), a stamp is conditionally created for the
 * previous directory's filesystem. This ensures that that filesystem is
 * free'ed. (see: _do_panel_cd() -> vfs_release_path(); mc_chdir()).
 *
 * We've spoken here of "conditionally creating" a stamp. What we mean is
 * that vfs_stamp_create() is to be used: this function creates a stamp
 * only if no directories are open (aka "active") in your filesystem. (If
 * there _are_ directories open, it means that the filesystem is in use, in
 * which case we don't want to free it.)
 */

/*** global variables ****************************************************************************/

int vfs_timeout = 60;           /* VFS timeout in seconds */

/*** file scope macro definitions ****************************************************************/

#define VFS_STAMPING(a) ((struct vfs_stamping *)(a))

/*** file scope type declarations ****************************************************************/

struct vfs_stamping
{
    struct vfs_class *v;
    vfsid id;
    gint64 time;
};

/*** file scope variables ************************************************************************/

static GSList *stamps = NULL;

/* --------------------------------------------------------------------------------------------- */
/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

static gint
vfs_stamp_compare (gconstpointer a, gconstpointer b)
{
    const struct vfs_stamping *vsa = (const struct vfs_stamping *) a;
    const struct vfs_stamping *vsb = (const struct vfs_stamping *) b;

    return (vsa == NULL || vsb == NULL || (vsa->v == vsb->v && vsa->id == vsb->id)) ? 0 : 1;
}

/* --------------------------------------------------------------------------------------------- */

static void
vfs_addstamp (struct vfs_class *v, vfsid id)
{
    if ((v->flags & VFSF_LOCAL) == 0 && id != NULL && !vfs_stamp (v, id))
    {
        struct vfs_stamping *stamp;

        stamp = g_new (struct vfs_stamping, 1);
        stamp->v = v;
        stamp->id = id;
        stamp->time = g_get_real_time ();

        stamps = g_slist_append (stamps, stamp);
    }
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

gboolean
vfs_stamp (struct vfs_class *v, vfsid id)
{
    struct vfs_stamping what = {
        .v = v,
        .id = id
    };
    GSList *stamp;
    gboolean ret = FALSE;

    stamp = g_slist_find_custom (stamps, &what, vfs_stamp_compare);
    if (stamp != NULL && stamp->data != NULL)
    {
        VFS_STAMPING (stamp->data)->time = g_get_real_time ();
        ret = TRUE;
    }

    return ret;
}

/* --------------------------------------------------------------------------------------------- */

void
vfs_rmstamp (struct vfs_class *v, vfsid id)
{
    struct vfs_stamping what = {
        .v = v,
        .id = id
    };
    GSList *stamp;

    stamp = g_slist_find_custom (stamps, &what, vfs_stamp_compare);
    if (stamp != NULL)
    {
        g_free (stamp->data);
        stamps = g_slist_delete_link (stamps, stamp);
    }
}

/* --------------------------------------------------------------------------------------------- */

void
vfs_stamp_path (const vfs_path_t * vpath)
{
    vfsid id;
    const vfs_path_element_t *path_element;

    path_element = vfs_path_get_by_index (vpath, -1);

    id = vfs_getid (vpath);
    vfs_addstamp (path_element->class, id);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Create a new timestamp item by VFS class and VFS id.
 */

void
vfs_stamp_create (struct vfs_class *vclass, vfsid id)
{
    vfsid nvfsid;

    ev_vfs_stamp_create_t event_data = { vclass, id, FALSE };
    const vfs_path_t *vpath;
    const vfs_path_element_t *path_element;

    /* There are three directories we have to take care of: current_dir,
       current_panel->cwd and other_panel->cwd. Athough most of the time either
       current_dir and current_panel->cwd or current_dir and other_panel->cwd are the
       same, it's possible that all three are different -- Norbert */

    if (!mc_event_present (MCEVENT_GROUP_CORE, "vfs_timestamp"))
        return;

    vpath = vfs_get_raw_current_dir ();
    path_element = vfs_path_get_by_index (vpath, -1);

    nvfsid = vfs_getid (vpath);
    vfs_rmstamp (path_element->class, nvfsid);

    if (!(id == NULL || (path_element->class == vclass && nvfsid == id)))
    {
        mc_event_raise (MCEVENT_GROUP_CORE, "vfs_timestamp", (gpointer) & event_data);

        if (!event_data.ret && vclass != NULL && vclass->nothingisopen != NULL
            && vclass->nothingisopen (id))
            vfs_addstamp (vclass, id);
    }
}

/* --------------------------------------------------------------------------------------------- */
/** This is called from timeout handler with now = FALSE,
    or can be called with now = TRUE to force freeing all filesystems */

void
vfs_expire (gboolean now)
{
    static gboolean locked = FALSE;
    gint64 curr_time, exp_time;
    GSList *stamp;

    /* Avoid recursive invocation, e.g. when one of the free functions
       calls message */
    if (locked)
        return;
    locked = TRUE;

    curr_time = g_get_real_time ();
    exp_time = curr_time - vfs_timeout * G_USEC_PER_SEC;

    if (now)
    {
        /* reverse list to free nested VFSes at first */
        stamps = g_slist_reverse (stamps);
    }

    /* NULLize stamps that point to expired VFS */
    for (stamp = stamps; stamp != NULL; stamp = g_slist_next (stamp))
    {
        struct vfs_stamping *stamping = VFS_STAMPING (stamp->data);

        if (now)
        {
            /* free VFS forced */
            if (stamping->v->free != NULL)
                stamping->v->free (stamping->id);
            MC_PTR_FREE (stamp->data);
        }
        else if (stamping->time <= exp_time)
        {
            /* update timestamp of VFS that is in use, or free unused VFS */
            if (stamping->v->nothingisopen != NULL && !stamping->v->nothingisopen (stamping->id))
                stamping->time = curr_time;
            else
            {
                if (stamping->v->free != NULL)
                    stamping->v->free (stamping->id);
                MC_PTR_FREE (stamp->data);
            }
        }
    }

    /* then remove NULLized stamps */
    stamps = g_slist_remove_all (stamps, NULL);

    locked = FALSE;
}

/* --------------------------------------------------------------------------------------------- */
/*
 * Return the number of seconds remaining to the vfs timeout.
 * FIXME: The code should be improved to actually return the number of
 * seconds until the next item times out.
 */

int
vfs_timeouts (void)
{
    return stamps != NULL ? 10 : 0;
}

/* --------------------------------------------------------------------------------------------- */

void
vfs_timeout_handler (void)
{
    vfs_expire (FALSE);
}

/* --------------------------------------------------------------------------------------------- */

void
vfs_release_path (const vfs_path_t * vpath)
{
    const vfs_path_element_t *path_element;

    path_element = vfs_path_get_by_index (vpath, -1);
    vfs_stamp_create (path_element->class, vfs_getid (vpath));
}

/* --------------------------------------------------------------------------------------------- */
/* Free all data */

void
vfs_gc_done (void)
{
    vfs_expire (TRUE);
}

/* --------------------------------------------------------------------------------------------- */
