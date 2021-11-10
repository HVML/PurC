/** \file fs.h
 *  \brief Header: fs compatibility definitions
 */

/* Include file to use opendir/closedir/readdir */

#ifndef MC_FS_H
#define MC_FS_H

#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

/*** typedefs(not structures) and defined constants **********************************************/

#ifdef S_ISREG
#define HAVE_S_ISREG 1
#else
#define HAVE_S_ISREG 0
#define S_ISREG(x) 0
#endif

#ifdef S_ISDIR
#define HAVE_S_ISDIR 1
#else
#define HAVE_S_ISDIR 0
#define S_ISDIR(x) 0
#endif

/* Replacement for permission bits missing in sys/stat.h */
#ifdef S_ISLNK
#define HAVE_S_ISLNK 1
#else
#define HAVE_S_ISLNK 0
#define S_ISLNK(x) 0
#endif

#ifdef S_ISSOCK
#define HAVE_S_ISSOCK 1
#else
#define HAVE_S_ISSOCK 0
#define S_ISSOCK(x) 0
#endif

#ifdef S_ISFIFO
#define HAVE_S_ISFIFO 1
#else
#define HAVE_S_ISFIFO 0
#define S_ISFIFO(x) 0
#endif

#ifdef S_ISCHR
#define HAVE_S_ISCHR 1
#else
#define HAVE_S_ISCHR 0
#define S_ISCHR(x) 0
#endif

#ifdef S_ISBLK
#define HAVE_S_ISBLK 1
#else
#define HAVE_S_ISBLK 0
#define S_ISBLK(x) 0
#endif

/* Door is something that only exists on Solaris */
#ifdef S_ISDOOR
#define HAVE_S_ISDOOR 1
#else
#define HAVE_S_ISDOOR 0
#define S_ISDOOR(x) 0
#endif

/* Special named files are widely used in QNX6 */
#ifdef S_ISNAM
#define HAVE_S_ISNAM 1
#else
#define HAVE_S_ISNAM 0
#define S_ISNAM(x) 0
#endif

#ifndef PATH_MAX
#ifdef _POSIX_VERSION
#define PATH_MAX _POSIX_PATH_MAX
#else
#ifdef MAXPATHLEN
#define PATH_MAX MAXPATHLEN
#else
#define PATH_MAX 1024
#endif
#endif
#endif

#ifndef MAXPATHLEN
#define MC_MAXPATHLEN 4096
#else
#define MC_MAXPATHLEN MAXPATHLEN
#endif

/* unistd.h defines _POSIX_VERSION on POSIX.1 systems. */
#define NLENGTH(dirent) (strlen ((dirent)->d_name))

/* DragonFlyBSD doesn't provide MAXNAMLEN macro */
#ifndef MAXNAMLEN
#define MAXNAMLEN NAME_MAX
#endif

#define MC_MAXFILENAMELEN MAXNAMLEN

#define DIR_IS_DOT(x) ((x)[0] == '.' && (x)[1] == '\0')
#define DIR_IS_DOTDOT(x) ((x)[0] == '.' && (x)[1] == '.' && (x)[2] == '\0')

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

/*** inline functions ****************************************************************************/

#endif
