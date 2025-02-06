include(PurCCommon)
include(target/PurC)
include(GoogleTest)

enable_testing()

# test_rwstream
PURC_EXECUTABLE_DECLARE(test_rwstream)

list(APPEND test_rwstream_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
    "${GTEST_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(test_rwstream)

set(test_rwstream_SOURCES
    test_rwstream.cpp
)

set(test_rwstream_LIBRARIES
    PurC::PurC
    "${GTEST_LIBRARIES}"
    pthread
)

PURC_COMPUTE_SOURCES(test_rwstream)
PURC_FRAMEWORK(test_rwstream)
GTEST_DISCOVER_TESTS(test_rwstream DISCOVERY_TIMEOUT 10)

