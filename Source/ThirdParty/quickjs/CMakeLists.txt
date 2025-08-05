set_property(DIRECTORY . PROPERTY FOLDER "QuickJS")

include(PurCCommon)

set(QuickJS_DIR "${THIRDPARTY_DIR}/quickjs")
set(QuickJS_DERIVED_SOURCES_DIR "${CMAKE_BINARY_DIR}/DerivedSources/QuickJS")

set(QuickJS_FRAMEWORK_HEADERS
    "${QuickJS_DIR}/quickjs/quickjs.h"
)

set(QuickJS_INSTALLED_HEADERS ${QuickJS_FRAMEWORK_HEADERS})

set(QuickJS_SOURCES
    ${QuickJS_DIR}/quickjs/quickjs.c
    ${QuickJS_DIR}/quickjs/dtoa.c
    ${QuickJS_DIR}/quickjs/libregexp.c
    ${QuickJS_DIR}/quickjs/libunicode.c
    ${QuickJS_DIR}/quickjs/cutils.c
    ${QuickJS_DIR}/quickjs/quickjs-libc.c
)

set(QuickJS_INTERFACE_LIBRARIES QuickJS)
set(QuickJS_LIBRARY_TYPE "STATIC")

PURC_FRAMEWORK_DECLARE(QuickJS)
PURC_COMPUTE_SOURCES(QuickJS)
PURC_FRAMEWORK(QuickJS)

set(QuickJS_INCLUDE_DIRS "${QuickJS_DIR}" PARENT_SCOPE)

file(READ ${QuickJS_DIR}/quickjs/VERSION QuickJS_VERSION)
string(STRIP ${QuickJS_VERSION} QuickJS_VERSION)

set(QuickJS_CFLAGS
    "-D_GNU_SOURCE"
    "-DCONFIG_VERSION=\\\"${QuickJS_VERSION}\\\""
    "-fwrapv"
    "-funsigned-char"
    "-Wno-sign-compare"
    "-Wno-missing-field-initializers"
    "-Wno-unused-parameter"
    "-Wno-cast-align"
    "-Wno-cast-function-type"
    "-Wno-cast-function-type-mismatch"
)

PURC_ADD_TARGET_C_FLAGS(QuickJS ${QuickJS_CFLAGS})

PURC_COPY_FILES(QuickJS_CopyHeaders
    DESTINATION ${FORWARDING_HEADERS_DIR}/quickjs
    FILES ${QuickJS_FRAMEWORK_HEADERS}
    FLATTENED
)
list(APPEND QuickJS_INTERFACE_DEPENDENCIES QuickJS_CopyHeaders)

install(FILES ${QuickJS_INSTALLED_HEADERS}
        DESTINATION "${HEADER_INSTALL_DIR}/purc"
)

PURC_EXECUTABLE_DECLARE(qjsc)
PURC_EXECUTABLE(qjsc)

list(APPEND qjsc_SOURCES
    ${QuickJS_DIR}/quickjs/qjsc.c
)

PURC_ADD_TARGET_C_FLAGS(qjsc ${QuickJS_CFLAGS})

set(qjsc_LIBRARIES
    QuickJS
    -lm
)

PURC_COMPUTE_SOURCES(qjsc)
PURC_FRAMEWORK(qjsc)

set_target_properties(qjsc PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    INSTALL_RPATH "${LIB_INSTALL_DIR}/"
)

install(TARGETS qjsc DESTINATION "${EXEC_INSTALL_DIR}/")

PURC_EXECUTABLE_DECLARE(qjs)
PURC_EXECUTABLE(qjs)

list(APPEND qjs_SOURCES
    ${QuickJS_DIR}/quickjs/qjs.c
)

add_custom_command(
    OUTPUT "${QuickJS_DERIVED_SOURCES_DIR}/repl.c"
    MAIN_DEPENDENCY "${QuickJS_DIR}/quickjs/repl.js"
    DEPENDS "${CMAKE_BINARY_DIR}/bin/qjsc"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${QuickJS_DERIVED_SOURCES_DIR}"
    COMMAND ${CMAKE_BINARY_DIR}/bin/qjsc -s -c -o "${QuickJS_DERIVED_SOURCES_DIR}/repl.c" -m "${QuickJS_DIR}/quickjs/repl.js"
    COMMENT "Generating repl.c from repl.js"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    VERBATIM)

list(APPEND qjs_SOURCES "${QuickJS_DERIVED_SOURCES_DIR}/repl.c")

PURC_ADD_TARGET_C_FLAGS(qjs ${QuickJS_CFLAGS})

set(qjs_LIBRARIES
    QuickJS
    -lm
)

PURC_COMPUTE_SOURCES(qjs)
PURC_FRAMEWORK(qjs)

set_target_properties(qjs PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    INSTALL_RPATH "${LIB_INSTALL_DIR}/"
)

install(TARGETS qjs DESTINATION "${EXEC_INSTALL_DIR}/")

