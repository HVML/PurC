
/**
 * \file
 * \brief Header: Virtual File System switch code
 */

#ifndef MC__VFS_VFS_H
#define MC__VFS_VFS_H

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>             /* DIR */
#ifdef HAVE_UTIMENSAT
#include <sys/time.h>
#elif defined (HAVE_UTIME_H)
#include <utime.h>
#endif
#include <stdio.h>
#include <unistd.h>
#include <stddef.h>

#include "lib/global.h"

#include "path.h"

/*** typedefs(not structures) and defined constants **********************************************/

#define VFS_CLASS(a) ((struct vfs_class *) (a))

#if defined (ENABLE_VFS_FTP) || defined (ENABLE_VFS_FISH)
#define ENABLE_VFS_NET 1
#endif

/**
 * This is the type of callback function passed to vfs_fill_names.
 * It gets the name of the virtual file system as its first argument.
 * See also:
 *    vfs_fill_names().
 */

#define VFS_ENCODING_PREFIX "#enc:"

#define O_ALL (O_CREAT | O_EXCL | O_NOCTTY | O_NDELAY | O_SYNC | O_WRONLY | O_RDWR | O_RDONLY)
/* Midnight commander code should _not_ use other flags than those
   listed above and O_APPEND */

#if (O_ALL & O_APPEND)
#warning "Unexpected problem with flags, O_LINEAR disabled, contact pavel@ucw.cz"
#define O_LINEAR 0
#define IS_LINEAR(a) 0
#define NO_LINEAR(a) a
#else
#define O_LINEAR O_APPEND
#define IS_LINEAR(a) ((a) == (O_RDONLY | O_LINEAR))     /* Return only 0 and 1 ! */
#define NO_LINEAR(a) (((a) == (O_RDONLY | O_LINEAR)) ? O_RDONLY : (a))
#endif

/* O_LINEAR is strange beast, be careful. If you open file asserting
 * O_RDONLY | O_LINEAR, you promise:
 *
 *      a) to read file linearly from beginning to the end
 *      b) not to open another file before you close this one
 *              (this will likely go away in future)
 *      as a special gift, you may
 *      c) lseek() immediately after open(), giving ftpfs chance to
 *         reget. Be warned that this lseek() can fail, and you _have_
 *         to handle that gratefully.
 *
 * O_LINEAR allows filesystems not to create temporary file in some
 * cases (ftp transfer).                                -- pavel@ucw.cz
 */

/* And now some defines for our errors. */

#ifdef ENOSYS
#define E_NOTSUPP ENOSYS        /* for use in vfs when module does not provide function */
#else
#define E_NOTSUPP EFAULT        /* Does this happen? */
#endif

#ifdef ENOMSG
#define E_UNKNOWN ENOMSG        /* if we do not know what error happened */
#else
#define E_UNKNOWN EIO           /* if we do not know what error happened */
#endif

#ifdef EREMOTEIO
#define E_REMOTE EREMOTEIO      /* if other side of ftp/fish reports error */
#else
#define E_REMOTE ENETUNREACH    /* :-( there's no EREMOTEIO on some systems */
#endif

#ifdef EPROTO
#define E_PROTO EPROTO          /* if other side fails to follow protocol */
#else
#define E_PROTO EIO
#endif

typedef void (*fill_names_f) (const char *);

typedef void *vfsid;

#ifdef HAVE_UTIMENSAT
typedef struct timespec mc_timesbuf_t[2];
#else
typedef struct utimbuf mc_timesbuf_t;
#endif

/*** enums ***************************************************************************************/

typedef enum
{
    VFSF_UNKNOWN = 0,
    VFSF_LOCAL = 1 << 0,        /* Class is local (not virtual) filesystem */
    VFSF_NOLINKS = 1 << 1,      /* Hard links not supported */

    VFSF_REMOTE = 1 << 2,
    VFSF_READONLY = 1 << 3,
    VFSF_USETMP = 1 << 4
} vfs_flags_t;

/* Operations for mc_ctl - on open file */
enum
{
    VFS_CTL_IS_NOTREADY
};

/* Operations for mc_setctl - on path */
enum
{
    VFS_SETCTL_FORGET,
    VFS_SETCTL_RUN,
    VFS_SETCTL_LOGFILE,
    VFS_SETCTL_FLUSH,           /* invalidate directory cache */

    /* Setting this makes vfs layer give out potentially incorrect data,
       but it also makes some operations much faster. Use with caution. */
    VFS_SETCTL_STALE_DATA
};

/*** structures declarations (and typedefs of structures)*****************************************/

typedef struct vfs_class
{
    const char *name;           /* "FIles over SHell" */
    vfs_flags_t flags;
    const char *prefix;         /* "fish:" */
    int verrno;                 /* can't use errno because glibc2 might define errno as function */
    gboolean flush;             /* if set to TRUE, invalidate directory cache */
    FILE *logfile;

    /* *INDENT-OFF* */
    int (*init) (struct vfs_class * me);
    void (*done) (struct vfs_class * me);

    /**
     * The fill_names method shall call the callback function for every
     * filesystem name that this vfs module supports.
     */
    void (*fill_names) (struct vfs_class * me, fill_names_f);

    /**
     * The which() method shall return the index of the vfs subsystem
     * or -1 if this vfs cannot handle the given pathname.
     */
    int (*which) (struct vfs_class * me, const char *path);

    void *(*open) (const vfs_path_t * vpath, int flags, mode_t mode);
    int (*close) (void *vfs_info);
    ssize_t (*read) (void *vfs_info, char *buffer, size_t count);
    ssize_t (*write) (void *vfs_info, const char *buf, size_t count);

    void *(*opendir) (const vfs_path_t * vpath);
    struct vfs_dirent *(*readdir) (void *vfs_info);
    int (*closedir) (void *vfs_info);

    int (*stat) (const vfs_path_t * vpath, struct stat * buf);
    int (*lstat) (const vfs_path_t * vpath, struct stat * buf);
    int (*fstat) (void *vfs_info, struct stat * buf);

    int (*chmod) (const vfs_path_t * vpath, mode_t mode);
    int (*chown) (const vfs_path_t * vpath, uid_t owner, gid_t group);
    int (*utime) (const vfs_path_t * vpath, mc_timesbuf_t * times);

    int (*readlink) (const vfs_path_t * vpath, char *buf, size_t size);
    int (*symlink) (const vfs_path_t * vpath1, const vfs_path_t * vpath2);
    int (*link) (const vfs_path_t * vpath1, const vfs_path_t * vpath2);
    int (*unlink) (const vfs_path_t * vpath);
    int (*rename) (const vfs_path_t * vpath1, const vfs_path_t * vpath2);
    int (*chdir) (const vfs_path_t * vpath);
    int (*ferrno) (struct vfs_class * me);
    off_t (*lseek) (void *vfs_info, off_t offset, int whence);
    int (*mknod) (const vfs_path_t * vpath, mode_t mode, dev_t dev);

    vfsid (*getid) (const vfs_path_t * vpath);

    gboolean (*nothingisopen) (vfsid id);
    void (*free) (vfsid id);

    vfs_path_t *(*getlocalcopy) (const vfs_path_t * vpath);
    int (*ungetlocalcopy) (const vfs_path_t * vpath, const vfs_path_t * local_vpath,
                           gboolean has_changed);

    int (*mkdir) (const vfs_path_t * vpath, mode_t mode);
    int (*rmdir) (const vfs_path_t * vpath);

    int (*ctl) (void *vfs_info, int ctlop, void *arg);
    int (*setctl) (const vfs_path_t * vpath, int ctlop, void *arg);
    /* *INDENT-ON* */
} vfs_class;

/*
 * This struct is used instead of standard dirent to hold file name of any length
 * (not limited to NAME_MAX).
 */
struct vfs_dirent
{
    /* private */
    GString *d_name_str;

    /* public */
    ino_t d_ino;
    char *d_name;               /* Alias of d_name_str->str */
};

/*** global variables defined in .c file *********************************************************/

extern int vfs_timeout;

#ifdef ENABLE_VFS_NET
extern int use_netrc;
#endif

/*** declarations of public functions ************************************************************/

/* lib/vfs/direntry.c: */
void vfs_init_class (struct vfs_class *vclass, const char *name, vfs_flags_t flags,
                     const char *prefix);

void *vfs_s_open (const vfs_path_t * vpath, int flags, mode_t mode);
int vfs_s_stat (const vfs_path_t * vpath, struct stat *buf);
int vfs_s_lstat (const vfs_path_t * vpath, struct stat *buf);
int vfs_s_fstat (void *fh, struct stat *buf);

void vfs_adjust_stat (struct stat *s);

vfsid vfs_getid (const vfs_path_t * vpath);

void vfs_init (void);
void vfs_shut (void);
/* Register a file system class */
gboolean vfs_register_class (struct vfs_class *vfs);
void vfs_unregister_class (struct vfs_class *vfs);

void vfs_setup_work_dir (void);

void vfs_timeout_handler (void);
int vfs_timeouts (void);
void vfs_expire (gboolean now);

const char *vfs_get_current_dir (void);
char *vfs_get_current_dir_n (void);
const vfs_path_t *vfs_get_raw_current_dir (void);
void vfs_set_raw_current_dir (const vfs_path_t * vpath);

gboolean vfs_current_is_local (void);
gboolean vfs_file_is_local (const vfs_path_t * vpath);

char *vfs_strip_suffix_from_filename (const char *filename);

vfs_flags_t vfs_file_class_flags (const vfs_path_t * vpath);

/* translate path back to terminal encoding, remove all #enc:
 * every invalid character is replaced with question mark
 * return static buffer */
const char *vfs_translate_path (const char *path);
/* return new string */
char *vfs_translate_path_n (const char *path);

void vfs_stamp_path (const vfs_path_t * path);

void vfs_release_path (const vfs_path_t * vpath);

struct vfs_dirent *vfs_dirent_init (struct vfs_dirent *d, const char *fname, ino_t ino);
void vfs_dirent_assign (struct vfs_dirent *d, const char *fname, ino_t ino);
void vfs_dirent_free (struct vfs_dirent *d);

void vfs_fill_names (fill_names_f);

/* *INDENT-OFF* */
void vfs_print_message (const char *msg, ...) G_GNUC_PRINTF (1, 2);
/* *INDENT-ON* */

int vfs_ferrno (struct vfs_class *vfs);

int vfs_new_handle (struct vfs_class *vclass, void *fsinfo);

struct vfs_class *vfs_class_find_by_handle (int handle, void **fsinfo);

void vfs_free_handle (int handle);

void vfs_setup_cwd (void);
char *_vfs_get_cwd (void);

int vfs_preallocate (int dest_desc, off_t src_fsize, off_t dest_fsize);

int vfs_clone_file (int dest_vfs_fd, int src_vfs_fd);

/**
 * Interface functions described in interface.c
 */
ssize_t mc_read (int handle, void *buffer, size_t count);
ssize_t mc_write (int handle, const void *buffer, size_t count);
int mc_utime (const vfs_path_t * vpath, mc_timesbuf_t * times);
int mc_readlink (const vfs_path_t * vpath, char *buf, size_t bufsiz);
int mc_close (int handle);
off_t mc_lseek (int fd, off_t offset, int whence);
DIR *mc_opendir (const vfs_path_t * vpath);
struct vfs_dirent *mc_readdir (DIR * dirp);
int mc_closedir (DIR * dir);
int mc_stat (const vfs_path_t * vpath, struct stat *buf);
int mc_mknod (const vfs_path_t * vpath, mode_t mode, dev_t dev);
int mc_link (const vfs_path_t * vpath1, const vfs_path_t * vpath2);
int mc_mkdir (const vfs_path_t * vpath, mode_t mode);
int mc_rmdir (const vfs_path_t * vpath);
int mc_fstat (int fd, struct stat *buf);
int mc_lstat (const vfs_path_t * vpath, struct stat *buf);
int mc_symlink (const vfs_path_t * vpath1, const vfs_path_t * vpath2);
int mc_rename (const vfs_path_t * vpath1, const vfs_path_t * vpath2);
int mc_chmod (const vfs_path_t * vpath, mode_t mode);
int mc_chown (const vfs_path_t * vpath, uid_t owner, gid_t group);
int mc_chdir (const vfs_path_t * vpath);
int mc_unlink (const vfs_path_t * vpath);
int mc_ctl (int fd, int ctlop, void *arg);
int mc_setctl (const vfs_path_t * vpath, int ctlop, void *arg);
int mc_open (const vfs_path_t * vpath, int flags, ...);
vfs_path_t *mc_getlocalcopy (const vfs_path_t * pathname_vpath);
int mc_ungetlocalcopy (const vfs_path_t * pathname_vpath, const vfs_path_t * local_vpath,
                       gboolean has_changed);
int mc_mkstemps (vfs_path_t ** pname_vpath, const char *prefix, const char *suffix);

/* Creating temporary files safely */
const char *mc_tmpdir (void);


/*** inline functions ****************************************************************************/

#endif /* MC_VFS_VFS_H */
