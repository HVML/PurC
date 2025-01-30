include(PurCCommon)
include(target/PurC)
include(GoogleTest)

enable_testing()

# test_messages
PURC_EXECUTABLE_DECLARE(test_messages)

list(APPEND test_messages_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
    "${GTEST_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(test_messages)

set(test_messages_SOURCES
    test_messages.cpp
)

set(test_messages_LIBRARIES
    PurC::PurC
    "${GTEST_LIBRARIES}"
    pthread
)

PURC_COMPUTE_SOURCES(test_messages)
PURC_FRAMEWORK(test_messages)
GTEST_DISCOVER_TESTS(test_messages DISCOVERY_TIMEOUT 10)

