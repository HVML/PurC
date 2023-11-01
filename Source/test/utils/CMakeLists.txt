include(PurCCommon)
include(target/PurC)
include(GoogleTest)

enable_testing()

# test_utils
PURC_EXECUTABLE_DECLARE(test_utils)

list(APPEND test_utils_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
)

list(APPEND test_utils_SYSTEM_INCLUDE_DIRECTORIES
    ${GLIB_INCLUDE_DIRS}
)

PURC_EXECUTABLE(test_utils)

set(test_utils_SOURCES
    test_utils.cpp
)

set(test_utils_LIBRARIES
    ${GLIB_LIBRARIES}
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_utils)
PURC_FRAMEWORK(test_utils)
GTEST_DISCOVER_TESTS(test_utils DISCOVERY_TIMEOUT 10)

# test_regex
PURC_EXECUTABLE_DECLARE(test_regex)

list(APPEND test_regex_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
)

PURC_EXECUTABLE(test_regex)

set(test_regex_SOURCES
    test_regex.cpp
)

set(test_regex_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_regex)
PURC_FRAMEWORK(test_regex)
GTEST_DISCOVER_TESTS(test_regex DISCOVERY_TIMEOUT 10)

# test_runloop
PURC_EXECUTABLE_DECLARE(test_runloop)

list(APPEND test_runloop_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${GLIB_INCLUDE_DIRS}
)

PURC_EXECUTABLE(test_runloop)

set(test_runloop_SOURCES
    test_runloop.cpp
)

set(test_runloop_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_runloop)
PURC_FRAMEWORK(test_runloop)
GTEST_DISCOVER_TESTS(test_runloop DISCOVERY_TIMEOUT 10)

# test_helpers
PURC_EXECUTABLE_DECLARE(test_helpers)

list(APPEND test_helpers_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${GLIB_INCLUDE_DIRS}
)

PURC_EXECUTABLE(test_helpers)

set(test_helpers_SOURCES
    test_helpers.cpp
)

set(test_helpers_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_helpers)
PURC_FRAMEWORK(test_helpers)
GTEST_DISCOVER_TESTS(test_helpers DISCOVERY_TIMEOUT 10)

