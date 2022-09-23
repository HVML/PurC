include(PurCCommon)

# testmain
PURC_EXECUTABLE_DECLARE(testmain)

list(APPEND testmain_PRIVATE_INCLUDE_DIRECTORIES
    "${CSSENG_DIR}/include"
)

list(APPEND testmain_SYSTEM_INCLUDE_DIRECTORIES
    "${FORWARDING_HEADERS_DIR}/csseng"
    "${LIBCHECK_INCLUDE_DIRS}"
)

list(APPEND testmain_SOURCES
    testmain.c
    basictests.c
)

set(testmain_LIBRARIES
    PurC::CSSEng
    ${LIBCHECK_LIBRARIES}
)

PURC_EXECUTABLE(testmain)
PURC_COMPUTE_SOURCES(testmain)

