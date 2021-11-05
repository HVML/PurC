
/**
 * \file
 * \brief Header: Utilities for VFS modules
 * \author Miguel de Icaza
 * \date 1995, 1996
 */

#ifndef MC_VFS_UTILVFS_H
#define MC_VFS_UTILVFS_H

#include <sys/stat.h>

#include "lib/global.h"
#include "path.h"

/*** typedefs(not structures) and defined constants **********************************************/

/*** enums ***************************************************************************************/

/** Bit flags for vfs_url_split()
 *
 *  Modify parsing parameters according to flag meaning.
 *  @see vfs_url_split()
 */
typedef enum
{
    URL_FLAGS_NONE = 0,
    URL_USE_ANONYMOUS = 1, /**< if set, empty *user will contain NULL instead of current */
    URL_NOSLASH = 2        /**< if set, 'proto://' part in url is not searched */
} vfs_url_flags_t;

/*** structures declarations (and typedefs of structures)*****************************************/

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

int vfs_finduid (const char *name);
int vfs_findgid (const char *name);

vfs_path_element_t *vfs_url_split (const char *path, int default_port, vfs_url_flags_t flags);
int vfs_split_text (char *p);

int vfs_mkstemps (vfs_path_t ** pname_vpath, const char *prefix, const char *basename);
void vfs_die (const char *msg);
char *vfs_get_password (const char *msg);

char *vfs_get_local_username (void);

gboolean vfs_parse_filetype (const char *s, size_t * ret_skipped, mode_t * ret_type);
gboolean vfs_parse_fileperms (const char *s, size_t * ret_skipped, mode_t * ret_perms);
gboolean vfs_parse_filemode (const char *s, size_t * ret_skipped, mode_t * ret_mode);
gboolean vfs_parse_raw_filemode (const char *s, size_t * ret_skipped, mode_t * ret_mode);

void vfs_parse_ls_lga_init (void);
gboolean vfs_parse_ls_lga (const char *p, struct stat *s, char **filename, char **linkname,
                           size_t * filename_pos);
size_t vfs_parse_ls_lga_get_final_spaces (void);
gboolean vfs_parse_month (const char *str, struct tm *tim);
int vfs_parse_filedate (int idx, time_t * t);

/*** inline functions ****************************************************************************/
#endif
