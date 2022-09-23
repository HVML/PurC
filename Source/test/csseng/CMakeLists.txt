include(PurCCommon)

# csdetect
PURC_EXECUTABLE_DECLARE(csdetect)

list(APPEND csdetect_PRIVATE_INCLUDE_DIRECTORIES
    "${CSSENG_DIR}/include"
    "${CSSENG_DIR}"
)

list(APPEND csdetect_SYSTEM_INCLUDE_DIRECTORIES
    "${FORWARDING_HEADERS_DIR}/csseng"
)

list(APPEND csdetect_SOURCES
    csdetect.c
)

set(csdetect_LIBRARIES
    CSSEng::CSSEng
)

PURC_EXECUTABLE(csdetect)
PURC_COMPUTE_SOURCES(csdetect)

# lexauto
PURC_EXECUTABLE_DECLARE(lexauto)

list(APPEND lexauto_PRIVATE_INCLUDE_DIRECTORIES
    "${CSSENG_DIR}/include"
    "${CSSENG_DIR}"
)

list(APPEND lexauto_SYSTEM_INCLUDE_DIRECTORIES
    "${FORWARDING_HEADERS_DIR}/csseng"
)

list(APPEND lexauto_SOURCES
    lexauto.c
)

set(lexauto_LIBRARIES
    CSSEng::CSSEng
)

PURC_EXECUTABLE(lexauto)
PURC_COMPUTE_SOURCES(lexauto)

# number
PURC_EXECUTABLE_DECLARE(number)

list(APPEND number_PRIVATE_INCLUDE_DIRECTORIES
    "${CSSENG_DIR}/include"
    "${CSSENG_DIR}"
)

list(APPEND number_SYSTEM_INCLUDE_DIRECTORIES
    "${FORWARDING_HEADERS_DIR}/csseng"
)

list(APPEND number_SOURCES
    number.c
)

set(number_LIBRARIES
    CSSEng::CSSEng
)

PURC_EXECUTABLE(number)
PURC_COMPUTE_SOURCES(number)

# parseauto
PURC_EXECUTABLE_DECLARE(parseauto)

list(APPEND parseauto_PRIVATE_INCLUDE_DIRECTORIES
    "${CSSENG_DIR}/include"
    "${CSSENG_DIR}"
)

list(APPEND parseauto_SYSTEM_INCLUDE_DIRECTORIES
    "${FORWARDING_HEADERS_DIR}/csseng"
)

list(APPEND parseauto_SOURCES
    parseauto.c
)

set(parseauto_LIBRARIES
    CSSEng::CSSEng
)

PURC_EXECUTABLE(parseauto)
PURC_COMPUTE_SOURCES(parseauto)

# parse2auto
PURC_EXECUTABLE_DECLARE(parse2auto)

list(APPEND parse2auto_PRIVATE_INCLUDE_DIRECTORIES
    "${CSSENG_DIR}/include"
    "${CSSENG_DIR}"
)

list(APPEND parse2auto_SYSTEM_INCLUDE_DIRECTORIES
    "${FORWARDING_HEADERS_DIR}/csseng"
)

list(APPEND parse2auto_SOURCES
    parse2auto.c
)

set(parse2auto_LIBRARIES
    CSSEng::CSSEng
)

PURC_EXECUTABLE(parse2auto)
PURC_COMPUTE_SOURCES(parse2auto)

# select
PURC_EXECUTABLE_DECLARE(select)

list(APPEND select_PRIVATE_INCLUDE_DIRECTORIES
    "${CSSENG_DIR}/include"
    "${CSSENG_DIR}"
)

list(APPEND select_SYSTEM_INCLUDE_DIRECTORIES
    "${FORWARDING_HEADERS_DIR}/csseng"
)

list(APPEND select_SOURCES
    select.c
)

set(select_LIBRARIES
    CSSEng::CSSEng
)

PURC_EXECUTABLE(select)
PURC_COMPUTE_SOURCES(select)

if (LIBCHECK_FOUND)
    add_subdirectory(wapcaplet)
endif ()

add_subdirectory(parserutils)

