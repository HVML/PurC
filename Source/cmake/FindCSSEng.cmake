# Copyright (C) 2022 FMSoft.
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
FindCSSEng
--------------

Find CSSEng headers and libraries.

Imported Targets
^^^^^^^^^^^^^^^^

``CSSEng::CSSEng``
  The CSSEng library, if found.

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables in your project:

``CSSEng_FOUND``
  true if (the requested version of) CSSEng is available.
``CSSEng_VERSION``
  the version of CSSEng.
``CSSEng_LIBRARIES``
  the libraries to link against to use CSSEng.
``CSSEng_INCLUDE_DIRS``
  where to find the CSSEng headers.
``CSSEng_COMPILE_OPTIONS``
  this should be passed to target_compile_options(), if the
  target is not used for linking

#]=======================================================================]

find_package(PkgConfig QUIET)
pkg_check_modules(PC_CSSENG QUIET csseng)
set(CSSEng_COMPILE_OPTIONS ${PC_CSSENG_CFLAGS_OTHER})
set(CSSEng_VERSION ${PC_CSSENG_VERSION})

find_path(CSSEng_INCLUDE_DIR
    NAMES csseng/csseng.h
    HINTS ${PC_CSSENG_INCLUDEDIR} ${PC_CSSENG_INCLUDE_DIR}
)

find_library(CSSEng_LIBRARY
    NAMES ${CSSEng_NAMES} csseng
    HINTS ${PC_CSSENG_LIBDIR} ${PC_CSSENG_LIBRARY_DIRS}
)

if (CSSEng_INCLUDE_DIR AND NOT CSSEng_VERSION)
    if (EXISTS "${CSSEng_INCLUDE_DIR}/csseng/csseng-version.h")
        file(STRINGS ${CSSEng_INCLUDE_DIR}/csseng/csseng-version.h _ver_line
            REGEX "^#define CSSENG_VERSION_STRING  *\"[0-9]+\\.[0-9]+\\.[0-9]+\""
            LIMIT_COUNT 1)
        string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+"
            CSSEng_VERSION "${_ver_line}")
        unset(_ver_line)
    endif ()
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CSSEng
    FOUND_VAR CSSEng_FOUND
    REQUIRED_VARS CSSEng_LIBRARY CSSEng_INCLUDE_DIR
    VERSION_VAR CSSEng_VERSION
)

if (CSSEng_LIBRARY AND NOT TARGET CSSEng::CSSEng)
    add_library(CSSEng::CSSEng UNKNOWN IMPORTED GLOBAL)
    set_target_properties(CSSEng::CSSEng PROPERTIES
        IMPORTED_LOCATION "${CSSEng_LIBRARY}"
        INTERFACE_COMPILE_OPTIONS "${CSSEng_COMPILE_OPTIONS}"
        INTERFACE_INCLUDE_DIRECTORIES "${CSSEng_INCLUDE_DIR}"
    )
endif ()

mark_as_advanced(CSSEng_INCLUDE_DIR CSSEng_LIBRARIES)

if (CSSEng_FOUND)
    set(CSSEng_LIBRARIES ${CSSEng_LIBRARY})
    set(CSSEng_INCLUDE_DIRS ${CSSEng_INCLUDE_DIR})
endif ()

