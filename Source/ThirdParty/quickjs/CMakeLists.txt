set_property(DIRECTORY . PROPERTY FOLDER "QuickJS")

include(PurCCommon)

set(QuickJS_DIR "${THIRDPARTY_DIR}/quickjs")

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

PURC_ADD_TARGET_C_FLAGS(QuickJS
    "-D_GNU_SOURCE"
    "-DCONFIG_VERSION=\\\"${QuickJS_VERSION}\\\""
    "-fwrapv"
    "-funsigned-char"
    "-Wno-sign-compare"
    "-Wno-missing-field-initializers"
    "-Wno-unused-parameter"
    "-Wno-cast-align"
    "-Wno-cast-function-type-mismatch"
)

PURC_COPY_FILES(QuickJS_CopyHeaders
    DESTINATION ${FORWARDING_HEADERS_DIR}/quickjs
    FILES ${QuickJS_FRAMEWORK_HEADERS}
    FLATTENED
)
list(APPEND QuickJS_INTERFACE_DEPENDENCIES QuickJS_CopyHeaders)

install(FILES ${QuickJS_INSTALLED_HEADERS}
        DESTINATION "${HEADER_INSTALL_DIR}/purc"
)

