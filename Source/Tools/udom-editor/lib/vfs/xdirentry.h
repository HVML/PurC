
/**
 * \file
 * \brief Header: Virtual File System directory structure
 */


#ifndef MC__VFS_XDIRENTRY_H
#define MC__VFS_XDIRENTRY_H

#include <stdio.h>
#include <sys/types.h>

#include "lib/global.h"         /* GList */
#include "lib/vfs/path.h"       /* vfs_path_t */

/*** typedefs(not structures) and defined constants **********************************************/

#define LINK_FOLLOW 15
#define LINK_NO_FOLLOW -1

/* For vfs_s_find_entry and vfs_s_find_inode */
#define FL_NONE 0
#define FL_MKDIR 1
#define FL_MKFILE 2
#define FL_DIR 4

/* For open_super */
#define FL_NO_OPEN 1

/* For vfs_s_entry_from_path */
#define FL_FOLLOW 1
#define FL_DIR 4

#define ERRNOR(a, b) do { me->verrno = a; return b; } while (0)

#define VFS_SUBCLASS(a) ((struct vfs_s_subclass *) (a))

#define VFS_SUPER(a) ((struct vfs_s_super *) (a))
#define CONST_VFS_SUPER(a) ((const struct vfs_s_super *) (a))
#define VFS_ENTRY(a) ((struct vfs_s_entry *) (a))
#define VFS_INODE(a) ((struct vfs_s_inode *) (a))

#define VFS_FILE_HANDLER(a) ((vfs_file_handler_t *) a)
#define VFS_FILE_HANDLER_SUPER(a) VFS_FILE_HANDLER (a)->ino->super

/*** enums ***************************************************************************************/

typedef enum
{
    LS_NOT_LINEAR = 0,
    LS_LINEAR_CLOSED = 1,
    LS_LINEAR_OPEN = 2,
    LS_LINEAR_PREOPEN = 3
} vfs_linear_state_t;

/*** structures declarations (and typedefs of structures)*****************************************/

/* Single connection or archive */
struct vfs_s_super
{
    struct vfs_class *me;
    struct vfs_s_inode *root;
    char *name;                 /* My name, whatever it means */
    int fd_usage;               /* Number of open files */
    int ino_usage;              /* Usage count of this superblock */
    gboolean want_stale;        /* If set, we do not flush cache properly */
#ifdef ENABLE_VFS_NET
    vfs_path_element_t *path_element;
#endif                          /* ENABLE_VFS_NET */
};

/*
 * Single virtual file - directory entry.  The same inode can have many
 * entries (i.e. hard links), but usually has only one.
 */
struct vfs_s_entry
{
    struct vfs_s_inode *dir;    /* Directory we are in, i.e. our parent */
    char *name;                 /* Name of this entry */
    struct vfs_s_inode *ino;    /* ... and its inode */
};

/* Single virtual file - inode */
struct vfs_s_inode
{
    struct vfs_s_super *super;  /* Archive the file is on */
    struct vfs_s_entry *ent;    /* Our entry in the parent directory -
                                   use only for directories because they
                                   cannot be hardlinked */
    GQueue *subdir;             /* If this is a directory, its entry. List of vfs_s_entry */
    struct stat st;             /* Parameters of this inode */
    char *linkname;             /* Symlink's contents */
    char *localname;            /* Filename of local file, if we have one */
    gint64 timestamp;           /* Subclass specific */
    off_t data_offset;          /* Subclass specific */
};

/* Data associated with an open file */
typedef struct
{
    struct vfs_s_inode *ino;
    off_t pos;                  /* This is for module's use */
    int handle;                 /* This is for module's use, but if != -1, will be mc_close()d */
    gboolean changed;           /* Did this file change? */
    vfs_linear_state_t linear;  /* Is that file open with O_LINEAR? */
} vfs_file_handler_t;

/*
 * One of our subclasses (tar, cpio, fish, ftpfs) with data and methods.
 * Extends vfs_class.
 */
struct vfs_s_subclass
{
    struct vfs_class base;      /* base class */

    GList *supers;
    int inode_counter;
    dev_t rdev;

    /* *INDENT-OFF* */
    int (*init_inode) (struct vfs_class * me, struct vfs_s_inode * ino);        /* optional */
    void (*free_inode) (struct vfs_class * me, struct vfs_s_inode * ino);       /* optional */
    int (*init_entry) (struct vfs_class * me, struct vfs_s_entry * entry);      /* optional */

    void *(*archive_check) (const vfs_path_t * vpath);  /* optional */
    int (*archive_same) (const vfs_path_element_t * vpath_element, struct vfs_s_super * psup,
                         const vfs_path_t * vpath, void *cookie);
    struct vfs_s_super *(*new_archive) (struct vfs_class * me);
    int (*open_archive) (struct vfs_s_super * psup,
                         const vfs_path_t * vpath, const vfs_path_element_t * vpath_element);
    void (*free_archive) (struct vfs_class * me, struct vfs_s_super * psup);

    vfs_file_handler_t *(*fh_new) (struct vfs_s_inode * ino, gboolean changed);
    int (*fh_open) (struct vfs_class * me, vfs_file_handler_t * fh, int flags, mode_t mode);
    int (*fh_close) (struct vfs_class * me, vfs_file_handler_t * fh);
    void (*fh_free) (vfs_file_handler_t * fh);

    struct vfs_s_entry *(*find_entry) (struct vfs_class * me,
                                       struct vfs_s_inode * root,
                                       const char *path, int follow, int flags);
    int (*dir_load) (struct vfs_class * me, struct vfs_s_inode * ino, char *path);
    gboolean (*dir_uptodate) (struct vfs_class * me, struct vfs_s_inode * ino);
    int (*file_store) (struct vfs_class * me, vfs_file_handler_t * fh, char *path, char *localname);

    int (*linear_start) (struct vfs_class * me, vfs_file_handler_t * fh, off_t from);
    ssize_t (*linear_read) (struct vfs_class * me, vfs_file_handler_t * fh, void *buf, size_t len);
    void (*linear_close) (struct vfs_class * me, vfs_file_handler_t * fh);
    /* *INDENT-ON* */
};

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

/* entries and inodes */
struct vfs_s_inode *vfs_s_new_inode (struct vfs_class *me,
                                     struct vfs_s_super *super, struct stat *initstat);
void vfs_s_free_inode (struct vfs_class *me, struct vfs_s_inode *ino);

struct vfs_s_entry *vfs_s_new_entry (struct vfs_class *me, const char *name,
                                     struct vfs_s_inode *inode);
void vfs_s_free_entry (struct vfs_class *me, struct vfs_s_entry *ent);
void vfs_s_insert_entry (struct vfs_class *me, struct vfs_s_inode *dir, struct vfs_s_entry *ent);
int vfs_s_entry_compare (const void *a, const void *b);
struct stat *vfs_s_default_stat (struct vfs_class *me, mode_t mode);

struct vfs_s_entry *vfs_s_generate_entry (struct vfs_class *me, const char *name,
                                          struct vfs_s_inode *parent, mode_t mode);
struct vfs_s_inode *vfs_s_find_inode (struct vfs_class *me,
                                      const struct vfs_s_super *super,
                                      const char *path, int follow, int flags);
struct vfs_s_inode *vfs_s_find_root (struct vfs_class *me, struct vfs_s_entry *entry);

/* outside interface */
void vfs_init_subclass (struct vfs_s_subclass *sub, const char *name, vfs_flags_t flags,
                        const char *prefix);
const char *vfs_s_get_path (const vfs_path_t * vpath, struct vfs_s_super **archive, int flags);
struct vfs_s_super *vfs_get_super_by_vpath (const vfs_path_t * vpath);

void vfs_s_invalidate (struct vfs_class *me, struct vfs_s_super *super);
char *vfs_s_fullpath (struct vfs_class *me, struct vfs_s_inode *ino);

void vfs_s_init_fh (vfs_file_handler_t * fh, struct vfs_s_inode *ino, gboolean changed);

/* network filesystems support */
int vfs_s_select_on_two (int fd1, int fd2);
int vfs_s_get_line (struct vfs_class *me, int sock, char *buf, int buf_len, char term);
int vfs_s_get_line_interruptible (struct vfs_class *me, char *buffer, int size, int fd);
/* misc */
int vfs_s_retrieve_file (struct vfs_class *me, struct vfs_s_inode *ino);

void vfs_s_normalize_filename_leading_spaces (struct vfs_s_inode *root_inode, size_t final_filepos);

/*** inline functions ****************************************************************************/

static inline void
vfs_s_store_filename_leading_spaces (struct vfs_s_entry *entry, size_t position)
{
    entry->ino->data_offset = (off_t) position;
}

#endif
