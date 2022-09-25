include(PurCCommon)

# test_aliases
PURC_EXECUTABLE_DECLARE(test_aliases)

list(APPEND test_aliases_PRIVATE_INCLUDE_DIRECTORIES
    "${CSSENG_DIR}"
)

list(APPEND test_aliases_SYSTEM_INCLUDE_DIRECTORIES
    "${FORWARDING_HEADERS_DIR}/csseng"
)

list(APPEND test_aliases_SOURCES
    aliases.c
)

set(test_aliases_LIBRARIES
    PurC::CSSEng
)

PURC_EXECUTABLE(test_aliases)
PURC_COMPUTE_SOURCES(test_aliases)

# cscodecutf8
PURC_EXECUTABLE_DECLARE(cscodecutf8)

list(APPEND cscodecutf8_PRIVATE_INCLUDE_DIRECTORIES
    "${CSSENG_DIR}"
)

list(APPEND cscodecutf8_SYSTEM_INCLUDE_DIRECTORIES
    "${FORWARDING_HEADERS_DIR}/csseng"
)

list(APPEND cscodecutf8_SOURCES
    cscodec-utf8.c
)

set(cscodecutf8_LIBRARIES
    PurC::CSSEng
)

PURC_EXECUTABLE(cscodecutf8)
PURC_COMPUTE_SOURCES(cscodecutf8)

# cscodecutf16
PURC_EXECUTABLE_DECLARE(cscodecutf16)

list(APPEND cscodecutf16_PRIVATE_INCLUDE_DIRECTORIES
    "${CSSENG_DIR}"
)

list(APPEND cscodecutf16_SYSTEM_INCLUDE_DIRECTORIES
    "${FORWARDING_HEADERS_DIR}/csseng"
)

list(APPEND cscodecutf16_SOURCES
    cscodec-utf16.c
)

set(cscodecutf16_LIBRARIES
    PurC::CSSEng
)

PURC_EXECUTABLE(cscodecutf16)
PURC_COMPUTE_SOURCES(cscodecutf16)

# cscodecext8
PURC_EXECUTABLE_DECLARE(cscodecext8)

list(APPEND cscodecext8_PRIVATE_INCLUDE_DIRECTORIES
    "${CSSENG_DIR}"
)

list(APPEND cscodecext8_SYSTEM_INCLUDE_DIRECTORIES
    "${FORWARDING_HEADERS_DIR}/csseng"
)

list(APPEND cscodecext8_SOURCES
    cscodec-ext8.c
)

set(cscodecext8_LIBRARIES
    PurC::CSSEng
)

PURC_EXECUTABLE(cscodecext8)
PURC_COMPUTE_SOURCES(cscodecext8)

# cscodec8859
PURC_EXECUTABLE_DECLARE(cscodec8859)

list(APPEND cscodec8859_PRIVATE_INCLUDE_DIRECTORIES
    "${CSSENG_DIR}"
)

list(APPEND cscodec8859_SYSTEM_INCLUDE_DIRECTORIES
    "${FORWARDING_HEADERS_DIR}/csseng"
)

list(APPEND cscodec8859_SOURCES
    cscodec-8859.c
)

set(cscodec8859_LIBRARIES
    PurC::CSSEng
)

PURC_EXECUTABLE(cscodec8859)
PURC_COMPUTE_SOURCES(cscodec8859)

# test_filter
PURC_EXECUTABLE_DECLARE(test_filter)

list(APPEND test_filter_PRIVATE_INCLUDE_DIRECTORIES
    "${CSSENG_DIR}"
)

list(APPEND test_filter_SYSTEM_INCLUDE_DIRECTORIES
    "${FORWARDING_HEADERS_DIR}/csseng"
)

list(APPEND test_filter_SOURCES
    filter.c
)

set(test_filter_LIBRARIES
    PurC::CSSEng
)

PURC_EXECUTABLE(test_filter)
PURC_COMPUTE_SOURCES(test_filter)

# inputstream
PURC_EXECUTABLE_DECLARE(inputstream)

list(APPEND inputstream_PRIVATE_INCLUDE_DIRECTORIES
    "${CSSENG_DIR}"
)

list(APPEND inputstream_SYSTEM_INCLUDE_DIRECTORIES
    "${FORWARDING_HEADERS_DIR}/csseng"
)

list(APPEND inputstream_SOURCES
    inputstream.c
)

set(inputstream_LIBRARIES
    PurC::CSSEng
)

PURC_EXECUTABLE(inputstream)
PURC_COMPUTE_SOURCES(inputstream)

