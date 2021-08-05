include(PurCCommon)
include(target/PurC)
include(GoogleTest)

enable_testing()

# test_stack
PURC_EXECUTABLE_DECLARE(test_utils)

list(APPEND test_utils_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
)

PURC_EXECUTABLE(test_utils)

set(test_utils_SOURCES
    test_utils.cpp
)

set(test_utils_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_utils)
PURC_FRAMEWORK(test_utils)
GTEST_DISCOVER_TESTS(test_utils)
