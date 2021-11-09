#ifndef MC_SRC_UTIL_H
#define MC_SRC_UTIL_H

/*** typedefs(not structures) and defined constants **********************************************/

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

/* Check if the file exists. If not copy the default */
gboolean check_for_default (const vfs_path_t * default_file_vpath, const vfs_path_t * file_vpath);

/*** inline functions ****************************************************************************/

#endif /* MC_SRC_UTIL_H */
