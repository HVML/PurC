/**
 * \file
 * \brief Header: Virtual File System: garbage collection code
 */

#ifndef MC__VFS_GC_H
#define MC__VFS_GC_H

#include "vfs.h"

/*** typedefs(not structures) and defined constants **********************************************/

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

gboolean vfs_stamp (struct vfs_class *vclass, vfsid id);
void vfs_rmstamp (struct vfs_class *vclass, vfsid id);
void vfs_stamp_create (struct vfs_class *vclass, vfsid id);
void vfs_gc_done (void);

/*** inline functions ****************************************************************************/
#endif /* MC_VFS_GC_H */
