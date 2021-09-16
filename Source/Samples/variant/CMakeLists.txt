include(PurCCommon)
include(target/PurC)

# test_variant
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
