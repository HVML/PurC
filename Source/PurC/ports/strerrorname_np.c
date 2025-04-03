/*
 * @file strerrorname_np.c
 * @author Vincent Wei (https://github.com/VincentWei)
 * @date 2025/02/10
 * @brief The implementation of strerrorname_np().
 *
 * Copyright (C) 2025 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of PurC (short for Purring Cat), an HVML interpreter.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#if !HAVE(STRERRORNAME_NP)

#include "private/utils.h"

#include <errno.h>

/* compliant version of strerrorname_np */
const char *strerrorname_np(int errnum)
{
    const char *name = NULL;

    switch (errnum) {
    case 0:
        name = "0";
        break;
    case EPERM:
        name = "EPERM";    // Operation not permitted
        break;
    case ENOENT:
        name = "ENOENT";   // No such file or directory
        break;
    case ESRCH:
        name = "ESRCH";    // No such process
        break;
    case EINTR:
        name = "EINTR";    // Interrupted system call
        break;
    case EIO:
        name = "EIO";  // Input/output error
        break;
    case ENXIO:
        name = "ENXIO";    // No such device or address
        break;
    case E2BIG:
        name = "E2BIG";    // Argument list too long
        break;
    case ENOEXEC:
        name = "ENOEXEC";  // Exec format error
        break;
    case EBADF:
        name = "EBADF";    // Bad file descriptor
        break;
    case ECHILD:
        name = "ECHILD";   // No child processes
        break;
#if EAGAIN == EWOULDBLOCK
    case EAGAIN:
        name = "EAGAIN";   // Resource temporarily unavailable
        break;
#else
    case EAGAIN:
        name = "EAGAIN";   // Resource temporarily unavailable
        break;
    case EWOULDBLOCK:
        name = "EWOULDBLOCK";  // Resource temporarily unavailable
        break;
#endif
    case ENOMEM:
        name = "ENOMEM";   // Cannot allocate memory
        break;
    case EACCES:
        name = "EACCES";   // Permission denied
        break;
    case EFAULT:
        name = "EFAULT";   // Bad address
        break;
    case ENOTBLK:
        name = "ENOTBLK";  // Block device required
        break;
    case EBUSY:
        name = "EBUSY";    // Device or resource busy
        break;
    case EEXIST:
        name = "EEXIST";   // File exists
        break;
    case EXDEV:
        name = "EXDEV";    // Invalid cross-device link
        break;
    case ENODEV:
        name = "ENODEV";   // No such device
        break;
    case ENOTDIR:
        name = "ENOTDIR";  // Not a directory
        break;
    case EISDIR:
        name = "EISDIR";   // Is a directory
        break;
    case EINVAL:
        name = "EINVAL";   // Invalid argument
        break;
    case ENFILE:
        name = "ENFILE";   // Too many open files in system
        break;
    case EMFILE:
        name = "EMFILE";   // Too many open files
        break;
    case ENOTTY:
        name = "ENOTTY";   // Inappropriate ioctl for device
        break;
    case ETXTBSY:
        name = "ETXTBSY";  // Text file busy
        break;
    case EFBIG:
        name = "EFBIG";    // File too large
        break;
    case ENOSPC:
        name = "ENOSPC";   // No space left on device
        break;
    case ESPIPE:
        name = "ESPIPE";   // Illegal seek
        break;
    case EROFS:
        name = "EROFS";    // Read-only file system
        break;
    case EMLINK:
        name = "EMLINK";   // Too many links
        break;
    case EPIPE:
        name = "EPIPE";    // Broken pipe
        break;
    case EDOM:
        name = "EDOM"; // Numerical argument out of domain
        break;
    case ERANGE:
        name = "ERANGE";   // Numerical result out of range
        break;
#ifndef EDEADLOCK
    case EDEADLK:
        name = "EDEADLK";  // Resource deadlock avoided
        break;
#endif
    case ENAMETOOLONG:
        name = "ENAMETOOLONG"; // File name = too long
        break;
    case ENOLCK:
        name = "ENOLCK";   // No locks available
        break;
    case ENOSYS:
        name = "ENOSYS";   // Function not implemented
        break;
    case ENOTEMPTY:
        name = "ENOTEMPTY";    // Directory not empty
        break;
    case ELOOP:
        name = "ELOOP";    // Too many levels of symbolic links
        break;
    case ENOMSG:
        name = "ENOMSG";   // No message of desired type
        break;
    case EIDRM:
        name = "EIDRM";    // Identifier removed
        break;
#ifdef ECHRNG
    case ECHRNG:
        name = "ECHRNG";   // Channel number out of range
        break;
#endif
#ifdef EL2NSYNC
    case EL2NSYNC:
        name = "EL2NSYNC"; // Level 2 not synchronized
        break;
#endif
#ifdef EL3HLT
    case EL3HLT:
        name = "EL3HLT";   // Level 3 halted
        break;
#endif
#ifdef EL3RST
    case EL3RST:
        name = "EL3RST";   // Level 3 reset
        break;
#endif
#ifdef ELNRNG
    case ELNRNG:
        name = "ELNRNG";   // Link number out of range
        break;
#endif
#ifdef EUNATCH
    case EUNATCH:
        name = "EUNATCH";  // Protocol driver not attached
        break;
#endif
#ifdef ENOCSI
    case ENOCSI:
        name = "ENOCSI";   // No CSI structure available
        break;
#endif
#ifdef EL2HLT
    case EL2HLT:
        name = "EL2HLT";   // Level 2 halted
        break;
#endif
#ifdef EBADE
    case EBADE:
        name = "EBADE";    // Invalid exchange
        break;
#endif
#ifdef EBADR
    case EBADR:
        name = "EBADR";    // Invalid request descriptor
        break;
#endif
#ifdef EXFULL
    case EXFULL:
        name = "EXFULL";   // Exchange full
        break;
#endif
#ifdef ENOANO
    case ENOANO:
        name = "ENOANO";   // No anode
        break;
#endif
#ifdef EBADRQC
    case EBADRQC:
        name = "EBADRQC";  // Invalid request code
        break;
#endif
#ifdef EBADSLT
    case EBADSLT:
        name = "EBADSLT";  // Invalid slot
        break;
#endif
#ifdef EDEADLOCK
    case EDEADLOCK:
        name = "EDEADLOCK";    // Resource deadlock avoided
        break;
#endif
#ifdef EBFONT
    case EBFONT:
        name = "EBFONT";   // Bad font file format
        break;
#endif
    case ENOSTR:
        name = "ENOSTR";   // Device not a stream
        break;
    case ENODATA:
        name = "ENODATA";  // No data available
        break;
    case ETIME:
        name = "ETIME";    // Timer expired
        break;
    case ENOSR:
        name = "ENOSR";    // Out of streams resources
        break;
#ifdef ENONET
    case ENONET:
        name = "ENONET";   // Machine is not on the network
        break;
#endif
#ifdef ENOPKG
    case ENOPKG:
        name = "ENOPKG";   // Package not installed
        break;
#endif
#ifdef EREMOTE
    case EREMOTE:
        name = "EREMOTE";  // Object is remote
        break;
#endif
#ifdef ENOLINK
    case ENOLINK:
        name = "ENOLINK";  // Link has been severed
        break;
#endif
#ifdef EADV
    case EADV:
        name = "EADV"; // Advertise error
        break;
#endif
#ifdef ESRMNT
    case ESRMNT:
        name = "ESRMNT";   // Srmount error
        break;
#endif
#ifdef ECOMM
    case ECOMM:
        name = "ECOMM";    // Communication error on send
        break;
#endif
#ifdef EPROTO
    case EPROTO:
        name = "EPROTO";   // Protocol error
        break;
#endif
#ifdef EMULTIHOP
    case EMULTIHOP:
        name = "EMULTIHOP";    // Multihop attempted
        break;
#endif
#ifdef EDOTDOT
    case EDOTDOT:
        name = "EDOTDOT";  // RFS specific error
        break;
#endif
    case EBADMSG:
        name = "EBADMSG";  // Bad message
        break;
    case EOVERFLOW:
        name = "EOVERFLOW";    // Value too large for defined data type
        break;
#ifdef ENOTUNIQ
    case ENOTUNIQ:
        name = "ENOTUNIQ"; // Name not unique on network
        break;
#endif
#ifdef EBADFD
    case EBADFD:
        name = "EBADFD";   // File descriptor in bad state
        break;
#endif
#ifdef EREMCHG
    case EREMCHG:
        name = "EREMCHG";  // Remote address changed
        break;
#endif
#ifdef ELIBACC
    case ELIBACC:
        name = "ELIBACC";  // Can not access a needed shared library
        break;
#endif
#ifdef ELIBBAD
    case ELIBBAD:
        name = "ELIBBAD";  // Accessing a corrupted shared library
        break;
#endif
#ifdef ELIBSCN
    case ELIBSCN:
        name = "ELIBSCN";  // .lib section in a.out corrupted
        break;
#endif
#ifdef ELIBMAX
    case ELIBMAX:
        name = "ELIBMAX";  // Attempting to link in too many shared libraries
        break;
#endif
#ifdef ELIBEXEC
    case ELIBEXEC:
        name = "ELIBEXEC"; // Cannot exec a shared library directly
        break;
#endif
#ifdef EILSEQ
    case EILSEQ:
        name = "EILSEQ";   // Invalid or incomplete multibyte or wide character
        break;
#endif
#ifdef ERESTART
    case ERESTART:
        name = "ERESTART"; // Interrupted system call should be restarted
        break;
#endif
#ifdef ESTRPIPE
    case ESTRPIPE:
        name = "ESTRPIPE"; // Streams pipe error
        break;
#endif
#ifdef EUSERS
    case EUSERS:
        name = "EUSERS";   // Too many users
        break;
#endif
    case ENOTSOCK:
        name = "ENOTSOCK"; // Socket operation on non-socket
        break;
    case EDESTADDRREQ:
        name = "EDESTADDRREQ"; // Destination address required
        break;
    case EMSGSIZE:
        name = "EMSGSIZE"; // Message too long
        break;
    case EPROTOTYPE:
        name = "EPROTOTYPE";   // Protocol wrong type for socket
        break;
    case ENOPROTOOPT:
        name = "ENOPROTOOPT";  // Protocol not available
        break;
    case EPROTONOSUPPORT:
        name = "EPROTONOSUPPORT";  // Protocol not supported
        break;
#ifdef ESOCKTNOSUPPORT
    case ESOCKTNOSUPPORT:
        name = "ESOCKTNOSUPPORT";  // Socket type not supported
        break;
#endif
#if ENOTSUP == EOPNOTSUPP
    case EOPNOTSUPP:
        name = "EOPNOTSUPP";   // Operation not supported
        break;
#else
    case EOPNOTSUPP:
        name = "EOPNOTSUPP";   // Operation not supported
        break;
    case ENOTSUP:
        name = "ENOTSUP";  // Operation not supported */
        break;
#endif

#ifdef EPFNOSUPPORT
    case EPFNOSUPPORT:
        name = "EPFNOSUPPORT"; // Protocol family not supported
        break;
#endif
    case EAFNOSUPPORT:
        name = "EAFNOSUPPORT"; // Address family not supported by protocol
        break;
    case EADDRINUSE:
        name = "EADDRINUSE";   // Address already in use
        break;
    case EADDRNOTAVAIL:
        name = "EADDRNOTAVAIL";    // Cannot assign requested address
        break;
    case ENETDOWN:
        name = "ENETDOWN"; // Network is down
        break;
    case ENETUNREACH:
        name = "ENETUNREACH";  // Network is unreachable
        break;
    case ENETRESET:
        name = "ENETRESET";    // Network dropped connection on reset
        break;
    case ECONNABORTED:
        name = "ECONNABORTED"; // Software caused connection abort
        break;
    case ECONNRESET:
        name = "ECONNRESET";   // Connection reset by peer
        break;
    case ENOBUFS:
        name = "ENOBUFS";  // No buffer space available
        break;
    case EISCONN:
        name = "EISCONN";  // Transport endpoint is already connected
        break;
    case ENOTCONN:
        name = "ENOTCONN"; // Transport endpoint is not connected
        break;
#ifdef ESHUTDOWN
    case ESHUTDOWN:
        name = "ESHUTDOWN";    // Cannot send after transport endpoint shutdown
        break;
#endif
#ifdef ETOOMANYREFS
    case ETOOMANYREFS:
        name = "ETOOMANYREFS"; // Too many references: cannot splice
        break;
#endif
    case ETIMEDOUT:
        name = "ETIMEDOUT";    // Connection timed out
        break;
    case ECONNREFUSED:
        name = "ECONNREFUSED"; // Connection refused
        break;
#ifdef EHOSTDOWN
    case EHOSTDOWN:
        name = "EHOSTDOWN";    // Host is down
        break;
#endif
    case EHOSTUNREACH:
        name = "EHOSTUNREACH"; // No route to host
        break;
    case EALREADY:
        name = "EALREADY"; // Operation already in progress
        break;
    case EINPROGRESS:
        name = "EINPROGRESS";  // Operation now in progress
        break;
    case ESTALE:
        name = "ESTALE";   // Stale file handle
        break;
#ifdef EUCLEAN
    case EUCLEAN:
        name = "EUCLEAN";  // Structure needs cleaning
        break;
#endif
#ifdef ENOTNAM
    case ENOTNAM:
        name = "ENOTNAM";  // Not a XENIX name =d type file
        break;
#endif
#ifdef ENAVAIL
    case ENAVAIL:
        name = "ENAVAIL";  // No XENIX semaphores available
        break;
#endif
#ifdef EISNAM
    case EISNAM:
        name = "EISNAM";   // Is a name =d type file
        break;
#endif
#ifdef EREMOTEIO
    case EREMOTEIO:
        name = "EREMOTEIO";    // Remote I/O error
        break;
#endif
    case EDQUOT:
        name = "EDQUOT";   // Disk quota exceeded
        break;
#ifdef ENOMEDIUM
    case ENOMEDIUM:
        name = "ENOMEDIUM";    // No medium found
        break;
#endif
#ifdef EMEDIUMTYPE
    case EMEDIUMTYPE:
        name = "EMEDIUMTYPE";  // Wrong medium type
        break;
#endif
    case ECANCELED:
        name = "ECANCELED";    // Operation canceled
        break;
#ifdef ENOKEY
    case ENOKEY:
        name = "ENOKEY";   // Required key not available
        break;
#endif
#ifdef EKEYEXPIRED
    case EKEYEXPIRED:
        name = "EKEYEXPIRED";  // Key has expired
        break;
#endif
#ifdef EKEYREVOKED
    case EKEYREVOKED:
        name = "EKEYREVOKED";  // Key has been revoked
        break;
#endif
#ifdef EKEYREJECTED
    case EKEYREJECTED:
        name = "EKEYREJECTED"; // Key was rejected by service
        break;
#endif
    case EOWNERDEAD:
        name = "EOWNERDEAD";   // Owner died
        break;
    case ENOTRECOVERABLE:
        name = "ENOTRECOVERABLE";  // State not recoverable
        break;
#ifdef ERFKILL
    case ERFKILL:
        name = "ERFKILL";  // Operation not possible due to RF-kill
        break;
#endif
#ifdef EHWPOISON
    case EHWPOISON:
        name = "EHWPOISON";    // Memory page has hardware error
        break;
#endif
    };

    if (name)
        return name;

    return "ErrorUnknown";
}

#endif /* !HAVE(STRERRORNAME_NP) */

