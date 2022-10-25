include(PurCCommon)
include(target/CSSEng)

PURC_EXECUTABLE_DECLARE(examples)

list(APPEND examples_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
)

set(examples_SOURCES
    example1.c
)

set(examples_LIBRARIES
    PurC::CSSEng
)

PURC_EXECUTABLE(examples)

PURC_COMPUTE_SOURCES(examples)
PURC_FRAMEWORK(examples)
