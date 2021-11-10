
/**
 * \file
 * \brief Header: Virtual File System: Network utilities
 */

#ifndef MC__VFS_NETUTIL_H
#define MC__VFS_NETUTIL_H

#include <signal.h>
/*** typedefs(not structures) and defined constants **********************************************/

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

/*** global variables defined in .c file *********************************************************/

extern SIG_ATOMIC_VOLATILE_T got_sigpipe;

/*** declarations of public functions ************************************************************/

void tcp_init (void);

/*** inline functions ****************************************************************************/
#endif /* MC_VFS_NETUTIL_H */
