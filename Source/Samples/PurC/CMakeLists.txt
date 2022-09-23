include(PurCCommon)
include(target/PurC)

# make_simple_array
PURC_EXECUTABLE_DECLARE(make_simple_array)

list(APPEND make_simple_array_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
)

PURC_EXECUTABLE(make_simple_array)

set(make_simple_array_SOURCES
    make_simple_array.c
)

set(make_simple_array_LIBRARIES
    PurC::PurC
    pthread
)

PURC_COMPUTE_SOURCES(make_simple_array)
PURC_FRAMEWORK(make_simple_array)

# make_simple_object
PURC_EXECUTABLE_DECLARE(make_simple_object)

list(APPEND make_simple_object_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
)

PURC_EXECUTABLE(make_simple_object)

set(make_simple_object_SOURCES
    make_simple_object.c
)

set(make_simple_object_LIBRARIES
    PurC::PurC
    pthread
)

PURC_COMPUTE_SOURCES(make_simple_object)
PURC_FRAMEWORK(make_simple_object)

# make_dynamic_object
PURC_EXECUTABLE_DECLARE(make_dynamic_object)

list(APPEND make_dynamic_object_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
)

PURC_EXECUTABLE(make_dynamic_object)

set(make_dynamic_object_SOURCES
    make_dynamic_object.c
)

set(make_dynamic_object_LIBRARIES
    PurC::PurC
    pthread
)

PURC_COMPUTE_SOURCES(make_dynamic_object)
PURC_FRAMEWORK(make_dynamic_object)

