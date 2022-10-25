# - Try to find LibCheck
# This module defines the following variables:
#
#  LIBCHECK_FOUND - LibCheck was found
#  LIBCHECK_INCLUDE_DIRS - the LibCheck include directories
#  LIBCHECK_LIBRARIES - link these to use LibCheck
#
# Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
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

# LibCheck does not provide an easy way to retrieve its version other than its
# .pc file, so we need to rely on PC_LIBCHECK_VERSION and REQUIRE the .pc file
# to be found.
find_package(PkgConfig QUIET)
pkg_check_modules(PC_LIBCHECK QUIET check)

find_path(LIBCHECK_INCLUDE_DIRS
    NAMES check.h
    HINTS ${PC_LIBCHECK_INCLUDEDIR}
          ${PC_LIBCHECK_INCLUDE_DIRS}
)

find_library(LIBCHECK_LIBRARIES
    NAMES check
    HINTS ${PC_LIBCHECK_LIBDIR}
          ${PC_LIBCHECK_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibCheck REQUIRED_VARS LIBCHECK_INCLUDE_DIRS LIBCHECK_LIBRARIES
                                          VERSION_VAR   PC_LIBCHECK_VERSION)

mark_as_advanced(
    LIBCHECK_INCLUDE_DIRS
    LIBCHECK_LIBRARIES
)
