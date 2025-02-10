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
    static const char *errornames[] = {
        "0",
        "EPERM",    // 1 Operation not permitted
        "ENOENT",   // 2 No such file or directory
        "ESRCH",    // 3 No such process
        "EINTR",    // 4 Interrupted system call
        "EIO",  // 5 Input/output error
        "ENXIO",    // 6 No such device or address
        "E2BIG",    // 7 Argument list too long
        "ENOEXEC",  // 8 Exec format error
        "EBADF",    // 9 Bad file descriptor
        "ECHILD",   // 10 No child processes
        "EAGAIN",   // 11 Resource temporarily unavailable
        "ENOMEM",   // 12 Cannot allocate memory
        "EACCES",   // 13 Permission denied
        "EFAULT",   // 14 Bad address
        "ENOTBLK",  // 15 Block device required
        "EBUSY",    // 16 Device or resource busy
        "EEXIST",   // 17 File exists
        "EXDEV",    // 18 Invalid cross-device link
        "ENODEV",   // 19 No such device
        "ENOTDIR",  // 20 Not a directory
        "EISDIR",   // 21 Is a directory
        "EINVAL",   // 22 Invalid argument
        "ENFILE",   // 23 Too many open files in system
        "EMFILE",   // 24 Too many open files
        "ENOTTY",   // 25 Inappropriate ioctl for device
        "ETXTBSY",  // 26 Text file busy
        "EFBIG",    // 27 File too large
        "ENOSPC",   // 28 No space left on device
        "ESPIPE",   // 29 Illegal seek
        "EROFS",    // 30 Read-only file system
        "EMLINK",   // 31 Too many links
        "EPIPE",    // 32 Broken pipe
        "EDOM", // 33 Numerical argument out of domain
        "ERANGE",   // 34 Numerical result out of range
        "EDEADLK",  // 35 Resource deadlock avoided
        "ENAMETOOLONG", // 36 File name too long
        "ENOLCK",   // 37 No locks available
        "ENOSYS",   // 38 Function not implemented
        "ENOTEMPTY",    // 39 Directory not empty
        "ELOOP",    // 40 Too many levels of symbolic links
        "EWOULDBLOCK",  // 11 Resource temporarily unavailable
        "ENOMSG",   // 42 No message of desired type
        "EIDRM",    // 43 Identifier removed
        "ECHRNG",   // 44 Channel number out of range
        "EL2NSYNC", // 45 Level 2 not synchronized
        "EL3HLT",   // 46 Level 3 halted
        "EL3RST",   // 47 Level 3 reset
        "ELNRNG",   // 48 Link number out of range
        "EUNATCH",  // 49 Protocol driver not attached
        "ENOCSI",   // 50 No CSI structure available
        "EL2HLT",   // 51 Level 2 halted
        "EBADE",    // 52 Invalid exchange
        "EBADR",    // 53 Invalid request descriptor
        "EXFULL",   // 54 Exchange full
        "ENOANO",   // 55 No anode
        "EBADRQC",  // 56 Invalid request code
        "EBADSLT",  // 57 Invalid slot
        "EDEADLOCK",    // 35 Resource deadlock avoided
        "EBFONT",   // 59 Bad font file format
        "ENOSTR",   // 60 Device not a stream
        "ENODATA",  // 61 No data available
        "ETIME",    // 62 Timer expired
        "ENOSR",    // 63 Out of streams resources
        "ENONET",   // 64 Machine is not on the network
        "ENOPKG",   // 65 Package not installed
        "EREMOTE",  // 66 Object is remote
        "ENOLINK",  // 67 Link has been severed
        "EADV", // 68 Advertise error
        "ESRMNT",   // 69 Srmount error
        "ECOMM",    // 70 Communication error on send
        "EPROTO",   // 71 Protocol error
        "EMULTIHOP",    // 72 Multihop attempted
        "EDOTDOT",  // 73 RFS specific error
        "EBADMSG",  // 74 Bad message
        "EOVERFLOW",    // 75 Value too large for defined data type
        "ENOTUNIQ", // 76 Name not unique on network
        "EBADFD",   // 77 File descriptor in bad state
        "EREMCHG",  // 78 Remote address changed
        "ELIBACC",  // 79 Can not access a needed shared library
        "ELIBBAD",  // 80 Accessing a corrupted shared library
        "ELIBSCN",  // 81 .lib section in a.out corrupted
        "ELIBMAX",  // 82 Attempting to link in too many shared libraries
        "ELIBEXEC", // 83 Cannot exec a shared library directly
        "EILSEQ",   // 84 Invalid or incomplete multibyte or wide character
        "ERESTART", // 85 Interrupted system call should be restarted
        "ESTRPIPE", // 86 Streams pipe error
        "EUSERS",   // 87 Too many users
        "ENOTSOCK", // 88 Socket operation on non-socket
        "EDESTADDRREQ", // 89 Destination address required
        "EMSGSIZE", // 90 Message too long
        "EPROTOTYPE",   // 91 Protocol wrong type for socket
        "ENOPROTOOPT",  // 92 Protocol not available
        "EPROTONOSUPPORT",  // 93 Protocol not supported
        "ESOCKTNOSUPPORT",  // 94 Socket type not supported
        "EOPNOTSUPP",   // 95 Operation not supported
        "EPFNOSUPPORT", // 96 Protocol family not supported
        "EAFNOSUPPORT", // 97 Address family not supported by protocol
        "EADDRINUSE",   // 98 Address already in use
        "EADDRNOTAVAIL",    // 99 Cannot assign requested address
        "ENETDOWN", // 100 Network is down
        "ENETUNREACH",  // 101 Network is unreachable
        "ENETRESET",    // 102 Network dropped connection on reset
        "ECONNABORTED", // 103 Software caused connection abort
        "ECONNRESET",   // 104 Connection reset by peer
        "ENOBUFS",  // 105 No buffer space available
        "EISCONN",  // 106 Transport endpoint is already connected
        "ENOTCONN", // 107 Transport endpoint is not connected
        "ESHUTDOWN",    // 108 Cannot send after transport endpoint shutdown
        "ETOOMANYREFS", // 109 Too many references: cannot splice
        "ETIMEDOUT",    // 110 Connection timed out
        "ECONNREFUSED", // 111 Connection refused
        "EHOSTDOWN",    // 112 Host is down
        "EHOSTUNREACH", // 113 No route to host
        "EALREADY", // 114 Operation already in progress
        "EINPROGRESS",  // 115 Operation now in progress
        "ESTALE",   // 116 Stale file handle
        "EUCLEAN",  // 117 Structure needs cleaning
        "ENOTNAM",  // 118 Not a XENIX named type file
        "ENAVAIL",  // 119 No XENIX semaphores available
        "EISNAM",   // 120 Is a named type file
        "EREMOTEIO",    // 121 Remote I/O error
        "EDQUOT",   // 122 Disk quota exceeded
        "ENOMEDIUM",    // 123 No medium found
        "EMEDIUMTYPE",  // 124 Wrong medium type
        "ECANCELED",    // 125 Operation canceled
        "ENOKEY",   // 126 Required key not available
        "EKEYEXPIRED",  // 127 Key has expired
        "EKEYREVOKED",  // 128 Key has been revoked
        "EKEYREJECTED", // 129 Key was rejected by service
        "EOWNERDEAD",   // 130 Owner died
        "ENOTRECOVERABLE",  // 131 State not recoverable
        "ERFKILL",  // 132 Operation not possible due to RF-kill
        "EHWPOISON",    // 133 Memory page has hardware error
        /* "ENOTSUP",  // 95 Operation not supported */
    };

    if (errnum < (int)PCA_TABLESIZE(errornames)) {
        return errornames[errnum];
    }

    return "ErrorUnknown";
}

#endif /* !HAVE(STRERRORNAME_NP) */

