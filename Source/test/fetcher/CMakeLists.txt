include(PurCCommon)
include(target/PurC)
include(GoogleTest)

enable_testing()

# test_local_fetcher
PURC_EXECUTABLE_DECLARE(test_local_fetcher)

list(APPEND test_local_fetcher_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
    "${FORWARDING_HEADERS_DIR}"
    "${GIO_UNIX_INCLUDE_DIRS}"
    "${GLIB_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(test_local_fetcher)

set(test_local_fetcher_SOURCES
    test_local_fetcher.cpp
)

set(test_local_fetcher_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_local_fetcher)
PURC_FRAMEWORK(test_local_fetcher)
GTEST_DISCOVER_TESTS(test_local_fetcher DISCOVERY_TIMEOUT 10)
