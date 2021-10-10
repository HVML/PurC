include(PurCCommon)
include(target/PurC)
include(GoogleTest)

enable_testing()

# test_executor
PURC_EXECUTABLE_DECLARE(test_executor)

list(APPEND test_executor_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_executor)

set(test_executor_SOURCES
    test-executor.cpp
)

set(test_executor_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_executor)
PURC_FRAMEWORK(test_executor)
GTEST_DISCOVER_TESTS(test_executor)


