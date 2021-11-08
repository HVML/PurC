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
    "${CMAKE_BINARY_DIR}/Source/PurC"
)

# TODO: List the headers should be installed to system here.
set(PurC_INSTALLED_HEADERS
    "${PurC_DERIVED_SOURCES_DIR}/purc-version.h"
    "${PurC_DERIVED_SOURCES_DIR}/purc-features.h"
    "${PURC_DIR}/include/purc-macros.h"
    "${PURC_DIR}/include/purc-errors.h"
    "${PURC_DIR}/include/purc-variant.h"
    "${PURC_DIR}/include/purc-rwstream.h"
    "${PURC_DIR}/include/purc-utils.h"
    "${PURC_DIR}/include/purc-ports.h"
    "${PURC_DIR}/include/purc-arraylist.h"
    "${PURC_DIR}/include/purc.h"
)

# TODO: List the directorires which do not contain any platform-dependent C source files
set(PurC_PLATFORM_INDEPENDENT_DIRS
    "instance"
    "utils"
    "variant"
    "html"
    "edom"
    "dvobjs"
    "hvml"
    "vdom"
)

# This macro will append all C/CXX source files in ${PurC_PLATFORM_INDEPENDENT_DIRS}
# to ${PurC_SOURCES}
APPEND_ALL_SOURCE_FILES_IN_DIRLIST(PurC_SOURCES "${PurC_PLATFORM_INDEPENDENT_DIRS}")

# TODO: List the source files individually.
list(APPEND PurC_SOURCES
    "${PURC_DIR}/ports/vasprintf.c"
    "${PURC_DIR}/ports/misc.c"
    "${PURC_DIR}/ejson/ejson.c"
    "${PURC_DIR}/vcm/vcm.c"
)

# Put the CXX source files which can be unified in `Sources.txt`
list(APPEND PurC_UNIFIED_SOURCE_LIST_FILES
    "Sources.txt"
)

set(PurC_LIBRARIES)
set(PurC_DEPENDENCIES)

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

set(dvobjs_PARSERS logical)
PURC_FLEX_BISON(PurC dvobjs "${dvobjs_PARSERS}")
unset(dvobjs_PARSERS)

# Generate constants for HTML ns/tags/interfaces
add_custom_command(
    OUTPUT "${PurC_DERIVED_SOURCES_DIR}/html_foo.c"
        ${PurC_DERIVED_SOURCES_DIR}/ns_const.h
        ${PurC_DERIVED_SOURCES_DIR}/ns_res.h
        ${PurC_DERIVED_SOURCES_DIR}/html_tag_const.h
        ${PurC_DERIVED_SOURCES_DIR}/html_tag_res.h
        ${PurC_DERIVED_SOURCES_DIR}/html_interface_res.h
        ${PurC_DERIVED_SOURCES_DIR}/html_tag_res_ext.h
    MAIN_DEPENDENCY ${PURC_DIR}/html/tags.py
    DEPENDS ${PURC_DIR}/html/interfaces.py
            ${PURC_DIR}/html/ns_const.h.in
            ${PURC_DIR}/html/ns_res.h.in
            ${PURC_DIR}/html/html_interface_res.h.in
            ${PURC_DIR}/html/html_tag_res_ext.h.in
            ${PURC_DIR}/html/html_tag_const.h.in
            ${PURC_DIR}/html/html_tag_res.h.in
    COMMAND ${Python3_EXECUTABLE} ${PURC_DIR}/html/tags.py "${PurC_DERIVED_SOURCES_DIR}" --without-print
    COMMAND touch "${PurC_DERIVED_SOURCES_DIR}/html_foo.c"
    COMMENT "Generating files for HTML tags and interfaces by using ${Python3_EXECUTABLE}"
    WORKING_DIRECTORY "${PURC_DIR}/html/"
    VERBATIM)
list(APPEND PurC_SOURCES ${PurC_DERIVED_SOURCES_DIR}/html_foo.c)

# Generate constants for HTML attributes
add_custom_command(
    OUTPUT "${PurC_DERIVED_SOURCES_DIR}/html_attr_foo.c" "${PurC_DERIVED_SOURCES_DIR}/html_attr_res.h" "${PurC_DERIVED_SOURCES_DIR}/html_attr_const.h"
    MAIN_DEPENDENCY ${PURC_DIR}/html/attr.py
    DEPENDS ${PURC_DIR}/html/attr_res.h.in ${PURC_DIR}/html/attr_const.h.in
    COMMAND ${Python3_EXECUTABLE} ${PURC_DIR}/html/attr.py "${PurC_DERIVED_SOURCES_DIR}" --without-print
    COMMAND touch "${PurC_DERIVED_SOURCES_DIR}/html_attr_foo.c"
    COMMENT "Generating html_attr_res.h and html_attr_const.h by using ${Python3_EXECUTABLE}"
    WORKING_DIRECTORY "${PURC_DIR}/html/"
    VERBATIM)
list(APPEND PurC_SOURCES ${PurC_DERIVED_SOURCES_DIR}/html_attr_foo.c)

# Copy str_res.h.in to DerivedSources
add_custom_command(
    OUTPUT "${PurC_DERIVED_SOURCES_DIR}/html_bar.c" "${PurC_DERIVED_SOURCES_DIR}/str_res.h"
    COMMAND ${CMAKE_COMMAND} -E copy "${PURC_DIR}/html/str_res.h.in"
        "${PurC_DERIVED_SOURCES_DIR}/str_res.h"
    COMMAND touch "${PurC_DERIVED_SOURCES_DIR}/html_bar.c"
    DEPENDS "${PURC_DIR}/html/str_res.h.in"
    COMMENT "Copying str_res.h.in to DerivedSources"
    VERBATIM)
list(APPEND PurC_SOURCES ${PurC_DERIVED_SOURCES_DIR}/html_bar.c)

# Generate attrs table for hvml parser
add_custom_command(
    OUTPUT "${PurC_DERIVED_SOURCES_DIR}/hvml-attr-foo.c"
           "${PurC_DERIVED_SOURCES_DIR}/hvml-attr-static-list.inc"
    MAIN_DEPENDENCY ${PURC_DIR}/hvml/make-attrs-table.py
    DEPENDS "${PURC_DIR}/hvml/data/attrs.txt"
            "${PURC_DIR}/hvml/data/attr-static-list.inc.in"
    COMMAND ${Python3_EXECUTABLE} ${PURC_DIR}/hvml/make-attrs-table.py "${PurC_DERIVED_SOURCES_DIR}" --without-print
    COMMAND touch "${PurC_DERIVED_SOURCES_DIR}/hvml-attr-foo.c"
    COMMENT "Generating hvml-attr-static-list.inc by using ${Python3_EXECUTABLE}"
    WORKING_DIRECTORY "${PURC_DIR}/hvml/"
    VERBATIM)
list(APPEND PurC_SOURCES ${PurC_DERIVED_SOURCES_DIR}/hvml-attr-foo.c)

# Generate tags table for hvml parser
add_custom_command(
    OUTPUT "${PurC_DERIVED_SOURCES_DIR}/hvml-tag-foo.c"
           "${PurC_DERIVED_SOURCES_DIR}/hvml-tag-static-list.inc"
           "${PurC_DERIVED_SOURCES_DIR}/hvml-tag.h"
    MAIN_DEPENDENCY ${PURC_DIR}/hvml/make-tags-table.py
    DEPENDS "${PURC_DIR}/hvml/data/tags.txt"
            "${PURC_DIR}/hvml/data/tag-static-list.inc.in"
            "${PURC_DIR}/hvml/data/tag.h.in"
    COMMAND ${Python3_EXECUTABLE} ${PURC_DIR}/hvml/make-tags-table.py "${PurC_DERIVED_SOURCES_DIR}" --without-print
    COMMAND touch "${PurC_DERIVED_SOURCES_DIR}/hvml-tag-foo.c"
    COMMENT "Generating hvml-tag-static-list.inc/hvml-tag.h by using ${Python3_EXECUTABLE}"
    WORKING_DIRECTORY "${PURC_DIR}/hvml/"
    VERBATIM)
list(APPEND PurC_SOURCES ${PurC_DERIVED_SOURCES_DIR}/hvml-tag-foo.c)



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

# add_subdirectory(res)

install(TARGETS PurC
        DESTINATION "${LIB_INSTALL_DIR}/"
)

install(FILES ${PurC_INSTALLED_HEADERS}
        DESTINATION "${HEADER_INSTALL_DIR}/purc"
)
