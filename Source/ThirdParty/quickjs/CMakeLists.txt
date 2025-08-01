set(QUICKJS_DIR "${THIRDPARTY_DIR}/quickjs")

set(QUICKJS_SOURCES
    ${QUICKJS_DIR}/quickjs/quickjs.c
    ${QUICKJS_DIR}/quickjs/dtoa.c
    ${QUICKJS_DIR}/quickjs/libregexp.c
    ${QUICKJS_DIR}/quickjs/libunicode.c
    ${QUICKJS_DIR}/quickjs/cutils.c
    ${QUICKJS_DIR}/quickjs/quickjs-libc.c
)

add_library(quickjs STATIC ${QUICKJS_SOURCES})

set(QUICKJS_INCLUDE_DIRS "${QUICKJS_DIR}" PARENT_SCOPE)

file(READ ${QUICKJS_DIR}/quickjs/VERSION QUICKJS_VERSION)
string(STRIP ${QUICKJS_VERSION} QUICKJS_VERSION)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE -DCONFIG_VERSION=\\\"${QUICKJS_VERSION}\\\" -fwrapv -funsigned-char -Wno-sign-compare -Wno-missing-field-initializers -Wno-unused-parameter -Wno-cast-align -Wno-cast-function-type-mismatch")
