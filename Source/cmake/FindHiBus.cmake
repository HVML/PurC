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
FindHiBus
--------------

Find HiBus headers and libraries.

Imported Targets
^^^^^^^^^^^^^^^^

``HiBus::HiBus``
  The HiBus library, if found.

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables in your project:

``HiBus_FOUND``
  true if (the requested version of) HiBus is available.
``HiBus_VERSION``
  the version of HiBus.
``HiBus_LIBRARIES``
  the libraries to link against to use HiBus.
``HiBus_INCLUDE_DIRS``
  where to find the HiBus headers.
``HiBus_COMPILE_OPTIONS``
  this should be passed to target_compile_options(), if the
  target is not used for linking

#]=======================================================================]

find_path(HiBus_INCLUDE_DIR NAMES hibus.h)

find_library(HiBus_LIBRARY NAMES ${HiBus_NAMES} hibus)

if (HiBus_INCLUDE_DIR)
    if (EXISTS "${HiBus_INCLUDE_DIR}/hibus.h")
        file(STRINGS ${HiBus_INCLUDE_DIR}/hibus.h _ver_line
            REGEX "^#define HIBUS_PROTOCOL_VERSION  *[0-9]+"
            LIMIT_COUNT 1)
        string(REGEX MATCH "[0-9]+"
            HiBus_VERSION "${_ver_line}")
        unset(_ver_line)
    endif ()
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(HiBus
    FOUND_VAR HiBus_FOUND
    REQUIRED_VARS HiBus_LIBRARY HiBus_INCLUDE_DIR
    VERSION_VAR HiBus_VERSION
)

if (HiBus_LIBRARY AND NOT TARGET HiBus::HiBus)
    add_library(HiBus::HiBus UNKNOWN IMPORTED GLOBAL)
    set_target_properties(HiBus::HiBus PROPERTIES
        IMPORTED_LOCATION "${HiBus_LIBRARY}"
        INTERFACE_COMPILE_OPTIONS "${HiBus_COMPILE_OPTIONS}"
        INTERFACE_INCLUDE_DIRECTORIES "${HiBus_INCLUDE_DIR}"
    )
endif ()

mark_as_advanced(HiBus_INCLUDE_DIR HiBus_LIBRARIES)

if (HiBus_FOUND)
    set(HiBus_LIBRARIES ${HiBus_LIBRARY})
    set(HiBus_INCLUDE_DIRS ${HiBus_INCLUDE_DIR})
endif ()
