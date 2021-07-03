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
FindHiBox
--------------

Find HiBox headers and libraries.

Imported Targets
^^^^^^^^^^^^^^^^

``HiBox::HiBox``
  The HiBox library, if found.

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables in your project:

``HiBox_FOUND``
  true if (the requested version of) HiBox is available.
``HiBox_VERSION``
  the version of HiBox.
``HiBox_LIBRARIES``
  the libraries to link against to use HiBox.
``HiBox_INCLUDE_DIRS``
  where to find the HiBox headers.
``HiBox_COMPILE_OPTIONS``
  this should be passed to target_compile_options(), if the
  target is not used for linking

#]=======================================================================]

find_package(PkgConfig QUIET)
pkg_check_modules(PC_HIBOX QUIET hibox)
set(HiBox_COMPILE_OPTIONS ${PC_HIBOX_CFLAGS_OTHER})
set(HiBox_VERSION ${PC_HIBOX_VERSION})

find_path(HiBox_INCLUDE_DIR
    NAMES hibox/utils.h
    HINTS ${PC_HIBOX_INCLUDEDIR} ${PC_HIBOX_INCLUDE_DIR}
)

find_library(HiBox_LIBRARY
    NAMES ${HiBox_NAMES} hibox
    HINTS ${PC_HIBOX_LIBDIR} ${PC_HIBOX_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(HiBox
    FOUND_VAR HiBox_FOUND
    REQUIRED_VARS HiBox_LIBRARY HiBox_INCLUDE_DIR
    VERSION_VAR HiBox_VERSION
)

if (HiBox_LIBRARY AND NOT TARGET HiBox::HiBox)
    add_library(HiBox::HiBox UNKNOWN IMPORTED GLOBAL)
    set_target_properties(HiBox::HiBox PROPERTIES
        IMPORTED_LOCATION "${HiBox_LIBRARY}"
        INTERFACE_COMPILE_OPTIONS "${HiBox_COMPILE_OPTIONS}"
        INTERFACE_INCLUDE_DIRECTORIES "${HiBox_INCLUDE_DIR}"
    )
endif ()

mark_as_advanced(HiBox_INCLUDE_DIR HiBox_LIBRARIES)

if (HiBox_FOUND)
    set(HiBox_LIBRARIES ${HiBox_LIBRARY})
    set(HiBox_INCLUDE_DIRS ${HiBox_INCLUDE_DIR})
endif ()
