include(PurCCommon)
include(target/PurC)
include(GoogleTest)

enable_testing()

# test_key
PURC_EXECUTABLE_DECLARE(test_key)

list(APPEND test_key_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/Source/PurC/executors
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_key)

set(test_key_SOURCES
    test-key.cpp
)

set(test_key_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_key)
PURC_FRAMEWORK(test_key)
GTEST_DISCOVER_TESTS(test_key)

# test_range
PURC_EXECUTABLE_DECLARE(test_range)

list(APPEND test_range_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/Source/PurC/executors
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_range)

set(test_range_SOURCES
    test-range.cpp
)

set(test_range_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_range)
PURC_FRAMEWORK(test_range)
GTEST_DISCOVER_TESTS(test_range)

# test_filter
PURC_EXECUTABLE_DECLARE(test_filter)

list(APPEND test_filter_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/Source/PurC/executors
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_filter)

set(test_filter_SOURCES
    test-filter.cpp
)

set(test_filter_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_filter)
PURC_FRAMEWORK(test_filter)
GTEST_DISCOVER_TESTS(test_filter)

