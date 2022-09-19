# Copyright (C) 2021 Beijing FMSoft Technologies Co., Ltd
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND ITS CONTRIBUTORS ``AS
# IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR ITS
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#[=======================================================================[.rst:
FindNcurses
--------------

Find Ncurses headers and libraries.

Imported Targets
^^^^^^^^^^^^^^^^

``Ncurses::Ncurses``
  The Ncurses library, if found.

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables in your project:

``Ncurses_FOUND``
  true if (the requested version of) Ncurses is available.
``Ncurses_VERSION``
  the version of Ncurses.
``Ncurses_LIBRARIES``
  the libraries to link against to use Ncurses.
``Ncurses_INCLUDE_DIRS``
  where to find the Ncurses headers.
``Ncurses_COMPILE_OPTIONS``
  this should be passed to target_compile_options(), if the
  target is not used for linking

#]=======================================================================]

# TODO: Remove when cmake_minimum_version bumped to 3.14.

find_package(PkgConfig QUIET)
pkg_check_modules(PC_NCURSES QUIET ncurses)
set(Ncurses_COMPILE_OPTIONS ${PC_NCURSES_CFLAGS_OTHER})
set(Ncurses_VERSION ${PC_NCURSES_VERSION})

find_path(Ncurses_INCLUDE_DIR
    NAMES ncurses.h
    HINTS ${PC_NCURSES_INCLUDEDIR} ${PC_NCURSES_INCLUDE_DIR}
)

# try ncursesw first
find_library(Ncurses_LIBRARY
    NAMES ${Ncurses_NAMES} ncursesw
    HINTS ${PC_NCURSES_LIBDIR} ${PC_NCURSES_LIBRARY_DIRS}
)

# if not ncursesw, try ncurses then
if (NOT Ncurses_LIBRARY)
    find_library(Ncurses_LIBRARY
        NAMES ${Ncurses_NAMES} ncurses
        HINTS ${PC_NCURSES_LIBDIR} ${PC_NCURSES_LIBRARY_DIRS}
    )
endif ()

if (Ncurses_INCLUDE_DIR AND NOT Ncurses_VERSION)
    if (EXISTS "${Ncurses_INCLUDE_DIR}/ncurses.h")
        file(STRINGS ${Ncurses_INCLUDE_DIR}/ncurses.h _ver_line
            REGEX "^#define NCURSES_VERSION *\"[0-9]+\\.[0-9]+\""
            LIMIT_COUNT 1)
        string(REGEX MATCH "[0-9]+\\.[0-9]+"
            Ncurses_VERSION "${_ver_line}")
        unset(_ver_line)
    endif ()
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Ncurses
    FOUND_VAR Ncurses_FOUND
    REQUIRED_VARS Ncurses_LIBRARY Ncurses_INCLUDE_DIR
    VERSION_VAR Ncurses_VERSION
)

if (Ncurses_LIBRARY AND NOT TARGET Ncurses::Ncurses)
    add_library(Ncurses::Ncurses UNKNOWN IMPORTED GLOBAL)
    set_target_properties(Ncurses::Ncurses PROPERTIES
        IMPORTED_LOCATION "${Ncurses_LIBRARY}"
        INTERFACE_COMPILE_OPTIONS "${Ncurses_COMPILE_OPTIONS}"
        INTERFACE_INCLUDE_DIRECTORIES "${Ncurses_INCLUDE_DIR}"
    )
endif ()

mark_as_advanced(Ncurses_INCLUDE_DIR Ncurses_LIBRARIES)

if (Ncurses_FOUND)
    PURC_CHECK_HAVE_SYMBOL(HAVE_ESCDELAY ESCDELAY "${Ncurses_INCLUDE_DIR}/ncurses.h")
    PURC_CHECK_HAVE_SYMBOL(HAVE_RESIZETERM resizeterm "${Ncurses_INCLUDE_DIR}/ncurses.h")

    set(Ncurses_LIBRARIES ${Ncurses_LIBRARY})
    set(Ncurses_INCLUDE_DIRS ${Ncurses_INCLUDE_DIR})
endif ()

