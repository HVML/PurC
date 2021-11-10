/** \file unixcompat.h
 *  \brief Header: collects differences between the various Unix
 *
 *  This header file collects differences between the various Unix
 *  variants that are supported by the Midnight Commander and provides
 *  replacement routines if they are not natively available.
 *  The major/minor macros are not specified in SUSv3, so we can only hope
 *  they are provided by the operating system or emulate it.
 */

#ifndef MC_UNIXCOMPAT_H
#define MC_UNIXCOMPAT_H

#include <sys/types.h>          /* BSD */

#ifdef MAJOR_IN_MKDEV
#include <sys/mkdev.h>
#elif defined MAJOR_IN_SYSMACROS
#include <sys/sysmacros.h>
#endif

#include <unistd.h>

/*** typedefs(not structures) and defined constants **********************************************/

#ifndef major
#warning major() is undefined. Device numbers will not be shown correctly.
#define major(devnum) (((devnum) >> 8) & 0xff)
#endif

#ifndef minor
#warning minor() is undefined. Device numbers will not be shown correctly.
#define minor(devnum) (((devnum) & 0xff))
#endif

#ifndef makedev
#warning makedev() is undefined. Device numbers will not be shown correctly.
#define makedev(major,minor) ((((major) & 0xff) << 8) | ((minor) & 0xff))
#endif

#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#endif

#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif

#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

/*** inline functions ****************************************************************************/

#endif
