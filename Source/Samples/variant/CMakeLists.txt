include(PurCCommon)
include(target/PurC)

# make_dynamic_object
PURC_EXECUTABLE_DECLARE(make_dynamic_object)

list(APPEND make_dynamic_object_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
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

# make_array
PURC_EXECUTABLE_DECLARE(make_array)

list(APPEND make_array_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
)

PURC_EXECUTABLE(make_array)

set(make_array_SOURCES
    make_array.c
)

set(make_array_LIBRARIES
    PurC::PurC
    pthread
)

PURC_COMPUTE_SOURCES(make_array)
PURC_FRAMEWORK(make_array)
