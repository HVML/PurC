set_property(DIRECTORY . PROPERTY FOLDER "PurC")

include(PurCCommon)
include(MyMacros.cmake)

configure_file(include/purc-version.h.in ${PurC_DERIVED_SOURCES_DIR}/purc-version.h)
configure_file(include/purc-features.h.in ${PurC_DERIVED_SOURCES_DIR}/purc-features.h)

set(PurC_PRIVATE_INCLUDE_DIRECTORIES
    "${CMAKE_BINARY_DIR}"
    "${PURC_DIR}"
    "${PURC_DIR}/include"
    "${PurC_DERIVED_SOURCES_DIR}"
    "${FORWARDING_HEADERS_DIR}"
)

# TODO: List the headers should be installed to system here.
set(PurC_INSTALLED_HEADERS
    "${PurC_DERIVED_SOURCES_DIR}/purc-version.h"
    "${PurC_DERIVED_SOURCES_DIR}/purc-features.h"
    "${PURC_DIR}/include/purc-macros.h"
    "${PURC_DIR}/include/purc-errors.h"
    "${PURC_DIR}/include/purc-variant.h"
    "${PURC_DIR}/include/purc-rwstream.h"
    "${PURC_DIR}/include/purc.h"
)

# TODO: List the directorires which do not contain any platform-dependent C source files
set(PurC_PLATFORM_INDEPENDENT_DIRS
    "instance"
    "utils"
)

# This macro will append all C/CXX source files in ${PurC_PLATFORM_INDEPENDENT_DIRS}
# to ${PurC_SOURCES}
APPEND_ALL_SOURCE_FILES_IN_DIRLIST(PurC_SOURCES "${PurC_PLATFORM_INDEPENDENT_DIRS}")

# TODO: List the source files individually.
list(APPEND PurC_SOURCES
    "${PURC_DIR}/ports/vasprintf.c"
    "${PURC_DIR}/ports/misc.c"
#    "${PURC_DIR}/variant/variant_module.c"
#    "${PURC_DIR}/variant/variant_basic.c"
    "${PURC_DIR}/variant/variant.c"
)

# Put the CXX source files which can be unified in `Sources.txt`
list(APPEND PurC_UNIFIED_SOURCE_LIST_FILES
    "Sources.txt"
)

set(PurC_LIBRARIES)

# TODO: the source files and libraries for optional features.
if (ENABLE_WEBSOCKET)
    list(APPEND PurC_UNIFIED_SOURCE_LIST_FILES
        "SourcesWebSocket.txt"
    )
    list(APPEND PurC_PRIVATE_INCLUDE_DIRECTORIES
    )
    list(APPEND PurC_SOURCES 
    )
endif ()

if (ENABLE_LSQL)
    list(APPEND PurC_LIBRARIES SQLite::SQLite3)
endif ()

if (ENABLE_XML)
    list(APPEND PurC_LIBRARIES LibXml2::LibXml2)
endif ()

set(PurC_INTERFACE_LIBRARIES PurC)
set(PurC_INTERFACE_INCLUDE_DIRECTORIES ${PurC_PRIVATE_FRAMEWORK_HEADERS_DIR})

PURC_FRAMEWORK_DECLARE(PurC)
PURC_INCLUDE_CONFIG_FILES_IF_EXISTS()

PURC_WRAP_SOURCELIST(${PurC_SOURCES})

PURC_COPY_FILES(PurC_CopyPrivateHeaders
    DESTINATION ${PurC_PRIVATE_FRAMEWORK_HEADERS_DIR}/PurC
    FILES ${PurC_PRIVATE_FRAMEWORK_HEADERS}
    FLATTENED
)
list(APPEND PurC_INTERFACE_DEPENDENCIES PurC_CopyPrivateHeaders)
# PurC_CopyPrivateHeaders needs to have a direct or indirect
# dependency of PurC target for CMake Visual Studio generator to
# eliminate duplicated custom commands. Otherwise, CSSPropertyNames.h
# will be generated in both projects.
if (NOT INTERNAL_BUILD)
    add_dependencies(PurC_CopyPrivateHeaders PurC)
endif ()

PURC_COMPUTE_SOURCES(PurC)
PURC_FRAMEWORK(PurC)
PURC_FRAMEWORK_TARGET(PurC)

# The -ftree-sra optimization (implicit with -O2) causes crashes when
# allocating large chunks of memory using bmalloc on Intel 32bit.
# See https://bugs.webkit.org/show_bug.cgi?id=146440
string(TOLOWER ${CMAKE_HOST_SYSTEM_PROCESSOR} LOWERCASE_CMAKE_HOST_SYSTEM_PROCESSOR)
if (CMAKE_COMPILER_IS_GNUCXX AND "${LOWERCASE_CMAKE_HOST_SYSTEM_PROCESSOR}" MATCHES "(i[3-6]86|x86)$")
    PURC_ADD_TARGET_CXX_FLAGS(PurC -fno-tree-sra)
endif ()

if (${PurC_LIBRARY_TYPE} MATCHES "SHARED")
    set_target_properties(PurC PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})
    install(TARGETS PurC DESTINATION "${LIB_INSTALL_DIR}")
endif ()

if (MSVC)
    set_target_properties(PurC PROPERTIES
        COMPILE_PDB_NAME ${PurC_OUTPUT_NAME}
    )
endif ()

install(TARGETS PurC
        DESTINATION "${LIB_INSTALL_DIR}/"
)

install(FILES ${PurC_INSTALLED_HEADERS}
        DESTINATION "${HEADER_INSTALL_DIR}/purc"
)
