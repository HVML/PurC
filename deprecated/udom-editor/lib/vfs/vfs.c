/*
   Virtual File System switch code

   Copyright (C) 1995-2021
   Free Software Foundation, Inc.

   Written by: 1995 Miguel de Icaza
   Jakub Jelinek, 1995
   Pavel Machek, 1998
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
 */

/**
 * \file
 * \brief Source: Virtual File System switch code
 * \author Miguel de Icaza
 * \author Jakub Jelinek
 * \author Pavel Machek
 * \date 1995, 1998
 * \warning functions like extfs_lstat() have right to destroy any
 * strings you pass to them. This is acutally ok as you g_strdup what
 * you are passing to them, anyway; still, beware.
 *
 * Namespace: exports *many* functions with vfs_ prefix; exports
 * parse_ls_lga and friends which do not have that prefix.
 */

#include <config.h>

#include <errno.h>
#include <stdlib.h>

#ifdef __linux__
#ifdef HAVE_LINUX_FS_H
#include <linux/fs.h>
#endif /* HAVE_LINUX_FS_H */
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif /* HAVE_SYS_IOCTL_H */
#endif /* __linux__ */

#include "lib/global.h"
#include "lib/strutil.h"
#include "lib/util.h"
#include "lib/widget.h"         /* message() */
#include "lib/event.h"

#ifdef HAVE_CHARSET
#include "lib/charsets.h"
#endif

#include "vfs.h"
#include "utilvfs.h"
#include "gc.h"

/* TODO: move it to the separate .h */
extern struct vfs_dirent *mc_readdir_result;
extern GPtrArray *vfs__classes_list;
extern GString *vfs_str_buffer;
extern vfs_class *current_vfs;

/*** global variables ****************************************************************************/

struct vfs_dirent *mc_readdir_result = NULL;
GPtrArray *vfs__classes_list = NULL;
GString *vfs_str_buffer = NULL;
vfs_class *current_vfs = NULL;

/*** file scope macro definitions ****************************************************************/

#define VFS_FIRST_HANDLE 100

/*** file scope type declarations ****************************************************************/

struct vfs_openfile
{
    int handle;
    vfs_class *vclass;
    void *fsinfo;
};

/*** file scope variables ************************************************************************/

/** They keep track of the current directory */
static vfs_path_t *current_path = NULL;

static GPtrArray *vfs_openfiles = NULL;
static long vfs_free_handle_list = -1;

/* --------------------------------------------------------------------------------------------- */
/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */
/* now used only by vfs_translate_path, but could be used in other vfs 
 * plugin to automatic detect encoding
 * path - path to translate
 * size - how many bytes from path translate
 * defcnv - convertor, that is used as default, when path does not contain any
 *          #enc: subtring
 * buffer - used to store result of translation
 */

static estr_t
_vfs_translate_path (const char *path, int size, GIConv defcnv, GString * buffer)
{
    estr_t state = ESTR_SUCCESS;
#ifdef HAVE_CHARSET
    const char *semi;

    if (size == 0)
        return ESTR_SUCCESS;

    size = (size > 0) ? size : (signed int) strlen (path);

    /* try found /#enc: */
    semi = g_strrstr_len (path, size, VFS_ENCODING_PREFIX);
    if (semi != NULL && (semi == path || IS_PATH_SEP (semi[-1])))
    {
        char encoding[16];
        const char *slash;
        GIConv coder = INVALID_CONV;
        int ms;

        /* first must be translated part before #enc: */
        ms = semi - path;

        state = _vfs_translate_path (path, ms, defcnv, buffer);

        if (state != ESTR_SUCCESS)
            return state;

        /* now can be translated part after #enc: */
        semi += strlen (VFS_ENCODING_PREFIX);   /* skip "#enc:" */
        slash = strchr (semi, PATH_SEP);
        /* ignore slashes after size; */
        if (slash - path >= size)
            slash = NULL;

        ms = (slash != NULL) ? slash - semi : (int) strlen (semi);
        ms = MIN ((unsigned int) ms, sizeof (encoding) - 1);
        /* limit encoding size (ms) to path size (size) */
        if (semi + ms > path + size)
            ms = path + size - semi;
        memcpy (encoding, semi, ms);
        encoding[ms] = '\0';

        if (is_supported_encoding (encoding))
            coder = str_crt_conv_to (encoding);

        if (coder != INVALID_CONV)
        {
            if (slash != NULL)
                state = str_vfs_convert_to (coder, slash + 1, path + size - slash - 1, buffer);
            str_close_conv (coder);
            return state;
        }

        errno = EINVAL;
        state = ESTR_FAILURE;
    }
    else
    {
        /* path can be translated whole at once */
        state = str_vfs_convert_to (defcnv, path, size, buffer);
    }
#else
    (void) size;
    (void) defcnv;

    g_string_assign (buffer, path);
#endif /* HAVE_CHARSET */

    return state;
}

/* --------------------------------------------------------------------------------------------- */

static struct vfs_openfile *
vfs_get_openfile (int handle)
{
    struct vfs_openfile *h;

    if (handle < VFS_FIRST_HANDLE || (guint) (handle - VFS_FIRST_HANDLE) >= vfs_openfiles->len)
        return NULL;

    h = (struct vfs_openfile *) g_ptr_array_index (vfs_openfiles, handle - VFS_FIRST_HANDLE);
    if (h == NULL)
        return NULL;

    g_assert (h->handle == handle);

    return h;
}

/* --------------------------------------------------------------------------------------------- */

static gboolean
vfs_test_current_dir (const vfs_path_t * vpath)
{
    struct stat my_stat, my_stat2;

    return (mc_global.vfs.cd_symlinks && mc_stat (vpath, &my_stat) == 0
            && mc_stat (vfs_get_raw_current_dir (), &my_stat2) == 0
            && my_stat.st_ino == my_stat2.st_ino && my_stat.st_dev == my_stat2.st_dev);
}


/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */
/** Free open file data for given file handle */

void
vfs_free_handle (int handle)
{
    const int idx = handle - VFS_FIRST_HANDLE;

    if (handle >= VFS_FIRST_HANDLE && (guint) idx < vfs_openfiles->len)
    {
        struct vfs_openfile *h;

        h = (struct vfs_openfile *) g_ptr_array_index (vfs_openfiles, idx);
        g_free (h);
        g_ptr_array_index (vfs_openfiles, idx) = (void *) vfs_free_handle_list;
        vfs_free_handle_list = idx;
    }
}


/* --------------------------------------------------------------------------------------------- */
/** Find VFS class by file handle */

struct vfs_class *
vfs_class_find_by_handle (int handle, void **fsinfo)
{
    struct vfs_openfile *h;

    h = vfs_get_openfile (handle);

    if (h == NULL)
        return NULL;

    if (fsinfo != NULL)
        *fsinfo = h->fsinfo;

    return h->vclass;
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Create new VFS handle and put it to the list
 */

int
vfs_new_handle (struct vfs_class *vclass, void *fsinfo)
{
    struct vfs_openfile *h;

    h = g_new (struct vfs_openfile, 1);
    h->fsinfo = fsinfo;
    h->vclass = vclass;

    /* Allocate the first free handle */
    h->handle = vfs_free_handle_list;
    if (h->handle == -1)
    {
        /* No free allocated handles, allocate one */
        h->handle = vfs_openfiles->len;
        g_ptr_array_add (vfs_openfiles, h);
    }
    else
    {
        vfs_free_handle_list = (long) g_ptr_array_index (vfs_openfiles, vfs_free_handle_list);
        g_ptr_array_index (vfs_openfiles, h->handle) = h;
    }

    h->handle += VFS_FIRST_HANDLE;
    return h->handle;
}

/* --------------------------------------------------------------------------------------------- */

int
vfs_ferrno (struct vfs_class *vfs)
{
    return vfs->ferrno ? (*vfs->ferrno) (vfs) : E_UNKNOWN;
    /* Hope that error message is obscure enough ;-) */
}

/* --------------------------------------------------------------------------------------------- */

gboolean
vfs_register_class (struct vfs_class * vfs)
{
    if (vfs->init != NULL)      /* vfs has own initialization function */
        if (!vfs->init (vfs))   /* but it failed */
            return FALSE;

    g_ptr_array_add (vfs__classes_list, vfs);

    return TRUE;
}

/* --------------------------------------------------------------------------------------------- */

void
vfs_unregister_class (struct vfs_class *vfs)
{
    if (vfs->done != NULL)
        vfs->done (vfs);

    g_ptr_array_remove (vfs__classes_list, vfs);
}

/* --------------------------------------------------------------------------------------------- */
/** Strip known vfs suffixes from a filename (possible improvement: strip
 *  suffix from last path component).
 *  \return a malloced string which has to be freed.
 */

char *
vfs_strip_suffix_from_filename (const char *filename)
{
    char *semi, *p;

    if (filename == NULL)
        vfs_die ("vfs_strip_suffix_from_path got NULL: impossible");

    p = g_strdup (filename);
    semi = g_strrstr (p, VFS_PATH_URL_DELIMITER);
    if (semi != NULL)
    {
        char *vfs_prefix;

        *semi = '\0';
        vfs_prefix = strrchr (p, PATH_SEP);
        if (vfs_prefix == NULL)
            *semi = *VFS_PATH_URL_DELIMITER;
        else
            *vfs_prefix = '\0';
    }

    return p;
}

/* --------------------------------------------------------------------------------------------- */

const char *
vfs_translate_path (const char *path)
{
    estr_t state;

    g_string_set_size (vfs_str_buffer, 0);
    state = _vfs_translate_path (path, -1, str_cnv_from_term, vfs_str_buffer);
    return (state != ESTR_FAILURE) ? vfs_str_buffer->str : NULL;
}

/* --------------------------------------------------------------------------------------------- */

char *
vfs_translate_path_n (const char *path)
{
    const char *result;

    result = vfs_translate_path (path);
    return g_strdup (result);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Get current directory without any OS calls.
 *
 * @return string contains current path
 */

const char *
vfs_get_current_dir (void)
{
    return current_path->str;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Get current directory without any OS calls.
 *
 * @return newly allocated string contains current path
 */

char *
vfs_get_current_dir_n (void)
{
    return g_strdup (current_path->str);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Get raw current directory object without any OS calls.
 *
 * @return object contain current path
 */

const vfs_path_t *
vfs_get_raw_current_dir (void)
{
    return current_path;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Set current directory object.
 *
 * @param vpath new path
 */
void
vfs_set_raw_current_dir (const vfs_path_t * vpath)
{
    vfs_path_free (current_path, TRUE);
    current_path = (vfs_path_t *) vpath;
}

/* --------------------------------------------------------------------------------------------- */
/* Return TRUE is the current VFS class is local */

gboolean
vfs_current_is_local (void)
{
    return (current_vfs->flags & VFSF_LOCAL) != 0;
}

/* --------------------------------------------------------------------------------------------- */
/* Return flags of the VFS class of the given filename */

vfs_flags_t
vfs_file_class_flags (const vfs_path_t * vpath)
{
    const vfs_path_element_t *path_element;

    path_element = vfs_path_get_by_index (vpath, -1);
    if (!vfs_path_element_valid (path_element))
        return VFSF_UNKNOWN;

    return path_element->class->flags;
}

/* --------------------------------------------------------------------------------------------- */

void
vfs_init (void)
{
    /* create the VFS handle arrays */
    vfs__classes_list = g_ptr_array_new ();

    /* create the VFS handle array */
    vfs_openfiles = g_ptr_array_new ();

    vfs_str_buffer = g_string_new ("");

    mc_readdir_result = vfs_dirent_init (NULL, "", -1);
}

/* --------------------------------------------------------------------------------------------- */

void
vfs_setup_work_dir (void)
{
    const vfs_path_element_t *path_element;

    vfs_setup_cwd ();

    /* FIXME: is we really need for this check? */
    /*
       if (strlen (current_dir) > MC_MAXPATHLEN - 2)
       vfs_die ("Current dir too long.\n");
     */

    path_element = vfs_path_get_by_index (current_path, -1);
    current_vfs = path_element->class;
}

/* --------------------------------------------------------------------------------------------- */

void
vfs_shut (void)
{
    guint i;

    vfs_gc_done ();

    vfs_set_raw_current_dir (NULL);

    for (i = 0; i < vfs__classes_list->len; i++)
    {
        struct vfs_class *vfs = VFS_CLASS (g_ptr_array_index (vfs__classes_list, i));

        if (vfs->done != NULL)
            vfs->done (vfs);
    }

    /* NULL-ize pointers to make unit tests happy */
    g_ptr_array_free (vfs_openfiles, TRUE);
    vfs_openfiles = NULL;
    g_ptr_array_free (vfs__classes_list, TRUE);
    vfs__classes_list = NULL;
    g_string_free (vfs_str_buffer, TRUE);
    vfs_str_buffer = NULL;
    current_vfs = NULL;
    vfs_free_handle_list = -1;
    vfs_dirent_free (mc_readdir_result);
    mc_readdir_result = NULL;
}

/* --------------------------------------------------------------------------------------------- */
/**
  * Init or create vfs_dirent structure
  *
  * @d vfs_dirent structure to init. If NULL, new structure is created.
  * @fname file name
  * @ino file inode number
  *
  * @return pointer to d if d isn't NULL, or pointer to newly created structure.
  */

struct vfs_dirent *
vfs_dirent_init (struct vfs_dirent *d, const char *fname, ino_t ino)
{
    struct vfs_dirent *ret = d;

    if (ret == NULL)
        ret = g_new0 (struct vfs_dirent, 1);

    if (ret->d_name_str == NULL)
        ret->d_name_str = g_string_sized_new (MC_MAXFILENAMELEN);

    vfs_dirent_assign (ret, fname, ino);

    return ret;
}

/* --------------------------------------------------------------------------------------------- */
/**
  * Assign members of vfs_dirent structure
  *
  * @d vfs_dirent structure for assignment
  * @fname file name
  * @ino file inode number
  */

void
vfs_dirent_assign (struct vfs_dirent *d, const char *fname, ino_t ino)
{
    g_string_assign (d->d_name_str, fname);
    d->d_name = d->d_name_str->str;
    d->d_ino = ino;
}

/* --------------------------------------------------------------------------------------------- */
/**
  * Destroy vfs_dirent structure
  *
  * @d vfs_dirent structure to destroy.
  */

void
vfs_dirent_free (struct vfs_dirent *d)
{
    g_string_free (d->d_name_str, TRUE);
    g_free (d);
}

/* --------------------------------------------------------------------------------------------- */

/**
 * These ones grab information from the VFS
 *  and handles them to an upper layer
 */

void
vfs_fill_names (fill_names_f func)
{
    guint i;

    for (i = 0; i < vfs__classes_list->len; i++)
    {
        struct vfs_class *vfs = VFS_CLASS (g_ptr_array_index (vfs__classes_list, i));

        if (vfs->fill_names != NULL)
            vfs->fill_names (vfs, func);
    }
}

/* --------------------------------------------------------------------------------------------- */

gboolean
vfs_file_is_local (const vfs_path_t * vpath)
{
    return (vfs_file_class_flags (vpath) & VFSF_LOCAL) != 0;
}

/* --------------------------------------------------------------------------------------------- */

void
vfs_print_message (const char *msg, ...)
{
    ev_vfs_print_message_t event_data;
    va_list ap;

    va_start (ap, msg);
    event_data.msg = g_strdup_vprintf (msg, ap);
    va_end (ap);

    mc_event_raise (MCEVENT_GROUP_CORE, "vfs_print_message", (gpointer) & event_data);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * If it's local, reread the current directory
 * from the OS.
 */

void
vfs_setup_cwd (void)
{
    char *current_dir;
    vfs_path_t *tmp_vpath;
    const vfs_path_element_t *path_element;

    if (vfs_get_raw_current_dir () == NULL)
    {
        current_dir = g_get_current_dir ();
        vfs_set_raw_current_dir (vfs_path_from_str (current_dir));
        g_free (current_dir);

        current_dir = getenv ("PWD");
        tmp_vpath = vfs_path_from_str (current_dir);

        if (tmp_vpath != NULL)
        {
            if (vfs_test_current_dir (tmp_vpath))
                vfs_set_raw_current_dir (tmp_vpath);
            else
                vfs_path_free (tmp_vpath, TRUE);
        }
    }

    path_element = vfs_path_get_by_index (vfs_get_raw_current_dir (), -1);

    if ((path_element->class->flags & VFSF_LOCAL) != 0)
    {
        current_dir = g_get_current_dir ();
        tmp_vpath = vfs_path_from_str (current_dir);
        g_free (current_dir);

        if (tmp_vpath != NULL)
        {
            /* One of directories in the path is not readable */

            /* Check if it is O.K. to use the current_dir */
            if (!vfs_test_current_dir (tmp_vpath))
                vfs_set_raw_current_dir (tmp_vpath);
            else
                vfs_path_free (tmp_vpath, TRUE);
        }
    }
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Return current directory.  If it's local, reread the current directory
 * from the OS.
 */

char *
_vfs_get_cwd (void)
{
    const vfs_path_t *current_dir_vpath;

    vfs_setup_cwd ();
    current_dir_vpath = vfs_get_raw_current_dir ();
    return g_strdup (vfs_path_as_str (current_dir_vpath));
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Preallocate space for file in new place for ensure that file
 * will be fully copied with less fragmentation.
 *
 * @param dest_vfs_fd mc VFS file handler
 * @param src_fsize source file size
 * @param dest_fsize destination file size (if destination exists, otherwise should be 0)
 *
 * @return 0 if success and non-zero otherwise.
 * Note: function doesn't touch errno global variable.
 */

int
vfs_preallocate (int dest_vfs_fd, off_t src_fsize, off_t dest_fsize)
{
#ifndef HAVE_POSIX_FALLOCATE
    (void) dest_vfs_fd;
    (void) src_fsize;
    (void) dest_fsize;
    return 0;

#else /* HAVE_POSIX_FALLOCATE */
    void *dest_fd = NULL;
    struct vfs_class *dest_class;

    if (src_fsize == 0)
        return 0;

    dest_class = vfs_class_find_by_handle (dest_vfs_fd, &dest_fd);
    if ((dest_class->flags & VFSF_LOCAL) == 0 || dest_fd == NULL)
        return 0;

    return posix_fallocate (*(int *) dest_fd, dest_fsize, src_fsize - dest_fsize);

#endif /* HAVE_POSIX_FALLOCATE */
}

 /* --------------------------------------------------------------------------------------------- */

int
vfs_clone_file (int dest_vfs_fd, int src_vfs_fd)
{
#ifdef FICLONE
    void *dest_fd = NULL;
    void *src_fd = NULL;
    struct vfs_class *dest_class;
    struct vfs_class *src_class;

    dest_class = vfs_class_find_by_handle (dest_vfs_fd, &dest_fd);
    if ((dest_class->flags & VFSF_LOCAL) == 0)
    {
        errno = EOPNOTSUPP;
        return (-1);
    }
    if (dest_fd == NULL)
    {
        errno = EBADF;
        return (-1);
    }

    src_class = vfs_class_find_by_handle (src_vfs_fd, &src_fd);
    if ((src_class->flags & VFSF_LOCAL) == 0)
    {
        errno = EOPNOTSUPP;
        return (-1);
    }
    if (src_fd == NULL)
    {
        errno = EBADF;
        return (-1);
    }

    return ioctl (*(int *) dest_fd, FICLONE, *(int *) src_fd);
#else
    (void) dest_vfs_fd;
    (void) src_vfs_fd;
    errno = EOPNOTSUPP;
    return (-1);
#endif
}

/* --------------------------------------------------------------------------------------------- */
