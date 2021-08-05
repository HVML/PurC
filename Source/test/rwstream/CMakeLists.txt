include(PurCCommon)
include(target/PurC)
include(GoogleTest)

enable_testing()

# test_rwstream
PURC_EXECUTABLE_DECLARE(test_rwstream)

list(APPEND test_rwstream_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_rwstream)

set(test_rwstream_SOURCES
    test_rwstream.cpp
)

set(test_rwstream_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_rwstream)
PURC_FRAMEWORK(test_rwstream)
GTEST_DISCOVER_TESTS(test_rwstream)

