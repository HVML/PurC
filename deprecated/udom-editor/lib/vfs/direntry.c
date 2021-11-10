/*
   Directory cache support

   Copyright (C) 1998-2021
   Free Software Foundation, Inc.

   Written by:
   Pavel Machek <pavel@ucw.cz>, 1998
   Slava Zanko <slavazanko@gmail.com>, 2013

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

   \warning Paths here do _not_ begin with '/', so root directory of
   archive/site is simply "".
 */

/** \file
 *  \brief Source: directory cache support
 *
 *  So that you do not have copy of this in each and every filesystem.
 *
 *  Very loosely based on tar.c from midnight and archives.[ch] from
 *  avfs by Miklos Szeredi (mszeredi@inf.bme.hu)
 *
 *  Unfortunately, I was unable to keep all filesystems
 *  uniform. tar-like filesystems use tree structure where each
 *  directory has pointers to its subdirectories. We can do this
 *  because we have full information about our archive.
 *
 *  At ftp-like filesystems, situation is a little bit different. When
 *  you cd /usr/src/linux/drivers/char, you do _not_ want /usr,
 *  /usr/src, /usr/src/linux and /usr/src/linux/drivers to be
 *  listed. That means that we do not have complete information, and if
 *  /usr is symlink to /4, we will not know. Also we have to time out
 *  entries and things would get messy with tree-like approach. So we
 *  do different trick: root directory is completely special and
 *  completely fake, it contains entries such as 'usr', 'usr/src', ...,
 *  and we'll try to use custom find_entry function.
 *
 *  \author Pavel Machek <pavel@ucw.cz>
 *  \date 1998
 *
 */

#include <config.h>

#include <errno.h>
#include <inttypes.h>           /* uintmax_t */
#include <stdarg.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#include <sys/types.h>
#include <unistd.h>

#include "lib/global.h"

#include "lib/tty/tty.h"        /* enable/disable interrupt key */
#include "lib/util.h"           /* custom_canonicalize_pathname() */
#if 0
#include "lib/widget.h"         /* message() */
#endif

#include "vfs.h"
#include "utilvfs.h"
#include "xdirentry.h"
#include "gc.h"                 /* vfs_rmstamp */

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

#define CALL(x) \
        if (VFS_SUBCLASS (me)->x != NULL) \
            VFS_SUBCLASS (me)->x

/*** file scope type declarations ****************************************************************/

struct dirhandle
{
    GList *cur;
    struct vfs_s_inode *dir;
};

/*** file scope variables ************************************************************************/

/* --------------------------------------------------------------------------------------------- */
/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

/* We were asked to create entries automagically */

static struct vfs_s_entry *
vfs_s_automake (struct vfs_class *me, struct vfs_s_inode *dir, char *path, int flags)
{
    struct vfs_s_entry *res;
    char *sep;

    sep = strchr (path, PATH_SEP);
    if (sep != NULL)
        *sep = '\0';

    res = vfs_s_generate_entry (me, path, dir, (flags & FL_MKDIR) != 0 ? (0777 | S_IFDIR) : 0777);
    vfs_s_insert_entry (me, dir, res);

    if (sep != NULL)
        *sep = PATH_SEP;

    return res;
}

/* --------------------------------------------------------------------------------------------- */
/* If the entry is a symlink, find the entry for its target */

static struct vfs_s_entry *
vfs_s_resolve_symlink (struct vfs_class *me, struct vfs_s_entry *entry, int follow)
{
    char *linkname;
    char *fullname = NULL;
    struct vfs_s_entry *target;

    if (follow == LINK_NO_FOLLOW)
        return entry;
    if (follow == 0)
        ERRNOR (ELOOP, NULL);
    if (entry == NULL)
        ERRNOR (ENOENT, NULL);
    if (!S_ISLNK (entry->ino->st.st_mode))
        return entry;

    linkname = entry->ino->linkname;
    if (linkname == NULL)
        ERRNOR (EFAULT, NULL);

    /* make full path from relative */
    if (!IS_PATH_SEP (*linkname))
    {
        char *fullpath;

        fullpath = vfs_s_fullpath (me, entry->dir);
        if (fullpath != NULL)
        {
            fullname = g_strconcat (fullpath, PATH_SEP_STR, linkname, (char *) NULL);
            linkname = fullname;
            g_free (fullpath);
        }
    }

    target =
        VFS_SUBCLASS (me)->find_entry (me, entry->dir->super->root, linkname, follow - 1, FL_NONE);
    g_free (fullname);
    return target;
}

/* --------------------------------------------------------------------------------------------- */
/*
 * Follow > 0: follow links, serves as loop protect,
 *       == -1: do not follow links
 */

static struct vfs_s_entry *
vfs_s_find_entry_tree (struct vfs_class *me, struct vfs_s_inode *root,
                       const char *a_path, int follow, int flags)
{
    size_t pseg;
    struct vfs_s_entry *ent = NULL;
    char *const pathref = g_strdup (a_path);
    char *path = pathref;

    /* canonicalize as well, but don't remove '../' from path */
    custom_canonicalize_pathname (path, CANON_PATH_ALL & (~CANON_PATH_REMDOUBLEDOTS));

    while (root != NULL)
    {
        GList *iter;

        while (IS_PATH_SEP (*path))     /* Strip leading '/' */
            path++;

        if (path[0] == '\0')
        {
            g_free (pathref);
            return ent;
        }

        for (pseg = 0; path[pseg] != '\0' && !IS_PATH_SEP (path[pseg]); pseg++)
            ;

        for (iter = g_queue_peek_head_link (root->subdir); iter != NULL; iter = g_list_next (iter))
        {
            ent = VFS_ENTRY (iter->data);
            if (strlen (ent->name) == pseg && strncmp (ent->name, path, pseg) == 0)
                /* FOUND! */
                break;
        }

        ent = iter != NULL ? VFS_ENTRY (iter->data) : NULL;

        if (ent == NULL && (flags & (FL_MKFILE | FL_MKDIR)) != 0)
            ent = vfs_s_automake (me, root, path, flags);
        if (ent == NULL)
        {
            me->verrno = ENOENT;
            goto cleanup;
        }

        path += pseg;
        /* here we must follow leading directories always;
           only the actual file is optional */
        ent = vfs_s_resolve_symlink (me, ent,
                                     strchr (path, PATH_SEP) != NULL ? LINK_FOLLOW : follow);
        if (ent == NULL)
            goto cleanup;
        root = ent->ino;
    }
  cleanup:
    g_free (pathref);
    return NULL;
}

/* --------------------------------------------------------------------------------------------- */

static struct vfs_s_entry *
vfs_s_find_entry_linear (struct vfs_class *me, struct vfs_s_inode *root,
                         const char *a_path, int follow, int flags)
{
    struct vfs_s_entry *ent = NULL;
    char *const path = g_strdup (a_path);
    GList *iter;

    if (root->super->root != root)
        vfs_die ("We have to use _real_ root. Always. Sorry.");

    /* canonicalize as well, but don't remove '../' from path */
    custom_canonicalize_pathname (path, CANON_PATH_ALL & (~CANON_PATH_REMDOUBLEDOTS));

    if ((flags & FL_DIR) == 0)
    {
        char *dirname, *name;
        struct vfs_s_inode *ino;

        dirname = g_path_get_dirname (path);
        name = g_path_get_basename (path);
        ino = vfs_s_find_inode (me, root->super, dirname, follow, flags | FL_DIR);
        ent = vfs_s_find_entry_tree (me, ino, name, follow, flags);
        g_free (dirname);
        g_free (name);
        g_free (path);
        return ent;
    }

    iter = g_queue_find_custom (root->subdir, path, (GCompareFunc) vfs_s_entry_compare);
    ent = iter != NULL ? VFS_ENTRY (iter->data) : NULL;

    if (ent != NULL && !VFS_SUBCLASS (me)->dir_uptodate (me, ent->ino))
    {
#if 1
        vfs_print_message (_("Directory cache expired for %s"), path);
#endif
        vfs_s_free_entry (me, ent);
        ent = NULL;
    }

    if (ent == NULL)
    {
        struct vfs_s_inode *ino;

        ino = vfs_s_new_inode (me, root->super, vfs_s_default_stat (me, S_IFDIR | 0755));
        ent = vfs_s_new_entry (me, path, ino);
        if (VFS_SUBCLASS (me)->dir_load (me, ino, path) == -1)
        {
            vfs_s_free_entry (me, ent);
            g_free (path);
            return NULL;
        }

        vfs_s_insert_entry (me, root, ent);

        iter = g_queue_find_custom (root->subdir, path, (GCompareFunc) vfs_s_entry_compare);
        ent = iter != NULL ? VFS_ENTRY (iter->data) : NULL;
    }
    if (ent == NULL)
        vfs_die ("find_linear: success but directory is not there\n");

#if 0
    if (vfs_s_resolve_symlink (me, ent, follow) == NULL)
    {
        g_free (path);
        return NULL;
    }
#endif
    g_free (path);
    return ent;
}

/* --------------------------------------------------------------------------------------------- */
/* Ook, these were functions around directory entries / inodes */
/* -------------------------------- superblock games -------------------------- */

static struct vfs_s_super *
vfs_s_new_super (struct vfs_class *me)
{
    struct vfs_s_super *super;

    super = g_new0 (struct vfs_s_super, 1);
    super->me = me;
    return super;
}

/* --------------------------------------------------------------------------------------------- */

static inline void
vfs_s_insert_super (struct vfs_class *me, struct vfs_s_super *super)
{
    VFS_SUBCLASS (me)->supers = g_list_prepend (VFS_SUBCLASS (me)->supers, super);
}

/* --------------------------------------------------------------------------------------------- */

static void
vfs_s_free_super (struct vfs_class *me, struct vfs_s_super *super)
{
    if (super->root != NULL)
    {
        vfs_s_free_inode (me, super->root);
        super->root = NULL;
    }

#if 0
    /* FIXME: We currently leak small ammount of memory, sometimes. Fix it if you can. */
    if (super->ino_usage != 0)
        message (D_ERROR, "Direntry warning",
                 "Super ino_usage is %d, memory leak", super->ino_usage);

    if (super->want_stale)
        message (D_ERROR, "Direntry warning", "%s", "Super has want_stale set");
#endif

    VFS_SUBCLASS (me)->supers = g_list_remove (VFS_SUBCLASS (me)->supers, super);

    CALL (free_archive) (me, super);
#ifdef ENABLE_VFS_NET
    vfs_path_element_free (super->path_element);
#endif
    g_free (super->name);
    g_free (super);
}

/* --------------------------------------------------------------------------------------------- */

static vfs_file_handler_t *
vfs_s_new_fh (struct vfs_s_inode *ino, gboolean changed)
{
    vfs_file_handler_t *fh;

    fh = g_new0 (vfs_file_handler_t, 1);
    vfs_s_init_fh (fh, ino, changed);

    return fh;
}

/* --------------------------------------------------------------------------------------------- */

static void
vfs_s_free_fh (struct vfs_s_subclass *s, vfs_file_handler_t * fh)
{
    if (s->fh_free != NULL)
        s->fh_free (fh);

    g_free (fh);
}

/* --------------------------------------------------------------------------------------------- */
/* Support of archives */
/* ------------------------ readdir & friends ----------------------------- */

static struct vfs_s_inode *
vfs_s_inode_from_path (const vfs_path_t * vpath, int flags)
{
    struct vfs_s_super *super;
    struct vfs_s_inode *ino;
    const char *q;
    const vfs_path_element_t *path_element;

    q = vfs_s_get_path (vpath, &super, 0);
    if (q == NULL)
        return NULL;

    path_element = vfs_path_get_by_index (vpath, -1);

    ino =
        vfs_s_find_inode (path_element->class, super, q,
                          (flags & FL_FOLLOW) != 0 ? LINK_FOLLOW : LINK_NO_FOLLOW,
                          flags & ~FL_FOLLOW);
    if (ino == NULL && *q == '\0')
        /* We are asking about / directory of ftp server: assume it exists */
        ino =
            vfs_s_find_inode (path_element->class, super, q,
                              (flags & FL_FOLLOW) != 0 ? LINK_FOLLOW : LINK_NO_FOLLOW,
                              FL_DIR | (flags & ~FL_FOLLOW));
    return ino;
}

/* --------------------------------------------------------------------------------------------- */

static void *
vfs_s_opendir (const vfs_path_t * vpath)
{
    struct vfs_s_inode *dir;
    struct dirhandle *info;
    const vfs_path_element_t *path_element;

    dir = vfs_s_inode_from_path (vpath, FL_DIR | FL_FOLLOW);
    if (dir == NULL)
        return NULL;

    path_element = vfs_path_get_by_index (vpath, -1);

    if (!S_ISDIR (dir->st.st_mode))
    {
        path_element->class->verrno = ENOTDIR;
        return NULL;
    }

    dir->st.st_nlink++;
#if 0
    if (dir->subdir == NULL)    /* This can actually happen if we allow empty directories */
    {
        path_element->class->verrno = EAGAIN;
        return NULL;
    }
#endif
    info = g_new (struct dirhandle, 1);
    info->cur = g_queue_peek_head_link (dir->subdir);
    info->dir = dir;

    return info;
}

/* --------------------------------------------------------------------------------------------- */

static struct vfs_dirent *
vfs_s_readdir (void *data)
{
    struct vfs_dirent *dir = NULL;
    struct dirhandle *info = (struct dirhandle *) data;
    const char *name;

    if (info->cur == NULL || info->cur->data == NULL)
        return NULL;

    name = VFS_ENTRY (info->cur->data)->name;
    if (name != NULL)
        dir = vfs_dirent_init (NULL, name, 0);
    else
        vfs_die ("Null in structure-cannot happen");

    info->cur = g_list_next (info->cur);

    return dir;
}

/* --------------------------------------------------------------------------------------------- */

static int
vfs_s_closedir (void *data)
{
    struct dirhandle *info = (struct dirhandle *) data;
    struct vfs_s_inode *dir = info->dir;

    vfs_s_free_inode (dir->super->me, dir);
    g_free (data);
    return 0;
}

/* --------------------------------------------------------------------------------------------- */

static int
vfs_s_chdir (const vfs_path_t * vpath)
{
    void *data;

    data = vfs_s_opendir (vpath);
    if (data == NULL)
        return (-1);
    vfs_s_closedir (data);
    return 0;
}

/* --------------------------------------------------------------------------------------------- */
/* --------------------------- stat and friends ---------------------------- */

static int
vfs_s_internal_stat (const vfs_path_t * vpath, struct stat *buf, int flag)
{
    struct vfs_s_inode *ino;

    ino = vfs_s_inode_from_path (vpath, flag);
    if (ino == NULL)
        return (-1);
    *buf = ino->st;
    return 0;
}

/* --------------------------------------------------------------------------------------------- */

static int
vfs_s_readlink (const vfs_path_t * vpath, char *buf, size_t size)
{
    struct vfs_s_inode *ino;
    size_t len;
    const vfs_path_element_t *path_element;

    ino = vfs_s_inode_from_path (vpath, 0);
    if (ino == NULL)
        return (-1);

    path_element = vfs_path_get_by_index (vpath, -1);

    if (!S_ISLNK (ino->st.st_mode))
    {
        path_element->class->verrno = EINVAL;
        return (-1);
    }

    if (ino->linkname == NULL)
    {
        path_element->class->verrno = EFAULT;
        return (-1);
    }

    len = strlen (ino->linkname);
    if (size < len)
        len = size;
    /* readlink() does not append a NUL character to buf */
    memcpy (buf, ino->linkname, len);
    return len;
}

/* --------------------------------------------------------------------------------------------- */

static ssize_t
vfs_s_read (void *fh, char *buffer, size_t count)
{
    vfs_file_handler_t *file = VFS_FILE_HANDLER (fh);
    struct vfs_class *me = VFS_FILE_HANDLER_SUPER (fh)->me;

    if (file->linear == LS_LINEAR_PREOPEN)
    {
        if (VFS_SUBCLASS (me)->linear_start (me, file, file->pos) == 0)
            return (-1);
    }

    if (file->linear == LS_LINEAR_CLOSED)
        vfs_die ("linear_start() did not set linear_state!");

    if (file->linear == LS_LINEAR_OPEN)
        return VFS_SUBCLASS (me)->linear_read (me, file, buffer, count);

    if (file->handle != -1)
    {
        ssize_t n;

        n = read (file->handle, buffer, count);
        if (n < 0)
            me->verrno = errno;
        return n;
    }
    vfs_die ("vfs_s_read: This should not happen\n");
    return (-1);
}

/* --------------------------------------------------------------------------------------------- */

static ssize_t
vfs_s_write (void *fh, const char *buffer, size_t count)
{
    vfs_file_handler_t *file = VFS_FILE_HANDLER (fh);
    struct vfs_class *me = VFS_FILE_HANDLER_SUPER (fh)->me;

    if (file->linear != LS_NOT_LINEAR)
        vfs_die ("no writing to linear files, please");

    file->changed = TRUE;
    if (file->handle != -1)
    {
        ssize_t n;

        n = write (file->handle, buffer, count);
        if (n < 0)
            me->verrno = errno;
        return n;
    }
    vfs_die ("vfs_s_write: This should not happen\n");
    return 0;
}

/* --------------------------------------------------------------------------------------------- */

static off_t
vfs_s_lseek (void *fh, off_t offset, int whence)
{
    vfs_file_handler_t *file = VFS_FILE_HANDLER (fh);
    off_t size = file->ino->st.st_size;

    if (file->linear == LS_LINEAR_OPEN)
        vfs_die ("cannot lseek() after linear_read!");

    if (file->handle != -1)
    {                           /* If we have local file opened, we want to work with it */
        off_t retval;

        retval = lseek (file->handle, offset, whence);
        if (retval == -1)
            VFS_FILE_HANDLER_SUPER (fh)->me->verrno = errno;
        return retval;
    }

    switch (whence)
    {
    case SEEK_CUR:
        offset += file->pos;
        break;
    case SEEK_END:
        offset += size;
        break;
    default:
        break;
    }
    if (offset < 0)
        file->pos = 0;
    else if (offset < size)
        file->pos = offset;
    else
        file->pos = size;
    return file->pos;
}

/* --------------------------------------------------------------------------------------------- */

static int
vfs_s_close (void *fh)
{
    vfs_file_handler_t *file = VFS_FILE_HANDLER (fh);
    struct vfs_s_super *super = VFS_FILE_HANDLER_SUPER (fh);
    struct vfs_class *me = super->me;
    struct vfs_s_subclass *sub = VFS_SUBCLASS (me);
    int res = 0;

    if (me == NULL)
        return (-1);

    super->fd_usage--;
    if (super->fd_usage == 0)
        vfs_stamp_create (me, VFS_FILE_HANDLER_SUPER (fh));

    if (file->linear == LS_LINEAR_OPEN)
        sub->linear_close (me, fh);
    if (sub->fh_close != NULL)
        res = sub->fh_close (me, fh);
    if ((me->flags & VFSF_USETMP) != 0 && file->changed && sub->file_store != NULL)
    {
        char *s;

        s = vfs_s_fullpath (me, file->ino);

        if (s == NULL)
            res = -1;
        else
        {
            res = sub->file_store (me, fh, s, file->ino->localname);
            g_free (s);
        }
        vfs_s_invalidate (me, super);
    }

    if (file->handle != -1)
    {
        close (file->handle);
        file->handle = -1;
    }

    vfs_s_free_inode (me, file->ino);
    vfs_s_free_fh (sub, fh);

    return res;
}

/* --------------------------------------------------------------------------------------------- */

static void
vfs_s_print_stats (const char *fs_name, const char *action,
                   const char *file_name, off_t have, off_t need)
{
    if (need != 0)
        vfs_print_message (_("%s: %s: %s %3d%% (%lld) bytes transferred"), fs_name, action,
                           file_name, (int) ((double) have * 100 / need), (long long) have);
    else
        vfs_print_message (_("%s: %s: %s %lld bytes transferred"), fs_name, action, file_name,
                           (long long) have);
}

/* --------------------------------------------------------------------------------------------- */
/* ------------------------------- mc support ---------------------------- */

static void
vfs_s_fill_names (struct vfs_class *me, fill_names_f func)
{
    GList *iter;

    for (iter = VFS_SUBCLASS (me)->supers; iter != NULL; iter = g_list_next (iter))
    {
        const struct vfs_s_super *super = (const struct vfs_s_super *) iter->data;
        char *name;

        name = g_strconcat (super->name, PATH_SEP_STR, me->prefix, VFS_PATH_URL_DELIMITER,
                            /* super->current_dir->name, */ (char *) NULL);
        func (name);
        g_free (name);
    }
}

/* --------------------------------------------------------------------------------------------- */

static int
vfs_s_ferrno (struct vfs_class *me)
{
    return me->verrno;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Get local copy of the given file.  We reuse the existing file cache
 * for remote filesystems.  Archives use standard VFS facilities.
 */

static vfs_path_t *
vfs_s_getlocalcopy (const vfs_path_t * vpath)
{
    vfs_file_handler_t *fh;
    vfs_path_t *local = NULL;

    if (vpath == NULL)
        return NULL;

    fh = vfs_s_open (vpath, O_RDONLY, 0);

    if (fh != NULL)
    {
        const struct vfs_class *me;

        me = vfs_path_get_by_index (vpath, -1)->class;
        if ((me->flags & VFSF_USETMP) != 0 && fh->ino != NULL)
            local = vfs_path_from_str_flags (fh->ino->localname, VPF_NO_CANON);

        vfs_s_close (fh);
    }

    return local;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Return the local copy.  Since we are using our cache, we do nothing -
 * the cache will be removed when the archive is closed.
 */

static int
vfs_s_ungetlocalcopy (const vfs_path_t * vpath, const vfs_path_t * local, gboolean has_changed)
{
    (void) vpath;
    (void) local;
    (void) has_changed;
    return 0;
}

/* --------------------------------------------------------------------------------------------- */

static int
vfs_s_setctl (const vfs_path_t * vpath, int ctlop, void *arg)
{
    const vfs_path_element_t *path_element;

    path_element = vfs_path_get_by_index (vpath, -1);

    switch (ctlop)
    {
    case VFS_SETCTL_STALE_DATA:
        {
            struct vfs_s_inode *ino;

            ino = vfs_s_inode_from_path (vpath, 0);
            if (ino == NULL)
                return 0;
            if (arg != NULL)
                ino->super->want_stale = TRUE;
            else
            {
                ino->super->want_stale = FALSE;
                vfs_s_invalidate (path_element->class, ino->super);
            }
            return 1;
        }
    case VFS_SETCTL_LOGFILE:
        path_element->class->logfile = fopen ((char *) arg, "w");
        return 1;
    case VFS_SETCTL_FLUSH:
        path_element->class->flush = TRUE;
        return 1;
    default:
        return 0;
    }
}

/* --------------------------------------------------------------------------------------------- */
/* ----------------------------- Stamping support -------------------------- */

static vfsid
vfs_s_getid (const vfs_path_t * vpath)
{
    struct vfs_s_super *archive = NULL;
    const char *p;

    p = vfs_s_get_path (vpath, &archive, FL_NO_OPEN);
    if (p == NULL)
        return NULL;

    return (vfsid) archive;
}

/* --------------------------------------------------------------------------------------------- */

static gboolean
vfs_s_nothingisopen (vfsid id)
{
    return (VFS_SUPER (id)->fd_usage <= 0);
}

/* --------------------------------------------------------------------------------------------- */

static void
vfs_s_free (vfsid id)
{
    vfs_s_free_super (VFS_SUPER (id)->me, VFS_SUPER (id));
}

/* --------------------------------------------------------------------------------------------- */

static gboolean
vfs_s_dir_uptodate (struct vfs_class *me, struct vfs_s_inode *ino)
{
    gint64 tim;

    if (me->flush)
    {
        me->flush = FALSE;
        return 0;
    }

    tim = g_get_real_time ();

    return (tim < ino->timestamp);
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

struct vfs_s_inode *
vfs_s_new_inode (struct vfs_class *me, struct vfs_s_super *super, struct stat *initstat)
{
    struct vfs_s_inode *ino;

    ino = g_try_new0 (struct vfs_s_inode, 1);
    if (ino == NULL)
        return NULL;

    if (initstat != NULL)
        ino->st = *initstat;
    ino->super = super;
    ino->subdir = g_queue_new ();
    ino->st.st_nlink = 0;
    ino->st.st_ino = VFS_SUBCLASS (me)->inode_counter++;
    ino->st.st_dev = VFS_SUBCLASS (me)->rdev;

    super->ino_usage++;

    CALL (init_inode) (me, ino);

    return ino;
}

/* --------------------------------------------------------------------------------------------- */

void
vfs_s_free_inode (struct vfs_class *me, struct vfs_s_inode *ino)
{
    if (ino == NULL)
        vfs_die ("Don't pass NULL to me");

    /* ==0 can happen if freshly created entry is deleted */
    if (ino->st.st_nlink > 1)
    {
        ino->st.st_nlink--;
        return;
    }

    while (g_queue_get_length (ino->subdir) != 0)
    {
        struct vfs_s_entry *entry;

        entry = VFS_ENTRY (g_queue_peek_head (ino->subdir));
        vfs_s_free_entry (me, entry);
    }

    g_queue_free (ino->subdir);
    ino->subdir = NULL;

    CALL (free_inode) (me, ino);
    g_free (ino->linkname);
    if ((me->flags & VFSF_USETMP) != 0 && ino->localname != NULL)
    {
        unlink (ino->localname);
        g_free (ino->localname);
    }
    ino->super->ino_usage--;
    g_free (ino);
}

/* --------------------------------------------------------------------------------------------- */

struct vfs_s_entry *
vfs_s_new_entry (struct vfs_class *me, const char *name, struct vfs_s_inode *inode)
{
    struct vfs_s_entry *entry;

    entry = g_new0 (struct vfs_s_entry, 1);

    entry->name = g_strdup (name);
    entry->ino = inode;
    entry->ino->ent = entry;
    CALL (init_entry) (me, entry);

    return entry;
}

/* --------------------------------------------------------------------------------------------- */

void
vfs_s_free_entry (struct vfs_class *me, struct vfs_s_entry *ent)
{
    if (ent->dir != NULL)
        g_queue_remove (ent->dir->subdir, ent);

    MC_PTR_FREE (ent->name);

    if (ent->ino != NULL)
    {
        ent->ino->ent = NULL;
        vfs_s_free_inode (me, ent->ino);
    }

    g_free (ent);
}

/* --------------------------------------------------------------------------------------------- */

void
vfs_s_insert_entry (struct vfs_class *me, struct vfs_s_inode *dir, struct vfs_s_entry *ent)
{
    (void) me;

    ent->dir = dir;

    ent->ino->st.st_nlink++;
    g_queue_push_tail (dir->subdir, ent);
}

/* --------------------------------------------------------------------------------------------- */

int
vfs_s_entry_compare (const void *a, const void *b)
{
    const struct vfs_s_entry *e = (const struct vfs_s_entry *) a;
    const char *name = (const char *) b;

    return strcmp (e->name, name);
}

/* --------------------------------------------------------------------------------------------- */

struct stat *
vfs_s_default_stat (struct vfs_class *me, mode_t mode)
{
    static struct stat st;
    mode_t myumask;

    (void) me;

    myumask = umask (022);
    umask (myumask);
    mode &= ~myumask;

    st.st_mode = mode;
    st.st_ino = 0;
    st.st_dev = 0;
#ifdef HAVE_STRUCT_STAT_ST_RDEV
    st.st_rdev = 0;
#endif
    st.st_uid = getuid ();
    st.st_gid = getgid ();
#ifdef HAVE_STRUCT_STAT_ST_BLKSIZE
    st.st_blksize = 512;
#endif
    st.st_size = 0;

    st.st_mtime = st.st_atime = st.st_ctime = time (NULL);
#ifdef HAVE_STRUCT_STAT_ST_MTIM
    st.st_atim.tv_nsec = st.st_mtim.tv_nsec = st.st_ctim.tv_nsec = 0;
#endif

    vfs_adjust_stat (&st);

    return &st;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Calculate number of st_blocks using st_size and st_blksize.
 * In according to stat(2), st_blocks is the size in 512-byte units.
 *
 * @param s stat info
 */

void
vfs_adjust_stat (struct stat *s)
{
#ifdef HAVE_STRUCT_STAT_ST_BLOCKS
    if (s->st_size == 0)
        s->st_blocks = 0;
    else
    {
#ifdef HAVE_STRUCT_STAT_ST_BLKSIZE
        blkcnt_t ioblocks;
        blksize_t ioblock_size;

        /* 1. Calculate how many IO blocks are occupied */
        ioblocks = 1 + (s->st_size - 1) / s->st_blksize;
        /* 2. Calculate size of st_blksize in 512-byte units */
        ioblock_size = 1 + (s->st_blksize - 1) / 512;
        /* 3. Calculate number of blocks */
        s->st_blocks = ioblocks * ioblock_size;
#else
        /* Let IO block size is 512 bytes */
        s->st_blocks = 1 + (s->st_size - 1) / 512;
#endif /* HAVE_STRUCT_STAT_ST_BLKSIZE */
    }
#else
    (void)s;
#endif /* HAVE_STRUCT_STAT_ST_BLOCKS */
}

/* --------------------------------------------------------------------------------------------- */

struct vfs_s_entry *
vfs_s_generate_entry (struct vfs_class *me, const char *name, struct vfs_s_inode *parent,
                      mode_t mode)
{
    struct vfs_s_inode *inode;
    struct stat *st;

    st = vfs_s_default_stat (me, mode);
    inode = vfs_s_new_inode (me, parent->super, st);

    return vfs_s_new_entry (me, name, inode);
}

/* --------------------------------------------------------------------------------------------- */

struct vfs_s_inode *
vfs_s_find_inode (struct vfs_class *me, const struct vfs_s_super *super,
                  const char *path, int follow, int flags)
{
    struct vfs_s_entry *ent;

    if (((me->flags & VFSF_REMOTE) == 0) && (*path == '\0'))
        return super->root;

    ent = VFS_SUBCLASS (me)->find_entry (me, super->root, path, follow, flags);
    return (ent != NULL ? ent->ino : NULL);
}

/* --------------------------------------------------------------------------------------------- */
/* Ook, these were functions around directory entries / inodes */
/* -------------------------------- superblock games -------------------------- */
/**
 * get superlock object by vpath
 *
 * @param vpath path
 * @return superlock object or NULL if not found
 */

struct vfs_s_super *
vfs_get_super_by_vpath (const vfs_path_t * vpath)
{
    GList *iter;
    void *cookie = NULL;
    const vfs_path_element_t *path_element;
    struct vfs_s_subclass *subclass;
    struct vfs_s_super *super = NULL;
    vfs_path_t *vpath_archive;

    path_element = vfs_path_get_by_index (vpath, -1);
    subclass = VFS_SUBCLASS (path_element->class);

    vpath_archive = vfs_path_clone (vpath);
    vfs_path_remove_element_by_index (vpath_archive, -1);

    if (subclass->archive_check != NULL)
    {
        cookie = subclass->archive_check (vpath_archive);
        if (cookie == NULL)
            goto ret;
    }

    if (subclass->archive_same == NULL)
        goto ret;

    for (iter = subclass->supers; iter != NULL; iter = g_list_next (iter))
    {
        int i;

        super = VFS_SUPER (iter->data);

        /* 0 == other, 1 == same, return it, 2 == other but stop scanning */
        i = subclass->archive_same (path_element, super, vpath_archive, cookie);
        if (i == 1)
            goto ret;
        if (i != 0)
            break;

        super = NULL;
    }

  ret:
    vfs_path_free (vpath_archive, TRUE);
    return super;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * get path from last VFS-element and create corresponding superblock
 *
 * @param vpath source path object
 * @param archive pointer to object for store newly created superblock
 * @param flags flags
 *
 * @return path from last VFS-element
 */
const char *
vfs_s_get_path (const vfs_path_t * vpath, struct vfs_s_super **archive, int flags)
{
    const char *retval = "";
    int result = -1;
    struct vfs_s_super *super;
    const vfs_path_element_t *path_element;
    struct vfs_s_subclass *subclass;

    path_element = vfs_path_get_by_index (vpath, -1);

    if (path_element->path != NULL)
        retval = path_element->path;

    super = vfs_get_super_by_vpath (vpath);
    if (super != NULL)
        goto return_success;

    if ((flags & FL_NO_OPEN) != 0)
    {
        path_element->class->verrno = EIO;
        return NULL;
    }

    subclass = VFS_SUBCLASS (path_element->class);

    super = subclass->new_archive != NULL ?
        subclass->new_archive (path_element->class) : vfs_s_new_super (path_element->class);

    if (subclass->open_archive != NULL)
    {
        vfs_path_t *vpath_archive;

        vpath_archive = vfs_path_clone (vpath);
        vfs_path_remove_element_by_index (vpath_archive, -1);

        result = subclass->open_archive (super, vpath_archive, path_element);
        vfs_path_free (vpath_archive, TRUE);
    }
    if (result == -1)
    {
        vfs_s_free_super (path_element->class, super);
        path_element->class->verrno = EIO;
        return NULL;
    }
    if (super->name == NULL)
        vfs_die ("You have to fill name\n");
    if (super->root == NULL)
        vfs_die ("You have to fill root inode\n");

    vfs_s_insert_super (path_element->class, super);
    vfs_stamp_create (path_element->class, super);

  return_success:
    *archive = super;
    return retval;
}

/* --------------------------------------------------------------------------------------------- */

void
vfs_s_invalidate (struct vfs_class *me, struct vfs_s_super *super)
{
    if (!super->want_stale)
    {
        vfs_s_free_inode (me, super->root);
        super->root = vfs_s_new_inode (me, super, vfs_s_default_stat (me, S_IFDIR | 0755));
    }
}

/* --------------------------------------------------------------------------------------------- */

char *
vfs_s_fullpath (struct vfs_class *me, struct vfs_s_inode *ino)
{
    if (ino->ent == NULL)
        ERRNOR (EAGAIN, NULL);

    if ((me->flags & VFSF_USETMP) == 0)
    {
        /* archives */
        char *path;

        path = g_strdup (ino->ent->name);

        while (TRUE)
        {
            char *newpath;

            ino = ino->ent->dir;
            if (ino == ino->super->root)
                break;

            newpath = g_strconcat (ino->ent->name, PATH_SEP_STR, path, (char *) NULL);
            g_free (path);
            path = newpath;
        }
        return path;
    }

    /* remote systems */
    if (ino->ent->dir == NULL || ino->ent->dir->ent == NULL)
        return g_strdup (ino->ent->name);

    return g_strconcat (ino->ent->dir->ent->name, PATH_SEP_STR, ino->ent->name, (char *) NULL);
}

/* --------------------------------------------------------------------------------------------- */

void
vfs_s_init_fh (vfs_file_handler_t * fh, struct vfs_s_inode *ino, gboolean changed)
{
    fh->ino = ino;
    fh->handle = -1;
    fh->changed = changed;
    fh->linear = LS_NOT_LINEAR;
}

/* --------------------------------------------------------------------------------------------- */
/* --------------------------- stat and friends ---------------------------- */

void *
vfs_s_open (const vfs_path_t * vpath, int flags, mode_t mode)
{
    gboolean was_changed = FALSE;
    vfs_file_handler_t *fh;
    struct vfs_s_super *super;
    const char *q;
    struct vfs_s_inode *ino;
    const vfs_path_element_t *path_element;
    struct vfs_s_subclass *s;

    q = vfs_s_get_path (vpath, &super, 0);
    if (q == NULL)
        return NULL;

    path_element = vfs_path_get_by_index (vpath, -1);

    ino = vfs_s_find_inode (path_element->class, super, q, LINK_FOLLOW, FL_NONE);
    if (ino != NULL && (flags & (O_CREAT | O_EXCL)) == (O_CREAT | O_EXCL))
    {
        path_element->class->verrno = EEXIST;
        return NULL;
    }

    s = VFS_SUBCLASS (path_element->class);

    if (ino == NULL)
    {
        char *dirname, *name;
        struct vfs_s_entry *ent;
        struct vfs_s_inode *dir;

        /* If the filesystem is read-only, disable file creation */
        if ((flags & O_CREAT) == 0 || path_element->class->write == NULL)
            return NULL;

        dirname = g_path_get_dirname (q);
        name = g_path_get_basename (q);
        dir = vfs_s_find_inode (path_element->class, super, dirname, LINK_FOLLOW, FL_DIR);
        if (dir == NULL)
        {
            g_free (dirname);
            g_free (name);
            return NULL;
        }

        ent = vfs_s_generate_entry (path_element->class, name, dir, 0755);
        ino = ent->ino;
        vfs_s_insert_entry (path_element->class, dir, ent);
        if ((VFS_CLASS (s)->flags & VFSF_USETMP) != 0)
        {
            int tmp_handle;
            vfs_path_t *tmp_vpath;

            tmp_handle = vfs_mkstemps (&tmp_vpath, path_element->class->name, name);
            ino->localname = vfs_path_free (tmp_vpath, FALSE);
            if (tmp_handle == -1)
            {
                g_free (dirname);
                g_free (name);
                return NULL;
            }

            close (tmp_handle);
        }
        g_free (dirname);
        g_free (name);
        was_changed = TRUE;
    }

    if (S_ISDIR (ino->st.st_mode))
    {
        path_element->class->verrno = EISDIR;
        return NULL;
    }

    fh = s->fh_new != NULL ? s->fh_new (ino, was_changed) : vfs_s_new_fh (ino, was_changed);

    if (IS_LINEAR (flags))
    {
        if (s->linear_start != NULL)
        {
            vfs_print_message ("%s", _("Starting linear transfer..."));
            fh->linear = LS_LINEAR_PREOPEN;
        }
    }
    else
    {
        if (s->fh_open != NULL && s->fh_open (path_element->class, fh, flags, mode) != 0)
        {
            vfs_s_free_fh (s, fh);
            return NULL;
        }
    }

    if ((VFS_CLASS (s)->flags & VFSF_USETMP) != 0 && fh->ino->localname != NULL)
    {
        fh->handle = open (fh->ino->localname, NO_LINEAR (flags), mode);
        if (fh->handle == -1)
        {
            vfs_s_free_fh (s, fh);
            path_element->class->verrno = errno;
            return NULL;
        }
    }

    /* i.e. we had no open files and now we have one */
    vfs_rmstamp (path_element->class, (vfsid) super);
    super->fd_usage++;
    fh->ino->st.st_nlink++;
    return fh;
}

/* --------------------------------------------------------------------------------------------- */

int
vfs_s_stat (const vfs_path_t * vpath, struct stat *buf)
{
    return vfs_s_internal_stat (vpath, buf, FL_FOLLOW);
}

/* --------------------------------------------------------------------------------------------- */

int
vfs_s_lstat (const vfs_path_t * vpath, struct stat *buf)
{
    return vfs_s_internal_stat (vpath, buf, FL_NONE);
}

/* --------------------------------------------------------------------------------------------- */

int
vfs_s_fstat (void *fh, struct stat *buf)
{
    *buf = VFS_FILE_HANDLER (fh)->ino->st;
    return 0;
}

/* --------------------------------------------------------------------------------------------- */

int
vfs_s_retrieve_file (struct vfs_class *me, struct vfs_s_inode *ino)
{
    /* If you want reget, you'll have to open file with O_LINEAR */
    off_t total = 0;
    char buffer[8192];
    int handle;
    ssize_t n;
    off_t stat_size = ino->st.st_size;
    vfs_file_handler_t *fh = NULL;
    vfs_path_t *tmp_vpath;
    struct vfs_s_subclass *s = VFS_SUBCLASS (me);

    if ((me->flags & VFSF_USETMP) == 0)
        return (-1);

    handle = vfs_mkstemps (&tmp_vpath, me->name, ino->ent->name);
    ino->localname = vfs_path_free (tmp_vpath, FALSE);
    if (handle == -1)
    {
        me->verrno = errno;
        goto error_4;
    }

    fh = s->fh_new != NULL ? s->fh_new (ino, FALSE) : vfs_s_new_fh (ino, FALSE);

    if (s->linear_start (me, fh, 0) == 0)
        goto error_3;

    /* Clear the interrupt status */
    tty_got_interrupt ();
    tty_enable_interrupt_key ();

    while ((n = s->linear_read (me, fh, buffer, sizeof (buffer))) != 0)
    {
        int t;

        if (n < 0)
            goto error_1;

        total += n;
        vfs_s_print_stats (me->name, _("Getting file"), ino->ent->name, total, stat_size);

        if (tty_got_interrupt ())
            goto error_1;

        t = write (handle, buffer, n);
        if (t != n)
        {
            if (t == -1)
                me->verrno = errno;
            goto error_1;
        }
    }
    s->linear_close (me, fh);
    close (handle);

    tty_disable_interrupt_key ();
    vfs_s_free_fh (s, fh);
    return 0;

  error_1:
    s->linear_close (me, fh);
  error_3:
    tty_disable_interrupt_key ();
    close (handle);
    unlink (ino->localname);
  error_4:
    MC_PTR_FREE (ino->localname);
    if (fh != NULL)
        vfs_s_free_fh (s, fh);
    return (-1);
}

/* --------------------------------------------------------------------------------------------- */
/* ----------------------------- Stamping support -------------------------- */

/* Initialize one of our subclasses - fill common functions */
void
vfs_init_class (struct vfs_class *vclass, const char *name, vfs_flags_t flags, const char *prefix)
{
    memset (vclass, 0, sizeof (struct vfs_class));

    vclass->name = name;
    vclass->flags = flags;
    vclass->prefix = prefix;

    vclass->fill_names = vfs_s_fill_names;
    vclass->open = vfs_s_open;
    vclass->close = vfs_s_close;
    vclass->read = vfs_s_read;
    if ((vclass->flags & VFSF_READONLY) == 0)
        vclass->write = vfs_s_write;
    vclass->opendir = vfs_s_opendir;
    vclass->readdir = vfs_s_readdir;
    vclass->closedir = vfs_s_closedir;
    vclass->stat = vfs_s_stat;
    vclass->lstat = vfs_s_lstat;
    vclass->fstat = vfs_s_fstat;
    vclass->readlink = vfs_s_readlink;
    vclass->chdir = vfs_s_chdir;
    vclass->ferrno = vfs_s_ferrno;
    vclass->lseek = vfs_s_lseek;
    vclass->getid = vfs_s_getid;
    vclass->nothingisopen = vfs_s_nothingisopen;
    vclass->free = vfs_s_free;
    vclass->setctl = vfs_s_setctl;
    if ((vclass->flags & VFSF_USETMP) != 0)
    {
        vclass->getlocalcopy = vfs_s_getlocalcopy;
        vclass->ungetlocalcopy = vfs_s_ungetlocalcopy;
    }
}

/* --------------------------------------------------------------------------------------------- */

void
vfs_init_subclass (struct vfs_s_subclass *sub, const char *name, vfs_flags_t flags,
                   const char *prefix)
{
    struct vfs_class *vclass = VFS_CLASS (sub);
    size_t len;
    char *start;

    vfs_init_class (vclass, name, flags, prefix);

    len = sizeof (struct vfs_s_subclass) - sizeof (struct vfs_class);
    start = (char *) sub + sizeof (struct vfs_class);
    memset (start, 0, len);

    if ((vclass->flags & VFSF_USETMP) != 0)
        sub->find_entry = vfs_s_find_entry_linear;
    else if ((vclass->flags & VFSF_REMOTE) != 0)
        sub->find_entry = vfs_s_find_entry_linear;
    else
        sub->find_entry = vfs_s_find_entry_tree;
    sub->dir_uptodate = vfs_s_dir_uptodate;
}

/* --------------------------------------------------------------------------------------------- */
/** Find VFS id for given directory name */

vfsid
vfs_getid (const vfs_path_t * vpath)
{
    const vfs_path_element_t *path_element;

    path_element = vfs_path_get_by_index (vpath, -1);
    if (!vfs_path_element_valid (path_element) || path_element->class->getid == NULL)
        return NULL;

    return (*path_element->class->getid) (vpath);
}

/* --------------------------------------------------------------------------------------------- */
/* ----------- Utility functions for networked filesystems  -------------- */

#ifdef ENABLE_VFS_NET
int
vfs_s_select_on_two (int fd1, int fd2)
{
    fd_set set;
    struct timeval time_out;
    int v;
    int maxfd = MAX (fd1, fd2) + 1;

    time_out.tv_sec = 1;
    time_out.tv_usec = 0;
    FD_ZERO (&set);
    FD_SET (fd1, &set);
    FD_SET (fd2, &set);

    v = select (maxfd, &set, 0, 0, &time_out);
    if (v <= 0)
        return v;
    if (FD_ISSET (fd1, &set))
        return 1;
    if (FD_ISSET (fd2, &set))
        return 2;
    return (-1);
}

/* --------------------------------------------------------------------------------------------- */

int
vfs_s_get_line (struct vfs_class *me, int sock, char *buf, int buf_len, char term)
{
    FILE *logfile = me->logfile;
    int i;
    char c;

    for (i = 0; i < buf_len - 1; i++, buf++)
    {
        if (read (sock, buf, sizeof (char)) <= 0)
            return 0;

        if (logfile != NULL)
        {
            size_t ret1;
            int ret2;

            ret1 = fwrite (buf, 1, 1, logfile);
            ret2 = fflush (logfile);
            (void) ret1;
            (void) ret2;
        }

        if (*buf == term)
        {
            *buf = '\0';
            return 1;
        }
    }

    /* Line is too long - terminate buffer and discard the rest of line */
    *buf = '\0';
    while (read (sock, &c, sizeof (c)) > 0)
    {
        if (logfile != NULL)
        {
            size_t ret1;
            int ret2;

            ret1 = fwrite (&c, 1, 1, logfile);
            ret2 = fflush (logfile);
            (void) ret1;
            (void) ret2;
        }
        if (c == '\n')
            return 1;
    }
    return 0;
}

/* --------------------------------------------------------------------------------------------- */

int
vfs_s_get_line_interruptible (struct vfs_class *me, char *buffer, int size, int fd)
{
    int i;
    int res = 0;

    (void) me;

    tty_enable_interrupt_key ();

    for (i = 0; i < size - 1; i++)
    {
        ssize_t n;

        n = read (fd, &buffer[i], 1);
        if (n == -1 && errno == EINTR)
        {
            buffer[i] = '\0';
            res = EINTR;
            goto ret;
        }
        if (n == 0)
        {
            buffer[i] = '\0';
            goto ret;
        }
        if (buffer[i] == '\n')
        {
            buffer[i] = '\0';
            res = 1;
            goto ret;
        }
    }

    buffer[size - 1] = '\0';

  ret:
    tty_disable_interrupt_key ();

    return res;
}
#endif /* ENABLE_VFS_NET */

/* --------------------------------------------------------------------------------------------- */
/**
 * Normalize filenames start position
 */

void
vfs_s_normalize_filename_leading_spaces (struct vfs_s_inode *root_inode, size_t final_num_spaces)
{
    GList *iter;

    for (iter = g_queue_peek_head_link (root_inode->subdir); iter != NULL;
         iter = g_list_next (iter))
    {
        struct vfs_s_entry *entry = VFS_ENTRY (iter->data);

        if ((size_t) entry->ino->data_offset > final_num_spaces)
        {
            char *source_name, *spacer;

            source_name = entry->name;
            spacer = g_strnfill (entry->ino->data_offset - final_num_spaces, ' ');
            entry->name = g_strconcat (spacer, source_name, (char *) NULL);
            g_free (spacer);
            g_free (source_name);
        }

        entry->ino->data_offset = -1;
    }
}

/* --------------------------------------------------------------------------------------------- */
