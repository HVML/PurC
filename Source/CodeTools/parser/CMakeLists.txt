include(PurCCommon)

PURC_EXECUTABLE_DECLARE(csspropgen)

list(APPEND csspropgen_PRIVATE_INCLUDE_DIRECTORIES
)

list(APPEND csspropgen_SYSTEM_INCLUDE_DIRECTORIES
)

list(APPEND csspropgen_SOURCES
    css_property_parser_gen.c
)

set(csspropgen_LIBRARIES
)

PURC_EXECUTABLE(csspropgen)
PURC_COMPUTE_SOURCES(csspropgen)

