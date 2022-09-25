include(PurCCommon)

# test_wapcaplet
PURC_EXECUTABLE_DECLARE(test_wapcaplet)

list(APPEND test_wapcaplet_PRIVATE_INCLUDE_DIRECTORIES
    "${CSSENG_DIR}/include"
)

list(APPEND test_wapcaplet_SYSTEM_INCLUDE_DIRECTORIES
    "${FORWARDING_HEADERS_DIR}/csseng"
    "${LIBCHECK_INCLUDE_DIRS}"
)

list(APPEND test_wapcaplet_SOURCES
    testmain.c
    basictests.c
)

set(test_wapcaplet_LIBRARIES
    PurC::CSSEng
    ${LIBCHECK_LIBRARIES}
)

PURC_EXECUTABLE(test_wapcaplet)
PURC_COMPUTE_SOURCES(test_wapcaplet)

