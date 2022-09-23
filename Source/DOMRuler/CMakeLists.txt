set_property(DIRECTORY . PROPERTY FOLDER "DOMRuler")

include(PurCCommon)
include(MyMacros.cmake)

configure_file(include/domruler-version.h.in ${FORWARDING_HEADERS_DIR}/domruler/domruler-version.h @ONLY)

set(DOMRuler_PRIVATE_INCLUDE_DIRECTORIES
#    "${CMAKE_BINARY_DIR}"  for cmakeconfig.h
#    "${DOMRULER_DIR}"      for config.h
    "${FORWARDING_HEADERS_DIR}"
    "${DOMRULER_DIR}/include"
    "${FORWARDING_HEADERS_DIR}/domruler"
)

set(DOMRuler_SYSTEM_INCLUDE_DIRECTORIES
    ${GLIB_INCLUDE_DIRS}
    ${PURC_INCLUDE_DIRS}
)

set(DOMRuler_FRAMEWORK_HEADERS
    "${DOMRULER_DIR}/include/domruler.h"
)

# List the headers should be installed to system here.
set(DOMRuler_INSTALLED_HEADERS ${DOMRuler_FRAMEWORK_HEADERS})

list(APPEND DOMRuler_INSTALLED_HEADERS
    "${FORWARDING_HEADERS_DIR}/domruler/domruler-version.h"
)

# TODO: List the directorires which do not contain any platform-dependent C source files
set(DOMRuler_PLATFORM_INDEPENDENT_DIRS
    "src"
)

# This macro will append all C/CXX source files in ${DOMRuler_PLATFORM_INDEPENDENT_DIRS}
# to ${DOMRuler_SOURCES}
APPEND_ALL_SOURCE_FILES_IN_DIRLIST(DOMRuler_SOURCES "${DOMRuler_PLATFORM_INDEPENDENT_DIRS}")

# TODO: List the source files individually.
list(APPEND DOMRuler_SOURCES
)

# TODO: Put the CXX source files which can be unified in `Sources.txt`
list(APPEND DOMRuler_UNIFIED_SOURCE_LIST_FILES
)

set(DOMRuler_LIBRARIES
    PurC::CSSEng
    PurC::PurC
    ${GLIB_LIBRARIES}
)
set(DOMRuler_DEPENDENCIES)

set(DOMRuler_INTERFACE_LIBRARIES DOMRuler)
set(DOMRuler_INTERFACE_INCLUDE_DIRECTORIES ${DOMRuler_FRAMEWORK_HEADERS_DIR})

PURC_FRAMEWORK_DECLARE(DOMRuler)
PURC_INCLUDE_CONFIG_FILES_IF_EXISTS()

PURC_WRAP_SOURCELIST(${DOMRuler_SOURCES})

PURC_COPY_FILES(DOMRuler_CopyHeaders
    DESTINATION ${DOMRuler_FRAMEWORK_HEADERS_DIR}/domruler
    FILES ${DOMRuler_FRAMEWORK_HEADERS}
    FLATTENED
)
list(APPEND DOMRuler_INTERFACE_DEPENDENCIES DOMRuler_CopyHeaders)

# DOMRuler_CopyHeaders needs to have a direct or indirect
# dependency of DOMRuler target for CMake Visual Studio generator to
# eliminate duplicated custom commands. Otherwise, CSSPropertyNames.h
# will be generated in both projects.
if (NOT INTERNAL_BUILD)
    add_dependencies(DOMRuler_CopyHeaders DOMRuler)
    add_dependencies(DOMRuler PurC)
    add_dependencies(DOMRuler CSSEng)
endif ()

PURC_COMPUTE_SOURCES(DOMRuler)
PURC_FRAMEWORK(DOMRuler)
PURC_FRAMEWORK_TARGET(DOMRuler)

# The -ftree-sra optimization (implicit with -O2) causes crashes when
# allocating large chunks of memory using bmalloc on Intel 32bit.
# See https://bugs.webkit.org/show_bug.cgi?id=146440
string(TOLOWER ${CMAKE_HOST_SYSTEM_PROCESSOR} LOWERCASE_CMAKE_HOST_SYSTEM_PROCESSOR)
if (CMAKE_COMPILER_IS_GNUCXX AND "${LOWERCASE_CMAKE_HOST_SYSTEM_PROCESSOR}" MATCHES "(i[3-6]86|x86)$")
    PURC_ADD_TARGET_CXX_FLAGS(DOMRuler -fno-tree-sra)
endif ()

if (${DOMRuler_LIBRARY_TYPE} MATCHES "SHARED")
    set_target_properties(DOMRuler PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})
    install(TARGETS DOMRuler DESTINATION "${LIB_INSTALL_DIR}")
endif ()

if (MSVC)
    set_target_properties(DOMRuler PROPERTIES
        COMPILE_PDB_NAME ${DOMRuler_OUTPUT_NAME}
    )
endif ()

install(TARGETS DOMRuler
        DESTINATION "${LIB_INSTALL_DIR}/"
)

install(FILES ${DOMRuler_INSTALLED_HEADERS}
        DESTINATION "${HEADER_INSTALL_DIR}/domruler"
)

