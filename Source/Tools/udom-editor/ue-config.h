/**
 * @file config.h
 * @author Vincent Wei (https://github.com/VincentWei)
 * @date 2021/11/05
 * @brief The configuration header file for UE (udom-editor).
 *
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of UE (short for eDOM Editor), a uDOM renderer
 * in text-mode.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#if defined(HAVE_CONFIG_H) && HAVE_CONFIG_H && defined(BUILDING_WITH_CMAKE)
#include "cmakeconfig.h"
#endif

#include <wtf/Platform.h>
#include <wtf/ExportMacros.h>

#define PACKAGE                     "udom-editor"
#define SEARCH_TYPE_GLIB            1
#define USE_NCURSES                 1
#define SIG_ATOMIC_VOLATILE_T       sig_atomic_t

#if HAVE(MODE_T_LT_INT)
#define PROMOTED_MODE_T             int
#else
#define PROMOTED_MODE_T             mode_t
#endif

/* foce undefine the following macros to disable the features */
#undef HAVE_TEXTMODE_X11_SUPPORT
#undef HAVE_CHARSET
#undef HAVE_ASPELL
#undef HAVE_LIBGPM
#undef HAVE_SLANG
#undef HAVE_QNX_KEYS

/* Undefine the macros for macros defined as 0 in cmakeconfig.h */
#if !HAVE(STRVERSCMP)
#undef HAVE_STRVERSCMP
#endif

#if !HAVE(POSIX_FALLOCATE)
#undef HAVE_POSIX_FALLOCATE
#endif

#if !HAVE(REALPATH)
#undef HAVE_REALPATH
#endif

#if !HAVE(RESIZETERM)
#undef HAVE_RESIZETERM
#endif

#if !HAVE(STRING_H)
#undef HAVE_STRING_H
#endif

#if !HAVE(MEMORY_H)
#undef HAVE_MEMORY_H
#endif

#if !HAVE(STRNCASECMP)
#undef HAVE_STRNCASECMP
#endif

#if !HAVE(STRUCT_STAT_ST_BLKSIZE)
#undef HAVE_STRUCT_STAT_ST_BLKSIZE
#endif

#if !HAVE(STRUCT_STAT_ST_BLOCKS)
#undef HAVE_STRUCT_STAT_ST_BLOCKS
#endif

#if !HAVE(STRUCT_STAT_ST_MTIM)
#undef HAVE_STRUCT_STAT_ST_MTIM
#endif

#if !HAVE(STRUCT_STAT_ST_RDEV)
#undef HAVE_STRUCT_STAT_ST_RDEV
#endif

#if !HAVE(STRCASECMP)
#undef HAVE_STRCASECMP
#endif

#if !HAVE(STRNCASECMP)
#undef HAVE_STRNCASECMP
#endif

#if !HAVE(STRVERSCMP)
#undef HAVE_STRVERSCMP
#endif

#if !HAVE(SYS_IOCTL_H)
#undef HAVE_SYS_IOCTL_H
#endif

#if !HAVE(SYS_PARAM_H)
#undef HAVE_SYS_PARAM_H
#endif

#if !HAVE(SYS_SELECT_H)
#undef HAVE_SYS_SELECT_H
#endif

#if !HAVE(UTIME_H)
#undef HAVE_UTIME_H
#endif

#if !HAVE(LINUX_FS_H)
#undef HAVE_LINUX_FS_H
#endif

#if !HAVE(UTIMENSAT)
#undef HAVE_UTIMENSAT
#endif

#if !HAVE(GET_PROCESS_STATS)
#undef HAVE_GET_PROCESS_STATS
#endif

#if !HAVE(POSIX_FALLOCATE)
#undef HAVE_POSIX_FALLOCATE
#endif
