/*
   Virtual File System: interface functions

   Copyright (C) 2011-2021
   Free Software Foundation, Inc.

   Written by:
   Slava Zanko <slavazanko@gmail.com>, 2011, 2013

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
 * \brief Source: Virtual File System: path handlers
 * \author Slava Zanko
 * \date 2011
 */


#include <config.h>

#include <stdio.h>
#include <stdlib.h>             /* For atol() */
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include <ctype.h>              /* is_digit() */
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>

#include "lib/global.h"

#include "lib/widget.h"         /* message() */
#include "lib/strutil.h"        /* str_crt_conv_from() */
#include "lib/util.h"

#include "vfs.h"
#include "utilvfs.h"
#include "path.h"
#include "gc.h"
#include "xdirentry.h"

/* TODO: move it to separate private .h */
extern GString *vfs_str_buffer;
extern vfs_class *current_vfs;
extern struct vfs_dirent *mc_readdir_result;

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

static vfs_path_t *
mc_def_getlocalcopy (const vfs_path_t * filename_vpath)
{
    vfs_path_t *tmp_vpath = NULL;
    int fdin, fdout = -1;
    ssize_t i;
    char buffer[BUF_1K * 8];
    struct stat mystat;

    fdin = mc_open (filename_vpath, O_RDONLY | O_LINEAR);
    if (fdin == -1)
        goto fail;

    fdout = vfs_mkstemps (&tmp_vpath, "vfs", vfs_path_get_last_path_str (filename_vpath));
    if (fdout == -1)
        goto fail;

    while ((i = mc_read (fdin, buffer, sizeof (buffer))) > 0)
    {
        if (write (fdout, buffer, i) != i)
            goto fail;
    }
    if (i == -1)
        goto fail;
    i = mc_close (fdin);
    fdin = -1;
    if (i == -1)
        goto fail;

    i = close (fdout);
    fdout = -1;
    if (i == -1)
        goto fail;

    if (mc_stat (filename_vpath, &mystat) != -1)
        mc_chmod (tmp_vpath, mystat.st_mode);

    return tmp_vpath;

  fail:
    vfs_path_free (tmp_vpath, TRUE);
    if (fdout != -1)
        close (fdout);
    if (fdin != -1)
        mc_close (fdin);
    return NULL;
}

/* --------------------------------------------------------------------------------------------- */

static int
mc_def_ungetlocalcopy (const vfs_path_t * filename_vpath,
                       const vfs_path_t * local_vpath, gboolean has_changed)
{
    int fdin = -1, fdout = -1;
    const char *local;

    local = vfs_path_get_last_path_str (local_vpath);

    if (has_changed)
    {
        char buffer[BUF_1K * 8];
        ssize_t i;

        if (vfs_path_get_last_path_vfs (filename_vpath)->write == NULL)
            goto failed;

        fdin = open (local, O_RDONLY);
        if (fdin == -1)
            goto failed;
        fdout = mc_open (filename_vpath, O_WRONLY | O_TRUNC);
        if (fdout == -1)
            goto failed;
        while ((i = read (fdin, buffer, sizeof (buffer))) > 0)
            if (mc_write (fdout, buffer, (size_t) i) != i)
                goto failed;
        if (i == -1)
            goto failed;

        if (close (fdin) == -1)
        {
            fdin = -1;
            goto failed;
        }
        fdin = -1;
        if (mc_close (fdout) == -1)
        {
            fdout = -1;
            goto failed;
        }
    }
    unlink (local);
    return 0;

  failed:
    message (D_ERROR, _("Changes to file lost"), "%s", vfs_path_get_last_path_str (filename_vpath));
    if (fdout != -1)
        mc_close (fdout);
    if (fdin != -1)
        close (fdin);
    unlink (local);
    return (-1);
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

int
mc_open (const vfs_path_t * vpath, int flags, ...)
{
    int result = -1;
    mode_t mode = 0;
    const vfs_path_element_t *path_element;

    if (vpath == NULL)
        return (-1);

    /* Get the mode flag */
    if ((flags & O_CREAT) != 0)
    {
        va_list ap;

        va_start (ap, flags);
        /* We have to use PROMOTED_MODE_T instead of mode_t. Doing 'va_arg (ap, mode_t)'
         * fails on systems where 'mode_t' is smaller than 'int' because of C's "default
         * argument promotions". */
        mode = va_arg (ap, PROMOTED_MODE_T);
        va_end (ap);
    }

    path_element = vfs_path_get_by_index (vpath, -1);
    if (vfs_path_element_valid (path_element) && path_element->class->open != NULL)
    {
        void *info;

        /* open must be supported */
        info = path_element->class->open (vpath, flags, mode);
        if (info == NULL)
            errno = vfs_ferrno (path_element->class);
        else
            result = vfs_new_handle (path_element->class, info);
    }
    else
        errno = -EOPNOTSUPP;

    return result;
}

/* --------------------------------------------------------------------------------------------- */

/* *INDENT-OFF* */

#define MC_NAMEOP(name, inarg, callarg) \
int mc_##name inarg \
{ \
    int result; \
    const vfs_path_element_t *path_element; \
\
    if (vpath == NULL) \
        return (-1); \
\
    path_element = vfs_path_get_by_index (vpath, -1); \
    if (!vfs_path_element_valid (path_element)) \
        return (-1); \
\
    result = path_element->class->name != NULL ? path_element->class->name callarg : -1; \
    if (result == -1) \
        errno = path_element->class->name != NULL ? vfs_ferrno (path_element->class) : E_NOTSUPP; \
    return result; \
}

MC_NAMEOP (chmod, (const vfs_path_t *vpath, mode_t mode), (vpath, mode))
MC_NAMEOP (chown, (const vfs_path_t *vpath, uid_t owner, gid_t group), (vpath, owner, group))
MC_NAMEOP (utime, (const vfs_path_t *vpath, mc_timesbuf_t * times), (vpath, times))
MC_NAMEOP (readlink, (const vfs_path_t *vpath, char *buf, size_t bufsiz), (vpath, buf, bufsiz))
MC_NAMEOP (unlink, (const vfs_path_t *vpath), (vpath))
MC_NAMEOP (mkdir, (const vfs_path_t *vpath, mode_t mode), (vpath, mode))
MC_NAMEOP (rmdir, (const vfs_path_t *vpath), (vpath))
MC_NAMEOP (mknod, (const vfs_path_t *vpath, mode_t mode, dev_t dev), (vpath, mode, dev))

/* *INDENT-ON* */

/* --------------------------------------------------------------------------------------------- */

int
mc_symlink (const vfs_path_t * vpath1, const vfs_path_t * vpath2)
{
    int result = -1;

    if (vpath1 != NULL && vpath2 != NULL)
    {
        const vfs_path_element_t *path_element;

        path_element = vfs_path_get_by_index (vpath2, -1);
        if (vfs_path_element_valid (path_element))
        {
            result =
                path_element->class->symlink != NULL ?
                path_element->class->symlink (vpath1, vpath2) : -1;

            if (result == -1)
                errno =
                    path_element->class->symlink != NULL ?
                    vfs_ferrno (path_element->class) : E_NOTSUPP;
        }
    }
    return result;
}

/* --------------------------------------------------------------------------------------------- */

/* *INDENT-OFF* */

#define MC_HANDLEOP(name) \
ssize_t mc_##name (int handle, C void *buf, size_t count) \
{ \
    struct vfs_class *vfs; \
    void *fsinfo = NULL; \
    int result; \
\
    if (handle == -1) \
        return (-1); \
\
    vfs = vfs_class_find_by_handle (handle, &fsinfo); \
    if (vfs == NULL) \
        return (-1); \
\
    result = vfs->name != NULL ? vfs->name (fsinfo, buf, count) : -1; \
    if (result == -1) \
        errno = vfs->name != NULL ? vfs_ferrno (vfs) : E_NOTSUPP; \
    return result; \
}

#define C
MC_HANDLEOP (read)
#undef C
#define C const
MC_HANDLEOP (write)
#undef C

/* --------------------------------------------------------------------------------------------- */

#define MC_RENAMEOP(name) \
int mc_##name (const vfs_path_t *vpath1, const vfs_path_t *vpath2) \
{ \
    int result; \
    const vfs_path_element_t *path_element1; \
    const vfs_path_element_t *path_element2; \
\
    if (vpath1 == NULL || vpath2 == NULL) \
        return (-1); \
\
    path_element1 = vfs_path_get_by_index (vpath1, (-1)); \
    path_element2 = vfs_path_get_by_index (vpath2, (-1)); \
\
    if (!vfs_path_element_valid (path_element1) || !vfs_path_element_valid (path_element2) || \
        path_element1->class != path_element2->class) \
    { \
        errno = EXDEV; \
        return (-1); \
    } \
\
    result = path_element1->class->name != NULL \
        ? path_element1->class->name (vpath1, vpath2) : -1; \
    if (result == -1) \
        errno = path_element1->class->name != NULL ? vfs_ferrno (path_element1->class) : E_NOTSUPP; \
    return result; \
}

MC_RENAMEOP (link)
MC_RENAMEOP (rename)

/* *INDENT-ON* */

/* --------------------------------------------------------------------------------------------- */

int
mc_ctl (int handle, int ctlop, void *arg)
{
    struct vfs_class *vfs;
    void *fsinfo = NULL;

    vfs = vfs_class_find_by_handle (handle, &fsinfo);

    return (vfs == NULL || vfs->ctl == NULL) ? 0 : vfs->ctl (fsinfo, ctlop, arg);
}

/* --------------------------------------------------------------------------------------------- */

int
mc_setctl (const vfs_path_t * vpath, int ctlop, void *arg)
{
    int result = -1;
    const vfs_path_element_t *path_element;

    if (vpath == NULL)
        vfs_die ("You don't want to pass NULL to mc_setctl.");

    path_element = vfs_path_get_by_index (vpath, -1);
    if (vfs_path_element_valid (path_element))
        result =
            path_element->class->setctl != NULL ? path_element->class->setctl (vpath,
                                                                               ctlop, arg) : 0;

    return result;
}

/* --------------------------------------------------------------------------------------------- */

int
mc_close (int handle)
{
    struct vfs_class *vfs;
    void *fsinfo = NULL;
    int result;

    if (handle == -1)
        return (-1);

    vfs = vfs_class_find_by_handle (handle, &fsinfo);
    if (vfs == NULL || fsinfo == NULL)
        return (-1);

    if (handle < 3)
        return close (handle);

    if (vfs->close == NULL)
        vfs_die ("VFS must support close.\n");
    result = vfs->close (fsinfo);
    vfs_free_handle (handle);
    if (result == -1)
        errno = vfs_ferrno (vfs);

    return result;
}

/* --------------------------------------------------------------------------------------------- */

DIR *
mc_opendir (const vfs_path_t * vpath)
{
    int handle, *handlep;
    void *info;
    vfs_path_element_t *path_element;

    if (vpath == NULL)
        return NULL;

    path_element = (vfs_path_element_t *) vfs_path_get_by_index (vpath, -1);
    if (!vfs_path_element_valid (path_element))
    {
        errno = E_NOTSUPP;
        return NULL;
    }

    info = path_element->class->opendir ? path_element->class->opendir (vpath) : NULL;
    if (info == NULL)
    {
        errno = path_element->class->opendir ? vfs_ferrno (path_element->class) : E_NOTSUPP;
        return NULL;
    }

    path_element->dir.info = info;

#ifdef HAVE_CHARSET
    path_element->dir.converter = (path_element->encoding != NULL) ?
        str_crt_conv_from (path_element->encoding) : str_cnv_from_term;
    if (path_element->dir.converter == INVALID_CONV)
        path_element->dir.converter = str_cnv_from_term;
#endif

    handle = vfs_new_handle (path_element->class, vfs_path_element_clone (path_element));

    handlep = g_new (int, 1);
    *handlep = handle;
    return (DIR *) (void *)handlep;
}

/* --------------------------------------------------------------------------------------------- */

struct vfs_dirent *
mc_readdir (DIR * dirp)
{
    int handle;
    struct vfs_class *vfs;
    void *fsinfo = NULL;
    struct vfs_dirent *entry = NULL;
    vfs_path_element_t *vfs_path_element;

    if (dirp == NULL)
    {
        errno = EFAULT;
        return NULL;
    }

    handle = *(int *) dirp;

    vfs = vfs_class_find_by_handle (handle, &fsinfo);
    if (vfs == NULL || fsinfo == NULL)
        return NULL;

    vfs_path_element = (vfs_path_element_t *) fsinfo;
    if (vfs->readdir != NULL)
    {
        entry = vfs->readdir (vfs_path_element->dir.info);
        if (entry == NULL)
            return NULL;

        g_string_set_size (vfs_str_buffer, 0);
#ifdef HAVE_CHARSET
        str_vfs_convert_from (vfs_path_element->dir.converter, entry->d_name, vfs_str_buffer);
#else
        g_string_assign (vfs_str_buffer, entry->d_name);
#endif
        vfs_dirent_assign (mc_readdir_result, vfs_str_buffer->str, entry->d_ino);
        vfs_dirent_free (entry);
    }
    if (entry == NULL)
        errno = vfs->readdir ? vfs_ferrno (vfs) : E_NOTSUPP;
    return (entry != NULL) ? mc_readdir_result : NULL;
}

/* --------------------------------------------------------------------------------------------- */

int
mc_closedir (DIR * dirp)
{
    int handle;
    struct vfs_class *vfs;
    void *fsinfo = NULL;
    int result = -1;

    if (dirp == NULL)
        return result;

    handle = *(int *) dirp;

    vfs = vfs_class_find_by_handle (handle, &fsinfo);
    if (vfs != NULL && fsinfo != NULL)
    {
        vfs_path_element_t *vfs_path_element = (vfs_path_element_t *) fsinfo;

#ifdef HAVE_CHARSET
        if (vfs_path_element->dir.converter != str_cnv_from_term)
        {
            str_close_conv (vfs_path_element->dir.converter);
            vfs_path_element->dir.converter = INVALID_CONV;
        }
#endif

        result = vfs->closedir ? (*vfs->closedir) (vfs_path_element->dir.info) : -1;
        vfs_free_handle (handle);
        vfs_path_element_free (vfs_path_element);
    }
    g_free (dirp);
    return result;
}

/* --------------------------------------------------------------------------------------------- */

int
mc_stat (const vfs_path_t * vpath, struct stat *buf)
{
    int result = -1;
    const vfs_path_element_t *path_element;

    if (vpath == NULL)
        return (-1);

    path_element = vfs_path_get_by_index (vpath, -1);
    if (vfs_path_element_valid (path_element))
    {
        result = path_element->class->stat ? path_element->class->stat (vpath, buf) : -1;
        if (result == -1)
            errno = path_element->class->name ? vfs_ferrno (path_element->class) : E_NOTSUPP;
    }

    return result;
}

/* --------------------------------------------------------------------------------------------- */

int
mc_lstat (const vfs_path_t * vpath, struct stat *buf)
{
    int result = -1;
    const vfs_path_element_t *path_element;

    if (vpath == NULL)
        return (-1);

    path_element = vfs_path_get_by_index (vpath, -1);
    if (vfs_path_element_valid (path_element))
    {
        result = path_element->class->lstat ? path_element->class->lstat (vpath, buf) : -1;
        if (result == -1)
            errno = path_element->class->name ? vfs_ferrno (path_element->class) : E_NOTSUPP;
    }

    return result;
}

/* --------------------------------------------------------------------------------------------- */

int
mc_fstat (int handle, struct stat *buf)
{
    struct vfs_class *vfs;
    void *fsinfo = NULL;
    int result;

    if (handle == -1)
        return (-1);

    vfs = vfs_class_find_by_handle (handle, &fsinfo);
    if (vfs == NULL)
        return (-1);

    result = vfs->fstat ? vfs->fstat (fsinfo, buf) : -1;
    if (result == -1)
        errno = vfs->fstat ? vfs_ferrno (vfs) : E_NOTSUPP;
    return result;
}

/* --------------------------------------------------------------------------------------------- */

vfs_path_t *
mc_getlocalcopy (const vfs_path_t * pathname_vpath)
{
    vfs_path_t *result = NULL;
    const vfs_path_element_t *path_element;

    if (pathname_vpath == NULL)
        return NULL;

    path_element = vfs_path_get_by_index (pathname_vpath, -1);
    if (vfs_path_element_valid (path_element))
    {
        result = path_element->class->getlocalcopy != NULL ?
            path_element->class->getlocalcopy (pathname_vpath) :
            mc_def_getlocalcopy (pathname_vpath);
        if (result == NULL)
            errno = vfs_ferrno (path_element->class);
    }
    return result;
}

/* --------------------------------------------------------------------------------------------- */

int
mc_ungetlocalcopy (const vfs_path_t * pathname_vpath, const vfs_path_t * local_vpath,
                   gboolean has_changed)
{
    int result = -1;
    const vfs_path_element_t *path_element;

    if (pathname_vpath == NULL)
        return (-1);

    path_element = vfs_path_get_by_index (pathname_vpath, -1);
    if (vfs_path_element_valid (path_element))
        result = path_element->class->ungetlocalcopy != NULL ?
            path_element->class->ungetlocalcopy (pathname_vpath, local_vpath, has_changed) :
            mc_def_ungetlocalcopy (pathname_vpath, local_vpath, has_changed);

    return result;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * VFS chdir.
 *
 * @param vpath VFS-path
 *
 * @return 0 on success, -1 on failure.
 */

int
mc_chdir (const vfs_path_t * vpath)
{
    struct vfs_class *old_vfs;
    vfsid old_vfsid;
    int result;
    const vfs_path_element_t *path_element;
    vfs_path_t *cd_vpath;

    if (vpath == NULL)
        return (-1);

    if (vpath->relative)
        cd_vpath = vfs_path_to_absolute (vpath);
    else
        cd_vpath = vfs_path_clone (vpath);

    path_element = vfs_path_get_by_index (cd_vpath, -1);
    if (!vfs_path_element_valid (path_element) || path_element->class->chdir == NULL)
        goto error_end;


    result = path_element->class->chdir (cd_vpath);
    if (result == -1)
    {
        errno = vfs_ferrno (path_element->class);
        goto error_end;
    }

    old_vfsid = vfs_getid (vfs_get_raw_current_dir ());
    old_vfs = current_vfs;

    /* Actually change directory */
    vfs_set_raw_current_dir (cd_vpath);

    current_vfs = path_element->class;

    /* This function uses the new current_dir implicitly */
    vfs_stamp_create (old_vfs, old_vfsid);

    /* Sometimes we assume no trailing slash on cwd */
    path_element = vfs_path_get_by_index (vfs_get_raw_current_dir (), -1);
    if (vfs_path_element_valid (path_element))
    {
        if (*path_element->path != '\0')
        {
            char *p;

            p = strchr (path_element->path, 0) - 1;
            if (IS_PATH_SEP (*p) && p > path_element->path)
                *p = '\0';
        }

#ifdef ENABLE_VFS_NET
        {
            struct vfs_s_super *super;

            super = vfs_get_super_by_vpath (vpath);
            if (super != NULL && super->path_element != NULL)
            {
                g_free (super->path_element->path);
                super->path_element->path = g_strdup (path_element->path);
            }
        }
#endif /* ENABLE_VFS_NET */
    }

    return 0;

  error_end:
    vfs_path_free (cd_vpath, TRUE);
    return (-1);
}

/* --------------------------------------------------------------------------------------------- */

off_t
mc_lseek (int fd, off_t offset, int whence)
{
    struct vfs_class *vfs;
    void *fsinfo = NULL;
    off_t result;

    if (fd == -1)
        return (-1);

    vfs = vfs_class_find_by_handle (fd, &fsinfo);
    if (vfs == NULL)
        return (-1);

    result = vfs->lseek ? vfs->lseek (fsinfo, offset, whence) : -1;
    if (result == -1)
        errno = vfs->lseek ? vfs_ferrno (vfs) : E_NOTSUPP;
    return result;
}

/* --------------------------------------------------------------------------------------------- */
/* Following code heavily borrows from libiberty, mkstemps.c */
/*
 * Arguments:
 * pname (output) - pointer to the name of the temp file (needs g_free).
 *                  NULL if the function fails.
 * prefix - part of the filename before the random part.
 *          Prepend $TMPDIR or /tmp if there are no path separators.
 * suffix - if not NULL, part of the filename after the random part.
 *
 * Result:
 * handle of the open file or -1 if couldn't open any.
 */

int
mc_mkstemps (vfs_path_t ** pname_vpath, const char *prefix, const char *suffix)
{
    char *p1, *p2;
    int fd;

    if (strchr (prefix, PATH_SEP) != NULL)
        p1 = g_strdup (prefix);
    else
    {
        /* Add prefix first to find the position of XXXXXX */
        p1 = g_build_filename (mc_tmpdir (), prefix, (char *) NULL);
    }

    p2 = g_strconcat (p1, "XXXXXX", suffix, (char *) NULL);
    g_free (p1);

    fd = g_mkstemp (p2);
    if (fd >= 0)
        *pname_vpath = vfs_path_from_str (p2);
    else
    {
        *pname_vpath = NULL;
        fd = -1;
    }

    g_free (p2);

    return fd;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Return the directory where mc should keep its temporary files.
 * This directory is (in Bourne shell terms) "${TMPDIR=/tmp}/mc-$USER"
 * When called the first time, the directory is created if needed.
 * The first call should be done early, since we are using fprintf()
 * and not message() to report possible problems.
 */

const char *
mc_tmpdir (void)
{
    static char buffer[PATH_MAX];
    static const char *tmpdir = NULL;
    const char *sys_tmp;
    struct passwd *pwd;
    struct stat st;
    const char *error = NULL;

    /* Check if already correctly initialized */
    if (tmpdir != NULL && lstat (tmpdir, &st) == 0 && S_ISDIR (st.st_mode) &&
        st.st_uid == getuid () && (st.st_mode & 0777) == 0700)
        return tmpdir;

    sys_tmp = getenv ("MC_TMPDIR");
    if (sys_tmp == NULL || !IS_PATH_SEP (sys_tmp[0]))
    {
        sys_tmp = getenv ("TMPDIR");
        if (sys_tmp == NULL || !IS_PATH_SEP (sys_tmp[0]))
            sys_tmp = TMPDIR_DEFAULT;
    }

    pwd = getpwuid (getuid ());
    if (pwd != NULL)
        g_snprintf (buffer, sizeof (buffer), "%s/mc-%s", sys_tmp, pwd->pw_name);
    else
        g_snprintf (buffer, sizeof (buffer), "%s/mc-%lu", sys_tmp, (unsigned long) getuid ());

    canonicalize_pathname (buffer);

    /* Try to create directory */
    if (mkdir (buffer, S_IRWXU) != 0)
    {
        if (errno == EEXIST && lstat (buffer, &st) == 0)
        {
            /* Sanity check for existing directory */
            if (!S_ISDIR (st.st_mode))
                error = _("%s is not a directory\n");
            else if (st.st_uid != getuid ())
                error = _("Directory %s is not owned by you\n");
            else if (((st.st_mode & 0777) != 0700) && (chmod (buffer, 0700) != 0))
                error = _("Cannot set correct permissions for directory %s\n");
        }
        else
        {
            fprintf (stderr,
                     _("Cannot create temporary directory %s: %s\n"),
                     buffer, unix_error_string (errno));
            error = "";
        }
    }

    if (error != NULL)
    {
        int test_fd;
        char *fallback_prefix;
        gboolean fallback_ok = FALSE;
        vfs_path_t *test_vpath;

        if (*error != '\0')
            fprintf (stderr, error, buffer);

        /* Test if sys_tmp is suitable for temporary files */
        fallback_prefix = g_strdup_printf ("%s/mctest", sys_tmp);
        test_fd = mc_mkstemps (&test_vpath, fallback_prefix, NULL);
        g_free (fallback_prefix);
        if (test_fd != -1)
        {
            close (test_fd);
            test_fd = open (vfs_path_as_str (test_vpath), O_RDONLY);
            if (test_fd != -1)
            {
                close (test_fd);
                unlink (vfs_path_as_str (test_vpath));
                fallback_ok = TRUE;
            }
        }

        if (fallback_ok)
        {
            fprintf (stderr, _("Temporary files will be created in %s\n"), sys_tmp);
            g_snprintf (buffer, sizeof (buffer), "%s", sys_tmp);
            error = NULL;
        }
        else
        {
            fprintf (stderr, _("Temporary files will not be created\n"));
            g_snprintf (buffer, sizeof (buffer), "%s", "/dev/null/");
        }

        vfs_path_free (test_vpath, TRUE);
        fprintf (stderr, "%s\n", _("Press any key to continue..."));
        getc (stdin);
    }

    tmpdir = buffer;

    if (error == NULL)
        g_setenv ("MC_TMPDIR", tmpdir, TRUE);

    return tmpdir;
}

/* --------------------------------------------------------------------------------------------- */
